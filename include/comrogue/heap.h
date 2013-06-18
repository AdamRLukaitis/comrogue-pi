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
#ifndef __HEAP_H_INCLUDED
#define __HEAP_H_INCLUDED

#ifndef __ASM__

#include <comrogue/compiler_macros.h>
#include <comrogue/types.h>
#include <comrogue/scode.h>
#include <comrogue/allocator.h>
#include <comrogue/mutex.h>

/*------------------------------------------------------------------------------------
 * The raw heap data used to hold the heap internals.  It's defined as opaque memory.
 *------------------------------------------------------------------------------------
 */

typedef struct tagRAWHEAPDATA
{
  UINT32 opaque[128];             /* opaque data, do not modify */
} RAWHEAPDATA, *PRAWHEAPDATA;

typedef void (*PFNRAWHEAPDATAFREE)(PRAWHEAPDATA); /* function that optionally frees the heap data */

/*--------------------
 * External functions
 *--------------------
 */

#define STD_CHUNK_BITS  22     /* standard number of bits in a memory chunk - yields 4Mb chunks */

/* Flag definitions */
#define PHDFLAGS_REDZONE   0x00000001U             /* use red zones? */
#define PHDFLAGS_JUNKFILL  0x00000002U             /* fill junk in heap? */
#define PHDFLAGS_ZEROFILL  0x00000004U             /* zero-fill allocations? */
#define PHDFLAGS_NOTCACHE  0x00000008U             /* thread cache disabled? */
#define PHDFLAGS_PROFILE   0x00000010U             /* profiling enabled? */

CDECL_BEGIN

extern HRESULT HeapCreate(PRAWHEAPDATA prhd, PFNRAWHEAPDATAFREE pfnFree, UINT32 uiFlags, UINT32 nChunkBits, 
			  IChunkAllocator *pChunkAllocator, IMutexFactory *pMutexFactory,
			  IMalloc **ppHeap);

CDECL_END

#endif /* __ASM__ */

#endif /* __HEAP_H_INCLUDED */
