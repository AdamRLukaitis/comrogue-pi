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
#ifndef __INITFUNCS_H_INCLUDED
#define __INITFUNCS_H_INCLUDED

#ifndef __ASM__

#include <comrogue/allocator.h>
#include <comrogue/internals/startup.h>

/* Pointer to a function to update the page database with a PTE address. */
typedef void (*PFNSETPTEADDR)(UINT32, PHYSADDR, BOOL);

/*------------------------------------
 * Prototypes for the init functions.
 *------------------------------------
 */

extern IMalloc *_MmGetInitHeap(void);
extern void _MmInitKernelSpace(PSTARTUP_INFO pstartup, PMALLOC pmInitHeap);
extern void _MmInitVMMap(PSTARTUP_INFO pstartup, PMALLOC pmInitHeap);
extern void _MmInitPageAlloc(PSTARTUP_INFO pstartup);

/* secondary init function in the VM mapper */
extern void _MmInitPTEMappings(PFNSETPTEADDR pfnSetPTEAddr);

#endif /* __ASM__ */

#endif /* __INITFUNCS_H_INCLUDED */
