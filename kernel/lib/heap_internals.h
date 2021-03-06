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

#include <stdarg.h>
#include <comrogue/compiler_macros.h>
#include <comrogue/types.h>
#include <comrogue/objectbase.h>
#include <comrogue/connpoint.h>
#include <comrogue/allocator.h>
#include <comrogue/stream.h>
#include <comrogue/mutex.h>
#include <comrogue/heap.h>
#include <comrogue/threadlocal.h>
#include <comrogue/objhelp.h>
#include <comrogue/internals/dlist.h>
#include <comrogue/internals/rbtree.h>
#include <comrogue/internals/seg.h>

#define LG_TINY_MIN 3                   /* smallest size class to support */
#define TINY_MIN (1U << LG_TINY_MIN)

#define LG_QUANTUM  3                   /* minimum allocation quantum is 2^3 = 8 bytes */
#define QUANTUM ((SIZE_T)(1U << LG_QUANTUM))
#define QUANTUM_MASK  (QUANTUM - 1)

#define QUANTUM_CEILING(a)  (((a) + QUANTUM_MASK) & ~QUANTUM_MASK)

#define LG_SIZEOF_LONG  LOG_INTSIZE
#define LONG            ((SIZE_T)(1U << LG_SIZEOF_LONG))
#define LONG_MASK       (LONG - 1)

#define LONG_CEILING(a)     (((a) + LONG_MASK) & ~LONG_MASK)

/* Size class data is (bin index, delta in bytes, size in bytes) */
#define SIZE_CLASSES          \
    SIZE_CLASS(0,  8,   8)    \
    SIZE_CLASS(1,  8,   16)   \
    SIZE_CLASS(2,  8,   24)   \
    SIZE_CLASS(3,  8,   32)   \
    SIZE_CLASS(4,  8,   40)   \
    SIZE_CLASS(5,  8,   48)   \
    SIZE_CLASS(6,  8,   56)   \
    SIZE_CLASS(7,  8,   64)   \
    SIZE_CLASS(8,  16,  80)   \
    SIZE_CLASS(9,  16,  96)   \
    SIZE_CLASS(10, 16,  112)  \
    SIZE_CLASS(11, 16,  128)  \
    SIZE_CLASS(12, 32,  160)  \
    SIZE_CLASS(13, 32,  192)  \
    SIZE_CLASS(14, 32,  224)  \
    SIZE_CLASS(15, 32,  256)  \
    SIZE_CLASS(16, 64,  320)  \
    SIZE_CLASS(17, 64,  384)  \
    SIZE_CLASS(18, 64,  448)  \
    SIZE_CLASS(19, 64,  512)  \
    SIZE_CLASS(20, 128, 640)  \
    SIZE_CLASS(21, 128, 768)  \
    SIZE_CLASS(22, 128, 896)  \
    SIZE_CLASS(23, 128, 1024) \
    SIZE_CLASS(24, 256, 1280) \
    SIZE_CLASS(25, 256, 1536) \
    SIZE_CLASS(26, 256, 1792) \
    SIZE_CLASS(27, 256, 2048) \
    SIZE_CLASS(28, 512, 2560) \
    SIZE_CLASS(29, 512, 3072) \
    SIZE_CLASS(30, 512, 3584) \

#define NBINS          31    /* number of bins */
#define SMALL_MAXCLASS 3584  /* max size for "small" class */

/*--------------------------------------------------------------
 * Radix tree implementation for keeping track of memory chunks
 *--------------------------------------------------------------
 */

#define RTREE_NODESIZE  (1U << 14)   /* node size for tree */

typedef struct tagMEMRTREE
{
  IMutex *pmtx;             /* mutex for tree */
  PPVOID ppRoot;            /* tree root */
  UINT32 uiHeight;          /* tree height */
  UINT32 auiLevel2Bits[0];  /* bits at level 2 - dynamically sized */
} MEMRTREE, *PMEMRTREE;

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

/*---------------------
 * Bitmap declarations
 *---------------------
 */

/* Maximum number of regions per run */
#define LG_RUN_MAXREGS             11
#define RUN_MAXREGS                (1U << LG_RUN_MAXREGS)

/* Maximum bitmap count */
#define LG_BITMAP_MAXBITS          LG_RUN_MAXREGS

typedef UINT32 BITMAP;             /* bitmap type definition */
typedef BITMAP *PBITMAP;
#define LG_SIZEOF_BITMAP           LOG_UINTSIZE

/* Number of bits per group */
#define LG_BITMAP_GROUP_NBITS      (LG_SIZEOF_BITMAP + 3)
#define BITMAP_GROUP_NBITS         (1U << LG_BITMAP_GROUP_NBITS)
#define BITMAP_GROUP_NBITS_MASK    (BITMAP_GROUP_NBITS - 1)

/* Maximum number of bitmap levels */
#define BITMAP_MAX_LEVELS \
  ((LG_BITMAP_MAXBITS / LG_SIZEOF_BITMAP) + !!(LG_BITMAP_MAXBITS % LG_SIZEOF_BITMAP))

/* Bitmap level structure */
typedef struct tagBITMAPLEVEL
{
  SIZE_T ofsGroup;                 /* offset of groups for this level within array */
} BITMAPLEVEL, *PBITMAPLEVEL;

/* Bitmap information structure */
typedef struct tagBITMAPINFO
{
  SIZE_T cBits;                    /* number of bits in bitmap */
  UINT32 nLevels;                  /* number of levels required for bits */
  BITMAPLEVEL aLevels[BITMAP_MAX_LEVELS + 1];  /* the levels - only first (nLevels + 1) used */
} BITMAPINFO, *PBITMAPINFO;
typedef const BITMAPINFO *PCBITMAPINFO;

/*---------------------------------
 * Thread-level cache declarations
 *---------------------------------
 */

#define TCACHE_NSLOTS_SMALL_MAX     200     /* max cache slots per small bin */
#define TCACHE_NSLOTS_LARGE         20      /* cache slots for large classes */
#define LG_TCACHE_MAXCLASS_DEFAULT  15      /* log2 of max class */
#define TCACHE_GC_SWEEP             8192    /* number of allocations between full GC sweeps */
/* number of allocations between incremental GC sweeps */
#define TCACHE_GC_INCR  ((TCACHE_GC_SWEEP / NBINS) + ((TCACHE_GC_SWEEP / NBINS == 0) ? 0 : 1))

/* enable status */
typedef enum tagTCACHE_ENABLE
{
  TCACHE_DISABLED = FALSE,
  TCACHE_ENABLED = TRUE,
  TCACHE_ENABLE_DEFAULT = 2
} TCACHE_ENABLE;

/* statistics per cache bin */
typedef struct tagTCACHEBINSTATS
{
  UINT64 nRequests;                             /* number of requests in this particular bin */
} TCACHEBINSTATS, *PTCACHEBINSTATS;

/* read-only info stored with each element of bins array */
typedef struct tagTCACHEBININFO
{
  UINT32 nCachedMax;                            /* upper limit on bin.nCached */
} TCACHEBININFO, *PTCACHEBININFO;

/* single bin of the cache */
typedef struct tagTCACHEBIN
{
  TCACHEBINSTATS stats;                         /* statistics for this bin */
  INT32 nLowWatermark;                          /* minimum number cached since last GC */
  UINT32 cbitFill;                              /* fill level */
  UINT32 nCached;                               /* number of cached objects */
  PPVOID ppvAvail;                              /* stack of cached objects */
} TCACHEBIN, *PTCACHEBIN;

typedef struct tagARENA ARENA, *PARENA;  /* forward declaration */

/* thread cache */
typedef struct tagTCACHE
{
  DLIST_FIELD_DECLARE(struct tagTCACHE, link);  /* link aggregator */
  UINT64 cbProfAccum;                           /* accumulated bytes */
  PARENA parena;                                /* this thread's arena */
  UINT32 cEvents;                               /* event count since incremental GC */
  UINT32 ndxNextGCBin;                          /* next bin to be GC'd */
  TCACHEBIN aBins[0];                           /* cache bins (dynamically sized) */
} TCACHE, *PTCACHE;

/* state values encoded as small non-NULL pointers */
#define TCACHE_STATE_DISABLED      ((PTCACHE)(UINT_PTR)1)
#define TCACHE_STATE_REINCARNATED  ((PTCACHE)(UINT_PTR)2)
#define TCACHE_STATE_PURGATORY     ((PTCACHE)(UINT_PTR)3)
#define TCACHE_STATE_MAX           TCACHE_STATE_PURGATORY

/*------------------------
 * Arena data definitions
 *------------------------
 */

/* Chunk map, each element corresponds to one page within the chunk */
typedef struct tagARENACHUNKMAP
{
  union
  {
    RBTREENODE rbtn;                                     /* tree of runs */
    DLIST_FIELD_DECLARE(struct tagARENACHUNKMAP, link);  /* list of runs in purgatory */
  } u;
  SIZE_T bits;                                           /* run address and various flags */
} ARENACHUNKMAP, *PARENACHUNKMAP;

#define CHUNK_MAP_BININD_SHIFT    4                      /* shift count for bin index mask */
#define BININD_INVALID            ((SIZE_T)0xFFU)        /* invalid bin index */
#define CHUNK_MAP_BININD_MASK     ((SIZE_T)0xFF0U)       /* bin index mask */
#define CHUNK_MAP_BININD_INVALID  CHUNK_MAP_BININD_MASK  /* invalid bin marker */
#define CHUNK_MAP_FLAGS_MASK      ((SIZE_T)0xCU)         /* flag bits mask */
#define CHUNK_MAP_DIRTY           ((SIZE_T)0x8U)         /* dirty flag */
#define CHUNK_MAP_UNZEROED        ((SIZE_T)0x4U)         /* non-zeroed flag */
#define CHUNK_MAP_LARGE           ((SIZE_T)0x2U)         /* large allocation flag */
#define CHUNK_MAP_ALLOCATED       ((SIZE_T)0x1U)         /* allocated flag */
#define CHUNK_MAP_KEY             CHUNK_MAP_ALLOCATED

/* chunk header within an arena */
typedef struct tagARENACHUNK
{
  PARENA parena;                 /* arena that owns the chunk */
  RBTREENODE rbtnDirty;          /* linkage for tree of chunks with dirty runs */
  SIZE_T cpgDirty;               /* number of dirty pages */
  SIZE_T cAvailRuns;             /* number of available runs */
  SIZE_T cAvailRunAdjacent;      /* number of available run adjacencies */
  ARENACHUNKMAP aMaps[0];        /* map of pages within chunk */
} ARENACHUNK, *PARENACHUNK;

/* per-bin allocation statistics */
typedef struct tagMALLOCBINSTATS
{
  SIZE_T cbAllocated;            /* current number of bytes allocated */
  UINT64 cMalloc;                /* number of allocation requests */
  UINT64 cDalloc;                /* number of deallocation requests */
  UINT64 cRequests;              /* number of total requests, including cache */
  UINT64 cFills;                 /* number of cache fills to this bin */
  UINT64 cFlushes;               /* number of cache flushes to this bin */
  UINT64 cRuns;                  /* total number of created runs */
  UINT64 cReRuns;                /* total number of reused runs */
  SIZE_T cRunsCurrent;           /* current number of runs in the bin */
} MALLOCBINSTATS, *PMALLOCBINSTATS;

/* large allocation statistics */
typedef struct tagMALLOCLARGESTATS
{
  UINT64 nMalloc;                /* number of allocation requests */
  UINT64 nDalloc;                /* number of deallocation requests */
  UINT64 nRequests;              /* number of allocation requests */
  SIZE_T cRuns;                  /* count of runs of this size class */
} MALLOCLARGESTATS, *PMALLOCLARGESTATS;

/* Arena statistics data. */
typedef struct tagARENASTATS
{
  SIZE_T cbMapped;               /* number of bytes currently mapped */
  UINT64 cPurges;                /* number of purge sweeps made */
  UINT64 cAdvise;                /* number of memory advise calls made */
  UINT64 cPagesPurged;           /* number of pages purged */
  SIZE_T cbAllocatedLarge;       /* number of bytes of large allocations */
  UINT64 cLargeMalloc;           /* number of large allocations */
  UINT64 cLargeDalloc;           /* number of large deallocations */
  UINT64 cLargeRequests;         /* number of large allocation requests */
  PMALLOCLARGESTATS amls;        /* array of stat elements, one per size class */
} ARENASTATS, *PARENASTATS;

#define REDZONE_MINSIZE   16     /* red zones must be at least this many bytes */

/* Arena bin information */
typedef struct tagARENABININFO
{
  SIZE_T cbRegions;              /* size of regions in a run */
  SIZE_T cbRedzone;              /* size of the red zone */
  SIZE_T cbInterval;             /* interval between regions */
  SIZE_T cbRunSize;              /* total size of a run for this size class */
  UINT32 nRegions;               /* number of regions in a run for this size class */
  UINT32 ofsBitmap;              /* offset of bitmap element in run header */
  BITMAPINFO bitmapinfo;         /* manipulates bitmaps associated with this bin's runs */
  UINT32 ofsCtx0;                /* offset of context in run header, or 0 */
  UINT32 ofsRegion0;             /* offset of first region in a run for size class */
} ARENABININFO, *PARENABININFO;

typedef struct tagARENABIN ARENABIN, *PARENABIN;  /* forward declaration */

typedef struct tagARENARUN
{
  PARENABIN pBin;                /* bin this run is associated with */
  UINT32 ndxNext;                /* index of next region never allocated */
  UINT32 nFree;                  /* number of free regions in run */
} ARENARUN, *PARENARUN;

struct tagARENABIN
{
  IMutex *pmtxLock;              /* arena bin lock */
  PARENARUN prunCurrent;         /* pointer to current run servicing this bin's size class */
  RBTREE rbtRuns;                /* tree of non-full runs */
  MALLOCBINSTATS stats;          /* bin statistics */
};

/* The actual arena definition. */
struct tagARENA
{
  UINT32 nIndex;                            /* index of this arena within array */
  UINT32 nThreads;                          /* number of threads assigned to this arena */
  IMutex *pmtxLock;                         /* arena lock */
  ARENASTATS stats;                         /* arena statistics */
  DLIST_HEAD_DECLARE(TCACHE, dlistTCache);  /* list of tcaches for threads in arena */
  UINT64 cbProfAccum;                       /* accumulated bytes */
  RBTREE rbtDirtyChunks;                    /* tree of dirty page-containing chunks */
  PARENACHUNK pchunkSpare;                  /* most recently freed chunk, one spare per arena */
  SIZE_T cpgActive;                         /* number of pages in active runs */
  SIZE_T cpgDirty;                          /* number of potential dirty pages */
  SIZE_T cpgPurgatory;                      /* number of pages being purged */
  RBTREE rbtAvailRuns;                      /* tree of the available runs */
  ARENABIN aBins[NBINS];                    /* bins for storing free regions */
};

/*----------------------------------
 * The actual heap data declaration
 *----------------------------------
 */

typedef struct tagHEAPDATA {
  IMalloc mallocInterface;                         /* pointer to IMalloc interface - MUST BE FIRST! */
  IConnectionPointContainer cpContainerInterface;  /* pointer to IConnectionPointContainer interface */
  IHeapConfiguration heapConfInterface;            /* pointer to IHeapConfiguration interface */
  UINT32 uiRefCount;                               /* reference count */
  UINT32 uiFlags;                                  /* flags word */
  PFNRAWHEAPDATAFREE pfnFreeRawHeapData;           /* pointer to function that frees the raw heap data, if any */
  PFNHEAPABORT pfnAbort;                           /* pointer to abort function */
  PVOID pvAbortArg;                                /* argument to abort function */
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
  SSIZE_T cbActiveDirtyRatio;                      /* active/dirty ratio parameter */
  IMutex *pmtxChunks;                              /* chunks mutex */
  RBTREE rbtExtSizeAddr;                           /* tree ordering extents by size and address */
  RBTREE rbtExtAddr;                               /* tree ordering extents by address */
  PMEMRTREE prtChunks;                             /* radix tree containing all chunk values */
  IMutex *pmtxBase;                                /* base mutex */
  PVOID pvBasePages;                               /* pages being used for internal memory allocation */
  PVOID pvBaseNext;                                /* next allocation location */
  PVOID pvBasePast;                                /* address immediately past pvBasePages */
  PEXTENT_NODE pexnBaseNodes;                      /* pointer to base nodes */
  SIZE_T cpgMapBias;                               /* number of header pages for arena chunks */
  SIZE_T szArenaMaxClass;                          /* maximum size class for arenas */
  SSIZE_T nTCacheMaxClassBits;                     /* bits in thread cache max class size */
  PTCACHEBININFO ptcbi;                            /* pointer to thread cache bin info */
  SIZE_T nHBins;                                   /* number of thread cache bins */
  SIZE_T cbTCacheMaxClass;                         /* max cached size class */
  UINT32 nStackElems;                              /* number of stack elements per tcache */
  IThreadLocal *pthrlTCache;                       /* thread-local tcache value */
  IThreadLocal *pthrlTCacheEnable;                 /* thread-local tcache enable value */
  ARENABININFO aArenaBinInfo[NBINS];               /* array of arena bin information */
} HEAPDATA, *PHEAPDATA;

/*---------------------------------
 * Utility and debugging functions
 *---------------------------------
 */

/* Get nearest aligned address at or below a. */
#define ALIGNMENT_ADDR2BASE(a, alignment)  ((PVOID)(((UINT_PTR)(a)) & ~(alignment)))

/* Get offset between a and ALIGNMENT_ADDR2BASE(a, alignment). */
#define ALIGNMENT_ADDR2OFFSET(a, alignment) ((SIZE_T)(((UINT_PTR)(a)) & (alignment)))

/* Returns the smallest alignment multiple greater than sz. */
#define ALIGNMENT_CEILING(sz, alignment)    (((sz) + (alignment)) & ~(alignment))

CDECL_BEGIN

extern void _HeapDbgWrite(PHEAPDATA phd, PCSTR sz);
extern void _HeapPrintf(PHEAPDATA phd, PCSTR szFormat, ...);
extern void _HeapAssertFailed(PHEAPDATA phd, PCSTR szFile, INT32 nLine);
extern SIZE_T _HeapPow2Ceiling(SIZE_T x);

CDECL_END

#define _H_THIS_FILE  __FILE__
#define _DECLARE_H_THIS_FILE static const char SEG_RODATA _H_THIS_FILE[] = __FILE__;

#define _H_ASSERT(phd, expr)  ((expr) ? (void)0 : _HeapAssertFailed(phd, _H_THIS_FILE, __LINE__))

/*---------------------------------
 * Radix tree management functions
 *---------------------------------
 */

CDECL_BEGIN

extern PMEMRTREE _HeapRTreeNew(PHEAPDATA phd, UINT32 cBits);
extern void _HeapRTreeDestroy(PMEMRTREE prt);
extern PVOID _HeapRTreeGetLocked(PHEAPDATA phd, PMEMRTREE prt, UINT_PTR uiKey);
extern PVOID _HeapRTreeGet(PHEAPDATA phd, PMEMRTREE prt, UINT_PTR uiKey);
extern BOOL _HeapRTreeSet(PHEAPDATA phd, PMEMRTREE prt, UINT_PTR uiKey, PVOID pv);

CDECL_END

/*-------------------------------------
 * Internal chunk management functions
 *-------------------------------------
 */

/* Get chunk address for allocated address a. */
#define CHUNK_ADDR2BASE(phd, a)             ALIGNMENT_ADDR2BASE((a), (phd)->uiChunkSizeMask)

/* Get chunk offset of allocated address a. */
#define CHUNK_ADDR2OFFSET(phd, a)           ALIGNMENT_ADDR2OFFSET((a), (phd)->uiChunkSizeMask)

/* Return the smallest chunk size multiple that can contain a certain size. */
#define CHUNK_CEILING(phd, sz)              ALIGNMENT_CEILING((sz), (phd)->uiChunkSizeMask)

CDECL_BEGIN

extern PVOID _HeapChunkAlloc(PHEAPDATA phd, SIZE_T sz, SIZE_T szAlignment, BOOL fBase, BOOL *pfZeroed);
extern void _HeapChunkUnmap(PHEAPDATA phd, PVOID pvChunk, SIZE_T sz);
extern void _HeapChunkDeAlloc(PHEAPDATA phd, PVOID pvChunk, SIZE_T sz, BOOL fUnmap);
extern HRESULT _HeapChunkSetup(PHEAPDATA phd);
extern void _HeapChunkShutdown(PHEAPDATA phd);

CDECL_END

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

/*------------------
 * Bitmap functions
 *------------------
 */

CDECL_BEGIN

extern void _HeapBitmapInfoInit(PBITMAPINFO pBInfo, SIZE_T cBits);
extern SIZE_T _HeapBitmapInfoNumGroups(PCBITMAPINFO pcBInfo);
extern SIZE_T _HeapBitmapSize(SIZE_T cBits);
extern void _HeapBitmapInit(PBITMAP pBitmap, PCBITMAPINFO pcBInfo);
extern BOOL _HeapBitmapFull(PBITMAP pBitmap, PCBITMAPINFO pcBInfo);
extern BOOL _HeapBitmapGet(PBITMAP pBitmap, PCBITMAPINFO pcBInfo, SIZE_T nBit);
extern void _HeapBitmapSet(PBITMAP pBitmap, PCBITMAPINFO pcBInfo, SIZE_T nBit);
extern SIZE_T _HeapBitmapSetFirstUnset(PBITMAP pBitmap, PCBITMAPINFO pcBInfo);
extern void _HeapBitmapUnset(PBITMAP pBitmap, PCBITMAPINFO pcBInfo, SIZE_T nBit);

CDECL_END

/*----------------------------
 * Arena management functions
 *----------------------------
 */

CDECL_BEGIN

extern const BYTE abSmallSize2Bin[];

extern void _HeapArenaPurgeAll(PHEAPDATA phd, PARENA pArena);
extern void _HeapArenaTCacheFillSmall(PHEAPDATA phd, PARENA pArena, PTCACHEBIN ptbin, SIZE_T ndxBin,
                                      UINT64 cbProfAccum);
extern void _HeapArenaAllocJunkSmall(PHEAPDATA phd, PVOID pv, PARENABININFO pBinInfo, BOOL fZero);
extern void _HeapArenaDAllocJunkSmall(PHEAPDATA phd, PVOID pv, PARENABININFO pBinInfo);
extern PVOID _HeapArenaMallocSmall(PHEAPDATA phd, PARENA pArena, SIZE_T sz, BOOL fZero);
extern PVOID _HeapArenaMallocLarge(PHEAPDATA phd, PARENA pArena, SIZE_T sz, BOOL fZero);
extern PVOID _HeapArenaPalloc(PHEAPDATA phd, PARENA pArena, SIZE_T sz, SIZE_T szAlignment, BOOL fZero);
extern void _HeapArenaProfPromoted(PHEAPDATA phd, PCVOID pv, SIZE_T sz);
extern void _HeapArenaDAllocBinLocked(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv,
				      PARENACHUNKMAP pMapElement);
extern void _HeapArenaDAllocBin(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv,
				PARENACHUNKMAP pMapElement);
extern void _HeapArenaDAllocSmall(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv, SIZE_T ndxPage);
extern void _HeapArenaDAllocLargeLocked(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv);
extern void _HeapArenaDAllocLarge(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PVOID pv);
extern PVOID _HeapArenaRAllocNoMove(PHEAPDATA phd, PVOID pv, SIZE_T szOld, SIZE_T sz, SIZE_T szExtra, BOOL fZero);
extern PVOID _HeapArenaRAlloc(PHEAPDATA phd, PVOID pv, SIZE_T szOld, SIZE_T sz, SIZE_T szExtra, SIZE_T szAlignment,
			      BOOL fZero, BOOL fTryTCacheAlloc, BOOL fTryTCacheDAlloc);
extern void _HeapArenaStatsMerge(PHEAPDATA phd, PARENA pArena, PPCCHAR dss, PSIZE_T pnActive, PSIZE_T pnDirty,
				 PARENASTATS pArenaStats, PMALLOCBINSTATS pBinStats, PMALLOCLARGESTATS pLargeStats);
extern BOOL _HeapArenaNew(PHEAPDATA phd, PARENA pArena, UINT32 ndx);
extern HRESULT _HeapArenaSetup(PHEAPDATA phd);
extern void _HeapArenaShutdown(PHEAPDATA phd);
extern PARENACHUNKMAP _HeapArenaMapPGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern PSIZE_T _HeapArenaMapBitsPGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern SIZE_T _HeapArenaMapBitsGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern SIZE_T _HeapArenaMapBitsUnallocatedSizeGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern SIZE_T _HeapArenaMapBitsLargeSizeGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern SIZE_T _HeapArenaMapBitsSmallRunIndexGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern SIZE_T _HeapArenaMapBitsBinIndexGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern SIZE_T _HeapArenaMapBitsDirtyGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern SIZE_T _HeapArenaMapBitsUnzeroedGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern SIZE_T _HeapArenaMapBitsLargeGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern SIZE_T _HeapArenaMapBitsAllocatedGet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage);
extern void _HeapArenaMapBitsUnallocatedSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T sz,
					    SIZE_T szFlags);
extern void _HeapArenaMapBitsUnallocatedSizeSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T sz);
extern void _HeapArenaMapBitsLargeSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T sz,
				      SIZE_T szFlags);
extern void _HeapArenaMapBitsLargeBinIndSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T ndxBin);
extern void _HeapArenaMapBitsSmallSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T ndxRun,
				      SIZE_T ndxBin, SIZE_T szFlags);
extern void _HeapArenaMapBitsUnzeroedSet(PHEAPDATA phd, PARENACHUNK pChunk, SIZE_T ndxPage, SIZE_T szUnzeroed);
extern BOOL _HeapArenaProfAccumImpl(PHEAPDATA phd, PARENA pArena, UINT64 cbAccum);
extern BOOL _HeapArenaProfAccumLocked(PHEAPDATA phd, PARENA pArena, UINT64 cbAccum);
extern BOOL _HeapArenaProfAccum(PHEAPDATA phd, PARENA pArena, UINT64 cbAccum);
extern SIZE_T _HeapArenaPtrSmallBinIndGet(PHEAPDATA phd, PCVOID pv, SIZE_T szMapBits);
extern SIZE_T _HeapArenaBinIndex(PHEAPDATA phd, PARENA pArena, PARENABIN pBin);
extern UINT32 _HeapArenaRunRegInd(PHEAPDATA phd, PARENARUN pRun, PARENABININFO pBinInfo, PCVOID pv);
extern PVOID _HeapArenaMalloc(PHEAPDATA phd, PARENA pArena, SIZE_T sz, BOOL fZero, BOOL fTryTCache);
extern SIZE_T _HeapArenaSAlloc(PHEAPDATA phd, PCVOID pv, BOOL fDemote);
extern void _HeapArenaDAlloc(PHEAPDATA phd, PARENA pArena, PARENACHUNK pChunk, PCVOID pv, BOOL fTryTCache);

CDECL_END

#define SMALL_SIZE2BIN(s)  (abSmallSize2Bin[(s - 1) >> LG_TINY_MIN])

/*------------------------------
 * Thread-level cache functions
 *------------------------------
 */

CDECL_BEGIN

extern void _HeapTCacheEvent(PHEAPDATA phd, PTCACHE ptcache);
extern void _HeapTCacheFlush(PHEAPDATA phd);
extern BOOL _HeapTCacheIsEnabled(PHEAPDATA phd);
extern PTCACHE _HeapTCacheGet(PHEAPDATA phd, BOOL fCreate);
extern void _HeapTCacheSetEnabled(PHEAPDATA phd, BOOL fEnabled);
extern PVOID _HeapTCacheAllocEasy(PHEAPDATA phd, PTCACHEBIN ptbin);
extern PVOID _HeapTCacheAllocSmall(PHEAPDATA phd, PTCACHE ptcache, SIZE_T sz, BOOL fZero);
extern PVOID _HeapTCacheAllocLarge(PHEAPDATA phd, PTCACHE ptcache, SIZE_T sz, BOOL fZero);
extern void _HeapTCacheDAllocSmall(PHEAPDATA phd, PTCACHE ptcache, PVOID pv, SIZE_T ndxBin);
extern void _HeapTCacheDAllocLarge(PHEAPDATA phd, PTCACHE ptcache, PVOID pv, SIZE_T sz);
extern SIZE_T _HeapTCacheSAlloc(PHEAPDATA phd, PCVOID pv);
extern void _HeapTCacheEventHard(PHEAPDATA phd, PTCACHE ptcache);
extern PVOID _HeapTCacheAllocSmallHard(PHEAPDATA phd, PTCACHE ptcache, PTCACHEBIN ptbin, SIZE_T ndxBin);
extern void _HeapTCacheBinFlushSmall(PHEAPDATA phd, PTCACHEBIN ptbin, SIZE_T ndxBin, UINT32 nRem, PTCACHE ptcache);
extern void _HeapTCacheBinFlushLarge(PHEAPDATA phd, PTCACHEBIN ptbin, SIZE_T ndxBin, UINT32 nRem, PTCACHE ptcache);
extern void _HeapTCacheArenaAssociate(PHEAPDATA phd, PTCACHE ptcache, PARENA pArena);
extern void _HeapTCacheArenaDisassociate(PHEAPDATA phd, PTCACHE ptcache);
extern PTCACHE _HeapTCacheCreate(PHEAPDATA phd, PARENA pArena);
extern void _HeapTCacheDestroy(PHEAPDATA phd, PTCACHE ptcache);
extern void _HeapTCacheStatsMerge(PHEAPDATA phd, PTCACHE ptcache, PARENA pArena);
extern HRESULT _HeapTCacheSetup(PHEAPDATA phd, IThreadLocalFactory *pthrlf);
extern void _HeapTCacheShutdown(PHEAPDATA phd);

CDECL_END

/*------------------------------
 * Top-level internal functions
 *------------------------------
 */

CDECL_BEGIN

extern PARENA _HeapChooseArenaHard(PHEAPDATA phd);
extern PARENA _HeapChooseArena(PHEAPDATA phd, PARENA parena);

CDECL_END

#endif /* __ASM__ */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __HEAP_INTERNALS_H_INCLUDED */
