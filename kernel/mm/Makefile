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
CRBASEDIR := $(abspath ../..)
include $(CRBASEDIR)/armcompile.mk

MM_OBJS = lowlevel.o memmgr.o vmmap.o pagealloc.o kernel_space.o
MM_INIT_OBJS = init_heap.o

all:	kernel-mm.o

kernel-mm.o:	$(MM_OBJS) $(MM_INIT_OBJS) kernel-mm.lds
	$(LD) -r -T kernel-mm.lds $(MM_OBJS) $(MM_INIT_OBJS) -o kernel-mm.o

clean:
	-rm *.o *.s *.lds
