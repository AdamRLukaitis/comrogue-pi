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
#define __COMROGUE_PRESTART__
#include <comrogue/types.h>
#include <comrogue/internals/seg.h>
#include <comrogue/internals/llio.h>
#include <comrogue/internals/auxdev.h>
#include <comrogue/internals/gpio.h>
#include <comrogue/internals/16550.h>
#include <comrogue/internals/trace.h>

/* Hex digits. */
static DECLARE_INIT_STRING8_CONST(szHexDigits, "0123456789ABCDEF");

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
SEG_INIT_CODE void ETrInit(void)
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
SEG_INIT_CODE static void write_trace(UINT32 c)
{
  register UINT32 lsr = llIORead(AUX_MU_REG_LSR);
  while (!(lsr & U16550_LSR_TXEMPTY))
    lsr = llIORead(AUX_MU_REG_LSR);
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
SEG_INIT_CODE void ETrWriteChar8(CHAR c)
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
SEG_INIT_CODE void ETrWriteString8(PCSTR psz)
{
  while (*psz)
    ETrWriteChar8(*psz++);
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
SEG_INIT_CODE void ETrWriteWord(UINT32 uiValue)
{
  register UINT32 uiShift = 32;
  do
  {
    uiShift -= 4;
    write_trace(szHexDigits[(uiValue >> uiShift) & 0xF]);
  } while (uiShift > 0);
}

/*
 * Dumps the values of memory words beginning at a specified address.
 *
 * Parameters:
 * - puiWords = Pointer to words to be dumped.
 * - cWords = Number of words to be dumped.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Many characters written to UART1.
 */
SEG_INIT_CODE void ETrDumpWords(PUINT32 puiWords, UINT32 cWords)
{
  static DECLARE_INIT_STRING8_CONST(szSpacer1, ": ");
  register UINT32 i;
  for (i = 0; i < cWords; i++)
  {
    if ((i & 0x3) == 0)
    {
      ETrWriteWord((UINT32)(puiWords + i));
      ETrWriteString8(szSpacer1);
    }
    ETrWriteWord(puiWords[i]);
    if ((i & 0x3) == 0x3)
      ETrWriteChar8('\n');
    else
      ETrWriteChar8(' ');
  }
  if ((cWords & 0x3) != 0)
    ETrWriteChar8('\n');
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
SEG_INIT_CODE void ETrAssertFailed(PCSTR pszFile, INT32 nLine)
{
  static DECLARE_INIT_STRING8_CONST(szPrefix, "** ASSERTION FAILED: ");
  ETrWriteString8(szPrefix);
  ETrWriteString8(pszFile);
  write_trace(':');
  ETrWriteWord((UINT32)nLine);
  ETrWriteChar8('\n');
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
SEG_INIT_CODE void ETrInfiniBlink(void)
{
  register UINT32 ra;

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
