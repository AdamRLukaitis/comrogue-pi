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

/*------------------------------
 * Thread-level cache functions
 *------------------------------
 */

/*
 * Record a thread cache event, which may cause a garbage collection.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - ptcache = Pointer to thread cache to record an event for.
 *
 * Returns:
 * Nothing.
 */
void _HeapTCacheEvent(PHEAPDATA phd, PTCACHE ptcache)
{
  if (TCACHE_GC_INCR == 0)
    return;
  ptcache->cEvents++;
  _H_ASSERT(phd, ptcache->cEvents <= TCACHE_GC_INCR);
  if (ptcache->cEvents == TCACHE_GC_INCR)
    _HeapTCacheEventHard(phd, ptcache);
}

/*
 * Destroys the current thread's cache if it exists.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 *
 * Returns:
 * Nothing.
 */
void _HeapTCacheFlush(PHEAPDATA phd)
{
  PTCACHE ptcache = NULL; /* pointer to thread cache */

  IThreadLocal_Get(phd->pthrlTCache, (PPVOID)(&ptcache));
  if (ptcache)
    IThreadLocal_Set(phd->pthrlTCache, NULL);
  if (ptcache > TCACHE_STATE_MAX)
    _HeapTCacheDestroy(phd, ptcache);
}

/*
 * Returns whether the thread cache is enabled for this thread.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * 
 * Returns:
 * - TRUE = If the thread cache is enabled for this thread.
 * - FALSE = If the thread cache is disabled for this thread.
 */
BOOL _HeapTCacheIsEnabled(PHEAPDATA phd)
{
  TCACHE_ENABLE te;  /* enable state */

  IThreadLocal_Get(phd->pthrlTCacheEnable, (PPVOID)(&te));
  if (te == TCACHE_ENABLE_DEFAULT)
  {
    te = (phd->uiFlags & PHDFLAGS_NOTCACHE) ? TCACHE_DISABLED : TCACHE_ENABLED;
    IThreadLocal_Set(phd->pthrlTCacheEnable, (PVOID)te);
  }

  return MAKEBOOL(te);
}

/*
 * Retrieve the thread cache for this thread, creating it if necessary and desired.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - fCreate = TRUE to create the thread cache if it does not exist, FALSE to skip that step.
 *
 * Returns:
 * - NULL = The cache was disabled or not created.
 * - Other = Pointer to the thread cache.
 */
PTCACHE _HeapTCacheGet(PHEAPDATA phd, BOOL fCreate)
{
  PTCACHE ptcache = NULL; /* pointer to thread cache */

  IThreadLocal_Get(phd->pthrlTCache, (PPVOID)(&ptcache));
  if ((UINT_PTR)ptcache <= (UINT_PTR)TCACHE_STATE_MAX)
  {
    if (ptcache == TCACHE_STATE_DISABLED)
      return NULL;  /* cache disabled */
    if (ptcache == NULL)
    {
      if (!fCreate)
	return NULL;
      if (!_HeapTCacheIsEnabled(phd))
      { /* disabled, record that fact */
	_HeapTCacheSetEnabled(phd, FALSE);
	return NULL;
      }
      return _HeapTCacheCreate(phd, _HeapChooseArena(phd, NULL));
    }
    if (ptcache == TCACHE_STATE_PURGATORY)
    {
      IThreadLocal_Set(phd->pthrlTCache, (PVOID)TCACHE_STATE_REINCARNATED);
      return NULL;
    }
    if (ptcache == TCACHE_STATE_REINCARNATED)
      return NULL;
    /* NOT REACHED */
  }

  return ptcache;
}

/*
 * Set the enable state of the thread cache for this thread.
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - fEnabled = TRUE to enable the cache, FALSE to disable it.
 *
 * Returns:
 * Nothing.
 */
void _HeapTCacheSetEnabled(PHEAPDATA phd, BOOL fEnabled)
{
  TCACHE_ENABLE te;  /* enable state */
  PTCACHE ptcache = NULL; /* pointer to thread cache */

  te = fEnabled ? TCACHE_ENABLED : TCACHE_DISABLED;
  IThreadLocal_Set(phd->pthrlTCacheEnable, (PVOID)te);
  IThreadLocal_Get(phd->pthrlTCache, (PPVOID)(&ptcache));
  if (fEnabled)
  {
    if (ptcache == TCACHE_STATE_DISABLED)
      IThreadLocal_Set(phd->pthrlTCache, NULL);
  }
  else
  {
    if (ptcache > TCACHE_STATE_MAX)
    {
      _HeapTCacheDestroy(phd, ptcache);
      ptcache = NULL;
    }
    if (ptcache == NULL)
      IThreadLocal_Set(phd->pthrlTCache, (PVOID)TCACHE_STATE_DISABLED);
  }
}

PVOID _HeapTCacheAllocEasy(PHEAPDATA phd, PTCACHEBIN ptbin)
{
  return NULL; /* TODO */
}

PVOID _HeapTCacheAllocSmall(PHEAPDATA phd, PTCACHE ptcache, SIZE_T sz, BOOL fZero)
{
  return NULL; /* TODO */
}

PVOID _HeapTCacheAllocLarge(PHEAPDATA phd, PTCACHE ptcache, SIZE_T sz, BOOL fZero)
{
  return NULL; /* TODO */
}

void _HeapTCacheDAllocSmall(PHEAPDATA phd, PTCACHE ptcache, PVOID pv, SIZE_T ndxBin)
{
  /* TODO */
}

void _HeapTCacheDAllocLarge(PHEAPDATA phd, PTCACHE ptcache, PVOID pv, SIZE_T sz)
{
  /* TODO */
}

SIZE_T _HeapTCacheSAlloc(PHEAPDATA phd, PCVOID pv)
{
  return 0; /* TODO */
}

void _HeapTCacheEventHard(PHEAPDATA phd, PTCACHE ptcache)
{
  /* TODO */
}

PVOID _HeapTCacheAllocSmallHard(PHEAPDATA phd, PTCACHE ptcache, PTCACHEBIN ptbin, SIZE_T ndxBin)
{
  return NULL; /* TODO */
}

void _HeapTCacheBinFlushSmall(PHEAPDATA phd, PTCACHEBIN ptbin, SIZE_T ndxBin, UINT32 nRem, PTCACHE ptcache)
{
  /* TODO */
}

void _HeapTCacheBinFlushLarge(PHEAPDATA phd, PTCACHEBIN ptbin, SIZE_T ndxBin, UINT32 nRem, PTCACHE ptcache)
{
  /* TODO */
}

void _HeapTCacheArenaAssociate(PHEAPDATA phd, PTCACHE ptcache, PARENA pArena)
{
  /* TODO */
}

void _HeapTCacheArenaDisassociate(PHEAPDATA phd, PTCACHE ptcache)
{
  /* TODO */
}

PTCACHE _HeapTCacheCreate(PHEAPDATA phd, PARENA pArena)
{
  return NULL; /* TODO */
}

void _HeapTCacheDestroy(PHEAPDATA phd, PTCACHE ptcache)
{
  /* TODO */
}

void _HeapTCacheStatsMerge(PHEAPDATA phd, PTCACHE ptcache, PARENA pArena)
{
  /* TODO */
}

static void tcacheCleanup(PVOID pvContents, PVOID pvArg)
{
  /*
  PHEAPDATA phd = (PHEAPDATA)pvArg;
  PTCACHE ptcache = (PTCACHE)pvContents;
  */
  /* TODO */
}

HRESULT _HeapTCacheSetup(PHEAPDATA phd, IThreadLocalFactory *pthrlf)
{
  register UINT32 i;  /* loop counter */
  HRESULT hr;

  /* figure out the size of the largest max class */
  if (phd->nTCacheMaxClassBits < 0 || (1U << phd->nTCacheMaxClassBits) < SMALL_MAXCLASS)
    phd->cbTCacheMaxClass = SMALL_MAXCLASS;
  else if ((1U << phd->nTCacheMaxClassBits) > phd->szArenaMaxClass)
    phd->cbTCacheMaxClass = phd->szArenaMaxClass;
  else
    phd->cbTCacheMaxClass = (1U << phd->nTCacheMaxClassBits);

  /* figure out bins count */
  phd->nHBins = NBINS + (phd->cbTCacheMaxClass >> SYS_PAGE_BITS);

  /* initialize the info block */
  phd->ptcbi = (PTCACHEBININFO)_HeapBaseAlloc(phd, phd->nHBins * sizeof(TCACHEBININFO));
  if (!(phd->ptcbi))
    return E_OUTOFMEMORY;
  phd->nStackElems = 0;
  for (i = 0; i < NBINS; i++)
  {
    if ((phd->aArenaBinInfo[i].nRegions << 1) <= TCACHE_NSLOTS_SMALL_MAX)
      phd->ptcbi[i].nCachedMax = (phd->aArenaBinInfo[i].nRegions << 1);
    else
      phd->ptcbi[i].nCachedMax = TCACHE_NSLOTS_SMALL_MAX;
    phd->nStackElems += phd->ptcbi[i].nCachedMax;
  }
  for (; i < phd->nHBins; i++)
  {
    phd->ptcbi[i].nCachedMax = TCACHE_NSLOTS_LARGE;
    phd->nStackElems += phd->ptcbi[i].nCachedMax;
  }

  /* Create the thread-local values */
  hr = IThreadLocalFactory_CreateThreadLocal(pthrlf, NULL, &(phd->pthrlTCache));
  if (FAILED(hr))
    goto error0;
  IThreadLocal_SetCleanupFunc(phd->pthrlTCache, tcacheCleanup, phd);
  hr = IThreadLocalFactory_CreateThreadLocal(pthrlf, (PVOID)TCACHE_ENABLE_DEFAULT, &(phd->pthrlTCacheEnable));
  if (FAILED(hr))
    goto error1;
  return S_OK;

error1:
  IUnknown_Release(phd->pthrlTCache);
error0:
  return hr;
}

void _HeapTCacheShutdown(PHEAPDATA phd)
{
  IUnknown_Release(phd->pthrlTCacheEnable);
  IUnknown_Release(phd->pthrlTCache);
}
