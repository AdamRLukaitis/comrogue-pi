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
#define __COMROGUE_PRESTART__
#include <comrogue/internals/asm-macros.h>
#include <comrogue/internals/layout.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/sctlr.h>

/*------------------------------------------------------------------------------------------
 * The prestart code that gets control when the kernel is first loaded; its objective is to
 * set up the MMU and hand over control to the actual start code.
 *------------------------------------------------------------------------------------------
 */
	
.section ".first.prestart.text"

.globl	COMROGUEPrestart

/* On entry: r0 = 0, r1 = machine type, r2 = atags address */
COMROGUEPrestart:
	/* Initialize a temporary stack area. */
	mov ip, # PHYSADDR_LOAD
	sub sp, ip, #4

	/* Go collect the startup info. */
	bl KiCollectStartupInfo
	mov r11, r0			/* r11 = address of startup info structure */
	/* at this point r0-r2 are free for other code to use */

	/* Early trace initialize; we reinitialize it later. */
	bl ETrInit
	ldr r0, =.initMessage
	bl ETrWriteString8

	/* Copy the early vector table into place, including the vector words that live after the table itself. */
	ldr r0, =.earlyVectorTable
	ldr r1, =.earlyVectorTableEnd
	mov r2, #0
	b .vec1
.vec0:
	ldm r0!, {r3-r6}                /* copy 16 bytes at a time */
	stm r2!, {r3-r6}
.vec1:
	cmp r0, r1
	bne .vec0

	/* Initialize early memory management (the TTB1). */
	mov r0, r11
	bl EMmInit
	mov r10, r0                     /* r10 = address of TTB */
	
	/* Set up and start the MMU. */
	mov ip, #0
	mcr p15, 0, ip, c7, c7, 0 	/* clear caches */
	mcr p15, 0, ip, c8, c7, 0	/* clear TLB */
	mov ip, #1
	mcr p15, 0, ip, c2, c0, 2       /* set TTBCR */
	mvn ip, #0
	mcr p15, 0, ip, c3, c0, 0	/* configure domain 0 = client, all others = invalid */
	mcr p15, 0, r10, c2, c0, 0	/* set TTB0 */
	mcr p15, 0, r10, c2, c0, 1	/* set TTB1 */

	mrc p15, 0, ip, c1, c0, 0	/* get control register 1 */
	orr ip, ip, # SCTLR_M		/* MMU = on */
	orr ip, ip, # SCTLR_XP		/* subpage AP bits disabled in 2nd-level page tables */

	instr_barrier
	mcr p15, 0, ip, c1, c0, 0	/* store control register 1 */
	mrc p15, 0, ip, c0, c0, 0       /* read ID register */
	instr_barrier

#if 0	
	ldr r0, =.msg1
	bl ETrWriteString8
#endif

	/* now go to the start area in kernel space */
	mov r0, r11
	ldr ip, =COMROGUEStart
	mov pc, ip

#if 0
	.balign 4
.msg1:
	.asciz "got here 1\n"
	.balign 4
#endif
	
/*-------------------------------------------------------------
 * Early exception-handler code, mainly for debugging purposes
 *-------------------------------------------------------------
 */

.earlyReset:
	ldr r0, =.resetMessage
	b .earlyFUBAR
.earlyUndef:
	ldr r0, =.undefMessage
	b .earlyFUBAR
.earlySVC:
	ldr r0, =.svcMessage
	b .earlyFUBAR
.earlyPrefetch:
	ldr r0, =.prefetchMessage
	b .earlyFUBAR
.earlyData:
	ldr r0, =.dataMessage
	b .earlyFUBAR
.earlyHyp:
	ldr r0, =.hypMessage
	b .earlyFUBAR
.earlyIRQ:
	ldr r0, =.irqMessage
	b .earlyFUBAR
.earlyFIQ:
	ldr r0, =.fiqMessage
.earlyFUBAR:
	bl ETrWriteString8
.hang:
	wfe
	b .hang

.earlyVectorTable:
	ldr pc, .resetVector
	ldr pc, .undefVector
	ldr pc, .svcVector
	ldr pc, .prefetchVector
	ldr pc, .dataVector
	ldr pc, .hypVector
	ldr pc, .irqVector
	ldr pc, .fiqVector
.resetVector:
	.word .earlyReset
.undefVector:
	.word .earlyUndef
.svcVector:
	.word .earlySVC
.prefetchVector:
	.word .earlyPrefetch
.dataVector:
	.word .earlyData
.hypVector:
	.word .earlyHyp
.irqVector:
	.word .earlyIRQ
.fiqVector:
	.word .earlyFIQ
.earlyVectorTableEnd:
	
	.balign 4
.initMessage:
	.asciz "COMROGUEPrestart\n"
	.balign 4
.resetMessage:
	.asciz "+++ RESET!!!\n"
	.balign 4
.undefMessage:
	.asciz "+++ UNDEF!!!\n"
	.balign 4
.svcMessage:
	.asciz "+++ SVC!!!\n"
	.balign 4
.prefetchMessage:
	.asciz "+++ PREFETCH!!!\n"
	.balign 4
.dataMessage:
	.asciz "+++ DATA!!!\n"
	.balign 4
.hypMessage:
	.asciz "+++ HYP!!!\n"
	.balign 4
.irqMessage:
	.asciz "+++ IRQ!!!\n"
	.balign 4
.fiqMessage:
	.asciz "+++ FIQ!!!\n"

/*--------------------------------------------------------------------------------------
 * Low-level IO functions that are placed here to make them callable from prestart code
 *--------------------------------------------------------------------------------------
 */

.section ".prestart.text"
	
/*
 * Writes a 32-bit word of data to a memory-mapped IO port.
 *
 * Parameters:
 *   - paPort = Physical address of the IO port to write to.
 *   - uiData = Data to be written.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Writing to any given IO port may have arbitrary side effects.
 */
.globl llIOWritePA
llIOWritePA:
	str r1,[r0]
	bx lr

/*
 * Reads a 32-bit word of data from a memory-mapped IO port.
 *
 * Parameters:
 *   - paPort = Physical address of the IO port we read from.
 *
 * Returns:
 * The word of data read from the IO port.
 *
 * Side effects:
 * Reading from any given IO port may have arbitrary side effects.
 */
.globl llIOReadPA
llIOReadPA:
	ldr r0,[r0]
	bx lr

/*
 * Delays for a certain number of cycles, to allow an IO operation to work.
 *
 * Parameters:
 *   - uiTicks = The number of "ticks" to delay.
 *
 * Returns:
 * Nothing.
 */
.globl llIODelayPA
llIODelayPA:
	push {lr}
.delaytop:
	nop
	nop
	bl .delayreturn
	nop
	nop
	subs r0, r0, #1
	bne .delaytop
	pop {lr}
.delayreturn:
	bx lr
