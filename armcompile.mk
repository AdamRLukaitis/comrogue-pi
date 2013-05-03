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

# Define the directory and prefix for ARM cross-compiler tools.
ARMDIR ?= /opt/gnuarm/bin
ARMPREFIX ?= arm-none-eabi

# Define the ARM cross-compiler tools.
CC := $(ARMDIR)/$(ARMPREFIX)-gcc
CPP := $(ARMDIR)/$(ARMPREFIX)-cpp
AS := $(ARMDIR)/$(ARMPREFIX)-as
LD := $(ARMDIR)/$(ARMPREFIX)-ld
OBJDUMP := $(ARMDIR)/$(ARMPREFIX)-objdump
OBJCOPY := $(ARMDIR)/$(ARMPREFIX)-objcopy

# Define the default flags for compilation.
DEFS := -D__COMROGUE_INTERNALS__
INCLUDES := -I$(CRBASEDIR)/include -I$(CRBASEDIR)/idl
CFLAGS := $(INCLUDES) -mabi=aapcs -mfloat-abi=hard -mcpu=arm1176jzf-s -Wall -O2 \
	  -nostdlib -nostartfiles -ffreestanding $(DEFS)
AFLAGS := -mcpu=arm1176jzf-s -mfloat-abi=hard
ASM_CPP_FLAGS := $(INCLUDES) $(DEFS) -D__ASM__

# Standard rule for pre-processing linker script files.
%.lds: %.ldi
	$(CPP) $(ASM_CPP_FLAGS) -P -o $@ $<

# Standard rule for preprocessing and assembling assembler files.
%.o: %.S
	$(CPP) $(ASM_CPP_FLAGS) -o $(basename $<).s $<
	$(AS) $(AFLAGS) -o $@ $(basename $<).s

# Standard rule for compiling C files.
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

