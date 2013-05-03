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
#include <comrogue/internals/seg.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/memmgr.h>
#include <comrogue/internals/startup.h>
#include <comrogue/internals/trace.h>

#ifdef THIS_FILE
#undef THIS_FILE
DECLARE_THIS_FILE
#endif

/* Lists we keep track of various pages on. */
typedef struct tagPAGELIST {
  UINT32 ndxLast;                  /* index of last page in list */
  UINT32 cpg;                      /* count of pages in list */
} PAGELIST, *PPAGELIST;

/* The Master Page Database */
static PMPDB g_pMasterPageDB = NULL;
static UINT32 g_cpgMaster = 0;

/* Individual page lists. */
static PAGELIST g_pglFree = { 0, 0 };           /* pages that are free */
static PAGELIST g_pglZeroed = { 0, 0 };         /* pages that are free and zeroed */
//static PAGELIST g_pglStandby = { 0, 0 };        /* pages removed but "in transition" */
//static PAGELIST g_pglModified = { 0, 0 };       /* pages removed but "in transition" and modified */
//static PAGELIST g_pglBad = { 0, 0 };            /* bad pages */

SEG_INIT_DATA static PAGELIST g_pglInit = { 0, 0 };  /* pages to be freed after initialization */

static KERNADDR g_kaZero = 0;                   /* kernel address where we map a page to zero it */

/*
 * Zeroes a page of memory by index.
 *
 * Parameters:
 * - ndxPage = Index of the page to be zeroed.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Specified page is zeroed.  TTB temporarily modified to map and unmap the page in memory.
 */
static void zero_page(UINT32 ndxPage)
{
  HRESULT hr = MmMapPages(NULL, mmPageIndex2PA(ndxPage), g_kaZero, 1, TTBPGTBL_ALWAYS,
			  PGTBLSM_ALWAYS|PGTBLSM_AP01|PGTBLSM_XN, PGAUX_NOTPAGE);
  ASSERT(SUCCEEDED(hr));
  if (SUCCEEDED(hr))
  {
    StrSetMem(g_kaZero, 0, SYS_PAGE_SIZE);
    VERIFY(SUCCEEDED(MmDemapPages(NULL, g_kaZero, 1)));
  }
}

/*
 * Sets the page table entry physical address pointer and the section-mapped flag for a given page.
 *
 * Parameters:
 * - ndxPage = Index of the page to set the PTE and section flag for.
 * - paPTE = Physical address of the page table entry that points to this page.
 * - bIsSection = If TRUE, paPTE is actually the physical address of the TTB section entry that points
 *                to this page.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Updates the MPDB entry indicated by ndxPage.
 */
static void set_pte_address(UINT32 ndxPage, PHYSADDR paPTE, BOOL bIsSection)
{
  g_pMasterPageDB[ndxPage].d.paPTE = paPTE;
  g_pMasterPageDB[ndxPage].d.sectionmap = (bIsSection ? 1 : 0);
}

/*
 * Finds the given page's predecessor in a circular list.
 *
 * Parameters:
 * - ndxPage = Index of the page to find the predecessor of.  Assumes that the page is part of a circular list.
 *
 * Returns:
 * Index of the page's predecessor in the circular list.
 */
static inline UINT32 find_predecessor(UINT32 ndxPage)
{
  register UINT32 i = ndxPage; /* search page index */

  while (g_pMasterPageDB[i].d.next != ndxPage)
    i = g_pMasterPageDB[i].d.next;
  return i;
}

/*
 * Unchains the given page from the circular list it's in.
 *
 * Parameters:
 * - ndxPage = Index of the page to be unchained from the list.
 * - ndxStartForScan = Index of the page to start scanning for the ndxPage page at.  Assumes that this page is
 *                     part of a circular list.
 *
 * Returns:
 * TRUE if the page was successfully unchained, FALSE if not.
 *
 * Side effects:
 * Entries in the MPDB may have their "next" pointer modified.
 */
static BOOL unchain_page(UINT32 ndxPage, UINT32 ndxStartForScan)
{
  register UINT32 i = ndxStartForScan;  /* search page index */

  do
  {
    if (g_pMasterPageDB[i].d.next == ndxPage)
    {
      g_pMasterPageDB[i].d.next = g_pMasterPageDB[ndxPage].d.next;
      return TRUE;
    }
    i = g_pMasterPageDB[i].d.next;
  } while (i != ndxStartForScan);
  return FALSE;
}

/*
 * Removes a page from a list.
 *
 * Parameters:
 * - ppgl = Pointer to page list to remove the page from.
 * - ndxPage = Index of the page to be removed from the list.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Modifies fields of the page list, and possibly links in the MPDB.
 */
static void remove_from_list(PPAGELIST ppgl, UINT32 ndxPage)
{
  if (ppgl->ndxLast == ndxPage)
    ppgl->ndxLast = find_predecessor(ndxPage);
  VERIFY(unchain_page(ndxPage, ppgl->ndxLast));
  if (--ppgl->cpg == 0)
    ppgl->ndxLast = 0;
}

/*
 * Adds a page to the end of a list.
 *
 * Parameters:
 * - ppgl = Pointer to page list to add the page to.
 * - ndxPage = Index of the page to be added to the list.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Modifies fields of the page list, and possibly links in the MPDB.
 */
static void add_to_list(PPAGELIST ppgl, UINT32 ndxPage)
{
  if (ppgl->cpg++ == 0)
      g_pMasterPageDB[ndxPage].d.next = ndxPage;
  else
  {
    g_pMasterPageDB[ndxPage].d.next = g_pMasterPageDB[ppgl->ndxLast].d.next;
    g_pMasterPageDB[ppgl->ndxLast].d.next = ndxPage;
  }
  ppgl->ndxLast = ndxPage;
}

/*
 * Allocates a page off one of our lists.
 *
 * Parameters:
 * - uiFlags = Flags for the page allocation.
 *
 * Returns:
 * INVALID_PAGE if the page could not be allocated, otherwise the index of the allocated page.
 */
static UINT32 allocate_page(UINT32 uiFlags)
{
  UINT32 rc;
  PPAGELIST ppgl = NULL;
  BOOL bZero = FALSE;

  if (uiFlags & PGALLOC_ZERO)
  { /* try zeroed list first, then free (but need to zero afterwards) */
    if (g_pglZeroed.cpg > 0)
      ppgl = &g_pglZeroed;
    else if (g_pglFree.cpg > 0)
    {
      ppgl = &g_pglFree;
      bZero = TRUE;
    }
  }
  else
  { /* try free list first, then zeroed */
    if (g_pglFree.cpg > 0)
      ppgl = &g_pglFree;
    else if (g_pglZeroed.cpg > 0)
      ppgl = &g_pglZeroed;
  }
  /* TODO: apply additional strategy if we don't yet have a page list */

  if (!ppgl)
    return INVALID_PAGE;
  rc = g_pMasterPageDB[ppgl->ndxLast].d.next;  /* take first page on list */
  remove_from_list(ppgl, rc);
  if (bZero)
    zero_page(rc);
  return rc;
}

/*
 * Allocate a memory page and return its physical address.
 *
 * Parameters:
 * - uiFlags = Flags for page allocation.
 * - tag = Tag to give the newly-allocated page.
 * - subtag = Subtag to give the newly-allocated page.
 * - ppaNewPage = Pointer to location  that will receive the physical address of the new page.
 *
 * Returns:
 * Standard HRESULT success/failure indication.
 */
HRESULT MmAllocatePage(UINT32 uiFlags, UINT32 tag, UINT32 subtag, PPHYSADDR ppaNewPage)
{
  register UINT32 ndxPage;  /* index of page to be allocated */

  if (!ppaNewPage)
    return E_POINTER;
  ndxPage = allocate_page(uiFlags);
  if (ndxPage == INVALID_PAGE)
    return E_OUTOFMEMORY;
  g_pMasterPageDB[ndxPage].d.tag = tag;
  g_pMasterPageDB[ndxPage].d.subtag = subtag;
  *ppaNewPage = mmPageIndex2PA(ndxPage);
  return S_OK;
}

/*
 * Frees up a previously-allocated memory page.
 *
 * Parameters:
 * - paPage = Physical address of the page to be freed.
 * - tag = Tag value we expect the page to have.
 * - subtag = Subtag value we expect the page to have.
 *
 * Returns:
 * Standard HRESULT success/failure indication.
 */
HRESULT MmFreePage(PHYSADDR paPage, UINT32 tag, UINT32 subtag)
{
  register UINT32 ndxPage = mmPA2PageIndex(paPage);

  if ((g_pMasterPageDB[ndxPage].d.tag != tag) || (g_pMasterPageDB[ndxPage].d.subtag != subtag))
    return MEMMGR_E_BADTAGS;
  g_pMasterPageDB[ndxPage].d.tag = MPDBTAG_NORMAL;
  g_pMasterPageDB[ndxPage].d.subtag = 0;
  add_to_list(&g_pglFree, ndxPage);
  return S_OK;
}

/*
 * Builds a "chain" of linked pages in the MPDB, setting their tags to known values, and optionally linking
 * them into a page list.
 *
 * Parameters:
 * - ndxFirstPage = First page of the chain to be built.
 * - cpg = Count of pages to include in the chain.
 * - tag = Tag value to give the pages in the chain.
 * - subtag = Subtag value to give the pages in the chain.
 * - ppglAddTo = Pointer to the page list we want to add the new page chain to.  May be NULL.
 *
 * Returns:
 * The index of the first page following the new chain that was built, i.e. the next start point for a chain.
 *
 * Side effects:
 * Modifies the MPDB accordingly.
 */
SEG_INIT_CODE static UINT32 build_page_chain(UINT32 ndxFirstPage, UINT32 cpg, unsigned tag, unsigned subtag,
					     PPAGELIST ppglAddTo)
{
  register UINT32 i;  /* loop counter */

  if (cpg == 0)
    return ndxFirstPage;  /* do nothing */
  for (i=0; i < cpg; i++)
  {
    g_pMasterPageDB[ndxFirstPage + i].d.tag = tag;
    g_pMasterPageDB[ndxFirstPage + i].d.subtag = subtag;
    if (i<(cpg - 1))
      g_pMasterPageDB[ndxFirstPage + i].d.next = ndxFirstPage + i + 1;
  }
  if (ppglAddTo)
  {
    if (ppglAddTo->cpg == 0)
      /* link as a circular list */
      g_pMasterPageDB[ndxFirstPage + cpg - 1].d.next = ndxFirstPage;
    else
    {
      /* link into existing circular list */
      g_pMasterPageDB[ndxFirstPage + cpg - 1].d.next = g_pMasterPageDB[ppglAddTo->ndxLast].d.next;
      g_pMasterPageDB[ppglAddTo->ndxLast].d.next = ndxFirstPage;
    }
    ppglAddTo->ndxLast = ndxFirstPage + cpg - 1;
    ppglAddTo->cpg += cpg;
  }
  return ndxFirstPage + cpg;
}

/* External references to symbols defined by the linker script. */
extern char cpgPrestartTotal, cpgLibraryCode, cpgKernelCode, cpgKernelData, cpgKernelBss, cpgInitCode,
  cpgInitData, cpgInitBss;

/* secondary init function in the VM mapper */
extern void _MmInitPTEMappings(PFNSETPTEADDR pfnSetPTEAddr);

/*
 * Initializes the page allocator and the Master Page Database.
 *
 * Parameters:
 * - pstartup = Pointer to startup information data structure.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Local variables and the Master Page Database initialized.
 */
SEG_INIT_CODE void _MmInitPageAlloc(PSTARTUP_INFO pstartup)
{
  register UINT32 i;        /* loop counter */

  /* Setup the master data pointers and zero the MPDB. */
  g_pMasterPageDB = (PMPDB)(pstartup->kaMPDB);
  g_cpgMaster = pstartup->cpgSystemTotal;
  StrSetMem(g_pMasterPageDB, 0, pstartup->cpgMPDB * SYS_PAGE_SIZE);

  /* Classify all pages in the system and add them to lists. */
  i = build_page_chain(0, 1, MPDBTAG_SYSTEM, MPDBSYS_ZEROPAGE, NULL);
  i = build_page_chain(i, (INT32)(&cpgPrestartTotal) - 1, MPDBTAG_NORMAL, 0, &g_pglFree);
  i = build_page_chain(i, (INT32)(&cpgLibraryCode), MPDBTAG_SYSTEM, MPDBSYS_LIBCODE, NULL);
  i = build_page_chain(i, (INT32)(&cpgKernelCode), MPDBTAG_SYSTEM, MPDBSYS_KCODE, NULL);
  i = build_page_chain(i, (INT32)(&cpgKernelData) + (INT32)(&cpgKernelBss), MPDBTAG_SYSTEM, MPDBSYS_KDATA, NULL);
  i = build_page_chain(i, (INT32)(&cpgInitCode) + (INT32)(&cpgInitData) + (INT32)(&cpgInitBss), MPDBTAG_SYSTEM,
		       MPDBSYS_INIT, &g_pglInit);
  i = build_page_chain(i, pstartup->cpgTTBGap, MPDBTAG_NORMAL, 0, &g_pglFree);
  i = build_page_chain(i, SYS_TTB1_SIZE / SYS_PAGE_SIZE, MPDBTAG_SYSTEM, MPDBSYS_TTB, NULL);
  i = build_page_chain(i, SYS_TTB1_SIZE / SYS_PAGE_SIZE, MPDBTAG_SYSTEM, MPDBSYS_TTBAUX, NULL);
  i = build_page_chain(i, pstartup->cpgMPDB, MPDBTAG_SYSTEM, MPDBSYS_MPDB, NULL);
  i = build_page_chain(i, pstartup->cpgPageTables, MPDBTAG_SYSTEM, MPDBSYS_PGTBL, NULL);
  i = build_page_chain(i, pstartup->cpgSystemAvail - i, MPDBTAG_NORMAL, 0, &g_pglFree);
  i = build_page_chain(i, pstartup->cpgSystemTotal - pstartup->cpgSystemAvail, MPDBTAG_SYSTEM, MPDBSYS_GPU, NULL);
  ASSERT(i == g_cpgMaster);

  /* Initialize the PTE mappings in the MPDB, and the VM mapper's hook function by which it keeps this up to date. */
  _MmInitPTEMappings(set_pte_address);

  /* Allocate the address we map a page to to zero it. */
  g_kaZero = _MmAllocKernelAddr(1);
}
