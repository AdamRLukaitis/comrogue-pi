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
#define S_OK                         SCODE_CAST(0x00000000)    /* OK return */
#define S_FALSE                      SCODE_CAST(0x00000001)    /* "False" return */

/* Storage success codes */
#define STG_S_CONVERTED              SCODE_CAST(0x00030200)    /* storage converted transparently */
#define STG_S_BLOCK                  SCODE_CAST(0x00030201)    /* block until we get more data */
#define STG_S_RETRYNOW               SCODE_CAST(0x00030202)    /* retry operation immediately */
#define STG_S_MONITORING             SCODE_CAST(0x00030203)    /* notified event sink won't affect storage */
#define STG_S_MULTIPLEOPENS          SCODE_CAST(0x00030204)    /* multiple opens prevent consolidation */
#define STG_S_CONSOLIDATIONFAILED    SCODE_CAST(0x00030205)    /* consolidation failed but commit OK */
#define STG_S_CANNOTCONSOLIDATE      SCODE_CAST(0x00030206)    /* cannot consolidate but commit OK */

/* Basic error codes */
#define E_NOTIMPL                    SCODE_CAST(0x80000001)    /* not implemented */
#define E_OUTOFMEMORY                SCODE_CAST(0x80000002)    /* out of memory */
#define E_INVALIDARG                 SCODE_CAST(0x80000003)    /* invalid argument */
#define E_NOINTERFACE                SCODE_CAST(0x80000004)    /* no such interface */
#define E_POINTER                    SCODE_CAST(0x80000005)    /* invalid pointer */
#define E_HANDLE                     SCODE_CAST(0x80000006)    /* invalid handle */
#define E_ABORT                      SCODE_CAST(0x80000007)    /* aborted operation */
#define E_FAIL                       SCODE_CAST(0x80000008)    /* unspecified failure */
#define E_ACCESSDENIED               SCODE_CAST(0x80000009)    /* access denied */
#define E_PENDING                    SCODE_CAST(0x8000000A)    /* data not yet available */
#define E_UNEXPECTED                 SCODE_CAST(0x8000FFFF)    /* unexpected error */

/* Storage error codes */
#define STG_E_INVALIDFUNCTION        SCODE_CAST(0x80030001)    /* invalid function */
#define STG_E_FILENOTFOUND           SCODE_CAST(0x80030002)    /* file not found */
#define STG_E_PATHNOTFOUND           SCODE_CAST(0x80030003)    /* path not found */
#define STG_E_TOOMANYOPENFILES       SCODE_CAST(0x80030004)    /* too many open files */
#define STG_E_ACCESSDENIED           SCODE_CAST(0x80030005)    /* access denied */
#define STG_E_INVALIDHANDLE          SCODE_CAST(0x80030006)    /* invalid handle */
#define STG_E_INSUFFICIENTMEMORY     SCODE_CAST(0x80030008)    /* insufficient memory */
#define STG_E_INVALIDPOINTER         SCODE_CAST(0x80030009)    /* invalid pointer */
#define STG_E_NOMOREFILES            SCODE_CAST(0x80030012)    /* no more files to return */
#define STG_E_DISKISWRITEPROTECTED   SCODE_CAST(0x80030013)    /* disk is write protected */
#define STG_E_SEEKERROR              SCODE_CAST(0x80030019)    /* error in seek operation */
#define STG_E_WRITEFAULT             SCODE_CAST(0x8003001D)    /* disk error during write */
#define STG_E_READFAULT              SCODE_CAST(0x8003001E)    /* disk error during read */
#define STG_E_SHAREVIOLATION         SCODE_CAST(0x80030020)    /* sharing violation */
#define STG_E_LOCKVIOLATION          SCODE_CAST(0x80030021)    /* lock violation */
#define STG_E_FILEALREADYEXISTS      SCODE_CAST(0x80030050)    /* file already exists */
#define STG_E_INVALIDPARAMETER       SCODE_CAST(0x80030057)    /* invalid parameter */
#define STG_E_MEDIUMFULL             SCODE_CAST(0x80030070)    /* out of disk space */
#define STG_E_PROPSETMISMATCHED      SCODE_CAST(0x800300F0)    /* illegal write of property to property set */
#define STG_E_ABNORMALAPIEXIT        SCODE_CAST(0x800300FA)    /* API call exited abnormally */
#define STG_E_INVALIDHEADER          SCODE_CAST(0x800300FB)    /* invalid file header */
#define STG_E_INVALIDNAME            SCODE_CAST(0x800300FC)    /* invalid name */
#define STG_E_UNKNOWN                SCODE_CAST(0x800300FD)    /* unknown/unexpected error */
#define STG_E_UNIMPLEMENTEDFUNCTION  SCODE_CAST(0x800300FE)    /* unimplemented function */
#define STG_E_INVALIDFLAG            SCODE_CAST(0x800300FF)    /* invalid flag */
#define STG_E_INUSE                  SCODE_CAST(0x80030100)    /* object is busy */
#define STG_E_NOTCURRENT             SCODE_CAST(0x80030101)    /* not current */
#define STG_E_REVERTED               SCODE_CAST(0x80030102)    /* object reverted and no longer exists */
#define STG_E_CANTSAVE               SCODE_CAST(0x80030103)    /* can't save */
#define STG_E_OLDFORMAT              SCODE_CAST(0x80030104)    /* older file format */
#define STG_E_OLDDLL                 SCODE_CAST(0x80030105)    /* newer file format */
#define STG_E_SHAREREQUIRED          SCODE_CAST(0x80030106)    /* file sharing required */
#define STG_E_NOTFILEBASEDSTORAGE    SCODE_CAST(0x80030107)    /* illegal operation on storage */
#define STG_E_EXTANTMARSHALLINGS     SCODE_CAST(0x80030108)    /* illegal operation on extant marshallings */
#define STG_E_DOCFILECORRUPT         SCODE_CAST(0x80030109)    /* docfile corrupt */
#define STG_E_BADBASEADDRESS         SCODE_CAST(0x80030110)    /* invalid base address */
#define STG_E_DOCFILETOOLARGE        SCODE_CAST(0x80030111)    /* docfile too large */
#define STG_E_NOTSIMPLEFORMAT        SCODE_CAST(0x80030112)    /* docfile not a simple format */
#define STG_E_INCOMPLETE             SCODE_CAST(0x80030201)    /* file incomplete */
#define STG_E_TERMINATED             SCODE_CAST(0x80030202)    /* download terminated */
#define STG_E_COPYPROTECTFAIL        SCODE_CAST(0x80030305)    /* copy protection failed */
#define STG_E_CSSAUTHFAIL            SCODE_CAST(0x80030306)    /* CSS authentication failed */
#define STG_E_CSSKEYNOTPRESENT       SCODE_CAST(0x80030307)    /* CSS key not present */
#define STG_E_CSSKEYNOTESTABLISHED   SCODE_CAST(0x80030308)    /* CSS key not established */
#define STG_E_CSSSCRAMBLED           SCODE_CAST(0x80030309)    /* encrypted sector */
#define STG_E_CSSINVALIDREGION       SCODE_CAST(0x8003030A)    /* region identifier mismatch */
#define STG_E_NOMOREREGIONRESETS     SCODE_CAST(0x8003030B)    /* can't reset drive region anymore */

/* Memory manager error codes */
#define MEMMGR_E_NOPGTBL             SCODE_CAST(0x86010001)    /* no page tables available */
#define MEMMGR_E_BADTTBFLG           SCODE_CAST(0x86010002)    /* bad TTB flags encountered */
#define MEMMGR_E_COLLIDED            SCODE_CAST(0x86010003)    /* memory mapping collided */
#define MEMMGR_E_ENDTTB              SCODE_CAST(0x86010004)    /* tried to "walk off" end of TTB */
#define MEMMGR_E_NOSACRED            SCODE_CAST(0x86010005)    /* tried to demap a "sacred" entry */
#define MEMMGR_E_NOKERNSPC           SCODE_CAST(0x86010006)    /* no kernel space */
#define MEMMGR_E_RECURSED            SCODE_CAST(0x86010007)    /* tried to recurse into page allocation */
#define MEMMGR_E_BADTAGS             SCODE_CAST(0x86010008)    /* invalid tags for freed page */

#endif /* __SCODE_H_INCLUDED */
