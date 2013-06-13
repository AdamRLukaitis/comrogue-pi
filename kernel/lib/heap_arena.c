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
#include <comrogue/internals/mmu.h>
#include "heap_internals.h"

#ifdef _H_THIS_FILE
#undef _H_THIS_FILE
_DECLARE_H_THIS_FILE
#endif

/*----------------------------
 * Arena management functions
 *----------------------------
 */

static SIZE_T bin_info_cbRunSize_calc(PHEAPDATA phd, PARENABININFO pBinInfo, SIZE_T cbMinRun)
{
  /*
  SIZE_T cbPad;
  SIZE_T cbTryRunSize;
  SIZE_T cbGoodRunSize;
  UINT32 nTryRegions;
  UINT32 nGoodRegions;
  UINT32 cbTryHeader;
  UINT32 cbGoodHeader;
  UINT32 ofsTryBitmap;
  UINT32 ofsGoodBitmap;
  UINT32 ofsTryCtx0;
  UINT32 ofsGoodCtx0;
  UINT32 ofsTryRedZone0;
  UINT32 ofsGoodRedZone0;
  */

  _H_ASSERT(phd, cbMinRun >= SYS_PAGE_SIZE);
  _H_ASSERT(phd, cbMinRun <= phd->szArenaMaxClass);


  return 0; /* TODO */
}

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

  /* TODO */
}

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

void _HeapArenaShutdown(PHEAPDATA phd)
{
  /* TODO */
}
