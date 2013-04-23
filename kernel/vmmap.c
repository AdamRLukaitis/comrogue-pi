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
  .uiMaxIndex = SYS_TTB1_ENTRIES
};         
static RBTREE g_rbtFreePageTables;    /* tree containing free page tables */

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
    if (pvmctxt->pTTB[ndxTTB].sec.c)
      _MmFlushCacheForSection(vmaStart, !(pvmctxt->pTTBAux[ndxTTB].aux.unwriteable));
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
  register UINT32 rc = uiPageAuxFlags & (PGAUX_SACRED|PGAUX_UNWRITEABLE);
  /* TODO if we define any other flags */
  return rc;
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
    {
      /* TODO: pull a new page out of our ass and assign its PA to paNewPage */
      if (paNewPage)
      { /* allocate kernel addresses to map it into */
	kaNewPage = _MmAllocKernelAddr(1);
	if (kaNewPage)
	{ /* map the new page in */
	  hr = map_pages0(pvmctxt, paNewPage, kaNewPage, 1,TTBFLAGS_KERNEL_DATA, PGTBLFLAGS_KERNEL_DATA,
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
      }
      else
	hr = E_OUTOFMEMORY;  /* no memory to allocate new page table */
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
  HRESULT hr;             /* return from this function */
  register INT32 i;       /* loop counter */

  switch (pvmctxt->pTTB[ndxTTB].data & TTBQUERY_MASK)
  {
    case TTBQUERY_FAULT:   /* not allocated, allocate a new page table for the slot */
      hr = alloc_page_table(pvmctxt, pvmctxt->pTTB + ndxTTB, pvmctxt->pTTBAux + ndxTTB, uiTableFlags, uiFlags, &pTab);
      if (FAILED(hr))
	return hr;
      break;

    case TTBQUERY_PGTBL:    /* existing page table */
      if ((pvmctxt->pTTB[ndxTTB].data & TTBPGTBL_ALLFLAGS) != uiTableFlags)
	return MEMMGR_E_BADTTBFLG;  /* table flags not compatible */
      pTab = resolve_pagetab(pvmctxt, pvmctxt->pTTB + ndxTTB);
      if (!pTab)
	return MEMMGR_E_NOPGTBL;  /* could not map the page table */
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
	  pTab->pgtbl[ndxPage + i].data = 0;
	  pTab->pgaux[ndxPage + i].data = 0;
	}
	hr = MEMMGR_E_COLLIDED;   /* stepping on existing mapping */
	goto exit;
      }
      pTab->pgtbl[ndxPage + i].data = paBase | uiPageFlags;
      pTab->pgaux[ndxPage + i].data = uiAuxFlags;
      paBase += SYS_PAGE_SIZE;
    }
  }
exit:
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
  PHYSADDR paPageTable;    /* PA of current page table */
  KERNADDR kaPageTable;    /* KA of current page table */
  PPAGENODE ppgn;          /* pointer to node being allocated & inserted */
  register UINT32 i;       /* loop counter */

  /* Initialize the local variables in this module. */
  g_pMalloc = pmInitHeap;
  IUnknown_AddRef(g_pMalloc);
  g_vmctxtKernel.pTTB = (PTTB)(pstartup->kaTTB);
  g_vmctxtKernel.pTTBAux = (PTTBAUX)(pstartup->kaTTBAux);
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
}
