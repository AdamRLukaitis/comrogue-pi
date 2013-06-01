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
#include <comrogue/allocator.h>
#include <comrogue/heap.h>
#include <comrogue/internals/seg.h>
#include "heap_internals.h"

/*------------------------
 * IMalloc implementation
 *------------------------
 */

static UINT32 IMalloc_AddRef(IUnknown *pThis)
{
  return ++(((PHEAPDATA)pThis)->uiRefCount);
}

static UINT32 IMalloc_Release(IUnknown *pThis)
{
  PHEAPDATA phd = (PHEAPDATA)pThis;  /* pointer to heap data */
  UINT32 rc;                         /* return from this function */
  PFNRAWHEAPDATAFREE pfnFree;        /* pointer to "free" function */

  rc = --(phd->uiRefCount);
  if (rc == 0)
  {
    IUnknown_Release(phd->pChunkAllocator);
    if (phd->pfnFreeRawHeapData)
    {
      pfnFree = phd->pfnFreeRawHeapData;
      (*pfnFree)((PRAWHEAPDATA)phd);
    }
  }
  return rc;
}

/* The IMalloc vtable. */
static const SEG_RODATA struct IMallocVTable vtblMalloc =
{
  .AddRef = IMalloc_AddRef,
  .Release = IMalloc_Release
};

/*------------------------------------------
 * IConnectionPointContainer implementation
 *------------------------------------------
 */

/* Quick macro to get the PHEAPDATA from the IConnectionPointContainer pointer */
#define HeapDataPtr(pcpc)    (((PBYTE)(pcpc)) - OFFSETOF(HEAPDATA, cpContainerInterface))

static UINT32 IConnectionPointContainer_AddRef(IUnknown *pThis)
{
  return IMalloc_AddRef((IUnknown *)HeapDataPtr(pThis));
}

static UINT32 IConnectionPointContainer_Release(IUnknown *pThis)
{
  return IMalloc_Release((IUnknown *)HeapDataPtr(pThis));
}

/* The IConnectionPointContainer vtable. */
static const SEG_RODATA struct IConnectionPointContainerVTable vtblConnectionPointContainer =
{
  .AddRef = IConnectionPointContainer_AddRef,
  .Release = IConnectionPointContainer_Release
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
  phd->pfnFreeRawHeapData = pfnFree;
  phd->pChunkAllocator = pChunkAllocator;
  IUnknown_AddRef(phd->pChunkAllocator);

  *ppHeap = (IMalloc *)phd;
  return S_OK;
}
