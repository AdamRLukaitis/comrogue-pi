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
#include <comrogue/internals/seg.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/startup.h>

/* Lists we keep track of various pages on. */
typedef struct tagPAGELIST {
  UINT32 ndxLast;                  /* index of last page in list */
  UINT32 cpg;                      /* count of pages in list */
} PAGELIST, *PPAGELIST;

/* The Master Page Database */
static PMPDB g_pMasterPageDB = NULL;

/* Individual page lists. */
//static PAGELIST g_pglFree = { 0, 0 };           /* pages that are free */
//static PAGELIST g_pglZeroed = { 0, 0 };         /* pages that are free and zeroed */
//static PAGELIST g_pglStandby = { 0, 0 };        /* pages removed but "in transition" */
//static PAGELIST g_pglModified = { 0, 0 };       /* pages removed but "in transition" and modified */
//static PAGELIST g_pglBad = { 0, 0 };            /* bad pages */



SEG_INIT_CODE void _MmInitPageAlloc(PSTARTUP_INFO pstartup)
{
  g_pMasterPageDB = (PMPDB)(pstartup->kaMPDB);
}
