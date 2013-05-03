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
#include <comrogue/types.h>
#include <comrogue/intlib.h>
#include <comrogue/str.h>
#include <comrogue/allocator.h>
#include <comrogue/objhelp.h>
#include <comrogue/internals/seg.h>
#include <comrogue/internals/layout.h>

/*-------------------------------------------------------------------------------------------------------------
 * Initial heap implementation.  Since this will only be used by initializer code and freed with the rest
 * of the initializer code and data, it doesn't need to be very efficient.  The implementation is adapted from
 * the original K&R storage allocator, with modifications to implement the full IMalloc interface.
 *-------------------------------------------------------------------------------------------------------------
 */

extern PCHAR g_pInitHeapBlock;   /* pointer to heap init block defined in assembly code */

typedef union tagBLOCK
{
  struct
  {
    union tagBLOCK *pNextFree;    /* pointer to next free block */
    SIZE_T cblk;                  /* size of this free block (in blocks) */
  } data;
  INT64 x;                        /* to force alignment */
} BLOCK, *PBLOCK;

typedef struct tagINITHEAP
{
  IMalloc hdr;                    /* object header must be first */
  BLOCK blkBase;                  /* base "zero" block */
  PBLOCK pblkLastAlloc;           /* last allocated block */
  UINT32 cbAlloc;                 /* number of bytes currently allocated */
  UINT32 cbAllocHiWat;            /* high watermark for bytes currently allocated */
} INITHEAP, *PINITHEAP;

/*
 * Allocates a block of memory.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 * - cb = Size of the memory block to be allocated, in bytes.
 *
 * Returns:
 * A pointer to the allocated block of memory, or NULL if memory could not be allocated.
 */
SEG_INIT_CODE static PVOID init_heap_Alloc(IMalloc *pThis, SIZE_T cb)
{
  PINITHEAP pih = (PINITHEAP)pThis;
  register PBLOCK p, q;
  register SIZE_T nBlocks = 1 + (cb + sizeof(BLOCK) - 1) / sizeof(BLOCK);

  q = pih->pblkLastAlloc;
  for (p = q->data.pNextFree; ; q = p, p = p->data.pNextFree)
  {
    if (p->data.cblk >= nBlocks)
    {
      if (p->data.cblk == nBlocks)
	q->data.pNextFree = p->data.pNextFree;
      else
      {
	p->data.cblk -= nBlocks;
	p += p->data.cblk;
	p->data.cblk = nBlocks;
      }
      pih->pblkLastAlloc = q;
      pih->cbAlloc += (nBlocks * sizeof(BLOCK));
      if (pih->cbAlloc > pih->cbAllocHiWat)
	pih->cbAllocHiWat = pih->cbAlloc;
      return (PVOID)(p + 1);
    }
    if (p == pih->pblkLastAlloc)
      break;
  }
  return NULL;
}

/*
 * Determines whether this allocator was used to allocate a block of memory.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 * - pv = Pointer to the memory block to test.  If this parameter is NULL, -1 is returned.
 *
 * Returns:
 * - 1 = If the memory block was allocated by this allocator.
 * - 0 = If the memory block was not allocated by this allocator.
 * - -1 = If this method cannot determine whether the allocator allocated the memory block.
 */
SEG_INIT_CODE static INT32 init_heap_DidAlloc(IMalloc *pThis, PVOID pv)
{
  register PCHAR p = (PCHAR)pv;
  if (!pv)
    return -1;  /* not our business */
  return ((p >= g_pInitHeapBlock) && (p < g_pInitHeapBlock + SIZE_INIT_HEAP)) ? 1 : 0;
}

/*
 * Frees a previously allocated block of memory.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 * - pv = Pointer to the memory block to be freed.  If this parameter is NULL, this method has no effect.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * After this call, the memory pointed to by pv is invalid and should not be used.
 */
SEG_INIT_CODE static void init_heap_Free(IMalloc *pThis, PVOID pv)
{
  PINITHEAP pih = (PINITHEAP)pThis;
  register PBLOCK p, q;
  register UINT32 nBlocks;

  if (init_heap_DidAlloc(pThis, pv) != 1)
    return;  /* not our business */
  p = ((PBLOCK)pv) - 1;
  nBlocks = p->data.cblk;
  for (q = pih->pblkLastAlloc; !((p > q) && (p < q->data.pNextFree)); q = q->data.pNextFree)
    if ((q >= q->data.pNextFree) && ((p > q) || (p < q->data.pNextFree)))
      break;    /* at one end or another */
  if (p + p->data.cblk == q->data.pNextFree)
  {
    /* coalesce block with next (free) block */
    p->data.cblk += q->data.pNextFree->data.cblk;
    p->data.pNextFree = q->data.pNextFree->data.pNextFree;
  }
  else
    p->data.pNextFree = q->data.pNextFree;  /* chain to next free block */
  if (q + q->data.cblk == p)
  {
    /* coalesce free block with previous (free) block */
    q->data.cblk += p->data.cblk;
    q->data.pNextFree = p->data.pNextFree;
  }
  else
    q->data.pNextFree = p;  /* chain to previous free block */
  pih->pblkLastAlloc = q;
  pih->cbAlloc -= (nBlocks * sizeof(BLOCK));
}

/*
 * Changes the size of a previously allocated block.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 * - pv = Pointer to the block of memory to be reallocated.  If this parameter is NULL, a block of memory
 *        of size cb is allocated and returned.
 * - cb = The new size of the memory block to be reallocated, in bytes.  If this parameter is 0 and pv is
 *        not NULL, the block of memory pointed to by pv is freed and NULL is returned.
 *
 * Returns:
 * If pv is not NULL and cb is 0, NULL is always returned.  Otherwise, NULL is returned if the block of memory
 * could not be reallocated, or the pointer to the reallocated block of memory is returned.
 */
SEG_INIT_CODE static PVOID init_heap_Realloc(IMalloc *pThis, PVOID pv, SIZE_T cb)
{
  PINITHEAP pih = (PINITHEAP)pThis;
  SIZE_T nBlocksNew, nBlocksExtra;
  PVOID pNew;
  UINT32 cbHiWatSave;
  register PBLOCK p, pNext, q, qp;

  /* Handle degenerate cases */
  if (!pv)
    return init_heap_Alloc(pThis, cb);
  if (cb == 0)
  {
    init_heap_Free(pThis, pv);
    return NULL;
  }
  if (init_heap_DidAlloc(pThis, pv) != 1)
    return NULL;  /* not our business */

  p = ((PBLOCK)pv) - 1;
  nBlocksNew = 1 + (cb + sizeof(BLOCK) - 1) / sizeof(BLOCK);
  if (nBlocksNew == p->data.cblk)
    return pv;   /* nothing to do! */

  if (nBlocksNew < p->data.cblk)
  { /* shrinking block - chop block in middle and free the upper end */
    pNext = p + nBlocksNew;
    pNext->data.cblk = p->data.cblk - nBlocksNew;
    p->data.cblk = nBlocksNew;
    init_heap_Free(pThis, (PVOID)(pNext + 1));  /* adjusts cbAlloc */
    return pv;
  }

  /* see if next block is free so we can expand in place */
  nBlocksExtra = nBlocksNew - p->data.cblk;
  pNext = p + p->data.cblk;
  qp = pih->pblkLastAlloc;
  for (q = qp->data.pNextFree; ; qp = q, q = q->data.pNextFree)
  {
    if (q == pNext)
    {
      if (q->data.cblk < nBlocksExtra)
	break;  /* cannot get enough blocks by combining next free block */
      qp->data.pNextFree = q->data.pNextFree; /* remove block from free list */
      pih->cbAlloc += (q->data.cblk * sizeof(BLOCK));  /* act like we allocated it all for the nonce */
      if (q->data.cblk == nBlocksExtra)
      { /* take it all */
	pih->pblkLastAlloc = qp->data.pNextFree;
      }
      else
      { /* chop in two, add first block to existing, free second block */
	pNext += nBlocksExtra;
	pNext->data.cblk = q->data.cblk - nBlocksExtra;
	init_heap_Free(pThis, (PVOID)(pNext + 1)); /* also fixes cbAlloc */
      }
      p->data.cblk = nBlocksNew;
      if (pih->cbAlloc > pih->cbAllocHiWat)
	pih->cbAllocHiWat = pih->cbAlloc;
      return pv;
    }
    if (q == pih->pblkLastAlloc)
      break;  /* not found */
  }

  /* last ditch effort: allocate new block and copy old contents in */
  cbHiWatSave = pih->cbAllocHiWat;
  pNew = init_heap_Alloc(pThis, cb);
  if (!pNew)
    return NULL;   /* cannot reallocate */
  StrCopyMem(pv, pNew, (p->data.cblk - 1) * sizeof(BLOCK));
  init_heap_Free(pThis, pv);
  pih->cbAllocHiWat = intMax(cbHiWatSave, pih->cbAlloc);
  return pNew;
}

/*
 * Returns the size of a previously-allocated block of memory.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 * - pv = Pointer to the block of memory.
 *
 * Returns:
 * The size of the allocated block of memory in bytes, which may be greater than the size requested when
 * it was allocated.
 */
SEG_INIT_CODE static SIZE_T init_heap_GetSize(IMalloc *pThis, PVOID pv)
{
  register PBLOCK p;

  if (init_heap_DidAlloc(pThis, pv) != 1)
    return (SIZE_T)(-1);  /* not our business */
  p = ((PBLOCK)pv) - 1;
  return (p->data.cblk - 1) * sizeof(BLOCK);
}

static const SEG_INIT_RODATA struct IMallocVTable vtblInitHeap =
{
  .QueryInterface = ObjHlpStandardQueryInterface_IMalloc,
  .AddRef = ObjHlpStaticAddRefRelease,
  .Release = ObjHlpStaticAddRefRelease,
  .Alloc = init_heap_Alloc,
  .Realloc = init_heap_Realloc,
  .Free = init_heap_Free,
  .GetSize = init_heap_GetSize,
  .DidAlloc = init_heap_DidAlloc,
  .HeapMinimize = (void (*)(IMalloc *))ObjHlpDoNothingReturnVoid
};

/*
 * Returns a reference to the initial heap.
 *
 * Parameters:
 * None.
 *
 * Returns:
 * A reference to the initial heap, in the form of a pointer to its IMalloc interface.
 */
SEG_INIT_CODE IMalloc *_MmGetInitHeap(void)
{
  static SEG_INIT_DATA INITHEAP initheap = { .pblkLastAlloc = NULL };
  register PBLOCK p;

  if (!(initheap.pblkLastAlloc))
  { /* initialize fields of initheap */
    initheap.hdr.pVTable = &vtblInitHeap;
    initheap.pblkLastAlloc = initheap.blkBase.data.pNextFree = &(initheap.blkBase);
    initheap.blkBase.data.cblk = 0;
    /* add g_pInitHeapBlock as the free block in the heap */
    p = (PBLOCK)g_pInitHeapBlock;
    p->data.cblk = SIZE_INIT_HEAP / sizeof(BLOCK);
    init_heap_Free((IMalloc *)(&initheap), (PVOID)(p + 1));
    initheap.cbAlloc = initheap.cbAllocHiWat = 0;  /* start from zero now */
  }
  return (IMalloc *)(&initheap);
}
