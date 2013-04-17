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
#ifndef __SEG_H_INCLUDED
#define __SEG_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

#ifndef __ASM__

#include <comrogue/types.h>

/*----------------------
 * Segment declarations
 *----------------------
 */

#ifdef __COMROGUE_PRESTART__

#define SEG_INIT_CODE    __attribute__((__section__(".prestart.text")))
#define SEG_INIT_DATA    __attribute__((__section__(".prestart.data")))
#define SEG_INIT_RODATA  __attribute__((__section__(".prestart.rodata")))
#define SEG_RODATA       SEG_INIT_RODATA
#define SEG_LIB_CODE     SEG_INIT_CODE
#define SEG_LIB_RODATA   SEG_INIT_RODATA

#else

#define SEG_INIT_CODE    __attribute__((__section__(".init.text")))
#define SEG_INIT_DATA    __attribute__((__section__(".init.data")))
#define SEG_INIT_RODATA  __attribute__((__section__(".init.rodata")))
#define SEG_RODATA       __attribute__((__section__(".rodata")))
#define SEG_LIB_CODE     __attribute__((__section__(".lib.text")))
#define SEG_LIB_RODATA   __attribute__((__section__(".lib.rodata")))

#endif  /* __COMROGUE_PRESTART__ */

/*------------------------------------
 * String constant declaration macros
 *------------------------------------
 */

#define DECLARE_STRING8_CONST_STGCLASS(name, value, stgclass) const CHAR stgclass name [] = value
#define DECLARE_INIT_STRING8_CONST(name, value) DECLARE_STRING8_CONST_STGCLASS(name, value, SEG_INIT_RODATA)
#define DECLARE_LIB_STRING8_CONST(name, value) DECLARE_STRING8_CONST_STGCLASS(name, value, SEG_LIB_RODATA)
#define DECLARE_STRING8_CONST(name, value) DECLARE_STRING8_CONST_STGCLASS(name, value, SEG_RODATA)

#endif  /* __ASM__ */

#endif  /* __COMROGUE_INTERNALS__ */

#endif  /* __SEG_H_INCLUDED */
