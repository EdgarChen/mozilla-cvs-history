/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Mozilla OS/2 libraries.
 *
 * The Initial Developer of the Original Code is John Fairhurst,
 * <john_fairhurst@iname.com>.  Portions created by John Fairhurst are
 * Copyright (C) 1999 John Fairhurst. All Rights Reserved.
 *
 * Contributor(s): Henry Sobotka <sobotka@axess.com>
 *
 * This Original Code has been modified by IBM Corporation. Modifications made by IBM 
 * described herein are Copyright (c) International Business Machines Corporation, 2000.
 * Modifications to Mozilla code or documentation identified per MPL Section 3.3
 *
 * Date             Modified by     Description of modification
 * 05/11/2000     IBM Corp.       Make more like Windows.
 */

#ifndef _nsRenderingContextOS2_h
#define _nsRenderingContextOS2_h

#include "nsIRenderingContext.h"
#include "nsFont.h"
#include "nsCRT.h"
#include "nsTransform2D.h"
#include "nscoord.h"
#include "nsDrawingSurfaceOS2.h"
#include "nsImageOS2.h"
#include "nsRenderingContextImpl.h"

class nsIDeviceContext;
class nsIFontMetrics;
class nsString;
class nsIWidget;
class nsPoint;
class nsRect;

class GraphicsState;
class nsDrawingSurfaceOS2;

class nsRenderingContextOS2 : public nsRenderingContextImpl
{
public:
  nsRenderingContextOS2();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIDeviceContext* aContext, nsIWidget *aWindow);
  NS_IMETHOD Init(nsIDeviceContext* aContext, nsDrawingSurface aSurface);

  NS_IMETHOD Reset(void);

  NS_IMETHOD GetDeviceContext(nsIDeviceContext *&aContext);

  NS_IMETHOD LockDrawingSurface(PRInt32 aX, PRInt32 aY, PRUint32 aWidth, PRUint32 aHeight,
                                void **aBits, PRInt32 *aStride, PRInt32 *aWidthBytes,
                                PRUint32 aFlags);
  NS_IMETHOD UnlockDrawingSurface(void);

  NS_IMETHOD SelectOffScreenDrawingSurface(nsDrawingSurface aSurface);
  NS_IMETHOD GetDrawingSurface(nsDrawingSurface *aSurface);
  NS_IMETHOD GetHints(PRUint32& aResult);

  NS_IMETHOD PushState(void);
  NS_IMETHOD PopState(PRBool &aClipState);

  NS_IMETHOD IsVisibleRect(const nsRect& aRect, PRBool &aClipState);

  NS_IMETHOD SetClipRect(const nsRect& aRect, nsClipCombine aCombine, PRBool &aCilpState);
  NS_IMETHOD GetClipRect(nsRect &aRect, PRBool &aClipState);
  NS_IMETHOD SetClipRegion(const nsIRegion& aRegion, nsClipCombine aCombine, PRBool &aClipState);
  NS_IMETHOD CopyClipRegion(nsIRegion &aRegion);
  NS_IMETHOD GetClipRegion(nsIRegion **aRegion);

  NS_IMETHOD SetLineStyle(nsLineStyle aLineStyle);
  NS_IMETHOD GetLineStyle(nsLineStyle &aLineStyle);

  NS_IMETHOD SetColor(nscolor aColor);
  NS_IMETHOD GetColor(nscolor &aColor) const;

  NS_IMETHOD SetFont(const nsFont& aFont);
  NS_IMETHOD SetFont(nsIFontMetrics *aFontMetrics);

  NS_IMETHOD GetFontMetrics(nsIFontMetrics *&aFontMetrics);

  NS_IMETHOD Translate(nscoord aX, nscoord aY);
  NS_IMETHOD Scale(float aSx, float aSy);
  NS_IMETHOD GetCurrentTransform(nsTransform2D *&aTransform);

  NS_IMETHOD CreateDrawingSurface(nsRect *aBounds, PRUint32 aSurfFlags, nsDrawingSurface &aSurface);
  NS_IMETHOD DestroyDrawingSurface(nsDrawingSurface aDS);

  NS_IMETHOD DrawLine(nscoord aX0, nscoord aY0, nscoord aX1, nscoord aY1);
  NS_IMETHOD DrawPolyline(const nsPoint aPoints[], PRInt32 aNumPoints);

  NS_IMETHOD DrawRect(const nsRect& aRect);
  NS_IMETHOD DrawRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  NS_IMETHOD FillRect(const nsRect& aRect);
  NS_IMETHOD FillRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  NS_IMETHOD InvertRect(const nsRect& aRect);
  NS_IMETHOD InvertRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  NS_IMETHOD DrawPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);
  NS_IMETHOD FillPolygon(const nsPoint aPoints[], PRInt32 aNumPoints);

  NS_IMETHOD DrawEllipse(const nsRect& aRect);
  NS_IMETHOD DrawEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);
  NS_IMETHOD FillEllipse(const nsRect& aRect);
  NS_IMETHOD FillEllipse(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight);

  NS_IMETHOD DrawArc(const nsRect& aRect,
                     float aStartAngle, float aEndAngle);
  NS_IMETHOD DrawArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                     float aStartAngle, float aEndAngle);
  NS_IMETHOD FillArc(const nsRect& aRect,
                     float aStartAngle, float aEndAngle);
  NS_IMETHOD FillArc(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                     float aStartAngle, float aEndAngle);

  NS_IMETHOD GetWidth(char aC, nscoord& aWidth);
  NS_IMETHOD GetWidth( PRUnichar aC, nscoord& aWidth,
                      PRInt32 *aFontID = nsnull);
  NS_IMETHOD GetWidth( const nsString& aString, nscoord& aWidth,
                      PRInt32 *aFontID = nsnull);
  NS_IMETHOD GetWidth(const char* aString, nscoord& aWidth);
  NS_IMETHOD GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth);
  NS_IMETHOD GetWidth( const PRUnichar* aString, PRUint32 aLength,
                      nscoord& aWidth, PRInt32 *aFontID = nsnull);
#if 0 // OS2TODO
  NS_IMETHOD GetWidth(const PRUnichar *aString,
                      PRInt32          aLength,
                      PRInt32          aAvailWidth,
                      PRInt32*         aBreaks,
                      PRInt32          aNumBreaks,
                      nscoord&         aWidth,
                      PRInt32&         aNumCharsFit,
                      PRInt32*         aFontID);
#endif
 
  NS_IMETHOD DrawString( const char *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        const nscoord* aSpacing = nsnull);
  NS_IMETHOD DrawString( const PRUnichar *aString, PRUint32 aLength,
                        nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull);
  NS_IMETHOD DrawString( const nsString& aString,
                        nscoord aX, nscoord aY,
                        PRInt32 aFontID = -1,
                        const nscoord* aSpacing = nsnull);
 
   NS_IMETHOD DrawImage( nsIImage *aImage, nscoord aX, nscoord aY);
   NS_IMETHOD DrawImage( nsIImage *aImage, nscoord aX, nscoord aY,
                         nscoord aWidth, nscoord aHeight); 
   NS_IMETHOD DrawImage( nsIImage *aImage, const nsRect& aRect);
   NS_IMETHOD DrawImage( nsIImage *aImage, const nsRect& aSRect, const nsRect& aDRect);

   NS_IMETHOD DrawTile(nsIImage *aImage,nscoord aX0,nscoord aY0,nscoord aX1,nscoord aY1,
                        nscoord aWidth,nscoord aHeight);
 

  NS_IMETHOD CopyOffScreenBits(nsDrawingSurface aSrcSurf, PRInt32 aSrcX, PRInt32 aSrcY,
                               const nsRect &aDestBounds, PRUint32 aCopyFlags);
  //~~~
  NS_IMETHOD RetrieveCurrentNativeGraphicData(PRUint32 * ngd);


#if 0 // OS2TODO
#ifdef MOZ_MATHML
  NS_IMETHOD
  GetBoundingMetrics(const char*        aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics);

  NS_IMETHOD
  GetBoundingMetrics(const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics,
                     PRInt32*           aFontID);
#endif
#endif

   // Convert XP-rects to OS/2 space.
   // World coordinates given & required, double inclusive rcl wanted.
   void NS2PM_ININ( const nsRect &in, RECTL &rcl);
   // World coordinates given & required, inclusive-exclusive rcl wanted.
   void NS2PM_INEX( const nsRect &in, RECTL &rcl);

   // Convert OS/2 rects to XP space.
   // World coords given & required, double-inclusive rcl wanted
   void PM2NS_ININ( const RECTL &in, nsRect &out);

   // Convert XP points to OS/2 space.
   void NS2PM( PPOINTL aPointl, ULONG cPointls);

private:
  // ConditionRect is used to fix coordinate overflow problems for
  // rectangles after they are transformed to screen coordinates
  NS_IMETHOD ConditionRect( nscoord &x, nscoord &y, nscoord &w, nscoord &h );

  nsresult CommonInit(void);
  nsresult SetupPS( HPS oldPS, HPS newPS);
  void     GetTargetHeight( PRUint32 &ht);

  // Colour/font setting; call before drawing things.
  void SetupDrawingColor( BOOL bForce = FALSE);
  void SetupFontAndColor( BOOL bForce = FALSE);

  // Primitive draw-ers
  void PMDrawRect( nsRect &rect, BOOL fill);
  void PMDrawPoly( const nsPoint aPoints[], PRInt32 aNumPoints, PRBool bFilled);
  void PMDrawArc( nsRect &rect, PRBool bFilled, PRBool bFull, PRInt32 start=0, PRInt32 end=0);

protected:

  ~nsRenderingContextOS2();

#if 0 // OS2TODO
  /** ---------------------------------------------------
   *  See documentation in nsIRenderingContextImpl.h
   *	@update 4/01/00 dwc
   */
  virtual PRBool CanTile(nscoord aWidth,nscoord aHeight);
#endif

   nsIDeviceContext    *mContext;         // device context
   nsDrawingSurfaceOS2 *mSurface;         // draw things here
   nsDrawingSurfaceOS2 *mFrontSurface;    // if offscreen, this is onscreen
   nscolor              mColor;           // current colour
   nsLineStyle          mLineStyle;       // current line style
   nsTransform2D        mTMatrix;         // current xform matrix
   float                mP2T;             // cache pix-2-app factor from DC
   GraphicsState       *mStateStack;      // stack of graphics states
   nsIFontMetrics      *mFontMetrics;     // current font
   nsIFontMetrics      *mCurrFontMetrics; // currently selected font
   nscolor              mCurrDrawingColor;// currently selected drawing color
  PRUint8           *mGammaTable;
   nscolor              mCurrTextColor;   // currently selected text color
   nsLineStyle          mCurrLineStyle;   // currently selected line style
   HDC                  mDC;  
   nsIWidget            *mDCOwner;

};

inline void nsRenderingContextOS2::GetTargetHeight( PRUint32 &ht)
{
   PRUint32 on, dummy, off;
   mSurface->GetDimensions( &dummy, &on);
   if( mSurface != mFrontSurface)
   {
      mFrontSurface->GetDimensions( &dummy, &off);
      if( off < on) on = off;
   }
   ht = on;
}

#endif
