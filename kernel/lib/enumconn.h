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
#ifndef __ENUMCONN_H_INCLUDED
#define __ENUMCONN_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

#ifndef __ASM__

#include <comrogue/types.h>
#include <comrogue/objectbase.h>
#include <comrogue/connpoint.h>
#include <comrogue/allocator.h>

/*---------------------------------------------------
 * Data structures for implementing IEnumConnections
 *---------------------------------------------------
 */

typedef struct tagENUMCONNDATA {
  UINT32 uiRefCount;                  /* reference count */
  UINT32 nConnectData;                /* count of CONNECTDATA */
  CONNECTDATA rgConnectData[0];       /* array of CONNECTDATA */
} ENUMCONNDATA, *PENUMCONNDATA;

typedef struct tagENUMCONN {
  IEnumConnections enumConnections;    /* interface pointer */
  UINT32 uiRefCount;                   /* reference count */
  UINT32 nCurrent;                     /* current pointer into array */
  PENUMCONNDATA pPayload;              /* payload array */
  IMalloc *pAllocator;                 /* allocator pointer */
} ENUMCONN, *PENUMCONN;

/*---------------------------------------------------
 * Functions for implementing IEnumConnections
 *---------------------------------------------------
 */

extern PENUMCONNDATA _ObjHlpAllocateEnumConnData(IMalloc *pAllocator, UINT32 nCapacity);
extern void _ObjHlpAddToEnumConnData(PENUMCONNDATA pecd, IUnknown *pUnk, UINT32 uiCookie);
extern void _ObjHlpDiscardEnumConnData(PENUMCONNDATA pecd);
extern PENUMCONN _ObjHlpAllocateEnumConn(IMalloc *pAllocator, PENUMCONNDATA pecd, UINT32 nCurrent);

#endif /* __ASM__ */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __ENUMCONN_H_INCLUDED */
