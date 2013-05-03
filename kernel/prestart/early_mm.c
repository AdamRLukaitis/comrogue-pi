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
#define __COMROGUE_PRESTART__
#include <comrogue/types.h>
#include <comrogue/internals/layout.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/startup.h>
#include <comrogue/internals/trace.h>

#ifdef THIS_FILE
#undef THIS_FILE
DECLARE_THIS_FILE
#endif

/*-----------------------------------------------------------------------------
 * Early memory-management code that handles creating the mappings for the TTB
 *-----------------------------------------------------------------------------
 */

/* Data stored in here temporarily and reflected back to startup info when we're done. */
static PTTB g_pTTB;                 /* pointer to TTB */
static PTTBAUX g_pTTBAux;           /* pointer to TTB auxiliary data */
static UINT32 g_cpgForPageTables;   /* number of pages being used for page tables */
static UINT32 g_ctblFreeonLastPage; /* number of page tables free on last page */
static PPAGETAB g_ptblNext;         /* pointer to next free page table */

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

/*
 * Allocates a new page table, initializes it, and initializes the pointed-to TTB entry with a
 * pointer to it.
 *
 * Parameters:
 * - pTTBEntry = Pointer to the TTB entry to be filled; this entry is modified.
 * - pAuxEntry = Pointer to the TTB aux entry to be filled; this entry is modified.
 * - uiTableFlags = Flags to be used for the TTB entry.
 *
 * Returns:
 * A pointer to the new page table.
 *
 * Side effects:
 * Modifies the global variables g_cpgForPageTables, g_ctblFreeonLastPage, and g_ptblNext.
 */
static PPAGETAB alloc_page_table(PTTB pTTBEntry, PTTBAUX pAuxEntry, UINT32 uiTableFlags)
{
  register PPAGETAB pTab;   /* pointer to new page table */
  register UINT32 i;        /* loop counter */

  if (g_ctblFreeonLastPage == 0)
  {
    g_cpgForPageTables++;
    g_ctblFreeonLastPage = 2;
  }
  g_ctblFreeonLastPage--;
  pTab = g_ptblNext++;
  for (i=0; i<SYS_PGTBL_ENTRIES; i++)
  {
    pTab->pgtbl[i].data = 0;  /* blank out the new page table */
    pTab->pgaux[i].data = 0;
  }
  pTTBEntry->data = ((UINT32)pTab) | uiTableFlags;  /* poke new entry */
  pAuxEntry->data = TTBAUXFLAGS_PAGETABLE;
  return pTab;
}

/*
 * Allocates page mapping entries within a single current entry in the TTB.
 *
 * Parameters:
 * - paBase = The page-aligned base physical address to map.
 * - pTTBEntry = Pointer to the TTB entry to be used.
 * - pAuxEntry = Pointer to the TTB auxiliary entry to be used.
 * - ndxPage = The "first" index within the current page to use.
 * - cpg = The maximum number of pages we want to map.  This function will only map as many pages as will
 *         fit in the current TTB entry, as indicated by ndxPage.
 * - uiTableFlags = Flags to be used or verified for the TTB entry.
 * - uiPageFlags = Flags to be used for new page table entries.
 * - uiAuxFlags = Flags to be used for new page table auxiliary entries.
 *
 * Returns:
 * The number of pages that were actually mapped by this function call, or -1 if there was an error in the mapping.
 *
 * Side effects:
 * May modify the TTB entry we point to, if it was not previously allocated.  May modify the current page
 * table that the TTB entry points to, where applicable.  If we need to allocate a new page table, may modify the
 * global variables g_cpgForPageTables, g_ctblFreeonLastPage, and g_ptblNext.
 */
static INT32 alloc_pages(PHYSADDR paBase, PTTB pTTBEntry, PTTBAUX pAuxEntry, INT32 ndxPage,
			 INT32 cpg, UINT32 uiTableFlags, UINT32 uiPageFlags, UINT32 uiAuxFlags)
{
  INT32 cpgCurrent;    /* number of pages we're mapping */
  PPAGETAB pTab;       /* pointer to current or new page table */
  register INT32 i;    /* loop counter */

  switch (pTTBEntry->data & TTBQUERY_MASK)
  {
    case TTBQUERY_FAULT:   /* not allocated, allocate a new page table for the slot */
      pTab = alloc_page_table(pTTBEntry, pAuxEntry, uiTableFlags);
      break;

    case TTBQUERY_PGTBL:    /* existing page table */
      if ((pTTBEntry->data & TTBPGTBL_ALLFLAGS) != uiTableFlags)
	return -1;  /* table flags not compatible */
      pTab = (PPAGETAB)(pTTBEntry->data & TTBPGTBL_BASE);
      break;

    case TTBQUERY_SEC:
    case TTBQUERY_PXNSEC:   /* existing section */
      if ((pTTBEntry->data & TTBSEC_ALLFLAGS) != make_section_flags(uiTableFlags, uiPageFlags))
	return -1;
      if (pAuxEntry->data != make_section_aux_flags(uiAuxFlags))
	return -1;
      if ((pTTBEntry->data & TTBSEC_BASE) != (paBase & TTBSEC_BASE))
	return -1;
      pTab = NULL;
      break;
  }

  /* Figure out how many entries we're going to map. */
  cpgCurrent = SYS_PGTBL_ENTRIES - ndxPage;  /* total free slots on page */
  if (cpg < cpgCurrent)
    cpgCurrent = cpg;     /* only map up to max requested */

  if (pTab)
  { /* fill in entries in the page table */
    for (i=0; i<cpgCurrent; i++)
    {
      if ((pTab->pgtbl[ndxPage + i].data & PGQUERY_MASK) != PGQUERY_FAULT)
	return -1;   /* stepping on existing mapping */
      pTab->pgtbl[ndxPage + i].data = paBase | uiPageFlags;
      pTab->pgaux[ndxPage + i].data = uiAuxFlags;
      paBase += SYS_PAGE_SIZE;
    }
  }
  return cpgCurrent;
}

/*
 * Maps a certain number of memory pages beginning at a specified physical address into virtual memory
 * beginning at a specified virtual address.
 *
 * Parameters:
 * - paBase = The page-aligned base physical address to map.
 * - vmaBase = The page-aligned virtual address to map those pages to.
 * - cpg = The number of pages to be mapped.
 * - uiTableFlags = Flags to be used or verified for TTB entries.
 * - uiPageFlags = Flags to be used for new page table entries.
 * - uiAuxFlags = Flags to be used for new auxiliary TTB entries.
 *
 * Returns:
 * TRUE if the mapping succeeded, FALSE if it failed.
 *
 * Side effects:
 * May modify unallocated entries in the TTB.  May modify any page tables that are pointed to by TTB entries,
 * where applicable.  If we need to allocate new page tables, may modify the global variables g_cpgForPageTables,
 * g_ctblFreeonLastPage, and g_ptblNext.
 */
static BOOL map_pages(PHYSADDR paBase, KERNADDR vmaBase, INT32 cpg, UINT32 uiTableFlags,
		      UINT32 uiPageFlags, UINT32 uiAuxFlags)
{
  static DECLARE_STRING8_CONST(sz1, "Map ");
  static DECLARE_STRING8_CONST(sz2, "->");
  static DECLARE_STRING8_CONST(sz3, ",cpg=");
  static DECLARE_STRING8_CONST(sz4, ",tf="); 
  static DECLARE_STRING8_CONST(sz5, ",pf=");
  static DECLARE_STRING8_CONST(sz6, ",af=");
  INT32 ndxTTB = mmVMA2TTBIndex(vmaBase);       /* TTB entry index */
  INT32 ndxPage = mmVMA2PGTBLIndex(vmaBase);    /* starting page entry index */
  INT32 cpgCurrent;                             /* current number of pages mapped */

  ETrWriteString8(sz1);
  ETrWriteWord(paBase);
  ETrWriteString8(sz2);
  ETrWriteWord(vmaBase);
  ETrWriteString8(sz3);
  ETrWriteWord(cpg);
  ETrWriteString8(sz4);
  ETrWriteWord(uiTableFlags);
  ETrWriteString8(sz5);
  ETrWriteWord(uiPageFlags);
  ETrWriteString8(sz6);
  ETrWriteWord(uiAuxFlags);
  ETrWriteChar8('\n');

  if ((cpg > 0) && (ndxPage > 0))
  {
    /* We are starting in the middle of a VM page.  Map to the end of the VM page. */
    cpgCurrent = alloc_pages(paBase, g_pTTB + ndxTTB, g_pTTBAux + ndxTTB, ndxPage, cpg, uiTableFlags,
			     uiPageFlags, uiAuxFlags);
    if (cpgCurrent < 0)
    {
      /* ETrWriteChar8('a'); */
      return FALSE;
    }
    /* adjust base physical address, page count, and TTB index */
    paBase += (cpgCurrent << SYS_PAGE_BITS);
    cpg -= cpgCurrent;
    ndxTTB++;
    /* N.B.: from this point on ndxPage will be treated as 0 */
  }
  while (cpg >= SYS_PGTBL_ENTRIES)
  {
    /* try to map a whole section's worth at a time */
    if ((paBase & TTBSEC_BASE) == paBase)
    {
      /* paBase is section-aligned now as well, we can use a direct 1Mb section mapping */
      switch (g_pTTB[ndxTTB].data & TTBQUERY_MASK)
      {
	case TTBQUERY_FAULT:   /* unmapped - map the section */
	  g_pTTB[ndxTTB].data = paBase | make_section_flags(uiTableFlags, uiPageFlags);
	  g_pTTBAux[ndxTTB].data = make_section_aux_flags(uiAuxFlags);
	  break;

	case TTBQUERY_PGTBL:   /* collided with a page table */
	  /* ETrWriteChar8('b'); */
	  return FALSE;

	case TTBQUERY_SEC:     /* test existing section */
	case TTBQUERY_PXNSEC:
	  if ((g_pTTB[ndxTTB].data & TTBSEC_ALLFLAGS) != make_section_flags(uiTableFlags, uiPageFlags))
	  {
	    /* ETrWriteChar8('c'); */
	    return FALSE;    /* invalid flags */
	  }
	  if (g_pTTBAux[ndxTTB].data != make_section_aux_flags(uiAuxFlags))
	  {
	    /* ETrWriteChar8('!'); */
	    return FALSE;    /* invalid aux flags */
	  }
	  if ((g_pTTB[ndxTTB].data & TTBSEC_BASE) != paBase)
	  {
	    /* ETrWriteChar8('d'); */
	    return FALSE;    /* invalid base address */
	  }
	  break;
      }
      cpgCurrent = SYS_PGTBL_ENTRIES;  /* we mapped a whole section worth */
    }
    else
    {
      /* just map 256 individual pages */
      cpgCurrent = alloc_pages(paBase, g_pTTB + ndxTTB, g_pTTBAux + ndxTTB, 0, cpg, uiTableFlags,
			       uiPageFlags, uiAuxFlags);
      if (cpgCurrent < 0)
      {
	/* ETrWriteChar8('e'); */
	return FALSE;
      }
    }
    /* adjust base physical address, page count, and TTB index */
    paBase += (cpgCurrent << SYS_PAGE_BITS);
    cpg -= cpgCurrent;
    ndxTTB++;
  }

  if (cpg > 0)
  {
    /* map the "tail end" onto the next TTB */
    if (alloc_pages(paBase, g_pTTB + ndxTTB, g_pTTBAux + ndxTTB, 0, cpg, uiTableFlags, uiPageFlags, uiAuxFlags) < 0)
    {
      /* ETrWriteChar8('f'); */
      return FALSE;
    }
  }
  return TRUE;
}

/* External references to symbols defined by the linker script. */
extern char paFirstFree, cpgPrestartTotal, paLibraryCode, vmaLibraryCode, cpgLibraryCode, paKernelCode,
  vmaKernelCode, cpgKernelCode, paKernelData, vmaKernelData, cpgKernelData, cpgKernelBss, paInitCode,
  vmaInitCode, cpgInitCode, paInitData, vmaInitData, cpgInitData, cpgInitBss, vmaFirstFree;

/*
 * Initializes a TTB (used as TTB1 and initially TTB0 as well) with mappings to all the pieces of the kernel and
 * to memory-mapped IO, and fills in details in the startup info structure.
 *
 * Parameters:
 * - pstartup - Pointer to startup info structure, which is modified by this function.
 *
 * Returns:
 * - Physical address of the new TTB1.
 *
 * Side effects:
 * Modifies physical memory beyond the end of the kernel to store TTB and page tables.  Uses several
 * static globals in this module for work space while performing memory mappings.
 */
PHYSADDR EMmInit(PSTARTUP_INFO pstartup)
{
  static DECLARE_STRING8_CONST(szTTBAt, "EMmInit: TTB1@");
#if 0
  static DECLARE_STRING8_CONST(szPageTable, "Page table pages:");
  static DECLARE_STRING8_CONST(szFree, "\nFree last page:");
#endif
  PHYSADDR paTTB = (PHYSADDR)(&paFirstFree);  /* location of the system TTB1 */
  UINT32 cbMPDB;                              /* number of bytes in the MPDB */
  register INT32 i;                           /* loop counter */

  /* Locate the appropriate place for TTB1, on a 16K boundary. */
  pstartup->cpgTTBGap = 0;
  while (paTTB & (SYS_TTB1_SIZE - 1))
  {
    paTTB += SYS_PAGE_SIZE;
    pstartup->cpgTTBGap++;
  }

  ETrWriteString8(szTTBAt);
  ETrWriteWord(paTTB);
  ETrWriteChar8('\n');

  /* Save off the TTB location and initialize it. */
  pstartup->paTTB = paTTB;
  g_pTTB = (PTTB)paTTB;
  for (i=0; i<SYS_TTB1_ENTRIES; i++)
    g_pTTB[i].data = 0;

  /* Save off the TTB auxiliary data location and initialize it. */
  pstartup->paTTBAux = paTTB + SYS_TTB1_SIZE;
  g_pTTBAux = (PTTBAUX)(pstartup->paTTBAux);
  for (i=0; i<SYS_TTB1_ENTRIES; i++)
    g_pTTBAux[i].data = 0;

  /* Allocate space for the Master Page Database but do not initialize it. */
  pstartup->paMPDB = pstartup->paTTBAux + SYS_TTB1_SIZE;
  cbMPDB = pstartup->cpgSystemTotal << 3;    /* 8 bytes per entry */
  pstartup->cpgMPDB = cbMPDB >> SYS_PAGE_BITS;
  if (cbMPDB & (SYS_PAGE_SIZE - 1))
  {
    pstartup->cpgMPDB++;
    cbMPDB = pstartup->cpgMPDB << SYS_PAGE_BITS;
  }

  /* Initialize the "next page table" pointer. */
  pstartup->paFirstPageTable = pstartup->paMPDB + cbMPDB;
  g_ptblNext = (PPAGETAB)(pstartup->paFirstPageTable);
  g_cpgForPageTables = g_ctblFreeonLastPage = 0;
  
  /* Map the "prestart" area (everything below load address, plus prestart code & data) as identity. */
  VERIFY(map_pages(0, 0, (INT32)(&cpgPrestartTotal), TTBPGTBL_ALWAYS, PGTBLSM_ALWAYS | PGTBLSM_AP01, 0));
  /* Map the IO area as identity. */
  VERIFY(map_pages(PHYSADDR_IO_BASE, PHYSADDR_IO_BASE, PAGE_COUNT_IO, TTBFLAGS_MMIO, PGTBLFLAGS_MMIO, 0));
  /* Map the library area. */
  VERIFY(map_pages((PHYSADDR)(&paLibraryCode), (KERNADDR)(&vmaLibraryCode), (INT32)(&cpgLibraryCode),
                   TTBFLAGS_LIB_CODE, PGTBLFLAGS_LIB_CODE, PGAUXFLAGS_LIB_CODE));
  /* Map the kernel code area. */
  VERIFY(map_pages((PHYSADDR)(&paKernelCode), (KERNADDR)(&vmaKernelCode), (INT32)(&cpgKernelCode),
                   TTBFLAGS_KERNEL_CODE, PGTBLFLAGS_KERNEL_CODE, PGAUXFLAGS_KERNEL_CODE));
  /* Map the kernel data/BSS area. */
  VERIFY(map_pages((PHYSADDR)(&paKernelData), (KERNADDR)(&vmaKernelData),
                   (INT32)(&cpgKernelData) + (INT32)(&cpgKernelBss), TTBFLAGS_KERNEL_DATA, PGTBLFLAGS_KERNEL_DATA,
		   PGAUXFLAGS_KERNEL_DATA));
  /* Map the kernel init code area. */
  VERIFY(map_pages((PHYSADDR)(&paInitCode), (KERNADDR)(&vmaInitCode), (INT32)(&cpgInitCode),
                   TTBFLAGS_INIT_CODE, PGTBLFLAGS_INIT_CODE, PGAUXFLAGS_INIT_CODE));
  /* Map the kernel init data/BSS area. */
  VERIFY(map_pages((PHYSADDR)(&paInitData), (KERNADDR)(&vmaInitData),
                   (INT32)(&cpgInitData) + (INT32)(&cpgInitBss), TTBFLAGS_INIT_DATA, PGTBLFLAGS_INIT_DATA,
		   PGAUXFLAGS_INIT_DATA));
  /* Map the TTB itself. */
  pstartup->kaTTB = (KERNADDR)(&vmaFirstFree);
  VERIFY(map_pages(paTTB, pstartup->kaTTB, SYS_TTB1_SIZE / SYS_PAGE_SIZE, TTBFLAGS_KERNEL_DATA,
		   PGTBLFLAGS_KERNEL_DATA, PGAUXFLAGS_KERNEL_DATA));
  /* Map the TTB auxiliary data. */
  pstartup->kaTTBAux = pstartup->kaTTB + SYS_TTB1_SIZE;
  VERIFY(map_pages(pstartup->paTTBAux, pstartup->kaTTBAux, SYS_TTB1_SIZE / SYS_PAGE_SIZE, TTBFLAGS_KERNEL_DATA,
		   PGTBLFLAGS_KERNEL_DATA, PGAUXFLAGS_KERNEL_DATA));
  /* Map the Master Page Database. */
  pstartup->kaMPDB = pstartup->kaTTBAux + SYS_TTB1_SIZE;
  VERIFY(map_pages(pstartup->paMPDB, pstartup->kaTTB + SYS_TTB1_SIZE, pstartup->cpgMPDB, TTBFLAGS_KERNEL_DATA,
		   PGTBLFLAGS_KERNEL_DATA, PGAUXFLAGS_KERNEL_DATA));
  /* Map the IO area into high memory as well. */
  VERIFY(map_pages(PHYSADDR_IO_BASE, VMADDR_IO_BASE, PAGE_COUNT_IO, TTBFLAGS_MMIO, PGTBLFLAGS_MMIO, PGAUXFLAGS_MMIO));

  /*
   * Allocate one extra page table, just to ensure that we have sufficient free page table entries when we get up
   * to the startup code.
   */
  i = mmVMA2TTBIndex(VMADDR_KERNEL_FENCE);
  while ((g_pTTB[i].data & TTBQUERY_MASK) != TTBQUERY_FAULT)
    i++;
  alloc_page_table(g_pTTB + i, g_pTTBAux + i, TTBFLAGS_KERNEL_DATA);

#if 0
  /* Dump the TTB and page tables to trace output. */
  ETrDumpWords((PUINT32)paTTB, (SYS_TTB1_SIZE + SYS_TTB1_SIZE + (g_cpgForPageTables << SYS_PAGE_BITS)) >> 2);
  ETrWriteString8(szPageTable);
  ETrWriteWord(g_cpgForPageTables);
  ETrWriteString8(szFree);
  ETrWriteWord(g_ctblFreeonLastPage);
  ETrWriteChar8('\n');
#endif

  /* Fill in the rest of the data in the startup info structure. */
  pstartup->cpgPageTables = g_cpgForPageTables;
  pstartup->ctblFreeOnLastPage = g_ctblFreeonLastPage;
  pstartup->paFirstFree = pstartup->paFirstPageTable + (g_cpgForPageTables << SYS_PAGE_BITS);
  pstartup->vmaFirstFree = pstartup->kaMPDB + cbMPDB;

  return paTTB;  /* return this for startup ASM code to use */
}
