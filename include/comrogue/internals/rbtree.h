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
#ifndef __RBTREE_H_INCLUDED
#define __RBTREE_H_INCLUDED

#ifndef __ASM__

#include <comrogue/types.h>
#include <comrogue/compiler_macros.h>

/*------------------------------------------------------------------------------------------------------
 * An implementation of left-leaning red-black 2-3 trees as detailed in "Left-leaning Red-Black Trees,"
 * Robert Sedgwick, Princeton University, 2008 (http://www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf).
 * See also Java source at https://gist.github.com/741080.
 *
 * Note that we store the node color as a single bit in the low-order bit of the "right" node pointer.
 * This is do-able since, for all cases, pointers to instances of RBTREENODE will be 4-byte aligned.
 *------------------------------------------------------------------------------------------------------
 */

/* Generic key and comparison function definitions */
typedef PVOID TREEKEY;
typedef INT32 (*PFNTREECOMPARE)(TREEKEY, TREEKEY);

/* A node may be colored red or black. */
#define RED    TRUE
#define BLACK  FALSE

/* The basic tree node. */
typedef struct tagRBTREENODE {
  struct tagRBTREENODE *ptnLeft;  /* pointer to left child */
  UINT_PTR ptnRightColor;         /* pointer to right child AND color stored in low-order bit */
} RBTREENODE, *PRBTREENODE;

/* Tree node macros, mostly to access either color or pointer or both from the pRightColor field */
#define rbtNodeRight(ptn)  ((PRBTREENODE)((ptn)->ptnRightColor & ~1))
#define rbtNodeColor(ptn)  ((ptn)->ptnRightColor & 1)
#define rbtIsRed(ptn)      ((ptn) ? rbtNodeColor(ptn) : FALSE)
#define rbtSetNodeRight(ptn, ptnR) \
  do { (ptn)->ptnRightColor = (((UINT_PTR)(ptnR)) & ~1) | ((ptn)->ptnRightColor & 1); } while (0)
#define rbtSetNodeColor(ptn, clr) \
        do { (ptn)->ptnRightColor = ((ptn)->ptnRightColor & ~1) | ((clr) ? 1 : 0); } while (0)
#define rbtToggleColor(ptn) do { if (ptn) (ptn)->ptnRightColor ^= 1; } while (0)
#define rbtInitNode(ptn, ptnL, ptnR, clr) \
  do { (ptn)->ptnLeft = (ptnL); (ptn)->ptnRightColor = (((UINT_PTR)(ptnR)) & ~1) | ((clr) ? 1 : 0); } while (0)
#define rbtNewNode(ptn) rbtInitNode(ptn, NULL, NULL, RED)

/* Accessor functions common to tree instances. */
typedef TREEKEY (*PFNGETTREEKEY)(PVOID);
typedef PRBTREENODE (*PFNGETTREENODEPTR)(PVOID);
typedef PVOID (*PFNGETFROMTREENODEPTR)(PRBTREENODE);

/* The head-of-tree structure. */
typedef struct tagRBTREE {
  PFNTREECOMPARE pfnTreeCompare;            /* pointer to comparison function */
  PFNGETTREEKEY pfnGetTreeKey;              /* pointer to key-retrieval function */
  PFNGETTREENODEPTR pfnGetNodePtr;          /* pointer to node-pointer retrieval function */
  PFNGETFROMTREENODEPTR pfnGetFromNodePtr;  /* pointer to func to get the tree node from node pointer */
  PRBTREENODE ptnRoot;                      /* pointer to root of tree */
} RBTREE, *PRBTREE;

/* Tree macros. */
#define rbtInitTree(ptree, pfnCompare, pfnGetKey, pfnToNodePtr, pfnFromNodePtr)	\
  do { (ptree)->pfnTreeCompare = (pfnCompare); (ptree)->pfnGetTreeKey = (pfnGetKey); \
       (ptree)->pfnGetNodePtr = (pfnToNodePtr); (ptree)->pfnGetFromNodePtr = (pfnFromNodePtr); \
       (ptree)->ptnRoot = NULL; } while (0)
#define rbtIsEmpty(ptree)  MAKEBOOL(!((ptree)->ptnRoot))

/* Type of function used by RbtWalk. */
typedef BOOL (*PFNRBTWALK)(PRBTREE, PVOID, PVOID);

/* Function prototypes. */
CDECL_BEGIN

extern INT32 RbtStdCompareByValue(TREEKEY k1, TREEKEY k2);
extern void RbtInsert(PRBTREE ptree, PVOID ptnNew);
extern PVOID RbtFind(PRBTREE ptree, TREEKEY key);
extern PVOID RbtFindPredecessor(PRBTREE ptree, TREEKEY key);
extern PVOID RbtFindSuccessor(PRBTREE ptree, TREEKEY key);
extern PVOID RbtFindMin(PRBTREE ptree);
extern void RbtDelete(PRBTREE ptree, TREEKEY key);
extern BOOL RbtWalk(PRBTREE ptree, PFNRBTWALK pfnWalk, PVOID pData);

CDECL_END

#endif /* __ASM__ */

#endif /* __RBTREE_H_INCLUDED */
