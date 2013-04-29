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
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/layout.h>

ENTRY(COMROGUEPrestart)

SECTIONS
{
  /* all these are prestart code that runs in low memory with MMU off */
  /* space is reclaimed once the kernel begins operation in high memory with MMU on */
  .prestart.text PHYSADDR_LOAD : AT (PHYSADDR_LOAD) {
    paPrestartCode = .;
    *(.prestartHEAD.text)  /* must be first! */
    *(.prestart.text)
    *(.prestart.rodata)
    *(.prestart.rodata.*)
    . = ALIGN(SYS_PAGE_SIZE);
  }
  cpgPrestartCode = (. - paPrestartCode) >> SYS_PAGE_BITS;
  paPrestartData = .;

  .prestart.data : AT(paPrestartData) {
    *(.prestart.data)
    . = ALIGN(SYS_PAGE_SIZE);
  }
  cpgPrestartData = (. - paPrestartData) >> SYS_PAGE_BITS;
  cpgPrestartTotal = . >> SYS_PAGE_BITS;
  paLibraryCode = .;

  /* kernel library code, in high memory mapped to be visible to user processes */
  .lib.text VMADDR_LIBRARY_FENCE : AT (paLibraryCode) {
    vmaLibraryCode = .;
    *(.lib.text)
    *(.lib.rodata)
    *(.lib.rodata.*)
    . = ALIGN(SYS_PAGE_SIZE);
  }
  cpgLibraryCode = (. - vmaLibraryCode) >> SYS_PAGE_BITS;
  paKernelCode = paLibraryCode + (. - vmaLibraryCode);

  /* kernel resident code, runs in high memory with MMU on */
  .text VMADDR_KERNEL_FENCE : AT(paKernelCode) {
    vmaKernelCode = .;
    *(.text)
    *(.rodata)
    *(.rodata.*)
    . = ALIGN(SYS_PAGE_SIZE);
  }
  cpgKernelCode = (. - vmaKernelCode) >> SYS_PAGE_BITS;
  paKernelData = paKernelCode + (. - vmaKernelCode);

  .data : AT(paKernelData) {
    vmaKernelData = .;
    *(.data)
    . = ALIGN(SYS_PAGE_SIZE);
  }
  cpgKernelData = (. - vmaKernelData) >> SYS_PAGE_BITS;
  paKernelBss = paKernelData + (. - vmaKernelData);

  .bss : AT(paKernelBss) {
    vmaKernelBss = .;
    *(.bss)
    *(COMMON)
    . = ALIGN(SYS_PAGE_SIZE);
  }
  cpgKernelBss = (. - vmaKernelBss) >> SYS_PAGE_BITS;
  paInitCode = paKernelBss + (. - vmaKernelBss);

  /* kernel one-time initialization code, runs in high memory with MMU on */
  /* space is reclaimed once the kernel finishes initialization */
  .init.text : AT(paInitCode) {
    vmaInitCode = .;
    *(.initHEAD.text)  /* must be first! */
    *(.init.text)
    *(.init.rodata)
    *(.init.rodata.*)
    . = ALIGN(SYS_PAGE_SIZE);
  }
  cpgInitCode = (. - vmaInitCode) >> SYS_PAGE_BITS;
  paInitData = paInitCode + (. - vmaInitCode);

  .init.data : AT(paInitData) {
    vmaInitData = .;
    *(.init.data)
    . = ALIGN(SYS_PAGE_SIZE);
  }
  cpgInitData = (. - vmaInitData) >> SYS_PAGE_BITS;
  paInitBss = paInitData + (. - vmaInitData);

  .init.bss : AT(paInitBss) {
    vmaInitBss = .;
    *(.init.bss)
    . = ALIGN(SYS_PAGE_SIZE);
  }
  cpgInitBss = (. - vmaInitBss) >> SYS_PAGE_BITS;
  paFirstFree = paInitBss + (. - vmaInitBss);
  vmaFirstFree = .;

  /DISCARD/ : {
    *(.comment)
    *(.note*)
    *(.ARM.attributes)
  }
}
