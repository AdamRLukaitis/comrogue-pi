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
#include <comrogue/internals/asm-macros.h>
#include <comrogue/internals/mmu.h>
	
.section ".text"

/*---------------------------------------
 * Low-level memory-management functions
 *---------------------------------------
 */

/*
 * Flushes the system cache of all data on a page.  Optionally writes back writeable data before flushing.
 *
 * Parameters:
 * - vmaPage = The page to be invalidated.
 * - bWriteback = TRUE to write back before invalidating, FALSE to not do so.
 *
 * Returns:
 * Nothing.
 */
.globl _MmFlushCacheForPage
_MmFlushCacheForPage:
	mov r2, # SYS_PAGE_SIZE
	sub r2, r2, #1
	orr ip, r0, r2			        /* expand so that [r0, ip] is the range to invalidate */
	bic r0, r0, r2
	tst r1, r1				/* is this a writeable page? */
	mcrrne p15, 0, ip, r0, c14		/* yes, clean and invalidate */
	mcrreq p15, 0, ip, r0, c6		/* no, just invalidate */
	mcrr p15, 0, ip, r0, c5			/* either way, invalidate instruction cache */
	bx lr

/*
 * Flushes the system cache of all data in a section.  Optionally writes back writeable data before flushing.
 *
 * Parameters:
 * - vmaSection = The section to be invalidated.
 * - bWriteback = TRUE to write back before invalidating, FALSE to not do so.
 *
 * Returns:
 * Nothing.
 */
.globl _MmFlushCacheForSection
_MmFlushCacheForSection:
	mov r2, # SYS_SEC_SIZE
	sub r2, r2, #1
	bic r0, r0, r2				/* expand so that [r0, ip] is the range to invalidate */
	orr ip, r0, r2
	tst r1, r1				/* is this a writeable section? */
	mcrrne p15, 0, ip, r0, c14		/* yes, clean and invalidate */
	mcrreq p15, 0, ip, r0, c6		/* no, just invalidate */
	mcrr p15, 0, ip, r0, c5			/* either way, invalidate instruction cache */
	bx lr

/*
 * Flushes the TLB for this page in the current address-space context.
 *
 * Parameters:
 * - vmaPage = The page to be invalidated.
 *
 * Returns:
 * Nothing.
 */
.globl _MmFlushTLBForPage
/*
 * Flushes the TLB for this page in a specified address-space context.
 *
 * Parameters:
 * - vmaPage = The page to be invalidated.
 * - uiASID = Address-space identifier.
 *
 * Returns:
 * Nothing.
 */
.globl _MmFlushTLBForPageAndContext
_MmFlushTLBForPage:
	mrc p15, 0, r1, c13, c0, 1		/* get current context */
_MmFlushTLBForPageAndContext:
	and r1, r1, #0xFF			/* get ASID */
	mov ip, # SYS_PAGE_SIZE
	sub ip, ip, #1
	bic r0, r0, ip				/* mask off "page" bits */
	orr r0, r0, r1				/* add in specified ASID */
	mcr p15, 0, r0, c8, c5, 1		/* invalidate TLB by virtual address */
	bx lr

/*
 * Flushes the TLB for this section in the current address-space context.
 *
 * Parameters:
 * - vmaPage = The section to be invalidated.
 *
 * Returns:
 * Nothing.
 */
.globl _MmFlushTLBForSection
/*
 * Flushes the TLB for this section in a specified address-space context.
 *
 * Parameters:
 * - vmaSection = The page to be invalidated.
 * - uiASID = Address-space identifier.
 *
 * Returns:
 * Nothing.
 */
.globl _MmFlushTLBForSectionAndContext
_MmFlushTLBForSection:
	mrc p15, 0, r1, c13, c0, 1		/* get current context */
_MmFlushTLBForSectionAndContext:
	and r1, r1, #0xFF			/* get ASID */
	mov ip, # SYS_SEC_SIZE
	sub ip, ip, #1
	and r0, r0, ip				/* r0 = first page to invalidate */
	orr r0, r0, r1
	add ip, r0, # SYS_SEC_SIZE		/* ip = last page to invalidate */
.flush1:
	mcr p15, 0, r0, c8, c5, 1		/* invalidate TLB by virtual address */
	add r0, r0, # SYS_PAGE_SIZE		/* next page */
	cmp r0, ip				/* are we done? */
	bxeq lr					/* yes, bug out */
	b .flush1				/* no, keep going */

/*
 * Returns the value of TTB0, the pointer to the process-level TTB.
 *
 * Parameters:
 * None.
 *
 * Returns:
 * A pointer to the process-level TTB.
 */
.globl _MmGetTTB0
_MmGetTTB0:
	mrc p15, 0, r0, c2, c0, 0
	bx lr

/*
 * Sets the value of TTB0, the pointer to the process-level TTB.
 *
 * Parameters:
 * - pTTB = Pointer to the new process-level TTB.
 *
 * Returns:
 * Nothing.
 *
 * N.B.:
 * Only call this from within kernel code, as otherwise the results can be unpredictable.
 */
.globl _MmSetTTB0
_MmSetTTB0:
	mov ip, #0
	mcr p15, 0, ip, c7, c7, 0 		/* clear caches */
	mcr p15, 0, ip, c8, c7, 0		/* clear TLB */
	instr_barrier
	mcr p15, 0, r0, c2, c0, 0		/* set TTB0 */	
	mrc p15, 0, ip, c0, c0, 0       	/* read ID register */
	instr_barrier
	bx lr
