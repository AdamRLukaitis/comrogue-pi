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
#ifndef __AUXDEV_H_INCLUDED
#define __AUXDEV_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

/*-------------------------------
 * BCM2835 auxiliary peripherals
 *-------------------------------
 */

/* Register physical addresses */
#define AUX_REG_IRQ          0x20215000   /* AUX interrupt status */
#define AUX_REG_ENABLE       0x20215004   /* AUX enable */
#define AUX_MU_REG_THR       0x20215040   /* Mini-UART Transmitter Holding Register */
#define AUX_MU_REG_RHR       0x20215040   /* Mini-UART Receiver Holding Register */
#define AUX_MU_REG_IER       0x20215044   /* Mini-UART Interrupt Enable Register */
#define AUX_MU_REG_IIR       0x20215048   /* Mini-UART Interrupt Identification Register */
#define AUX_MU_REG_FCR       0x20215048   /* Mini-UART FIFO Control Register */
#define AUX_MU_REG_LCR       0x2021504C   /* Mini-UART Line Control Register */
#define AUX_MU_REG_MCR       0x20215050   /* Mini-UART Modem Control Register */
#define AUX_MU_REG_LSR       0x20215054   /* Mini-UART Line Status Register */
#define AUX_MU_REG_MSR       0x20215058   /* Mini-UART Modem Status Register */
#define AUX_MU_REG_SCR       0x2021505C   /* Mini-UART Scratch Register */
#define AUX_MU_REG_CNTL      0x20215060   /* Mini-UART Auxiliary Control Register */
#define AUX_MU_REG_STAT      0x20215064   /* Mini-UART Auxiliary Status Register */
#define AUX_MU_REG_BAUD      0x20215068   /* Mini-UART Baud Rate Register */
#define AUX_SPI0_REG_CTL0    0x20215080   /* SPI 0 Control Register 0 */
#define AUX_SPI0_REG_CTL1    0x20215084   /* SPI 0 Control Register 1 */
#define AUX_SPI0_REG_STAT    0x20215088   /* SPI 0 Status Register */
#define AUX_SPI0_REG_IO      0x20215090   /* SPI 0 Data Register */
#define AUX_SPI0_REG_PEEK    0x20215094   /* SPI 0 Peek Register */
#define AUX_SPI1_REG_CTL0    0x202150C0   /* SPI 1 Control Register 0 */
#define AUX_SPI1_REG_CTL1    0x202150C4   /* SPI 1 Control Register 1 */
#define AUX_SPI1_REG_STAT    0x202150C8   /* SPI 1 Status Register */
#define AUX_SPI1_REG_IO      0x202150D0   /* SPI 1 Data Register */
#define AUX_SPI1_REG_PEEK    0x202150D4   /* SPI 1 Peek Register */

/* AUX IRQ register bits */
#define AUX_IRQ_MU           0x00000001   /* Mini-UART has interrupt pending */
#define AUX_IRQ_SPI0         0x00000002   /* SPI 0 has interrupt pending */
#define AUX_IRQ_SPI1         0x00000004   /* SPI 1 has interrupt pending */

/* AUX Enable register bits */
#define AUX_ENABLE_MU        0x00000001   /* Mini-UART enable */
#define AUX_ENABLE_SPI0      0x00000002   /* SPI 0 enable */
#define AUX_ENABLE_SPI1      0x00000004   /* SPI 1 enable */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __AUXDEV_H_INCLUDED__ */
