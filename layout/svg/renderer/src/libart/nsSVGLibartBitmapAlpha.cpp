/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is the Mozilla SVG project.
 *
 * The Initial Developer of the Original Code is
 * Alex Fritze.
 * Portions created by the Initial Developer are Copyright (C) 2002
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Alex Fritze <alex@croczilla.com> (original author)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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


#include "nsCOMPtr.h"
#include "nsISVGLibartBitmap.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsPresContext.h"
#include "nsRect.h"
#include "nsIImage.h"
#include "nsIComponentManager.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

/**
 * \addtogroup libart_renderer Libart Rendering Engine
 * @{
 */
////////////////////////////////////////////////////////////////////////
/**
 * A libart-bitmap implementation based on gfxIImageFrame that supports
 * compositing through a very ugly hack. Bitmap data is written to a
 * temporary buffer in RGBA format until the bitmap is flushed and then
 * the alpha data is extracted to a separate buffer and the remaining
 * RGB data placed in the actual bitmap buffer. This is because libart
 * does not seem to support writing alpha information to a separate
 * alpha buffer.
 */
class nsSVGLibartBitmapAlpha : public nsISVGLibartBitmap
{
public:
  nsSVGLibartBitmapAlpha();
  ~nsSVGLibartBitmapAlpha();
  nsresult Init(nsIRenderingContext *ctx,
                nsPresContext* presContext,
                const nsRect & rect);

  // nsISupports interface:
  NS_DECL_ISUPPORTS

  // nsISVGLibartBitmap interface:
  NS_IMETHOD_(PRUint8 *) GetBits();

  NS_IMETHOD_(nsISVGLibartBitmap::PixelFormat) GetPixelFormat();
  NS_IMETHOD_(int) GetLineStride();

  NS_IMETHOD_(int) GetWidth();
  NS_IMETHOD_(int) GetHeight();
  NS_IMETHOD_(void) LockRenderingContext(const nsRect& rect,
                                         nsIRenderingContext**ctx);
  NS_IMETHOD_(void) UnlockRenderingContext();
  NS_IMETHOD_(void) Flush();
  
private:
  void LockBuffer();
  void UnlockBuffer();

  PRBool mLocked;
  nsCOMPtr<nsIRenderingContext> mRenderingContext;
  nsCOMPtr<imgIContainer> mContainer;
  nsCOMPtr<gfxIImageFrame> mBuffer;
  nsRect mRectTwips;
  nsRect mRect;
  PRUint8 *mTempBits;
  PRInt32 mTempLineStride;
};

/** @} */

//----------------------------------------------------------------------
// implementation:

nsSVGLibartBitmapAlpha::nsSVGLibartBitmapAlpha()
    : mLocked(PR_FALSE), mTempBits(nsnull)
{
}

nsSVGLibartBitmapAlpha::~nsSVGLibartBitmapAlpha()
{
    if (mTempBits) {
      delete [] mTempBits;
      mTempBits = nsnull;
    }
}

nsresult
nsSVGLibartBitmapAlpha::Init(nsIRenderingContext* ctx,
                               nsPresContext* presContext,
                               const nsRect & rect)
{
  mRenderingContext = ctx;

  PRUint32 tempSize;
  float twipsPerPx;

  twipsPerPx = presContext->PixelsToTwips();
  mRectTwips.x = (nscoord)(rect.x*twipsPerPx);
  mRectTwips.y = (nscoord)(rect.y*twipsPerPx);
  mRectTwips.width = (nscoord)(rect.width*twipsPerPx);
  mRectTwips.height = (nscoord)(rect.height*twipsPerPx);
  mRect = rect;
  
  mContainer = do_CreateInstance("@mozilla.org/image/container;1");
  mContainer->Init(rect.width, rect.height, nsnull);
    
  mBuffer = do_CreateInstance("@mozilla.org/gfx/image/frame;2");
  mBuffer->Init(0, 0, rect.width, rect.height, gfxIFormats::RGB_A8, 24);
  mContainer->AppendFrame(mBuffer);

  mTempLineStride = rect.width * 4; // i.e. 4-bytes per pixel (RGBA)
  mTempLineStride = (mTempLineStride + 3) & ~0x3; // 32-bit align
  tempSize = mTempLineStride * rect.height;
  mTempBits = new PRUint8[tempSize];
  if (!mTempBits) return NS_ERROR_OUT_OF_MEMORY;

  memset(mTempBits, 0, tempSize); // initialise to black and transparent
  
  return NS_OK;
}

nsresult
NS_NewSVGLibartBitmap(nsISVGLibartBitmap **result,
                      nsIRenderingContext *ctx,
                      nsPresContext* presContext,
                      const nsRect & rect)
{
  nsSVGLibartBitmapAlpha* bm = new nsSVGLibartBitmapAlpha();
  if (!bm) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(bm);

  nsresult rv = bm->Init(ctx, presContext, rect);

  if (NS_FAILED(rv)) {
    NS_RELEASE(bm);
    return rv;
  }
  
  *result = bm;
  return rv;
}

//----------------------------------------------------------------------
// nsISupports methods:

NS_IMPL_ADDREF(nsSVGLibartBitmapAlpha)
NS_IMPL_RELEASE(nsSVGLibartBitmapAlpha)

NS_INTERFACE_MAP_BEGIN(nsSVGLibartBitmapAlpha)
  NS_INTERFACE_MAP_ENTRY(nsISVGLibartBitmap)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

//----------------------------------------------------------------------
// Implementation helpers:
void
nsSVGLibartBitmapAlpha::LockBuffer()
{
  // XXX not really sure if this makes sense now that we're using a temporary
  // buffer. Really we should lock the temporary buffer instead.
  if (mLocked) return;

  mBuffer->LockImageData();
  mBuffer->LockAlphaData();
  mLocked = PR_TRUE;
}

void
nsSVGLibartBitmapAlpha::UnlockBuffer()
{
  if (!mLocked) return;

  mBuffer->UnlockAlphaData();
  mBuffer->UnlockImageData();
  mLocked = PR_FALSE;
}


//----------------------------------------------------------------------
// nsISVGLibartBitmap methods:

NS_IMETHODIMP_(PRUint8 *)
nsSVGLibartBitmapAlpha::GetBits()
{
  LockBuffer();
  return mTempBits;
}


NS_IMETHODIMP_(nsISVGLibartBitmap::PixelFormat)
nsSVGLibartBitmapAlpha::GetPixelFormat()
{
#if defined(XP_WIN) || defined(XP_BEOS) || defined(MOZ_WIDGET_PHOTON)
  return PIXEL_FORMAT_32_BGRA;
#else
  return PIXEL_FORMAT_32_RGBA;
#endif
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapAlpha::GetLineStride()
{
  return (int) mTempLineStride;
}


NS_IMETHODIMP_(int)
nsSVGLibartBitmapAlpha::GetWidth()
{
  return mRect.width; 
}

NS_IMETHODIMP_(int)
nsSVGLibartBitmapAlpha::GetHeight()
{
  return mRect.height;
}

NS_IMETHODIMP_(void)
nsSVGLibartBitmapAlpha::LockRenderingContext(const nsRect& rect,
                                             nsIRenderingContext** ctx)
{
  // doesn't work on alpha bitmap!
  *ctx = nsnull;
}

NS_IMETHODIMP_(void)
nsSVGLibartBitmapAlpha::UnlockRenderingContext()
{
  // doesn't work on alpha bitmap!
}

NS_IMETHODIMP_(void)
nsSVGLibartBitmapAlpha::Flush()
{
  UnlockBuffer();

  nsCOMPtr<nsIDeviceContext> ctx;
  mRenderingContext->GetDeviceContext(*getter_AddRefs(ctx));

  nsCOMPtr<nsIInterfaceRequestor> ireq(do_QueryInterface(mBuffer));
  if (ireq) {
    nsCOMPtr<nsIImage> img(do_GetInterface(ireq));

    // Flip image first so we don't have to flip the alpha buffer separately
    if (!img->GetIsRowOrderTopToBottom()) {
      // XXX we need to flip the image. This is silly. Blt should take
      // care of it
      int stride = mTempLineStride;
      int height = GetHeight();
      PRUint8* bits = mTempBits;
      PRUint8* rowbuf = new PRUint8[stride];
      for (int row=0; row<height/2; ++row) {
        memcpy(rowbuf, bits+row*stride, stride);
        memcpy(bits+row*stride, bits+(height-1-row)*stride, stride);
        memcpy(bits+(height-1-row)*stride, rowbuf, stride);
      }
      delete[] rowbuf;
    }

    int width = GetWidth();
    int height = GetHeight();

    const PRUint8* const src = mTempBits;
    PRUint8* const dst = img->GetBits();
    PRUint8* const dstAlpha = img->GetAlphaBits();

    int dstStride = img->GetLineStride();
    int dstAlphaStride = img->GetAlphaLineStride();

    // Copy the bits from the temporary buffer to the nsImage image buffer and
    // alpha buffer
    for (int row = 0; row < height; ++row) {
      // XXX Definitely not the most efficient way of doing this but it is
      // consistent with nsImageWin::CreateImageWithAlphaBits
      const PRUint8 *srcRow = src + row * mTempLineStride;
      PRUint8 *dstRow = dst + row * dstStride;
      PRUint8 *dstAlphaRow = dstAlpha + row * dstAlphaStride;
      for (int x = 0; x < width; x++) {
#ifdef XP_MACOSX
        *dstRow++      = 0;
#endif
        *dstRow++      = *srcRow++;
        *dstRow++      = *srcRow++;
        *dstRow++      = *srcRow++;
        *dstAlphaRow++ = *srcRow++;
      }
    }
    
    nsRect r(0, 0, GetWidth(), GetHeight());
    img->ImageUpdated(ctx, nsImageUpdateFlags_kBitsChanged, &r);
  }
  
  mContainer->DecodingComplete();
  mRenderingContext->DrawTile(mContainer, mRectTwips.x, mRectTwips.y,
    &mRectTwips);
}

