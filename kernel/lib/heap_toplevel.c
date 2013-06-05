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
#include <comrogue/types.h>
#include <comrogue/str.h>
#include <comrogue/objhelp.h>
#include <comrogue/stdobj.h>
#include <comrogue/allocator.h>
#include <comrogue/heap.h>
#include <comrogue/internals/seg.h>
#include "heap_internals.h"
#include "enumgeneric.h"

#define PHDFLAGS_DELETING   0x80000000           /* deleting the heap */

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
    ObjHlpFixedCpTeardown(&(phd->fcpMallocSpy));
    ObjHlpFixedCpTeardown(&(phd->fcpSequentialStream));
    IUnknown_Release(phd->pChunkAllocator);
    if (phd->pfnFreeRawHeapData)
    {
      pfnFree = phd->pfnFreeRawHeapData;
      (*pfnFree)((PRAWHEAPDATA)phd);
    }
  }
  return rc;
}

static PVOID malloc_Alloc(IMalloc *pThis, SIZE_T cb)
{
  return NULL; /* TODO */
}

static PVOID malloc_Realloc(IMalloc *pThis, PVOID pv, SIZE_T cb)
{
  return NULL; /* TODO */
}

static void malloc_Free(IMalloc *pThis, PVOID pv)
{
  /* TODO */
}

static SIZE_T malloc_GetSize(IMalloc *pThis, PVOID pv)
{
  return -1; /* TODO */
}

static INT32 malloc_DidAlloc(IMalloc *pThis, PVOID pv)
{
  return -1; /* TODO */
}

static void malloc_HeapMinimize(IMalloc *pThis)
{
  /* TODO */
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

/*------------------------
 * Heap creation function
 *------------------------
 */

/*
 * Creates a heap implementation and returns a pointer to its IMalloc interface.
 *
 * Parameters:
 * - prhd = Pointer to a RAWHEAPDATA structure, which contains sufficient memory to hold the global data
 *          for the heap.
 * - pfnFree = Pointer to a function called as the last stage of releasing the heap, which frees the
 *             "prhd" block.  May be NULL.
 * - pChunkAllocator = Pointer to the IChunkAllocator interface used by the heap to allocate chunks of memory
 *                     for carving up by the heap.
 * - ppHeap = Pointer location that will receive a pointer to the heap's IMalloc interface.
 *
 * Returns:
 * Standard HRESULT success/failure.
 */
HRESULT HeapCreate(PRAWHEAPDATA prhd, PFNRAWHEAPDATAFREE pfnFree, IChunkAllocator *pChunkAllocator,
		   IMalloc **ppHeap)
{
  PHEAPDATA phd;   /* pointer to actual heap data */

  if (sizeof(RAWHEAPDATA) < sizeof(HEAPDATA))
    return MEMMGR_E_BADHEAPDATASIZE;  /* bogus size of raw heap data */
  if (!prhd || !pChunkAllocator || !ppHeap)
    return E_POINTER;      /* invalid pointers */

  /* initialize heap data */
  phd = (PHEAPDATA)prhd;
  StrSetMem(phd, 0, sizeof(HEAPDATA));
  phd->mallocInterface.pVTable = &vtblMalloc;
  phd->cpContainerInterface.pVTable = &vtblConnectionPointContainer;
  phd->uiRefCount = 1;
  phd->uiFlags = 0;
  phd->pfnFreeRawHeapData = pfnFree;
  phd->pChunkAllocator = pChunkAllocator;
  IUnknown_AddRef(phd->pChunkAllocator);
  ObjHlpFixedCpSetup(&(phd->fcpMallocSpy), (PUNKNOWN)phd, &IID_IMallocSpy, (IUnknown **)(&(phd->pMallocSpy)), 1, NULL);
  ObjHlpFixedCpSetup(&(phd->fcpSequentialStream), (PUNKNOWN)phd, &IID_ISequentialStream,
		     (IUnknown **)(&(phd->pDebugStream)), 1, NULL);

  *ppHeap = (IMalloc *)phd;
  return S_OK;
}
