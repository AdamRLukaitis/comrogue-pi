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
#ifndef __DLIST_H_INCLUDED
#define __DLIST_H_INCLUDED

#ifndef __ASM__

#include <comrogue/types.h>

/*----------------------------------------
 * Doubly-linked circular list operations
 *----------------------------------------
 */

/* Declare a field in a structure to make it part of a doubly-linked list. */
#define DLIST_FIELD_DECLARE(type, fieldname) \
  struct { type *pDLNext; type *pDLPrev; } fieldname

/* Access "next" and "previous" elements. */
#define dlistNext(ptr, fieldname)  ((ptr)->fieldname.pDLNext)
#define dlistPrev(ptr, fieldname)  ((ptr)->fieldname.pDLPrev)

/* Initialize a node to be a list of only itself. */
#define dlistNodeInit(ptr, fieldname) \
  do { dlistNext(ptr, fieldname) = dlistPrev(ptr, fieldname) = (ptr); } while (0)

/* Insert a node before another in a list. */
#define dlistInsertBefore(ptrInsBefore, ptrNew, fieldname) \
  do { dlistPrev(ptrNew, fieldname) = dlistPrev(ptrInsBefore, fieldname); \
       dlistNext(ptrNew, fieldname) = (ptrInsBefore); \
       dlistNext(dlistPrev(ptrInsBefore, fieldname), fieldname) = (ptrNew); \
       dlistPrev(ptrInsBefore) = (ptrNew); } while (0)

/* Insert a node after another in a list. */
#define dlistInsertAfter(ptrInsAfter, ptrNew, fieldname) \
  do { dlistNext(ptrNew, fieldname) = dlistNext(ptrInsAfter, fieldname); \
       dlistPrev(ptrNew, fieldname) = (ptrInsAfter); \
       dlistPrev(dlistNext(ptrInsAfter, fieldname), fieldname) = (ptrNew); \
       dlistNext(ptrInsAfter, fieldname) = (ptrNew); } while (0)

/* Join two lists together. */
#define dlistJoin(ptrA, ptrB, fieldname) \
  do { void *tmp; \
       dlistNext(dlistPrev(ptrA, fieldname), fieldname) = (ptrB); \
       dlistNext(dlistPrev(ptrB, fieldname), fieldname) = (ptrA); \
       tmp = dlistPrev(ptrA, fieldname); dlistPrev(ptrA, fieldname) = dlistPrev(ptrB, fieldname); \
       dlistPrev(ptrB, fieldname) = tmp; } while (0)

/* Split a list into two lists. (Same code as joining) */
#define dlistSplit(ptrA, ptrB, fieldname)  dlistJoin(ptrA, ptrB, fieldname)

/* Remove an element from a list. */
#define dlistRemove(ptr, fieldname) \
  do { dlistPrev(dlistNext(ptr, fieldname), fieldname) = dlistPrev(ptr, fieldname); \
       dlistNext(dlistPrev(ptr, fieldname), fieldname) = dlistNext(ptr, fieldname); \
       dlistNodeInit(ptr, fieldname); } while (0)

/* Declare a loop to iterate through a list. */
#define dlistForEach(var, ptr, fieldname) \
  for ((var) = (ptr); (var); (var) = ((dlistNext(var, fieldname) != ptr) ? dlistNext(var, fieldname) : NULL))

/* Declare a loop to iterate through a list in reverse order. */
#define dlistForEachReverse(var, ptr, fieldname) \
  for ((var) = ((ptr) ? dlistPrev(ptr, fieldname) : NULL); (var); \
       (var) = (((var) != (ptr)) ? dlistPrev(var, fieldname) : NULL))

/*------------------------------------
 * List header and operations thereon
 *------------------------------------
 */

/* Declaration and static initializers for a list header field. */
#define DLIST_HEAD_DECLARE(type, headfieldname) \
  struct { type *pDLHFirst; } headfieldname
#define DLIST_HEAD_INIT { NULL }
#define DLIST_HEAD_NAMED_INIT(headfieldname) \
  .headfieldname = DLIST_HEAD_INIT

/* Pointers to the first and last elements of a list. */
#define dlistFirst(hptr, fieldname)  ((hptr)->pDLHFirst)
#define dlistLast(hptr, fieldname)   (dlistFirst(hptr) ? dlistPrev(dlistFirst(hptr), fieldname) : NULL)

/* Reinitializer for a list. */
#define dlistListInit(hptr, fieldname) do { dlistFirst(hptr, fieldname) = NULL; } while (0)

/* Move forward and back in a list, returning NULL if we go past the "end" in either case. */
#define dlistListNext(hptr, ptr, fieldname) \
  ((dlistLast(hptr, fieldname) != (ptr)) ? dlistNext(ptr, fieldname) : NULL)
#define dlistListPrev(hptr, ptr, fieldname) \
  ((dlistFirst(hptr, fieldname) != (ptr)) ? dlistPrev(ptr, fieldname) : NULL)

/* Insert an element before another one in a list. */
#define dlistListInsertBefore(hptr, ptrInsBefore, ptrNew, fieldname) \
  do { dlistInsertBefore(ptrInsBefore, ptrNew, fieldname); \
    if (dlistFirst(hptr, fieldname) == (ptrInsBefore)) { dlistFirst(hptr, fieldname) = (ptrNew); } } while (0)

/* Insert an element after another one in a list. */
#define dlistListInsertAfter(hptr, ptrInsAfter, ptrNew, fieldname) \
  dlistInsertAfter(ptrInsAfter, ptrNew, fieldname)

/* Insert an element as the first one in a list. */
#define dlistListInsertFirst(hptr, ptrNew, fieldname) \
  do { if (dlistFirst(hptr, fieldname)) { dlistInsertBefore(dlistFirst(hptr, fieldname), ptrNew, fieldname); } \
       dlistFirst(hptr, fieldname) = (ptrNew); } while (0)

/* Insert an element as the last one in a list. */
#define dlistListInsertLast(hptr, ptrNew, fieldname) \
  do { if (dlistFirst(hptr, fieldname)) { dlistInsertBefore(dlistFirst(hptr, fieldname), ptrNew, fieldname); } \
       dlistFirst(hptr, fieldname) = dlistNext(ptrNew, fieldname); } while (0)

/* Remove an element from a list. */
#define dlistListRemove(hptr, ptr, fieldname) \
  do { if (dlistFirst(hptr, fieldname) == (ptr)) { dlistFirst(hptr, fieldname) = dlistNext(ptr, fieldname); } \
       if (dlistFirst(hptr, fieldname) != (ptr)) { dlistRemove(ptr, fieldname); } \
       else { dlistListInit(hptr, fieldname); } } while (0)

/* Remove the first element from a list. */
#define dlistListRemoveFirst(hptr, type, fieldname) \
  do { type *tmp = dlistFirst(hptr, fieldname); dlistListRemove(hptr, tmp, fieldname); } while (0)

/* Remove the last element from a list. */
#define dlistListRemoveLast(hptr, type, fieldname) \
  do { type *tmp = dlistLast(hptr, fieldname); dlistListRemove(hptr, tmp, fieldname); } while (0)

/* Declare a loop to iterate through a list. */
#define dlistListForEach(var, hptr, fieldname) dlistForEach(var, dlistFirst(hptr, fieldname), fieldname)

/* Declare a loop to iterate through a list in reverse order. */
#define dlistListForEachReverse(var, hptr, fieldname) dlistForEachReverse(var, dlistFirst(hptr, fieldname), fieldname)

#endif /* __ASM__ */

#endif /* __DLIST_H_INCLUDED */
