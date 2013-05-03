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
#define __COMROGUE_PRESTART__
#include <comrogue/types.h>
#include <comrogue/internals/mmu.h>
#include <comrogue/internals/seg.h>
#include <comrogue/internals/startup.h>

/* The startup information buffer. */
static STARTUP_INFO startup_info = {
  .cb = sizeof(STARTUP_INFO),
  .paTTB = 0,
  .kaTTB = 0,
  .cpgTTBGap = 0,
  .paTTBAux = 0,
  .kaTTBAux = 0,
  .paMPDB = 0,
  .kaMPDB = 0,
  .cpgMPDB = 0,
  .paFirstPageTable = 0,
  .cpgPageTables = 0,
  .ctblFreeOnLastPage = 0,
  .paFirstFree = 0,
  .vmaFirstFree = 0
};

/*
 * Find a variable by name on the command line.
 *
 * Parameters:
 *   - pszCmdLine = Command-line pointer.
 *   - pszVariableName = Name of variable to search for.
 *
 * Returns:
 * NULL if the variable was not found, otherwise a pointer to the first character in the command line
 * following the variable name and associated = sign (first character of the value).
 */
static PCSTR find_variable(PCSTR pszCmdLine, PCSTR pszVariableName)
{
  register PCSTR p = pszCmdLine;
  register PCSTR p1, q;
  while (*p)
  {
    for (; *p && (*p != *pszVariableName); p++) ;
    if (*p)
    {
      for (p1 = p, q = pszVariableName; *p1 && *q && (*p1 == *q); p1++, q++) ;
      if (!(*q) && *p1 == '=')
	return p1 + 1;
      p++;
    }
  }
  return NULL;
}

/*
 * Returns whether a character is a valid digit.
 *
 * Parameters:
 *   - ch = The character to be tested.
 *   - base = The base of the number being converted.
 *
 * Returns:
 * TRUE if the character is a valid digit, FALSE otherwise.
 */
static BOOL is_valid_digit(CHAR ch, UINT32 base)
{
  register UINT32 nbase;

  if (base > 10)
  {
    if ((ch >= 'a') && (ch < ('a' + base - 10)))
      return TRUE;
    if ((ch >= 'A') && (ch < ('A' + base - 10)))
      return TRUE;
  }
  nbase = (base > 10) ? 10 : base;
  if ((ch >= '0') && (ch < ('0' + nbase)))
    return TRUE;
  return FALSE;
}

/*
 * Decodes a number in a command-line parameter.  Decoding stops at the first character that is not a valid digit.
 *
 * Parameters:
 *   - pVal = Pointer to the value in the command line.
 *   - base = Base of the number to convert.  If this is 16, the decoder will skip over a "0x" or "0X" prefix
 *            of the value, if one exists.
 *
 * Returns:
 * The converted numeric value.
 */
static UINT32 decode_number(PCSTR pVal, UINT32 base)
{
  register UINT32 accum = 0;
  register UINT32 digit;

  if ((base == 16) && (*pVal == '0') && ((*(pVal + 1) == 'x') || (*(pVal + 1) == 'X')))
    pVal += 2;
  while (is_valid_digit(*pVal, base))
  {
    if (*pVal >= 'a')
      digit = (*pVal - 'a') + 10;
    else if (*pVal >= 'A')
      digit = (*pVal - 'A') + 10;
    else
      digit = (*pVal - '0');
    accum = accum * base + digit;
    pVal++;
  }
  return accum;
}

/*
 * Parses the command line passed in via the ATAGS and extracts certain values to the startup info
 * data structure.
 *
 * Parameters:
 *   - pszCmdLine = Pointer to the command line.
 *
 * Returns:
 * Nothing.
 *
 * Side effects:
 * Modifies the "startup_info" structure.
 */
static void parse_cmdline(PCSTR pszCmdLine)
{
  static DECLARE_STRING8_CONST(szFBWidth, "bcm2708_fb.fbwidth");
  static DECLARE_STRING8_CONST(szFBHeight, "bcm2708_fb.fbheight");
  static DECLARE_STRING8_CONST(szRevision, "bcm2708.boardrev");
  static DECLARE_STRING8_CONST(szSerial, "bcm2708.serial");
  static DECLARE_STRING8_CONST(szMACAddr, "smsc95xx.macaddr");
  static DECLARE_STRING8_CONST(szEMMCFreq, "sdhci-bcm2708.emmc_clock_freq");
  static DECLARE_STRING8_CONST(szVCMemBase, "vc_mem.mem_base");
  static DECLARE_STRING8_CONST(szVCMemSize, "vc_mem.mem_size");
  register PCSTR p;
  register int i;

  p = find_variable(pszCmdLine, szFBWidth);
  startup_info.cxFBWidth = (UINT16)(p ? decode_number(p, 10) : 0);
  p = find_variable(pszCmdLine, szFBHeight);
  startup_info.cyFBHeight = (UINT16)(p ? decode_number(p, 10) : 0);
  p = find_variable(pszCmdLine, szRevision);
  startup_info.uiRevision = (p ? decode_number(p, 16) : 0);
  p = find_variable(pszCmdLine, szSerial);
  startup_info.uiSerialNumber = (p ? decode_number(p, 16) : 0);
  p = find_variable(pszCmdLine, szMACAddr);
  if (p)
  {
    for (i=0; i<6; i++)
    {
      startup_info.abMACAddress[i] = (BYTE)decode_number(p, 16);
      p += 3;
    }
  }
  else
  {
    for (i=0; i<6; i++)
      startup_info.abMACAddress[i] = 0;
  }
  p = find_variable(pszCmdLine, szEMMCFreq);
  startup_info.uiEMMCClockFreq = (p ? decode_number(p, 10) : 0);
  p = find_variable(pszCmdLine, szVCMemBase);
  startup_info.paVCMem = (PHYSADDR)(p ? decode_number(p, 16) : 0);
  p = find_variable(pszCmdLine, szVCMemSize);
  startup_info.cbVCMem = (p ? decode_number(p, 16) : 0);
  startup_info.cpgSystemTotal = startup_info.cbVCMem >> SYS_PAGE_BITS;
}

/*
 * Collects startup information in an init-data buffer and returns a pointer to that buffer.
 *
 * Parameters:
 *   - always0 = Always 0.
 *   - uiMachineType = Machine type constant.
 *   - pAtags = Pointer to ATAGS data set up before kernel was started.
 *
 * Returns:
 * A pointer to the assembled STARTUP_INFO data structure.
 *
 * Side effects:
 * Modifies the "startup_info" structure, which it returns a pointer to.
 */
PSTARTUP_INFO KiCollectStartupInfo(UINT32 always0, UINT32 uiMachineType, PATAG_HEADER pAtags)
{
  /* Fill in the information we can calculate right away. */
  startup_info.uiMachineType = uiMachineType;

  /* Scan the ATAG headers to determine what other info to include. */
  while (pAtags->uiTag != ATAGTYPE_NONE)
  {
    switch (pAtags->uiTag)
    {
      case ATAGTYPE_CORE:
	/* nothing really useful in this block */
	break;

      case ATAGTYPE_MEM:
	/* fill in total number of available system memory pages */
	startup_info.cpgSystemAvail = ((PATAG_MEM)pAtags)->uiSize >> SYS_PAGE_BITS;
	break;

      case ATAGTYPE_CMDLINE:
	parse_cmdline(((PATAG_CMDLINE)pAtags)->szCommandLine);
	break;

      default:
	/* no other ATAG types are seen on Raspberry Pi */
	break;
    }

    pAtags = kiNextATAG(pAtags);
  }

  return &startup_info;
}
