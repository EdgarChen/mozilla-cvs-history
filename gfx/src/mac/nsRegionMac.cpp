/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

#include "nsRegionMac.h"
#include "prmem.h"

static NS_DEFINE_IID(kRegionIID, NS_IREGION_IID);

//---------------------------------------------------------------------

nsRegionMac :: nsRegionMac()
{
  NS_INIT_REFCNT();
	mRegion = nsnull;
  mRegionType = eRegionComplexity_empty;
}

//---------------------------------------------------------------------

nsRegionMac :: ~nsRegionMac()
{
  if (mRegion)
    ::DisposeRgn(mRegion);
  mRegion = nsnull;
}

NS_IMPL_QUERY_INTERFACE(nsRegionMac, kRegionIID)
NS_IMPL_ADDREF(nsRegionMac)
NS_IMPL_RELEASE(nsRegionMac)

//---------------------------------------------------------------------

nsresult nsRegionMac :: Init(void)
{
	mRegion = ::NewRgn();
  mRegionType = eRegionComplexity_empty;
  return NS_OK;
}

//---------------------------------------------------------------------

void nsRegionMac :: SetTo(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
	::CopyRgn(pRegion->mRegion, mRegion);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: SetTo(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	::SetRectRgn(mRegion, aX, aY, aX + aWidth, aY + aHeight);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Intersect(const nsIRegion &aRegion)
{
  nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
  ::SectRgn(mRegion, pRegion->mRegion, mRegion);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Intersect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = ::NewRgn();
	::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
  ::SectRgn(mRegion, rectRgn, mRegion);
  ::DisposeRgn(rectRgn);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Union(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
  ::UnionRgn(mRegion, pRegion->mRegion, mRegion);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Union(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = ::NewRgn();
	::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
  ::UnionRgn(mRegion, rectRgn, mRegion);
  ::DisposeRgn(rectRgn);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Subtract(const nsIRegion &aRegion)
{
	nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
  ::DiffRgn(mRegion, pRegion->mRegion, mRegion);
  SetRegionType();
}

//---------------------------------------------------------------------

void nsRegionMac :: Subtract(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = ::NewRgn();
	::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
  ::DiffRgn(mRegion, rectRgn, mRegion);
  ::DisposeRgn(rectRgn);
  SetRegionType();
}

//---------------------------------------------------------------------

PRBool nsRegionMac :: IsEmpty(void)
{
  if (mRegionType == eRegionComplexity_empty)
    return PR_TRUE;
	else
  	return PR_FALSE;
}

//---------------------------------------------------------------------

PRBool nsRegionMac :: IsEqual(const nsIRegion &aRegion)
{
  nsRegionMac* pRegion = (nsRegionMac*)&aRegion;
  return(::EqualRgn(mRegion, pRegion->mRegion));
}

//---------------------------------------------------------------------

void nsRegionMac :: GetBoundingBox(PRInt32 *aX, PRInt32 *aY, PRInt32 *aWidth, PRInt32 *aHeight)
{
  Rect macRect = (**mRegion).rgnBBox;

  *aX = macRect.left;
  *aY = macRect.top;
  *aWidth  = macRect.right - macRect.left;
  *aHeight = macRect.bottom - macRect.top;
}

//---------------------------------------------------------------------

void nsRegionMac :: Offset(PRInt32 aXOffset, PRInt32 aYOffset)
{
  ::OffsetRgn(mRegion, aXOffset, aYOffset);
}

//---------------------------------------------------------------------

PRBool nsRegionMac :: ContainsRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	Rect macRect;
	::SetRect(&macRect, aX, aY, aX + aWidth, aY + aHeight);
	return(::RectInRgn(&macRect, mRegion));
}

//---------------------------------------------------------------------

PRBool nsRegionMac :: ForEachRect(nsRectInRegionFunc *func, void *closure)
{
/* This is a minor adaptation of code written by Hugh Fisher
   and published in the RegionToRectangles example in the InfoMac archives.
   ported to raptor from old macfe. MMP
*/
#define EndMark 	32767
#define MaxY		32767
#define StackMax	1024

	typedef struct {
		short	size;
		Rect	bbox;
		short	data[];
		} ** Internal;
	
	Internal region;
	short	 width, xAdjust, y, index, x1, x2, x;
	nsRect	 box;
	short	 stackStorage[1024];
	short *	 buffer;
	
	region = (Internal)mRegion;
	
	/* Check for plain rectangle */
	if ((**region).size == 10) {
		box.x = (**region).bbox.left;
		box.y = (**region).bbox.top;
		box.width = (**region).bbox.right - box.x;
		box.height = (**region).bbox.bottom - box.y;
		(*func)(closure, box);
		return PR_FALSE;
	}
	/* Got to scale x coordinates into range 0..something */
	xAdjust = (**region).bbox.left;
	width = (**region).bbox.right - xAdjust;
	/* Most regions will be less than 1024 pixels wide */
	if (width < StackMax)
		buffer = stackStorage;
	else {
		buffer = (short *)PR_Malloc(width * 2);
		if (buffer == NULL)
			/* Truly humungous region or very low on memory.
			   Quietly doing nothing seems to be the
			   traditional Quickdraw response. */
			return PR_FALSE;
	}
	/* Initialise scan line list to bottom edges */
	for (x = (**region).bbox.left; x < (**region).bbox.right; x++)
		buffer[x - xAdjust] = MaxY;
	index = 0;
	/* Loop until we hit an empty scan line */
	while ((**region).data[index] != EndMark) {
		y = (**region).data[index];
		index ++;
		/* Loop through horizontal runs on this line */
		while ((**region).data[index] != EndMark) {
			x1 = (**region).data[index];
			index ++;
			x2 = (**region).data[index];
			index ++;
			x = x1;
			while (x < x2) {
				if (buffer[x - xAdjust] < y) {
					/* We have a bottom edge - how long for? */
					box.x = x;
					box.y  = buffer[x - xAdjust];
					while (x < x2 && buffer[x - xAdjust] == box.y) {
						buffer[x - xAdjust] = MaxY;
						x ++;
					}
					/* Pass to client proc */
					box.width  = x - box.x;
					box.height = y - box.y;
					(*func)(closure, box);
				} else {
					/* This becomes a top edge */
					buffer[x - xAdjust] = y;
					x ++;
				}
			}
		}
		index ++;
	}
	/* Clean up after ourselves */
	if (width >= StackMax)
		PR_Free((void *)buffer);
#undef EndMark
#undef MaxY
#undef StackMax

  return PR_FALSE;
}

//---------------------------------------------------------------------


NS_IMETHODIMP nsRegionMac :: GetNativeRegion(void *&aRegion) const
{
  aRegion = (void *)mRegion;
  return NS_OK;
}

//---------------------------------------------------------------------

NS_IMETHODIMP nsRegionMac :: GetRegionComplexity(nsRegionComplexity &aComplexity) const
{
  aComplexity = mRegionType;
  return NS_OK;
}

//---------------------------------------------------------------------


void nsRegionMac :: SetRegionType()
{
  if (::EmptyRgn(mRegion) == PR_TRUE)
    mRegionType = eRegionComplexity_empty;
  else
    mRegionType = eRegionComplexity_rect;
}


//---------------------------------------------------------------------


void nsRegionMac :: SetRegionEmpty()
{
  ::SetEmptyRgn(mRegion);
}

//---------------------------------------------------------------------


RgnHandle nsRegionMac :: CreateRectRegion(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight)
{
	RgnHandle rectRgn = ::NewRgn();
	::SetRectRgn(rectRgn, aX, aY, aX + aWidth, aY + aHeight);
  return rectRgn;
}








