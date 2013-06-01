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
#include <stdarg.h>
#include <comrogue/types.h>
#include <comrogue/str.h>
#include <comrogue/stream.h>
#include <comrogue/objhelp.h>
#include <comrogue/internals/seg.h>
#include <comrogue/internals/llio.h>
#include <comrogue/internals/auxdev.h>
#include <comrogue/internals/gpio.h>
#include <comrogue/internals/16550.h>

/* Hex digits. */
static DECLARE_STRING8_CONST(szHexDigits, "0123456789ABCDEF");

/*
 * Initializes the trace functionality (the aux UART).
 *
 * Parameters:
 * None.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * GPIO 14 configured for output from UART1; UART1 initialized to 115200 8N1 and enabled for output.
 */
void TrInit(void)
{
  register UINT32 ra;

  /* Initialize UART */
  llIOWrite(AUX_REG_ENABLE, AUX_ENABLE_MU); /* enable UART1 */
  llIOWrite(AUX_MU_REG_IER, 0);
  llIOWrite(AUX_MU_REG_CNTL, 0);
  llIOWrite(AUX_MU_REG_LCR, U16550_LCR_LENGTH_8|U16550_LCR_PARITY_NONE); /* 8 bits, no parity */
  llIOWrite(AUX_MU_REG_MCR, 0);
  llIOWrite(AUX_MU_REG_IER, 0);
  llIOWrite(AUX_MU_REG_FCR, U16550_FCR_RXCLEAR|U16550_FCR_TXCLEAR|U16550_FCR_LEVEL_14);
  llIOWrite(AUX_MU_REG_BAUD, 270);  /* 115200 baud */
  ra = llIORead(GPFSEL1_REG);
  ra &= ~GP_FUNC_MASK(4);              /* GPIO 14 - connects to pin 8 on GPIO connector */
  ra |= GP_FUNC_BITS(4, GP_PIN_ALT5);  /* Alt function 5 - UART1 TxD */
  llIOWrite(GPFSEL1_REG, ra);
  llIOWrite(GPPUD_REG, 0);
  llIODelay(150);
  llIOWrite(GPPUDCLK0_REG, GP_BIT(14));
  llIODelay(150);
  llIOWrite(GPPUDCLK0_REG, 0);
  llIOWrite(AUX_MU_REG_CNTL, AUXMU_CNTL_TXENABLE);
}

/*
 * Makes sure all trace output has been flushed out the UART.
 *
 * Parameters:
 * None.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * UART1 transmitter is empty upon return from this function.
 */
static void flush_trace(void)
{
  register UINT32 lsr = llIORead(AUX_MU_REG_LSR);
  while (!(lsr & U16550_LSR_TXEMPTY))
    lsr = llIORead(AUX_MU_REG_LSR);
}

/*
 * Writes a raw character to trace output.
 *
 * Parameters:
 * - c = The character to be written.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Character written to UART1.
 */
static void write_trace(UINT32 c)
{
  flush_trace();
  llIOWrite(AUX_MU_REG_THR, c);
}

/*
 * Writes a character to trace output, translating newlines.
 *
 * Parameters:
 * - c = The character to be written.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Either 1 or 2 characters written to UART1.
 */
void TrWriteChar8(CHAR c)
{
  if (c == '\n')
    write_trace('\r');
  write_trace((UINT32)c);
}

/*
 * Writes a null-terminated string to trace output.  Newlines in the string are translated.
 *
 * Parameters:
 * - psz = Pointer to string to be written.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Characters in string written to UART1.
 */
void TrWriteString8(PCSTR psz)
{
  while (*psz)
    TrWriteChar8(*psz++);
}

/*
 * Writes the value of a 32-bit word to trace output.
 *
 * Parameters:
 * - uiValue = Value to be written to trace output.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * 8 characters written to UART1.
 */
void TrWriteWord(UINT32 uiValue)
{
  register UINT32 uiShift = 32;
  do
  {
    uiShift -= 4;
    write_trace(szHexDigits[(uiValue >> uiShift) & 0xF]);
  } while (uiShift > 0);
}

/*
 * Used by the "trace printf" functions to actually output character data.
 *
 * Parameters:
 * - pDummy = Always points to a NULL value; not used.
 * - pchData = Pointer to the data to be written.
 * - cchData = The number of characters to be written.
 *
 * Returns:
 * S_OK in all circumstances.
 *
 * Side effects:
 * cchData characters written to UART1.
 */
static HRESULT write_trace_buf8(PPVOID pDummy, PCCHAR pchData, UINT32 cchData)
{
  register UINT32 i;
  for (i=0; i<cchData; i++)
    TrWriteChar8(pchData[i]);
  return S_OK;
}

/*
 * Formats output to the trace output.
 *
 * Parameters:
 * - pszFormat = Format string for the output, like a printf format string.
 * - pargs = Pointer to varargs-style function arguments.
 *
 * Returns:
 * A HRESULT as follows: Severity indicates whether the output function failed or succeeded, facility is always
 * FACILITY_STRFORMAT, code indicates the number of characters output.
 *
 * Side effects:
 * Characters written to UART1, the count of which is given by the output code.
 */
HRESULT TrVPrintf8(PCSTR pszFormat, va_list pargs)
{
  return StrFormatV8(write_trace_buf8, NULL, pszFormat, pargs);
}

/*
 * Formats output to the trace output.
 *
 * Parameters:
 * - pszFormat = Format string for the output, like a printf format string.
 * - <args> = Arguments to be substitute dinto the format string.
 *
 * Returns:
 * A HRESULT as follows: Severity indicates whether the output function failed or succeeded, facility is always
 * FACILITY_STRFORMAT, code indicates the number of characters output.
 *
 * Side effects:
 * Characters written to UART1, the count of which is given by the output code.
 */
HRESULT TrPrintf8(PCSTR pszFormat, ...)
{
  HRESULT hr;
  va_list argp;
  va_start(argp, pszFormat);
  hr = StrFormatV8(write_trace_buf8, NULL, pszFormat, argp);
  va_end(argp);
  return hr;
}

/*
 * Prints an "assertion failed" message to trace output.
 *
 * Parameters:
 * - pszFile = The file name the assertion was declared in.
 * - nLine = The line number the assertion was declared at.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Characters written to UART1.
 */
void TrAssertFailed(PCSTR pszFile, INT32 nLine)
{
  static DECLARE_STRING8_CONST(szMessage, "*** ASSERTION FAILED at %s:%d\n");
  TrPrintf8(szMessage, pszFile, nLine);
}

/*
 * Writes a buffer full of data (assumed 8-bit) to the trace output.
 *
 * Parameters:
 * - pThis = ISequentialStream output pointer (ignored).
 * - pv = Pointer to buffer to be written.
 * - cb = Number of bytes to be written.
 * - pcbWritten = If non-NULL, points to variable that will receive the number of bytes actually written.
 *
 * Returns:
 * Standard HRESULT success/failure indicator.
 */
static HRESULT traceStreamWrite(ISequentialStream *pThis, PCVOID pv, UINT32 cb, UINT32 *pcbWritten)
{
  register PCHAR p1, p2;   /* buffer pointers */

  if (!pv)
    return STG_E_INVALIDPOINTER;
  p1 = (PCHAR)pv;
  p2 = p1 + cb;
  while (p1 < p2)
    TrWriteChar8(*p1++);
  if (pcbWritten)
    *pcbWritten = cb;
  return S_OK;
}

/* VTable for an implementation of ISequentialStream that outputs to the trace output. */
static const SEG_RODATA struct ISequentialStreamVTable vtblTraceStream =
{
  .QueryInterface = ObjHlpStandardQueryInterface_ISequentialStream,
  .AddRef = ObjHlpStaticAddRefRelease,
  .Release = ObjHlpStaticAddRefRelease,
  .Read = (HRESULT (*)(ISequentialStream*, PVOID, UINT32, UINT32*))ObjHlpNotImplemented,
  .Write = traceStreamWrite
};

/* Implementation of ISequentialStream that outputs to the trace output. */
static const SEG_RODATA ISequentialStream traceStream = { &vtblTraceStream };

/*
 * Stores a pointer to the ISequentialStream implementation that outputs to the trace output.  When
 * done with the pointer, be sure to call Release() on it.
 *
 * Parameters:
 * - ppstm = Pointer to the variable to receive the ISequentialStream pointer.
 *
 * Returns:
 * Standard HRESULT success/failure indicator.
 */
HRESULT TrGetSequentialStream(ISequentialStream **ppstm)
{
  if (!ppstm)
    return E_POINTER;
  *ppstm = (ISequentialStream *)(&traceStream);
  IUnknown_AddRef(*ppstm);
  return S_OK;
}

/*
 * Puts the CPU into a loop where it blinks the green ACTIVITY light forever.
 *
 * Parameters:
 * None.
 *
 * Returns:
 * DOES NOT RETURN!
 *
 * Side effects:
 * GPIO 16 configured for output and turns on and off indefinitely.
 */
void TrInfiniBlink(void)
{
  register UINT32 ra;

  flush_trace();
  ra = llIORead(GPFSEL1_REG);
  ra &= GP_FUNC_MASK(6);    /* GPIO 16 - connects to green ACTIVITY LED */
  ra |= GP_FUNC_BITS(6, GP_PIN_OUTPUT);   /* output in all circumstances */
  llIOWrite(GPFSEL1_REG, ra);

  for(;;)
  {
    llIOWrite(GPSET0_REG, GP_BIT(16));
    llIODelay(0x100000);
    llIOWrite(GPCLR0_REG, GP_BIT(16));
    llIODelay(0x100000);
  }
}
