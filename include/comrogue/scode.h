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
#ifndef __SCODE_H_INCLUDED
#define __SCODE_H_INCLUDED

#include <comrogue/types.h>

/*-------------------------------------------------------------------
 * Status codes are defined as follows:
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |S|        Facility             |            Code               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 9 8 7 6 5 4 3 2 1 0
 *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *
 * S = Severity: 0 = success, 1 = error
 * Facility = Facility code
 * Code = Specific error code
 *-------------------------------------------------------------------
 */

/* Severity codes */
#define SEVERITY_SUCCESS 0x00000000
#define SEVERITY_ERROR   0x80000000

/* Facility codes - compatible with M$ */
#define FACILITY_NULL       0
#define FACILITY_STORAGE    3
#define FACILITY_ITF        4
#define FACILITY_COMROGUE   7
#define FACILITY_STRFORMAT  0x333
#define FACILITY_MEMMGR     0x601

#ifndef __ASM__

#include <comrogue/types.h>

#define SUCCEEDED(s)  (((s) & SEVERITY_ERROR) == 0)
#define FAILED(s)     (((s) & SEVERITY_ERROR) != 0)

#define SCODE_FACILITY(s)  (((s) >> 16) & 0x7FFF)
#define SCODE_CODE(s)      ((s) & 0xFFFF)

#define MAKE_SCODE(sev, fac, err) ((SCODE)((sev) | (((fac) & 0x7FFF) << 16) | ((err) & 0xFFFF)))

#define SCODE_CAST(x) ((SCODE)(x))

#else

#define SCODE_CAST(x) x

#endif /* __ASM__ */

/* Basic success codes */
#define S_OK             SCODE_CAST(0x00000000)    /* OK return */
#define S_FALSE          SCODE_CAST(0x00000001)    /* "False" return */

/* Basic error codes */
#define E_NOTIMPL           SCODE_CAST(0x80000001)    /* not implemented */
#define E_OUTOFMEMORY       SCODE_CAST(0x80000002)    /* out of memory */
#define E_INVALIDARG        SCODE_CAST(0x80000003)    /* invalid argument */
#define E_NOINTERFACE       SCODE_CAST(0x80000004)    /* no such interface */
#define E_POINTER           SCODE_CAST(0x80000005)    /* invalid pointer */
#define E_HANDLE            SCODE_CAST(0x80000006)    /* invalid handle */
#define E_ABORT             SCODE_CAST(0x80000007)    /* aborted operation */
#define E_FAIL              SCODE_CAST(0x80000008)    /* unspecified failure */
#define E_ACCESSDENIED      SCODE_CAST(0x80000009)    /* access denied */
#define E_PENDING           SCODE_CAST(0x8000000A)    /* data not yet available */
#define E_UNEXPECTED        SCODE_CAST(0x8000FFFF)    /* unexpected error */

/* Memory manager error codes */
#define MEMMGR_E_NOPGTBL    SCODE_CAST(0x86010001)    /* no page tables available */
#define MEMMGR_E_BADTTBFLG  SCODE_CAST(0x86010002)    /* bad TTB flags encountered */
#define MEMMGR_E_COLLIDED   SCODE_CAST(0x86010003)    /* memory mapping collided */
#define MEMMGR_E_ENDTTB     SCODE_CAST(0x86010004)    /* tried to "walk off" end of TTB */
#define MEMMGR_E_NOSACRED   SCODE_CAST(0x86010005)    /* tried to demap a "sacred" entry */
#define MEMMGR_E_NOKERNSPC  SCODE_CAST(0x86010006)    /* no kernel space */
#define MEMMGR_E_RECURSED   SCODE_CAST(0x86010007)    /* tried to recurse into page allocation */
#define MEMMGR_E_BADTAGS    SCODE_CAST(0x86010008)    /* invalid tags for freed page */

#endif /* __SCODE_H_INCLUDED */
