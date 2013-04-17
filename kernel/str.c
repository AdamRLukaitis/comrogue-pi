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
#define __COMROGUE_KERNEL_LIB__
#include <comrogue/types.h>
#include <comrogue/scode.h>
#include <comrogue/intlib.h>
#include <comrogue/str.h>
#include <comrogue/internals/seg.h>

/*-------------------------
 * Basic string operations
 *-------------------------
 */

/*
 * Returns whether or not the character is a digit (0-9).
 *
 * Parameters:
 * - ch = Character to be tested.
 *
 * Returns:
 * TRUE if the character is a digit, FALSE otherwise.
 */
SEG_LIB_CODE BOOL StrIsDigit8(CHAR ch)
{
  /* TODO: replace with something nicer later */
  return MAKEBOOL((ch >= '0') && (ch <= '9'));
}

/*
 * Returns the length of a null-terminated string.
 *
 * Parameters:
 * - psz = The string to get the length of.
 *
 * Returns:
 * The length of the string in characters.
 */
SEG_LIB_CODE INT32 StrLength8(PCSTR psz)
{
  register PCSTR p = psz;
  while (*p)
    p++;
  return (INT32)(p - psz);
}

/*
 * Locates a character within a null-terminated string.
 *
 * Parameters:
 * - psz = String to search through for character.
 * - ch = Character to look for.
 *
 * Returns:
 * NULL if character was not found, otherwise pointer to character within string.
 */
SEG_LIB_CODE PCHAR StrChar8(PCSTR psz, INT32 ch)
{
  const CHAR mych = ch;
  for (; *psz; psz++)
    if (*psz == mych)
      return (PCHAR)psz;
  return NULL;
}

/*-------------------------------
 * Numeric-to-string conversions
 *-------------------------------
 */

/*
 * Formats an unsigned numeric value into a buffer.  This function formats the value into the END of the
 * buffer and returns a pointer to the first character of the number.  Note that the string-converted number
 * is NOT null-terminated!
 *
 * Parameters:
 * - value = The value to be converted.
 * - pchBuf = Pointer to the buffer in which to store the converted value.
 * - nBytes = Number of characters in the buffer.
 * - nBase = The numeric base to use to output the number.  Valid values are in the interval [2,36].
 * - fUppercase = TRUE to use upper-case letters as digits in output, FALSE to use lower-case letters.
 *
 * Returns:
 * A pointer to the first digit of the converted value, which will be somewhere in the interval
 * [pchBuf, pchBuf + nBytes).
 */
SEG_LIB_CODE static PCHAR convert_number_engine8(UINT64 value, PCHAR pchBuf, INT32 nBytes, INT32 nBase,
						 BOOL fUppercase)
{
  static DECLARE_LIB_STRING8_CONST(szDigitsUppercase, "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  static DECLARE_LIB_STRING8_CONST(szDigitsLowercase, "0123456789abcdefghijklmnopqrstuvwxyz");
  PCSTR pszDigits = (fUppercase ? szDigitsUppercase : szDigitsLowercase);  /* digit set to use */
  PCHAR p = pchBuf + nBytes;     /* output pointer */
  INT64 accumulator;             /* accumulator of quotients */
  LDIV ldiv;                     /* buffer for division */

  /* do initial division to get rid of high-order bit & handle 0 */
  *--p = pszDigits[value % nBase];
  accumulator = (INT64)(value / nBase);

  while ((p > pchBuf) && (accumulator > 0))
  { /* output digits */
    IntLDiv(&ldiv, accumulator, nBase);
    *--p = pszDigits[ldiv.rem];
    accumulator = ldiv.quot;
  }
  return p;
}

/*-----------------------------------------
 * StrFormatV8 and its support definitions
 *-----------------------------------------
 */

/* Flag values for conversion characters */
#define FLAG_LEFTJUST  0x01        /* left-justify output field */
#define FLAG_PLUSSIGN  0x02        /* use a '+' for the sign if >= 0 */
#define FLAG_SPACESIGN 0x04        /* use a space for the sign if >= 0 */
#define FLAG_ALTMODE   0x08        /* alternate conversion mode */
#define FLAG_ZEROPAD   0x10        /* zero-pad to width */

/* Macro that calls the output function and either updates total output or exits */
#define do_output(pBuf, nChr) \
  do { if ((nChr) > 0) { if (SUCCEEDED((*pfnFormat)(&pArg, (pBuf), (nChr)))) nTotalOutput += (nChr); \
	 else { errorFlag = SEVERITY_ERROR; goto error; } } } while (0)

/* Strings used for padding */
static DECLARE_LIB_STRING8_CONST(szPadSpace, "                                                          ");
static DECLARE_LIB_STRING8_CONST(szPadZero,  "0000000000000000000000000000000000000000000000000000000000");

#define PAD_SIZE (sizeof(szPadSpace) - 1)    /* number of characters available in pad string */

/* Macro that outputs characters for padding */
#define do_pad_output(szPad, nChr) \
  do { int n, nt = (nChr); while (nt > 0) { n = intMin(nt, PAD_SIZE); do_output(szPad, n); nt -= n; } } while (0)

#define MAX_VAL 509             /* maximum field value for conversion */
#define BUFSIZE 64              /* buffer size on stack for conversion */
#define NUM_CONVERT_BYTES 32    /* maximum number of bytes for converting a numeric value */

/*
 * Format a series of arguments like "printf," calling a function to perform the actual character output.
 *
 * Parameters:
 * - pfnFormat = Pointer to the format function
 * - pArg = Pointer argument passed to the format function, which may modify it
 * - pszFormat = Format string, like a printf format string.
 * - pargs = Pointer to varargs-style function arguments.
 *
 * Returns:
 * A HRESULT as follows: Severity indicates whether the output function failed or succeeded, facility is always
 * FACILITY_STRFORMAT, code indicates the number of characters output.
 *
 * Notes:
 * Floating-point conversion formats are not supported at this time.
 */
SEG_LIB_CODE HRESULT StrFormatV8(PFNFORMAT8 pfnFormat, PVOID pArg, PCSTR pszFormat, va_list pargs)
{
  static DECLARE_LIB_STRING8_CONST(szFlagChars, "0+- #");   /* flag characters and values */
  static const UINT32 SEG_LIB_RODATA auiFlagValues[] =
      { FLAG_ZEROPAD, FLAG_LEFTJUST, FLAG_PLUSSIGN, FLAG_SPACESIGN, FLAG_ALTMODE, 0 };
  static DECLARE_LIB_STRING8_CONST(szConversionChars, "hl");  /* conversion characters */
  PCSTR p;                               /* pointer used to walk the format string */
  PCHAR pFlag;                           /* pointer to flag character/temp buffer pointer */
  PCHAR pchOutput;                       /* pointer to characters actually output */
  INT32 nchOutput;                       /* number of characters output */
  PCHAR pchPrefix;                       /* pointer to prefix characters */
  INT32 nchPrefix;                       /* number of prefix characters */
  INT32 nchZeroPrefix;                   /* number of zeroes to write as a prefix */
  INT32 nTotalOutput = 0;                /* total number of characters output */
  UINT32 errorFlag = SEVERITY_SUCCESS;   /* error flag to return */
  UINT32 flags;                          /* conversion flags seen */
  INT32 ncWidth;                         /* width for field */
  INT32 ncPrecision;                     /* precision for field */
  INT32 nBase;                           /* numeric base for field */
  INT64 input;                           /* input integer value */
  CHAR chConvert;                        /* conversion character for field */
  CHAR achBuffer[BUFSIZE];               /* buffer into which output is formatted */

  while (*pszFormat)
  {
    /* locate all plain characters up to next % or end of string and output them */
    p = pszFormat;
    while (*p && (*p != '%'))
      p++;
    do_output(pszFormat, p - pszFormat);

    if (*p == '%')
    {
      flags = 0;  /* convert flags first */
      for (p++; (pFlag = StrChar8(szFlagChars, *p)) != NULL; p++)
	flags |= auiFlagValues[pFlag - szFlagChars];

      ncWidth = 0;  /* now convert width */
      if (*p == '*')
      {
	ncWidth = va_arg(pargs, INT32);  /* specified by arg */
	p++;
	if (ncWidth < 0)
	{
	  flags |= FLAG_LEFTJUST;  /* negative width is taken as use of - flag */
	  ncWidth = -ncWidth;
	}
      }
      else
      {
	while (StrIsDigit8(*p))
	  ncWidth = ncWidth * 10 + (*p++ - '0');
      }
      if (ncWidth > MAX_VAL)
	ncWidth = MAX_VAL;

      ncPrecision = -1;   /* now convert precision */
      if (*p == '.')
      {
	p++;
	if (*p == '*')
	{
	  ncPrecision = va_arg(pargs, INT32);  /* specified by arg */
	  p++;
	  /* N.B. ncPrecision < 0 = "omitted precision" */
	}
	else
	{
	  ncPrecision = 0;
	  while (StrIsDigit8(*p))
	    ncPrecision = ncPrecision * 10 + (*p++ - '0');
	}
	if (ncPrecision > MAX_VAL)
	  ncPrecision = MAX_VAL;
      }

      chConvert = StrChar8(szConversionChars, *p) ? *p++ : '\0'; /* get conversion character */

      /* based on field character, convert the field */
      pchOutput = achBuffer;
      nchOutput = 0;
      pchPrefix = NULL;
      nchPrefix = 0;
      nchZeroPrefix = 0;
      switch (*p)
      {
	case 'c':              /* output a character */
	  achBuffer[nchOutput++] = (CHAR)va_arg(pargs, INT32);
	  break;

	case 'd':
	case 'i':              /* output a decimal number */
	  if (ncPrecision < 0)
	    ncPrecision = 1;   /* default precision is 1 */
	  if (chConvert == 'h')
	    input = va_arg(pargs, INT32);
	  else if (chConvert == '\0')
	    input = va_arg(pargs, INT32);
	  else
	    input = va_arg(pargs, INT64);
	  if ((input == 0) && (ncPrecision == 0))
	    break;  /* no output */

	  if (input < 0)
	  {
	    achBuffer[nchPrefix++] = '-';  /* minus sign */
	    input = -input;
	  }
	  else
	  {
	    /* plus or space for sign if flags dictate */
	    if (flags & FLAG_PLUSSIGN)
	      achBuffer[nchPrefix++] = '+';
	    else if (flags & FLAG_SPACESIGN)
	      achBuffer[nchPrefix++] = ' ';
	  }
	  if (nchPrefix > 0)
	  {
	    pchPrefix = achBuffer;  /* adjust output pointer and set prefix */
	    pchOutput = achBuffer + nchPrefix;
	  }

	  /* clamp input value and run it through number conversion engine */
	  if (chConvert == 'h')
	    input &= UINT16_MAX;
	  else if (chConvert == '\0')
	    input &= UINT32_MAX;
	  pFlag = pchOutput;
	  pchOutput = convert_number_engine8((UINT64)input, pFlag, NUM_CONVERT_BYTES, 10, FALSE);
	  nchOutput = (pFlag + NUM_CONVERT_BYTES) - pchOutput;

	  /* calculate zero prefix based on precision */
	  if ((flags & (FLAG_ZEROPAD|FLAG_LEFTJUST)) == FLAG_ZEROPAD)
	    nchZeroPrefix = intMax(0, ncWidth - (nchPrefix + nchOutput));
	  else if (ncPrecision > 0)
	    nchZeroPrefix = intMax(0, ncPrecision - nchOutput);
	  break;

	case 'o':
	case 'u':
	case 'x':
	case 'X':              /* output an unsigned number */
	  if (ncPrecision < 0)
	    ncPrecision = 1;   /* defualt precision is 1 */
	  if (chConvert == 'h')
	    input = va_arg(pargs, UINT32);
	  else if (chConvert == '\0')
	    input = va_arg(pargs, UINT32);
	  else
	    input = (INT64)va_arg(pargs, UINT64);
	  if ((input == 0) && (ncPrecision == 0))
	    break;  /* no output */

	  /* select numeric base of output */
	  if (*p == 'o')
	    nBase = 8;
	  else if (*p == 'u')
	    nBase = 10;
	  else
	    nBase = 16;

	  if (flags & FLAG_ALTMODE)
	  {
	    if (*p == 'o')
	    { /* forced octal notation */
	      achBuffer[nchPrefix++] = '0';
	      ncPrecision--;
	    }
	    else if (*p != 'u')
	    { /* forced hexadecimal notation */
	      achBuffer[nchPrefix++] = '0';
	      achBuffer[nchPrefix++] = *p;
	    }
	  }
	  if (nchPrefix > 0)
	  {
	    pchPrefix = achBuffer;  /* adjust output pointer and set prefix */
	    pchOutput = achBuffer + nchPrefix;
	  }

	  /* clamp input value and run it through number conversion engine */
	  if (chConvert == 'h')
	    input &= UINT16_MAX;
	  else if (chConvert == '\0')
	    input &= UINT32_MAX;
	  pFlag = pchOutput;
	  pchOutput = convert_number_engine8((UINT64)input, pFlag, NUM_CONVERT_BYTES, nBase, MAKEBOOL(*p == 'X'));
	  nchOutput = (pFlag + NUM_CONVERT_BYTES) - pchOutput;

	  /* calculate zero prefix based on precision */
	  if ((flags & (FLAG_ZEROPAD|FLAG_LEFTJUST)) == FLAG_ZEROPAD)
	    nchZeroPrefix = intMax(0, ncWidth - (nchPrefix + nchOutput));
	  else if (ncPrecision > 0)
	    nchZeroPrefix = intMax(0, ncPrecision - nchOutput);
	  break;

	case 'p':              /* output a pointer value - treated as unsigned hex with precision of 8 */
	  input = (INT_PTR)va_arg(pargs, PVOID);
	  input &= UINT_PTR_MAX;
	  pFlag = pchOutput;
	  pchOutput = convert_number_engine8((UINT64)input, pFlag, NUM_CONVERT_BYTES, 16, FALSE);
	  nchOutput = (pFlag + NUM_CONVERT_BYTES) - pchOutput;
	  nchZeroPrefix = intMax(0, (sizeof(UINT_PTR) / sizeof(CHAR) * 2) - nchOutput);
	  break;

	case 's':              /* output a string */
	  pchOutput = (PCHAR)va_arg(pargs, PCSTR);
	  nchOutput = StrLength8(pchOutput);
	  if ((ncPrecision >= 0) && (ncPrecision < nchOutput))
	    nchOutput = ncPrecision;
	  break;

	case 'n':              /* output nothing, just store number of characters output so far */
	  *(va_arg(pargs, PINT32)) = nTotalOutput;
	  goto no_output;

	case '%':              /* output a single % sign */
	default:               /* unrecognized format character, output it and continue */
	  achBuffer[nchOutput++] = *p;
	  break;
      }

      ncWidth -= (nchPrefix + nchZeroPrefix + nchOutput);
      if (!(flags & FLAG_LEFTJUST))
	do_pad_output(szPadSpace, ncWidth);
      do_output(pchPrefix, nchPrefix);
      do_pad_output(szPadZero, nchZeroPrefix);
      do_output(pchOutput, nchOutput);
      if (flags & FLAG_LEFTJUST)
	do_pad_output(szPadSpace, ncWidth);

no_output:
      p++;
    }
    pszFormat = p;
  }
error:
  return MAKE_SCODE(errorFlag, FACILITY_STRFORMAT, nTotalOutput);
}
