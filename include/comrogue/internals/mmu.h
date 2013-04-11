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
#ifndef __MMU_H_INCLUDED
#define __MMU_H_INCLUDED

#ifdef __COMROGUE_INTERNALS__

/*----------------------------------------------
 * BCM2835 ARM Memory Management Unit constants
 *----------------------------------------------
 */

/* Memory system constants */
#define SYS_PAGE_SIZE       4096          /* standard page size for normal page */
#define SYS_PAGE_BITS       12            /* log2(SYS_PAGE_SIZE), number of bits in a page address */
#define SYS_TTB0_SIZE       8192          /* TTB0 must be located on this boundary and is this size */
#define SYS_TTB1_SIZE       16384         /* TTB1 must be located on this boundary and is this size */
#define SYS_TTB1_ENTRIES    4096          /* SYS_TTB1_SIZE/4, number of entries in TTB1 */
#define SYS_TTB_BITS        12            /* log2(SYS_TTB1_SIZE/4), number of bits in a TTB address */
#define SYS_SEC_SIZE        1048576       /* standard section size */
#define SYS_SEC_PAGES       256           /* SYS_SEC_SIZE/SYS_PAGE_SIZE, number of pages equivalent to a section */
#define SYS_PGTBL_SIZE      1024          /* page tables must be located on this boundary and are this size */
#define SYS_PGTBL_BITS      8             /* log2(SYS_PGTBL_SIZE/4), number of bits in a page table address */
#define SYS_PGTBL_ENTRIES   256           /* SYS_PGTBL_SIZE/4, number of entries in a page table */

/* Section descriptor bits */
#define TTBSEC_PXN          0x00000001    /* Privileged Execute-Never */
#define TTBSEC_ALWAYS       0x00000002    /* this bit must always be set for a section */
#define TTBSEC_B            0x00000004    /* memory region attribute bit */
#define TTBSEC_C            0x00000008    /* memory region attribute bit */
#define TTBSEC_XN           0x00000010    /* Execute-Never */
#define TTBSEC_DOM_MASK     0x000001E0    /* domain indicator */
#define TTBSEC_P            0x00000200    /* ECC enabled (not supported) */
#define TTBSEC_AP           0x00000C00    /* access permission bits */
#define TTBSEC_TEX          0x00007000    /* memory type flags */
#define TTBSEC_APX          0x00008000    /* access permission extended */
#define TTBSEC_S            0x00010000    /* Shared */
#define TTBSEC_NG           0x00020000    /* Not Global */
#define TTBSEC_SUPER        0x00040000    /* Set for supersections */
#define TTBSEC_NS           0x00080000    /* Not Secure */
#define TTBSEC_ALLFLAGS     0x000FFFFF    /* "all flags" mask */
#define TTBSEC_BASE         0xFFF00000    /* section base address mask */
#define TTBSEC_SBASE        0xFF000000    /* supersection base address mask */
#define TTBSEC_SBASEHI      0x00F00000    /* supersection high base address mask */

/* AP bits for the standard access control model */
#define TTBSEC_AP00         0x00000000    /* no access */
#define TTBSEC_AP01         0x00000400    /* supervisor only access */
#define TTBSEC_AP10         0x00000800    /* user read-only access */
#define TTBSEC_AP11         0x00000C00    /* user read-write access */

/* AP bits for the simplified access control model */
#define TTBSEC_ACCESS       0x00000400    /* Accessed bit */
#define TTBSEC_AP_PL0       0x00000800    /* enable access at PL0 */
#define TTBSEC_AP_RO        TTBSEC_APX    /* read-only access */

/* Page table descriptor bits */
#define TTBPGTBL_ALWAYS     0x00000001    /* bottom-two bits are always this */
#define TTBPGTBL_PXN        0x00000004    /* Privileged Execute-Never */
#define TTBPGTBL_NS         0x00000008    /* Not Secure */
#define TTBPGTBL_DOM_MASK   0x000001E0    /* domain indicator */
#define TTBPGTBL_P          0x00000200    /* ECC enabled (not supported) */
#define TTBPGTBL_ALLFLAGS   0x000003FF    /* "all flags" mask */
#define TTBPGTBL_BASE       0xFFFFFC00    /* page table base address mask */

/* Bits to query the type of TTB entry we're looking at */
#define TTBQUERY_MASK       0x00000003    /* bits we can query */
#define TTBQUERY_FAULT      0x00000000    /* indicates a fault */
#define TTBQUERY_PGTBL      0x00000001    /* indicates a page table */
#define TTBQUERY_SEC        0x00000002    /* indicates a section */
#define TTBQUERY_PXNSEC     0x00000003    /* indicates a section with PXN (or reserved) */

/* Small page table entry bits */
#define PGTBLSM_XN          0x00000001    /* Execute-Never */
#define PGTBLSM_ALWAYS      0x00000002    /* this bit must always be set for a page table entry */
#define PGTBLSM_B           0x00000004    /* memory region attribute bit */
#define PGTBLSM_C           0x00000008    /* memory region attribute bit */
#define PGTBLSM_AP          0x00000030    /* access permission bits */
#define PGTBLSM_TEX         0x000001C0    /* memory type flags */
#define PGTBLSM_APX         0x00000200    /* access permission extended */
#define PGTBLSM_S           0x00000400    /* Shared */
#define PGTBLSM_NG          0x00000800    /* Not Global */
#define PGTBLSM_PAGE        0xFFFFF000    /* page base address mask */

/* AP bits for the standard access control model */
#define PGTBLSM_AP00        0x00000000    /* no access */
#define PGTBLSM_AP01        0x00000010    /* supervisor only access */
#define PGTBLSM_AP10        0x00000020    /* user read-only access */
#define PGTBLSM_AP11        0x00000030    /* user read-write access */

/* Bits to query the type of page table entry we're looking at */
#define PGQUERY_MASK        0x00000003    /* bits we can query */
#define PGQUERY_FAULT       0x00000000    /* indicates a fault */
#define PGQUERY_LG          0x00000001    /* large page (64K) */
#define PGQUERY_SM          0x00000002    /* small page (4K) */
#define PGQUERY_SM_XN       0x00000003    /* small page with Execute-Never set */

#ifndef __ASM__

/*-------------------------------------------------------
 * Data structures defining the TTB and page table data.
 *-------------------------------------------------------
 */

/* TTB fault descriptor */
typedef struct tagTTBFAULT {
  unsigned always0 : 2;            /* bits are always 0 for a fault entry */
  unsigned ignored : 30;           /* ignored (COMROGUE may define these later) */
} TTBFAULT, *PTTBFAULT;

/* TTB page table descriptor */
typedef struct tagTTBPGTBL {
  unsigned always01 : 2;           /* bits are always 01 for a page table */
  unsigned pxn : 1;                /* Privileged Execute-Never bit */
  unsigned ns : 1;                 /* Not Secure bit */
  unsigned always0 : 1;            /* bit is always 0 for a page table */
  unsigned domain : 4;             /* protection domain */
  unsigned p : 1;                  /* not supported, should be 0 */
  unsigned baseaddr : 22;          /* upper 22 bits of base address of page table */
} TTBPGTBL, *PTTBPGTBL;

/* TTB section descriptor */
typedef struct tagTTBSEC {
  unsigned pxn : 1;                /* Privileged Execute-Never bit */
  unsigned always1: 1;             /* bit is always 1 for a section */
  unsigned b : 1;                  /* attribute bit ("Buffered") */
  unsigned c : 1;                  /* attribute bit ("Cached") */
  unsigned xn : 1;                 /* Execute-Never bit */
  unsigned domain : 4;             /* protection domain */
  unsigned p : 1;                  /* not supported, should be 0 */
  unsigned ap : 2;                 /* access permissions */
  unsigned tex : 3;                /* memory type flags */
  unsigned ap2 : 1;                /* access permission extension */
  unsigned s : 1;                  /* Shared bit */
  unsigned ng : 1;                 /* Not Global bit */
  unsigned always0 : 1;            /* bit is always 0 for a section */
  unsigned ns : 1;                 /* Not Secure bit */
  unsigned baseaddr : 12;          /* upper 12 bits of base address of section */
} TTBSEC, *PTTBSEC;

/* Single TTB entry descriptor */
typedef union tagTTB {
  UINT32 data;                     /* raw data for entry */
  TTBFAULT fault;                  /* "fault" data */
  TTBPGTBL pgtbl;                  /* page table data */
  TTBSEC sec;                      /* 1Mb section data */
} TTB, *PTTB;

/* page table descriptor for a fault entry */
typedef struct tagPGTBLFAULT {
  unsigned always0 : 2;            /* bits are always 0 for a fault entry */
  unsigned ignored : 30;           /* ignored (COMROGUE may define these later) */
} PGTBLFAULT, *PPGTBLFAULT;

/* page table descriptor for regular 4K pages */
typedef struct tagPGTBLSM {
  unsigned xn : 1;                 /* Execute Never */
  unsigned always1 : 1;            /* always 1 for a 4k small page */
  unsigned b : 1;                  /* attribute bit ("Buffered") */
  unsigned c : 1;                  /* attribute bit ("Cached") */
  unsigned ap : 2;                 /* access permissions */
  unsigned tex : 3;                /* memory type flags */
  unsigned apx : 1;                /* access permission extension */
  unsigned s : 1;                  /* Shared bit */
  unsigned ng : 1;                 /* Not Global bit */
  unsigned pgaddr : 20;            /* upper 20 bits of base address of page */
} PGTBLSM, *PPGTBLSM;

/* single page table entry */
typedef union tagPGTBL {
  UINT32 data;                     /* raw data for entry */
  PGTBLFAULT fault;                /* "fault" data */
  PGTBLSM pg;                      /* small page descriptor */
} PGTBL, *PPGTBL;

/* page table auxiliary entry */
typedef union tagPGAUX {
  UINT32 data;                     /* raw data for entry */
  /* TODO */
} PGAUX, *PPGAUX;

/* complete structure of a page table, hardware + auxiliary */
typedef struct tagPAGETAB {
  PGTBL pgtbl[SYS_PGTBL_ENTRIES];  /* hardware page table entries */
  PGAUX pgaux[SYS_PGTBL_ENTRIES];  /* auxiliary page table entries */
} PAGETAB, *PPAGETAB;

/* VMA index macros */
#define mmVMA2TTBIndex(vma)     (((vma) >> (SYS_PAGE_BITS + SYS_PGTBL_BITS)) & ((1 << SYS_TTB_BITS) - 1))
#define mmVMA2PGTBLIndex(vma)   (((vma) >> SYS_PAGE_BITS) & ((1 << SYS_PGTBL_BITS) - 1))

/*
 * Data structures for the Master Page Database.
 */

/* internal structure of a MPDB entry */
typedef struct tagMPDB1 {
  unsigned next : 20;              /* index of "next" entry in list */
  unsigned tag : 12;               /* page tag */
} MPDB1;

/* The MPDB entry itself. */
typedef union tagMPDB {
  UINT32 raw;                      /* raw data */
  MPDB1 d;                         /* structured data */
} MPDB, *PMPDB;

#endif /* __ASM__ */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __MMU_H_INCLUDED */
