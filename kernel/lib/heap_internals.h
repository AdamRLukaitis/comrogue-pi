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
#ifndef __HEAP_INTERNALS_H_INCLUDED
#define __HEAP_INTERNALS_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

#ifndef __ASM__

#include <comrogue/compiler_macros.h>
#include <comrogue/types.h>
#include <comrogue/objectbase.h>
#include <comrogue/connpoint.h>
#include <comrogue/allocator.h>
#include <comrogue/mutex.h>
#include <comrogue/heap.h>
#include <comrogue/objhelp.h>
#include <comrogue/internals/rbtree.h>

/*-------------------
 * Extent management
 *-------------------
 */

/* Tree of extents managed by the heap management code. */
typedef struct tagEXTENT_NODE
{
  RBTREENODE rbtnSizeAddress;    /* tree node for size and address ordering */
  RBTREENODE rbtnAddress;        /* tree node for address ordering */
  /* TODO prof_ctx? */
  PVOID pv;                      /* base pointer to region */
  SIZE_T sz;                     /* size of region */
  BOOL fZeroed;                  /* is this extent zeroed? */
} EXTENT_NODE, *PEXTENT_NODE;
typedef PEXTENT_NODE *PPEXTENT_NODE;

/*----------------------------------
 * The actual heap data declaration
 *----------------------------------
 */

typedef struct tagHEAPDATA {
  IMalloc mallocInterface;                         /* pointer to IMalloc interface - MUST BE FIRST! */
  IConnectionPointContainer cpContainerInterface;  /* pointer to IConnectionPointContainer interface */
  UINT32 uiRefCount;                               /* reference count */
  UINT32 uiFlags;                                  /* flags word */
  PFNRAWHEAPDATAFREE pfnFreeRawHeapData;           /* pointer to function that frees the raw heap data, if any */
  IChunkAllocator *pChunkAllocator;                /* chunk allocator pointer */
  IMutexFactory *pMutexFactory;                    /* mutex factory pointer */
  FIXEDCPDATA fcpMallocSpy;                        /* connection point for IMallocSpy */
  FIXEDCPDATA fcpSequentialStream;                 /* connection point for ISequentialStream for debugging */
  IMallocSpy *pMallocSpy;                          /* IMallocSpy interface for the allocator */
  ISequentialStream *pDebugStream;                 /* debugging output stream */
  UINT32 nChunkBits;                               /* number of bits in a chunk */
  UINT32 szChunk;                                  /* size of a chunk */
  UINT32 uiChunkSizeMask;                          /* bitmask for a chunk */
  UINT32 cpgChunk;                                 /* number of pages in a chunk */
  RBTREE rbtExtSizeAddr;                           /* tree ordering extents by size and address */
  RBTREE rbtExtAddr;                               /* tree ordering extents by address */
  IMutex *pmtxBase;                                /* base mutex */
  PVOID pvBasePages;                               /* pages being used for internal memory allocation */
  PVOID pvBaseNext;                                /* next allocation location */
  PVOID pvBasePast;                                /* address immediately past pvBasePages */
  PEXTENT_NODE pexnBaseNodes;                      /* pointer to base nodes */
} HEAPDATA, *PHEAPDATA;

/*-------------------------------------
 * Internal chunk management functions
 *-------------------------------------
 */

/* Get chunk address for allocated address a. */
#define CHUNK_ADDR2BASE(phd, a)   ((PVOID)(((UINT_PTR)(a)) & ~((phd)->uiChunkSizeMask)))

/* Get chunk offset of allocated address a. */
#define CHUNK_ADDR2OFFSET(phd, a) ((SIZE_T)(((UINT_PTR)(a)) & (phd)->uiChunkSizeMask))

/* Return the smallest chunk size multiple that can contain a certain size. */
#define CHUNK_CEILING(phd, sz)    (((sz) + (phd)->uiChunkSizeMask) & ~((phd)->uiChunkSizeMask))

extern PVOID _HeapChunkAlloc(PHEAPDATA phd, SIZE_T sz, SIZE_T szAlignment, BOOL fBase, BOOL *pfZeroed);
extern void _HeapChunkUnmap(PHEAPDATA phd, PVOID pvChunk, SIZE_T sz);
extern void _HeapChunkDeAlloc(PHEAPDATA phd, PVOID pvChunk, SIZE_T sz, BOOL fUnmap);
extern HRESULT _HeapChunkSetup(PHEAPDATA phd);
extern void _HeapChunkShutdown(PHEAPDATA phd);

/*------------------------------------
 * Internal base management functions
 *------------------------------------
 */

CDECL_BEGIN

extern PVOID _HeapBaseAlloc(PHEAPDATA phd, SIZE_T sz);
extern PEXTENT_NODE _HeapBaseNodeAlloc(PHEAPDATA phd);
extern void _HeapBaseNodeDeAlloc(PHEAPDATA phd, PEXTENT_NODE pexn);
extern HRESULT _HeapBaseSetup(PHEAPDATA phd);
extern void _HeapBaseShutdown(PHEAPDATA phd);

CDECL_END

#endif /* __ASM__ */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __HEAP_INTERNALS_H_INCLUDED */
