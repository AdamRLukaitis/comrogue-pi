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
/*
 * This code is based on/inspired by jemalloc-3.3.1.  Please see LICENSE.jemalloc for further details.
 */
#include <comrogue/compiler_macros.h>
#include <comrogue/types.h>
#include <comrogue/objectbase.h>
#include <comrogue/scode.h>
#include <comrogue/stdobj.h>
#include <comrogue/internals/mmu.h>
#include "heap_internals.h"

/*------------------------------------------------------------------------
 * Functions driving the red-black trees that contain EXTENT_NODE objects
 *------------------------------------------------------------------------
 */

static INT32 compare_sizeaddr(PEXTENT_NODE pexnA, PEXTENT_NODE pexnB)
{
  SIZE_T szA = pexnA->sz;
  SIZE_T szB = pexnB->sz;
  INT32 rc = (szA > szB) - (szA < szB);

  if (rc == 0)
  {
    UINT_PTR addrA = (UINT_PTR)(pexnA->pv);
    UINT_PTR addrB = (UINT_PTR)(pexnB->pv);
    rc = (addrA > addrB) - (addrA < addrB);
  }
  return rc;
}

static INT32 compare_addr(PEXTENT_NODE pexnA, PEXTENT_NODE pexnB)
{
  UINT_PTR addrA = (UINT_PTR)(pexnA->pv);
  UINT_PTR addrB = (UINT_PTR)(pexnB->pv);
  return (addrA > addrB) - (addrA < addrB);
}

static PRBTREENODE get_sizeaddr_node(PEXTENT_NODE pexn)
{
  return &(pexn->rbtnSizeAddress);
}

static PRBTREENODE get_addr_node(PEXTENT_NODE pexn)
{
  return &(pexn->rbtnAddress);
}

static PEXTENT_NODE get_from_sizeaddr_node(PRBTREENODE prbtn)
{
  return (PEXTENT_NODE)(((UINT_PTR)prbtn) - OFFSETOF(EXTENT_NODE, rbtnSizeAddress));
}

static PEXTENT_NODE get_from_addr_node(PRBTREENODE prbtn)
{
  return (PEXTENT_NODE)(((UINT_PTR)prbtn) - OFFSETOF(EXTENT_NODE, rbtnAddress));
}

/*----------------------
 * Heap chunk functions
 *----------------------
 */

PVOID _HeapChunkAlloc(PHEAPDATA phd, SIZE_T sz, SIZE_T szAlignment, BOOL fBase, BOOL *pfZeroed)
{
  return NULL; /* TODO */
}

void _HeapChunkUnmap(PHEAPDATA phd, PVOID pvChunk, SIZE_T sz)
{
  /* TODO */
}

void _HeapChunkDeAlloc(PHEAPDATA phd, PVOID pvChunk, SIZE_T sz, BOOL fUnmap)
{
  /* TODO */
}

HRESULT _HeapChunkSetup(PHEAPDATA phd)
{
  rbtInitTree(&(phd->rbtExtSizeAddr), (PFNTREECOMPARE)compare_sizeaddr, (PFNGETTREEKEY)get_from_sizeaddr_node,
	      (PFNGETTREENODEPTR)get_sizeaddr_node, (PFNGETFROMTREENODEPTR)get_from_sizeaddr_node);
  rbtInitTree(&(phd->rbtExtAddr), (PFNTREECOMPARE)compare_addr, (PFNGETTREEKEY)get_from_addr_node,
	      (PFNGETTREENODEPTR)get_addr_node, (PFNGETFROMTREENODEPTR)get_from_addr_node);
  return S_OK;
}

extern void _HeapChunkShutdown(PHEAPDATA phd)
{
  /* TODO */
}
