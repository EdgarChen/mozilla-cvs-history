/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: NPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Lars Knoll <knoll@kde.org>
 *   Zack Rusin <zack@kde.org>
 *	 John C. Griggs <johng@corel.com>
 *
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the NPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the NPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef nsImageQt_h___
#define nsImageQt_h___

#include "nsIImage.h"
#include "nsRect.h"

#include <qpixmap.h>
#include <qimage.h>

#undef Bool

class nsImageQt : public nsIImage
{
public:
    nsImageQt();
    virtual ~nsImageQt();

    NS_DECL_ISUPPORTS

    /**
     *  @see nsIImage.h
     */
    virtual nsresult    Init(PRInt32 aWidth, PRInt32 aHeight,
                             PRInt32 aDepth, nsMaskRequirements aMaskRequirements);

    virtual PRInt32     GetBytesPix();
    virtual PRBool      GetIsRowOrderTopToBottom() {return PR_TRUE;}
    virtual PRInt32     GetHeight();
    virtual PRInt32     GetWidth();
    virtual PRUint8     *GetBits();
    virtual PRInt32     GetLineStride();
    virtual PRBool      GetHasAlphaMask() {return mAlphaBits != nsnull;}
    virtual PRUint8     *GetAlphaBits();
    virtual PRInt32     GetAlphaLineStride();

    virtual void ImageUpdated(nsIDeviceContext *aContext,
                              PRUint8 aFlags,nsRect *aUpdateRect);

    virtual nsresult    Optimize(nsIDeviceContext *aContext);

    virtual nsColorMap *GetColorMap();

    NS_IMETHOD Draw(nsIRenderingContext &aContext,
                    nsIDrawingSurface* aSurface,
                    PRInt32 aX, PRInt32 aY,
                    PRInt32 aWidth, PRInt32 aHeight);

    NS_IMETHOD Draw(nsIRenderingContext &aContext,
                    nsIDrawingSurface *aSurface,
                    PRInt32 aSX, PRInt32 aSY,
                    PRInt32 aSWidth, PRInt32 aSHeight,
                    PRInt32 aDX, PRInt32 aDY,
                    PRInt32 aDWidth, PRInt32 aDHeight);

    NS_IMETHOD DrawTile(nsIRenderingContext &aContext,
                        nsIDrawingSurface* aSurface,
                        PRInt32 aSXOffset, PRInt32 aSYOffset,
                        PRInt32 aPadX, PRInt32 aPadY,
                        const nsRect &aTileRect);

    NS_IMETHOD DrawToImage(nsIImage *aDstImage, PRInt32 aDX, PRInt32 aDY,
                           PRInt32 aDWidth, PRInt32 aDHeight);

    virtual PRInt8 GetAlphaDepth() { return(mAlphaDepth); }

    virtual void        *GetBitInfo();

    NS_IMETHOD   LockImagePixels(PRBool aMaskPixels);

    NS_IMETHOD   UnlockImagePixels(PRBool aMaskPixels);

private:
    /**
     * Calculate the amount of memory needed for the initialization of the
     * image
     */
    void updatePixmap();

private:
    PRInt32   mWidth;
    PRInt32   mHeight;
    PRInt32   mDepth;
    PRInt32   mRowBytes;
    PRUint8   *mImageBits;

    PRInt8    mAlphaDepth;    // alpha layer depth
    PRInt16   mAlphaRowBytes; // alpha bytes per row
    PRUint8   *mAlphaBits;


    PRInt8    mNumBytesPixel;

    nsRect    mDecodedRect;   // Keeps track of what part of image has been decoded.

    QPixmap pixmap;
    PRBool pixmapDirty;

    PRUint32  mID;
};

#endif
