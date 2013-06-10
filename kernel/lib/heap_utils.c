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
/*
 * This code is based on/inspired by jemalloc-3.3.1.  Please see LICENSE.jemalloc for further details.
 */
#include <stdarg.h>
#include <comrogue/compiler_macros.h>
#include <comrogue/types.h>
#include <comrogue/str.h>
#include <comrogue/objectbase.h>
#include <comrogue/stdobj.h>
#include <comrogue/stream.h>
#include <comrogue/internals/seg.h>
#include "heap_internals.h"

#ifdef _H_THIS_FILE
#undef _H_THIS_FILE
_DECLARE_H_THIS_FILE
#endif

/*---------------------------------
 * Utility and debugging functions
 *---------------------------------
 */

/*
 * Write a string to debugging output (if it's been configured).
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - sz = String to be written.
 *
 * Returns:
 * Nothing.
 */
void _HeapDbgWrite(PHEAPDATA phd, PCSTR sz)
{
  if (phd->pDebugStream)
    ISequentialStream_Write(phd->pDebugStream, sz, StrLength8(sz), NULL);
}

/*
 * Internal function called to write formatted output to the debugging output stream.
 *
 * Parameters:
 * - ppvArg = Pointer argument to StrFormatV8 (actually the ISequentialStream pointer).
 * - pchData = Pointer to data to be written.
 * - cbData = Number of characters to be written.
 *
 * Returns:
 * Standard HRESULT success/failure indicator.
 */
static HRESULT heap_printf_func(PPVOID ppvArg, PCCHAR pchData, UINT32 cbData)
{
  return ISequentialStream_Write(((PSEQUENTIALSTREAM)ppvArg), pchData, cbData, NULL);
}

/*
 * Formats data to the debugging output (if it's been configured).
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - szFormat = Printf-style format string.
 * - <var> = Arguments to be substituted into the printf-style format string.
 *
 * Returns:
 * Nothing.
 */
void _HeapPrintf(PHEAPDATA phd, PCSTR szFormat, ...)
{
  va_list pargs;

  if (phd->pDebugStream)
  {
    va_start(pargs, szFormat);
    StrFormatV8(heap_printf_func, (PPVOID)(phd->pDebugStream), szFormat, pargs);
    va_end(pargs);
  }
}

/*
 * Called when an assertion in the code fails; it prints the assertion failure to the debugging output (if set)
 * and calls the heap's abort function (if set).
 *
 * Parameters:
 * - phd = Pointer to the HEAPDATA block.
 * - szFile = File name where the assertion failed.
 * - szLine = Line number where the assertion failed.
 *
 * Returns:
 * Nothing.
 */
void _HeapAssertFailed(PHEAPDATA phd, PCSTR szFile, INT32 nLine)
{
  static const char SEG_RODATA szMessage[] = "_HeapAssertFailed at %s:%d\n";
  _HeapPrintf(phd, szMessage, szFile, nLine);
  if (phd->pfnAbort)
    (*(phd->pfnAbort))(phd->pvAbortArg);
}

/*
 * Compute the smallest power of 2 greater than or equal to its argument.
 *
 * Parameters:
 * - x = The value to compute.
 *
 * Returns:
 * The value that is the smallest power of 2 greater than or equal to x.
 */
SIZE_T _HeapPow2Ceiling(SIZE_T x)
{
  x--;
  x |= (x >> 1);  /* this sequence sets all bits at or below the topmost one to 1 */
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return ++x;     /* which ensures that this is a power of 2 */
}
