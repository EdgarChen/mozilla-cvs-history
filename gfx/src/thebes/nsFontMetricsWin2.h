/* -*- Mode: C++; tab-width: 20; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * mozilla.org.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Stuart Parmenter <pavlov@pavlov.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef NSTHEBESFONTMETRICS__H__
#define NSTHEBESFONTMETRICS__H__

#include "nsIThebesFontMetrics.h"
#include "nsThebesRenderingContext.h"
#include "nsCOMPtr.h"
#include "nsThebesDeviceContext.h"
#include "nsIAtom.h"

#include <windows.h>

class nsFontMetricsWin : public nsIThebesFontMetrics
{
public:
    nsFontMetricsWin();
    virtual ~nsFontMetricsWin();

    NS_DECL_ISUPPORTS

    NS_IMETHOD  Init(const nsFont& aFont, nsIAtom* aLangGroup,
                     nsIDeviceContext *aContext);
    NS_IMETHOD  Destroy();
    NS_IMETHOD  GetXHeight(nscoord& aResult);
    NS_IMETHOD  GetSuperscriptOffset(nscoord& aResult);
    NS_IMETHOD  GetSubscriptOffset(nscoord& aResult);
    NS_IMETHOD  GetStrikeout(nscoord& aOffset, nscoord& aSize);
    NS_IMETHOD  GetUnderline(nscoord& aOffset, nscoord& aSize);
    NS_IMETHOD  GetHeight(nscoord &aHeight);
    NS_IMETHOD  GetInternalLeading(nscoord &aLeading);
    NS_IMETHOD  GetExternalLeading(nscoord &aLeading);
    NS_IMETHOD  GetEmHeight(nscoord &aHeight);
    NS_IMETHOD  GetEmAscent(nscoord &aAscent);
    NS_IMETHOD  GetEmDescent(nscoord &aDescent);
    NS_IMETHOD  GetMaxHeight(nscoord &aHeight);
    NS_IMETHOD  GetMaxAscent(nscoord &aAscent);
    NS_IMETHOD  GetMaxDescent(nscoord &aDescent);
    NS_IMETHOD  GetMaxAdvance(nscoord &aAdvance);
    NS_IMETHOD  GetLangGroup(nsIAtom** aLangGroup);
    NS_IMETHOD  GetFontHandle(nsFontHandle &aHandle);
    NS_IMETHOD  GetAveCharWidth(nscoord& aAveCharWidth);
    NS_IMETHOD  GetSpaceWidth(nscoord& aSpaceCharWidth);
    NS_IMETHOD  GetLeading(nscoord& aLeading);
    NS_IMETHOD  GetNormalLineHeight(nscoord& aLineHeight);




    virtual nsresult GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth,
                              nsThebesRenderingContext *aContext);
    // aCachedOffset will be updated with a new offset.
    virtual nsresult GetWidth(const PRUnichar* aString, PRUint32 aLength,
                              nscoord& aWidth, PRInt32 *aFontID,
                              nsThebesRenderingContext *aContext);

    // Get the text dimensions for this string
    virtual nsresult GetTextDimensions(const PRUnichar* aString,
                                       PRUint32 aLength,
                                       nsTextDimensions& aDimensions, 
                                       PRInt32* aFontID);
    virtual nsresult GetTextDimensions(const char*         aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID);
    virtual nsresult GetTextDimensions(const PRUnichar*    aString,
                                       PRInt32             aLength,
                                       PRInt32             aAvailWidth,
                                       PRInt32*            aBreaks,
                                       PRInt32             aNumBreaks,
                                       nsTextDimensions&   aDimensions,
                                       PRInt32&            aNumCharsFit,
                                       nsTextDimensions&   aLastWordDimensions,
                                       PRInt32*            aFontID);

    // Draw a string using this font handle on the surface passed in.  
    virtual nsresult DrawString(const char *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                const nscoord* aSpacing,
                                nsThebesRenderingContext *aContext);
    // aCachedOffset will be updated with a new offset.
    virtual nsresult DrawString(const PRUnichar* aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID,
                                const nscoord* aSpacing,
                                nsThebesRenderingContext *aContext);

#ifdef MOZ_MATHML
    // These two functions get the bounding metrics for this handle,
    // updating the aBoundingMetrics in Points.  This means that the
    // caller will have to update them to twips before passing it
    // back.
    virtual nsresult GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                        nsBoundingMetrics &aBoundingMetrics);
    // aCachedOffset will be updated with a new offset.
    virtual nsresult GetBoundingMetrics(const PRUnichar *aString,
                                        PRUint32 aLength,
                                        nsBoundingMetrics &aBoundingMetrics,
                                        PRInt32 *aFontID);
#endif /* MOZ_MATHML */

    // Set the direction of the text rendering
    virtual nsresult SetRightToLeftText(PRBool aIsRTL);




protected:
    void RealizeFont(HDC dc);
    void FillLogFont(LOGFONTW* logFont, const nsString *aFontName);
    PRInt32 MeasureOrDrawUniscribe(nsThebesRenderingContext *aContext,
                                   const PRUnichar *aString, PRUint32 aLength,
                                   PRBool aDraw, PRInt32 aX, PRInt32 aY, const PRInt32 *aSpacing);

    cairo_font_face_t   *MakeCairoFontFace(const nsString *aFontName);
    cairo_scaled_font_t *MakeCairoScaledFont(cairo_t *cr, cairo_font_face_t *aFontFace);


public:
    nsStringArray       mFonts;
    nsString            mGeneric;
    PRInt32             mGenericIndex;


private:
    LOGFONTW mLogFont;
    cairo_font_face_t *mCairoFontFace;
    cairo_scaled_font_t *mCairoFont;


    nsThebesDeviceContext *mDeviceContext;
    nsCOMPtr<nsIAtom> mLangGroup;
    float mDev2App;


    nscoord             mExternalLeading;
    nscoord             mInternalLeading;
    nscoord             mEmHeight;
    nscoord             mEmAscent;
    nscoord             mEmDescent;
    nscoord             mMaxHeight;
    nscoord             mMaxAscent;
    nscoord             mMaxDescent;
    nscoord             mMaxAdvance;
    nscoord             mAveCharWidth;
    nscoord             mXHeight;
    nscoord             mSuperscriptOffset;
    nscoord             mSubscriptOffset;
    nscoord             mStrikeoutSize;
    nscoord             mStrikeoutOffset;
    nscoord             mUnderlineSize;
    nscoord             mUnderlineOffset;
    nscoord             mSpaceWidth;
};

#endif /* NSTHEBESFONTMETRICS__H__ */
