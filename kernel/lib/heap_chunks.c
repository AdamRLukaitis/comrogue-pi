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
#include <comrogue/str.h>
#include <comrogue/objectbase.h>
#include <comrogue/scode.h>
#include <comrogue/stdobj.h>
#include <comrogue/internals/mmu.h>
#include "heap_internals.h"

#ifdef _H_THIS_FILE
#undef _H_THIS_FILE
_DECLARE_H_THIS_FILE
#endif

/*------------------------------------------------------------------------
 * Functions driving the red-black trees that contain EXTENT_NODE objects
 *------------------------------------------------------------------------
 */

/*
 * Compare two extent nodes, first by size, then by address.
 *
 * Parameters:
 * - pexnA = First extent node to compare.
 * - pexnB = Second extent node to compare.
 *
 * Returns:
 * A value less than, equal to, or greater than 0 as pexnA is less than, equal to, or greater than pexnB.
 */
static INT32 compare_sizeaddr(PEXTENT_NODE pexnA, PEXTENT_NODE pexnB)
{
  /* compare sizes first */
  SIZE_T szA = pexnA->sz;
  SIZE_T szB = pexnB->sz;
  INT32 rc = (szA > szB) - (szA < szB);

  if (rc == 0)
  { /* compare addresses */
    UINT_PTR addrA = (UINT_PTR)(pexnA->pv);
    UINT_PTR addrB = (UINT_PTR)(pexnB->pv);
    rc = (addrA > addrB) - (addrA < addrB);
  }
  return rc;
}

/*
 * Compare two extent nodes by address.
 *
 * Parameters:
 * - pexnA = First extent node to compare.
 * - pexnB = Second extent node to compare.
 *
 * Returns:
 * A value less than, equal to, or greater than 0 as pexnA is less than, equal to, or greater than pexnB.
 */
static INT32 compare_addr(PEXTENT_NODE pexnA, PEXTENT_NODE pexnB)
{
  UINT_PTR addrA = (UINT_PTR)(pexnA->pv);
  UINT_PTR addrB = (UINT_PTR)(pexnB->pv);
  return (addrA > addrB) - (addrA < addrB);
}

/*
 * Given a pointer to an extent node, returns a pointer to its RBTREENODE for the size-address tree.
 *
 * Parameters:
 * - pexn = Pointer to the extent node.
 *
 * Returns:
 * Pointer to the extent node's "size-address" RBTREENODE.
 */
static PRBTREENODE get_sizeaddr_node(PEXTENT_NODE pexn)
{
  return &(pexn->rbtnSizeAddress);
}

/*
 * Given a pointer to an extent node, returns a pointer to its RBTREENODE for the address tree.
 *
 * Parameters:
 * - pexn = Pointer to the extent node.
 *
 * Returns:
 * Pointer to the extent node's "address" RBTREENODE.
 */
static PRBTREENODE get_addr_node(PEXTENT_NODE pexn)
{
  return &(pexn->rbtnAddress);
}

/*
 * Given a pointer to an extent node's "size-address" RBTREENODE, returns a pointer to the extent node.
 *
 * Parameters:
 * - prbtn = Pointer to the "size-address" RBTREENODE.
 *
 * Returns:
 * Pointer to the base extent node.
 */
static PEXTENT_NODE get_from_sizeaddr_node(PRBTREENODE prbtn)
{
  return (PEXTENT_NODE)(((UINT_PTR)prbtn) - OFFSETOF(EXTENT_NODE, rbtnSizeAddress));
}

/*
 * Given a pointer to an extent node's "address" RBTREENODE, returns a pointer to the extent node.
 *
 * Parameters:
 * - prbtn = Pointer to the "address" RBTREENODE.
 *
 * Returns:
 * Pointer to the base extent node.
 */
static PEXTENT_NODE get_from_addr_node(PRBTREENODE prbtn)
{
  return (PEXTENT_NODE)(((UINT_PTR)prbtn) - OFFSETOF(EXTENT_NODE, rbtnAddress));
}

/*----------------------
 * Heap chunk functions
 *----------------------
 */

/*
 * Looks for a chunk in the "free extents" list that can be used to satisfy an allocation, and returns it
 * if one is found.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - sz = Size of the chunk to be allocated.
 * - szAlignment = Specifies the alignment that the allocated chunk should have.
 * - pfZeroed = Points to a flag which indicates if the allocated chunk should be zeroed. If the actual
 *              chunk we find is zeroed, this gets toggled to TRUE.
 *
 * Returns:
 * - NULL = A recycled chunk could not be found.
 * - Other = Pointer to the recycled chunk.
 */
static PVOID chunk_recycle(PHEAPDATA phd, SIZE_T sz, SIZE_T szAlignment, BOOL *pfZeroed)
{
  PVOID rc;             /* return from this function */
  SIZE_T szAlloc;       /* allocation size to look for */
  SIZE_T szLeading;     /* leading size of the chunk that can be returned */
  SIZE_T szTrailing;    /* trailing size of the chunk that can be returned */
  BOOL fZeroed;         /* is this block zeroed? */
  PEXTENT_NODE pexn;    /* pointer to extent node we find */
  EXTENT_NODE exnKey;   /* key for tree search */

  /* Look for a big enough block in the tree. */
  szAlloc = sz + szAlignment - phd->szChunk;
  if (szAlloc < sz)
    return NULL;  /* the allocator wrapped around */
  exnKey.pv = NULL;
  exnKey.sz = szAlloc;
  IMutex_Lock(phd->pmtxChunks);
  pexn = (PEXTENT_NODE)RbtFindSuccessor(&(phd->rbtExtSizeAddr), (TREEKEY)(&exnKey));
  if (!pexn)
  { /* no big enough block found, bug out */
    IMutex_Unlock(phd->pmtxChunks);
    return NULL;
  }

  /* Compute leading and trailing sizes in this block and point to return data. */
  szLeading = ALIGNMENT_CEILING((UINT_PTR)(pexn->pv), szAlignment - 1) - (UINT_PTR)(pexn->pv);
  _H_ASSERT(phd, pexn->sz >= szLeading + sz);
  szTrailing = pexn->sz - szLeading - sz;
  rc = (PVOID)((UINT_PTR)(pexn->pv) + szLeading);
  fZeroed = pexn->fZeroed;
  if (fZeroed)
    *pfZeroed = TRUE;

  /* Remove existing node from the trees. */
  RbtDelete(&(phd->rbtExtSizeAddr), (TREEKEY)pexn);
  RbtDelete(&(phd->rbtExtAddr), (TREEKEY)pexn);

  if (szLeading > 0)
  { /* insert leading space as smaller chunk */
    pexn->sz = szLeading;
    rbtNewNode(&(pexn->rbtnSizeAddress));
    rbtNewNode(&(pexn->rbtnAddress));
    RbtInsert(&(phd->rbtExtSizeAddr), pexn);
    RbtInsert(&(phd->rbtExtAddr), pexn);
    pexn = NULL;  /* reused the node, no longer there */
  }

  if (szTrailing > 0)
  { /* insert trailing space as smaller chunk */
    if (!pexn)
    { /* allocate another node, dropping mutex first to avoid deadlock */
      IMutex_Unlock(phd->pmtxChunks);
      pexn = _HeapBaseNodeAlloc(phd);
      if (!pexn)
      { /* node allocation failed, reverse allocation and bug out */
	_HeapChunkDeAlloc(phd, rc, sz, TRUE);
	return NULL;
      }
      IMutex_Lock(phd->pmtxChunks);
    }
    pexn->pv = (PVOID)(((UINT_PTR)rc) + sz);
    pexn->sz = szTrailing;
    pexn->fZeroed = fZeroed;
    rbtNewNode(&(pexn->rbtnSizeAddress));
    rbtNewNode(&(pexn->rbtnAddress));
    RbtInsert(&(phd->rbtExtSizeAddr), pexn);
    RbtInsert(&(phd->rbtExtAddr), pexn);
    pexn = NULL;  /* used the node */
  }

  IMutex_Unlock(phd->pmtxChunks);
  if (pexn)
    _HeapBaseNodeDeAlloc(phd, pexn);   /* deallocate unused node, if any */
  if (*pfZeroed && !fZeroed)
    StrSetMem(rc, 0, sz);   /* zero memory if we wanted it and don't have it */
  return rc;
}

/*
 * Allocate a new chunk for heap use, either from the current "free" chunks or from the underlying
 * IChunkAllocator interface.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - sz = Size of the chunk to be allocated.
 * - szAlignment = Specifies the alignment that the allocated chunk should have.
 * - fBase = TRUE if this is an allocation from the heap base code, FALSE if not.
 * - pfZeroed = Points to a flag which indicates if the allocated chunk should be zeroed. If the actual
 *              chunk we find is zeroed, this gets toggled to TRUE.
 * 
 * Returns:
 * - NULL = A chunk could not be found (out of memory).
 * - Other = Pointer to the new chunk.
 */
PVOID _HeapChunkAlloc(PHEAPDATA phd, SIZE_T sz, SIZE_T szAlignment, BOOL fBase, BOOL *pfZeroed)
{
  PVOID rc = NULL;       /* return from this function */
  HRESULT hr;            /* return from chunk allocator */

  _H_ASSERT(phd, sz != 0);
  _H_ASSERT(phd, (sz & phd->uiChunkSizeMask) == 0);
  _H_ASSERT(phd, szAlignment != 0);
  _H_ASSERT(phd, (szAlignment & phd->uiChunkSizeMask) == 0);

  /* try getting a recycled chunk first */
  if (!fBase)  /* don't try to recycle if we're allocating on behalf of the base */
    rc = chunk_recycle(phd, sz, szAlignment, pfZeroed);
  if (rc)
    goto returning;

  /* call the chunk allocator for a new chunk */
  hr = IChunkAllocator_AllocChunk(phd->pChunkAllocator, sz, szAlignment, &rc);
  if (SUCCEEDED(hr))
  { /* got it! */
    if (hr != MEMMGR_S_NONZEROED)
      *pfZeroed = TRUE;  /* we got zeroed memory even though we didn't ask for it */
    goto returning;
  }

  /* allocation completely failed */
  rc = NULL;
returning:
  if (rc && !fBase)
  { /* log the chunk we return in our radix tree */
    if (_HeapRTreeSet(phd, phd->prtChunks, (UINT_PTR)rc, rc))
    { /* bogus! deallocate the chunk */
      _HeapChunkDeAlloc(phd, rc, sz, TRUE);
      return NULL;
    }
  }

  _H_ASSERT(phd, CHUNK_ADDR2BASE(phd, rc) == rc);
  return rc;
}

/*
 * Record a chunk in the "free extents" list.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pvChunk = The chunk to be recorded in the free list.
 * - sz = The size of the chunk to be recorded in the free list.
 *
 * Returns:
 * Nothing.
 */
static void chunk_record(PHEAPDATA phd, PVOID pvChunk, SIZE_T sz)
{
  HRESULT hr;             /* allocator return value */
  BOOL fUnzeroed;         /* TRUE if region is not zeroed */
  PEXTENT_NODE pexnNew;   /* new extent node */
  PEXTENT_NODE pexn;      /* pointer to found extent node */
  PEXTENT_NODE pexnPrev;  /* pointer to previous node */
  EXTENT_NODE exnKey;     /* key value to search for */

  /* tell the allocator to purge the region */
  hr = IChunkAllocator_PurgeUnusedRegion(phd->pChunkAllocator, pvChunk, sz);
  fUnzeroed = MAKEBOOL(hr == MEMMGR_S_NONZEROED);

  /* allocate new node which we may need */
  pexnNew = _HeapBaseNodeAlloc(phd);
  IMutex_Lock(phd->pmtxChunks);

  /* try looking to coalesce with next node */
  exnKey.pv = (PVOID)(((UINT_PTR)pvChunk) + sz);
  pexn = (PEXTENT_NODE)RbtFindSuccessor(&(phd->rbtExtAddr), (TREEKEY)(&exnKey));
  if (pexn && (pexn->pv == exnKey.pv))
  { /* coalesce node forward with address range */
    RbtDelete(&(phd->rbtExtSizeAddr), (TREEKEY)pexn);
    pexn->pv = pvChunk;
    pexn->sz += sz;
    pexn->fZeroed = MAKEBOOL(pexn->fZeroed && !fUnzeroed);
    rbtNewNode(&(pexn->rbtnSizeAddress));
    RbtInsert(&(phd->rbtExtSizeAddr), pexn);
    if (pexnNew)  /* didn't need new node after all */
      _HeapBaseNodeDeAlloc(phd, pexnNew);
  }
  else
  { /* could not coalesce forward, insert new node */
    if (!pexnNew)
    { /* this is not likely but in kernel context it could be significant */
      _H_ASSERT(phd, FALSE);  /* record an error here */
      IMutex_Unlock(phd->pmtxChunks);
      return;
    }
    pexn = pexnNew;
    pexn->pv = pvChunk;
    pexn->sz = sz;
    pexn->fZeroed = MAKEBOOL(!fUnzeroed);
    rbtNewNode(&(pexn->rbtnSizeAddress));
    rbtNewNode(&(pexn->rbtnAddress));
    RbtInsert(&(phd->rbtExtSizeAddr), pexn);
    RbtInsert(&(phd->rbtExtAddr), pexn);
  }

  /* Try to coalesce backwards now. */
  pexnPrev = (PEXTENT_NODE)RbtFindPredecessor(&(phd->rbtExtAddr), (TREEKEY)pexn);
  if (pexnPrev && ((PVOID)(((UINT_PTR)(pexnPrev->pv)) + pexnPrev->sz)) == pvChunk)
  { /* Coalesce chunk with previous address range. */
    RbtDelete(&(phd->rbtExtSizeAddr), (TREEKEY)pexnPrev);
    RbtDelete(&(phd->rbtExtAddr), (TREEKEY)pexnPrev);

    RbtDelete(&(phd->rbtExtSizeAddr), (TREEKEY)pexn);
    pexn->pv = pexnPrev->pv;
    pexn->sz += pexnPrev->sz;
    pexn->fZeroed = MAKEBOOL(pexn->fZeroed && pexnPrev->fZeroed);
    rbtNewNode(&(pexn->rbtnSizeAddress));
    RbtInsert(&(phd->rbtExtSizeAddr), pexn);
    _HeapBaseNodeDeAlloc(phd, pexnPrev);
  }

  IMutex_Unlock(phd->pmtxChunks); /* done */
}

/*
 * "Unmap" a chunk and record it as "free."
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pvChunk = The chunk to be unmapped.
 * - sz = The size of the chunk to be unmapped.
 *
 * Returns:
 * Nothing.
 */
void _HeapChunkUnmap(PHEAPDATA phd, PVOID pvChunk, SIZE_T sz)
{
  _H_ASSERT(phd, pvChunk);
  _H_ASSERT(phd, CHUNK_ADDR2BASE(phd, pvChunk) == pvChunk);
  _H_ASSERT(phd, sz);
  _H_ASSERT(phd, (sz && phd->uiChunkSizeMask) == 0);
  chunk_record(phd, pvChunk, sz);
}

/*
 * Deallocate a chunk of memory.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pvChunk = The chunk to be deallocated.
 * - sz = The size of the chunk to be deallocated.
 * - fUnmap = If TRUE, this chunk will be unmapped.
 *
 * Returns:
 * Nothing.
 */
void _HeapChunkDeAlloc(PHEAPDATA phd, PVOID pvChunk, SIZE_T sz, BOOL fUnmap)
{
  _H_ASSERT(phd, pvChunk);
  _H_ASSERT(phd, CHUNK_ADDR2BASE(phd, pvChunk) == pvChunk);
  _H_ASSERT(phd, sz);
  _H_ASSERT(phd, (sz && phd->uiChunkSizeMask) == 0);

  _HeapRTreeSet(phd, phd->prtChunks, (UINT_PTR)pvChunk, NULL);
  if (fUnmap)
    _HeapChunkUnmap(phd, pvChunk, sz);
}

/*
 * Set up the chunk allocation code in the heap.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 *
 * Returns:
 * Standard HRESULT success/failure indicator.
 */
HRESULT _HeapChunkSetup(PHEAPDATA phd)
{
  HRESULT hr;  /* intermediate result */

  hr = IMutexFactory_CreateMutex(phd->pMutexFactory, &(phd->pmtxChunks));
  if (FAILED(hr))
    return hr;
  rbtInitTree(&(phd->rbtExtSizeAddr), (PFNTREECOMPARE)compare_sizeaddr, (PFNGETTREEKEY)get_from_sizeaddr_node,
	      (PFNGETTREENODEPTR)get_sizeaddr_node, (PFNGETFROMTREENODEPTR)get_from_sizeaddr_node);
  rbtInitTree(&(phd->rbtExtAddr), (PFNTREECOMPARE)compare_addr, (PFNGETTREEKEY)get_from_addr_node,
	      (PFNGETTREENODEPTR)get_addr_node, (PFNGETFROMTREENODEPTR)get_from_addr_node);
  phd->prtChunks = _HeapRTreeNew(phd, (1U << (LOG_PTRSIZE + 3)) - phd->nChunkBits);
  if (!(phd->prtChunks))
  {
    IUnknown_Release(phd->pmtxChunks);
    return E_OUTOFMEMORY;
  }
  return S_OK;
}

/*
 * Shut down the chunk allocation code in the heap.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 *
 * Returns:
 * Nothing.
 */
extern void _HeapChunkShutdown(PHEAPDATA phd)
{
  /* TODO: find chunks owned by either the extent tree or the radix tree and attempt to return them */
  _HeapRTreeDestroy(phd->prtChunks);
  IUnknown_Release(phd->pmtxChunks);
}
