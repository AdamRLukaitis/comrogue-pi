/*
 * This file is part of the COMROGUE Operating System for Raspberry Pi
 *
 * Copyright (c) 2013, Eric J. Bowersox / Erbosoft Enterprises
 * All rights reserved.
 *
 * This program is free for commercial and non-commercial use as long as the following conditions are
 * adhered to.
 *
 * Copyright in this file remains Eric J. Bowersox and/or Erbosoft, and as such any copyright notices
 * in the code are not to be removed.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 * 
 * * Redistributions of source code must retain the above copyright notice, this list of conditions and
 *   the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *   the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * "Raspberry Pi" is a trademark of the Raspberry Pi Foundation.
 */
/*
 * This code is based on/inspired by jemalloc-3.3.1.  Please see LICENSE.jemalloc for further details.
 */
#include <comrogue/compiler_macros.h>
#include <comrogue/types.h>
#include <comrogue/scode.h>
#include <comrogue/str.h>
#include <comrogue/objhelp.h>
#include <comrogue/stdobj.h>
#include <comrogue/allocator.h>
#include <comrogue/heap.h>
#include <comrogue/internals/seg.h>
#include <comrogue/internals/mmu.h>
#include "heap_internals.h"
#include "enumgeneric.h"

#define PHDFLAGS_DELETING        0x80000000           /* deleting the heap */
#define PHDFLAGS_PROFILE_ACTIVE  0x40000000           /* profile is active */

#define PHDFLAGS_INIT (PHDFLAGS_REDZONE|PHDFLAGS_JUNKFILL|PHDFLAGS_ZEROFILL|PHDFLAGS_NOTCACHE|PHDFLAGS_PROFILE)

/*------------------------
 * IMalloc implementation
 *------------------------
 */

/*
 * Queries for an interface on the heap object.
 *
 * Parameters:
 * - pThis = Pointer to the heap data object (actually its IMalloc interface pointer).
 * - riid = Reference to the IID of the interface we want to load.
 * - ppvObject = Pointer to the location to receive the new interface pointer.
 *
 * Returns:
 * Standard HRESULT success/failure indicator:
 * - S_OK = New interface pointer was returned.
 * - E_NOINTERFACE = The object does not support this interface.
 * - E_POINTER = The ppvObject pointer is not valid.
 */
static HRESULT malloc_QueryInterface(IUnknown *pThis, REFIID riid, PPVOID ppvObject)
{
  if (!ppvObject)
    return E_POINTER;
  *ppvObject = NULL;
  if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IMalloc))
    *ppvObject = pThis;
  else if (IsEqualIID(riid, &IID_IConnectionPointContainer))
    *ppvObject = &(((PHEAPDATA)pThis)->cpContainerInterface);
  else if (IsEqualIID(riid, &IID_IHeapConfiguration))
    *ppvObject = &(((PHEAPDATA)pThis)->heapConfInterface);
  else
    return E_NOINTERFACE;
  IUnknown_AddRef((IUnknown *)(*ppvObject));
  return S_OK;
}

/*
 * Adds a reference to the heap data object.
 *
 * Parameters:
 * - pThis = Pointer to the heap data object (actually its IMalloc interface pointer).
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 malloc_AddRef(IUnknown *pThis)
{
  return ++(((PHEAPDATA)pThis)->uiRefCount);
}

/*
 * Shuts down everything that was allocated at top level.
 *
 * Parameters:
 * - phd = Pointer to HEAPDATA structure.
 *
 * Returns:
 * Nothing.
 */
static void toplevel_shutdown(PHEAPDATA phd)
{
    ObjHlpFixedCpTeardown(&(phd->fcpMallocSpy));
    ObjHlpFixedCpTeardown(&(phd->fcpSequentialStream));
    IUnknown_Release(phd->pMutexFactory);
    IUnknown_Release(phd->pChunkAllocator);
}

/*
 * Removes a reference from the heap data object.  The object is freed when its reference count reaches 0.
 *
 * Parameters:
 * - pThis = Pointer to the heap data object (actually its IMalloc interface pointer).
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 malloc_Release(IUnknown *pThis)
{
  PHEAPDATA phd = (PHEAPDATA)pThis;  /* pointer to heap data */
  UINT32 rc;                         /* return from this function */
  PFNRAWHEAPDATAFREE pfnFree;        /* pointer to "free" function */

  rc = --(phd->uiRefCount);
  if ((rc == 0) && !(phd->uiFlags & PHDFLAGS_DELETING))
  {
    phd->uiFlags |= PHDFLAGS_DELETING;
    /* Do subsystem shutdown. */
    _HeapChunkShutdown(phd);
    _HeapBaseShutdown(phd);
    toplevel_shutdown(phd);
    if (phd->pfnFreeRawHeapData)
    { /* free the raw heap data */
      pfnFree = phd->pfnFreeRawHeapData;
      (*pfnFree)((PRAWHEAPDATA)phd);
    }
  }
  return rc;
}

static PVOID malloc_Alloc(IMalloc *pThis, SIZE_T cb)
{
  PHEAPDATA phd = (PHEAPDATA)pThis;  /* pointer to heap data */
  SIZE_T cbActual = cb;              /* actual number of bytes requested */
  PVOID rc;                          /* return from this function */

  /* handle PreAlloc call */
  if (phd->pMallocSpy)
  {
    cbActual = IMallocSpy_PreAlloc(phd->pMallocSpy, cb);
    if ((cbActual == 0) && (cb != 0))
      return NULL;  /* simulated memory failure */
  }

  if (cbActual == 0)
    cbActual = 1;  /* allocate at least SOMETHING */

  rc = NULL; /* TODO */

  /* handle PostAlloc call */
  if (phd->pMallocSpy)
    rc = IMallocSpy_PostAlloc(phd->pMallocSpy, rc);

  return rc;
}

static BOOL isSpyedByCurrentSpy(PHEAPDATA phd, PVOID pBlock)
{
  return FALSE; /* TODO */
}

static PVOID malloc_Realloc(IMalloc *pThis, PVOID pv, SIZE_T cb)
{
  PHEAPDATA phd = (PHEAPDATA)pThis;  /* pointer to heap data */
  PVOID pvActual = pv;               /* actual heap block pointer */
  SIZE_T cbActual = cb;              /* actual number of bytes requested */
  PVOID rc;                          /* return from this function */
  BOOL fSpyed;                       /* were we allocated while currently spyed on? */

  /* handle PreRealloc call */
  if (phd->pMallocSpy)
  {
    fSpyed = isSpyedByCurrentSpy(phd, pv);
    cbActual = IMallocSpy_PreRealloc(phd->pMallocSpy, pv, cb, &pvActual, fSpyed);
    if ((cbActual == 0) && (cb != 0))
      return NULL; /* simulated memory failure */
  }

  rc = NULL; /* TODO */

  /* handle PostRealloc call */
  if (phd->pMallocSpy)
    rc = IMallocSpy_PostRealloc(phd->pMallocSpy, rc, fSpyed);

  return rc;
}

static void malloc_Free(IMalloc *pThis, PVOID pv)
{
  PHEAPDATA phd = (PHEAPDATA)pThis;  /* pointer to heap data */
  PVOID pvActual = pv;               /* actual heap block pointer */
  BOOL fSpyed;                       /* were we allocated while currently spyed on? */

  if (!pv)
    return;  /* no effect if pointer is NULL */

  /* handle PreFree call */
  if (phd->pMallocSpy)
  {
    fSpyed = isSpyedByCurrentSpy(phd, pv);
    pvActual = IMallocSpy_PreFree(phd->pMallocSpy, pv, fSpyed);
  }

  /* TODO */
  UNUSED(pvActual);

  /* handle PostFree call */
  if (phd->pMallocSpy)
    IMallocSpy_PostFree(phd->pMallocSpy, fSpyed);
}

static SIZE_T malloc_GetSize(IMalloc *pThis, PVOID pv)
{
  PHEAPDATA phd = (PHEAPDATA)pThis;  /* pointer to heap data */
  PVOID pvActual = pv;               /* actual heap block pointer */
  BOOL fSpyed;                       /* were we allocated while currently spyed on? */
  SIZE_T rc;                         /* return from this function */

  /* handle PreGetSize call */
  if (phd->pMallocSpy)
  {
    fSpyed = isSpyedByCurrentSpy(phd, pv);
    pvActual = IMallocSpy_PreGetSize(phd->pMallocSpy, pv, fSpyed);
  }

  rc = -1;  /* TODO */
  UNUSED(pvActual);

  /* handle PostGetSize call */
  if (phd->pMallocSpy)
    rc = IMallocSpy_PostGetSize(phd->pMallocSpy, rc, fSpyed);

  return rc;
}

static INT32 malloc_DidAlloc(IMalloc *pThis, PVOID pv)
{
  PHEAPDATA phd = (PHEAPDATA)pThis;  /* pointer to heap data */
  PVOID pvActual = pv;               /* actual heap block pointer */
  BOOL fSpyed;                       /* were we allocated while currently spyed on? */
  INT32 rc;                          /* return from this function */

  /* handle PreDidAlloc call */
  if (phd->pMallocSpy)
  {
    fSpyed = isSpyedByCurrentSpy(phd, pv);
    pvActual = IMallocSpy_PreDidAlloc(phd->pMallocSpy, pv, fSpyed);
  }

  rc = -1; /* TODO */
  UNUSED(pvActual);

  /* handle PostDidAlloc call */
  if (phd->pMallocSpy)
    rc = IMallocSpy_PostDidAlloc(phd->pMallocSpy, pv, fSpyed, rc);

  return rc;
}

static void malloc_HeapMinimize(IMalloc *pThis)
{
  PHEAPDATA phd = (PHEAPDATA)pThis;  /* pointer to heap data */

  /* handle PreHeapMinimize call */
  if (phd->pMallocSpy)
    IMallocSpy_PreHeapMinimize(phd->pMallocSpy);

  /* TODO */

  /* handle PostHeapMinimize call */
  if (phd->pMallocSpy)
    IMallocSpy_PostHeapMinimize(phd->pMallocSpy);
}

/* The IMalloc vtable. */
static const SEG_RODATA struct IMallocVTable vtblMalloc =
{
  .QueryInterface = malloc_QueryInterface,
  .AddRef = malloc_AddRef,
  .Release = malloc_Release,
  .Alloc = malloc_Alloc,
  .Realloc = malloc_Realloc,
  .Free = malloc_Free,
  .GetSize = malloc_GetSize,
  .DidAlloc = malloc_DidAlloc,
  .HeapMinimize = malloc_HeapMinimize
};

/*------------------------------------------
 * IConnectionPointContainer implementation
 *------------------------------------------
 */

/* Quick macro to get the PHEAPDATA from the IConnectionPointContainer pointer */
#define HeapDataPtr(pcpc)    (((PBYTE)(pcpc)) - OFFSETOF(HEAPDATA, cpContainerInterface))

/*
 * Queries for an interface on the heap object.
 *
 * Parameters:
 * - pThis = Pointer to the ConnectionPointContainer interface in the heap data object.
 * - riid = Reference to the IID of the interface we want to load.
 * - ppvObject = Pointer to the location to receive the new interface pointer.
 *
 * Returns:
 * Standard HRESULT success/failure indicator:
 * - S_OK = New interface pointer was returned.
 * - E_NOINTERFACE = The object does not support this interface.
 * - E_POINTER = The ppvObject pointer is not valid.
 */
static HRESULT cpc_QueryInterface(IUnknown *pThis, REFIID riid, PPVOID ppvObject)
{
  return malloc_QueryInterface((IUnknown *)HeapDataPtr(pThis), riid, ppvObject);
}

/*
 * Adds a reference to the heap data object.
 *
 * Parameters:
 * - pThis = Pointer to the ConnectionPointContainer interface in the heap data object.
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 cpc_AddRef(IUnknown *pThis)
{
  return malloc_AddRef((IUnknown *)HeapDataPtr(pThis));
}

/*
 * Removes a reference from the heap data object.  The object is freed when its reference count reaches 0.
 *
 * Parameters:
 * - pThis = Pointer to the ConnectionPointContainer interface in the heap data object.
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 cpc_Release(IUnknown *pThis)
{
  return malloc_Release((IUnknown *)HeapDataPtr(pThis));
}

/*
 * Creates an enumerator object to iterate through all the connection points in this container.
 *
 * Parameters:
 * - pThis = Pointer to the ConnectionPointContainer interface in the heap data object.
 * - ppEnum = Pointer to a variable to receive a new reference to IEnumConnectionPoints.
 *
 * Returns:
 * - S_OK = Enumerator created successfully; pointer to it in *ppEnum.
 * - E_OUTOFMEMORY = Unable to allocate the enumerator; *ppEnum is NULL.
 * - E_POINTER = The ppEnum pointer is not valid.
 */
static HRESULT cpc_EnumConnectionPoints(IConnectionPointContainer *pThis, IEnumConnectionPoints **ppEnum)
{
  PHEAPDATA phd = (PHEAPDATA)HeapDataPtr(pThis);  /* pointer to heap data */
  PENUMGENERICDATA pegd;     /* pointer to data block */
  PENUMGENERIC peg;          /* pointer to enumerator */

  if (!ppEnum)
    return E_POINTER;
  *ppEnum = NULL;

  /* Allocate the data block. */
  pegd = _ObjHlpAllocateEnumGenericData((IMalloc *)phd, &IID_IConnectionPoint, 2);
  if (!pegd)
    return E_OUTOFMEMORY;

  /* Build the data block. */
  _ObjHlpAddToEnumGenericData(pegd, (IUnknown *)(&(phd->fcpMallocSpy)));
  _ObjHlpAddToEnumGenericData(pegd, (IUnknown *)(&(phd->fcpSequentialStream)));

  /* Allocate the enumerator. */
  peg = _ObjHlpAllocateEnumGeneric((IMalloc *)phd, &IID_IEnumConnectionPoints, pegd, 0);
  if (peg)
  {
    IUnknown_QueryInterface((IUnknown *)peg, &IID_IEnumConnectionPoints, (PPVOID)ppEnum);
    return S_OK;
  }

  /* Enumerator allocation failed?!? */
  _ObjHlpDiscardEnumGenericData(pegd);
  malloc_Free((IMalloc *)phd, pegd);
  return E_OUTOFMEMORY;
}

/*
 * Returns a reference to the IConnectionPoint interface corresponding to a specific outgoing IID.
 *
 * Parameters:
 * - pThis = Pointer to the ConnectionPointContainer interface in the heap data object.
 * - riid = IID of the outgoing interface to get the connection point for.
 * - ppCP = Pointer to a variable to receive the IConnectionPoint interface pointer.
 *
 * Returns:
 * - S_OK = Found connection point; interface pointer is in *ppCP.
 * - E_POINTER = The ppCP pointer is not valid.
 * - CONNECT_E_NOCONNECTION = The object does not support the interface indicated by riid. *ppCP is NULL.
 */
static HRESULT cpc_FindConnectionPoint(IConnectionPointContainer *pThis, REFIID riid, IConnectionPoint **ppCP)
{
  PHEAPDATA phd = (PHEAPDATA)HeapDataPtr(pThis);  /* pointer to heap data */

  if (!ppCP)
    return E_POINTER;
  *ppCP = NULL;
  if (IsEqualIID(riid, &IID_IMallocSpy))
    *ppCP = (IConnectionPoint *)(&(phd->fcpMallocSpy));
  else if (IsEqualIID(riid, &IID_ISequentialStream))
    *ppCP = (IConnectionPoint *)(&(phd->fcpSequentialStream));
  else
    return CONNECT_E_NOCONNECTION;
  IUnknown_AddRef(*ppCP);
  return S_OK;
}

/* The IConnectionPointContainer vtable. */
static const SEG_RODATA struct IConnectionPointContainerVTable vtblConnectionPointContainer =
{
  .QueryInterface = cpc_QueryInterface,
  .AddRef = cpc_AddRef,
  .Release = cpc_Release,
  .EnumConnectionPoints = cpc_EnumConnectionPoints,
  .FindConnectionPoint = cpc_FindConnectionPoint
};

/*-----------------------------------
 * IHeapConfiguration implementation
 *-----------------------------------
 */

/* Quick macro to get the PHEAPDATA from the IHeapConfiguration pointer */
#undef HeapDataPtr
#define HeapDataPtr(pcpc)    (((PBYTE)(pcpc)) - OFFSETOF(HEAPDATA, heapConfInterface))

/*
 * Queries for an interface on the heap object.
 *
 * Parameters:
 * - pThis = Pointer to the HeapConfiguration interface in the heap data object.
 * - riid = Reference to the IID of the interface we want to load.
 * - ppvObject = Pointer to the location to receive the new interface pointer.
 *
 * Returns:
 * Standard HRESULT success/failure indicator:
 * - S_OK = New interface pointer was returned.
 * - E_NOINTERFACE = The object does not support this interface.
 * - E_POINTER = The ppvObject pointer is not valid.
 */
static HRESULT heapconf_QueryInterface(IUnknown *pThis, REFIID riid, PPVOID ppvObject)
{
  return malloc_QueryInterface((IUnknown *)HeapDataPtr(pThis), riid, ppvObject);
}

/*
 * Adds a reference to the heap data object.
 *
 * Parameters:
 * - pThis = Pointer to the HeapConfiguration interface in the heap data object.
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 heapconf_AddRef(IUnknown *pThis)
{
  return malloc_AddRef((IUnknown *)HeapDataPtr(pThis));
}

/*
 * Removes a reference from the heap data object.  The object is freed when its reference count reaches 0.
 *
 * Parameters:
 * - pThis = Pointer to the HeapConfiguration interface in the heap data object.
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 heapconf_Release(IUnknown *pThis)
{
  return malloc_Release((IUnknown *)HeapDataPtr(pThis));
}

/*
 * Sets the function to call if the heap decides to abort operation.
 *
 * Parameters:
 * - pThis = Pointer to the HeapConfiguration interface in the heap data object.
 * - pfnHeapAbort = Pointer to the function to call when the heap aborts.
 * - pvArg = Pointer to argument to pass to the heap abort function.
 *
 * Returns:
 * Standard HRESULT success/failure indicator.
 */
static HRESULT heapconf_SetAbortProc(IHeapConfiguration *pThis, PFNHEAPABORT pfnHeapAbort, PVOID pvArg)
{
  PHEAPDATA phd = (PHEAPDATA)HeapDataPtr(pThis);  /* pointer to heap data */
  phd->pfnAbort = pfnHeapAbort;
  phd->pvAbortArg = pvArg;
  return S_OK;
}

/*
 * Retrieves the current ratio of active to dirty pages in the heap, expressed as a base-2 logarithm value.
 * A value of -1 disables dirty page purging.
 *
 * Parameters:
 * - pThis = Pointer to the HeapConfiguration interface in the heap data object.
 * - pcbRatio = Pointer to location to receive the current ratio value.
 *
 * Returns:
 * - S_OK = Retrieved the value successfully.
 * - E_POINTER = Invalid pointer for the pcbRatio object.
 */
static HRESULT heapconf_GetActiveDirtyRatio(IHeapConfiguration *pThis, SSIZE_T *pcbRatio)
{
  PHEAPDATA phd = (PHEAPDATA)HeapDataPtr(pThis);  /* pointer to heap data */
  if (!pcbRatio)
    return E_POINTER;
  *pcbRatio = phd->cbActiveDirtyRatio;
  return S_OK;
}

/*
 * Sets the current ratio of active to dirty pages in the heap, expressed as a base-2 logarithm value.
 * A value of -1 disables dirty page purging.
 *
 * Parameters:
 * - pThis = Pointer to the HeapConfiguration interface in the heap data object.
 * - cbRatio = The new ratio value.
 *
 * Returns:
 * - S_OK = Set the value successfully.
 * - E_INVALIDARG = Invalid value for the ratio object.
 */
static HRESULT heapconf_SetActiveDirtyRatio(IHeapConfiguration *pThis, SSIZE_T cbRatio)
{
  PHEAPDATA phd = (PHEAPDATA)HeapDataPtr(pThis);  /* pointer to heap data */
  if ((cbRatio < -1) || (cbRatio > ((sizeof(SIZE_T) << 3) - 1)))
    return E_INVALIDARG;
  phd->cbActiveDirtyRatio = cbRatio;
  return S_OK;
}

/* The IHeapConfiguration vtable. */
static const SEG_RODATA struct IHeapConfigurationVTable vtblHeapConfiguration =
{
  .QueryInterface = heapconf_QueryInterface,
  .AddRef = heapconf_AddRef,
  .Release = heapconf_Release,
  .SetAbortProc = heapconf_SetAbortProc,
  .GetActiveDirtyRatio = heapconf_GetActiveDirtyRatio,
  .SetActiveDirtyRatio = heapconf_SetActiveDirtyRatio
};

/*------------------------
 * Heap creation function
 *------------------------
 */

#define DEFAULT_CBACTIVEDIRTYRATIO 3

/*
 * Creates a heap implementation and returns a pointer to its IMalloc interface.
 *
 * Parameters:
 * - prhd = Pointer to a RAWHEAPDATA structure, which contains sufficient memory to hold the global data
 *          for the heap.
 * - pfnFree = Pointer to a function called as the last stage of releasing the heap, which frees the
 *             "prhd" block.  May be NULL.
 * - uiFlags = Flag bits for the heap.
 * - nChunkBits = Number of "bits" in a memory chunk that gets allocated.
 * - pChunkAllocator = Pointer to the IChunkAllocator interface used by the heap to allocate chunks of memory
 *                     for carving up by the heap.
 * - pMutexFactory = Pointer to the IMutexFactory interface used to allocate IMutex objects.
 * - ppHeap = Pointer location that will receive a pointer to the heap's IMalloc interface.
 *
 * Returns:
 * Standard HRESULT success/failure.
 */
HRESULT HeapCreate(PRAWHEAPDATA prhd, PFNRAWHEAPDATAFREE pfnFree, UINT32 uiFlags, UINT32 nChunkBits, 
		   IChunkAllocator *pChunkAllocator, IMutexFactory *pMutexFactory, IMalloc **ppHeap)
{
  PHEAPDATA phd;   /* pointer to actual heap data */
  HRESULT hr;      /* HRESULT of intermediate operations */

  if (sizeof(RAWHEAPDATA) < sizeof(HEAPDATA))
    return MEMMGR_E_BADHEAPDATASIZE;  /* bogus size of raw heap data */
  if (uiFlags & ~PHDFLAGS_INIT)
    return E_INVALIDARG;   /* invalid flags */
  if (!prhd || !pChunkAllocator || !ppHeap)
    return E_POINTER;      /* invalid pointers */

  /* initialize heap data */
  phd = (PHEAPDATA)prhd;
  StrSetMem(phd, 0, sizeof(HEAPDATA));
  phd->mallocInterface.pVTable = &vtblMalloc;
  phd->cpContainerInterface.pVTable = &vtblConnectionPointContainer;
  phd->heapConfInterface.pVTable = &vtblHeapConfiguration;
  phd->uiRefCount = 1;
  phd->uiFlags = uiFlags | PHDFLAGS_PROFILE_ACTIVE;
  phd->pfnFreeRawHeapData = pfnFree;
  phd->nChunkBits = nChunkBits;
  phd->szChunk = 1 << nChunkBits;
  if (phd->szChunk < SYS_PAGE_SIZE)
    return MEMMGR_E_BADCHUNKSIZE;
  phd->uiChunkSizeMask = phd->szChunk - 1;
  phd->cpgChunk = phd->szChunk >> SYS_PAGE_BITS;
  phd->cbActiveDirtyRatio = DEFAULT_CBACTIVEDIRTYRATIO;

  /* Set up the top-level data. */
  phd->pChunkAllocator = pChunkAllocator;
  IUnknown_AddRef(phd->pChunkAllocator);
  phd->pMutexFactory = pMutexFactory;
  IUnknown_AddRef(phd->pMutexFactory);
  ObjHlpFixedCpSetup(&(phd->fcpMallocSpy), (PUNKNOWN)phd, &IID_IMallocSpy, (IUnknown **)(&(phd->pMallocSpy)), 1, NULL);
  ObjHlpFixedCpSetup(&(phd->fcpSequentialStream), (PUNKNOWN)phd, &IID_ISequentialStream,
		     (IUnknown **)(&(phd->pDebugStream)), 1, NULL);

  /* Setup all the various "subsystems." */
  hr = _HeapBaseSetup(phd);
  if (FAILED(hr))
    goto error0;
  hr = _HeapChunkSetup(phd);
  if (FAILED(hr))
    goto error1;




  *ppHeap = (IMalloc *)phd;
  return S_OK;

  /*error2:*/
  _HeapChunkShutdown(phd);
error1:
  _HeapBaseShutdown(phd);
error0:
  toplevel_shutdown(phd);
  *ppHeap = NULL;
  return hr;
}
