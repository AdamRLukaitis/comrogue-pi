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
#define __COMROGUE_INIT__
#include <comrogue/types.h>
#include <comrogue/internals/seg.h>
#include <comrogue/internals/trace.h>
#include <comrogue/internals/startup.h>
#include <comrogue/internals/mmu.h>

#ifdef THIS_FILE
#undef THIS_FILE
DECLARE_THIS_FILE
#endif

/*--------------------------------------------------------------------------------------------------------
 * Startup info buffer.  Data is copied into here by the assembly code before KiSystemStartup is invoked.
 *--------------------------------------------------------------------------------------------------------
 */

SEG_INIT_DATA STARTUP_INFO kiStartupInfo;  /* startup info buffer */

SEG_INIT_CODE static void dump_startup_info()
{
  TrPrintf8("Mach type %x, revision %x, serial %x\n", kiStartupInfo.uiMachineType, kiStartupInfo.uiRevision,
	    kiStartupInfo.uiSerialNumber);
  TrPrintf8("Memory: %u total pages (%u available), %u TTB gap\n", kiStartupInfo.cpgSystemTotal,
	    kiStartupInfo.cpgSystemAvail, kiStartupInfo.cpgTTBGap);
  TrPrintf8("TTB1 @ PA %08X, VMA %08X\n", kiStartupInfo.paTTB, kiStartupInfo.kaTTB);
  TrPrintf8("MPDB @ PA %08X, VMA %08X, %u pages\n", kiStartupInfo.paMPDB, kiStartupInfo.kaMPDB, kiStartupInfo.cpgMPDB);
  TrPrintf8("Page tables @ PA %08X, %u pages, %u free on last one\n", kiStartupInfo.paFirstPageTable,
	    kiStartupInfo.cpgPageTables, kiStartupInfo.ctblFreeOnLastPage);
  TrPrintf8("First free PA = %08X, first free VMA = %08X\n", kiStartupInfo.paFirstFree, kiStartupInfo.vmaFirstFree);
  TrPrintf8("EMMC clock freq = %u\n", kiStartupInfo.uiEMMCClockFreq);
  TrPrintf8("VC memory is at PHYS:%08X, %u bytes, framebuffer %ux%u\n", kiStartupInfo.paVCMem, kiStartupInfo.cbVCMem,
	    kiStartupInfo.cxFBWidth, kiStartupInfo.cyFBHeight);
  TrPrintf8("MAC address %02X:%02X:%02X:%02X:%02X:%02X\n", kiStartupInfo.abMACAddress[0],
	    kiStartupInfo.abMACAddress[1], kiStartupInfo.abMACAddress[2], kiStartupInfo.abMACAddress[3],
	    kiStartupInfo.abMACAddress[4], kiStartupInfo.abMACAddress[5]);
}

/*---------------------
 * System startup code
 *---------------------
 */

static DECLARE_INIT_STRING8_CONST(banner, "--- COMROGUE %d.%02d\n- KiSystemStartup\n");

/*
 * Starts the COMROGUE Operating System.
 *
 * Parameters:
 * None.
 *
 * Returns:
 * DOES NOT RETURN!
 *
 * Side effects:
 * Uses the information in kiStartupInfo.
 */
SEG_INIT_CODE void KiSystemStartup(void)
{
  TrPrintf8(banner, 0, 0);
  dump_startup_info();
  TrInfiniBlink();
}
