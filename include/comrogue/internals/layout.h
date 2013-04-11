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
#ifndef __LAYOUT_H_INCLUDED
#define __LAYOUT_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

/*-----------------------------------------------------------
 * Constants defining the layout of the COMROGUE memory map.
 *-----------------------------------------------------------
 */

#define PHYSADDR_LOAD            0x8000               /* physical address at which the loader loads the kernel */
#define PHYSADDR_IO_BASE         0x20000000           /* physical address that's the base for memory-mapped IO */
#define VMADDR_TTB_FENCE         0x80000000           /* address that's the dividing point between TTBs */
#define VMADDR_LIBRARY_FENCE     0xB0000000           /* base address for kernel "shared library" code */
#define VMADDR_KERNEL_FENCE      0xC0000000           /* base address for the internal kernel code */
#define VMADDR_IO_BASE           0xE0000000           /* base address for memory-mapped IO */
#define PAGE_COUNT_IO            1024                 /* 4 megabytes mapped for IO */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __LAYOUT_H_INCLUDED */
