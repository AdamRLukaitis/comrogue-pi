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
#include <comrogue/allocator.h>
#include <comrogue/internals/memmgr.h>
#include <comrogue/internals/rbtree.h>
#include <comrogue/internals/layout.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/seg.h>
#include <comrogue/internals/startup.h>
#include <comrogue/internals/trace.h>

#ifdef THIS_FILE
#undef THIS_FILE
DECLARE_THIS_FILE
#endif

/*------------------------------------------
 * Operations with kernel address intervals
 *------------------------------------------
 */

/* Definiton of an address interval */
typedef struct tagAINTERVAL {
  KERNADDR kaFirst;              /* first kernel address in the interval */
  KERNADDR kaLast;               /* first kernel address NOT in the interval */
} AINTERVAL, *PAINTERVAL;
typedef const AINTERVAL *PCAINTERVAL;

/*
 * Compares two address intervals.
 *
 * Parameters:
 * - paiLeft = Pointer to first address interval to compare.
 * - paiRight = Pointer to second address interval to compare.
 *
 * Returns:
 * - -1 = If the interval paiLeft is entirely before the interval paiRight.
 * - 0 = If the interval paiLeft is entirely contained within (or equal to) the interval paiRight.
 * - 1 = If the interval paiLeft is entirely after the interval paiRight.
 *
 * N.B.:
 * It is an error if the intervals overlap without paiLeft being entirely contained within paiRight.
 * (This should not happen.)
 */
static INT32 interval_compare(PCAINTERVAL paiLeft, PCAINTERVAL paiRight)
{
  static DECLARE_STRING8_CONST(szFitCheck, "interval_compare fitcheck: [%08x,%08x] <?> [%08x,%08x]");

  ASSERT(paiLeft->kaFirst < paiLeft->kaLast);
  ASSERT(paiRight->kaFirst < paiRight->kaLast);
  if ((paiLeft->kaFirst >= paiRight->kaFirst) && (paiLeft->kaLast <= paiRight->kaLast))
    return 0;
  if (paiLeft->kaLast <= paiRight->kaFirst)
    return -1;
  if (paiLeft->kaFirst >= paiRight->kaLast)
    return 1;
  /* if get here, bugbugbugbugbug */
  TrPrintf8(szFitCheck, paiLeft->kaFirst, paiLeft->kaLast, paiRight->kaFirst, paiRight->kaLast);
  /* TODO: bugcheck */
  return 0;
}

/*
 * Determines if two intervals are adjacent, that is, if the end of the first is the start of the next.
 *
 * Parameters:
 * - paiLeft = Pointer to first address interval to compare.
 * - paiRight = Pointer to second address interval to compare.
 *
 * Returns:
 * TRUE if paiLeft is adjacent to paiRight, FALSE otherwise.
 */
static inline BOOL intervals_adjacent(PCAINTERVAL paiLeft, PCAINTERVAL paiRight)
{
  return MAKEBOOL(paiLeft->kaLast == paiRight->kaFirst);
}

/*
 * Returns the number of pages described by an interval.
 *
 * Parameters:
 * - pai = The interval to test.
 *
 * Returns:
 * The number of pages described by this interval.
 */
static inline UINT32 interval_numpages(PCAINTERVAL pai)
{
  return (pai->kaLast - pai->kaFirst) >> SYS_PAGE_BITS;
}

/*
 * Initializes an interval's start and end points.
 *
 * Parameters:
 * - pai = Pointer to the interval to be initialized.
 * - kaFirst = First address in the interval.
 * - kaLast = Last address in the interval.
 *
 * Returns:
 * pai.
 */
static inline PAINTERVAL init_interval(PAINTERVAL pai, KERNADDR kaFirst, KERNADDR kaLast)
{
  pai->kaFirst = kaFirst;
  pai->kaLast = kaLast;
  return pai;
}

/*
 * Initializes an interval to start at a specified location and cover a specific number of pages.
 *
 * Parameters:
 * - pai = Pointer to the interval to be initialized.
 * - kaBase = Base address of the interval.
 * - cpg = Number of pages the interval is to contain.
 *
 * Returns:
 * pai.
 */
static inline PAINTERVAL init_interval_pages(PAINTERVAL pai, KERNADDR kaBase, UINT32 cpg)
{
  pai->kaFirst = kaBase;
  pai->kaLast = kaBase + (cpg << SYS_PAGE_BITS);
  return pai;
}

/*----------------------------------------
 * Kernel address manipulation operations
 *----------------------------------------
 */

/* Tree structure in which we store "free" address intervals. */
typedef struct tagADDRTREENODE {
  RBTREENODE rbtn;                   /* tree node structure */
  AINTERVAL ai;                      /* address interval this represents */
} ADDRTREENODE, *PADDRTREENODE;

/* Structure used in allocating address space. */
typedef struct tagALLOC_STRUC {
  UINT32 cpgNeeded;                  /* count of number of pages needed */
  PADDRTREENODE patnFound;           /* pointer to "found" tree node */
} ALLOC_STRUC, *PALLOC_STRUC;

static RBTREE g_rbtFreeAddrs;          /* free address tree */
static PMALLOC g_pMalloc = NULL;       /* allocator we use */

/*
 * Inserts a kernel address range into the tree.
 *
 * Parameters:
 * - kaFirst = First address in the range to be inserted.
 * - kaLast = Last address in the range to be inserted.
 *
 * Returns:
 * - Nothing.
 *
 * Side effects:
 * Modifies g_rbtFreeAddrs; allocates space from the g_pMalloc heap.
 */
static void insert_into_tree(KERNADDR kaFirst, KERNADDR kaLast)
{
  PADDRTREENODE pnode = IMalloc_Alloc(g_pMalloc, sizeof(ADDRTREENODE));
  ASSERT(pnode);
  rbtNewNode(&(pnode->rbtn), init_interval(&(pnode->ai), kaFirst, kaLast));
  RbtInsert(&g_rbtFreeAddrs, (PRBTREENODE)pnode);
}

/*
 * Subfunction called from a tree walk to find a free address interval in the tree that can supply us with
 * the number of pages we need.
 *
 * Parameters:
 * - pUnused = Not used.
 * - pnode = Current tree node we're walking over.
 * - palloc = Pointer to allocation structure.
 *
 * Returns:
 * FALSE if we found a node containing enough space (written to palloc->patnFound), TRUE otherwise.
 */
static BOOL alloc_check_space(PVOID pUnused, PADDRTREENODE pnode, PALLOC_STRUC palloc)
{
  if (interval_numpages(&(pnode->ai)) >= palloc->cpgNeeded)
  {
    palloc->patnFound = pnode;
    return FALSE;
  }
  return TRUE;
}

/*
 * Allocates a block of kernel addresses suitable to contain a certain number of pages.
 *
 * Parameters:
 * - cpgNeeded = Number of pages of kernel address space that are needed.
 *
 * Returns:
 * Base address of the block of address space we got.
 *
 * Side effects:
 * May modify g_rbtFreeAddrs and free space to the g_pMalloc heap.
 *
 * N.B.:
 * Running out of kernel address space should be a bug.
 */
KERNADDR _MmAllocKernelAddr(UINT32 cpgNeeded)
{
  register KERNADDR rc;                           /* return from this function */
  BOOL bResult;                                   /* result of tree walk */
  ALLOC_STRUC alloc_struc = { cpgNeeded, NULL };  /* allocation structure */

  /* Walk the tree to find a block of free addresses that are big enough. */
  bResult = RbtWalk(&g_rbtFreeAddrs, (PFNRBTWALK)alloc_check_space, &alloc_struc);
  ASSERT(!bResult);
  if (bResult)
  {
    /* TODO: bug check */
    return 0;
  }

  /* We allocate address space from the start of the interval we found. */
  rc = alloc_struc.patnFound->ai.kaFirst;
  if (interval_numpages(&(alloc_struc.patnFound->ai)) == cpgNeeded)
  {
    /* This node is all used up by this allocation.  Remove it from the tree and free it. */
    RbtDelete(&g_rbtFreeAddrs, (TREEKEY)(&(alloc_struc.patnFound->ai)));
    IMalloc_Free(g_pMalloc, alloc_struc.patnFound);
  }
  else
  {
    /*
     * Chop off the number of pages we're taking.  This does not change the ordering of nodes in the tree
     * because we're just shortening this one's interval.
     */
    alloc_struc.patnFound->ai.kaFirst += (cpgNeeded << SYS_PAGE_BITS);
  }
  return rc;
}

/*
 * Frees a block of kernel addresses that was previously allocated.
 *
 * Parameters:
 * - kaBase = Base address of the kernel address space region to be freed.
 * - cpgToFree = Number of pages of kernel address space to be freed.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * May modify g_rbtFreeAddrs and allocate or free space in the g_pMalloc heap.
 */
void _MmFreeKernelAddr(KERNADDR kaBase, UINT32 cpgToFree)
{
  register PADDRTREENODE patnPred, patnSucc;  /* predecessor and successor pointers */
  AINTERVAL aiFree;                           /* actual interval we're freeing */

  init_interval_pages(&aiFree, kaBase, cpgToFree);
  ASSERT(!RbtFind(&g_rbtFreeAddrs, (TREEKEY)(&aiFree)));
  patnPred = (PADDRTREENODE)RbtFindPredecessor(&g_rbtFreeAddrs, (TREEKEY)(&aiFree));
  patnSucc = (PADDRTREENODE)RbtFindSuccessor(&g_rbtFreeAddrs, (TREEKEY)(&aiFree));
  if (patnPred && intervals_adjacent(&(patnPred->ai), &aiFree))
  {
    if (patnSucc && intervals_adjacent(&aiFree, &(patnSucc->ai)))
    { /* combine predecessor, interval, and successor into one big node */
      RbtDelete(&g_rbtFreeAddrs, (TREEKEY)(&(patnPred->ai)));
      patnPred->ai.kaLast = patnSucc->ai.kaLast;
      IMalloc_Free(g_pMalloc, patnSucc);
    }
    else  /* combine with predecessor */
      patnPred->ai.kaLast = aiFree.kaLast;
  }
  else if (patnSucc && intervals_adjacent(&aiFree, &(patnSucc->ai)))
    patnSucc->ai.kaFirst = aiFree.kaFirst;  /* combine with successor */
  else  /* insert as a new address range */
    insert_into_tree(aiFree.kaFirst, aiFree.kaLast);
}

/*
 * Initializes the kernel address space management code.
 *
 * Parameters:
 * - pstartup = Pointer to startup information block.
 * - pmInitHeap = Pointer to initialization heap allocator.
 *
 * Returns:
 * Nothing.
 */
SEG_INIT_CODE void _MmInitKernelSpace(PSTARTUP_INFO pstartup, PMALLOC pmInitHeap)
{
  g_pMalloc = pmInitHeap;
  IUnknown_AddRef(g_pMalloc);
  rbtInitTree(&g_rbtFreeAddrs, (PFNTREECOMPARE)interval_compare);
  insert_into_tree(pstartup->vmaFirstFree, VMADDR_IO_BASE);
  insert_into_tree(VMADDR_IO_BASE + (PAGE_COUNT_IO * SYS_PAGE_SIZE), VMADDR_KERNEL_NOMANS);
}
