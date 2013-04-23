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
#ifndef __MEMMGR_H_INCLUDED
#define __MEMMGR_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

#ifndef __ASM__

#include <comrogue/types.h>
#include <comrogue/compiler_macros.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/rbtree.h>
#include <comrogue/internals/startup.h>

/*------------------------------------------
 * The COMROGUE memory management subsystem
 *------------------------------------------
 */

/* Nodes in the page table tree. */
typedef struct tagPAGENODE {
  RBTREENODE rbtn;           /* RBT node containing physical address as key */
  PPAGETAB ppt;              /* pointer to page table */
} PAGENODE, *PPAGENODE;

/* Virtual memory context. */
typedef struct tagVMCTXT {
  PTTB pTTB;                 /* pointer to the TTB */
  PTTBAUX pTTBAux;           /* pointer to the TTB auxiliary data */
  UINT32 uiMaxIndex;         /* max index into the above tables */
  RBTREE rbtPageTables;      /* tree containing page tables this context owns */
} VMCTXT, *PVMCTXT;

CDECL_BEGIN

/* Low-level maintenance functions */
extern void _MmFlushCacheForPage(KERNADDR vmaPage, BOOL bWriteback);
extern void _MmFlushCacheForSection(KERNADDR vmaSection, BOOL bWriteback);
extern void _MmFlushTLBForPage(KERNADDR vmaPage);
extern void _MmFlushTLBForPageAndContext(KERNADDR vmaPage, UINT32 uiASID);
extern void _MmFlushTLBForSection(KERNADDR vmaSection);
extern void _MmFlushTLBForSectionAndContext(KERNADDR vmaSection, UINT32 uiASID);
extern PTTB _MmGetTTB0(void);
extern void _MmSetTTB0(PTTB pTTB);

/* Kernel address space functions */
extern KERNADDR _MmAllocKernelAddr(UINT32 cpgNeeded);
extern void _MmFreeKernelAddr(KERNADDR kaBase, UINT32 cpgToFree);

/* Page mapping functions */
extern PHYSADDR MmGetPhysAddr(PVMCTXT pvmctxt, KERNADDR vma);
extern HRESULT MmDemapPages(PVMCTXT pvmctxt, KERNADDR vmaBase, UINT32 cpg);
extern HRESULT MmMapPages(PVMCTXT pvmctxt, PHYSADDR paBase, KERNADDR vmaBase, UINT32 cpg,
			  UINT32 uiTableFlags, UINT32 uiPageFlags, UINT32 uiAuxFlags);
extern HRESULT MmMapKernelPages(PHYSADDR paBase, UINT32 cpg, UINT32 uiTableFlags,
				UINT32 uiPageFlags, UINT32 uiAuxFlags, PKERNADDR pvmaLocation);
extern HRESULT MmDemapKernelPages(KERNADDR vmaBase, UINT32 cpg);

/* Initialization functions only */
extern void _MmInit(PSTARTUP_INFO pstartup);

CDECL_END

#endif /* __ASM__ */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __MEMMGR_H_INCLUDED */
