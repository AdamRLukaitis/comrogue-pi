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
#include <comrogue/internals/seg.h>
#include "enumgeneric.h"

/*---------------------------------------
 * IEnumUnknown interface implementation
 *---------------------------------------
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

/*
 * Adds a reference to the enumerator object.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumUnknown interface pointer).
 *
 * Returns:
 * The new reference count on the object.
 */
static UINT32 enumGeneric_AddRef(IUnknown *pThis)
{
  return ++(((PENUMGENERIC)pThis)->uiRefCount);
}

/*
 * Removes a reference from the enumerator object.  The object is freed when its reference count reaches 0.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumUnknown interface pointer).
 *
 * Returns:
 * The new reference count on the object.
 */
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

/*
 * Retrieves the next number of interface pointers in the enumeration sequence.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumUnknown interface pointer).
 * - celt = Number of interface pointers to be retrieved.  If there are more than this many interface pointers
 *          in the enumeration sequence, this many are returned.
 * - rgelt = Array of enumerated items which will be filled in by this function.  Note that the caller is
 *           responsible for releasing all interface pointers returned as a result of this call.
 * - pceltFetched = Pointer to a variable which will receive the number of interface pointers retrieved.  This
 *                  value will always be less than or equal to celt.
 *
 * Returns:
 * - S_OK = All the interface pointers requested were retrieved. *pceltFetched is equal to celt.
 * - S_FALSE = Not all the interface pointers requested could be retrieved. *pceltFetched is less than celt.
 * - Other = An error result.
 */
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
  { /* fetch the items - note we QI for the actual interface we're supposed to be enumerating for */
    if (FAILED(IUnknown_QueryInterface(peg->pPayload->rgObjects[i + peg->nCurrent], peg->pPayload->riidObjects,
				       (PPVOID)(&(rgelt[i])))))
      return E_UNEXPECTED;
  }

  peg->nCurrent += *pceltFetched;  /* advance pointer */
  return (*pceltFetched == celt) ? S_OK : S_FALSE;
}

/*
 * Skips over the next number of interface pointers in the enumeration sequence.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumUnknown interface pointer).
 * - celt = Number of elements to be skipped.  If there are fewer than this many elements
 *          in the enumeration sequence, we skip straight to the end.
 *
 * Returns:
 * - S_OK = We were able to skip over all the specified items.
 * - S_FALSE = We were not able to skip over all the specified items.  The current position of the enumeration
 *             is at the end of the sequence.
 */
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

/*
 * Resets the enumeration sequence to the beginning.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumUnknown interface pointer).
 *
 * Returns:
 * S_OK.
 */
static HRESULT enumGeneric_Reset(IEnumUnknown *pThis)
{
  ((PENUMGENERIC)pThis)->nCurrent = 0;
  return S_OK;
}

/*
 * Creates a new enumerator with the same state as the current one.
 *
 * Parameters:
 * - pThis = Pointer to the enumerator object (actually its IEnumUnknown interface pointer).
 * - ppEnum = Pointer to variable to receive the new enumerator object.  The caller is responsible for releasing
 *            this pointer.
 *
 * Returns:
 * - S_OK = Success.
 * - E_INVALIDARG = The pointer argument is not valid.
 * - E_OUTOFMEMORY = The memory for a new enumerator could not be allocated.
 */
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

/* VTable for the IEnumUnknown interface. */
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

/*
 * Allocate the "inner" connection data object for an enumerator.
 *
 * Parameters:
 * - pAllocator = Allocator to use to allocate the memory.
 * - riidObjects = IID that the objects stored in this block will actually have.  NULL is interpreted
 *                 as the IID of IUnknown.
 * - nCapacity = Number of slots to allocate for the object pointers.
 *
 * Returns:
 * - NULL = The data object could not be allocated.
 * - Other = Pointer to the new data object.  Its reference count is 0 and it contains no elements.
 */
PENUMGENERICDATA _ObjHlpAllocateEnumGenericData(IMalloc *pAllocator, REFIID riidObjects, UINT32 nCapacity)
{
  register PENUMGENERICDATA rc;               /* return from this function */
  register SIZE_T cbReturn = sizeof(ENUMGENERICDATA) + (nCapacity * sizeof(PUNKNOWN));  /* bytes to allocate */

  rc = (PENUMGENERICDATA)(IMalloc_Alloc(pAllocator, cbReturn));
  if (rc != NULL)
  {
    StrSetMem(rc, 0, cbReturn);
    rc->uiRefCount = 0;
    rc->riidObjects = riidObjects ? riidObjects : &IID_IUnknown;
    rc->nObjects = 0;
  }
  return rc;
}

/*
 * Add an element to the specified "inner" object reference data object.  Used when building the object.
 *
 * Parameters:
 * - pegd = Pointer to the reference data object.
 * - pUnk = IUnknown pointer to add to the "next" slot.  A reference is added to this pointer.
 *
 * Returns:
 * Nothing.
 */
void _ObjHlpAddToEnumGenericData(PENUMGENERICDATA pegd, IUnknown *pUnk)
{
  pegd->rgObjects[pegd->nObjects] = pUnk;
  IUnknown_AddRef(pegd->rgObjects[pegd->nObjects]);
  pegd->nObjects++;
}

/*
 * Discards the contents of an "inner" object reference data object by releasing all the interface pointers
 * it contains.
 *
 * Parameters:
 * - pegd = Pointer to the reference data object.
 *
 * Returns:
 * Nothing.
 */
void _ObjHlpDiscardEnumGenericData(PENUMGENERICDATA pegd)
{
  register UINT32 i;   /* loop counter */

  for (i = 0; i < pegd->nObjects; i++)
    IUnknown_Release(pegd->rgObjects[i]);
}

/*
 * Allocates a new "enumerator" data object.
 *
 * Parameters:
 * - pAllocator = Allocator to use to allocate the memory.
 * - riidActual = IID that this enumerator will actually support, in addition to IUnknown.  NULL is
 *                interpreted as IID_IEnumUnknown.
 * - pegd = Pointer to the "inner" reference data object to be used.
 * - nCurrent = Value to set the "current" element pointer to.
 *
 * Returns:
 * - NULL = The enumerator object could not be allocated.
 * - Other = Pointer to the new enumerator object.  Its reference count is 0.
 */
PENUMGENERIC _ObjHlpAllocateEnumGeneric(IMalloc *pAllocator, REFIID riidActual, PENUMGENERICDATA pegd,
					UINT32 nCurrent)
{
  register PENUMGENERIC rc;  /* return from this function */

  rc = (PENUMGENERIC)(IMalloc_Alloc(pAllocator, sizeof(ENUMGENERIC)));
  if (rc)
  { /* initialize all the fields */
    rc->enumUnknown.pVTable = &vtblEnum;
    rc->uiRefCount = 0;
    rc->riidActual = riidActual ? riidActual : &IID_IEnumUnknown;
    rc->nCurrent = nCurrent;
    rc->pPayload = pegd;
    pegd->uiRefCount++;
    rc->pAllocator = pAllocator;
    IUnknown_AddRef(rc->pAllocator);
  }
  return rc;
}
