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
#include <comrogue/connpoint.h>
#include <comrogue/allocator.h>
#include <comrogue/objhelp.h>
#include "enumconn.h"

/*-------------------------------------------
 * IEnumConnections interface implementation
 *-------------------------------------------
 */

/*
 * Adds a reference to the enumerator object.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumConnections interface pointer).
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 enumConn_AddRef(IUnknown *pThis)
{
  return ++(((PENUMCONN)pThis)->uiRefCount);
}

/*
 * Removes a reference from the enumerator object.  The object is freed when its reference count reaches 0.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumConnections interface pointer).
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 enumConn_Release(IUnknown *pThis)
{
  register PENUMCONN pec = (PENUMCONN)pThis;  /* pointer to actual data block */
  register UINT32 rc;     /* return from this function */
  IMalloc *pAllocator;    /* temporary for allocator */
  PENUMCONNDATA pecd;     /* temporary for underlying connections data block */

  rc = --(pec->uiRefCount);
  if (rc == 0)
  { /* save off allocator before deallocating */
    pAllocator = pec->pAllocator;
    IUnknown_AddRef(pAllocator);
    /* save off payload too */
    pecd = pec->pPayload;
    /* erase enumerator */
    IUnknown_Release(pec->pAllocator);
    IMalloc_Free(pAllocator, pec);
    if (--(pecd->uiRefCount) == 0)
    { /* we need to erase the data block too */
      _ObjHlpDiscardEnumConnData(pecd);
      IMalloc_Free(pAllocator, pecd);
    }
    IUnknown_Release(pAllocator); /* release temporary reference we added */
  }
  return rc;
}

/*
 * Retrieves the next number of connections in the enumeration sequence.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumConnections interface pointer).
 * - cConnections = Number of connections to be retrieved.  If there are more than this many connections
 *                  in the enumeration sequence, this many are returned.
 * - rgcd = Array of enumerated items which will be filled in by this function.  Note that the caller is
 *          responsible for releasing all interface pointers returned as a result of this call.
 * - pcFetched = Pointer to a variable which will receive the number of connections retrieved.  This value will
 *               always be less than or equal to cConnections.
 *
 * Returns:
 * - S_OK = All the connections requested were retrieved. *pcFetched is equal to cConnections.
 * - S_FALSE = Not all the connections requested could be retrieved. *pcFetched is less than cConnections.
 * - Other = An error result.
 */
static HRESULT enumConn_Next(IEnumConnections *pThis, UINT32 cConnections, PCONNECTDATA rgcd, UINT32 *pcFetched)
{
  register PENUMCONN pec = (PENUMCONN)pThis; /* pointer to data block */
  register UINT32 i;    /* loop counter */

  if (cConnections == 0)
  { /* nothing to retrieve */
    if (pcFetched)
      *pcFetched = 0;
    return S_OK;
  }

  if (!pcFetched)
    return E_POINTER;  /* invalid pointer */

  /* get number of items to fetch */
  *pcFetched = intMin(pec->pPayload->nConnectData - pec->nCurrent, cConnections);
  for (i = 0; i < *pcFetched; i++)
  { /* fetch the items */
    rgcd[i].pUnk = pec->pPayload->rgConnectData[i + pec->nCurrent].pUnk;
    IUnknown_AddRef(rgcd[i].pUnk);
    rgcd[i].uiCookie = pec->pPayload->rgConnectData[i + pec->nCurrent].uiCookie;
  }

  pec->nCurrent += *pcFetched;  /* advance pointer */
  return (*pcFetched == cConnections) ? S_OK : S_FALSE;
}

/*
 * Skips over the next number of connections in the enumeration sequence.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumConnections interface pointer).
 * - cConnections = Number of connections to be skipped.  If there are fewer than this many connections
 *                  in the enumeration sequence, we skip straight to the end.
 *
 * Returns:
 * - S_OK = We were able to skip over all the specified items.
 * - S_FALSE = We were not able to skip over all the specified items.  The current position of the enumeration
 *             is at the end of the sequence.
 */
static HRESULT enumConn_Skip(IEnumConnections *pThis, UINT32 cConnections)
{
  register PENUMCONN pec = (PENUMCONN)pThis;  /* pointer to data block */
  register HRESULT rc = S_OK;   /* return from this function */

  pec->nCurrent += cConnections;
  if (pec->nCurrent > pec->pPayload->nConnectData)
  { /* clamp current pointer and return FALSE */
    pec->nCurrent = pec->pPayload->nConnectData;
    rc = S_FALSE;
  }
  return rc;
}

/*
 * Resets the enumeration sequence to the beginning.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumConnections interface pointer).
 *
 * Returns:
 * S_OK.
 */
static HRESULT enumConn_Reset(IEnumConnections *pThis)
{
  ((PENUMCONN)pThis)->nCurrent = 0;
  return S_OK;
}

/*
 * Creates a new enumerator with the same state as the current one.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumConnections interface pointer).
 * - ppEnum = Pointer to variable to receive the new enumerator object.  The caller is responsible for releasing
 *            this pointer.
 *
 * Returns:
 * - S_OK = Success.
 * - E_INVALIDARG = The pointer argument is not valid.
 * - E_OUTOFMEMORY = The memory for a new enumerator could not be allocated.
 */
static HRESULT enumConn_Clone(IEnumConnections *pThis, IEnumConnections **ppEnum)
{
  register PENUMCONN pec = (PENUMCONN)pThis;  /* pointer to data block */
  register PENUMCONN pecNew;    /* pointer to new enumerator */

  if (!ppEnum)   /* no pointer? */
    return E_INVALIDARG;
  *ppEnum = NULL;
  pecNew = _ObjHlpAllocateEnumConn(pec->pAllocator, pec->pPayload, pec->nCurrent);
  if (!pecNew)  /* failed to allocate? */
    return E_OUTOFMEMORY;
  IUnknown_QueryInterface((IUnknown *)pecNew, &IID_IEnumConnections, (PPVOID)ppEnum);
  return S_OK;
}

/* The VTable for the IEnumConnections interface. */
static const SEG_RODATA struct IEnumConnectionsVTable vtblEnumConnections = 
{
  .QueryInterface = ObjHlpStandardQueryInterface_IEnumConnections,
  .AddRef = enumConn_AddRef,
  .Release = enumConn_Release,
  .Next = enumConn_Next,
  .Skip = enumConn_Skip,
  .Reset = enumConn_Reset,
  .Clone = enumConn_Clone
};

/*-----------------------------------
 * IEnumConnections helper functions
 *-----------------------------------
 */

/*
 * Allocate the "inner" connection data object for an enumerator.
 *
 * Parameters:
 * - pAllocator = Allocator to use to allocate the memory.
 * - nCapacity = Number of slots to allocate for the connection data.
 *
 * Returns:
 * - NULL = The data object could not be allocated.
 * - Other = Pointer to the new data object.  Its reference count is 0 and it contains no elements.
 */
PENUMCONNDATA _ObjHlpAllocateEnumConnData(IMalloc *pAllocator, UINT32 nCapacity)
{
  register PENUMCONNDATA rc;               /* return from this function */
  register SIZE_T cbReturn = sizeof(ENUMCONNDATA) + (nCapacity * sizeof(CONNECTDATA));  /* bytes to allocate */

  rc = (PENUMCONNDATA)(IMalloc_Alloc(pAllocator, cbReturn));
  if (rc != NULL)
  {
    StrSetMem(rc, 0, cbReturn);
    rc->uiRefCount = 0;
    rc->nConnectData = 0;
  }
  return rc;
}

/*
 * Add an element to the specified "inner" connection data object.  Used when building the object.
 *
 * Parameters:
 * - pecd = Pointer to the connection data object.
 * - pUnk = IUnknown pointer to add to the "next" slot.  A reference is added to this pointer.
 * - uiCookie = Cookie value to add to the "next" slot.
 *
 * Returns:
 * Nothing.
 */
void _ObjHlpAddToEnumConnData(PENUMCONNDATA pecd, IUnknown *pUnk, UINT32 uiCookie)
{
  pecd->rgConnectData[pecd->nConnectData].pUnk = pUnk;
  IUnknown_AddRef(pecd->rgConnectData[pecd->nConnectData].pUnk);
  pecd->rgConnectData[pecd->nConnectData].uiCookie = uiCookie;
  pecd->nConnectData++;
}

/*
 * Discards the contents of an "inner" connection data object by releasing all the interface pointers
 * it contains.
 *
 * Parameters:
 * - pecd = Pointer to the connection data object.
 *
 * Returns:
 * Nothing.
 */
void _ObjHlpDiscardEnumConnData(PENUMCONNDATA pecd)
{
  register UINT32 i;  /* loop counter */

  for (i = 0; i < pecd->nConnectData; i++)
    IUnknown_Release(pecd->rgConnectData[i].pUnk);
}

/*
 * Allocates a new "enumerator" data object.
 *
 * Parameters:
 * - pAllocator = Allocator to use to allocate the memory.
 * - pecd = Pointer to the "inner" connection data object to be used.
 * - nCurrent = Value to set the "current" element pointer to.
 *
 * Returns:
 * - NULL = The enumerator object could not be allocated.
 * - Other = Pointer to the new enumerator object.  Its reference count is 0.
 */
PENUMCONN _ObjHlpAllocateEnumConn(IMalloc *pAllocator, PENUMCONNDATA pecd, UINT32 nCurrent)
{
  register PENUMCONN rc;  /* return from this function */

  rc = (PENUMCONN)(IMalloc_Alloc(pAllocator, sizeof(ENUMCONN)));
  if (rc)
  { /* initialize all the fields */
    rc->enumConnections.pVTable = &vtblEnumConnections;
    rc->uiRefCount = 0;
    rc->nCurrent = nCurrent;
    rc->pPayload = pecd;
    pecd->uiRefCount++;
    rc->pAllocator = pAllocator;
    IUnknown_AddRef(rc->pAllocator);
  }
  return rc;
}
