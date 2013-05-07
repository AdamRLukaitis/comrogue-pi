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
#ifndef __OBJECT_DEFINITION_MACROS_H_INCLUDED
#define __OBJECT_DEFINITION_MACROS_H_INCLUDED

#ifndef __ASM__

#include <comrogue/compiler_macros.h>
#include <comrogue/types.h>

/*------------------------------------------------------------------------------------
 * GUID definition macros which define either a GUID or an external reference to one,
 * controlled by symbol INITGUID
 *------------------------------------------------------------------------------------
 */
#ifndef GUIDATTR
#ifdef __COMROGUE_INTERNALS__
#include <comrogue/internals/seg.h>
#define GUIDATTR   SEG_RODATA
#else
#define GUIDATTR
#endif  /* __COMROGUE_INTERNALS__ */
#endif /* GUIDATTR */

#ifdef INITGUID

#define DEFINE_UUID_TYPE(typ, name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
  EXTERN_C const typ GUIDATTR name = { l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } }

#else

#define DEFINE_UUID_TYPE(typ, name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
  EXTERN_C extern const typ name

#endif /* INITGUID */

#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
  DEFINE_UUID_TYPE(GUID, name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
#define DEFINE_IID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
  DEFINE_UUID_TYPE(IID, name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)
#define DEFINE_CLSID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
  DEFINE_UUID_TYPE(CLSID, name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)

/*----------------------------------------------------------------------------------------
 * Macros used in the auto-generated object declarations, controlled by symbol CINTERFACE
 *----------------------------------------------------------------------------------------
 */

#if !defined(__cplusplus) && !defined(CINTERFACE)
#define CINTERFACE
#endif

#define interface struct

#ifdef CINTERFACE

#define STDMETHOD_(name, typ)   typ (*name)
#define STDMETHOD(name)         HRESULT (*name)
#define THIS_(typ)              typ *pThis,
#define THIS(typ)               typ *pThis
#define PURE
#define END_METHODS
#define INHERIT_METHODS(sym)    sym

#define DECLARE_INTERFACE(typ) \
  struct typ ## VTable; \
  typedef interface tagIf ## typ { const struct typ ## VTable *pVTable; } typ;
#define BEGIN_INTERFACE(typ) \
  struct typ ## VTable {
#define BEGIN_INTERFACE_(typ, parent)   BEGIN_INTERFACE(typ)
#define END_INTERFACE(typ)     };

#else

#define STDMETHOD_(name, typ)   typ name
#define STDMETHOD(name)         HRESULT name
#define THIS_(typ)
#define THIS(typ)
#define PURE                    = 0
#define END_METHODS
#define INHERIT_METHODS(sym)

#define DECLARE_INTERFACE(typ) \
  interface typ;
#define BEGIN_INTERFACE(typ) \
  interface typ {
#define BEGIN_INTERFACE_(typ, parent) \
  interface typ : parent {
#define END_INTERFACE(typ)   };

#endif  /* CINTERFACE */

#endif /* __ASM__ */

#endif /* __OBJECT_DEFINITION_MACROS_H_INCLUDED */
