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
#include <comrogue/connpoint.h>
#include <comrogue/objhelp.h>
#include "enumconn.h"

/* Mask we XOR with the index of the slot plus 1. */
#define FIXEDCP_MASK 0x4669782A      /* "Fix*" */

/*---------------------------------------------
 * IConnectionPoint vtable and implementations
 *---------------------------------------------
 */

/*
 * Adds a reference to the connection point object.
 *
 * Parameters:
 * - pThis = Pointer to the connection point object (actually its IConnectionPoint interface pointer).
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 fixedcp_AddRef(IUnknown *pThis)
{
  return IUnknown_AddRef(((PFIXEDCPDATA)pThis)->punkOuter);
}

/*
 * Removes a reference from the connection point object.  The object is freed when its reference count reaches 0.
 *
 * Parameters:
 * - pThis = Pointer to the connection point object (actually its IConnectionPoint interface pointer).
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 fixedcp_Release(IUnknown *pThis)
{
  return IUnknown_Release(((PFIXEDCPDATA)pThis)->punkOuter);
}

/*
 * Retrieves the IID of the outgoing interface managed by this connection point.
 *
 * Parameters:
 * - pThis = Pointer to the connection point object (actually its IConnectionPoint interface pointer).
 * - pIID = Pointer to buffer to receive the IID of the outgoing interface managed by this connection point.
 *
 * Returns:
 * Standard HRESULT success/failure indicator.
 */
static HRESULT fixedcp_GetConnectionInterface(IConnectionPoint *pThis, IID *pIID)
{
  if (!pIID)
    return E_POINTER;
  StrCopyMem(pIID, ((PFIXEDCPDATA)pThis)->riidConnection, sizeof(IID));
  return S_OK;
}

/*
 * Retrieves the IConnectionPointContainer interface for this connection point's parent connectable object.
 *
 * Parameters:
 * - pThis = Pointer to the connection point object (actually its IConnectionPoint interface pointer).
 * - ppCPC = Pointer to variable to receive the IConnectionPointContainer interface. The caller is responsible
 *           for releasing this interface pointer.
 *
 * Returns:
 * Standard HRESULT success/failure indicator.
 */
static HRESULT fixedcp_GetConnectionPointContainer(IConnectionPoint *pThis, IConnectionPointContainer **ppCPC)
{
  register HRESULT rc;  /* return from underlying QI */

  rc = IUnknown_QueryInterface(((PFIXEDCPDATA)pThis)->punkOuter, &IID_IConnectionPointContainer, (PPVOID)ppCPC);
  if (rc == E_NOINTERFACE)
    rc = E_UNEXPECTED;
  return rc;
}

/*
 * Establishes a connection between the connection point and an event sink interface.
 *
 * Parameters:
 * - pThis = Pointer to the connection point object (actually its IConnectionPoint interface pointer).
 * - punkSink = Pointer to the event sink interface to attach.
 * - puiCookie = Pointer to a variable to receive a "cookie" that can later be used with the Unadvise method
 *               on this interface to delete the connection.
 *
 * Returns:
 * - S_OK = Connection established; *puiCookie contains the connection cookie.
 * - E_POINTER = Either punkSink or puiCookie is invalid.
 * - CONNECT_E_ADVISELIMIT = The connection point cannot accept any more connections.
 * - CONNECT_E_CANNOTCONNECT = The event sink does not support the correct interface.
 */
static HRESULT fixedcp_Advise(IConnectionPoint *pThis, IUnknown *punkSink, UINT32 *puiCookie)
{
  register PFIXEDCPDATA pData;       /* pointer to actual data block */
  register UINT32 i;                 /* index of slot to put sink in */
  IUnknown *punkActualSink = NULL;   /* actual sink pointer */

  if (!punkSink || !puiCookie)
    return E_POINTER;   /* bogus */
  *puiCookie = 0;

  pData = (PFIXEDCPDATA)pThis;
  if (pData->ncpSize == pData->ncpCapacity)
    return CONNECT_E_ADVISELIMIT;  /* too many connections */

  if (FAILED(IUnknown_QueryInterface(punkSink, pData->riidConnection, (PPVOID)(&punkActualSink))))
    return CONNECT_E_CANNOTCONNECT; /* cannot QI for our interface */

  for (i = 0; i < pData->ncpCapacity; i++)
  { /* search for a slot */
    if (!(pData->ppSlots[i]))
    { /* found slot, store sink */
      pData->ppSlots[i] = punkActualSink;
      *puiCookie = (UINT32)((i + 1) ^ FIXEDCP_MASK);
      pData->ncpSize++;
      return S_OK;
    }
  }
  
  /* slot not found?!? */
  IUnknown_Release(punkActualSink);
  return E_UNEXPECTED;
}

/*
 * Terminates a previously-established connection between the connection point and an event sink interface.
 *
 * Parameters:
 * - pThis = Pointer to the connection point object (actually its IConnectionPoint interface pointer).
 * - uiCookie = Cookie previously returned by the Advise method on this interface.
 *
 * Returns:
 * - S_OK = Connection was terminated successfully.
 * - E_POINTER = The cookie value is invalid.
 */
static HRESULT fixedcp_Unadvise(IConnectionPoint *pThis, UINT32 uiCookie)
{
  register PFIXEDCPDATA pData = (PFIXEDCPDATA)pThis; /* pointer to actual data block */
  register UINT32 i;                                 /* index of slot sink is in */

  /* Decode the cookie value */
  i = (UINT32)(uiCookie ^ FIXEDCP_MASK) - 1;
  if ((i < 0) || (i >= pData->ncpCapacity) || !(pData->ppSlots[i]))
    return E_POINTER;

  /* Free up the slot */
  IUnknown_Release(pData->ppSlots[i]);
  pData->ppSlots[i] = NULL;
  pData->ncpSize--;
  return S_OK;
}

/*
 * Creates an enumerator object to iterate through the current connections on this connection point.
 *
 * Parameters:
 * - pThis = Pointer to the connection point object (actually its IConnectionPoint interface pointer).
 * - ppEnum = Pointer to variable to receive the enumerator object interface.  The caller is responsible for
 *            releasing this object when it's no longer needed.
 *
 * Returns:
 * - S_OK = Created the enumerator successfully.
 * - E_NOTIMPL = Enumeration is not supported.
 * - E_POINTER = ppEnum was not a valid pointer.
 * - E_OUTOFMEMORY = Could not allocate the enumerator.
 */
static HRESULT fixedcp_EnumConnections(IConnectionPoint *pThis, IEnumConnections **ppEnum)
{
  register PFIXEDCPDATA pData = (PFIXEDCPDATA)pThis; /* pointer to actual data block */
  PENUMCONNDATA pecd;                                /* pointer to enumeration data */
  PENUMCONN pec;                                     /* pointer to enumerator */
  register UINT32 i;                                 /* loop counter */

  if (!(pData->pAllocator))
    return E_NOTIMPL;   /* enumeration not implemented */

  if (!ppEnum)
    return E_POINTER;  /* bad return pointer */
  *ppEnum = NULL;

  /* Allocate a data block. */
  pecd = _ObjHlpAllocateEnumConnData(pData->pAllocator, pData->ncpSize);
  if (!pecd)
    return E_OUTOFMEMORY;

  /* Fill the allocated data block. */
  for (i = 0; i < pData->ncpCapacity; i++)
    if (pData->ppSlots[i])
      _ObjHlpAddToEnumConnData(pecd, pData->ppSlots[i], (UINT32)((i + 1) ^ FIXEDCP_MASK));

  /* Allocate the enumerator. */
  pec = _ObjHlpAllocateEnumConn(pData->pAllocator, pecd, 0);
  if (pec)
  { /* QI it for our return value */
    IUnknown_QueryInterface((IUnknown *)pec, &IID_IEnumConnections, (PPVOID)ppEnum);
    return S_OK;
  }

  /* unable to allocate enumerator - bug out */
  _ObjHlpDiscardEnumConnData(pecd);
  IMalloc_Free(pData->pAllocator, pecd);
  return E_OUTOFMEMORY;
}

/* VTable for the fixed-size connection point implementation. */
static const SEG_RODATA struct IConnectionPointVTable vtblFixedCP = 
{
  .QueryInterface = ObjHlpStandardQueryInterface_IConnectionPoint,
  .AddRef = fixedcp_AddRef,
  .Release = fixedcp_Release,
  .GetConnectionInterface = fixedcp_GetConnectionInterface,
  .GetConnectionPointContainer = fixedcp_GetConnectionPointContainer,
  .Advise = fixedcp_Advise,
  .Unadvise = fixedcp_Unadvise,
  .EnumConnections = fixedcp_EnumConnections
};

/*-----------------------------------------------
 * Connection point setup and teardown functions
 *-----------------------------------------------
 */

/*
 * Sets up a fixed-size connection point object.
 *
 * Parameters:
 * - pData = Pointer to the data block to initialize.
 * - punkOuter = Pointer to "outer unknown" for the connection point, which must implement IConnectionPointContainer.
 * - riidConnection = Reference to the IID of the outgoing event interface.
 * - ppSlots = Pointer to the array of IUnknown "slots" used by this connection point.
 * - nSlots = Number of slots to allocate.
 * - pAllocator = Pointer to a memory allocator to use to allocate enumerators.  This may be NULL if you don't
 *                want to support enumeration.
 *
 * Returns:
 * Nothing.
 */
void ObjHlpFixedCpSetup(PFIXEDCPDATA pData, PUNKNOWN punkOuter, REFIID riidConnection,
			IUnknown **ppSlots, UINT32 nSlots, IMalloc *pAllocator)
{
  pData->connectionPointInterface.pVTable = &vtblFixedCP;
  pData->punkOuter = punkOuter;
  pData->riidConnection = riidConnection;
  pData->ppSlots = ppSlots;
  pData->ncpSize = 0;
  pData->ncpCapacity = nSlots;
  pData->pAllocator = pAllocator;
  if (pData->pAllocator)
    IUnknown_AddRef(pData->pAllocator);
  StrSetMem(ppSlots, 0, nSlots * sizeof(IUnknown *));
}

/*
 * Tears down a fixed-size connection-point object.
 *
 * Parameters:
 * - pData = Pointer to the data block to destroy.
 *
 * Returns:
 * Nothing.
 */
void ObjHlpFixedCpTeardown(PFIXEDCPDATA pData)
{
  register UINT32 i; /* loop counter */

  for (i = 0; i < pData->ncpCapacity; i++)
    if (pData->ppSlots[i])
      IUnknown_Release(pData->ppSlots[i]);
  if (pData->pAllocator)
    IUnknown_Release(pData->pAllocator);
  StrSetMem(pData, 0, sizeof(FIXEDCPDATA));
}
