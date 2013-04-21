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
#include <comrogue/allocator.h>
#include <comrogue/internals/seg.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/memmgr.h>
#include <comrogue/internals/rbtree.h>
#include <comrogue/internals/startup.h>
#include <comrogue/internals/trace.h>

#ifdef THIS_FILE
#undef THIS_FILE
DECLARE_THIS_FILE
#endif

/* Tree node storing mapping of physical addresses of page table pages to their kernel addresses */
typedef struct tagPGTMAP {
  RBTREENODE rbtn;                   /* tree node structure */
  KERNADDR kaPGTPage;                /* page table page kernel address */
  UINT32 uiRefCount;                 /* reference count for mapping */
} PGTMAP, *PPGTMAP;

#define NMAPFRAMES  4                 /* number of frame mappings */

static PMALLOC g_pMalloc = NULL;      /* allocator used */
static PTTB g_pttb1 = NULL;           /* pointer to TTB1 */
static PTTBAUX g_pttb1Aux = NULL;     /* pointer to TTB1 aux data */
static RBTREE g_rbtPageTables;        /* tree mapping page table PAs to KAs */

/* Forward declaration. */
static HRESULT map_pages0(PTTB pTTB, PTTBAUX pTTBAux, PHYSADDR paBase, KERNADDR vmaBase, UINT32 cpg,
			  UINT32 uiTableFlags, UINT32 uiPageFlags, UINT32 uiAuxFlags);

/*
 * Maps a page table's page into kernel memory space where we can examine it.
 *
 * Parameters:
 * - paPageTable = Physical address of the page table to map.
 *
 * Returns:
 * Pointer to the pagetable in kernel memory, or NULL if we weren't able to map it.
 *
 * Side effects:
 * May modify g_rbtPageTables, and may modify TTB1 if we map a page into memory.  May allocate
 * memory from g_pMalloc.
 */
static PPAGETAB map_pagetable(PHYSADDR paPageTable)
{
  register PHYSADDR paOfPage = paPageTable & ~(SYS_PAGE_SIZE - 1);  /* actual page table page's PA */
  register PPGTMAP ppgtmap;

  ppgtmap = (PPGTMAP)RbtFind(&g_rbtPageTables, (TREEKEY)paOfPage);
  if (!ppgtmap)
  {
    ppgtmap = IMalloc_Alloc(g_pMalloc, sizeof(PGTMAP));
    ppgtmap->kaPGTPage = _MmAllocKernelAddr(1);
    ASSERT(ppgtmap->kaPGTPage);
    if (SUCCEEDED(map_pages0(g_pttb1, g_pttb1Aux, paOfPage, ppgtmap->kaPGTPage, 1, TTBFLAGS_KERNEL_DATA,
			     PGTBLFLAGS_KERNEL_DATA, PGAUXFLAGS_KERNEL_DATA)))
    {
      ppgtmap->uiRefCount = 1;
      rbtNewNode(&(ppgtmap->rbtn), paOfPage);
      RbtInsert(&g_rbtPageTables, (PRBTREENODE)ppgtmap);
    }
    else
    {
      _MmFreeKernelAddr(ppgtmap->kaPGTPage, 1);
      IMalloc_Free(g_pMalloc, ppgtmap);
      return NULL;
    }
  }
  else
    ppgtmap->uiRefCount++;
  return (PPAGETAB)(ppgtmap->kaPGTPage | (paPageTable & (SYS_PAGE_SIZE - 1)));
}

/* Forward declaration. */
static HRESULT demap_pages0(PTTB pTTB, PTTBAUX pTTBAux, KERNADDR vmaBase, UINT32 cpg, UINT32 uiFlags);

/*
 * Demaps a page table's page from kernel memory space.
 *
 * Parameters:
 * - ppgtbl = Pointer to the page table.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * May modify g_rbtPageTables, and may modify TTB1 if we unmap a page from memory.  May free
 * memory in g_pMalloc.
 */
static void demap_pagetable(PPAGETAB ppgtbl)
{
  register PHYSADDR paOfPage;
  register PPGTMAP ppgtmap;

  paOfPage = MmGetPhysAddr(g_pttb1, ((KERNADDR)ppgtbl) & ~(SYS_PAGE_SIZE - 1));
  ppgtmap = (PPGTMAP)RbtFind(&g_rbtPageTables, (TREEKEY)paOfPage);
  if (ppgtmap)
  {
    if (--(ppgtmap->uiRefCount) == 0)
    {
      RbtDelete(&g_rbtPageTables, (TREEKEY)paOfPage);
      demap_pages0(g_pttb1, g_pttb1Aux, ppgtmap->kaPGTPage, 1, 0);
      _MmFreeKernelAddr(ppgtmap->kaPGTPage, 1);
      IMalloc_Free(g_pMalloc, ppgtmap);
    }
  }
}

/*
 * Resolves a specified TTB to either itself or the global TTB1, depending on whether one was specified
 * and on the virtual address to be worked with.
 *
 * Parameters:
 * - pTTB = The specified TTB pointer.
 * - vma = The base virtual address we're working with.
 *
 * Returns:
 * The pointer to the selected TTB, which may be the global variable g_pttb1.
 */
static inline PTTB resolve_ttb(PTTB pTTB, KERNADDR vma)
{
  if (!pTTB || (vma & 0x80000000))
    return g_pttb1;  /* if no TTB specified or address is out of range for TTB0, use TTB1 */
  return pTTB;
}

/*
 * Resolves a specified TTB auxiliary table to either itself or the global TTB1Aux, depending on whether one
 * was specified and on the virtual address to be worked with.
 *
 * Parameters:
 * - pTTBAux = The specified TTB aux table pointer.
 * - vma = The base virtual address we're working with.
 *
 * Returns:
 * The pointer to the selected TTB aux table, which may be the global variable g_pttb1Aux.
 */
static inline PTTBAUX resolve_ttbaux(PTTBAUX pTTBAux, KERNADDR vma)
{
  if (!pTTBAux || (vma & 0x80000000))
    return g_pttb1Aux;
  return pTTBAux;
}

/*
 * Returns the physical address corresponding to a virtual memory address.
 *
 * Parameters:
 * - pTTB = The TTB to resolve the VM address against.  If this is NULL or if the address specified
 *          is outside the TTB0 range, the system TTB is used.
 *
 * Returns:
 * The physical address corresponding to the virtual memory address, or NULL if the address could
 * not be resolved (is not mapped, or page table could not be mapped).
 */
PHYSADDR MmGetPhysAddr(PTTB pTTB, KERNADDR vma)
{
  PTTB pTTBEntry = resolve_ttb(pTTB, vma) + mmVMA2TTBIndex(vma);
  PPAGETAB pTab;
  PHYSADDR rc;

  if ((pTTBEntry->data & TTBQUERY_MASK) == TTBQUERY_FAULT)
    return NULL;  /* we're not allocated */
  if (pTTBEntry->data & TTBSEC_ALWAYS)
    return (pTTBEntry->data & TTBSEC_BASE) | (vma & ~TTBSEC_BASE); /* resolve section address */

  pTab = map_pagetable(pTTBEntry->data & TTBPGTBL_BASE);
  if (!pTab)
    return NULL;  /* could not map the page table */
  rc = (pTab->pgtbl[mmVMA2PGTBLIndex(vma)].pg.pgaddr << SYS_PAGE_BITS) | (vma & (SYS_PAGE_SIZE - 1));
  demap_pagetable(pTab);
  return rc;
}

/* Flags for demapping. */
#define DEMAP_NOTHING_SACRED  0x00000001  /* disregard "sacred" flag */

/*
 * Deallocates page mapping entries within a single current entry in the TTB.
 *
 * Parameters:
 * - pTTBEntry = Pointer to the TTB entry to deallocate in.
 * - pTTBAuxEntry = Pointer to the TTB aux table entry to deallocate in.
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
static HRESULT demap_pages1(PTTB pTTBEntry, PTTBAUX pTTBAuxEntry, UINT32 ndxPage, UINT32 cpg, UINT32 uiFlags)
{
  UINT32 cpgCurrent;      /* number of pages we're mapping */
  PPAGETAB pTab = NULL;   /* pointer to current or new page table */
  HRESULT hr;             /* return from this function */
  register INT32 i;       /* loop counter */

  /* Figure out how many entries we're going to demap. */
  cpgCurrent = SYS_PGTBL_ENTRIES - ndxPage;  /* total free slots on page */
  if (cpg < cpgCurrent)
    cpgCurrent = cpg;     /* only map up to max requested */
  hr = MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_MEMMGR, cpgCurrent);

  if ((pTTBEntry->data & TTBSEC_ALWAYS) && (cpgCurrent == SYS_PGTBL_ENTRIES) && (ndxPage == 0))
  { /* we can kill off the whole section */
    if (pTTBAuxEntry->aux.sacred && !(uiFlags & DEMAP_NOTHING_SACRED))
      return MEMMGR_E_NOSACRED;  /* can't demap a sacred mapping */
    pTTBEntry->data = 0;
    pTTBAuxEntry->data = 0;
    /* TODO: handle TLB and cache */
  }
  else if (pTTBEntry->data & TTBPGTBL_ALWAYS)
  {
    pTab = map_pagetable(pTTBEntry->data & TTBPGTBL_BASE);
    if (!pTab)
      return MEMMGR_E_NOPGTBL;
    for (i = 0; i<cpgCurrent; i++)
    {
      if (pTab->pgaux[ndxPage + i].aux.sacred && !(uiFlags & DEMAP_NOTHING_SACRED))
      { /* can't demap a sacred mapping */
	hr = MEMMGR_E_NOSACRED;
	goto pageError;
      }
    }
    for (i = 0; i<cpgCurrent; i++)
    {
      pTab->pgtbl[ndxPage + i].data = 0;
      pTab->pgaux[ndxPage + i].data = 0;
      /* TODO: handle TLB and cache */
    }
    /* TODO: check to see if page table can be deallocated */
pageError:
    demap_pagetable(pTab);
  }
  return hr;
}

static HRESULT demap_pages0(PTTB pTTB, PTTBAUX pTTBAux, KERNADDR vmaBase, UINT32 cpg, UINT32 uiFlags)
{
  UINT32 ndxTTBMax = (pTTB == g_pttb1) ? SYS_TTB1_ENTRIES : SYS_TTB0_ENTRIES;
  UINT32 ndxTTB = mmVMA2TTBIndex(vmaBase);      /* TTB entry index */
  UINT32 ndxPage = mmVMA2PGTBLIndex(vmaBase);   /* starting page entry index */
  UINT32 cpgRemaining = cpg;                    /* number of pages remaining to demap */
  HRESULT hr;                                   /* temporary result */

  if ((cpgRemaining > 0) && (ndxPage > 0))
  {
    /* We are starting in the middle of a VM page.  Demap to the end of the VM page. */
    hr = demap_pages1(pTTB + ndxTTB, pTTBAux + ndxTTB, ndxPage, cpgRemaining, uiFlags);
    if (FAILED(hr))
      return hr;
    cpgRemaining -= SCODE_CODE(hr);
    if (++ndxTTB == ndxTTBMax)
      return MEMMGR_E_ENDTTB;
  }

  while (cpgRemaining > 0)
  {
    hr = demap_pages1(pTTB + ndxTTB, pTTBAux + ndxTTB, 0, cpgRemaining, uiFlags);
    if (FAILED(hr))
      return hr;
    cpgRemaining -= SCODE_CODE(hr);
    if (++ndxTTB == ndxTTBMax)
      return MEMMGR_E_ENDTTB;
  }
  return S_OK;
}

HRESULT MmDemapPages(PTTB pTTB, PTTBAUX pTTBAux, KERNADDR vmaBase, UINT32 cpg)
{
  return demap_pages0(resolve_ttb(pTTB, vmaBase), resolve_ttbaux(pTTBAux, vmaBase), vmaBase, cpg, 0);
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
  register UINT32 rc = uiPageAuxFlags & (PGAUX_SACRED);
  /* TODO if we define any other flags */
  return rc;
}

static PPAGETAB alloc_page_table(PTTB pttbEntry, PTTBAUX pttbAuxEntry, UINT32 uiTableFlags)
{
  PPAGETAB pTab = NULL;   /* new page table pointer */
  register INT32 i;       /* loop counter */

  /* TODO: pull pTab out of our ass somewhere */
  if (pTab)
  {
    for (i=0; i<SYS_PGTBL_ENTRIES; i++)
    {
      pTab->pgtbl[i].data = 0;  /* blank out the new page table */
      pTab->pgaux[i].data = 0;
    }
    /* TODO: use physical address of page here */
    pttbEntry->data = MmGetPhysAddr(g_pttb1, (KERNADDR)pTab) | uiTableFlags;  /* poke new entry */
    pttbAuxEntry->data = TTBAUXFLAGS_PAGETABLE;
  }
  return pTab;
}

static HRESULT map_pages1(PTTB pttbEntry, PTTBAUX pttbAuxEntry, PHYSADDR paBase, UINT32 ndxPage, UINT32 cpg,
			  UINT32 uiTableFlags, UINT32 uiPageFlags, UINT32 uiAuxFlags)
{
  UINT32 cpgCurrent;      /* number of pages we're mapping */
  PPAGETAB pTab = NULL;   /* pointer to current or new page table */
  HRESULT hr;             /* return from this function */
  register INT32 i;       /* loop counter */

  switch (pttbEntry->data & TTBQUERY_MASK)
  {
    case TTBQUERY_FAULT:   /* not allocated, allocate a new page table for the slot */
      pTab = alloc_page_table(pttbEntry, pttbAuxEntry, uiTableFlags);
      if (!pTab)
	return MEMMGR_E_NOPGTBL;
      break;

    case TTBQUERY_PGTBL:    /* existing page table */
      if ((pttbEntry->data & TTBPGTBL_ALLFLAGS) != uiTableFlags)
	return MEMMGR_E_BADTTBFLG;  /* table flags not compatible */
      pTab = map_pagetable(pttbEntry->data & TTBPGTBL_BASE);
      if (!pTab)
	return MEMMGR_E_NOPGTBL;  /* could not map the page table */
      break;

    case TTBQUERY_SEC:
    case TTBQUERY_PXNSEC:
      /* this is a section, make sure its base address covers this mapping and its flags are compatible */
      if ((pttbEntry->data & TTBSEC_ALLFLAGS) != make_section_flags(uiTableFlags, uiPageFlags))
	return MEMMGR_E_BADTTBFLG;
      if (pttbAuxEntry->data != make_section_aux_flags(uiAuxFlags))
	return MEMMGR_E_BADTTBFLG;
      if ((pttbEntry->data & TTBSEC_BASE) != (paBase & TTBSEC_BASE))
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
  demap_pagetable(pTab);
  return hr;
}

static HRESULT map_pages0(PTTB pTTB, PTTBAUX pTTBAux, PHYSADDR paBase, KERNADDR vmaBase, UINT32 cpg,
			  UINT32 uiTableFlags, UINT32 uiPageFlags, UINT32 uiAuxFlags)
{
  UINT32 ndxTTBMax = (pTTB == g_pttb1) ? SYS_TTB1_ENTRIES : SYS_TTB0_ENTRIES;
  UINT32 ndxTTB = mmVMA2TTBIndex(vmaBase);      /* TTB entry index */
  UINT32 ndxPage = mmVMA2PGTBLIndex(vmaBase);   /* starting page entry index */
  UINT32 cpgRemaining = cpg;                    /* number of pages remaining to map */
  HRESULT hr;                                   /* temporary result */

  if ((cpgRemaining > 0) && (ndxPage > 0))
  {
    /* We are starting in the middle of a VM page.  Map to the end of the VM page. */
    hr = map_pages1(pTTB + ndxTTB, pTTBAux + ndxTTB, paBase, ndxPage, cpgRemaining, uiTableFlags,
		    uiPageFlags, uiAuxFlags);
    if (FAILED(hr))
      return hr;
    cpgRemaining -= SCODE_CODE(hr);
    paBase += (SCODE_CODE(hr) << SYS_PAGE_BITS);
    if (++ndxTTB == ndxTTBMax)
    {
      hr = MEMMGR_E_ENDTTB;
      goto errorExit;
    }
  }

  while (cpgRemaining >= SYS_PGTBL_ENTRIES)
  {
    /* try to map a whole section's worth at a time */
    if ((paBase & TTBSEC_BASE) == paBase)
    {
      /* paBase is section-aligned now as well, we can use a direct 1Mb section mapping */
      switch (pTTB[ndxTTB].data & TTBQUERY_MASK)
      {
	case TTBQUERY_FAULT:   /* unmapped - map the section */
	  pTTB[ndxTTB].data = paBase | make_section_flags(uiTableFlags, uiPageFlags);
	  pTTBAux[ndxTTB].data = make_section_aux_flags(uiAuxFlags);
	  break;

	case TTBQUERY_PGTBL:   /* collided with a page table */
	  hr = MEMMGR_E_COLLIDED;
	  goto errorExit;

	case TTBQUERY_SEC:     /* test existing section */
	case TTBQUERY_PXNSEC:
	  if ((pTTB[ndxTTB].data & TTBSEC_ALLFLAGS) != make_section_flags(uiTableFlags, uiPageFlags))
	  {
	    hr = MEMMGR_E_BADTTBFLG;
	    goto errorExit;
	  }
	  if (pTTBAux[ndxTTB].data != make_section_aux_flags(uiAuxFlags))
	  {
	    hr = MEMMGR_E_BADTTBFLG;
	    goto errorExit;
	  }
	  if ((pTTB[ndxTTB].data & TTBSEC_BASE) != paBase)
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
      hr = map_pages1(pTTB + ndxTTB, pTTBAux + ndxTTB, paBase, 0, cpgRemaining, uiTableFlags, uiPageFlags, uiAuxFlags);
      if (FAILED(hr))
	goto errorExit;
    }
    /* adjust base physical address, page count, and TTB index */
    paBase += (SCODE_CODE(hr) << SYS_PAGE_BITS);
    cpgRemaining -= SCODE_CODE(hr);
    if (++ndxTTB == ndxTTBMax)
    {
      hr = MEMMGR_E_ENDTTB;
      goto errorExit;
    }
  }

  if (cpgRemaining > 0)
  {
    /* map the "tail end" onto the next TTB */
    hr = map_pages1(pTTB + ndxTTB, pTTBAux + ndxTTB, paBase, 0, cpgRemaining, uiTableFlags, uiPageFlags, uiAuxFlags);
    if (FAILED(hr))
      goto errorExit;
  }
  return S_OK;
errorExit:
  /* demap everything we've managed to map thusfar */
  demap_pages0(pTTB, pTTBAux, vmaBase, cpg - cpgRemaining, DEMAP_NOTHING_SACRED);
  return hr;
}

HRESULT MmMapPages(PTTB pTTB, PTTBAUX pTTBAux, PHYSADDR paBase, KERNADDR vmaBase, UINT32 cpg, UINT32 uiTableFlags,
		   UINT32 uiPageFlags, UINT32 uiAuxFlags)
{
  return map_pages0(resolve_ttb(pTTB, vmaBase), resolve_ttbaux(pTTBAux, vmaBase), paBase, vmaBase, cpg,
		    uiTableFlags, uiPageFlags, uiAuxFlags);
}

HRESULT MmMapKernelPages(PTTB pTTB, PTTBAUX pTTBAux, PHYSADDR paBase, UINT32 cpg, UINT32 uiTableFlags,
			 UINT32 uiPageFlags, UINT32 uiAuxFlags, PKERNADDR pvmaLocation)
{
  register HRESULT hr;

  if (!pvmaLocation)
    return E_POINTER;
  *pvmaLocation = _MmAllocKernelAddr(cpg);
  if (!(*pvmaLocation))
    return MEMMGR_E_NOKERNSPC;
  hr = MmMapPages(pTTB, pTTBAux, paBase, *pvmaLocation, cpg, uiTableFlags, uiPageFlags, uiAuxFlags);
  if (FAILED(hr))
  {
    _MmFreeKernelAddr(*pvmaLocation, cpg);
    *pvmaLocation = NULL;
  }
  return hr;
}

HRESULT MmDemapKernelPages(PTTB pTTB, PTTBAUX pTTBAux, KERNADDR vmaBase, UINT32 cpg)
{
  register HRESULT hr;

  if ((vmaBase & 0xC0000000) != 0xC0000000)
    return E_INVALIDARG;
  hr = MmDemapPages(pTTB, pTTBAux, vmaBase, cpg);
  if (SUCCEEDED(hr))
    _MmFreeKernelAddr(vmaBase, cpg);
  return hr;
}

/*---------------------
 * Initialization code
 *---------------------
 */

SEG_INIT_CODE void _MmInitVMMap(PSTARTUP_INFO pstartup, PMALLOC pmInitHeap)
{
  g_pMalloc = pmInitHeap;
  IUnknown_AddRef(g_pMalloc);
  g_pttb1 = (PTTB)(pstartup->kaTTB);
  g_pttb1Aux = (PTTBAUX)(pstartup->kaTTBAux);
  rbtInitTree(&g_rbtPageTables, RbtStdCompareByValue);
}
