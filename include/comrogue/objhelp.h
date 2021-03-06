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
#ifndef __OBJHELP_H_INCLUDED
#define __OBJHELP_H_INCLUDED

#ifndef __ASM__

#include <comrogue/compiler_macros.h>
#include <comrogue/objectbase.h>
#include <comrogue/connpoint.h>
#include <comrogue/allocator.h>
#include <comrogue/stream.h>

/*------------------------------------------------
 * Generic fixed connection point data structure.
 *------------------------------------------------
 */

typedef struct tagFIXEDCPDATA
{
  IConnectionPoint connectionPointInterface;  /* the connection point interface */
  PUNKNOWN punkOuter;                         /* outer unknown used for reference counts */
  REFIID riidConnection;                      /* IID of outgoing interface for this connection point */
  IUnknown **ppSlots;                         /* pointer to actual slots used for connection point storage */
  UINT32 ncpSize;                             /* number of connection points actually connected */
  UINT32 ncpCapacity;                         /* maximum number of connection points connectable */
  IMalloc *pAllocator;                        /* pointer to allocator */
} FIXEDCPDATA, *PFIXEDCPDATA;

/*---------------------
 * Function prototypes
 *---------------------
 */

CDECL_BEGIN

/* QueryInterface helpers */
extern HRESULT ObjHlpStandardQueryInterface_IConnectionPoint(IUnknown *pThis, REFIID riid, PPVOID ppvObject);
extern HRESULT ObjHlpStandardQueryInterface_IEnumConnections(IUnknown *pThis, REFIID riid, PPVOID ppvObject);
extern HRESULT ObjHlpStandardQueryInterface_IMalloc(IUnknown *pThis, REFIID riid, PPVOID ppvObject);
extern HRESULT ObjHlpStandardQueryInterface_ISequentialStream(IUnknown *pThis, REFIID riid, PPVOID ppvObject);

/* AddRef/Release helpers */
extern UINT32 ObjHlpStaticAddRefRelease(IUnknown *pThis);

/* Connection point helpers */
extern void ObjHlpFixedCpSetup(PFIXEDCPDATA pData, PUNKNOWN punkOuter, REFIID riidConnection,
			       IUnknown **ppSlots, UINT32 nSlots, IMalloc *pAllocator);
extern void ObjHlpFixedCpTeardown(PFIXEDCPDATA pData);

/* Other helpers */
extern void ObjHlpDoNothingReturnVoid(IUnknown *pThis);
extern HRESULT ObjHlpNotImplemented(IUnknown *pThis);

CDECL_END

#endif /* __ASM__ */

#endif /* __OBJHELP_H_INCLUDED */
