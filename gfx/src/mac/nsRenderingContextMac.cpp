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
 
/* 
 String

translated to a number via  getfnum()

TextFont(number)
TextSize
TextFace  -- may not play well with new style  
DrawString();  DrawText for cstrings
*/

#include "nsRenderingContextMac.h"
#include "nsDeviceContextMac.h"
#include "nsFontMetricsMac.h"

#include <math.h>
#include "nspr.h"
#include <QDOffscreen.h>
#include <windows.h>
#include "nsRegionMac.h"
#include "nsGfxCIID.h"
#include <Fonts.h>

//#define NO_CLIP


class GraphicsState
{
public:
  GraphicsState();
  ~GraphicsState();

	GrafPtr					*mCurPort;		// port set up on the stack
  nsTransform2D   *mMatrix;
  nsRect          mLocalClip;
  RgnHandle       mClipRegion;
  nscolor         mColor;
  nsIFontMetrics  *mFontMetrics;
  PRInt32        	mFont;
  PRInt32					mOffx;
  PRInt32					mOffy;
};

//------------------------------------------------------------------------

GraphicsState :: GraphicsState()
{
  mMatrix = nsnull;  
  mLocalClip.x = mLocalClip.y = mLocalClip.width = mLocalClip.height = 0;
  mClipRegion = nsnull;
  mColor = NS_RGB(0, 0, 0);
  mFontMetrics = nsnull;
  mFont = 0;
  mOffx = 0;
  mOffy = 0;
}

//------------------------------------------------------------------------

GraphicsState :: ~GraphicsState()
{
	mFont = 0;
}

static NS_DEFINE_IID(kRenderingContextIID, NS_IRENDERING_CONTEXT_IID);

//------------------------------------------------------------------------

nsRenderingContextMac :: nsRenderingContextMac()
{
  NS_INIT_REFCNT();

  mFontMetrics = nsnull ;
  mContext = nsnull ;
  mRenderingSurface = nsnull ;        
  mCurrentColor = NS_RGB(255,255,255);
  mTMatrix = nsnull;
  mP2T = 1.0f;
  mStateCache = new nsVoidArray();
  mClipRegion = nsnull;
  mCurrFontHandle = 0;
  mOffx = 0;
  mOffy = 0;
  PushState();
}

//------------------------------------------------------------------------

nsRenderingContextMac :: ~nsRenderingContextMac()
{

	if(mRenderingSurface)
		{
  	::SetPort(mOriSurface);
  	::SetOrigin(0,0);
  	}

  if (mClipRegion) 
  	{
    ::DisposeRgn(mClipRegion);
    mClipRegion = nsnull;
  	}
  	
  mTMatrix = nsnull;

  // Destroy the State Machine
  if (nsnull != mStateCache)
  	{
    PRInt32 cnt = mStateCache->Count();

    while (--cnt >= 0)
    	{
      GraphicsState *state = (GraphicsState *)mStateCache->ElementAt(cnt);
      mStateCache->RemoveElementAt(cnt);

      if (nsnull != state)
        delete state;
    	}
    delete mStateCache;
    mStateCache = nsnull;
  	}

  NS_IF_RELEASE(mFontMetrics);
  NS_IF_RELEASE(mContext);

}

NS_IMPL_QUERY_INTERFACE(nsRenderingContextMac, kRenderingContextIID)
NS_IMPL_ADDREF(nsRenderingContextMac)
NS_IMPL_RELEASE(nsRenderingContextMac)

//------------------------------------------------------------------------

nsresult nsRenderingContextMac :: Init(nsIDeviceContext* aContext,nsIWidget *aWindow)
{
PRInt32	offx,offy;

  if (nsnull == aWindow->GetNativeData(NS_NATIVE_WINDOW))
    return NS_ERROR_NOT_INITIALIZED;

  mContext = aContext;
  NS_IF_ADDREF(mContext);

	offx = (PRInt32)aWindow->GetNativeData(NS_NATIVE_OFFSETX);
	offy = (PRInt32)aWindow->GetNativeData(NS_NATIVE_OFFSETY);

  mRenderingSurface = (nsDrawingSurfaceMac)aWindow->GetNativeData(NS_NATIVE_DISPLAY);
  mFrontBuffer = mRenderingSurface;
  
  mMainRegion = (RgnHandle)aWindow->GetNativeData(NS_NATIVE_REGION);

	mOriSurface = mRenderingSurface;    // we need to know when we set back
  ::SetPort(mRenderingSurface);
  ::SetOrigin(-offx,-offy);
  
  return (CommonInit());
}

//------------------------------------------------------------------------


// this drawing surface init should only be called for an offscreen drawing surface, without and offset or clip region
nsresult nsRenderingContextMac :: Init(nsIDeviceContext* aContext,
					nsDrawingSurface aSurface)
{

  mContext = aContext;
  NS_IF_ADDREF(mContext);

  mRenderingSurface = (nsDrawingSurfaceMac) aSurface;
  
  return (CommonInit());
}

//------------------------------------------------------------------------


nsresult nsRenderingContextMac :: CommonInit()
{

	((nsDeviceContextMac *)mContext)->SetDrawingSurface(mRenderingSurface);
  //((nsDeviceContextMac *)mContext)->InstallColormap();

  mContext->GetDevUnitsToAppUnits(mP2T);
  float app2dev;
  mContext->GetAppUnitsToDevUnits(app2dev);
  mTMatrix->AddScale(app2dev, app2dev);
  this->SetColor(mCurrentColor);
  

  return NS_OK;
}

//------------------------------------------------------------------------

nsresult nsRenderingContextMac :: SelectOffScreenDrawingSurface(nsDrawingSurface aSurface)
{  
  if (nsnull == aSurface)
    mRenderingSurface = mFrontBuffer;
  else
  	mRenderingSurface = (nsDrawingSurfaceMac)aSurface;

	return NS_OK;
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: Reset()
{
}

//------------------------------------------------------------------------

nsIDeviceContext * nsRenderingContextMac :: GetDeviceContext(void)
{
  NS_IF_ADDREF(mContext);
  return mContext;
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: PushState(void)
{
nsRect 	rect;
Rect		mac_rect;

	
  GraphicsState * state = new GraphicsState();

  // Push into this state object, add to vector
  state->mMatrix = mTMatrix;

  mStateCache->AppendElement(state);

  if (nsnull == mTMatrix)
    mTMatrix = new nsTransform2D();
  else
    mTMatrix = new nsTransform2D(mTMatrix);

  GetClipRect(state->mLocalClip);

  state->mClipRegion = mClipRegion;

  if (nsnull != state->mClipRegion) 
  	{
    mClipRegion = NewRgn();
    
    mac_rect.left = state->mLocalClip.x;
    mac_rect.top = state->mLocalClip.y;
    mac_rect.right = state->mLocalClip.x + state->mLocalClip.width;
    mac_rect.bottom = state->mLocalClip.y + state->mLocalClip.height;
    
    SetRectRgn(mClipRegion,state->mLocalClip.x,state->mLocalClip.y,
    					state->mLocalClip.x + state->mLocalClip.width,state->mLocalClip.y + state->mLocalClip.height);
  	}

  state->mOffx = mOffx;
  state->mOffy = mOffy;
  
  state->mColor = mCurrentColor;
}

//------------------------------------------------------------------------

PRBool nsRenderingContextMac :: PopState(void)
{
PRBool bEmpty = PR_FALSE;

	
  PRUint32 cnt = mStateCache->Count();
  GraphicsState * state;

  if (cnt > 0) 
  	{
    state = (GraphicsState *)mStateCache->ElementAt(cnt - 1);
    mStateCache->RemoveElementAt(cnt - 1);

    // Assign all local attributes from the state object just popped
    if (mTMatrix)
      delete mTMatrix;
    mTMatrix = state->mMatrix;

    if (nsnull != mClipRegion)
      ::DisposeRgn(mClipRegion);

    mClipRegion = state->mClipRegion;

    if (nsnull != mClipRegion && ::EmptyRgn(mClipRegion) == PR_TRUE)
    	{
      bEmpty = PR_TRUE;
    	}
  	else
  		{
      // Select in the old region.  We probably want to set a dirty flag and only 
      // do this IFF we need to draw before the next Pop.  We'd need to check the
      // state flag on every draw operation.
      if (nsnull != mClipRegion)
      	{
      	// set this region as the currentclippig region
      	::SetClip(mClipRegion);
				}
    	}

    if (state->mColor != mCurrentColor)
      SetColor(state->mColor);
    
    mOffy = state->mOffy;
    mOffx = state->mOffx;
    ::SetOrigin(mOffy,mOffx);
    
    // Delete this graphics state object
    delete state;
  	}

  return bEmpty;
  return PR_FALSE;
}

//------------------------------------------------------------------------

PRBool nsRenderingContextMac :: IsVisibleRect(const nsRect& aRect)
{
  return PR_TRUE;
}

//------------------------------------------------------------------------

PRBool nsRenderingContextMac :: SetClipRectInPixels(const nsRect& aRect, nsClipCombine aCombine)
{
PRBool 		bEmpty = PR_FALSE;
nsRect  	trect = aRect;
RgnHandle	theregion,tregion;

  theregion = ::NewRgn();
  SetRectRgn(theregion,trect.x,trect.y,trect.x+trect.width,trect.y+trect.height);

  if (aCombine == nsClipCombine_kIntersect)
  	{
    if (nsnull != mClipRegion) 
    	{
    	tregion = ::NewRgn();
    	::SectRgn(theregion,mClipRegion,tregion);
      ::DisposeRgn(theregion);
      ::DisposeRgn(mClipRegion);
      mClipRegion = tregion;
    	} 
  	}
  else 
  	if (aCombine == nsClipCombine_kUnion)
  		{
	    if (nsnull != mClipRegion) 
	    	{
	      tregion = ::NewRgn();
	      ::UnionRgn(theregion, mClipRegion, tregion);
	      ::DisposeRgn(mClipRegion);
	      ::DisposeRgn(theregion);
	      mClipRegion = tregion;
	    	} 
  		}
  	else 
  		if (aCombine == nsClipCombine_kSubtract)
  			{
		    if (nsnull != mClipRegion) 
		    	{
		      tregion = ::NewRgn();
		      ::DiffRgn(mClipRegion, theregion, tregion);
		      ::DisposeRgn(mClipRegion);
		      ::DisposeRgn(theregion);
		      mClipRegion = tregion;
		    	} 
  			}
  	else 
  		if (aCombine == nsClipCombine_kReplace)
  			{
		    if (nsnull != mClipRegion)
		      ::DisposeRgn(mClipRegion);

		    mClipRegion = theregion;
  			}
  		else
    		NS_ASSERTION(PR_FALSE, "illegal clip combination");

  if (nsnull == mClipRegion)
  	{
    bEmpty = PR_TRUE;
    // clip to the size of the window
    ::SetPort(mRenderingSurface);
    ::ClipRect(&mRenderingSurface->portRect);
  	} 
 	else 
 		{
 		// set the clipping area of this windowptr
 		::SetPort(mRenderingSurface);
    ::SetClip(mClipRegion);
  	}

  return bEmpty;
}

//------------------------------------------------------------------------

PRBool nsRenderingContextMac :: SetClipRect(const nsRect& aRect, nsClipCombine aCombine)
{
nsRect  trect = aRect;

  mTMatrix->TransformCoord(&trect.x, &trect.y,&trect.width, &trect.height);
  return(SetClipRectInPixels(trect,aCombine));
}

//------------------------------------------------------------------------

PRBool nsRenderingContextMac :: GetClipRect(nsRect &aRect)
{
Rect	cliprect;

  if (mClipRegion != nsnull) 
  	{
  	cliprect = (**mClipRegion).rgnBBox;
    aRect.SetRect(cliprect.left, cliprect.top, cliprect.right-cliprect.left, cliprect.bottom-cliprect.top);
    return(PR_FALSE);
  	} 
 	else 
 		{
    aRect.SetRect(0,0,0,0);
    return (PR_TRUE);
  	}
}

//------------------------------------------------------------------------

PRBool nsRenderingContextMac :: SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine)
{
nsRect 			rect;
Rect			 	mrect;
RgnHandle		mregion;

  nsRegionMac *pRegion = (nsRegionMac *)&aRegion;
  
  
  mregion = pRegion->GetRegion();
  mrect = (**mregion).rgnBBox;
  
  rect.x = mrect.left;
  rect.y = mrect.top;
  rect.width = mrect.right-mrect.left;
  rect.height = mrect.bottom-mrect.top;

  SetClipRectInPixels(rect, aCombine);

  if (::EmptyRgn(mClipRegion) == PR_TRUE)
    return PR_TRUE;
  else
    return PR_FALSE;
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: GetClipRegion(nsIRegion **aRegion)
{
nsIRegion * pRegion ;

  static NS_DEFINE_IID(kCRegionCID, NS_REGION_CID);
  static NS_DEFINE_IID(kIRegionIID, NS_IREGION_IID);

  nsresult rv = nsRepository::CreateInstance(kCRegionCID,nsnull,  kIRegionIID, (void **)aRegion);

  if (NS_OK == rv) 
  	{
    nsRect rect;
    pRegion = (nsIRegion *)&aRegion;
    pRegion->Init();    
    this->GetClipRect(rect);
    pRegion->Union(rect.x,rect.y,rect.width,rect.height);
  	}
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: SetColor(nscolor aColor)
{
RGBColor	thecolor;
GrafPtr		curport;

	GetPort(&curport);
	SetPort(mRenderingSurface);
	thecolor.red = NS_GET_R(aColor)<<8;
	thecolor.green = NS_GET_G(aColor)<<8;
	thecolor.blue = NS_GET_B(aColor)<<8;
	::RGBForeColor(&thecolor);
  mCurrentColor = aColor ;
  SetPort(curport);
 
}

//------------------------------------------------------------------------

nscolor nsRenderingContextMac :: GetColor() const
{
  return mCurrentColor;
}

//------------------------------------------------------------------------

nsresult nsRenderingContextMac :: SetLineStyle(nsLineStyle aLineStyle)
{
  return NS_OK;
}

//------------------------------------------------------------------------

nsresult nsRenderingContextMac :: GetLineStyle(nsLineStyle &aLineStyle)
{
  return NS_OK;
}


//------------------------------------------------------------------------

void nsRenderingContextMac :: SetFont(const nsFont& aFont)
{
	NS_IF_RELEASE(mFontMetrics);
	if (mContext)
		mContext->GetMetricsFor(aFont, mFontMetrics);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: SetFont(nsIFontMetrics *aFontMetrics)
{
	NS_IF_RELEASE(mFontMetrics);
	mFontMetrics = aFontMetrics;
	NS_IF_ADDREF(mFontMetrics);
}

//------------------------------------------------------------------------

const nsFont& nsRenderingContextMac :: GetFont()
{
  const nsFont* font = nsnull;
  if(mFontMetrics)
  	mFontMetrics->GetFont(font);
  return *font;
}

//------------------------------------------------------------------------

nsIFontMetrics* nsRenderingContextMac :: GetFontMetrics()
{
  NS_IF_ADDREF(mFontMetrics);
  return mFontMetrics;
}

//------------------------------------------------------------------------

// add the passed in translation to the current translation
void nsRenderingContextMac :: Translate(nscoord aX, nscoord aY)
{

  mTMatrix->AddTranslation((float)aX,(float)aY);

}

//------------------------------------------------------------------------

// add the passed in scale to the current scale
void nsRenderingContextMac :: Scale(float aSx, float aSy)
{

  mTMatrix->AddScale(aSx, aSy);

}

//------------------------------------------------------------------------

nsTransform2D * nsRenderingContextMac :: GetCurrentTransform()
{

  return mTMatrix;
}

//------------------------------------------------------------------------

nsDrawingSurface nsRenderingContextMac :: CreateDrawingSurface(nsRect *aBounds, PRUint32 aSurfFlags)
{
PRUint32	depth;
GWorldPtr	theoff;
Rect			bounds;
QDErr			myerr;

  // Must make sure this code never gets called when nsRenderingSurface is nsnull
  mContext->GetDepth(depth);
  if(aBounds!=nsnull)
  	::SetRect(&bounds,aBounds->x,aBounds->y,aBounds->x+aBounds->width,aBounds->height);
  else
  	::SetRect(&bounds,0,0,2,2);
  myerr = ::NewGWorld(&theoff,depth,&bounds,0,0,0);
  
  return theoff;  
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DestroyDrawingSurface(nsDrawingSurface aDS)
{
GWorldPtr	theoff;

	if(aDS)
		{
		theoff = (GWorldPtr)aDS;
		DisposeGWorld(theoff);
		}
	
	theoff = nsnull;
	mRenderingSurface = mFrontBuffer;		// point back at the front surface
		
  //if (mRenderingSurface == (GrafPtr)theoff)
    //mRenderingSurface = nsnull;

  //DisposeRgn(mMainRegion);
  //mMainRegion = nsnull;
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1)
{
  mTMatrix->TransformCoord(&aX0,&aY0);
  mTMatrix->TransformCoord(&aX1,&aY1);

	::SetPort(mRenderingSurface);
	::SetClip(mMainRegion);
	::MoveTo(aX0, aY0);
	::LineTo(aX1, aY1);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints)
{
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawRect(const nsRect& aRect)
{
  DrawRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
nscoord x,y,w,h;
Rect		therect;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

	::SetPort(mRenderingSurface);
	::SetClip(mMainRegion);
  mTMatrix->TransformCoord(&x,&y,&w,&h);
	::SetRect(&therect,x,y,x+w,y+h);
	::FrameRect(&therect);

}

//------------------------------------------------------------------------

void nsRenderingContextMac :: FillRect(const nsRect& aRect)
{
	FillRect(aRect.x, aRect.y, aRect.width, aRect.height);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
nscoord x,y,w,h;
Rect		therect;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

  mTMatrix->TransformCoord(&x,&y,&w,&h);

	::SetPort(mRenderingSurface);
	::SetClip(mMainRegion);
	::SetRect(&therect,x,y,x+w,y+h);
	::PaintRect(&therect);
  
}

//------------------------------------------------------------------------

void nsRenderingContextMac::DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
PRUint32 		i ;
PolyHandle	thepoly;
PRInt32			x,y;

	SetPort(mRenderingSurface);
	thepoly = ::OpenPoly();
	
	x = aPoints[0].x;
	y = aPoints[0].y;
	mTMatrix->TransformCoord((PRInt32*)&x,(PRInt32*)&y);
	::MoveTo(x,y);
	for (i = 1; i < aNumPoints; i++)
		{
    x = aPoints[i].x;
    y = aPoints[i].y;
		mTMatrix->TransformCoord((PRInt32*)&x,(PRInt32*)&y);
		::LineTo(x,y);
		}
	ClosePoly();
	
	::FramePoly(thepoly);
}

//------------------------------------------------------------------------

void nsRenderingContextMac::FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints)
{
PRUint32 		i ;
PolyHandle	thepoly;
PRInt32			x,y;

	::SetPort(mRenderingSurface);
	::SetClip(mMainRegion);
	thepoly = ::OpenPoly();
	
	x = aPoints[0].x;
	y = aPoints[0].y;
	mTMatrix->TransformCoord((PRInt32*)&x,(PRInt32*)&y);
	::MoveTo(x,y);
	for (i = 1; i < aNumPoints; i++)
		{
    x = aPoints[i].x;
    y = aPoints[i].y;
		mTMatrix->TransformCoord((PRInt32*)&x,(PRInt32*)&y);
		::LineTo(x,y);
		}
	::ClosePoly();
	
	::PaintPoly(thepoly);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawEllipse(const nsRect& aRect)
{
  DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
nscoord x,y,w,h;
Rect		therect;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

	::SetPort(mRenderingSurface);
	::SetClip(mMainRegion);
  mTMatrix->TransformCoord(&x,&y,&w,&h);
  ::SetRect(&therect,x,y,x+w,x+h);
  ::FrameOval(&therect);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: FillEllipse(const nsRect& aRect)
{
  FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight)
{
nscoord x,y,w,h;
Rect		therect;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

	::SetPort(mRenderingSurface);
	::SetClip(mMainRegion);
  mTMatrix->TransformCoord(&x,&y,&w,&h);
  ::SetRect(&therect,x,y,x+w,x+h);
  ::PaintOval(&therect);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  this->DrawArc(aRect.x,aRect.y,aRect.width,aRect.height,aStartAngle,aEndAngle);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
nscoord x,y,w,h;
Rect		therect;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

	::SetPort(mRenderingSurface);
	::SetClip(mMainRegion);
  mTMatrix->TransformCoord(&x,&y,&w,&h);
  ::SetRect(&therect,x,y,x+w,x+h);
  ::FrameArc(&therect,aStartAngle,aEndAngle);

}

//------------------------------------------------------------------------

void nsRenderingContextMac :: FillArc(const nsRect& aRect,
                                 float aStartAngle, float aEndAngle)
{
  this->FillArc(aRect.x, aRect.y, aRect.width, aRect.height, aStartAngle, aEndAngle);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                                 float aStartAngle, float aEndAngle)
{
nscoord x,y,w,h;
Rect		therect;

  x = aX;
  y = aY;
  w = aWidth;
  h = aHeight;

	::SetPort(mRenderingSurface);
	::SetClip(mMainRegion);
  mTMatrix->TransformCoord(&x,&y,&w,&h);
  ::SetRect(&therect,x,y,x+w,x+h);
  ::PaintArc(&therect,aStartAngle,aEndAngle);
}

//------------------------------------------------------------------------

NS_IMETHODIMP nsRenderingContextMac :: GetWidth(char ch, nscoord &aWidth)
{
  char buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth);
}

//------------------------------------------------------------------------

NS_IMETHODIMP nsRenderingContextMac :: GetWidth(PRUnichar ch, nscoord &aWidth)
{
  PRUnichar buf[1];
  buf[0] = ch;
  return GetWidth(buf, 1, aWidth);
}

//------------------------------------------------------------------------

NS_IMETHODIMP nsRenderingContextMac :: GetWidth(const nsString& aString, nscoord &aWidth)
{
  return GetWidth(aString.GetUnicode(), aString.Length(), aWidth);
}

//------------------------------------------------------------------------

NS_IMETHODIMP nsRenderingContextMac :: GetWidth(const char *aString, nscoord &aWidth)
{
  return GetWidth(aString, strlen(aString), aWidth);
}

//------------------------------------------------------------------------

NS_IMETHODIMP
nsRenderingContextMac :: GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth)
{
	// set native font and attributes
	nsFont font = GetFont();
	nsFontMetricsMac::SetFont(font, mContext);

	// measure text
	short textWidth = ::TextWidth(aString, 0, aLength);
	aWidth = NSToCoordRound(float(textWidth) * mP2T);

	// add a bit for italic
	switch (font.style)
	{
		case NS_FONT_STYLE_ITALIC:
		case NS_FONT_STYLE_OBLIQUE:
			nscoord aAdvance;
			mFontMetrics->GetMaxAdvance(aAdvance);
			aWidth += aAdvance;
			break;
	}

	return NS_OK;
}

//------------------------------------------------------------------------

NS_IMETHODIMP nsRenderingContextMac :: GetWidth(const PRUnichar *aString, PRUint32 aLength, nscoord &aWidth)
{
	nsString nsStr;
	nsStr.SetString(aString, aLength);
	char* cStr = nsStr.ToNewCString();
  GetWidth(cStr, aLength, aWidth);
	delete[] cStr;
  return NS_OK;
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawString(const char *aString, PRUint32 aLength,
                                         nscoord aX, nscoord aY,
                                         nscoord aWidth,
                                         const nscoord* aSpacing)
{
	PRInt32 x = aX;
	PRInt32 y = aY;

	::SetPort(mRenderingSurface);
	::SetClip(mMainRegion);

	if (mFontMetrics)
	{
		// set native font and attributes
		nsFontMetricsMac::SetFont(GetFont(), mContext);

		// substract ascent since drawing specifies baseline
		nscoord ascent = 0;
		mFontMetrics->GetMaxAscent(ascent);
		y += ascent;
	}

	mTMatrix->TransformCoord(&x,&y);

	::MoveTo(x,y);
	::DrawText(aString,0,aLength);

	if (mFontMetrics)
	{
		const nsFont* font;
		mFontMetrics->GetFont(font);
		PRUint8 deco = font->decorations;

		if (deco & NS_FONT_DECORATION_OVERLINE)
			DrawLine(aX, aY, aX + aWidth, aY);

		if (deco & NS_FONT_DECORATION_UNDERLINE)
		{
			nscoord ascent = 0;
			nscoord descent = 0;
			mFontMetrics->GetMaxAscent(ascent);
			mFontMetrics->GetMaxDescent(descent);

			DrawLine(aX, aY + ascent + (descent >> 1),
						aX + aWidth, aY + ascent + (descent >> 1));
		}

		if (deco & NS_FONT_DECORATION_LINE_THROUGH)
		{
			nscoord height = 0;
			mFontMetrics->GetHeight(height);

			DrawLine(aX, aY + (height >> 1), aX + aWidth, aY + (height >> 1));
		}
	}
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawString(const PRUnichar *aString, PRUint32 aLength,
                                         nscoord aX, nscoord aY, nscoord aWidth,
                                         const nscoord* aSpacing)
{
	nsString nsStr;
	nsStr.SetString(aString, aLength);
	char* cStr = nsStr.ToNewCString();
		DrawString(cStr, aLength, aX, aY, aWidth);
	delete[] cStr;
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawString(const nsString& aString,
                                         nscoord aX, nscoord aY, nscoord aWidth,
                                         const nscoord* aSpacing)
{
	char* cStr = aString.ToNewCString();
		DrawString(cStr, aString.Length(), aX, aY, aWidth, aSpacing);
	delete[] cStr;
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY)
{
nscoord width,height;

  width = NSToCoordRound(mP2T * aImage->GetWidth());
  height = NSToCoordRound(mP2T * aImage->GetHeight());
  
  this->DrawImage(aImage,aX,aY,width,height);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawImage(nsIImage *aImage, nscoord aX, nscoord aY,
                                        nscoord aWidth, nscoord aHeight) 
{
nsRect	tr;

  tr.x = aX;
  tr.y = aY;
  tr.width = aWidth;
  tr.height = aHeight;
  this->DrawImage(aImage,tr);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawImage(nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect)
{
nsRect	sr,dr;
  
  sr = aSRect;
  mTMatrix ->TransformCoord(&sr.x,&sr.y,&sr.width,&sr.height);

  dr = aDRect;
  mTMatrix->TransformCoord(&dr.x,&dr.y,&dr.width,&dr.height);
  
  ((nsImageMac*)aImage)->Draw(*this,mRenderingSurface,sr.x,sr.y,sr.width,sr.height,
                                 dr.x,dr.y,dr.width,dr.height);
}

//------------------------------------------------------------------------

void nsRenderingContextMac :: DrawImage(nsIImage *aImage, const nsRect& aRect)
{
nsRect	tr;

  tr = aRect;
  mTMatrix->TransformCoord(&tr.x,&tr.y,&tr.width,&tr.height);
  
  if (aImage != nsnull) 
  	{
		::SetPort(mRenderingSurface);
		::SetClip(mMainRegion);
    ((nsImageMac*)aImage)->Draw(*this,mRenderingSurface,tr.x,tr.y,tr.width,tr.height);
  	} 
  else 
  	{
    printf("Image is NULL!\n");
  	}
}

//------------------------------------------------------------------------

NS_IMETHODIMP nsRenderingContextMac :: CopyOffScreenBits(nsDrawingSurface aSrcSurf,
                                                         PRInt32 aSrcX, PRInt32 aSrcY,
                                                         const nsRect &aDestBounds,
                                                         PRUint32 aCopyFlags)
{
  PixMapHandle	      offscreenPM;
  PixMapPtr			      srcpix;
  PixMapPtr			      destpix;
  RGBColor			      rgbblack = {0x0000,0x0000,0x0000};
  RGBColor			      rgbwhite = {0xFFFF,0xFFFF,0xFFFF};
  Rect					      srcrect,dstrect;
  PRInt32             x = aSrcX;
  PRInt32             y = aSrcY;
  nsRect              drect = aDestBounds;
  nsDrawingSurfaceMac destport;

  if (aCopyFlags & NS_COPYBITS_TO_BACK_BUFFER)
  {
    NS_ASSERTION(!(nsnull == destport), "no back buffer");
    destport = mRenderingSurface;
  }
  else
    destport = mFrontBuffer;

  if (aCopyFlags & NS_COPYBITS_XFORM_SOURCE_VALUES)
    mTMatrix->TransformCoord(&x, &y);

  if (aCopyFlags & NS_COPYBITS_XFORM_DEST_VALUES)
    mTMatrix->TransformCoord(&drect.x, &drect.y, &drect.width, &drect.height);

	::SetRect(&srcrect,x,y,drect.width,drect.height);
	::SetRect(&dstrect,drect.x,drect.y,drect.width,drect.height);

	::SetPort(destport);

  if (aCopyFlags & NS_COPYBITS_USE_SOURCE_CLIP_REGION)
  {
  	::SetEmptyRgn(destport->clipRgn);
	  ::CopyRgn(((nsDrawingSurfaceMac)aSrcSurf)->clipRgn, destport->clipRgn);
  }

	destpix = *((CGrafPtr)destport)->portPixMap;

	offscreenPM = ::GetGWorldPixMap((GWorldPtr)aSrcSurf);
	LockPixels(offscreenPM);
	srcpix = (PixMapPtr)*offscreenPM;
	::RGBForeColor(&rgbblack);
	::RGBBackColor(&rgbwhite);
	
	::CopyBits((BitMap*)srcpix,(BitMap*)destpix,&srcrect,&dstrect,ditherCopy,0L);
	UnlockPixels(offscreenPM);
	
  return NS_OK;
}















