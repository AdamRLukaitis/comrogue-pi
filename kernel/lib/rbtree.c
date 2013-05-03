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
#include <comrogue/types.h>
#include <comrogue/internals/rbtree.h>
#include <comrogue/internals/trace.h>

#ifdef THIS_FILE
#undef THIS_FILE
DECLARE_THIS_FILE
#endif

/*------------------------------------------------------------------------------------------------------
 * An implementation of left-leaning red-black 2-3 trees as detailed in "Left-leaning Red-Black Trees,"
 * Robert Sedgwick, Princeton University, 2008 (http://www.cs.princeton.edu/~rs/talks/LLRB/LLRB.pdf).
 * See also Java source at https://gist.github.com/741080.
 *
 * Note that we store the node color as a single bit in the low-order bit of the "right" node pointer.
 * This is do-able since, for all cases, pointers to instances of RBTREENODE will be 4-byte aligned.
 *------------------------------------------------------------------------------------------------------
 */

/*
 * A standard compare-by-value function for tree keys, which compares the numeric values of the keys as
 * unsigned integers.
 *
 * Parameters:
 * - k1 = First key value to compare.
 * - k2 = Second key value to compare.
 *
 * Returns:
 * 0 if the keys are equal; an integer less than 0 if k1 is less than k2; an integer greater than 0 if
 * k1 is greater than k2.
 */
INT32 RbtStdCompareByValue(TREEKEY k1, TREEKEY k2)
{
  if (k1 == k2)
    return 0;
  if (((UINT_PTR)k1) < ((UINT_PTR)k2))
    return -1;
  return 1;
}

/*
 * Rotates a subtree "leftward," so that the root node of the tree is the former root node's right child.
 * The new root node inherits the former root node's color, and the former root node turns red.
 *
 * Parameters:
 * - ptn = Pointer to the root node of the subtree.
 *
 * Returns:
 * Pointer to the new root node of the subtree after the rotation.
 */
static PRBTREENODE rotate_left(PRBTREENODE ptn)
{
  register PRBTREENODE ptnNewRoot = rbtNodeRight(ptn);
  ASSERT(ptnNewRoot);
  rbtSetNodeRight(ptn, ptnNewRoot->ptnLeft);
  ptnNewRoot->ptnLeft = ptn;
  rbtSetNodeColor(ptnNewRoot, rbtNodeColor(ptn));
  rbtSetNodeColor(ptn, RED);
  return ptnNewRoot;
}

/*
 * Rotates a subtree "rightward," so that the root node of the tree is the former root node's left child.
 * The new root node inherits the former root node's color, and the former root node turns red.
 *
 * Parameters:
 * - ptn = Pointer to the root node of the subtree.
 *
 * Returns:
 * Pointer to the new root node of the subtree after the rotation.
 */
static PRBTREENODE rotate_right(PRBTREENODE ptn)
{
  register PRBTREENODE ptnNewRoot = ptn->ptnLeft;
  ASSERT(ptnNewRoot);
  ptn->ptnLeft = rbtNodeRight(ptnNewRoot);
  rbtSetNodeRight(ptnNewRoot, ptn);
  rbtSetNodeColor(ptnNewRoot, rbtNodeColor(ptn));
  rbtSetNodeColor(ptn, RED);
  return ptnNewRoot;
}

/*
 * Flips the color of the specified node and both its immediate children.
 *
 * Parameters:
 * - ptn = Pointer to the node to be color-flipped.
 *
 * Returns:
 * Nothing.
 */
static void color_flip(PRBTREENODE ptn)
{
  rbtToggleColor(ptn);
  rbtToggleColor(ptn->ptnLeft);
  rbtToggleColor(rbtNodeRight(ptn));
}

/*
 * Fixes up the given subtree after an insertion or deletion, to ensure that it maintains the invariants
 * that no two consecutive links in the tree may be red, and that all red links must lean left.
 *
 * Parameters:
 * - ptn = Pointer to the root node of the subtree to be fixed up.
 *
 * Returns:
 * Pointer to the new root node of the subtree after fixup is performed.
 */
static PRBTREENODE fix_up(PRBTREENODE ptn)
{
  if (rbtIsRed(rbtNodeRight(ptn)) && !rbtIsRed(ptn->ptnLeft))
    ptn = rotate_left(ptn);
  if (rbtIsRed(ptn->ptnLeft) && rbtIsRed(ptn->ptnLeft->ptnLeft))
    ptn = rotate_right(ptn);
  if (rbtIsRed(ptn->ptnLeft) && rbtIsRed(rbtNodeRight(ptn)))
    color_flip(ptn);
  return ptn;
}

/*
 * Inserts a new node under the current subtree.  An O(log n) operation.
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure, containing the compare function.
 * - ptnCurrent = Pointer to the current subtree we're inserting into.
 * - ptnNew = Pointer to the new tree node to be inserted.  This node must have been initialized with
 *            the rbtInitNode macro to contain a key, NULL left and right pointers, and be red.  It is
 *            assumed that the node's key does NOT already exist in the tree.
 *
 * Returns:
 * The pointer to the new subtree after the insertion is performed.
 *
 * N.B.:
 * This function is recursive; however, the nature of the tree guarantees that the stack space consumed
 * by its stack frames will be O(log n).
 */
static PRBTREENODE insert_under(PRBTREE ptree, PRBTREENODE ptnCurrent, PRBTREENODE ptnNew)
{
  register int cmp;  /* compare result */

  if (!ptnCurrent)
    return ptnNew;  /* degenerate case */
  cmp = (*(ptree->pfnTreeCompare))(ptnNew->treekey, ptnCurrent->treekey);
  ASSERT(cmp != 0);
  if (cmp < 0)
    ptnCurrent->ptnLeft = insert_under(ptree, ptnCurrent->ptnLeft, ptnNew);
  else
    rbtSetNodeRight(ptnCurrent, insert_under(ptree, rbtNodeRight(ptnCurrent), ptnNew));
  return fix_up(ptnCurrent);
}

/*
 * Inserts a new node into the tree.  An O(log n) operation.
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure.
 * - ptnNew = Pointer to the new tree node to be inserted.  This node must have been initialized with
 *            the rbtInitNode macro to contain a key, NULL left and right pointers, and be red.  It is
 *            assumed that the node's key does NOT already exist in the tree.
 *
 * Returns:
 * Nothing.
 */
void RbtInsert(PRBTREE ptree, PRBTREENODE ptnNew)
{
  ptree->ptnRoot = insert_under(ptree, ptree->ptnRoot, ptnNew);
  rbtSetNodeColor(ptree->ptnRoot, BLACK);
}

/*
 * Locates a node in the tree by key.  An O(log n) operation.
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure.
 * - key = Key value to be looked up.
 *
 * Returns:
 * Pointer to the node where the key is found, or NULL if not found.
 */
PRBTREENODE RbtFind(PRBTREE ptree, TREEKEY key)
{
  register PRBTREENODE ptn = ptree->ptnRoot; /* current node */
  register int cmp;  /* compare result */

  while (ptn)
  {
    cmp = (*(ptree->pfnTreeCompare))(key, ptn->treekey);
    if (cmp == 0)
      break;  /* found */
    else if (cmp < 0)
      ptn = ptn->ptnLeft;
    else
      ptn = rbtNodeRight(ptn);
  }

  return ptn;
}

/*
 * Given a key, returns either the node that matches the key, if the key is in the tree, or the node
 * that has a key that most immediately precedes the supplied key.  An O(log n) operation.
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure.
 * - key = Key value to be looked up.
 *
 * Returns:
 * Pointer to the node where the key is found, or pointer to the predecessor node, or NULL if the key
 * is less than every key in the tree and hence has no predecessor.
 */
PRBTREENODE RbtFindPredecessor(PRBTREE ptree, TREEKEY key)
{
  register PRBTREENODE ptn = ptree->ptnRoot; /* current node */
  register int cmp;  /* compare result */

  while (ptn)
  {
    cmp = (*(ptree->pfnTreeCompare))(key, ptn->treekey);
    if (cmp == 0)
      break;  /* found */
    else if (cmp > 0)
    {
      if (rbtNodeRight(ptn))
	ptn = rbtNodeRight(ptn);
      else
	break;  /* found predecessor */
    }
    else
      ptn = ptn->ptnLeft;
  }
  return ptn;
}

/*
 * Given a key, returns either the node that matches the key, if the key is in the tree, or the node
 * that has a key that most immediately succeeds the supplied key.  An O(log n) operation.
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure.
 * - key = Key value to be looked up.
 *
 * Returns:
 * Pointer to the node where the key is found, or pointer to the successor node, or NULL if the key
 * is greater than every key in the tree and hence has no successor.
 */
PRBTREENODE RbtFindSuccessor(PRBTREE ptree, TREEKEY key)
{
  register PRBTREENODE ptn = ptree->ptnRoot; /* current node */
  register int cmp;  /* compare result */

  while (ptn)
  {
    cmp = (*(ptree->pfnTreeCompare))(key, ptn->treekey);
    if (cmp == 0)
      break;  /* found */
    else if (cmp < 0)
    {
      if (ptn->ptnLeft)
	ptn = ptn->ptnLeft;
      else
	break;  /* found successor */
    }
    else
      ptn = rbtNodeRight(ptn);
  }
  return ptn;
}

/*
 * Finds the "minimum" node in the subtree (the one at the bottom end of the left spine of the subtree).
 *
 * Parameters:
 * - ptn = Pointer to the subtree to be searched.
 *
 * Returns:
 * Pointer to the leftmost node in the subtree.
 */
static PRBTREENODE find_min(PRBTREENODE ptn)
{
  while (ptn->ptnLeft)
    ptn = ptn->ptnLeft;
  return ptn;
}

/*
 * Finds the "minimum" node in the tree (the one at the bottom end of the left spine of the tree).
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure.
 * 
 * Returns:
 * Pointer to the leftmost node in the tree. If the tree has no nodes, NULL is returned.
 */
PRBTREENODE RbtFindMin(PRBTREE ptree)
{
  return ptree->ptnRoot ? find_min(ptree->ptnRoot) : NULL;
}

/*
 * Fixup required to delete the leftmost node; we maintain the invariant that either the current node
 * or its left child is red.  After a color flip, we resolve any successive reds on the right with rotations
 * and another color flip.
 *
 * Parameters:
 * - ptn = Pointer to root of subtree to be fixed up.
 *
 * Returns:
 * Pointer to root of subtree after fixup.
 */
static PRBTREENODE move_red_left(PRBTREENODE ptn)
{
  color_flip(ptn);
  if (rbtNodeRight(ptn) && rbtIsRed(rbtNodeRight(ptn)->ptnLeft))
  {
    rbtSetNodeRight(ptn, rotate_right(rbtNodeRight(ptn)));
    ptn = rotate_left(ptn);
    color_flip(ptn);
  }
  return ptn;
}

/*
 * Fixup required to delete an internal node, rotating left-leaning red links to the right.
 *
 * Parameters:
 * - ptn = Pointer to root of subtree to be fixed up.
 *
 * Returns:
 * Pointer to root of subtree after fixup.
 */
static PRBTREENODE move_red_right(PRBTREENODE ptn)
{
  color_flip(ptn);
  if (ptn->ptnLeft && rbtIsRed(ptn->ptnLeft->ptnLeft))
  {
    ptn = rotate_right(ptn);
    color_flip(ptn);
  }
  return ptn;
}

/*
 * Deletes the leftmost node in the subtree.  (Note that "deletes" means "removes from the tree." No memory
 * delete operation is actually performed.)
 *
 * Parameters:
 * - ptn = Pointer to root of subtree to have its leftmost node deleted.
 *
 * Returns:
 * Pointer to root of subtree after having the leftmost node deleted.
 *
 * N.B.:
 * This function is recursive; however, the nature of the tree guarantees that the stack space consumed
 * by its stack frames will be O(log n).
 */
static PRBTREENODE delete_min(PRBTREENODE ptn)
{
  if (!(ptn->ptnLeft))
    return rbtNodeRight(ptn);
  if (!rbtIsRed(ptn->ptnLeft) && !rbtIsRed(ptn->ptnLeft->ptnLeft))
    ptn = move_red_left(ptn);
  ptn->ptnLeft = delete_min(ptn->ptnLeft);
  return fix_up(ptn);
}

/*
 * Deletes the node in the subtree having an arbitrary key. (Note that "deletes" means "removes from the tree."
 * No memory delete operation is actually performed.) An O(log n) operation.
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure, containing the compare function.
 * - ptnCurrent = Pointer to the root of the current subtree we're deleting from.
 * - key = Key value we're deleting from the tree.  It is assumed that this key value exists in the subtree.
 *
 * Returns:
 * Pointer to the root of the subtree after the node has been deleted.
 *
 * N.B.:
 * This function is recursive; however, the nature of the tree guarantees that the stack space consumed
 * by its stack frames (and those of delete_min, where we call it) will be O(log n).
 */
static PRBTREENODE delete_from_under(PRBTREE ptree, PRBTREENODE ptnCurrent, TREEKEY key)
{
  register int cmp = (*(ptree->pfnTreeCompare))(key, ptnCurrent->treekey);
  if (cmp < 0)
  {
    /* hunt down the left subtree */
    if (!rbtIsRed(ptnCurrent->ptnLeft) && !rbtIsRed(ptnCurrent->ptnLeft->ptnLeft))
      ptnCurrent = move_red_left(ptnCurrent);
    ptnCurrent->ptnLeft = delete_from_under(ptree, ptnCurrent->ptnLeft, key);
  }
  else
  {
    if (rbtIsRed(ptnCurrent->ptnLeft))
    {
      ptnCurrent = rotate_right(ptnCurrent);
      cmp = (*(ptree->pfnTreeCompare))(key, ptnCurrent->treekey);
    }
    if ((cmp == 0) && !rbtNodeRight(ptnCurrent))
      return ptnCurrent->ptnLeft;  /* degenerate case */
    if (   !rbtIsRed(rbtNodeRight(ptnCurrent))
	&& (!rbtNodeRight(ptnCurrent) || !rbtIsRed(rbtNodeRight(ptnCurrent)->ptnLeft)))
    {
      ptnCurrent = move_red_right(ptnCurrent);
      cmp = (*(ptree->pfnTreeCompare))(key, ptnCurrent->treekey);
    }
    if (cmp == 0)
    {
      /*
       * Here we find the minimum node in the right subtree, unlink it, and link it into place in place of
       * ptnCurrent (i.e. node pointed to by ptnCurrent should no longer be referenced).  We inherit the
       * child pointers and color of ptnCurrent (minus the reference from the right-hand tree where applicable).
       */
      register PRBTREENODE ptnMin = find_min(rbtNodeRight(ptnCurrent));
      rbtSetNodeRight(ptnMin, delete_min(rbtNodeRight(ptnCurrent)));
      ptnMin->ptnLeft = ptnCurrent->ptnLeft;
      rbtSetNodeColor(ptnMin, rbtNodeColor(ptnCurrent));
      ptnCurrent = ptnMin;
    }
    else /* hunt down the right subtree */
      rbtSetNodeRight(ptnCurrent, delete_from_under(ptree, rbtNodeRight(ptnCurrent), key));
  }
  return fix_up(ptnCurrent);
}

/*
 * Deletes the node in the tree having an arbitrary key. (Note that "deletes" means "removes from the tree."
 * No memory delete operation is actually performed.) An O(log n) operation.
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure.
 * - key = Key value we're deleting from the tree.  It is assumed that this key value exists in the subtree.
 *
 * Returns:
 * Nothing.
 */
void RbtDelete(PRBTREE ptree, TREEKEY key)
{
  ptree->ptnRoot = delete_from_under(ptree, ptree->ptnRoot, key);
  if (ptree->ptnRoot)
    rbtSetNodeColor(ptree->ptnRoot, BLACK);
}

/*
 * Performs an inorder traversal of the tree rooted at the specified node. An O(n) operation.
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure.
 * - ptn = Pointer to the root of the current tree node.
 * - pfnWalk = Pointer to a function called for each tree node we encounter.  This function returns TRUE
 *             to continue the traversal or FALSE to stop it.
 * - pData = Arbitrary data pointer that gets passed to the pfnWalk function.
 *
 * Returns:
 * TRUE if the tree was entirely traversed, FALSE if the tree walk was interrupted.
 *
 * N.B.:
 * This function is recursive; however, the nature of the tree guarantees that the stack space consumed
 * by its stack frames will be O(log n).
 */
static BOOL do_walk(PRBTREE ptree, PRBTREENODE ptn, PFNRBTWALK pfnWalk, PVOID pData)
{
  register BOOL rc = TRUE;
  if (ptn->ptnLeft)
    rc = do_walk(ptree, ptn->ptnLeft, pfnWalk, pData);
  if (rc)
    rc = (*pfnWalk)(ptree, ptn, pData);
  if (rc && rbtNodeRight(ptn))
    rc = do_walk(ptree, rbtNodeRight(ptn), pfnWalk, pData);
  return rc;
}

/*
 * Performs an inorder traversal of the tree.  An O(n) operation.
 *
 * Parameters:
 * - ptree = Pointer to the tree head structure.
 * - pfnWalk = Pointer to a function called for each tree node we encounter.  This function returns TRUE
 *             to continue the traversal or FALSE to stop it.
 * - pData = Arbitrary data pointer that gets passed to the pfnWalk function.
 *
 * Returns:
 * TRUE if the tree was entirely traversed, FALSE if the tree walk was interrupted.
 */
BOOL RbtWalk(PRBTREE ptree, PFNRBTWALK pfnWalk, PVOID pData)
{
  return (ptree->ptnRoot ? do_walk(ptree, ptree->ptnRoot, pfnWalk, pData) : TRUE);
}
