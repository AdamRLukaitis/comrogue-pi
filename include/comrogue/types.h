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
#ifndef __TYPES_H_INCLUDED
#define __TYPES_H_INCLUDED

/* Integral limit values */
#define INT16_MIN    0x8000
#define INT16_MAX    0x7FFF
#define UINT16_MIN   0
#define UINT16_MAX   0xFFFF
#define INT32_MIN    0x80000000
#define INT32_MAX    0x7FFFFFFF
#define UINT32_MIN   0
#define UINT32_MAX   0xFFFFFFFF
#define INT64_MIN    0x8000000000000000
#define INT64_MAX    0x7FFFFFFFFFFFFFFF
#define UINT64_MIN   0
#define UINT64_MAX   0xFFFFFFFFFFFFFFFF

#define INT_PTR_MIN  INT32_MIN
#define INT_PTR_MAX  INT32_MAX
#define UINT_PTR_MIN UINT32_MIN
#define UINT_PTR_MAX UINT32_MAX

/* Number of bits */
#define INT16_BITS  16
#define UINT16_BITS 16
#define INT32_BITS  32
#define UINT32_BITS 32
#define INT64_BITS  64
#define UINT64_BITS 64

#define LOG_PTRSIZE     2   /* log2(sizeof(void *)) */
#define LOG_INTSIZE     2   /* log2(sizeof(int)) */
#define LOG_UINTSIZE    2   /* log2(sizeof(UINT32)) */
#define LOG_INT64SIZE   3   /* log2(sizeof(long long)) */

/* Boolean values */
#define TRUE  1
#define FALSE 0

/* NULL value */
#ifndef NULL
#define NULL 0
#endif

#ifndef __ASM__

/* The basic types referenced by object_types.h. */
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned short wchar_t;

#include <comrogue/object_types.h>

#define MAKEBOOL(val) ((val) ? TRUE : FALSE)

#define OFFSETOF(struc, field)   ((UINT_PTR)(&(((struc *)0)->field)))

#ifdef __COMROGUE_INTERNALS__

/* Internal system types */
typedef UINT_PTR PHYSADDR;      /* physical address */
typedef UINT_PTR KERNADDR;      /* kernel address */
typedef PHYSADDR *PPHYSADDR;
typedef KERNADDR *PKERNADDR;

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __ASM__ */

#endif /* __TYPES_H_INCLUDED */
