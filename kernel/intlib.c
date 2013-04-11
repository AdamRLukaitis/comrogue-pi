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
#include <comrogue/internals/seg.h>
#include "quad.h"

/*---------------------------
 * Integer library functions
 *---------------------------
 */

/* Flags used internally to IntLDiv. */
#define FLIP_QUOTIENT  0x00000001
#define FLIP_REMAINDER 0x00000002

/*
 * Divides two 64-bit integers and returns a quotient and a remainder in a structure.
 *
 * Parameters:
 * - pResult = Pointer to an LDIV structure to contain the quotient and remainder.
 * - num = The numerator (dividend) for the division.
 * - demom = The demoninator (divisor) for the division.
 *
 * Returns:
 * Standard SUCCEEDED/FAILED HRESULT.
 */
SEG_LIB_CODE HRESULT IntLDiv(PLDIV pResult, INT64 num, INT64 denom)
{
  UINT32 mode = 0;
  if (denom == 0)
  {
    pResult->quot = pResult->rem = 0;
    return E_INVALIDARG;
  }
  if (denom < 0)
  {
    mode ^= FLIP_QUOTIENT;
    denom = -denom;
  }
  if (num < 0)
  {
    mode ^= (FLIP_QUOTIENT|FLIP_REMAINDER);
    num = -num;
  }
  pResult->quot = __qdivrem(num, denom, (PUINT64)(&(pResult->rem)));
  if (mode & FLIP_QUOTIENT)
    pResult->quot = -(pResult->quot);
  if (mode & FLIP_REMAINDER)
    pResult->rem = -(pResult->rem);
  return S_OK;
}
