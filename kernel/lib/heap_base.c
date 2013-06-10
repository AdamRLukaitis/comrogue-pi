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
#include <comrogue/objectbase.h>
#include <comrogue/scode.h>
#include <comrogue/stdobj.h>
#include <comrogue/mutex.h>
#include <comrogue/internals/mmu.h>
#include "heap_internals.h"

/*-----------------------------------
 * Base memory allocation functions.
 *-----------------------------------
 */

/*
 * Allocate a new chunk of memory for use by the base allocator, and point to it with the allocator pointers.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - szMinimum = Minimum number of bytes we need to allocate.
 *
 * Returns:
 * - TRUE = Allocation failed.
 * - FALSE = Allocation succeeded.
 */
static BOOL base_alloc_new_chunk(PHEAPDATA phd, SIZE_T szMinimum)
{
  SIZE_T szAdjusted;   /* adjusted size to multiple of chunk size */
  PVOID pvNewChunk;    /* pointer to new chunk */
  PVOID pvTemp;        /* temporary pointer */
  BOOL fZero = FALSE;  /* do we need this zeroed? (no) */

  szAdjusted = CHUNK_CEILING(phd, szMinimum);
  pvNewChunk = _HeapChunkAlloc(phd, szAdjusted, phd->szChunk, TRUE, &fZero);
  if (!pvNewChunk)
    return TRUE;  /* allocation failed */
  *((PPVOID)pvNewChunk) = phd->pvBasePages;
  pvTemp = (PVOID)(((UINT_PTR)pvNewChunk) + sizeof(PVOID));
  *((SIZE_T *)pvTemp) = szAdjusted;
  phd->pvBasePages = pvNewChunk;
  phd->pvBaseNext = (PVOID)(((UINT_PTR)pvNewChunk) + sizeof(PVOID) + sizeof(SIZE_T));
  phd->pvBasePast = (PVOID)(((UINT_PTR)pvNewChunk) + szAdjusted);
  return FALSE;
}

/*
 * Allocate a chunk of memory from the base allocator.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - sz = Number of bytes to allocate.
 *
 * Returns:
 * - NULL = Allocation failed.
 * - Other = Pointer to the allocated block of memory.
 */
PVOID _HeapBaseAlloc(PHEAPDATA phd, SIZE_T sz)
{
  PVOID rc = NULL;      /* return from this function */
  SIZE_T szAdjusted;    /* adjusted size */

  szAdjusted = SYS_CACHELINE_CEILING(sz);  /* round up to cache line size */

  IMutex_Lock(phd->pmtxBase);
  if (((UINT_PTR)(phd->pvBaseNext) + szAdjusted) > (UINT_PTR)(phd->pvBasePast))
  { /* allocate new chunk if we don't have enough space for the allocation */
    if (base_alloc_new_chunk(phd, szAdjusted + sizeof(PVOID) + sizeof(SIZE_T)))
      goto error0;  /* failed */
  }

  rc = phd->pvBaseNext;  /* perform the allocation */
  phd->pvBaseNext = (PVOID)((UINT_PTR)(phd->pvBaseNext) + szAdjusted);
 
error0:
  IMutex_Unlock(phd->pmtxBase);
  return rc;
}

/*
 * Allocate a new EXTENT_NODE from the base allocator.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 *
 * Returns:
 * - NULL = Allocation failed.
 * - Other = Pointer to the new EXTENT_NODE.
 */
PEXTENT_NODE _HeapBaseNodeAlloc(PHEAPDATA phd)
{
  PEXTENT_NODE rc = NULL;  /* return from this function */

  IMutex_Lock(phd->pmtxBase);
  if (phd->pexnBaseNodes)
  { /* pull a node off the free list */
    rc = phd->pexnBaseNodes;
    phd->pexnBaseNodes = *((PPEXTENT_NODE)rc);
  }

  IMutex_Unlock(phd->pmtxBase);
  if (!rc)  /* use base allocator to get one */
    rc = (PEXTENT_NODE)_HeapBaseAlloc(phd, sizeof(EXTENT_NODE));
  return rc;
}

/*
 * Returns an EXTENT_NODE to the free list.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pexn = Pointer to the EXTENT_NODE to be freed.
 *
 * Returns:
 * Nothing.
 */
void _HeapBaseNodeDeAlloc(PHEAPDATA phd, PEXTENT_NODE pexn)
{
  /* add it to the free list */
  IMutex_Lock(phd->pmtxBase);
  *((PPEXTENT_NODE)pexn) = phd->pexnBaseNodes;
  phd->pexnBaseNodes = pexn;
  IMutex_Unlock(phd->pmtxBase);
}

/*
 * Set up the base allocator.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 *
 * Returns:
 * Standard HRESULT success/failure indicator.
 */
HRESULT _HeapBaseSetup(PHEAPDATA phd)
{
  HRESULT hr = IMutexFactory_CreateMutex(phd->pMutexFactory, &(phd->pmtxBase));
  if (FAILED(hr))
    return hr;
  phd->pexnBaseNodes = NULL;
  return S_OK;
}

/*
 * Shut down the base allocator.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 *
 * Returns:
 * Nothing.
 */
void _HeapBaseShutdown(PHEAPDATA phd)
{
  PVOID pChunk;      /* the chunk we're working on */
  PVOID pNextChunk;  /* the next chunk to be dealt with */
  PVOID pvTemp;      /* temporary pointer */
  SIZE_T szChunk;    /* size of allocated chunk */

  /* All allocated "base" chunks come straight from the IChunkAllocator, so it's safe to free them this way. */
  pChunk = phd->pvBasePages;
  while (pChunk)
  {
    pNextChunk = *((PPVOID)pChunk);
    pvTemp = (PVOID)(((UINT_PTR)pChunk) + sizeof(PVOID));
    szChunk = *((SIZE_T *)pvTemp);
    IChunkAllocator_FreeChunk(phd->pChunkAllocator, pChunk, szChunk);
    pChunk = pNextChunk;
  }

  IUnknown_Release(phd->pmtxBase);
}
