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
#include <comrogue/types.h>
#include <comrogue/scode.h>
#include <comrogue/str.h>
#include <comrogue/object_types.h>
#include <comrogue/objectbase.h>
#include <comrogue/allocator.h>
#include <comrogue/stdobj.h>
#include <comrogue/stream.h>
#include <comrogue/objhelp.h>

/*--------------------------------------------
 * Standard object functions (API-type calls)
 *--------------------------------------------
 */

/*
 * Determines whether two GUID references are equal.
 *
 * Parameters:
 * - guid1 = First GUID to compare.
 * - guid2 = Second GUID to compare.
 *
 * Returns:
 * TRUE if the GUIDs are equal, FALSE otherwise.
 */
BOOL IsEqualGUID(REFGUID guid1, REFGUID guid2)
{
  return MAKEBOOL(StrCompareMem(guid1, guid2, sizeof(GUID)) == 0);
}

/*-----------------------------------------------------------------
 * Functions to be used in the construction of C interface vtables
 *-----------------------------------------------------------------
 */

/*
 * Retrieves pointers to the supported interfaces on an object.  Any pointer returned by this method
 * has AddRef called on it before it returns.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 * - riid = The identifier of the interface being requested.
 * - ppvObject = Address of a pointer variable that receives the interface pointer requested by riid.
 *               On return, *ppvObject contains the requested interface pointer, or NULL if the interface
 *               is not supported.
 *
 * Returns:
 * - S_OK = If the interface is supported. *ppvObject contains the pointer to the interface.
 * - E_NOINTERFACE = If the interface is not supported. *ppvObject contains NULL.
 * - E_POINTER = If ppvObject is NULL.
 */
/* This macro makes a ObjHlpStandardQueryInterface_IXXX function for any interface directly derived from IUnknown */
#define MAKE_BASE_QI(iface) \
HRESULT ObjHlpStandardQueryInterface_ ## iface (IUnknown *pThis, REFIID riid, PPVOID ppvObject) \
{ \
  if (!ppvObject) return E_POINTER; \
  *ppvObject = NULL; \
  if (!IsEqualIID(riid, &IID_ ## iface) && !IsEqualIID(riid, &IID_IUnknown)) \
    return E_NOINTERFACE; \
  IUnknown_AddRef(pThis); \
  *ppvObject = (PVOID)pThis; \
  return S_OK; \
} 

MAKE_BASE_QI(IConnectionPoint)
MAKE_BASE_QI(IEnumConnections)
MAKE_BASE_QI(IMalloc)
MAKE_BASE_QI(ISequentialStream)

/*
 * "Dummy" version of AddRef/Release used for static objects.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 *
 * Returns:
 * 1.
 */
UINT32 ObjHlpStaticAddRefRelease(IUnknown *pThis)
{
  return 1;
}

/*
 * Method returning void that does nothing.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 *
 * Returns:
 * Nothing.
 */
void ObjHlpDoNothingReturnVoid(IUnknown *pThis)
{
  /* do nothing */
}

/*
 * Method that does nothing and returns E_NOTIMPL.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 *
 * Returns:
 * E_NOTIMPL.
 */
HRESULT ObjHlpNotImplemented(IUnknown *pThis)
{
  return E_NOTIMPL;
}
