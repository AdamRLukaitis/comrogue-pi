/*
 * Portions of this file are subject to the following notices:
 *
 * Copyright (C) 2012 Andrew Turner
 * All rights reserved.
 *
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* Derived from FreeBSD libkern divsi3.S, uldivmod.S, and ldivmod.S, munged by Erbo to COMROGUE standards */
.globl	__aeabi_uidiv
.globl	__aeabi_uidivmod
__aeabi_uidiv:
__aeabi_uidivmod:
	eor	r0, r1, r0		/* r0 = r0 / r1; r1 = r0 % r1 */
	eor	r1, r0, r1
	eor	r0, r1, r0		/* r0 = r1 / r0; r1 = r1 % r0 */

	cmp	r0, #1
	bcc	.L_overflow
	beq	.L_divide_l0
	mov	ip, #0
	movs	r1, r1
	bpl	.L_divide_l1
	orr	ip, ip, #0x20000000	/* indicates r1 is negative */
	movs	r1, r1, lsr #1
	orrcs	ip, ip, #0x10000000	/* indicates bit 0 of r1 */
	b	.L_divide_l1

.L_divide_l0:
	mov	r0, r1			/* r0 == 1 */
	mov	r1, #0
	bx	lr

.globl __aeabi_idiv
.globl __aeabi_idivmod
__aeabi_idiv:
__aeabi_idivmod:
	eor	r0, r1, r0		/* r0 = r0 / r1; r1 = r0 % r1 */
	eor	r1, r0, r1
	eor	r0, r1, r0		/* r0 = r1 / r0; r1 = r1 % r0 */

	cmp	r0, #1
	bcc	.L_overflow
	beq	.L_divide_l0
	ands	ip, r0, #0x80000000
	rsbmi	r0, r0, #0
	ands	r2, r1, #0x80000000
	eor	ip, ip, r2
	rsbmi	r1, r1, #0
	orr	ip, r2, ip, lsr #1	/* bit 0x40000000 = negative division, bit 0x80000000 = negative remainder */

.L_divide_l1:
	mov	r2, #1
	mov	r3, #0

	/*
	 * If the highest bit of the dividend is set, we have to be
	 * careful when shifting the divisor. Test this. 	
	 */
	movs	r1, r1
	bpl	.L_old_code
	
	/*
	 * At this point, the highest bit of r1 is known to be set.
	 * We abuse this below in the tst instructions.
	 */
	tst	r1, r0 /*, lsl #0 */
	bmi	.L_divide_b1
	tst	r1, r0, lsl #1
	bmi	.L_divide_b2
	tst	r1, r0, lsl #2
	bmi	.L_divide_b3
	tst	r1, r0, lsl #3
	bmi	.L_divide_b4
	tst	r1, r0, lsl #4
	bmi	.L_divide_b5
	tst	r1, r0, lsl #5
	bmi	.L_divide_b6
	tst	r1, r0, lsl #6
	bmi	.L_divide_b7
	tst	r1, r0, lsl #7
	bmi	.L_divide_b8
	tst	r1, r0, lsl #8
	bmi	.L_divide_b9
	tst	r1, r0, lsl #9
	bmi	.L_divide_b10
	tst	r1, r0, lsl #10
	bmi	.L_divide_b11
	tst	r1, r0, lsl #11
	bmi	.L_divide_b12
	tst	r1, r0, lsl #12
	bmi	.L_divide_b13
	tst	r1, r0, lsl #13
	bmi	.L_divide_b14
	tst	r1, r0, lsl #14
	bmi	.L_divide_b15
	tst	r1, r0, lsl #15
	bmi	.L_divide_b16
	tst	r1, r0, lsl #16
	bmi	.L_divide_b17
	tst	r1, r0, lsl #17
	bmi	.L_divide_b18
	tst	r1, r0, lsl #18
	bmi	.L_divide_b19
	tst	r1, r0, lsl #19
	bmi	.L_divide_b20
	tst	r1, r0, lsl #20
	bmi	.L_divide_b21
	tst	r1, r0, lsl #21
	bmi	.L_divide_b22
	tst	r1, r0, lsl #22
	bmi	.L_divide_b23
	tst	r1, r0, lsl #23
	bmi	.L_divide_b24
	tst	r1, r0, lsl #24
	bmi	.L_divide_b25
	tst	r1, r0, lsl #25
	bmi	.L_divide_b26
	tst	r1, r0, lsl #26
	bmi	.L_divide_b27
	tst	r1, r0, lsl #27
	bmi	.L_divide_b28
	tst	r1, r0, lsl #28
	bmi	.L_divide_b29
	tst	r1, r0, lsl #29
	bmi	.L_divide_b30
	tst	r1, r0, lsl #30
	bmi	.L_divide_b31
/*
 * instead of:
 *	tst	r1, r0, lsl #31
 *	bmi	.L_divide_b32
 */
	b	.L_divide_b32

.L_old_code:
	cmp	r1, r0
	bcc	.L_divide_b0
	cmp	r1, r0, lsl #1
	bcc	.L_divide_b1
	cmp	r1, r0, lsl #2
	bcc	.L_divide_b2
	cmp	r1, r0, lsl #3
	bcc	.L_divide_b3
	cmp	r1, r0, lsl #4
	bcc	.L_divide_b4
	cmp	r1, r0, lsl #5
	bcc	.L_divide_b5
	cmp	r1, r0, lsl #6
	bcc	.L_divide_b6
	cmp	r1, r0, lsl #7
	bcc	.L_divide_b7
	cmp	r1, r0, lsl #8
	bcc	.L_divide_b8
	cmp	r1, r0, lsl #9
	bcc	.L_divide_b9
	cmp	r1, r0, lsl #10
	bcc	.L_divide_b10
	cmp	r1, r0, lsl #11
	bcc	.L_divide_b11
	cmp	r1, r0, lsl #12
	bcc	.L_divide_b12
	cmp	r1, r0, lsl #13
	bcc	.L_divide_b13
	cmp	r1, r0, lsl #14
	bcc	.L_divide_b14
	cmp	r1, r0, lsl #15
	bcc	.L_divide_b15
	cmp	r1, r0, lsl #16
	bcc	.L_divide_b16
	cmp	r1, r0, lsl #17
	bcc	.L_divide_b17
	cmp	r1, r0, lsl #18
	bcc	.L_divide_b18
	cmp	r1, r0, lsl #19
	bcc	.L_divide_b19
	cmp	r1, r0, lsl #20
	bcc	.L_divide_b20
	cmp	r1, r0, lsl #21
	bcc	.L_divide_b21
	cmp	r1, r0, lsl #22
	bcc	.L_divide_b22
	cmp	r1, r0, lsl #23
	bcc	.L_divide_b23
	cmp	r1, r0, lsl #24
	bcc	.L_divide_b24
	cmp	r1, r0, lsl #25
	bcc	.L_divide_b25
	cmp	r1, r0, lsl #26
	bcc	.L_divide_b26
	cmp	r1, r0, lsl #27
	bcc	.L_divide_b27
	cmp	r1, r0, lsl #28
	bcc	.L_divide_b28
	cmp	r1, r0, lsl #29
	bcc	.L_divide_b29
	cmp	r1, r0, lsl #30
	bcc	.L_divide_b30
.L_divide_b32:
	cmp	r1, r0, lsl #31
	subhs	r1, r1,r0, lsl #31
	addhs	r3, r3,r2, lsl #31
.L_divide_b31:
	cmp	r1, r0, lsl #30
	subhs	r1, r1,r0, lsl #30
	addhs	r3, r3,r2, lsl #30
.L_divide_b30:
	cmp	r1, r0, lsl #29
	subhs	r1, r1,r0, lsl #29
	addhs	r3, r3,r2, lsl #29
.L_divide_b29:
	cmp	r1, r0, lsl #28
	subhs	r1, r1,r0, lsl #28
	addhs	r3, r3,r2, lsl #28
.L_divide_b28:
	cmp	r1, r0, lsl #27
	subhs	r1, r1,r0, lsl #27
	addhs	r3, r3,r2, lsl #27
.L_divide_b27:
	cmp	r1, r0, lsl #26
	subhs	r1, r1,r0, lsl #26
	addhs	r3, r3,r2, lsl #26
.L_divide_b26:
	cmp	r1, r0, lsl #25
	subhs	r1, r1,r0, lsl #25
	addhs	r3, r3,r2, lsl #25
.L_divide_b25:
	cmp	r1, r0, lsl #24
	subhs	r1, r1,r0, lsl #24
	addhs	r3, r3,r2, lsl #24
.L_divide_b24:
	cmp	r1, r0, lsl #23
	subhs	r1, r1,r0, lsl #23
	addhs	r3, r3,r2, lsl #23
.L_divide_b23:
	cmp	r1, r0, lsl #22
	subhs	r1, r1,r0, lsl #22
	addhs	r3, r3,r2, lsl #22
.L_divide_b22:
	cmp	r1, r0, lsl #21
	subhs	r1, r1,r0, lsl #21
	addhs	r3, r3,r2, lsl #21
.L_divide_b21:
	cmp	r1, r0, lsl #20
	subhs	r1, r1,r0, lsl #20
	addhs	r3, r3,r2, lsl #20
.L_divide_b20:
	cmp	r1, r0, lsl #19
	subhs	r1, r1,r0, lsl #19
	addhs	r3, r3,r2, lsl #19
.L_divide_b19:
	cmp	r1, r0, lsl #18
	subhs	r1, r1,r0, lsl #18
	addhs	r3, r3,r2, lsl #18
.L_divide_b18:
	cmp	r1, r0, lsl #17
	subhs	r1, r1,r0, lsl #17
	addhs	r3, r3,r2, lsl #17
.L_divide_b17:
	cmp	r1, r0, lsl #16
	subhs	r1, r1,r0, lsl #16
	addhs	r3, r3,r2, lsl #16
.L_divide_b16:
	cmp	r1, r0, lsl #15
	subhs	r1, r1,r0, lsl #15
	addhs	r3, r3,r2, lsl #15
.L_divide_b15:
	cmp	r1, r0, lsl #14
	subhs	r1, r1,r0, lsl #14
	addhs	r3, r3,r2, lsl #14
.L_divide_b14:
	cmp	r1, r0, lsl #13
	subhs	r1, r1,r0, lsl #13
	addhs	r3, r3,r2, lsl #13
.L_divide_b13:
	cmp	r1, r0, lsl #12
	subhs	r1, r1,r0, lsl #12
	addhs	r3, r3,r2, lsl #12
.L_divide_b12:
	cmp	r1, r0, lsl #11
	subhs	r1, r1,r0, lsl #11
	addhs	r3, r3,r2, lsl #11
.L_divide_b11:
	cmp	r1, r0, lsl #10
	subhs	r1, r1,r0, lsl #10
	addhs	r3, r3,r2, lsl #10
.L_divide_b10:
	cmp	r1, r0, lsl #9
	subhs	r1, r1,r0, lsl #9
	addhs	r3, r3,r2, lsl #9
.L_divide_b9:
	cmp	r1, r0, lsl #8
	subhs	r1, r1,r0, lsl #8
	addhs	r3, r3,r2, lsl #8
.L_divide_b8:
	cmp	r1, r0, lsl #7
	subhs	r1, r1,r0, lsl #7
	addhs	r3, r3,r2, lsl #7
.L_divide_b7:
	cmp	r1, r0, lsl #6
	subhs	r1, r1,r0, lsl #6
	addhs	r3, r3,r2, lsl #6
.L_divide_b6:
	cmp	r1, r0, lsl #5
	subhs	r1, r1,r0, lsl #5
	addhs	r3, r3,r2, lsl #5
.L_divide_b5:
	cmp	r1, r0, lsl #4
	subhs	r1, r1,r0, lsl #4
	addhs	r3, r3,r2, lsl #4
.L_divide_b4:
	cmp	r1, r0, lsl #3
	subhs	r1, r1,r0, lsl #3
	addhs	r3, r3,r2, lsl #3
.L_divide_b3:
	cmp	r1, r0, lsl #2
	subhs	r1, r1,r0, lsl #2
	addhs	r3, r3,r2, lsl #2
.L_divide_b2:
	cmp	r1, r0, lsl #1
	subhs	r1, r1,r0, lsl #1
	addhs	r3, r3,r2, lsl #1
.L_divide_b1:
	cmp	r1, r0
	subhs	r1, r1, r0
	addhs	r3, r3, r2
.L_divide_b0:

	tst	ip, #0x20000000
	bne	.L_udivide_l1
	mov	r0, r3
	cmp	ip, #0
	rsbmi	r1, r1, #0
	movs	ip, ip, lsl #1
	bicmi	r0, r0, #0x80000000	/* Fix incase we divided 0x80000000 */
	rsbmi	r0, r0, #0
	bx	lr
	
.L_udivide_l1:
	tst	ip, #0x10000000
	mov	r1, r1, lsl #1
	orrne	r1, r1, #1
	mov	r3, r3, lsl #1
	cmp	r1, r0
	subhs	r1, r1, r0
	addhs	r3, r3, r2
	mov	r0, r3
	bx	lr

.L_overflow:
	/* TODO: cause an exception or something? */
	mvn	r0, #0
	bx 	lr

.globl	__aeabi_uldivmod
__aeabi_uldivmod:
	push	{r4,lr}
	sub	sp, sp, #8
	mov	r4, sp
	push 	{r4}
	bl	__qdivrem
	pop	{r4}
	/*
	 * The remainder is already on the stack just waiting to be popped
	 * into r2/r3.
	 */
	pop	{r2-r4,lr}
	bx	lr

.globl	__aeabi_ldivmod
__aeabi_ldivmod:
	push	{r4-r5, sl, lr}
	mov	r5, #0	/* r5 = negative indicator */
	
	cmp	r3, #0
	bge	2f
	eor	r5, r5, #1	/* flip quotient sign */
	bl	.Lnegate_b
	bcs	.Lmaxdenom

2:
	cmp	r1, #0
	/* bge	3f */
	eorlt	r5, r5, #3	/* flip quotient sign, flip remainder sign */
	bllt	.Lnegate_a
3:
	/*
	 * Arguments are setup, allocate some stack for the remainder
	 * and call __qdivrem for the heavy lifting.
	 */
	sub	sp, sp, #8
	mov	r4, sp		/* pointer to remainder */
	push	{r4}
	bl	__qdivrem
	pop	{r4}

	teq	r5, #0		/* any signs to flip? */
	/*
	 * The quotient is already in the right place and neither value
	 * needs its sign flipped.
	 */
	popeq	{r2-r5, sl, lr}
	bxeq	lr

	pop	{r2, r3}
	tst	r5, #2		/* does remainder need to be negative? */
	bleq	.Lnegate_b
	tst	r5, #1		/* does quotient need to be negative? */
	bleq	.Lnegate_a
	pop	{r4-r5, sl, lr}
	bx	lr

.Lnegate_a:
        rsbs	r0, r0, #0
        rsc	r1, r1, #0
	bx	lr

.Lnegate_b:
        rsbs	r2, r2, #0
        rsc	r3, r3, #0
	bx	lr

.Lmaxdenom:
	/*
	 * We had a carry so the denominator must have INT64_MIN
	 * Also BLO and BHI never changed values so we can use
	 * them to see if the numerator has the same value.  We
	 * don't have to worry about sign.
	 */
	teq	r3, r1 
	teqeq	r2, r0
	bne	1f

	/*
	 * They were equal, so we return a quotient of 1 and remainder of 0.
	 */
	mov	r0, #1
	mov	r1, #0
	mov	r2, #0
	mov	r3, #0
	pop	{r4-r5, sl, lr}
	bx	lr

	/*
	 * Our remainder must be the numerator and our quotient is 0.
	 */
1:	mov	r2, r0
	mov	r3, r1
	mov	r0, #0
	mov	r1, #0
	pop	{r4-r5, sl, lr}
	bx	lr
