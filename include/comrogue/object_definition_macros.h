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
  EXTERN_C extern const typ GUIDATTR name

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

#define BEGIN_INTERFACE(typ) \
  struct typ ## VTable {
#define BEGIN_INTERFACE_(typ, parent) \
  struct typ ## VTable {
#define END_INTERFACE(typ)     }; \
  typedef interface typ { const struct typ ## VTable *pVTable; } typ;

#else

#define STDMETHOD_(name, typ)   typ name
#define STDMETHOD(name)         HRESULT name
#define THIS_(typ)
#define THIS(typ)
#define PURE                    = 0
#define END_METHODS
#define INHERIT_METHODS(sym)

#define BEGIN_INTERFACE(typ) \
  interface typ {
#define BEGIN_INTERFACE_(typ, parent) \
  interface typ : parent {
#define END_INTERFACE(typ)   };

#endif  /* CINTERFACE */

#endif /* __ASM__ */

#endif /* __OBJECT_DEFINITION_MACROS_H_INCLUDED */
