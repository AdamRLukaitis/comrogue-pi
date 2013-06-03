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

/*---------------------------------------------
 * IConnectionPoint vtable and implementations
 *---------------------------------------------
 */

static UINT32 fixedcp_AddRef(IUnknown *pThis)
{
  return IUnknown_AddRef(((PFIXEDCPDATA)pThis)->punkOuter);
}

static UINT32 fixedcp_Release(IUnknown *pThis)
{
  return IUnknown_Release(((PFIXEDCPDATA)pThis)->punkOuter);
}

static HRESULT fixedcp_GetConnectionInterface(IConnectionPoint *pThis, IID *pIID)
{
  if (!pIID)
    return E_POINTER;
  StrCopyMem(pIID, ((PFIXEDCPDATA)pThis)->riidConnection, sizeof(IID));
  return S_OK;
}

static HRESULT fixedcp_GetConnectionPointContainer(IConnectionPoint *pThis, IConnectionPointContainer *ppCPC)
{
  register HRESULT rc;  /* return from underlying QI */

  rc = IUnknown_QueryInterface(((PFIXEDCPDATA)pThis)->punkOuter, &IID_IConnectionPointContainer, (PPVOID)ppCPC);
  if (rc == E_NOINTERFACE)
    rc = E_UNEXPECTED;
  return rc;
}

static HRESULT fixedcp_Advise(IConnectionPoint *pThis, IUnknown *punkSink, UINT32 *puiCookie)
{
  return E_NOTIMPL; /* TODO */
}

static HRESULT fixedcp_Unadvise(IConnectionPoint *pThis, UINT32 uiCookie)
{
  return E_NOTIMPL; /* TODO */
}

static HRESULT fixedcp_EnumConnections(IConnectionPoint *pThis, IEnumConnections **ppEnum)
{
  return E_NOTIMPL; /* TODO */
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

void ObjHlpFixedCpSetup(PFIXEDCPDATA pData, PUNKNOWN punkOuter, REFIID riidConnection,
			IUnknown **ppSlots, INT32 nSlots)
{
  pData->connectionPointInterface.pVTable = &vtblFixedCP;
  pData->punkOuter = punkOuter;
  pData->riidConnection = riidConnection;
  pData->ppSlots = ppSlots;
  pData->ncpSize = 0;
  pData->ncpCapacity = nSlots;
  StrSetMem(ppSlots, 0, nSlots * sizeof(IUnknown *));
}

void ObjHlpFixedCpTeardown(PFIXEDCPDATA pData)
{
  register INT32 i; /* loop counter */

  for (i = 0; i < pData->ncpCapacity; i++)
    if (pData->ppSlots[i])
      IUnknown_Release(pData->ppSlots[i]);
  StrSetMem(pData, 0, sizeof(FIXEDCPDATA));
}
