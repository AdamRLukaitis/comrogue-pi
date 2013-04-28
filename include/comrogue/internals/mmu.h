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

/*---------------------------------------------------------------------------------------------
 * BCM2835 ARM Memory Management Unit constants (and other COMROGUE-specific memory constants)
 *---------------------------------------------------------------------------------------------
 */

/* Memory system constants */
#define SYS_PAGE_SIZE       4096          /* standard page size for normal page */
#define SYS_PAGE_BITS       12            /* log2(SYS_PAGE_SIZE), number of bits in a page address */
#define SYS_TTB0_SIZE       8192          /* TTB0 must be located on this boundary and is this size */
#define SYS_TTB0_ENTRIES    2048          /* SYS_TTB0_SIZE/4, number of entries in TTB0 */
#define SYS_TTB1_SIZE       16384         /* TTB1 must be located on this boundary and is this size */
#define SYS_TTB1_ENTRIES    4096          /* SYS_TTB1_SIZE/4, number of entries in TTB1 */
#define SYS_TTB_BITS        12            /* log2(SYS_TTB1_SIZE/4), number of bits in a TTB address */
#define SYS_SEC_SIZE        1048576       /* standard section size */
#define SYS_SEC_BITS        20            /* number of bits in a section address */
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

/* Flags that are safe to alter for a section. */
#define TTBSEC_SAFEFLAGS    (TTBSEC_ALLFLAGS & ~(TTBSEC_ALWAYS | TTBSEC_SUPER))

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

/* Flags that are safe to alter for a TTB page table entry. */
#define TTBPGTBL_SAFEFLAGS  (TTBPGTBL_ALLFLAGS & ~0x03)

/* Bits to query the type of TTB entry we're looking at */
#define TTBQUERY_MASK       0x00000003    /* bits we can query */
#define TTBQUERY_FAULT      0x00000000    /* indicates a fault */
#define TTBQUERY_PGTBL      0x00000001    /* indicates a page table */
#define TTBQUERY_SEC        0x00000002    /* indicates a section */
#define TTBQUERY_PXNSEC     0x00000003    /* indicates a section with PXN (or reserved) */

/* TTB auxiliary descriptor bits */
#define TTBAUX_SACRED       0x00000001    /* sacred entry, do not deallocate */
#define TTBAUX_UNWRITEABLE  0x00000002    /* entry unwriteable */
#define TTBAUX_NOTPAGE      0x00000004    /* entry not mapped in page database */
#define TTBAUX_ALLFLAGS     0x00000007    /* "all flags" mask */

/* Flags that are safe to alter for the TTB auxiliary table. */
#define TTBAUX_SAFEFLAGS    (TTBAUX_ALLFLAGS & ~TTBAUX_NOTPAGE)

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
#define PGTBLSM_ALLFLAGS    0x00000FFF    /* "all flags" mask */
#define PGTBLSM_PAGE        0xFFFFF000    /* page base address mask */

/* Flags that are safe to alter for a page table entry. */
#define PGTBLSM_SAFEFLAGS   (PGTBLSM_ALLFLAGS & ~PGTBLSM_ALWAYS)

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

/* Page auxiliary descriptor bits */
#define PGAUX_SACRED        0x00000001    /* sacred entry, do not deallocate */
#define PGAUX_UNWRITEABLE   0x00000002    /* entry unwriteable */
#define PGAUX_NOTPAGE       0x00000004    /* entry not mapped in page database */
#define PGAUX_ALLFLAGS      0x00000007    /* "all flags" mask */

/* Flags that are safe to alter for the page auxiliary table. */
#define PGAUX_SAFEFLAGS     (PGAUX_ALLFLAGS & ~PGAUX_NOTPAGE)

/* Combinations of flags we use regularly. */
#define TTBFLAGS_LIB_CODE       TTBPGTBL_ALWAYS
#define PGTBLFLAGS_LIB_CODE     (PGTBLSM_ALWAYS | PGTBLSM_B | PGTBLSM_C | PGTBLSM_AP10)
#define PGAUXFLAGS_LIB_CODE     (PGAUX_SACRED | PGAUX_UNWRITEABLE)
#define TTBFLAGS_KERNEL_CODE    TTBPGTBL_ALWAYS
#define PGTBLFLAGS_KERNEL_CODE  (PGTBLSM_ALWAYS | PGTBLSM_B | PGTBLSM_C | PGTBLSM_AP01)
#define PGAUXFLAGS_KERNEL_CODE  (PGAUX_SACRED | PGAUX_UNWRITEABLE)
#define TTBFLAGS_KERNEL_DATA    TTBPGTBL_ALWAYS
#define PGTBLFLAGS_KERNEL_DATA  (PGTBLSM_XN | PGTBLSM_ALWAYS | PGTBLSM_B | PGTBLSM_C | PGTBLSM_AP01)
#define PGAUXFLAGS_KERNEL_DATA  PGAUX_SACRED
#define TTBFLAGS_INIT_CODE      TTBFLAGS_KERNEL_CODE
#define PGTBLFLAGS_INIT_CODE    PGTBLFLAGS_KERNEL_CODE
#define PGAUXFLAGS_INIT_CODE    PGAUX_UNWRITEABLE
#define TTBFLAGS_INIT_DATA      TTBFLAGS_KERNEL_DATA
#define PGTBLFLAGS_INIT_DATA    PGTBLFLAGS_KERNEL_DATA
#define PGAUXFLAGS_INIT_DATA    0
#define TTBFLAGS_MMIO           TTBPGTBL_ALWAYS
#define PGTBLFLAGS_MMIO         (PGTBLSM_ALWAYS | PGTBLSM_AP01)
#define PGAUXFLAGS_MMIO         (PGAUX_SACRED | PGAUX_NOTPAGE)
#define TTBAUXFLAGS_PAGETABLE   0

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

/* TTB auxiliary descriptor */
typedef struct tagTTBAUXENTRY {
  unsigned sacred : 1;             /* sacred TTB - should never be deallocated */
  unsigned unwriteable : 1;        /* entry is not writeable */
  unsigned notpage : 1;            /* entry not mapped in the page database */
  unsigned reserved : 29;          /* reserved for future allocation */
} TTBAUXENTRY, *PTTBAUXENTRY;

/* TTB auxiliary table entry */
typedef union tagTTBAUX {
  UINT32 data;                     /* raw data for entry */
  TTBAUXENTRY aux;                 /* aux entry itself */
} TTBAUX, *PTTBAUX;

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

/* page auxiliary descriptor */
typedef struct tagPGAUXENTRY {
  unsigned sacred : 1;             /* sacred page - should never be deallocated */
  unsigned unwriteable : 1;        /* entry is not writeable */
  unsigned notpage : 1;            /* entry not mapped in the page database */
  unsigned reserved : 29;          /* reserved for future allocation */
} PGAUXENTRY, *PPGAUXENTRY;

/* page table auxiliary entry */
typedef union tagPGAUX {
  UINT32 data;                     /* raw data for entry */
  PGAUXENTRY aux;                  /* the auxiliary entry itself */
} PGAUX, *PPGAUX;

/* complete structure of a page table, hardware + auxiliary */
typedef struct tagPAGETAB {
  PGTBL pgtbl[SYS_PGTBL_ENTRIES];  /* hardware page table entries */
  PGAUX pgaux[SYS_PGTBL_ENTRIES];  /* auxiliary page table entries */
} PAGETAB, *PPAGETAB;

/* VMA index macros */
#define mmVMA2TTBIndex(vma)     (((vma) >> (SYS_PAGE_BITS + SYS_PGTBL_BITS)) & ((1 << SYS_TTB_BITS) - 1))
#define mmVMA2PGTBLIndex(vma)   (((vma) >> SYS_PAGE_BITS) & ((1 << SYS_PGTBL_BITS) - 1))
#define mmIndices2VMA3(ttb, pgtbl, ofs) \
  ((((ttb) & ((1 << SYS_TTB_BITS) - 1)) << (SYS_PAGE_BITS + SYS_PGTBL_BITS)) | \
   (((pgtbl) & ((1 << SYS_PGTBL_BITS) - 1)) << SYS_PAGE_BITS) | ((ofs) & (SYS_PAGE_SIZE - 1)))

/*-----------------------------------------------
 * Data structures for the Master Page Database.
 *-----------------------------------------------
 */

/* internal structure of a MPDB entry */
typedef struct tagMPDB1 {
  PHYSADDR paPTE;                  /* PA of page table entry for the page */
  unsigned next : 20;              /* index of "next" entry in list */
  unsigned sectionmap : 1;         /* set if page is part of a section mapping */
  unsigned tag : 3;                /* page tag */
  unsigned subtag : 8;             /* page subtag */
} MPDB1;

/* MPDB tags */
#define MPDBTAG_UNKNOWN       0    /* unknown, should never be used */
#define MPDBTAG_NORMAL        1    /* normal user/free page */
#define MPDBTAG_SYSTEM        2    /* system allocation */

/* MPDB system subtags */
#define MPDBSYS_ZEROPAGE      0    /* zero page allocation */
#define MPDBSYS_LIBCODE       1    /* library code */
#define MPDBSYS_KCODE         2    /* kernel code */
#define MPDBSYS_KDATA         3    /* kernel data */
#define MPDBSYS_INIT          4    /* init code & data (to be freed later) */
#define MPDBSYS_TTB           5    /* the system TTB */
#define MPDBSYS_TTBAUX        6    /* the system auxiliary TTB table */
#define MPDBSYS_MPDB          7    /* the MPDB itself */
#define MPDBSYS_PGTBL         8    /* page tables */
#define MPDBSYS_GPU           9    /* GPU reserved pages */

/* The MPDB entry itself. */
typedef union tagMPDB {
  UINT64 raw;                      /* raw data */
  MPDB1 d;                         /* structured data */
} MPDB, *PMPDB;

/* Page index macros */
#define mmPA2PageIndex(pa)      ((pa) >> SYS_PAGE_BITS)
#define mmPageIndex2PA(ndx)     ((ndx) << SYS_PAGE_BITS)

#endif /* __ASM__ */

#endif /* __COMROGUE_INTERNALS__ */

#endif /* __MMU_H_INCLUDED */
