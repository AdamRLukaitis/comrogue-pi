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
#include <comrogue/scode.h>
#include <comrogue/intlib.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/seg.h>
#include "heap_internals.h"

#ifdef _H_THIS_FILE
#undef _H_THIS_FILE
_DECLARE_H_THIS_FILE
#endif

/* Overhead computations use binary fixed point math. */
#define RUN_BFP                 12    /* binary fixed point for overhead computations */

/*                                    \/---- the fixed decimal point is here */
#define RUN_MAX_OVRHD           0x0000003DU
#define RUN_MAX_OVRHD_RELAX     0x00001800U

/* Lookup table for sorting allocation sizes into bins. */
const SEG_RODATA BYTE abSmallSize2Bin[] =
{
#define S2B_8(x)                       x,
#define S2B_16(x)                      S2B_8(x) S2B_8(x)
#define S2B_32(x)                      S2B_16(x) S2B_16(x)
#define S2B_64(x)                      S2B_32(x) S2B_32(x)
#define S2B_128(x)                     S2B_64(x) S2B_64(x)
#define S2B_256(x)                     S2B_128(x) S2B_128(x)
#define S2B_512(x)                     S2B_256(x) S2B_256(x)
#define S2B_1024(x)                    S2B_512(x) S2B_512(x)
#define S2B_2048(x)                    S2B_1024(x) S2B_1024(x)
#define S2B_4096(x)                    S2B_2048(x) S2B_2048(x)
#define S2B_8192(x)                    S2B_4096(x) S2B_4096(x)
#define SIZE_CLASS(bin, delta, size)   S2B_##delta(bin)
  SIZE_CLASSES
#undef S2B_8
#undef S2B_16
#undef S2B_32
#undef S2B_64
#undef S2B_128
#undef S2B_256
#undef S2B_512
#undef S2B_1024
#undef S2B_2048
#undef S2B_4096
#undef S2B_8192
#undef SIZE_CLASS
};

/*----------------------------
 * Arena management functions
 *----------------------------
 */

/*
 * Returns the pointer to a chunk map within an arena chunk corresponding to a particular page.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the chunk map pointer for.
 *
 * Returns:
 * Pointer to the chunk map within the arena chunk.
 */
PARENACHUNKMAP _HeapArenaMapPGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  _H_ASSERT(phd, ndxPage >= phd->cpgMapBias);
  _H_ASSERT(phd, ndxPage < phd->cpgChunk);
  return &(pChunk->aMaps[ndxPage - phd->cpgMapBias]);
}

/*
 * Returns a pointer to the "bits" field of the chunk map within an arena chunk corresponding to a particular page.
 * This field contains the page's run address and other flags.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the bits pointer for.
 *
 * Returns:
 * Pointer to the chunk map's "bits" field within the arena chunk.
 */
PSIZE_T _HeapArenaMapBitsPGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  return &(_HeapArenaMapPGet(phd, pChunk, ndxPage)->bits);
}

/*
 * Returns the "bits" field of the chunk map within an arena chunk corresponding to a particular page.
 * This field contains the page's run address and other flags.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the bits for.
 *
 * Returns:
 * The chunk map's "bits" field within the arena chunk.
 */
SIZE_T _HeapArenaMapBitsGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  return _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits;
}

/*
 * Returns the size of an unallocated page within a chunk.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the size for.
 *
 * Returns:
 * The size of the unallocated page.
 */
SIZE_T _HeapArenaMapBitsUnallocatedSizeGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  register SIZE_T szMapBits = _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits;
  _H_ASSERT(phd, (szMapBits & (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED)) == 0);
  return szMapBits & ~SYS_PAGE_MASK;
}

/*
 * Returns the size of a page allocated as part of a large allocation within a chunk.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the size for.
 *
 * Returns:
 * The size of the allocated page.
 */
SIZE_T _HeapArenaMapBitsLargeSizeGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  register SIZE_T szMapBits = _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits;
  _H_ASSERT(phd, (szMapBits & (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED)) == (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED));
  return szMapBits & ~SYS_PAGE_MASK;
}

/*
 * Returns the run index of a page used for small allocations within a chunk.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the run index for.
 *
 * Returns:
 * The run index of the allocated page.
 */
SIZE_T _HeapArenaMapBitsSmallRunIndexGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  register SIZE_T szMapBits = _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits;
  _H_ASSERT(phd, (szMapBits & (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED)) == CHUNK_MAP_ALLOCATED);
  return szMapBits >> SYS_PAGE_BITS;
}

/*
 * Returns the bin index of a page within a chunk.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the bin index for.
 *
 * Returns:
 * The bin index of the page.
 */
SIZE_T _HeapArenaMapBitsBinIndexGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  register SIZE_T szMapBits = _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits;
  register SIZE_T ndxBin = (szMapBits & CHUNK_MAP_BININD_MASK) >> CHUNK_MAP_BININD_SHIFT;
  _H_ASSERT(phd, (ndxBin < NBINS) || (ndxBin == BININD_INVALID));
  return ndxBin;
}

/*
 * Returns whether or not a page within a chunk is dirty.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the dirty status for.
 *
 * Returns:
 * - 0 = Page is not dirty.
 * - Other = Page is dirty.
 */
SIZE_T _HeapArenaMapBitsDirtyGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  register SIZE_T szMapBits = _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits;
  return szMapBits & CHUNK_MAP_DIRTY;
}

/*
 * Returns whether or not a page within a chunk is unzeroed.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the dirty status for.
 *
 * Returns:
 * - 0 = Page is zeroed.
 * - Other = Page is unzeroed.
 */
SIZE_T _HeapArenaMapBitsUnzeroedGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  register SIZE_T szMapBits = _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits;
  return szMapBits & CHUNK_MAP_UNZEROED;
}

/*
 * Returns whether or not a page within a chunk is part of a large allocation.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the allocation flag for.
 *
 * Returns:
 * - 0 = Page is not part of a large allocation.
 * - Other = Page is part of a large allocation.
 */
SIZE_T _HeapArenaMapBitsLargeGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  register SIZE_T szMapBits = _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits;
  return szMapBits & CHUNK_MAP_LARGE;
}

/*
 * Returns whether or not a page within a chunk is allocated.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to get the allocation status for.
 *
 * Returns:
 * - 0 = Page is not allocated.
 * - Other = Page is allocated.
 */
SIZE_T _HeapArenaMapBitsAllocatedGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage)
{
  register SIZE_T szMapBits = _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits;
  return szMapBits & CHUNK_MAP_ALLOCATED;
}

/*
 * Sets the size/flags bits of the given page to be "unallocated."
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to set the status for.
 * - sz = Size to set for the page.
 * - szFlags = Combination of the "dirty" and "unzeroed" flags to set for the page.
 *
 * Returns:
 * Nothing.
 */
void _HeapArenaMapBitsUnallocatedSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T sz, SIZE_T szFlags)
{
  _H_ASSERT(phd, (sz & SYS_PAGE_MASK) == 0);
  _H_ASSERT(phd, (szFlags & ~CHUNK_MAP_FLAGS_MASK) == 0);
  _H_ASSERT(phd, (szFlags & (CHUNK_MAP_DIRTY|CHUNK_MAP_UNZEROED)) == szFlags);
  _HeapArenaMapPGet(phd, pChunk, ndxPage)->bits = sz | CHUNK_MAP_BININD_INVALID | szFlags;
}

/*
 * Sets the size of the given unallocated page.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to set the size for.
 * - sz = Size to set for the page.
 *
 * Returns:
 * Nothing.
 */
void _HeapArenaMapBitsUnallocatedSizeSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T sz)
{
  register PSIZE_T pMapBits = _HeapArenaMapBitsPGet(phd, pChunk, ndxPage);
  _H_ASSERT(phd, (sz & SYS_PAGE_MASK) == 0);
  _H_ASSERT(phd, (*pMapBits & (CHUNK_MAP_LARGE|CHUNK_MAP_ALLOCATED)) == 0);
  *pMapBits = sz | (*pMapBits & SYS_PAGE_MASK);
}

/*
 * Sets the size/flags bits of the given page to be "allocated as part of a large allocation."
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to set the status for.
 * - sz = Size to set for the page.
 * - szFlags = May be either CHUNK_MAP_DIRTY or 0, to set the page "dirty" flag.
 *
 * Returns:
 * Nothing.
 */
void _HeapArenaMapBitsLargeSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T sz, SIZE_T szFlags)
{
  register PSIZE_T pMapBits = _HeapArenaMapBitsPGet(phd, pChunk, ndxPage);
  _H_ASSERT(phd, (sz & SYS_PAGE_MASK) == 0);
  _H_ASSERT(phd, (szFlags & CHUNK_MAP_DIRTY) == szFlags);
  /* preserve the existing "unzeroed" flag */
  *pMapBits = sz | CHUNK_MAP_BININD_INVALID | szFlags | (*pMapBits & CHUNK_MAP_UNZEROED) | CHUNK_MAP_LARGE
                 | CHUNK_MAP_ALLOCATED;
}

/*
 * Sets the bin index of the given page that is allocated as part of a large allocation.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to set the bin index for.
 * - ndxBin = Index of the bin to set into the page.
 *
 * Returns:
 * Nothing.
 */
void _HeapArenaMapBitsLargeBinIndSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T ndxBin)
{
  register PSIZE_T pMapBits;
  _H_ASSERT(phd, ndxBin <= BININD_INVALID);
  pMapBits = _HeapArenaMapBitsPGet(phd, pChunk, ndxPage);
  _H_ASSERT(phd, _HeapArenaMapBitsLargeSizeGet(phd, pChunk, ndxPage) == SYS_PAGE_SIZE);
  *pMapBits = (*pMapBits & ~CHUNK_MAP_BININD_MASK) | (ndxBin << CHUNK_MAP_BININD_SHIFT);
}

/*
 * Sets the run/flags bits of the given page to mark it as part of a "small" allocation.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to set the status for.
 * - ndxRun = Index of the run the page belongs to.
 * - ndxBin = Index of the bin to set into the page.
 * - szFlags = May be either CHUNK_MAP_DIRTY or 0, to set the page "dirty" flag.
 *
 * Returns:
 * Nothing.
 */
void _HeapArenaMapBitsSmallSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T ndxRun, SIZE_T ndxBin,
			       SIZE_T szFlags)
{
  register PSIZE_T pMapBits = _HeapArenaMapBitsPGet(phd, pChunk, ndxPage);
  _H_ASSERT(phd, ndxBin < BININD_INVALID);
  _H_ASSERT(phd, ndxPage - ndxRun >= phd->cpgMapBias);
  _H_ASSERT(phd, (szFlags & CHUNK_MAP_DIRTY) == szFlags);
  /* preserve the existing "unzeroed" flag */
  *pMapBits = (ndxRun << SYS_PAGE_BITS) | (ndxBin << CHUNK_MAP_BININD_SHIFT) | szFlags
            | (*pMapBits & CHUNK_MAP_UNZEROED) | CHUNK_MAP_ALLOCATED;
}

/*
 * Sets the unzeroed bit of the given page.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pChunk = Pointer to the arena chunk.
 * - ndxPage = Index of the page to set the status for.
 * - szUnzeroed = Either 0 or CHUNK_MAP_UNZEROED, to set the status flag.
 *
 * Returns:
 * Nothing.
 */
void _HeapArenaMapBitsUnzeroedSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T szUnzeroed)
{
  register PSIZE_T pMapBits = _HeapArenaMapBitsPGet(phd, pChunk, ndxPage);
  *pMapBits = (*pMapBits & ~CHUNK_MAP_UNZEROED) | szUnzeroed;
}

/*
 * Adds a byte count to the arena's profile accumulator, triggering some behavior if the profile accumulator
 * rolls over the interval count.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pArena = Pointer to the arena to add a count to.
 * - cbAccum = Number of bytes to add to the accumulator.
 *
 * Returns:
 * - TRUE = The accumulator rolled over the profile interval and was adjusted.
 * - FALSE = The accumulator did not roll over the profile interval.
 */
BOOL _HeapArenaProfAccumImpl(PHEAPDATA phd, PARENA pArena, UINT64 cbAccum)
{
  /* XXX assert the profile interval is non-zero */
  pArena->cbProfAccum += cbAccum;
  /* XXX
    if (pArena->cbProfAccum >= phd->prof_interval)
    {
      pArena->cbProfAccum -= phd->prof_interval;
      return TRUE;
    }
   */
  return FALSE;
}

/*
 * Adds a byte count to the arena's profile accumulator, triggering some behavior if the profile accumulator
 * rolls over the interval count.  Assumes the arena mutex is locked.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pArena = Pointer to the arena to add a count to.
 * - cbAccum = Number of bytes to add to the accumulator.
 *
 * Returns:
 * - TRUE = The accumulator rolled over the profile interval and was adjusted.
 * - FALSE = The accumulator did not roll over the profile interval, or profiling was not enabled.
 */
BOOL _HeapArenaProfAccumLocked(PHEAPDATA phd, PARENA pArena, UINT64 cbAccum)
{
  return FALSE; /* XXX if phd->prof_interval == 0 */
  /* XXX return _HeapArenaProfAccumImpl(phd, pArena, cbAccum); */
}

/*
 * Adds a byte count to the arena's profile accumulator, triggering some behavior if the profile accumulator
 * rolls over the interval count.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pArena = Pointer to the arena to add a count to.
 * - cbAccum = Number of bytes to add to the accumulator.
 *
 * Returns:
 * - TRUE = The accumulator rolled over the profile interval and was adjusted.
 * - FALSE = The accumulator did not roll over the profile interval, or profiling was not enabled.
 */
BOOL _HeapArenaProfAccum(PHEAPDATA phd, PARENA pArena, UINT64 cbAccum)
{
  return FALSE; /* XXX if phd->prof_interval == 0 */
  /* XXX
     {
       BOOL rc;
       IMutex_Lock(pArena->pmtxLock);
       rc = _HeapArenaProfAccumImpl(phd, pArena, cbAccum);
       IMutex_Unlock(pArena->pmtxLock);
       return rc;
     }
   */
}

/*
 * Returns the bin index of the given page the pointer is on used for small allocations.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pv = Pointer to allocation to get the bin index for.
 * - szMapBits = Map bits for the page the pointer points to.
 *
 * Returns:
 * The associated bin index.
 */
SIZE_T _HeapArenaPtrSmallBinIndGet(PHEAPDATA phd, PCVOID pv, SIZE_T szMapBits)
{
  SIZE_T ndxBin = (szMapBits & CHUNK_MAP_BININD_MASK) >> CHUNK_MAP_BININD_SHIFT;

#if 0 /* debugging code */
  {
    PARENACHUNK pChunk;       /* pointer to enclosing chunk */
    PARENA pArena;            /* pointer to arena for the chunk */
    SIZE_T ndxPage;           /* calculated page index */
    SIZE_T szMapBitsActual;   /* actual retrieved map bits */
    PARENARUN pRun;           /* pointer to run for this allocation */
    PARENABIN pBin;           /* pointer to bin for this allocation */
    SIZE_T ndxBinActual;      /* actual bin index for this allocation */
    PARENABININFO pBinInfo;   /* pointer to bin info */

    _H_ASSERT(phd, ndxBin != BININD_INVALID);
    _H_ASSERT(phd, ndxBin < NBINS);
    pChunk = (PARENACHUNK)CHUNK_ADDR2BASE(phd, pv);
    pArena = pChunk->parena;
    ndxPage = ((UINT_PTR)pv - (UINT_PTR)pChunk) >> SYS_PAGE_BITS;
    szMapBitsActual = _HeapArenaMapBitsGet(phd, pChunk, ndxPage);
    _H_ASSERT(phd, szMapBits == szMapBitsActual);
    _H_ASSERT(phd, _HeapArenaMapBitsLargeGet(phd, pChunk, ndxPage) == 0);
    _H_ASSERT(phd, _HeapArenaMapBitsAllocatedGet(phd, pChunk, ndxPage) != 0);
    pRun = (PARENARUN)((UINT_PTR)pChunk + (UINT_PTR)((ndxPage - (szMapBitsActual >> SYS_PAGE_SIZE)) << SYS_PAGE_SIZE));
    pBin = pRun->pBin;
    ndxBinActual = pBin - pArena->aBins;
    _H_ASSERT(phd, ndxBin == ndxBinActual);
    pBinInfo = &(phd->aArenaBinInfo[ndxBinActual]);
    _H_ASSERT(phd, ((UINT_PTR)pv - ((UINT_PTR)pRun + (UINT_PTR)(pBinInfo->ofsRegion0))) % pBinInfo->cbInterval == 0);
  }
#endif

  return ndxBin;
}

/*
 * Returns the index of a specific bin within an arena.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - pArena = Pointer to the arena.
 * - pBin = Pointer to the bin within the arena.
 *
 * Returns:
 * Index of the bin within the arena.
 */
SIZE_T _HeapArenaBinIndex(PHEAPDATA phd, PARENA pArena, PARENABIN pBin)
{
  SIZE_T ndxBin = pBin - pArena->aBins;  /* return from this function */
  _H_ASSERT(phd, ndxBin < NBINS);
  return ndxBin;
}

UINT32 _HeapArenaRunRegInd(PHEAPDATA phd, PARENARUN pRun, PARENABININFO pBinInfo, PCVOID pv)
{
  UINT32 ofsDiff;        /* offset between pointer and start of run */
  SIZE_T cbInterval;     /* interval between regions */
  UINT32 nShift;         /* shift for bit scaling */
  UINT32 nRegionIndex;   /* region index - return from this function */

  /* Don't free a pointer lower than Region 0. */
  _H_ASSERT(phd, (UINT_PTR)pv >= ((UINT_PTR)pRun + (UINT_PTR)(pBinInfo->ofsRegion0)));

  ofsDiff = (UINT32)((UINT_PTR)pv - (UINT_PTR)pRun - pBinInfo->ofsRegion0);

  /* rescale diff and interval by powers of two */
  cbInterval = pBinInfo->cbInterval;
  nShift = IntFirstSet(cbInterval) - 1;
  ofsDiff >>= nShift;
  cbInterval >>= nShift;

  if (cbInterval == 1)
  {
    nRegionIndex = ofsDiff;  /* divisor was a power of 2 */
  }
  else
  { /* compute ofsDiff / cbInterval by using multiply and right-shift with a lookup table */
#define SIZE_INV_SHIFT   ((sizeof(UINT32) << 3) - LG_RUN_MAXREGS)
#define SIZE_INV(s)      (((1U << SIZE_INV_SHIFT) / (s)) + 1)
    static const UINT32 SEG_RODATA acbIntervalInvs[] =
    {
      SIZE_INV(3),
      SIZE_INV(4), SIZE_INV(5), SIZE_INV(6), SIZE_INV(7),
      SIZE_INV(8), SIZE_INV(9), SIZE_INV(10), SIZE_INV(11),
      SIZE_INV(12), SIZE_INV(13), SIZE_INV(14), SIZE_INV(15),
      SIZE_INV(16), SIZE_INV(17), SIZE_INV(18), SIZE_INV(19),
      SIZE_INV(20), SIZE_INV(21), SIZE_INV(22), SIZE_INV(23),
      SIZE_INV(24), SIZE_INV(25), SIZE_INV(26), SIZE_INV(27),
      SIZE_INV(28), SIZE_INV(29), SIZE_INV(30), SIZE_INV(31)
    };

    if (cbInterval <= ((sizeof(acbIntervalInvs) / sizeof(UINT32)) + 2))
      nRegionIndex = (ofsDiff * acbIntervalInvs[cbInterval - 3]) >> SIZE_INV_SHIFT;
    else
      nRegionIndex = ofsDiff / cbInterval;
#undef SIZE_INV
#undef SIZE_INV_SHIFT
  }

  _H_ASSERT(phd, ofsDiff == nRegionIndex * cbInterval);
  _H_ASSERT(phd, nRegionIndex < pBinInfo->nRegions);

  return nRegionIndex;
}

PVOID _HeapArenaMalloc(PHEAPDATA phd, PARENA pArena, SIZE_T sz, BOOL fZero, BOOL fTryTCache)
{
  return NULL; /* TODO */
}

SIZE_T _HeapArenaSAlloc(PHEAPDATA phd, PCVOID pv, BOOL fDemote)
{
  return 0; /* TODO */
}

void _HeapArenaDAlloc(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PCVOID pv, BOOL fTryTCache)
{
  /* TODO */
}

void _HeapArenaPurgeAll(PHEAPDATA phd, PARENA pArena)
{
  /* TODO */
}

void _HeapArenaTCacheFillSmall(PHEAPDATA phd, PARENA pArena, PTCACHEBIN ptbin, SIZE_T ndxBin, UINT64 cbProfAccum)
{
  /* TODO */
}

void _HeapArenaAllocJunkSmall(PHEAPDATA phd, PVOID pv, PARENABININFO pBinInfo, BOOL fZero)
{
  /* TODO */
}

void _HeapArenaDAllocJunkSmall(PHEAPDATA phd, PVOID pv, PARENABININFO pBinInfo)
{
  /* TODO */
}

PVOID _HeapArenaMallocSmall(PHEAPDATA phd, PARENA pArena, SIZE_T sz, BOOL fZero)
{
  return NULL; /* TODO */
}

PVOID _HeapArenaMallocLarge(PHEAPDATA phd, PARENA pArena, SIZE_T sz, BOOL fZero)
{
  return NULL; /* TODO */
}

PVOID _HeapArenaPalloc(PHEAPDATA phd, PARENA pArena, SIZE_T sz, SIZE_T szAlignment, BOOL fZero)
{
  return NULL; /* TODO */
}

void _HeapArenaProfPromoted(PHEAPDATA phd, PCVOID pv, SIZE_T sz)
{
  /* TODO */
}

void _HeapArenaDAllocBinLocked(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv, PARENACHUNKMAP pMapElement)
{
  /* TODO */
}

void _HeapArenaDAllocBin(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv, PARENACHUNKMAP pMapElement)
{
  /* TODO */
}

void _HeapArenaDAllocSmall(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv, SIZE_T ndxPage)
{
  /* TODO */
}

void _HeapArenaDAllocLargeLocked(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv)
{
  /* TODO */
}

void _HeapArenaDAllocLarge(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv)
{
  /* TODO */
}

PVOID _HeapArenaRAllocNoMove(PHEAPDATA phd, PVOID pv, SIZE_T szOld, SIZE_T sz, SIZE_T szExtra, BOOL fZero)
{
  return NULL; /* TODO */
}

PVOID _HeapArenaRAlloc(PHEAPDATA phd, PVOID pv, SIZE_T szOld, SIZE_T sz, SIZE_T szExtra, SIZE_T szAlignment,
		       BOOL fZero, BOOL fTryTCacheAlloc, BOOL fTryTCacheDAlloc)
{
  return NULL; /* TODO */
}

void _HeapArenaStatsMerge(PHEAPDATA phd, PARENA pArena, PPCCHAR dss, PSIZE_T pnActive, PSIZE_T pnDirty,
			  PARENASTATS pArenaStats, PMALLOCBINSTATS pBinStats, PMALLOCLARGESTATS pLargeStats)
{
  /* TODO */
}

BOOL _HeapArenaNew(PHEAPDATA phd, PARENA pArena, UINT32 ndx)
{
  return FALSE; /* TODO */
}

/*
 * Calculate the run size and other key pieces of data for an arena bin based on its region size.
 *
 * Parameters:
 * - phd = Pointer to HEAPDATA block.
 * - pBinInfo = Pointer to arena bin information being set up.
 * - cbMinRun = Minimum size in bytes of a run; this will always be a multiple of SYS_PAGE_SIZE.
 *
 * Returns:
 * The size of a run for this bin, which will be used as a starting point for the next bin calculation.
 */
static SIZE_T bin_info_cbRunSize_calc(PHEAPDATA phd, PARENABININFO pBinInfo, SIZE_T cbMinRun)
{
  SIZE_T cbPad;               /* bytes of padding required */
  SIZE_T cbTryRunSize;        /* trial for pBinInfo->cbRunSize */
  SIZE_T cbGoodRunSize;       /* good value for pBinInfo->cbRunSize */
  UINT32 nTryRegions;         /* trial for pBinInfo->nRegions */
  UINT32 nGoodRegions;        /* good value for pBinInfo->nRegions */
  UINT32 cbTryHeader;         /* trial for header data size */
  UINT32 cbGoodHeader;        /* good value for header data size */
  UINT32 ofsTryBitmap;        /* trial for pBinInfo->ofsBitmap */
  UINT32 ofsGoodBitmap;       /* good value for pBinInfo->ofsBitmap */
  UINT32 ofsTryCtx0;          /* trial for pBinInfo->ofsCtx0 */
  UINT32 ofsGoodCtx0;         /* good value for pBinInfo->ofsCtx0 */
  UINT32 ofsTryRedZone0;      /* trial for first red zone offset */
  UINT32 ofsGoodRedZone0;     /* good value for first red zone offset */

  _H_ASSERT(phd, cbMinRun >= SYS_PAGE_SIZE);
  _H_ASSERT(phd, cbMinRun <= phd->szArenaMaxClass);

  /*
   * Determine red zone size based on minimum alignment and minimum red zone size; add padding
   * to the end if needed to align regions.
   */
  if (phd->uiFlags & PHDFLAGS_REDZONE)
  {
    SIZE_T cbAlignMin = 1 << (IntFirstSet(pBinInfo->cbRegions) - 1);
    if (cbAlignMin <= REDZONE_MINSIZE)
    {
      pBinInfo->cbRedzone = REDZONE_MINSIZE;
      cbPad = 0;
    }
    else
    {
      pBinInfo->cbRedzone = cbAlignMin >> 1;
      cbPad = pBinInfo->cbRedzone;
    }
  }
  else
  {
    pBinInfo->cbRedzone = 0;
    cbPad = 0;
  }
  pBinInfo->cbInterval = pBinInfo->cbRegions + (pBinInfo->cbRedzone << 1);

  /*
   * Calculate known-valid settings before entering the cbRunSize expansion loop, so the first part of the loop
   * always copies valid settings.  Since the header's mask length and the number of regions depend on each other,
   * trying to calculate it in a non-iterative fashion would be too gnarly.
   */
  cbTryRunSize = cbMinRun;
  nTryRegions = ((cbTryRunSize - sizeof(ARENARUN)) / pBinInfo->cbInterval) + 1; /* will be decremented early on */
  if (nTryRegions > RUN_MAXREGS)
    nTryRegions = RUN_MAXREGS + 1;  /* will be decremented early on */
  do
  {
    nTryRegions--;
    cbTryHeader = sizeof(ARENARUN);
    cbTryHeader = LONG_CEILING(cbTryHeader);  /* pad to long integer boundary */
    ofsTryBitmap = cbTryHeader;
    cbTryHeader += _HeapBitmapSize(nTryRegions); /* add bitmap space */
    ofsTryCtx0 = 0;  /* XXX not using profiling */
    ofsTryRedZone0 = cbTryRunSize - (nTryRegions * pBinInfo->cbInterval) - cbPad;

  } while (cbTryHeader > ofsTryRedZone0);
  
  /* Run size expansion loop */
  do
  { /* Copy valid settings before trying more aggressive ones. */
    cbGoodRunSize = cbTryRunSize;
    nGoodRegions = nTryRegions;
    cbGoodHeader = cbTryHeader;
    ofsGoodBitmap = ofsTryBitmap;
    ofsGoodCtx0 = ofsTryCtx0;
    ofsGoodRedZone0 = ofsTryRedZone0;

    /* Try more aggressive settings. */
    cbTryRunSize += SYS_PAGE_SIZE;
    nTryRegions = ((cbTryRunSize - sizeof(ARENARUN) - cbPad) / pBinInfo->cbInterval) + 1; /* will be decremented */
    if (nTryRegions > RUN_MAXREGS)
      nTryRegions = RUN_MAXREGS + 1;  /* will be decremented early on */
    do
    {
      nTryRegions--;
      cbTryHeader = sizeof(ARENARUN);
      cbTryHeader = LONG_CEILING(cbTryHeader);  /* pad to long integer boundary */
      ofsTryBitmap = cbTryHeader;
      cbTryHeader += _HeapBitmapSize(nTryRegions); /* add bitmap space */
      ofsTryCtx0 = 0;  /* XXX not using profiling */
      ofsTryRedZone0 = cbTryRunSize - (nTryRegions * pBinInfo->cbInterval) - cbPad;

    } while (cbTryHeader > ofsTryRedZone0);

  } while (   (cbTryRunSize <= phd->szArenaMaxClass)
           && (RUN_MAX_OVRHD * (pBinInfo->cbInterval << 3) > RUN_MAX_OVRHD_RELAX)
	   && ((ofsTryRedZone0 << RUN_BFP) > RUN_MAX_OVRHD * cbTryRunSize)
	   && (nTryRegions < RUN_MAXREGS));

  _H_ASSERT(phd, cbGoodHeader <= ofsGoodRedZone0);

  /* Copy the final settings. */
  pBinInfo->cbRunSize = cbGoodRunSize;
  pBinInfo->nRegions = nGoodRegions;
  pBinInfo->ofsBitmap = ofsGoodBitmap;
  pBinInfo->ofsCtx0 = ofsGoodCtx0;
  pBinInfo->ofsRegion0 = ofsGoodRedZone0 + pBinInfo->cbRedzone;

  _H_ASSERT(phd, pBinInfo->ofsRegion0 - pBinInfo->cbRedzone + (pBinInfo->nRegions * pBinInfo->cbInterval)
                                      + cbPad == pBinInfo->cbRunSize);

  return cbGoodRunSize;
}

/*
 * Initialize the arena bin information structures in the heap data block.
 *
 * Parameters:
 * - phd = Pointer to HEAPDATA block.
 *
 * Returns:
 * Nothing.
 */
static void bin_info_init(PHEAPDATA phd)
{
  PARENABININFO pBinInfo;             /* pointer to arena bin information */
  SIZE_T szPrevRun = SYS_PAGE_SIZE;   /* previous run size */

  /* Initialize all the bins. */
#define SIZE_CLASS(bin, delta, size) \
  pBinInfo = &(phd->aArenaBinInfo[bin]); \
  pBinInfo->cbRegions = size; \
  szPrevRun = bin_info_cbRunSize_calc(phd, pBinInfo, szPrevRun); \
  _HeapBitmapInfoInit(&(pBinInfo->bitmapinfo), pBinInfo->nRegions);
  SIZE_CLASSES
#undef SIZE_CLASS
}

/*
 * Set up the heap arena information.
 *
 * Parameters:
 * - phd = Pointer to HEAPDATA block.
 *
 * Returns:
 * Standard HRESULT success/failure indicator.
 */
HRESULT _HeapArenaSetup(PHEAPDATA phd)
{
  SIZE_T szHeader;   /* size of the header */
  UINT32 i;          /* loop counter */

  phd->cpgMapBias = 0;
  for (i = 0; i < 3; i++)
  {
    szHeader = sizeof(ARENACHUNK) + (sizeof(ARENACHUNKMAP) * (phd->cpgChunk - phd->cpgMapBias));
    phd->cpgMapBias = (szHeader >> SYS_PAGE_BITS) + ((szHeader & SYS_PAGE_MASK) != 0);
  }

  _H_ASSERT(phd, phd->cpgMapBias > 0);
  phd->szArenaMaxClass = phd->szChunk - (phd->cpgMapBias << SYS_PAGE_BITS);
  bin_info_init(phd);
  return S_OK;
}

/*
 * Shut down the heap arena information.
 *
 * Parameters:
 * - phd = Pointer to HEAPDATA block.
 *
 * Returns:
 * Nothing.
 */
void _HeapArenaShutdown(PHEAPDATA phd)
{
  /* TODO */
}
