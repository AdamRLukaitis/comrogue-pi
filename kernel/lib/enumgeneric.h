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
#ifndef __ENUMGENERIC_H_INCLUDED
#define __ENUMGENERIC_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

#ifndef __ASM__

#include <comrogue/types.h>
#include <comrogue/objectbase.h>
#include <comrogue/allocator.h>

/*---------------------------------------------------------------------------------------------------
 * Data structures for implementing IEnumUnknown or any interface that enumerates interface pointers
 *---------------------------------------------------------------------------------------------------
 */

typedef struct tagENUMGENERICDATA {
  UINT32 uiRefCount;            /* reference count */
  UINT32 nObjects;              /* count of IUnknown pointers */
  PUNKNOWN rgObjects[0];        /* array of IUnknown pointers */
} ENUMGENERICDATA, *PENUMGENERICDATA;

typedef struct tagENUMGENERIC {
  IEnumUnknown enumUnknown;     /* interface pointer */
  UINT32 uiRefCount;            /* reference count */
  REFIID riidActual;            /* IID of actual interface */
  UINT32 nCurrent;              /* current pointer into array */
  PENUMGENERICDATA pPayload;    /* payload array */
  IMalloc *pAllocator;          /* allocator pointer */
} ENUMGENERIC, *PENUMGENERIC;

/*---------------------------------------------------------------------------------------------
 * Functions for implementing IEnumUnknown or any interface that enumerates interface pointers
 *---------------------------------------------------------------------------------------------
 */

extern PENUMGENERICDATA _ObjHlpAllocateEnumGenericData(IMalloc *pAllocator, UINT32 nCapacity);
extern void _ObjHlpAddToEnumGenericData(PENUMGENERICDATA pegd, IUnknown *pUnk);
extern void _ObjHlpDiscardEnumGenericData(PENUMGENERICDATA pecd);
extern PENUMGENERIC _ObjHlpAllocateEnumGeneric(IMalloc *pAllocator, REFIID riidActual, PENUMGENERICDATA pegd,
					       UINT32 nCurrent);

#endif /* __ASM__ */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __ENUMGENERIC_H_INCLUDED */
