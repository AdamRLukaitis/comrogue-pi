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
#ifndef __GPIO_H_INCLUDED
#define __GPIO_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

/*--------------------
 * BCM2835 GPIO lines
 *--------------------
 */

/* GPIO register physical addresses */
#define GPFSEL0_REG       0x20200000        /* GPIO Function Select 0 (lines 0-9) */
#define GPFSEL1_REG       0x20200004        /* GPIO Function Select 1 (lines 10-19) */
#define GPFSEL2_REG       0x20200008        /* GPIO Function Select 2 (lines 20-29) */
#define GPFSEL3_REG       0x2020000C        /* GPIO Function Select 3 (lines 30-39) */
#define GPFSEL4_REG       0x20200010        /* GPIO Function Select 4 (lines 40-49) */
#define GPFSEL5_REG       0x20200014        /* GPIO Function Select 5 (lines 50-53) */
#define GPSET0_REG        0x2020001C        /* GPIO Output Set 0 (lines 0-31) */
#define GPSET1_REG        0x20200020        /* GPIO Output Set 1 (lines 32-53) */
#define GPCLR0_REG        0x20200028        /* GPIO Output Clear 0 (lines 0-31) */
#define GPCLR1_REG        0x2020002C        /* GPIO Output Clear 1 (lines 32-53) */
#define GPLEV0_REG        0x20200034        /* GPIO Pin Level Detect 0 (lines 0-31) */
#define GPLEV1_REG        0x20200038        /* GPIO Pin Level Detect 1 (lines 32-53) */
#define GPEDS0_REG        0x20200040        /* GPIO Pin Event Detect Status 0 (lines 0-31) */
#define GPEDS1_REG        0x20200044        /* GPIO Pin Event Detect Status 1 (lines 32-53) */
#define GPREN0_REG        0x2020004C        /* GPIO Pin Rising Edge Detect Enable 0 (lines 0-31) */
#define GPREN1_REG        0x20200050        /* GPIO Pin Rising Edge Detect Enable 1 (lines 32-53) */
#define GPFEN0_REG        0x20200085        /* GPIO Pin Falling Edge Detect Enable 0 (lines 0-31) */
#define GPFEN1_REG        0x2020005C        /* GPIO Pin Falling Edge Detect Enable 1 (lines 32-53) */
#define GPHEN0_REG        0x20200064        /* GPIO Pin High Level Detect Enable 0 (lines 0-31) */
#define GPHEN1_REG        0x20200068        /* GPIO Pin High Level Detect Enable 1 (lines 32-53) */
#define GPLEN0_REG        0x20200070        /* GPIO Pin Low Level Detect Enable 0 (lines 0-31) */
#define GPLEN1_REG        0x20200074        /* GPIO Pin Low Level Detect Enable 1 (lines 32-53) */
#define GPAREN0_REG       0x2020007C        /* GPIO Pin Async Rising Edge Detect Enable 0 (lines 0-31) */
#define GPAREN1_REG       0x20200080        /* GPIO Pin Async Rising Edge Detect Enable 1 (lines 32-53) */
#define GPAFEN0_REG       0x20200088        /* GPIO Pin Async Falling Edge Detect Enable 0 (lines 0-31) */
#define GPAFEN1_REG       0x2020008C        /* GPIO Pin Async Falling Edge Detect Enable 1 (lines 32-53) */
#define GPPUD_REG         0x20200094        /* GPIO Pin Pull-up/down Enable */
#define GPPUDCLK0_REG     0x20200098        /* GPIO Pin Pull-up/down Enable Clock 0 (lines 0-31) */
#define GPPUDCLK1_REG     0x2020009C        /* GPIO Pin Pull-up/down Enable Clock 1 (lines 32-53) */

#define GP_PIN_MASK       0x0007            /* GPIO pin function select mask */
#define GP_PIN_INPUT      0x0000            /* GPIO pin function is input */
#define GP_PIN_OUTPUT     0x0001            /* GPIO pin function is output */
#define GP_PIN_ALT0       0x0004            /* GPIO pin alternate function 0 */
#define GP_PIN_ALT1       0x0005            /* GPIO pin alternate function 1 */
#define GP_PIN_ALT2       0x0006            /* GPIO pin alternate function 2 */
#define GP_PIN_ALT3       0x0007            /* GPIO pin alternate function 3 */
#define GP_PIN_ALT4       0x0003            /* GPIO pin alternate function 4 */
#define GP_PIN_ALT5       0x0002            /* GPIO pin alternate function 5 */

#define GP_FUNC_MASK(n)         (GP_PIN_MASK << ((n) * 3))
#define GP_FUNC_BITS(n, bits)   (((bits) & GP_PIN_MASK) << ((n) * 3))
#define GP_BIT(n)               (1 << (n))

#define GPPUD_DISABLE     0x0000            /* Disable pull-up/pull-down */
#define GPPUD_PULLDOWN    0x0001            /* Enable pull-down */
#define GPPUD_PULLUP      0x0002            /* Enable pull-up */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __GPIO_H_INCLUDED */
