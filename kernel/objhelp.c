#include <comrogue/types.h>
#include <comrogue/scode.h>
#include <comrogue/str.h>
#include <comrogue/object_types.h>
#include <comrogue/objectbase.h>
#include <comrogue/allocator.h>
#include <comrogue/stdobj.h>
#include <comrogue/objhelp.h>
#include <comrogue/internals/seg.h>

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
SEG_LIB_CODE BOOL IsEqualGUID(REFGUID guid1, REFGUID guid2)
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
SEG_LIB_CODE HRESULT ObjHlpStandardQueryInterface_ ## iface (IUnknown *pThis, REFIID riid, PPVOID ppvObject) \
{ \
  if (!ppvObject) return E_POINTER; \
  *ppvObject = NULL; \
  if (!IsEqualIID(riid, &IID_ ## iface) && !IsEqualIID(riid, &IID_IUnknown)) \
    return E_NOINTERFACE; \
  IUnknown_AddRef(pThis); \
  *ppvObject = (PVOID)pThis; \
  return S_OK; \
} 

MAKE_BASE_QI(IMalloc)

/*
 * "Dummy" version of AddRef/Release used for static objects.
 *
 * Parameters:
 * - pThis = Base interface pointer.
 *
 * Returns:
 * 1.
 */
SEG_LIB_CODE UINT32 ObjHlpStaticAddRefRelease(IUnknown *pThis)
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
SEG_LIB_CODE void ObjHlpDoNothingReturnVoid(IUnknown *pThis)
{
  /* do nothing */
}
