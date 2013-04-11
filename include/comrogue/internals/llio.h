#ifndef __LLIO_H_INCLUDED
#define __LLIO_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

#ifndef __ASM__

#include <comrogue/types.h>
#include <comrogue/compiler_macros.h>

/*--------------------------------------------------------------------------------------
 * Functions for performing low-level IO (direct read and write of memory-mapped ports)
 *--------------------------------------------------------------------------------------
 */

#ifdef __COMROGUE_PRESTART__

CDECL_BEGIN

extern void llIOWritePA(PHYSADDR paPort, UINT32 uiData);
extern UINT32 llIOReadPA(PHYSADDR paPort);
extern void llIODelayPA(UINT32 uiTicks);

#define llIOWrite(addr, data) llIOWritePA(addr, data)
#define llIORead(addr)        llIOReadPA(addr)
#define llIODelay(ticks)      llIODelayPA(ticks)

CDECL_END

#else

CDECL_BEGIN

extern void llIOWriteK(KERNADDR kaPort, UINT32 uiData);
extern UINT32 llIOReadK(KERNADDR kaPort);
extern void llIODelay(UINT32 uiTicks);

CDECL_END

#define _LLIOMAP(addr)        ((addr) + 0xC0000000)

#define llIOWrite(addr, data) llIOWriteK(_LLIOMAP(addr), data)
#define llIORead(addr)        llIOReadK(_LLIOMAP(addr))

#endif /* __COMROGUE_PRESTART__ */

#endif  /* __ASM__ */

#endif  /* __COMROGUE_INTERNALS__ */

#endif  /* __LLIO_H_INCLUDED */
