#
# This file is part of the COMROGUE Operating System for Raspberry Pi
#
# Copyright (c) 2013, Eric J. Bowersox / Erbosoft Enterprises
# All rights reserved.
#
# This program is free for commercial and non-commercial use as long as the following conditions are
# adhered to.
#
# Copyright in this file remains Eric J. Bowersox and/or Erbosoft, and as such any copyright notices
# in the code are not to be removed.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted
# provided that the following conditions are met:
# 
# * Redistributions of source code must retain the above copyright notice, this list of conditions and
#   the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
#   the following disclaimer in the documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# "Raspberry Pi" is a trademark of the Raspberry Pi Foundation.

MAKEFLAGS += -rR

ARMDIR ?= /opt/gnuarm/bin
ARMPREFIX ?= arm-none-eabi
CC = $(ARMDIR)/$(ARMPREFIX)-gcc
CPP = $(ARMDIR)/$(ARMPREFIX)-cpp
AS = $(ARMDIR)/$(ARMPREFIX)-as
LD = $(ARMDIR)/$(ARMPREFIX)-ld
OBJDUMP = $(ARMDIR)/$(ARMPREFIX)-objdump
OBJCOPY = $(ARMDIR)/$(ARMPREFIX)-objcopy

DEFS = -D__COMROGUE_INTERNALS__
INCLUDES = -I../include -I../idl
CFLAGS = $(INCLUDES) -mabi=aapcs -mfloat-abi=hard -mcpu=arm1176jzf-s -Wall -O2 \
	-nostdlib -nostartfiles -ffreestanding $(DEFS)
AFLAGS = -mcpu=arm1176jzf-s -mfloat-abi=hard
ASM_CPP_FLAGS = $(INCLUDES) $(DEFS) -D__ASM__

PRESTART_OBJS = prestart.o early_trace.o collect_startup.o early_mm.o
LIB_OBJS = divide.o qdivrem.o intlib.o objhelp.o rbtree.o str.o strcopymem.o strcomparemem.o strsetmem.o lib_guids.o
RES_OBJS = lowlevel.o trace.o memmgr.o vmmap.o pagealloc.o
INIT_OBJS = start.o kistart.o init_heap.o

all:	kernel.img

comrogue-kernel.elf : $(PRESTART_OBJS) $(LIB_OBJS) $(RES_OBJS) $(INIT_OBJS) comrogue-kernel.lds
	$(CPP) $(ASM_CPP_FLAGS) -P -o comrogue-kernel.lds.parsed comrogue-kernel.lds
	$(LD) -T comrogue-kernel.lds.parsed $(PRESTART_OBJS) $(LIB_OBJS) $(RES_OBJS) \
		$(INIT_OBJS) -o comrogue-kernel.elf

kernel.img : comrogue-kernel.elf
	$(OBJDUMP) -D comrogue-kernel.elf > comrogue-kernel.list
	$(OBJDUMP) -t comrogue-kernel.elf > comrogue-kernel.syms
	sort comrogue-kernel.syms > comrogue-kernel.syms.sorted
	$(OBJCOPY) comrogue-kernel.elf -O binary kernel.img

%.o: %.S
	$(CPP) $(ASM_CPP_FLAGS) -o $(basename $<).s $<
	$(AS) $(AFLAGS) -o $@ $(basename $<).s

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm *.o *.s kernel.img comrogue-kernel.elf comrogue-kernel.lds.parsed comrogue-kernel.list \
			comrogue-kernel.syms*
