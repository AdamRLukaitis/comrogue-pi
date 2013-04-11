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
#ifndef __SCTLR_H_INCLUDED
#define __SCTLR_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

/*------------------------------------------------------
 * Bits in the System Control Register (SCTLR), CP15 c1
 *------------------------------------------------------
 */

#define SCTLR_M          0x00000001      /* MMU: 1 = enabled, 0 = disabled */
#define SCTLR_A          0x00000002      /* Alignment check: 1 = enabled, 0 = disabled */
#define SCTLR_C          0x00000004      /* Cache: 1 = enabled, 0 = disabled */
#define SCTLR_CP15BEN    0x00000020      /* CP15 barrier operations: 1 = enabled (default), 0 = disabled */
#define SCTLR_B          0x00000080      /* Endianness: 0 = default, do not modify */
#define SCTLR_SW         0x00000400      /* SWP/SWPB instructions: 0 = disable, 1 = enable */
#define SCTLR_Z          0x00000800      /* Branch prediction: 0 = disable, 1 = enable */
#define SCTLR_I          0x00001000      /* Instruction cache: 0 = disable, 1 = enable */
#define SCTLR_V          0x00002000      /* Exception vectors: 0 = 0x00000000 (configurable), 1 = 0xFFFF0000 (fixed) */
#define SCTLR_RR         0x00004000      /* Cache strategy: 0 = normal, 1 = round-robin */
#define SCTLR_L4         0x00008000      /* PC load reset T-bit: 0 = yes, 1 = no */
#define SCTLR_HA         0x00020000      /* Hardware access flag: 0 = disabled, 1 = enabled */
#define SCTLR_WXN        0x00080000      /* Write permission implies XN: 0 = no, 1 = yes */
#define SCTLR_UWXN       0x00100000      /* Unprivileged write permission implies PL1 XN: 0 = no, 1 = yes */
#define SCTLR_FI         0x00200000      /* Fast interrupts: 0 = normal, 1 = low-latency */
#define SCTLR_U          0x00400000      /* unaligned access: 0 = disabled (default), 1 = enabled */
#define SCTLR_XP         0x00800000      /* subpage AP bits: 0 = enabled, 1 = disabled */
#define SCTLR_VE         0x01000000      /* Interrupt vectors: 0 = standard, 1 = determined by VIC */
#define SCTLR_EE         0x02000000      /* Exception endianness: 0 = little-endian (default), 1 = big-endian */
#define SCTLR_NMFI       0x08000000      /* Non-maskable FIQ: 0 = disabled, 1 = enabled */
#define SCTLR_TRE        0x10000000      /* TEX attributes remap: 0 = disabled, 1 = enabled */
#define SCTLR_AFE        0x20000000      /* AP[0] = Access Flag: 0 = disabled, 1 = enabled */
#define SCTLR_TE         0x40000000      /* Instruction set state for exceptions: 0 = ARM (default), 1 = Thumb */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __SCTLR_H_INCLUDED */
