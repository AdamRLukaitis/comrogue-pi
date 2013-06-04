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
#include <comrogue/intlib.h>
#include <comrogue/str.h>
#include <comrogue/objectbase.h>
#include <comrogue/allocator.h>
#include <comrogue/objhelp.h>
#include <comrogue/stdobj.h>
#include "enumgeneric.h"

/*---------------------------------------
 * IEnumUnknown interface implementation
 *---------------------------------------
 */

static HRESULT enumGeneric_QueryInterface(IUnknown *pThis, REFIID riid, PPVOID ppvObject)
{
  PENUMGENERIC peg = (PENUMGENERIC)pThis;

  if (!ppvObject)
    return E_POINTER;
  if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, peg->riidActual))
  {
    *ppvObject = pThis;
    IUnknown_AddRef((PUNKNOWN)(*ppvObject));
    return S_OK;
  }

  *ppvObject = NULL;
  return E_NOINTERFACE;
}

static UINT32 enumGeneric_AddRef(IUnknown *pThis)
{
  return ++(((PENUMGENERIC)pThis)->uiRefCount);
}

static UINT32 enumGeneric_Release(IUnknown *pThis)
{
  register PENUMGENERIC peg = (PENUMGENERIC)pThis;  /* pointer to actual data block */
  register UINT32 rc;     /* return from this function */
  IMalloc *pAllocator;    /* temporary for allocator */
  PENUMGENERICDATA pegd;  /* temporary for underlying objects data block */

  rc = --(peg->uiRefCount);
  if (rc == 0)
  { /* save off allocator before deallocating */
    pAllocator = peg->pAllocator;
    IUnknown_AddRef(pAllocator);
    /* save off payload too */
    pegd = peg->pPayload;
    /* erase enumerator */
    IUnknown_Release(peg->pAllocator);
    IMalloc_Free(pAllocator, peg);
    if (--(pegd->uiRefCount) == 0)
    { /* we need to erase the data block too */
      _ObjHlpDiscardEnumGenericData(pegd);
      IMalloc_Free(pAllocator, pegd);
    }
    IUnknown_Release(pAllocator); /* release temporary reference we added */
  }
  return rc;
}

static HRESULT enumGeneric_Next(IEnumUnknown *pThis, UINT32 celt, IUnknown **rgelt, UINT32 *pceltFetched)
{
  register PENUMGENERIC peg = (PENUMGENERIC)pThis; /* pointer to data block */
  register UINT32 i;    /* loop counter */

  if (celt == 0)
  { /* nothing to retrieve */
    if (pceltFetched)
      *pceltFetched = 0;
    return S_OK;
  }

  if (!pceltFetched)
    return E_POINTER;  /* invalid pointer */

  /* get number of items to fetch */
  *pceltFetched = intMin(peg->pPayload->nObjects - peg->nCurrent, celt);
  for (i = 0; i < *pceltFetched; i++)
  { /* fetch the items */
    rgelt[i] = peg->pPayload->rgObjects[i + peg->nCurrent];
    IUnknown_AddRef(rgelt[i]);
  }

  peg->nCurrent += *pceltFetched;  /* advance pointer */
  return (*pceltFetched == celt) ? S_OK : S_FALSE;
}

static HRESULT enumGeneric_Skip(IEnumUnknown *pThis, UINT32 celt)
{
  register PENUMGENERIC peg = (PENUMGENERIC)pThis; /* pointer to data block */
  register HRESULT rc = S_OK;   /* return from this function */

  peg->nCurrent += celt;
  if (peg->nCurrent > peg->pPayload->nObjects)
  { /* clamp current pointer and return FALSE */
    peg->nCurrent = peg->pPayload->nObjects;
    rc = S_FALSE;
  }
  return rc;
}

static HRESULT enumGeneric_Reset(IEnumUnknown *pThis)
{
  ((PENUMGENERIC)pThis)->nCurrent = 0;
  return S_OK;
}

static HRESULT enumGeneric_Clone(IEnumUnknown *pThis, IEnumUnknown **ppEnum)
{
  register PENUMGENERIC peg = (PENUMGENERIC)pThis;  /* pointer to data block */
  register PENUMGENERIC pegNew;    /* pointer to new enumerator */

  if (!ppEnum)   /* no pointer? */
    return E_INVALIDARG;
  *ppEnum = NULL;
  pegNew = _ObjHlpAllocateEnumGeneric(peg->pAllocator, peg->riidActual, peg->pPayload, peg->nCurrent);
  if (!pegNew)  /* failed to allocate? */
    return E_OUTOFMEMORY;
  IUnknown_QueryInterface((IUnknown *)pegNew, peg->riidActual, (PPVOID)ppEnum);
  return S_OK;
}

static const SEG_RODATA struct IEnumUnknownVTable vtblEnum =
{
  .QueryInterface = enumGeneric_QueryInterface,
  .AddRef = enumGeneric_AddRef,
  .Release = enumGeneric_Release,
  .Next = enumGeneric_Next,
  .Skip = enumGeneric_Skip,
  .Reset = enumGeneric_Reset,
  .Clone = enumGeneric_Clone
};

/*---------------------------------
 * IEnum[Generic] helper functions
 *---------------------------------
 */

PENUMGENERICDATA _ObjHlpAllocateEnumGenericData(IMalloc *pAllocator, UINT32 nCapacity)
{
  register PENUMGENERICDATA rc;               /* return from this function */
  register SIZE_T cbReturn = sizeof(ENUMGENERICDATA) + (nCapacity * sizeof(PUNKNOWN));  /* bytes to allocate */

  rc = (PENUMGENERICDATA)(IMalloc_Alloc(pAllocator, cbReturn));
  if (rc != NULL)
  {
    StrSetMem(rc, 0, cbReturn);
    rc->uiRefCount = 0;
    rc->nObjects = 0;
  }
  return rc;

}

void _ObjHlpAddToEnumGenericData(PENUMGENERICDATA pegd, IUnknown *pUnk)
{
  pegd->rgObjects[pegd->nObjects] = pUnk;
  IUnknown_AddRef(pegd->rgObjects[pegd->nObjects]);
  pegd->nObjects++;
}

void _ObjHlpDiscardEnumGenericData(PENUMGENERICDATA pegd)
{
  register UINT32 i;   /* loop counter */

  for (i = 0; i < pegd->nObjects; i++)
    IUnknown_Release(pegd->rgObjects[i]);
}

PENUMGENERIC _ObjHlpAllocateEnumGeneric(IMalloc *pAllocator, REFIID riidActual, PENUMGENERICDATA pegd,
					UINT32 nCurrent)
{
  register PENUMGENERIC rc;  /* return from this function */

  rc = (PENUMGENERIC)(IMalloc_Alloc(pAllocator, sizeof(ENUMGENERIC)));
  if (rc)
  { /* initialize all the fields */
    rc->enumUnknown.pVTable = &vtblEnum;
    rc->uiRefCount = 0;
    rc->riidActual = riidActual;
    rc->nCurrent = nCurrent;
    rc->pPayload = pegd;
    pegd->uiRefCount++;
    rc->pAllocator = pAllocator;
    IUnknown_AddRef(rc->pAllocator);
  }
  return rc;
}
