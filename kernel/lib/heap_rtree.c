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
#include <comrogue/str.h>
#include <comrogue/intlib.h>
#include <comrogue/objectbase.h>
#include <comrogue/scode.h>
#include <comrogue/stdobj.h>
#include <comrogue/mutex.h>
#include "heap_internals.h"

#ifdef _H_THIS_FILE
#undef _H_THIS_FILE
_DECLARE_H_THIS_FILE
#endif

/*---------------------------------
 * Radix tree management functions
 *---------------------------------
 */

/*
 * Allocate a new radix tree structure and its root node.
 *
 * Parameters:
 * - phd = Pointer to HEAPDATA block.
 * - cBits = Number of bits of key information to store in the tree.
 *
 * Returns:
 * - NULL = Allocation failed.
 * - Other = Pointer to the new tree structure.
 */
PMEMRTREE _HeapRTreeNew(PHEAPDATA phd, UINT32 cBits)
{
  PMEMRTREE rc;           /* return from this function */
  UINT32 cBitsPerLevel;   /* number of bits per level of the tree */
  UINT32 uiHeight;        /* tree height */
  SIZE_T cb;              /* number of bytes for allocation */
  register UINT32 i;      /* loop counter */

  /* Compute the number of bits per level and the height of the tree. */
  cBitsPerLevel = IntFirstSet(_HeapPow2Ceiling(RTREE_NODESIZE / sizeof(PVOID))) - 1;
  uiHeight = cBits / cBitsPerLevel;
  if ((uiHeight * cBitsPerLevel) != cBits)
    uiHeight++;
  _H_ASSERT(phd, (uiHeight * cBitsPerLevel) >= cBits);

  /* Allocate the tree node. */
  cb = sizeof(MEMRTREE) + (sizeof(UINT32) * uiHeight);
  rc = (PMEMRTREE)_HeapBaseAlloc(phd, cb);
  if (!rc)
    return NULL;
  StrSetMem(rc, 0, cb);

  /* Allocate the mutex and fill the tree data. */
  if (FAILED(IMutexFactory_CreateMutex(phd->pMutexFactory, &(rc->pmtx))))
    return NULL;  /* just leak the allocation */
  rc->uiHeight = uiHeight;
  if ((uiHeight * cBitsPerLevel) > cBits)
    rc->auiLevel2Bits[0] = cBits % cBitsPerLevel;
  else
    rc->auiLevel2Bits[0] = cBitsPerLevel;
  for (i = 1; i < uiHeight; i++)
    rc->auiLevel2Bits[i] = cBitsPerLevel;

  /* Allocate the tree root data. */
  cb = sizeof(PVOID) << rc->auiLevel2Bits[0];
  rc->ppRoot = (PPVOID)_HeapBaseAlloc(phd, cb);
  if (!(rc->ppRoot))
  { /* failed allocation, just leak the base node */
    IUnknown_Release(rc->pmtx);
    return NULL;
  }
  StrSetMem(rc->ppRoot, 0, cb);
  return rc;
}

/* 
 * Destroy a radix tree data structure.
 *
 * Parameters:
 * - prt = Pointer to the radix tree structure to be destroyed.
 *
 * Returns:
 * Nothing.
 */
void _HeapRTreeDestroy(PMEMRTREE prt)
{
  IUnknown_Release(prt->pmtx);  /* destroy the mutex */
  /* don't need to destroy anything else, it will get killed with the base allocations */
}

/*
 * Retrieve a value from the radix tree.  (Can either do it with the tree locked or not.)
 *
 * Parameters:
 * - phd = Pointer to HEAPDATA block.
 * - prt = Pointer to the radix tree structure to search.
 * - uiKey = Key value to look for in the tree.
 *
 * Returns:
 * - NULL = Key value not found.
 * - Other = Pointer value stashed in the tree under that key value.
 */
#define GENERATE_GET_FUNC(name)                                                 \
PVOID name (PHEAPDATA phd, PMEMRTREE prt, UINT_PTR uiKey)                       \
{                                                                               \
  PVOID rc;           /* return from this function */                           \
  UINT_PTR uiSubkey;  /* subkey value */                                        \
  UINT32 i;           /* loop counter */                                        \
  UINT32 nLeftShift;  /* number of bits to shift key left */                    \
  UINT32 uiHeight;    /* tree height value */                             	\
  UINT32 cBits;       /* number of bits at this level */                        \
  PPVOID ppNode;      /* pointer to current node */                             \
  PPVOID ppChild;     /* pointer to child node */                               \
                                                                                \
  DO_LOCK(prt->pmtx);                                                           \
  /* Walk the tree to find the right leaf node */                               \
  for (i = nLeftShift = 0, uiHeight = prt->uiHeight, ppNode = prt->ppRoot;      \
       i < (uiHeight - 1);                                                      \
       i++, nLeftShift += cBits, ppNode = ppChild)                              \
  {                                                                             \
    cBits = prt->auiLevel2Bits[i];                                              \
    uiSubkey = (uiKey << nLeftShift) >> ((1U << (LOG_PTRSIZE + 3)) - cBits);    \
    ppChild = (PPVOID)(ppNode[uiSubkey]);                                       \
    if (!ppChild)                                                               \
    {                                                                           \
      DO_UNLOCK(prt->pmtx);                                                     \
      return NULL;                                                              \
    }                                                                           \
  }                                                                             \
                                                                                \
  /* this is a leaf node, it contains values, not pointers */                   \
  cBits = prt->auiLevel2Bits[i];                                                \
  uiSubkey = (uiKey << nLeftShift) >> ((1U << (LOG_PTRSIZE + 3)) - cBits);      \
  rc = ppNode[uiSubkey];                                                        \
  DO_UNLOCK(prt->pmtx);	                                                        \
  DO_GET_VALIDATE                                                               \
  return rc;                                                                    \
}

/* Generate locked version of function */
#define DO_LOCK(mtx)    IMutex_Lock(mtx)
#define DO_UNLOCK(mtx)  IMutex_Unlock(mtx)
#define DO_GET_VALIDATE
GENERATE_GET_FUNC(_HeapRTreeGetLocked)
#undef DO_LOCK
#undef DO_UNLOCK
#undef DO_GET_VALIDATE

/* Generate unlocked version of function */
#define DO_LOCK(mtx)
#define DO_UNLOCK(mtx)
#define DO_GET_VALIDATE   _H_ASSERT(phd, _HeapRTreeGetLocked(phd, prt, uiKey) == rc);
GENERATE_GET_FUNC(_HeapRTreeGet)
#undef DO_LOCK
#undef DO_UNLOCK
#undef DO_GET_VALIDATE

/*
 * Sets a pointer value into the radix tree under a specified key.
 *
 * Parameters:
 * - phd = Pointer to HEAPDATA block.
 * - prt = Pointer to the radix tree structure to set.
 * - uiKey = Key value to set in the tree.
 * - pv = Pointer value to set in the tree.
 *
 * Returns:
 * - TRUE = Allocation failed, tree value not set.
 * - FALSE = Tree value set.
 */
BOOL _HeapRTreeSet(PHEAPDATA phd, PMEMRTREE prt, UINT_PTR uiKey, PVOID pv)
{
  UINT_PTR uiSubkey;  /* subkey value */
  UINT32 i;           /* loop counter */
  UINT32 nLeftShift;  /* number of bits to shift key left */
  UINT32 uiHeight;    /* tree height value */
  UINT32 cBits;       /* number of bits at this level */
  PPVOID ppNode;      /* pointer to current node */
  PPVOID ppChild;     /* pointer to child node */
  SIZE_T cb;          /* number of bytes to allocate */

  IMutex_Lock(prt->pmtx);
  /* Walk the tree to find the right leaf node (creating where applicable) */
  for (i = nLeftShift = 0, uiHeight = prt->uiHeight, ppNode = prt->ppRoot;
       i < (uiHeight - 1);
       i++, nLeftShift += cBits, ppNode = ppChild)
  {
    cBits = prt->auiLevel2Bits[i];
    uiSubkey = (uiKey << nLeftShift) >> ((1U << (LOG_PTRSIZE + 3)) - cBits);
    ppChild = (PPVOID)(ppNode[uiSubkey]);
    if (!ppChild)
    { /* allocate new node for next level */
      cb = sizeof(PVOID) << prt->auiLevel2Bits[i + 1];
      ppChild = (PPVOID)_HeapBaseAlloc(phd, cb);
      if (!ppChild)
      { /* allocation failed, bug out */
	IMutex_Unlock(prt->pmtx);
	return TRUE;
      }
      StrSetMem(ppChild, 0, cb);
      ppNode[uiSubkey] = (PVOID)ppChild;
    }
  }

  /* this is a leaf node, it contains values, not pointers */
  cBits = prt->auiLevel2Bits[i];
  uiSubkey = (uiKey << nLeftShift) >> ((1U << (LOG_PTRSIZE + 3)) - cBits);
  ppNode[uiSubkey] = pv;
  IMutex_Unlock(prt->pmtx);
  return FALSE;
}
