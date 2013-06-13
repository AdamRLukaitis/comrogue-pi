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
#include <comrogue/types.h>
#include <comrogue/intlib.h>
#include <comrogue/str.h>
#include "heap_internals.h"

/*-----------------------------
 * Bitmap management functions
 *-----------------------------
 */

/*
 * Computes a number of groups from a number of bits.
 *
 * Parameters:
 * - cBits = The number of bits to compute.
 *
 * Returns:
 * The corresponding number of groups.
 */
static SIZE_T bits_to_groups(SIZE_T cBits)
{
  return (cBits >> LG_BITMAP_GROUP_NBITS) + !!(cBits & BITMAP_GROUP_NBITS_MASK);
}

/*
 * Initialize a bitmap info structure that is intended to contain a certain number of bits.
 *
 * Parameters:
 * - pBInfo = Pointer to info structure to be initialized.
 * - cBits = Number of bits the bitmap is to contain.
 *
 * Returns:
 * Nothing.
 */
void _HeapBitmapInfoInit(PBITMAPINFO pBInfo, SIZE_T cBits)
{
  register UINT32 i;           /* loop counter */
  register SIZE_T cGroups;     /* number of groups */

  /*assert(cBits > 0);*/
  /*assert(cBits <= (1U << LG_BITMAP_MAXBITS));*/

  /*
   * Compute the number of groups necessary to store that number of bits, and work upward through the levels
   * until we reach a level that requires only one group.
   */
  pBInfo->aLevels[0].ofsGroup = 0;
  cGroups = bits_to_groups(cBits);
  for (i = 1; cGroups > 1; i++)
  {
    /*assert(i < BITMAP_MAX_LEVELS);*/
    pBInfo->aLevels[i].ofsGroup = pBInfo->aLevels[i - 1].ofsGroup + cGroups;
    cGroups = bits_to_groups(cGroups);
  }
  pBInfo->aLevels[i].ofsGroup = pBInfo->aLevels[i - 1].ofsGroup + cGroups;
  pBInfo->nLevels = i;
  pBInfo->cBits = cBits;
}

/*
 * Returns the number of bytes required for a bitmap with the specified info.
 *
 * Parameters:
 * - pcBInfo = Pointer to the bitmap info structure.
 *
 * Returns:
 * The number of bytes required for a bitmap.
 */
SIZE_T _HeapBitmapInfoNumGroups(PCBITMAPINFO pcBInfo)
{
  return pcBInfo->aLevels[pcBInfo->nLevels].ofsGroup << LG_SIZEOF_BITMAP;
}

/*
 * Returns the size of a bitmap that is to contain a specified number of bits.
 *
 * Parameters:
 * - cBits = The number of bits the bitmap is to contain.
 *
 * Returns:
 * The size of the resulting bitmap.
 */
SIZE_T _HeapBitmapSize(SIZE_T cBits)
{
  BITMAPINFO binfo;

  _HeapBitmapInfoInit(&binfo, cBits);
  return _HeapBitmapInfoNumGroups(&binfo);
}

/*
 * Initializes a bitmap to "empty".
 *
 * Parameters:
 * - pBitmap = Pointer to the bitmap to be initialized.
 * - pcBInfo = Pointer to the corresponding bitmap info structure.
 *
 * Returns:
 * Nothing.
 */
void _HeapBitmapInit(PBITMAP pBitmap, PCBITMAPINFO pcBInfo)
{
  SIZE_T cbExtra;      /* number of "extra" bits */
  register UINT32 i;   /* loop counter */

  /*
   * Note that bitmaps are stored with the bits they contain "inverted," i.e. a 1 bit represents a bit
   * that has been *cleared*, and a 0 bit represents one that has been *set*, except for trailing unused
   * bits if any.  Bit 0 is always the first logical bit in the group.
   */
  StrSetMem(pBitmap, 0xFFU, pcBInfo->aLevels[pcBInfo->nLevels].ofsGroup << LG_SIZEOF_BITMAP);
  cbExtra = (BITMAP_GROUP_NBITS - (pcBInfo->cBits & BITMAP_GROUP_NBITS_MASK)) & BITMAP_GROUP_NBITS_MASK;
  if (cbExtra != 0)
    pBitmap[pcBInfo->aLevels[1].ofsGroup - 1] >>= cbExtra;
  for (i = 1; i < pcBInfo->nLevels; i++)
  {
    SIZE_T cGroups = pcBInfo->aLevels[i].ofsGroup - pcBInfo->aLevels[i - 1].ofsGroup;
    cbExtra = (BITMAP_GROUP_NBITS - (cGroups & BITMAP_GROUP_NBITS_MASK)) & BITMAP_GROUP_NBITS_MASK;
    if (cbExtra != 0)
      pBitmap[pcBInfo->aLevels[i + 1].ofsGroup - 1] >>= cbExtra;
  }
}

/*
 * Returns whether or not the specified group is full.
 *
 * Parameters:
 * - pBitmap = Pointer to the bitmap to be tested.
 * - pcBInfo = Pointer to the corresponding bitmap info structure.
 *
 * Returns:
 * - TRUE = If the bitmap is full.
 * - FALSE = If the bitmap is not full.
 */
BOOL _HeapBitmapFull(PBITMAP pBitmap, PCBITMAPINFO pcBInfo)
{
  register UINT32 ofsRootGroup = pcBInfo->aLevels[pcBInfo->nLevels].ofsGroup - 1;
  register BITMAP bmp = pBitmap[ofsRootGroup];
  return MAKEBOOL(bmp == 0);
}

/*
 * Returns a value of a bit in a bitmap.
 *
 * Parameters:
 * - pBitmap = Pointer to the bitmap to be tested.
 * - pcBInfo = Pointer to the corresponding bitmap info structure.
 * - nBit = The index of the bit to test.
 *
 * Returns:
 * - TRUE = If the bit is set.
 * - FALSE = If the bit is not set.
 */
BOOL _HeapBitmapGet(PBITMAP pBitmap, PCBITMAPINFO pcBInfo, SIZE_T nBit)
{
  register SIZE_T ofsGroup;  /* offset of group to look in */
  register BITMAP g;         /* current bitmap */

  if (nBit >= pcBInfo->cBits)
    return FALSE;
  ofsGroup = nBit >> LG_BITMAP_GROUP_NBITS;
  g = pBitmap[ofsGroup];
  return MAKEBOOL(!(g & (1U << (nBit & BITMAP_GROUP_NBITS_MASK))));
}

/*
 * Sets a bit in a bitmap to "on."
 *
 * Parameters:
 * - pBitmap = Pointer to the bitmap to be altered.
 * - pcBInfo = Pointer to the corresponding bitmap info structure.
 * - nBit = The index of the bit to set.
 *
 * Returns:
 * Nothing.
 */
void _HeapBitmapSet(PBITMAP pBitmap, PCBITMAPINFO pcBInfo, SIZE_T nBit)
{
  register SIZE_T ofsGroup;  /* offset of group to look in */
  register PBITMAP pg;       /* pointer to current bitmap */
  register BITMAP g;         /* current bitmap */

  if ((nBit < pcBInfo->cBits) && !_HeapBitmapGet(pBitmap, pcBInfo, nBit))
  {
    ofsGroup = nBit >> LG_BITMAP_GROUP_NBITS;
    pg = &(pBitmap[ofsGroup]);
    g = *pg;
    g ^= 1U << (nBit & BITMAP_GROUP_NBITS_MASK);
    *pg = g;
    if (g == 0)
    { /* Propagate state changes up the tree. */
      register UINT32 i;
      for (i = 1; i < pcBInfo->nLevels; i++)
      {
	nBit = ofsGroup;
	ofsGroup = nBit >> LG_BITMAP_GROUP_NBITS;
	pg = &(pBitmap[pcBInfo->aLevels[i].ofsGroup + ofsGroup]);
	g = *pg;
	g ^= 1U << (nBit & BITMAP_GROUP_NBITS_MASK);
	*pg = g;
	if (g != 0)
	  break;
      }
    }
  }
}

/*
 * Sets the first bit in the bitmap that is not yet set and returns its index.
 *
 * Parameters:
 * - pBitmap = Pointer to the bitmap to be altered.
 * - pcBInfo = Pointer to the corresponding bitmap info structure.
 *
 * Returns:
 * The index of the bit in the bitmap that was set as a result of this function.
 */
SIZE_T _HeapBitmapSetFirstUnset(PBITMAP pBitmap, PCBITMAPINFO pcBInfo)
{
  register SIZE_T nBit = (SIZE_T)(-1);  /* bit index; return from this function */
  register BITMAP g;                    /* current bitmap */
  register UINT32 i;                    /* index pointer */

  if (!_HeapBitmapFull(pBitmap, pcBInfo))
  {
    i = pcBInfo->nLevels - 1;
    g = pBitmap[pcBInfo->aLevels[i].ofsGroup];
    nBit = IntFirstSet(g) - 1;
    while (i > 0)
    {
      i--;
      g = pBitmap[pcBInfo->aLevels[i].ofsGroup + nBit];
      nBit = (nBit << LG_BITMAP_GROUP_NBITS) + (IntFirstSet(g) - 1);
    }
    _HeapBitmapSet(pBitmap, pcBInfo, nBit);
  }
  return nBit;
}

/*
 * Sets a bit in a bitmap to "off."
 *
 * Parameters:
 * - pBitmap = Pointer to the bitmap to be altered.
 * - pcBInfo = Pointer to the corresponding bitmap info structure.
 * - nBit = The index of the bit to unset.
 *
 * Returns:
 * Nothing.
 */
void _HeapBitmapUnset(PBITMAP pBitmap, PCBITMAPINFO pcBInfo, SIZE_T nBit)
{
  register SIZE_T ofsGroup;  /* offset of group to look in */
  register PBITMAP pg;       /* pointer to current bitmap */
  register BITMAP g;         /* current bitmap */
  BOOL fPropagate;           /* propagate changes? */

  if ((nBit < pcBInfo->cBits) && _HeapBitmapGet(pBitmap, pcBInfo, nBit))
  {
    ofsGroup = nBit >> LG_BITMAP_GROUP_NBITS;
    pg = &(pBitmap[ofsGroup]);
    g = *pg;
    fPropagate = MAKEBOOL(g == 0);
    g ^= 1U << (nBit & BITMAP_GROUP_NBITS_MASK);
    *pg = g;
    if (fPropagate)
    { /* Propagate state changes up the tree. */
      register UINT32 i;
      for (i = 1; i < pcBInfo->nLevels; i++)
      {
	nBit = ofsGroup;
	ofsGroup = nBit >> LG_BITMAP_GROUP_NBITS;
	pg = &(pBitmap[pcBInfo->aLevels[i].ofsGroup + ofsGroup]);
	g = *pg;
	fPropagate = MAKEBOOL(g == 0);
	g ^= 1U << (nBit & BITMAP_GROUP_NBITS_MASK);
	*pg = g;
	if (!fPropagate)
	  break;
      }
    }
  }
}
