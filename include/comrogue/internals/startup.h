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
#ifndef __STARTUP_H_INCLUDED
#define __STARTUP_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

#ifdef __COMROGUE_PRESTART__

/*------------------------------------------------
 * ATAG data passed to the kernel at startup time
 *------------------------------------------------
 */

/* The different ATAG types. */
#define ATAGTYPE_NONE                 0                /* none - end of list */
#define ATAGTYPE_CORE                 0x54410001       /* core - this one must be first */
#define ATAGTYPE_MEM                  0x54410002       /* memory */
#define ATAGTYPE_VIDEOTEXT            0x54410003       /* text-mode video */
#define ATAGTYPE_RAMDISK              0x54410004       /* RAM disk */
#define ATAGTYPE_INITRD2              0x54420005       /* compressed initial RAM disk */
#define ATAGTYPE_SERIAL               0x54410006       /* serial number */
#define ATAGTYPE_REVISION             0x54410007       /* revision */
#define ATAGTYPE_VIDEOLFB             0x54410008       /* linear framebuffer video */
#define ATAGTYPE_CMDLINE              0x54410009       /* command line */

/* Flags for the core ATAG's uiFlags member. */
#define ATAG_COREFLAG_READONLY        0x00000001       /* read only */

#define ATAG_RAMDISKFLAG_LOAD         0x00000001
#define ATAG_RAMDISKFLAG_PROMPT       0x00000002

#endif  /* __COMROGUE_PRESTART__ */

#ifndef __ASM__

#include <comrogue/types.h>

#ifdef __COMROGUE_PRESTART__

/* Header of all ATAGs. */
typedef struct tagATAG_HEADER {
  UINT32 uiSize;              /* size of the tag, in 32-bit words */
  UINT32 uiTag;               /* the tag indicator */
} ATAG_HEADER, *PATAG_HEADER;

/* Core ATAG - must be first in the list */
typedef struct tagATAG_CORE {
  ATAG_HEADER hdr;            /* header */
  UINT32 uiFlags;             /* core flags, see above */
  UINT32 uiPageSize;          /* system page size in bytes */
  UINT32 uiRootDevice;        /* root device number */
} ATAG_CORE, *PATAG_CORE;

/* Memory ATAG */
typedef struct tagATAG_MEM {
  ATAG_HEADER hdr;            /* header */
  UINT32 uiSize;              /* size of memory */
  PHYSADDR paStart;           /* physical address of start of memory */
} ATAG_MEM, *PATAG_MEM;

/* Text-mode video ATAG */
typedef struct tagATAG_VIDEOTEXT {
  ATAG_HEADER hdr;            /* header */
  BYTE bWidth;                /* display width */
  BYTE bHeight;               /* display height */
  UINT16 uiVideoPage;
  BYTE bVideoMode;
  BYTE bColumns;
  UINT16 uiEGABX;
  BYTE bLines;
  BYTE bIsVGA;
  UINT16 uiPoints;
} ATAG_VIDEOTEXT, *PATAG_VIDEOTEXT;

/* RAM disk ATAG */
typedef struct tagATAG_RAMDISK {
  ATAG_HEADER hdr;            /* header */
  UINT32 uiFlags;             /* RAM disk flags, see above */
  UINT32 uiSize;              /* size of ramdisk in kB */
  UINT32 uiStartBlock;        /* starting block of ramdisk */
} ATAG_RAMDISK, *PATAG_RAMDISK;

/* Initial ramdisk image ATAG */
typedef struct tagATAG_INITRD2 {
  ATAG_HEADER hdr;            /* header */
  PHYSADDR paStart;           /* physical address of compressed ramdisk image */
  UINT32 uiSize;              /* size of compressed ramdisk image in bytes */
} ATAG_INITRD2, *PATAG_INITRD2;

/* Serial number ATAG */
typedef struct tagATAG_SERIAL {
  ATAG_HEADER hdr;            /* header */
  UINT32 uiSerialNumberLow;   /* low word of serial number */
  UINT32 uiSerialNumberHigh;  /* high word of serial number */
} ATAG_SERIAL, *PATAG_SERIAL;

/* Revision number ATAG */
typedef struct tagATAG_REVISION {
  ATAG_HEADER hdr;            /* header */
  UINT32 uiRevision;          /* revision number */
} ATAG_REVISION, *PATAG_REVISION;

/* Linear framebuffer video ATAG */
typedef struct tagATAG_VIDEOLFB {
  ATAG_HEADER hdr;            /* header */
  UINT16 uiFrameBufferWidth;
  UINT16 uiFrameBufferHeight;
  UINT16 uiFrameBufferDepth;
  UINT16 uiFrameBufferLineLength;
  UINT32 uiFrameBufferBase;
  UINT32 uiFrameBufferSize;
  BYTE bRedSize;
  BYTE bRedPos;
  BYTE bGreenSize;
  BYTE bGreenPos;
  BYTE bBlueSize;
  BYTE bBluePos;
  BYTE bReservedSize;
  BYTE bReservedPos;
} ATAG_VIDEOLFB, *PATAG_VIDEOLFB;

/* Command line ATAG */
typedef struct tagATAG_CMDLINE {
  ATAG_HEADER hdr;            /* header */
  CHAR szCommandLine[1];      /* text of command line, null-terminated */
} ATAG_CMDLINE, *PATAG_CMDLINE;

/* Macro to advance to the next ATAG */
#define kiNextATAG(p)   ((PATAG_HEADER)(((PUINT32)(p)) + ((PATAG_HEADER)(p))->uiSize))

#endif  /* __COMROGUE_PRESTART__ */

/*----------------------------------------------------------------
 * The startup information buffer filled in by the prestart code.
 *----------------------------------------------------------------
 */

typedef struct tagSTARTUP_INFO {
  /* start of structure accessed from ASM - be careful here */
  UINT32 cb;                      /* number of bytes in this structure */
  PHYSADDR paTTB;                 /* physical address of the TTB */
  KERNADDR kaTTB;                 /* kernel address of the TTB */
  /* end of structure accessed from ASM - be careful here */
  UINT32 uiMachineType;           /* machine type indicator */
  UINT32 uiRevision;              /* board revision */
  UINT32 uiSerialNumber;          /* serial number */
  UINT32 cpgSystemTotal;          /* total number of memory pages in the system */
  UINT32 cpgSystemAvail;          /* available memory pages in the system after GPU takes its bite */
  UINT32 cpgTTBGap;               /* number of pages in the "gap" between the end of kernel and TTB */
  PHYSADDR paMPDB;                /* physical address of the Master Page Database */
  KERNADDR kaMPDB;                /* kernel address of the Master Page Database */
  UINT32 cpgMPDB;                 /* number of pages we allocated for Master Page Database */
  PHYSADDR paFirstPageTable;      /* physical address of the first page table */
  UINT32 cpgPageTables;           /* number of pages we allocated for page tables */
  UINT32 ctblFreeOnLastPage;      /* number of page tables free on last page (0 or 1) */
  PHYSADDR paFirstFree;           /* first free physical address after initial page tables */
  KERNADDR vmaFirstFree;          /* first free virtual memory address after mapped TTB */
  UINT32 uiEMMCClockFreq;         /* EMMC clock frequency */
  PHYSADDR paVCMem;               /* VideoCore memory base */
  UINT32 cbVCMem;                 /* VideoCore memory size in bytes */
  UINT16 cxFBWidth;               /* frame buffer width in pixels */
  UINT16 cyFBHeight;              /* frame buffer height in pixels */
  BYTE abMACAddress[6];           /* MAC address of the network interface */
} STARTUP_INFO, *PSTARTUP_INFO;

#endif  /* __ASM__ */

#endif  /* __COMROGUE_INTERNALS__ */

#endif  /* __STARTUP_H_INCLUDED */
