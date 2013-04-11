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
#ifndef __X__16550_H_INCLUDED
#define __X__16550_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

/*------------------------------------------------------------------------------------------------------------
 * Standard bits used in the registers of a 16550 UART.  Note that not all of these bits are supported by the
 * BCM2835 mini-UART; they are included here for completeness.
 *------------------------------------------------------------------------------------------------------------
 */

/* Interrupt Enable Register (read/write) */
#define U16550_IER_RXREADY       0x01       /* Receive Ready interrupt */
#define U16550_IER_TXEMPTY       0x02       /* Transmit Empty interrupt */
#define U16550_IER_ERRBRK        0x04       /* Error/Break iterrupt */
#define U16550_IER_SINPUT        0x08       /* Status change interrupt */

/* Interrupt Identification Register (read-only) */
#define U16550_IIR_NOPENDING     0x01       /* Set if no interrupt is pending */
#define U16550_IIR_ID_MASK       0x0E       /* Pending interrupt mask */
#define U16550_IIR_ID_ERRBRK     0x06       /* Error/Break interrupt */
#define U16550_IIR_ID_RXREADY    0x04       /* Receive Ready interrupt */
#define U16550_IIR_ID_RXTIMEOUT  0x0C       /* Receive Timeout interrupt */
#define U16550_IIR_ID_TXEMPTY    0x02       /* Transmit Empty interrupt */
#define U16550_IIR_ID_SINPUT     0x00       /* Status change interrupt */
#define U16550_IIR_RXFIFO        0x40       /* Receive FIFO enabled */
#define U16550_IIR_TXFIFO        0x80       /* Transmit FIFO enabled */

/* FIFO Control Register (write-only) */
#define U16550_FCR_ENABLE        0x01       /* Enable FIFOs */
#define U16550_FCR_RXCLEAR       0x02       /* Clear receive FIFO */
#define U16550_FCR_TXCLEAR       0x04       /* Clear transmit FIFO */
#define U16550_FCR_MODE          0x08       /* FIFO mode select */
#define U16550_FCR_LEVEL_MASK    0xC0       /* FIFO level mask */
#define U16550_FCR_LEVEL_1       0x00       /* FIFO level: 1 character */
#define U16550_FCR_LEVEL_4       0x40       /* FIFO level: 4 characters */
#define U16550_FCR_LEVEL_8       0x80       /* FIFO level: 8 characters */
#define U16550_FCR_LEVEL_14      0xC0       /* FIFO level: 14 characters */

/* Line Control Register (read/write) */
#define U16550_LCR_LENGTH_MASK   0x03       /* Data length control mask */
#define U16550_LCR_LENGTH_5      0x00       /* Data length: 5 bits */
#define U16550_LCR_LENGTH_6      0x01       /* Data length: 6 bits */
#define U16550_LCR_LENGTH_7      0x02       /* Data length: 7 bits */
#define U16550_LCR_LENGTH_8      0x03       /* Data length: 8 bits */
#define U16550_LCR_STOP          0x04       /* Set = 2 stop bits, Clear = 1 stop bit */
#define U16550_LCR_PARITY_MASK   0x38       /* Parity control mask */
#define U16550_LCR_PARITY_NONE   0x00       /* Parity: NONE */
#define U16550_LCR_PARITY_ODD    0x08       /* Parity: ODD */
#define U16550_LCR_PARITY_EVEN   0x18       /* Parity: EVEN */
#define U16550_LCR_PARITY_MARK   0x28       /* Parity: MARK */
#define U16550_LCR_PARITY_SPACE  0x38       /* Parity: SPACE */
#define U16550_LCR_BREAK         0x40       /* Send BREAK when set */
#define U16550_LCR_DLAB          0x80       /* Divisor Latch Access Bit */

/* Modem Control Register (read/write) */
#define U16550_MCR_DTR           0x01       /* Set Data Terminal Ready */
#define U16550_MCR_RTS           0x02       /* Set Request To Send */
#define U16550_MCR_OP1           0x04       /* Set General-Purpose Output 1 */
#define U16550_MCR_OP2           0x08       /* Set General-Purpose Output 2 */
#define U16550_MCR_LOOPBACK      0x10       /* Loopback test mode */

/* Line Status Register (read-only) */
#define U16550_LSR_RXDATA        0x01       /* Received data ready */
#define U16550_LSR_OVERRUNERR    0x02       /* Receiver overrun error */
#define U16550_LSR_PARITYERR     0x04       /* Parity error */
#define U16550_LSR_FRAMEERR      0x08       /* Framing error */
#define U16550_LSR_BREAK         0x10       /* BREAK detected */
#define U16550_LSR_TXBUFEMPTY    0x20       /* Transmitter buffer empty */
#define U16550_LSR_TXEMPTY       0x40       /* Transmitter empty */

/* Modem Status Register (read-only) */
#define U16550_MSR_DELTA_CTS     0x01       /* change in Clear To Send line */
#define U16550_MSR_DELTA_DSR     0x02       /* change in Data Set Ready line */
#define U16550_MSR_DELTA_RI      0x04       /* change in Ring Indicator line */
#define U16550_MSR_DELTA_CD      0x08       /* change in Carrier Detect line */
#define U16550_MSR_CTS           0x10       /* Clear To Send */
#define U16550_MSR_DSR           0x20       /* Data Set Ready */
#define U16550_MSR_RI            0x40       /* Ring Indicator */
#define U16550_MSR_CD            0x80       /* Carrier Detect */

/* BCM2835-specific: Auxiliary control register (read/write) */
#define AUXMU_CNTL_RXENABLE      0x00000001 /* Receiver enable */
#define AUXMU_CNTL_TXENABLE      0x00000002 /* Transmitter enable */
#define AUXMU_CNTL_RXAUTOFLOW    0x00000004 /* Reciever auto-flow-control enabled */
#define AUXMU_CNTL_TXAUTOFLOW    0x00000008 /* Transmitter auto-flow-control enabled */
#define AUXMU_CNTL_RXAUTO_MASK   0x00000030 /* Receiver auto-flow-control level */
#define AUXMU_CNTL_RXAUTO_3      0x00000000 /* Receiver auto-flow-control: 3 spaces remaining */
#define AUXMU_CNTL_RXAUTO_2      0x00000010 /* Receiver auto-flow-control: 2 spaces remaining */
#define AUXMU_CNTL_RXAUTO_1      0x00000020 /* Receiver auto-flow-control: 1 space remaining */
#define AUXMU_CNTL_RXAUTO_4      0x00000030 /* Receiver auto-flow-control: 4 spaces remaining */
#define AUXMU_CNTL_RTSASSERT     0x00000040 /* RTS assert level invert */
#define AUXMU_CNTL_CTSASSERT     0x00000080 /* CTS assert level invert */

/* BCM2835-specific: Auxiliary status register (read-only) */
#define AUXMU_STAT_RXREADY       0x00000001 /* Receive ready */
#define AUXMU_STAT_TXREADY       0x00000002 /* Transmit ready */
#define AUXMU_STAT_RXIDLE        0x00000004 /* Receiver idle */
#define AUXMU_STAT_TXIDLE        0x00000008 /* Transmitter idle */
#define AUXMU_STAT_RXOVERRUN     0x00000010 /* Receiver overrun */
#define AUXMU_STAT_TXFULL        0x00000020 /* Transmit FIFO is full */
#define AUXMU_STAT_RTS           0x00000040 /* Request To Send line */
#define AUXMU_STAT_CTS           0x00000080 /* Clear To Send line */
#define AUXMU_STAT_TXDONE        0x00000100 /* Transmitter done (Tx idle, FIFO empty) */
#define AUXMU_STAT_RXLEVEL_MASK  0x000F0000 /* Receive FIFO fill level mask */
#define AUXMU_STAT_TXLEVEL_MASK  0x0F000000 /* Transmit FIFO fill level mask */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __X__16550_H_INCLUDED */
