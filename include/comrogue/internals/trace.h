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
#ifndef __TRACE_H_INCLUDED
#define __TRACE_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

#ifndef __ASM__

#include <stdarg.h>
#include <comrogue/types.h>
#include <comrogue/scode.h>
#include <comrogue/compiler_macros.h>
#include <comrogue/internals/seg.h>

/*------------------------
 * System trace functions
 *------------------------
 */

CDECL_BEGIN

#ifdef __COMROGUE_PRESTART__

extern void ETrInit(void);
extern void ETrWriteChar8(CHAR c);
extern void ETrWriteString8(PCSTR psz);
extern void ETrWriteWord(UINT32 uiValue);
extern void ETrDumpWords(PUINT32 puiWords, UINT32 cWords);
extern void ETrAssertFailed(PCSTR pszFile, INT32 nLine);
extern void ETrInfiniBlink(void);

#define ASSERT_FAIL_FUNC ETrAssertFailed

#else

extern void TrInit(void);
extern void TrWriteChar8(CHAR c);
extern void TrWriteString8(PCSTR str);
extern void TrWriteWord(UINT32 uiValue);
extern HRESULT TrVPrintf8(PCSTR pszFormat, va_list pargs);
extern HRESULT TrPrintf8(PCSTR pszFormat, ...);
extern void TrAssertFailed(PCSTR pszFile, INT32 nLine);
extern void TrInfiniBlink(void);

#define ASSERT_FAIL_FUNC TrAssertFailed

#endif /* __COMROGUE_PRESTART__ */

CDECL_END

/*------------------------------------------------
 * Macro definitions for the assert functionality
 *------------------------------------------------
 */

#ifndef NDEBUG

#define THIS_FILE __FILE__

#if defined(__COMROGUE_PRESTART__) || defined(__COMROGUE_INIT__)
#define DECLARE_THIS_FILE     static DECLARE_INIT_STRING8_CONST(THIS_FILE, __FILE__);
#else
#define DECLARE_THIS_FILE     static DECLARE_STRING8_CONST(THIS_FILE, __FILE__);
#endif

#define ASSERT(x)     ((x) ? (void)0 : ASSERT_FAIL_FUNC(THIS_FILE, __LINE__))
#define VERIFY(x)     ASSERT(x)

#else

#define DECLARE_THIS_FILE

#define ASSERT(x)     ((void)0)
#define VERIFY(x)     ((void)(x))

#endif /* NDEBUG */

#endif /* __ASM__ */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __TRACE_H_INCLUDED */
