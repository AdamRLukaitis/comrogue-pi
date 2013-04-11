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
#include <comrogue/internals/layout.h>
#include <comrogue/internals/sctlr.h>
	
.section ".initHEAD.text"

.globl	COMROGUEStart

/* entry: r0 = physical (low-memory) address of STARTUP_INFO1 structure */
COMROGUEStart:
	/* initialize the stack pointer */
	ldr sp, =.initstack

	/* clear init bss */
	ldr r4, =vmaInitBss
	ldr r5, =vmaFirstFree
	mov r6, #0
	mov r7, #0
	mov r8, #0
	mov r9, #0
	bl .erase

	/* clear resident bss */
	ldr r4, =vmaKernelBss
	ldr r5, =vmaInitCode
	bl .erase

	/* initialize trace */
	push {r0}
	bl TrInit
#if 0	
	ldr r0, =.msg0
	bl TrWriteString8
#endif
	pop {r0}

	/* copy startup info to our local buffer */
	mov r1, r0
	ldr r0, =kiStartupInfo
	mov r11, r0                   /* save for later */
	ldr r2, [r1]                  /* get number of bytes to copy */
	ldr ip, =StrCopyMem	      /* this is needed to branch to a library function */
	ldr lr, =.postCopy
	mov pc, ip

.postCopy:
#if 0
	ldr r0, =.msg1
	bl TrWriteString8
#endif

	mcr p15, 0, r6, c7, c7, 0     /* clear caches */
	mcr p15, 0, r6, c8, c7, 0     /* clear TLB */
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, # SCTLR_I         /* activate instruction cache */
	orr r0, r0, # SCTLR_C         /* activate data cache */
	mcr p15, 0, r0, c1, c0, 0

	/* transfer to C code to continue initialization */
	ldr ip, =KiSystemStartup
	ldr lr, =.hang
	mov pc, ip		      /* transfer to C code */
.hang:
	wfe			      /* if KiSystemStartup returns, just hang */
	b .hang

	/* Simple erase routine. Start and end addresses in r4 and r5. r6-r9 assumed to be 0. */
	/* Do not touch r0-r2. */
.erase:
	cmp r4, r5                    /* are we done? */
	movhs pc, lr                  /* yes, return */
	stm r4!, {r6-r9}              /* stomp 16 bytes */
	b .erase

#if 0
	.balign 4
.msg0:
	.asciz "got here 2\n"
	.balign 4
.msg1:
	.asciz "got here 3\n"
#endif

.section ".init.bss"
	.balign 4
	.space 4096
.initstack:
