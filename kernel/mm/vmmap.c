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
#include <comrogue/scode.h>
#include <comrogue/str.h>
#include <comrogue/allocator.h>
#include <comrogue/internals/seg.h>
#include <comrogue/internals/layout.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/memmgr.h>
#include <comrogue/internals/rbtree.h>
#include <comrogue/internals/startup.h>
#include <comrogue/internals/trace.h>
#include "initfuncs.h"

#ifdef THIS_FILE
#undef THIS_FILE
DECLARE_THIS_FILE
#endif

/*-----------------------------------------------------------------------------------
 * Virtual-memory mapping code that is part of the COMROGUE memory management system
 *-----------------------------------------------------------------------------------
 */

static PMALLOC g_pMalloc = NULL;      /* allocator used */
static VMCTXT g_vmctxtKernel = {      /* kernel VM context */
  .pTTB = NULL,
  .pTTBAux = NULL,
  .uiMaxIndex = SYS_TTB1_ENTRIES,
  .paTTB = 0
};         
static RBTREE g_rbtFreePageTables;    /* tree containing free page tables */
static PFNSETPTEADDR g_pfnSetPTEAddr = NULL;  /* hook function into page database */

/*------------------------------
 * Inline resolution operations
 *------------------------------
 */

/*
 * Resolves a given page table reference for a TTB entry within a VM context.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context.
 * - pTTBEntry = Pointer to the TTB entry containing the page table reference to resolve.
 *
 * Returns:
 * Pointer to the page table, or NULL if the reference could not be resolved.
 */
static inline PPAGETAB resolve_pagetab(PVMCTXT pvmctxt, PTTB pTTBEntry)
{
  register PPAGENODE ppgn = (PPAGENODE)RbtFind(&(pvmctxt->rbtPageTables), (TREEKEY)(pTTBEntry->data & TTBPGTBL_BASE));
  return ppgn ? ppgn->ppt : NULL;
}

/*
 * Resolves a specified VM context pointer to either itself or the kernel VM context, depending on whether one
 * was specified and on the virtual address to be worked with.
 *
 * Parameters:
 * - pvmctxt = The specified VM context pointer.
 * - vma = The base virtual address we're working with.
 *
 * Returns:
 * The pointer to the selected VM context, which may be to g_vmctxtKernel.
 */
static inline PVMCTXT resolve_vmctxt(PVMCTXT pvmctxt, KERNADDR vma)
{
  if (!pvmctxt || (vma & VMADDR_TTB_FENCE))
    return &g_vmctxtKernel;
  return pvmctxt;
}

/*-----------------------------------------
 * Virtual-to-physical functionality group
 *-----------------------------------------
 */

/*
 * Returns the physical address corresponding to a virtual memory address.
 *
 * Parameters:
 * - pvmctxt = The VM context to resolve the address against.
 * - vma = The virtual memory address to resolve.
 *
 * Returns:
 * The physical address corresponding to the virtual memory address, or NULL if the address could
 * not be resolved (is not mapped, or page table could not be mapped).
 */
static PHYSADDR virt_to_phys(PVMCTXT pvmctxt, KERNADDR vma)
{
  register PTTB pTTBEntry = pvmctxt->pTTB + mmVMA2TTBIndex(vma); /* TTB entry pointer */
  register PPAGETAB pTab;                /* page table pointer */

  if ((pTTBEntry->data & TTBQUERY_MASK) == TTBQUERY_FAULT)
    return NULL;  /* we're not allocated */
  if (pTTBEntry->data & TTBSEC_ALWAYS)
    return (pTTBEntry->data & TTBSEC_BASE) | (vma & ~TTBSEC_BASE); /* resolve section address */

  pTab = resolve_pagetab(pvmctxt, pTTBEntry);
  if (!pTab)
    return NULL;  /* could not map the page table */
  return (pTab->pgtbl[mmVMA2PGTBLIndex(vma)].pg.pgaddr << SYS_PAGE_BITS) | (vma & (SYS_PAGE_SIZE - 1));
}

/*
 * Returns the physical address corresponding to a virtual memory address.
 *
 * Parameters:
 * - pvmctxt = The VM context to resolve the address against.  If this is NULL or the address specified
 *             is above the TTB0 fence, the kernel VM context is used.
 * - vma = The virtual memory address to resolve.
 *
 * Returns:
 * The physical address corresponding to the virtual memory address, or NULL if the address could
 * not be resolved (is not mapped, or page table could not be mapped).
 */
PHYSADDR MmGetPhysAddr(PVMCTXT pvmctxt, KERNADDR vma)
{
  return virt_to_phys(resolve_vmctxt(pvmctxt, vma), vma);
}

/*---------------------------
 * Demap functionality group
 *---------------------------
 */

/*
 * Determines whether or not the specified page table is empty.
 *
 * Parameters:
 * - ppgt = Pointer to the page table.
 * 
 * Returns:
 * TRUE if the page table is empty, FALSE otherwise.
 */
static BOOL is_pagetable_empty(PPAGETAB ppgt)
{
  register UINT32 i;  /* loop counter */

  for (i = 0; i < SYS_PGTBL_ENTRIES; i++)
    if ((ppgt->pgtbl[i].data & PGQUERY_MASK) != PGQUERY_FAULT)
      return FALSE;
  return TRUE;
}

/*
 * Free a page table by returning it to the free list.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context.
 * - ppgt = Pointer to the page table to be freed.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * May modify the VM context's page-table tree and g_rbtFreePageTables.
 */
static void free_page_table(PVMCTXT pvmctxt, PPAGETAB ppgt)
{
  PHYSADDR pa = virt_to_phys(pvmctxt, (KERNADDR)ppgt);
  PPAGENODE ppgn = (PPAGENODE)RbtFind(&(pvmctxt->rbtPageTables), (TREEKEY)pa);
  if (ppgn)
  {
    RbtDelete(&(pvmctxt->rbtPageTables), (TREEKEY)pa);
    rbtNewNode(&(ppgn->rbtn), ppgn->rbtn.treekey);
    RbtInsert(&g_rbtFreePageTables, (PRBTREENODE)ppgn);
  }
}

/* Flags for demapping. */
#define DEMAP_NOTHING_SACRED  0x00000001  /* disregard "sacred" flag */

/*
 * Deallocates page mapping entries within a single current entry in the TTB.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context.
 * - vmaStart = The starting VMA of the region to demap.
 * - ndxTTB = Index in the TTB that we're manipulating.
 * - ndxPage = Starting index in the page table of the first entry to deallocate.
 * - cpg = Count of the number of pages to deallocate.  Note that this function will not deallocate more
 *         page mapping entries than remain on the page, as indicated by ndxPage.
 * - uiFlags = Flags for operation.
 *
 * Returns:
 * Standard HRESULT success/failure.  If the result is successful, the SCODE_CODE of the result will
 * indicate the number of pages actually deallocated.
 *
 * Side effects:
 * May modify the TTB entry/aux entry pointed to, and the page table it points to, where applicable.  If the
 * page table is empty after we finish demapping entries, it may be deallocated.
 */
static HRESULT demap_pages1(PVMCTXT pvmctxt, KERNADDR vmaStart, UINT32 ndxTTB, UINT32 ndxPage, UINT32 cpg,
			    UINT32 uiFlags)
{
  UINT32 cpgCurrent;                                  /* number of pages we're mapping */
  PPAGETAB pTab = NULL;                               /* pointer to page table */
  PHYSADDR pa;                                        /* temporary for physical address */
  HRESULT hr;                                         /* return from this function */
  register INT32 i;                                   /* loop counter */

  /* Figure out how many entries we're going to demap. */
  cpgCurrent = SYS_PGTBL_ENTRIES - ndxPage;  /* total free slots on page */
  if (cpg < cpgCurrent)
    cpgCurrent = cpg;     /* only demap up to max requested */
  hr = MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_MEMMGR, cpgCurrent);

  if ((pvmctxt->pTTB[ndxTTB].data & TTBSEC_ALWAYS) && (cpgCurrent == SYS_PGTBL_ENTRIES) && (ndxPage == 0))
  { /* we can kill off the whole section */
    if (pvmctxt->pTTBAux[ndxTTB].aux.sacred && !(uiFlags & DEMAP_NOTHING_SACRED))
      return MEMMGR_E_NOSACRED;  /* can't demap a sacred mapping */
    pa = pvmctxt->pTTB[ndxTTB].data & TTBSEC_BASE;
    if (pvmctxt->pTTB[ndxTTB].sec.c)
      _MmFlushCacheForSection(vmaStart, !(pvmctxt->pTTBAux[ndxTTB].aux.unwriteable));
    if (g_pfnSetPTEAddr && !(pvmctxt->pTTBAux[ndxTTB].aux.notpage))
      for (i = 0; i < SYS_SEC_PAGES; i++)
	(*g_pfnSetPTEAddr)(mmPA2PageIndex(pa) + i, 0, FALSE);
    pvmctxt->pTTB[ndxTTB].data = 0;
    pvmctxt->pTTBAux[ndxTTB].data = 0;
    _MmFlushTLBForSection(vmaStart);
  }
  else if (pvmctxt->pTTB[ndxTTB].data & TTBPGTBL_ALWAYS)
  {
    pTab = resolve_pagetab(pvmctxt, pvmctxt->pTTB + ndxTTB);
    if (!pTab)
      return MEMMGR_E_NOPGTBL;
    for (i = 0; i<cpgCurrent; i++)
    {
      if (pTab->pgaux[ndxPage + i].aux.sacred && !(uiFlags & DEMAP_NOTHING_SACRED))
	return MEMMGR_E_NOSACRED;  /* can't demap a sacred mapping */
    }
    for (i = 0; i<cpgCurrent; i++)
    {
      if (pTab->pgtbl[ndxPage + i].pg.c)  /* only flush cache if cacheable */
	_MmFlushCacheForPage(vmaStart, !(pTab->pgaux[ndxPage + i].aux.unwriteable));
      if (g_pfnSetPTEAddr && !(pTab->pgaux[ndxPage + i].aux.notpage))
	(*g_pfnSetPTEAddr)(mmPA2PageIndex(pTab->pgtbl[ndxPage + i].data & PGTBLSM_PAGE), 0, FALSE);
      pTab->pgtbl[ndxPage + i].data = 0;
      pTab->pgaux[ndxPage + i].data = 0;
      _MmFlushTLBForPage(vmaStart);
      vmaStart += SYS_PAGE_SIZE;
    }
    if (is_pagetable_empty(pTab))
    { /* The page table is now empty; demap it and put it on our free list. */
      pvmctxt->pTTB[ndxTTB].data = 0;
      pvmctxt->pTTBAux[ndxTTB].data = 0;
      free_page_table(pvmctxt, pTab);
      _MmFlushTLBForSection(mmIndices2VMA3(ndxTTB, 0, 0));
    }
  }
  return hr;
}

/*
 * Deallocates page mapping entries in the specified VM context.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context to use.
 * - vmaBase = Base VM address of the region to demap.
 * - cpg = Count of the number of pages of memory to demap.
 * - uiFlags = Flags for operation.
 *
 * Returns:
 * Standard HRESULT success/failure.
 */
static HRESULT demap_pages0(PVMCTXT pvmctxt, KERNADDR vmaBase, UINT32 cpg, UINT32 uiFlags)
{
  UINT32 ndxTTB = mmVMA2TTBIndex(vmaBase);      /* TTB entry index */
  UINT32 ndxPage = mmVMA2PGTBLIndex(vmaBase);   /* starting page entry index */
  UINT32 cpgRemaining = cpg;                    /* number of pages remaining to demap */
  HRESULT hr;                                   /* temporary result */

  if ((cpgRemaining > 0) && (ndxPage > 0))
  { /* We are starting in the middle of a VM page.  Demap to the end of the VM page. */
    hr = demap_pages1(pvmctxt, vmaBase, ndxTTB, ndxPage, cpgRemaining, uiFlags);
    if (FAILED(hr))
      return hr;
    cpgRemaining -= SCODE_CODE(hr);
    if (++ndxTTB == pvmctxt->uiMaxIndex)
      return MEMMGR_E_ENDTTB;
    vmaBase = mmIndices2VMA3(ndxTTB, 0, 0);
  }

  while (cpgRemaining > 0)
  {
    hr = demap_pages1(pvmctxt, vmaBase, ndxTTB, 0, cpgRemaining, uiFlags);
    if (FAILED(hr))
      return hr;
    cpgRemaining -= SCODE_CODE(hr);
    if (++ndxTTB == pvmctxt->uiMaxIndex)
      return MEMMGR_E_ENDTTB;
    vmaBase += SYS_SEC_SIZE;
  }
  return S_OK;
}

/*
 * Deallocates page mapping entries in the specified VM context.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context to use.  If this is NULL or the vmaBase address specified is
 *             above the TTB0 fence, the kernel VM context is used.
 * - vmaBase = Base VM address of the region to demap.
 * - cpg = Count of the number of pages of memory to demap.
 *
 * Returns:
 * Standard HRESULT success/failure.
 */
HRESULT MmDemapPages(PVMCTXT pvmctxt, KERNADDR vmaBase, UINT32 cpg)
{
  return demap_pages0(resolve_vmctxt(pvmctxt, vmaBase), vmaBase, cpg, 0);
}

/*------------------------------------------------------
 * Flag-morphing operations used for reflag and mapping
 *------------------------------------------------------
 */

/*
 * Morphs the "flags" bits used for a page table entry in the TTB and for a page entry in the page table
 * into the "flags" bits used for a section entry in the TTB.
 *
 * Parameters:
 * - uiTableFlags = Flag bits that would be used for a page table entry in the TTB.
 * - uiPageFlags = Flag bits that would be used for a page entry in the page table.
 *
 * Returns:
 * The flag bits that would be used for a section entry in the TTB.  If a bit or option is set
 * in either uiTableFlags or uiPageFlags, it will be set in the appropriate place in the result.
 */
static UINT32 make_section_flags(UINT32 uiTableFlags, UINT32 uiPageFlags)
{
  register UINT32 rc = TTBSEC_ALWAYS;
  rc |= ((uiTableFlags & TTBPGTBL_PXN) >> 2);
  rc |= ((uiTableFlags & TTBPGTBL_NS) << 16);
  rc |= (uiTableFlags & TTBPGTBL_DOM_MASK);
  rc |= (uiTableFlags & TTBPGTBL_P);
  rc |= ((uiPageFlags & PGTBLSM_XN) << 4);
  rc |= (uiPageFlags & PGTBLSM_B);
  rc |= (uiPageFlags & PGTBLSM_C);
  rc |= ((uiPageFlags & PGTBLSM_AP) << 6);
  rc |= ((uiPageFlags & PGTBLSM_TEX) << 6);
  rc |= ((uiPageFlags & PGTBLSM_APX) << 6);
  rc |= ((uiPageFlags & PGTBLSM_S) << 6);
  rc |= ((uiPageFlags & PGTBLSM_NG) << 6);
  return rc;
}

/*
 * Morphs the "auxiliary flags" bits used for a page table entry into "auxiliary flags" used for a TTB entry.
 *
 * Parameters:
 * - uiPageAuxFlags = Page auxiliary flag bits that would be used for a page table entry.
 *
 * Returns:
 * TTB auxiliary flag bits that would be used for a TTB entry.
 */
static UINT32 make_section_aux_flags(UINT32 uiPageAuxFlags)
{
  register UINT32 rc = uiPageAuxFlags & (PGAUX_SACRED|PGAUX_UNWRITEABLE|PGAUX_NOTPAGE);
  /* TODO if we define any other flags */
  return rc;
}

/*-------------------------
 * Reflag operations group
 *-------------------------
 */

/* Structure that defines flag operations on pages. */
typedef struct tagFLAG_OPERATIONS {
  UINT32 uiTableFlags[2];         /* table flag alterations */
  UINT32 uiPageFlags[2];          /* page flag alterations */
  UINT32 uiAuxFlags[2];           /* auxiliary flag alterations */
} FLAG_OPERATIONS, *PFLAG_OPERATIONS;
typedef const FLAG_OPERATIONS *PCFLAG_OPERATIONS;

/* Reflag operation control bits. */
#define FLAGOP_TABLE_COPY0     0x00000001   /* copy uiTableFlags[0] to table flags */
#define FLAGOP_TABLE_SET0      0x00000002   /* set bits in uiTableFlags[0] in table flags */
#define FLAGOP_TABLE_CLEAR0    0x00000004   /* clear bits in uiTableFlags[0] in table flags */
#define FLAGOP_TABLE_CLEAR1    0x00000008   /* clear bits in uiTableFlags[1] in table flags */
#define FLAGOP_PAGE_COPY0      0x00000010   /* copy uiPageFlags[0] to page flags */
#define FLAGOP_PAGE_SET0       0x00000020   /* set bits in uiPageFlags[0] in page flags */
#define FLAGOP_PAGE_CLEAR0     0x00000040   /* clear bits in uiPageFlags[0] in page flags */
#define FLAGOP_PAGE_CLEAR1     0x00000080   /* clear bits in uiPageFlags[1] in page flags */
#define FLAGOP_AUX_COPY0       0x00000100   /* copy uiAuxFlags[0] to aux flags */
#define FLAGOP_AUX_SET0        0x00000200   /* set bits in uiAuxFlags[0] in aux flags */
#define FLAGOP_AUX_CLEAR0      0x00000400   /* clear bits in uiAuxFlags[0] in aux flags */
#define FLAGOP_AUX_CLEAR1      0x00000800   /* clear bits in uiAuxFlags[1] in aux flags */
#define FLAGOP_NOTHING_SACRED  0x80000000   /* reset bits even if marked "sacred" */
#define FLAGOP_PRECALCULATED   0x40000000   /* precalculation of set/clear masks already done */

/*
 * Given a set of flag operations dictated by a FLAG_OPERATIONS structure and a set of control flags,
 * turns them into another FLAG_OPERATIONS structure where the 0 element of each array represents bits
 * to be cleared and the 1 element of each array represents bits to be set.
 *
 * Parameters:
 * - pDest = Pointer to destination buffer.  Will be filled with values by this function.
 * - pSrc = Pointer to source buffer.
 * - uiFlags = Control flags for the operation.
 *
 * Returns:
 * Nothing.
 */
static void precalculate_masks(PFLAG_OPERATIONS pDest, PCFLAG_OPERATIONS pSrc, UINT32 uiFlags)
{
  StrSetMem(pDest, 0, sizeof(FLAG_OPERATIONS));

  /* Precalculate clear and set masks for table flags. */
  if (uiFlags & FLAGOP_TABLE_COPY0)
    pDest->uiTableFlags[0] = TTBPGTBL_SAFEFLAGS;
  else if (uiFlags & FLAGOP_TABLE_CLEAR0)
    pDest->uiTableFlags[0] = pSrc->uiTableFlags[0];
  if (uiFlags & FLAGOP_TABLE_CLEAR1)
    pDest->uiTableFlags[0] |= pSrc->uiTableFlags[1];
  if (uiFlags & (FLAGOP_TABLE_COPY0|FLAGOP_TABLE_SET0))
    pDest->uiTableFlags[1] = pSrc->uiTableFlags[0];
  pDest->uiTableFlags[0] &= ~TTBPGTBL_SAFEFLAGS;
  pDest->uiTableFlags[1] &= ~TTBPGTBL_SAFEFLAGS;

  /* Precalculate clear and set masks for page flags. */
  if (uiFlags & FLAGOP_PAGE_COPY0)
    pDest->uiPageFlags[0] = PGTBLSM_SAFEFLAGS;
  else if (uiFlags & FLAGOP_PAGE_CLEAR0)
    pDest->uiPageFlags[0] = pSrc->uiPageFlags[0];
  if (uiFlags & FLAGOP_PAGE_CLEAR1)
    pDest->uiPageFlags[0] |= pSrc->uiPageFlags[1];
  if (uiFlags & (FLAGOP_PAGE_COPY0|FLAGOP_PAGE_SET0))
    pDest->uiPageFlags[1] = pSrc->uiPageFlags[0];
  pDest->uiPageFlags[0] &= ~PGTBLSM_SAFEFLAGS;
  pDest->uiPageFlags[1] &= ~PGTBLSM_SAFEFLAGS;

  /* Precalculate clear and set masks for auxiliary flags. */
  if (uiFlags & FLAGOP_AUX_COPY0)
    pDest->uiAuxFlags[0] = PGAUX_SAFEFLAGS;
  else if (uiFlags & FLAGOP_AUX_CLEAR0)
    pDest->uiAuxFlags[0] = pSrc->uiAuxFlags[0];
  if (uiFlags & FLAGOP_AUX_CLEAR1)
    pDest->uiAuxFlags[0] |= pSrc->uiAuxFlags[1];         
  if (uiFlags & (FLAGOP_AUX_COPY0|FLAGOP_AUX_SET0))
    pDest->uiAuxFlags[1] = pSrc->uiAuxFlags[0];          
  pDest->uiAuxFlags[0] &= ~PGAUX_SAFEFLAGS;
  pDest->uiAuxFlags[1] &= ~PGAUX_SAFEFLAGS;
}

/*
 * Reflags page mapping entries within a single current entry in the TTB.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context.
 * - vmaStart = The starting VMA of the region to reflag.
 * - ndxTTB = Index in the TTB that we're manipulating.
 * - ndxPage = Starting index in the page table of the first entry to reflag.
 * - cpg = Count of the number of pages to reflag.  Note that this function will not reflag more
 *         page mapping entries than remain on the page, as indicated by ndxPage.
 * - ops = Flag operations, which should be precalculated.
 * - uiFlags = Flags for operation, which should include FLAGOP_PRECALCULATED.
 *
 * Returns:
 * Standard HRESULT success/failure.  If the result is successful, the SCODE_CODE of the result will
 * indicate the number of pages actually reflagged.
 *
 * Side effects:
 * May modify the TTB entry/aux entry pointed to, and the page table it points to, where applicable.
 */
static HRESULT reflag_pages1(PVMCTXT pvmctxt, KERNADDR vmaStart, UINT32 ndxTTB, UINT32 ndxPage, UINT32 cpg,
			     PCFLAG_OPERATIONS ops, UINT32 uiFlags)
{
  UINT32 cpgCurrent;                                  /* number of pages we're mapping */
  PPAGETAB pTab = NULL;                               /* pointer to page table */
  HRESULT hr;                                         /* return from this function */
  register INT32 i;                                   /* loop counter */
  BOOL bFlipSection = FALSE;                          /* are we flipping the entire section? */
  UINT32 uiTemp;                                      /* temporary for new table data */

  ASSERT(uiFlags & FLAGOP_PRECALCULATED);

  /* Figure out how many entries we're going to reflag. */
  cpgCurrent = SYS_PGTBL_ENTRIES - ndxPage;  /* total free slots on page */
  if (cpg < cpgCurrent)
    cpgCurrent = cpg;     /* only reflag up to max requested */
  hr = MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_MEMMGR, cpgCurrent);

  if (!(pvmctxt->pTTB[ndxTTB].data & TTBQUERY_MASK))
    return hr;  /* section not allocated - nothing to do */

  if ((pvmctxt->pTTB[ndxTTB].data & TTBSEC_ALWAYS) && (cpgCurrent == SYS_PGTBL_ENTRIES) && (ndxPage == 0))
  { /* we can remap the section directly */
    if (pvmctxt->pTTBAux[ndxTTB].aux.sacred && !(uiFlags & FLAGOP_NOTHING_SACRED))
      return MEMMGR_E_NOSACRED;  /* can't reflag a sacred mapping */
    if (pvmctxt->pTTB[ndxTTB].sec.c)
      _MmFlushCacheForSection(vmaStart, !(pvmctxt->pTTBAux[ndxTTB].aux.unwriteable));
    pvmctxt->pTTB[ndxTTB].data = (pvmctxt->pTTB[ndxTTB].data
				  & ~make_section_flags(ops->uiTableFlags[0], ops->uiPageFlags[0]))
      | make_section_flags(ops->uiTableFlags[1], ops->uiPageFlags[1]);
    pvmctxt->pTTBAux[ndxTTB].data = (pvmctxt->pTTBAux[ndxTTB].data & ~make_section_aux_flags(ops->uiAuxFlags[0]))
      | make_section_aux_flags(ops->uiAuxFlags[1]);
    _MmFlushTLBForSection(vmaStart);
  }
  else if (pvmctxt->pTTB[ndxTTB].data & TTBPGTBL_ALWAYS)
  {
    pTab = resolve_pagetab(pvmctxt, pvmctxt->pTTB + ndxTTB);
    if (!pTab)
      return MEMMGR_E_NOPGTBL;
    for (i = 0; i<cpgCurrent; i++)
    {
      if (pTab->pgaux[ndxPage + i].aux.sacred && !(uiFlags & FLAGOP_NOTHING_SACRED))
	return MEMMGR_E_NOSACRED;  /* can't reflag a sacred mapping */
    }
    /*
     * If our remapping changes the table flags, then all the page table entries in this section that we're NOT
     * changing had better be unallocated.  If not, that's an error.
     */
    uiTemp = (pvmctxt->pTTB[ndxTTB].data & ~(ops->uiTableFlags[0])) | ops->uiTableFlags[1];
    if (pvmctxt->pTTB[ndxTTB].data != uiTemp)
    {
      for (i = 0; i < ndxPage; i++)
	if (pTab->pgtbl[i].data & PGQUERY_MASK)
	  return MEMMGR_E_COLLIDED;
      for (i = ndxPage + cpgCurrent; i < SYS_PGTBL_ENTRIES; i++)
	if (pTab->pgtbl[i].data & PGQUERY_MASK)
	  return MEMMGR_E_COLLIDED;
      bFlipSection = TRUE;  /* flag it for later */
      _MmFlushCacheForSection(mmIndices2VMA3(ndxTTB, 0, 0), !(pvmctxt->pTTBAux[ndxTTB].aux.unwriteable));
      pvmctxt->pTTB[ndxTTB].data = uiTemp;
    }
    for (i = 0; i < cpgCurrent; i++)
    {
      if (!(pTab->pgtbl[ndxPage + i].data & PGQUERY_MASK))
	continue;  /* skip unallocated pages */
      if (!bFlipSection && pTab->pgtbl[ndxPage + i].pg.c)  /* only flush cache if cacheable */
	_MmFlushCacheForPage(vmaStart, !(pTab->pgaux[ndxPage + i].aux.unwriteable));
      pTab->pgtbl[ndxPage + i].data = (pTab->pgtbl[ndxPage + i].data & ~(ops->uiPageFlags[0])) | ops->uiPageFlags[1];
      pTab->pgaux[ndxPage + i].data = (pTab->pgaux[ndxPage + i].data & ~(ops->uiAuxFlags[0])) | ops->uiAuxFlags[1];
      if (!bFlipSection)
	_MmFlushTLBForPage(vmaStart);
      vmaStart += SYS_PAGE_SIZE;
    }
    if (bFlipSection)
      _MmFlushTLBForSection(mmIndices2VMA3(ndxTTB, 0, 0));
  }
  return hr;
}

/*
 * Reflags page mapping entries in the specified VM context.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context to use.
 * - vmaBase = Base VM address of the region to reflag.
 * - cpg = Count of the number of pages of memory to reflag.
 * - ops = Flag operations structure.
 * - uiFlags = Flags for operation.
 *
 * Returns:
 * Standard HRESULT success/failure.
 */
static HRESULT reflag_pages0(PVMCTXT pvmctxt, KERNADDR vmaBase, UINT32 cpg, PCFLAG_OPERATIONS ops, UINT32 uiFlags)
{
  UINT32 ndxTTB = mmVMA2TTBIndex(vmaBase);      /* TTB entry index */
  UINT32 ndxPage = mmVMA2PGTBLIndex(vmaBase);   /* starting page entry index */
  UINT32 cpgRemaining = cpg;                    /* number of pages remaining to demap */
  HRESULT hr;                                   /* temporary result */
  FLAG_OPERATIONS opsReal;                      /* real operations buffer (precalculated) */

  if (!ops)
    return E_POINTER;
  if (uiFlags & FLAGOP_PRECALCULATED)
    StrCopyMem(&opsReal, ops, sizeof(FLAG_OPERATIONS));
  else
    precalculate_masks(&opsReal, ops, uiFlags);

  if ((cpgRemaining > 0) && (ndxPage > 0))
  { /* We are starting in the middle of a VM page.  Reflag to the end of the VM page. */
    hr = reflag_pages1(pvmctxt, vmaBase, ndxTTB, ndxPage, cpgRemaining, &opsReal, uiFlags|FLAGOP_PRECALCULATED);
    if (FAILED(hr))
      return hr;
    cpgRemaining -= SCODE_CODE(hr);
    if (++ndxTTB == pvmctxt->uiMaxIndex)
      return MEMMGR_E_ENDTTB;
    vmaBase = mmIndices2VMA3(ndxTTB, 0, 0);
  }

  while (cpgRemaining > 0)
  {
    hr = reflag_pages1(pvmctxt, vmaBase, ndxTTB, 0, cpgRemaining, &opsReal, uiFlags|FLAGOP_PRECALCULATED);
    if (FAILED(hr))
      return hr;
    cpgRemaining -= SCODE_CODE(hr);
    if (++ndxTTB == pvmctxt->uiMaxIndex)
      return MEMMGR_E_ENDTTB;
    vmaBase += SYS_SEC_SIZE;
  }
  return S_OK;
}

/* Flags for mapping. */
#define MAP_DONT_ALLOC  0x00000001  /* don't try to allocate new page tables */

/* Forward declaration. */
static HRESULT map_pages0(PVMCTXT pvmctxt, PHYSADDR paBase, KERNADDR vmaBase, UINT32 cpg, UINT32 uiTableFlags,
			  UINT32 uiPageFlags, UINT32 uiAuxFlags, UINT32 uiFlags);

/*
 * Allocates a new page table and associates it with the given TTB entry.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context.
 * - pttbEntry = Pointer to the TTB entry.  On successful return, this will be updated.
 * - pttbAuxEntry = Pointer to the TTB auxiliary table entry.  On successful return, this will be updated.
 * - uiTableFlags = Flags to apply to the TTB entry.
 * - uiFlags = Flags for the mapping operation.
 * - pppt = Pointer to variable to receive new page table pointer.
 *
 * Returns:
 * Standard HRESULT success/failure.
 *
 * Side effects:
 * The new page table is erased before it is returned.  May modify the VM context's page-table tree and
 * g_rbtFreePageTables.  May also allocate a new page of memory.
 */
static HRESULT alloc_page_table(PVMCTXT pvmctxt, PTTB pttbEntry, PTTBAUX pttbAuxEntry, UINT32 uiTableFlags,
				UINT32 uiFlags, PPAGETAB *pppt)
{
  register PPAGENODE ppgn = NULL;  /* page node pointer */
  PPAGENODE ppgnFree;              /* additional pointer for new "free" entry */
  HRESULT hr = S_OK;               /* return from this function */
  PHYSADDR paNewPage = 0;          /* physical address of new page */
  KERNADDR kaNewPage = 0;          /* kernel address of new page */

  if (rbtIsEmpty(&g_rbtFreePageTables))
  {
    if (!(uiFlags & MAP_DONT_ALLOC))
    { /* allocate a new page */
      hr = MmAllocatePage(0, MPDBTAG_SYSTEM, MPDBSYS_PGTBL, &paNewPage);
      if (SUCCEEDED(hr))
      { /* allocate kernel addresses to map it into */
	kaNewPage = _MmAllocKernelAddr(1);
	if (kaNewPage)
	{ /* map the new page in */
	  hr = map_pages0(pvmctxt, paNewPage, kaNewPage, 1, TTBFLAGS_KERNEL_DATA, PGTBLFLAGS_KERNEL_DATA,
			  PGAUXFLAGS_KERNEL_DATA, MAP_DONT_ALLOC);
	  if (SUCCEEDED(hr))
	  { /* allocate heap memory for two nodes to describe the page tables */
	    ppgnFree = IMalloc_Alloc(g_pMalloc, sizeof(PAGENODE));
	    if (ppgnFree)
	      ppgn = IMalloc_Alloc(g_pMalloc, sizeof(PAGENODE));
	    if (ppgnFree && ppgn)
	    { /* prepare the new nodes and insert them in their respective trees */
	      rbtNewNode(&(ppgnFree->rbtn), paNewPage + sizeof(PAGETAB));
	      ppgnFree->ppt = ((PPAGETAB)kaNewPage) + 1;
	      RbtInsert(&g_rbtFreePageTables, (PRBTREENODE)ppgnFree);
	      rbtNewNode(&(ppgn->rbtn), paNewPage);
	      ppgn->ppt = (PPAGETAB)kaNewPage;
	      RbtInsert(&(pvmctxt->rbtPageTables), (PRBTREENODE)ppgn);
	    }
	    else
	    { /* could not allocate both, free one if was allocated */
	      if (ppgnFree)
		IMalloc_Free(g_pMalloc, ppgnFree);
	      hr = E_OUTOFMEMORY;
	    }
	    if (FAILED(hr))
	      demap_pages0(pvmctxt, kaNewPage, 1, 0);
	  }
	  if (FAILED(hr))
	    _MmFreeKernelAddr(kaNewPage, 1);
	}
	else
	  hr = MEMMGR_E_NOKERNSPC;  /* no kernel space available */
	if (FAILED(hr))
	  VERIFY(SUCCEEDED(MmFreePage(paNewPage, MPDBTAG_SYSTEM, MPDBSYS_PGTBL)));
      }
    }
    else
      hr = MEMMGR_E_RECURSED; /* recursive entry */
  }
  else
  { /* get the first item out of the free-pages tree and reinsert it into the current VM context */
    ppgn = (PPAGENODE)RbtFindMin(&g_rbtFreePageTables);
    RbtDelete(&g_rbtFreePageTables, ppgn->rbtn.treekey);
    rbtNewNode(&(ppgn->rbtn), ppgn->rbtn.treekey);
    RbtInsert(&(pvmctxt->rbtPageTables), (PRBTREENODE)ppgn);
  }

  if (SUCCEEDED(hr))
  { /* prepare new page table and insert it into the TTB */
    StrSetMem(ppgn->ppt, 0, sizeof(PAGETAB));
    pttbEntry->data = (PHYSADDR)(ppgn->rbtn.treekey) | uiTableFlags;  /* poke new entry */
    pttbAuxEntry->data = TTBAUXFLAGS_PAGETABLE;
    *pppt = ppgn->ppt;
  }
  else
    *pppt = NULL;
  return hr;
}

/*
 * Maps pages in the specified VM context within a single TTB entry.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context.
 * - paBase = Base physical address to be mapped.
 * - ndxTTB = Index in the TTB that we're manipulating.
 * - ndxPage = Starting index in the page table of the first entry to allocate.
 * - cpg = Count of the number of pages to allocate.  Note that this function will not allocate more
 *         page mapping entries than remain on the page, as indicated by ndxPage.
 * - uiTableFlags = TTB-level flags to use for the page table entry.
 * - uiPageFlags = Page-level flags to use for the page table entry.
 * - uiAuxFlags = Auxiliary data flags to use for the page table entry.
 * - uiFlags = Flags for the mapping operation.
 *
 * Returns:
 * Standard HRESULT success/failure.  If the result is successful, the SCODE_CODE of the result will
 * indicate the number of pages actually deallocated.
 *
 * Side effects:
 * May modify the TTB entry/aux entry pointed to, and the page table it points to, where applicable.  May
 * also allocate a new page table, which may modify other data structures.
 */
static HRESULT map_pages1(PVMCTXT pvmctxt, PHYSADDR paBase, UINT32 ndxTTB, UINT32 ndxPage,
			  UINT32 cpg, UINT32 uiTableFlags, UINT32 uiPageFlags, UINT32 uiAuxFlags, UINT32 uiFlags)
{
  UINT32 cpgCurrent;      /* number of pages we're mapping */
  PPAGETAB pTab = NULL;   /* pointer to current or new page table */
  PHYSADDR paPTab;        /* PA of the page table */
  HRESULT hr;             /* return from this function */
  register INT32 i;       /* loop counter */

  switch (pvmctxt->pTTB[ndxTTB].data & TTBQUERY_MASK)
  {
    case TTBQUERY_FAULT:   /* not allocated, allocate a new page table for the slot */
      hr = alloc_page_table(pvmctxt, pvmctxt->pTTB + ndxTTB, pvmctxt->pTTBAux + ndxTTB, uiTableFlags, uiFlags, &pTab);
      if (FAILED(hr))
	return hr;
      paPTab = (PHYSADDR)(pvmctxt->pTTB[ndxTTB].data & TTBPGTBL_BASE);
      break;

    case TTBQUERY_PGTBL:    /* existing page table */
      if ((pvmctxt->pTTB[ndxTTB].data & TTBPGTBL_ALLFLAGS) != uiTableFlags)
	return MEMMGR_E_BADTTBFLG;  /* table flags not compatible */
      pTab = resolve_pagetab(pvmctxt, pvmctxt->pTTB + ndxTTB);
      if (!pTab)
	return MEMMGR_E_NOPGTBL;  /* could not map the page table */
      paPTab = (PHYSADDR)(pvmctxt->pTTB[ndxTTB].data & TTBPGTBL_BASE);
      break;

    case TTBQUERY_SEC:
    case TTBQUERY_PXNSEC:
      /* this is a section, make sure its base address covers this mapping and its flags are compatible */
      if ((pvmctxt->pTTB[ndxTTB].data & TTBSEC_ALLFLAGS) != make_section_flags(uiTableFlags, uiPageFlags))
	return MEMMGR_E_BADTTBFLG;
      if (pvmctxt->pTTBAux[ndxTTB].data != make_section_aux_flags(uiAuxFlags))
	return MEMMGR_E_BADTTBFLG;
      if ((pvmctxt->pTTB[ndxTTB].data & TTBSEC_BASE) != (paBase & TTBSEC_BASE))
	return MEMMGR_E_COLLIDED;
      pTab = NULL;
      paPTab = pvmctxt->paTTB + (ndxTTB * sizeof(TTB));
      break;
  }

  /* Figure out how many entries we're going to map. */
  cpgCurrent = SYS_PGTBL_ENTRIES - ndxPage;  /* total free slots on page */
  if (cpg < cpgCurrent)
    cpgCurrent = cpg;     /* only map up to max requested */
  hr = MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_MEMMGR, cpgCurrent);

  if (pTab)
  { /* fill in entries in the page table */
    for (i=0; i < cpgCurrent; i++)
    {
      if ((pTab->pgtbl[ndxPage + i].data & PGQUERY_MASK) != PGQUERY_FAULT)
      {
	while (--i >= 0)
	{ /* reverse any mapping we've done in this function */
	  if (g_pfnSetPTEAddr && !(uiAuxFlags & PGAUX_NOTPAGE))
	    (*g_pfnSetPTEAddr)(mmPA2PageIndex(pTab->pgtbl[ndxPage + i].data & PGTBLSM_PAGE), 0, FALSE);
	  pTab->pgtbl[ndxPage + i].data = 0;
	  pTab->pgaux[ndxPage + i].data = 0;
	}
	return MEMMGR_E_COLLIDED;   /* stepping on existing mapping */
      }
      if (g_pfnSetPTEAddr && !(uiAuxFlags & PGAUX_NOTPAGE))
	(*g_pfnSetPTEAddr)(mmPA2PageIndex(paBase), paPTab + ((ndxPage + i) * sizeof(PGTBL)), FALSE);
      pTab->pgtbl[ndxPage + i].data = paBase | uiPageFlags;
      pTab->pgaux[ndxPage + i].data = uiAuxFlags;
      paBase += SYS_PAGE_SIZE;
    }
  }
  else if (g_pfnSetPTEAddr && !(uiAuxFlags & PGAUX_NOTPAGE))
  {
    for (i=0; i < cpgCurrent; i++)
      (*g_pfnSetPTEAddr)(mmPA2PageIndex(paBase & TTBSEC_BASE) + ndxPage + i, paPTab, TRUE);
  }
  return hr;
}

/*
 * Maps pages in the specified VM context.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context.
 * - paBase = Base physical address to be mapped.
 * - vmaBase = Base virtual address to be mapped.
 * - cpg = Count of the number of pages to map.
 * - uiTableFlags = TTB-level flags to use for the page table entry.
 * - uiPageFlags = Page-level flags to use for the page table entry.
 * - uiAuxFlags = Auxiliary data flags to use for the page table entry.
 * - uiFlags = Flags for the mapping operation.
 *
 * Returns:
 * Standard HRESULT success/failure.
 */
static HRESULT map_pages0(PVMCTXT pvmctxt, PHYSADDR paBase, KERNADDR vmaBase, UINT32 cpg, UINT32 uiTableFlags,
			  UINT32 uiPageFlags, UINT32 uiAuxFlags, UINT32 uiFlags)
{
  UINT32 ndxTTB = mmVMA2TTBIndex(vmaBase);      /* TTB entry index */
  UINT32 ndxPage = mmVMA2PGTBLIndex(vmaBase);   /* starting page entry index */
  UINT32 cpgRemaining = cpg;                    /* number of pages remaining to map */
  BOOL bCanMapBySection;                        /* can we map by section? */
  UINT32 uiSecFlags = 0;                        /* section flags */
  UINT32 uiSecAuxFlags = 0;                     /* section auxiliary flags */
  register UINT32 i;                            /* loop counter */
  HRESULT hr;                                   /* temporary result */

  if ((cpgRemaining > 0) && (ndxPage > 0))
  {
    /* We are starting in the middle of a VM page.  Map to the end of the VM page. */
    hr = map_pages1(pvmctxt, paBase, ndxTTB, ndxPage, cpgRemaining, uiTableFlags, uiPageFlags, uiAuxFlags, uiFlags);
    if (FAILED(hr))
      return hr;
    cpgRemaining -= SCODE_CODE(hr);
    paBase += (SCODE_CODE(hr) << SYS_PAGE_BITS);
    if (++ndxTTB == pvmctxt->uiMaxIndex)
    {
      hr = MEMMGR_E_ENDTTB;
      goto errorExit;
    }
  }
  if (cpgRemaining == 0)
    return S_OK;  /* bail out if we finished mapping in first stage */

  bCanMapBySection = MAKEBOOL((cpgRemaining >= SYS_PGTBL_ENTRIES) && ((paBase & TTBSEC_BASE) == paBase));
  if (bCanMapBySection)
  {
    uiSecFlags = make_section_flags(uiTableFlags, uiPageFlags);
    uiSecAuxFlags = make_section_aux_flags(uiAuxFlags);
  }

  while (cpgRemaining >= SYS_PGTBL_ENTRIES)
  { /* try to map a whole section's worth at a time */
    if (bCanMapBySection)
    { /* paBase is section-aligned now as well, we can use a direct 1Mb section mapping */
      switch (pvmctxt->pTTB[ndxTTB].data & TTBQUERY_MASK)
      {
	case TTBQUERY_FAULT:   /* unmapped - map the section */
	  if (g_pfnSetPTEAddr && !(uiAuxFlags & PGAUX_NOTPAGE))
	  {
	    for (i = 0; i < SYS_SEC_PAGES; i++)
	      (*g_pfnSetPTEAddr)(mmPA2PageIndex(paBase) + i, pvmctxt->paTTB + (ndxTTB * sizeof(TTB)), TRUE);
	  }
	  pvmctxt->pTTB[ndxTTB].data = paBase | uiSecFlags;
	  pvmctxt->pTTBAux[ndxTTB].data = uiSecAuxFlags;
	  break;

	case TTBQUERY_PGTBL:   /* page table here */
	  goto pageTableFallback;

	case TTBQUERY_SEC:     /* test existing section */
	case TTBQUERY_PXNSEC:
	  if (   ((pvmctxt->pTTB[ndxTTB].data & TTBSEC_ALLFLAGS) != uiSecFlags)
	      || (pvmctxt->pTTBAux[ndxTTB].data != uiSecAuxFlags))
	  {
	    hr = MEMMGR_E_BADTTBFLG;
	    goto errorExit;
	  }
	  if ((pvmctxt->pTTB[ndxTTB].data & TTBSEC_BASE) != paBase)
	  {
	    hr = MEMMGR_E_COLLIDED;
	    goto errorExit;
	  }
	  break;
      }
      /* we mapped a whole section worth */
      hr = MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_MEMMGR, SYS_PGTBL_ENTRIES);
    }
    else
    {
      /* just map 256 individual pages */
pageTableFallback:
      hr = map_pages1(pvmctxt, paBase, ndxTTB, 0, cpgRemaining, uiTableFlags, uiPageFlags, uiAuxFlags, uiFlags);
      if (FAILED(hr))
	goto errorExit;
    }
    /* adjust base physical address, page count, and TTB index */
    paBase += (SCODE_CODE(hr) << SYS_PAGE_BITS);
    cpgRemaining -= SCODE_CODE(hr);
    if (++ndxTTB == pvmctxt->uiMaxIndex)
    {
      hr = MEMMGR_E_ENDTTB;
      goto errorExit;
    }
  }

  if (cpgRemaining > 0)
  { /* map the "tail end" onto the next TTB */
    hr = map_pages1(pvmctxt, paBase, ndxTTB, 0, cpgRemaining, uiTableFlags, uiPageFlags, uiAuxFlags, uiFlags);
    if (FAILED(hr))
      goto errorExit;
  }
  return S_OK;
errorExit:
  /* demap everything we've managed to map thusfar */
  demap_pages0(pvmctxt, vmaBase, cpg - cpgRemaining, DEMAP_NOTHING_SACRED);
  return hr;
}

/*
 * Maps pages in the specified VM context.
 *
 * Parameters:
 * - pvmctxt = Pointer to the VM context to use.  If this is NULL or the vmaBase address specified is
 *             above the TTB0 fence, the kernel VM context is used.
 * - paBase = Base physical address to be mapped.
 * - vmaBase = Base virtual address to be mapped.
 * - cpg = Count of the number of pages to map.
 * - uiTableFlags = TTB-level flags to use for the page table entry.
 * - uiPageFlags = Page-level flags to use for the page table entry.
 * - uiAuxFlags = Auxiliary data flags to use for the page table entry.
 *
 * Returns:
 * Standard HRESULT success/failure.
 */
HRESULT MmMapPages(PVMCTXT pvmctxt, PHYSADDR paBase, KERNADDR vmaBase, UINT32 cpg, UINT32 uiTableFlags,
		   UINT32 uiPageFlags, UINT32 uiAuxFlags)
{
  return map_pages0(resolve_vmctxt(pvmctxt, vmaBase), paBase, vmaBase, cpg, uiTableFlags, uiPageFlags, uiAuxFlags, 0);
}

/*
 * Maps pages into the kernel address space.  The mapping is done in the kernel VM context.
 *
 * Parameters:
 * - paBase = Base physical address to be mapped.
 * - cpg = Count of the number of pages to map.
 * - uiTableFlags = TTB-level flags to use for the page table entry.
 * - uiPageFlags = Page-level flags to use for the page table entry.
 * - uiAuxFlags = Auxiliary data flags to use for the page table entry.
 * - pvmaLocation = Pointer to a variable which will receive the VM address of the mapped pages.
 *
 * Returns:
 * Standard HRESULT success/failure.
 */
HRESULT MmMapKernelPages(PHYSADDR paBase, UINT32 cpg, UINT32 uiTableFlags,
			 UINT32 uiPageFlags, UINT32 uiAuxFlags, PKERNADDR pvmaLocation)
{
  register HRESULT hr;  /* return from this function */

  if (!pvmaLocation)
    return E_POINTER;
  *pvmaLocation = _MmAllocKernelAddr(cpg);
  if (!(*pvmaLocation))
    return MEMMGR_E_NOKERNSPC;
  hr = map_pages0(&g_vmctxtKernel, paBase, *pvmaLocation, cpg, uiTableFlags, uiPageFlags, uiAuxFlags, 0);
  if (FAILED(hr))
  {
    _MmFreeKernelAddr(*pvmaLocation, cpg);
    *pvmaLocation = NULL;
  }
  return hr;
}

/*
 * Unmaps pages from the kernel address space and reclaims that address space for later use.
 * The mapping is done in the kernel VM context.
 *
 * Parameters:
 * - vmaBase = Base VM address of the region to be unmapped.
 * - cpg = Number of pages to be unmapped.
 *
 * Returns:
 * Standard HRESULT success/failure.
 */
HRESULT MmDemapKernelPages(KERNADDR vmaBase, UINT32 cpg)
{
  register HRESULT hr;

  if ((vmaBase & VMADDR_KERNEL_FENCE) != VMADDR_KERNEL_FENCE)
    return E_INVALIDARG;
  hr = demap_pages0(&g_vmctxtKernel, vmaBase, cpg, 0);
  if (SUCCEEDED(hr))
    _MmFreeKernelAddr(vmaBase, cpg);
  return hr;
}

/*---------------------
 * Initialization code
 *---------------------
 */

/* External references to linker-defined symbols. */
extern char cpgPrestartTotal;

/*
 * Initialize the virtual-memory mapping.
 *
 * Parameters:
 * - pstartup = Pointer to the STARTUP_INFO data structure.
 * - pmInitHeap = Pointer to the initialization heap's IMalloc interface.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Sets up the data structures allocated statically in this file.
 */
SEG_INIT_CODE void _MmInitVMMap(PSTARTUP_INFO pstartup, PMALLOC pmInitHeap)
{
  SEG_INIT_DATA static FLAG_OPERATIONS opsReflagZeroPage = {
    .uiTableFlags = { TTBPGTBL_SAFEFLAGS, TTBFLAGS_KERNEL_DATA },
    .uiPageFlags = { PGTBLSM_SAFEFLAGS, PGTBLFLAGS_KERNEL_DATA, },
    .uiAuxFlags = { PGAUX_SAFEFLAGS, PGAUXFLAGS_KERNEL_DATA|PGAUX_NOTPAGE }
  };
  PHYSADDR paPageTable;    /* PA of current page table */
  KERNADDR kaPageTable;    /* KA of current page table */
  PPAGENODE ppgn;          /* pointer to node being allocated & inserted */
  register UINT32 i;       /* loop counter */

  /* Initialize the local variables in this module. */
  g_pMalloc = pmInitHeap;
  IUnknown_AddRef(g_pMalloc);
  g_vmctxtKernel.pTTB = (PTTB)(pstartup->kaTTB);
  g_vmctxtKernel.pTTBAux = (PTTBAUX)(pstartup->kaTTBAux);
  g_vmctxtKernel.paTTB = pstartup->paTTB;
  rbtInitTree(&(g_vmctxtKernel.rbtPageTables), RbtStdCompareByValue);
  rbtInitTree(&g_rbtFreePageTables, RbtStdCompareByValue);

  /*
   * Load all the page tables we know about.  They all get mapped in as part of the kernel context, except if
   * there's one free on the last page; it gets added to the free list.
   */
  paPageTable = pstartup->paFirstPageTable;
  for (i = 0; i < pstartup->cpgPageTables; i++)
  { /* map page table into kernel space */
    kaPageTable = _MmAllocKernelAddr(1);
    ASSERT(kaPageTable);
    VERIFY(SUCCEEDED(map_pages0(&g_vmctxtKernel, paPageTable, kaPageTable, 1, TTBFLAGS_KERNEL_DATA,
				PGTBLFLAGS_KERNEL_DATA, PGAUXFLAGS_KERNEL_DATA, MAP_DONT_ALLOC)));

    /* allocate node for first page table on page */
    ppgn = IMalloc_Alloc(g_pMalloc, sizeof(PAGENODE));
    ASSERT(ppgn);
    rbtNewNode(&(ppgn->rbtn), paPageTable);
    ppgn->ppt = (PPAGETAB)kaPageTable;
    RbtInsert(&(g_vmctxtKernel.rbtPageTables), (PRBTREENODE)ppgn);

    /* allocate node for second page table on page */
    ppgn = IMalloc_Alloc(g_pMalloc, sizeof(PAGENODE));
    ASSERT(ppgn);
    rbtNewNode(&(ppgn->rbtn), paPageTable + sizeof(PAGETAB));
    ppgn->ppt = ((PPAGETAB)kaPageTable) + 1;
    if ((i == (pstartup->cpgPageTables - 1)) && pstartup->ctblFreeOnLastPage)
      RbtInsert(&g_rbtFreePageTables, (PRBTREENODE)ppgn);
    else
      RbtInsert(&(g_vmctxtKernel.rbtPageTables), (PRBTREENODE)ppgn);

    paPageTable += SYS_PAGE_SIZE;  /* advance to next page table page */
  }

  /*
   * Undo the "temporary" low-memory mappings we created in the prestart code. But we keep the "zero page"
   * in place, because that's where the exception handlers are.  Note that these pages were not flagged as
   * "sacred" at prestart time.
   */
  VERIFY(SUCCEEDED(demap_pages0(&g_vmctxtKernel, SYS_PAGE_SIZE, (UINT32)(&cpgPrestartTotal) - 1, 0)));
  VERIFY(SUCCEEDED(demap_pages0(&g_vmctxtKernel, PHYSADDR_IO_BASE, PAGE_COUNT_IO, 0)));
  /* Reset page attributes on the zero page. */
  VERIFY(SUCCEEDED(reflag_pages0(&g_vmctxtKernel, 0, 1, &opsReflagZeroPage,
				 FLAGOP_NOTHING_SACRED|FLAGOP_PRECALCULATED)));
}

/*
 * Initialize the PTE mapping hook and the PTE mappings for all existing mapped pages.
 *
 * Parameters:
 * - pfnSetPTEAddr = Pointer to the PTE mapping hook function.  This function will be called multiple times
 *                   to initialize the PTE mappings in the MPDB.
 *
 * Returns:
 * Nothing.
 */
SEG_INIT_CODE void _MmInitPTEMappings(PFNSETPTEADDR pfnSetPTEAddr)
{
  register UINT32 i, j;  /* loop counters */
  PHYSADDR paPTE;        /* PA of the PTE */
  PPAGETAB pTab;         /* page table pointer */

  g_pfnSetPTEAddr = pfnSetPTEAddr;  /* set up hook function */
  for (i = 0; i < SYS_TTB1_ENTRIES; i++)
  {
    switch (g_vmctxtKernel.pTTB[i].data & TTBQUERY_MASK)
    {
      case TTBQUERY_PGTBL:
	/* walk page table and assign page table entry pointers to allocated entries */
	paPTE = (PHYSADDR)(g_vmctxtKernel.pTTB[i].data & TTBPGTBL_BASE);
	pTab = resolve_pagetab(&g_vmctxtKernel, g_vmctxtKernel.pTTB + i);
	for (j = 0; j < SYS_PGTBL_ENTRIES; j++)
	{ /* set PTE entry for each entry in turn */
	  if ((pTab->pgtbl[j].data & PGTBLSM_ALWAYS) && !(pTab->pgaux[j].aux.notpage))
	    (*pfnSetPTEAddr)(mmPA2PageIndex(pTab->pgtbl[j].data & PGTBLSM_PAGE), paPTE, FALSE);
	  paPTE += sizeof(PGTBL);
	}
	break;

      case TTBQUERY_SEC:
      case TTBQUERY_PXNSEC:
	if (!(g_vmctxtKernel.pTTBAux[i].aux.notpage))
	{ /* set PTE entry (actually pointer to TTB entry) for the entire section */
	  paPTE = g_vmctxtKernel.paTTB + (i * sizeof(TTB));
	  for (j = 0; j < SYS_SEC_PAGES; j++)
	    (*pfnSetPTEAddr)(mmPA2PageIndex(g_vmctxtKernel.pTTB[i].data & TTBSEC_BASE) + j, paPTE, TRUE);
	}
	break;

      default:
	break;
    }
  }
}
