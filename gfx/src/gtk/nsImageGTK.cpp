/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is mozilla.org code.
 * 
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 2000 Netscape Communications Corporation.  All
 * Rights Reserved.
 * 
 * Contributor(s):
 *    Stuart Parmenter <pavlov@netscape.com>
 *    Tim Rowley <tor@cs.brown.edu> -- 8bit alpha compositing
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "nsImageGTK.h"
#include "nsRenderingContextGTK.h"

#include "nspr.h"

#define IsFlagSet(a,b) ((a) & (b))

// Defining this will trace the allocation of images.  This includes
// ctor, dtor and update.
#undef TRACE_IMAGE_ALLOCATION

#undef CHEAP_PERFORMANCE_MEASURMENT

/* XXX we are simply creating a GC and setting its function to Copy.
   we shouldn't be doing this every time this method is called.  this creates
   way more trips to the server than we should be doing so we are creating a
   static one.
*/
static GdkGC *s1bitGC = nsnull;
static GdkGC *sXbitGC = nsnull;

NS_IMPL_ISUPPORTS1(nsImageGTK, nsIImage)

//------------------------------------------------------------

nsImageGTK::nsImageGTK()
{
  NS_INIT_REFCNT();
  mImageBits = nsnull;
  mWidth = 0;
  mHeight = 0;
  mDepth = 0;
  mAlphaBits = nsnull;
  mAlphaPixmap = nsnull;
  mImagePixmap = nsnull;
  mAlphaDepth = 0;
  mRowBytes = 0;
  mSizeImage = 0;
  mAlphaHeight = 0;
  mAlphaWidth = 0;
  mConvertedBits = nsnull;
#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::nsImageGTK(this=%p)\n",
         this);
#endif
}

//------------------------------------------------------------

nsImageGTK::~nsImageGTK()
{
  if(nsnull != mImageBits) {
    delete[] (PRUint8*)mImageBits;
    mImageBits = nsnull;
  }

  if (nsnull != mAlphaBits) {
    delete[] (PRUint8*)mAlphaBits;
    mAlphaBits = nsnull;
  }

  if (mAlphaPixmap) {
    gdk_pixmap_unref(mAlphaPixmap);
  }

  if (mImagePixmap) {
    gdk_pixmap_unref(mImagePixmap);
  }

#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::~nsImageGTK(this=%p)\n",
         this);
#endif
}

//------------------------------------------------------------

nsresult nsImageGTK::Init(PRInt32 aWidth, PRInt32 aHeight,
                          PRInt32 aDepth, nsMaskRequirements aMaskRequirements)
{
  g_return_val_if_fail ((aWidth != 0) || (aHeight != 0), NS_ERROR_FAILURE);

  if (nsnull != mImageBits) {
   delete[] (PRUint8*)mImageBits;
   mImageBits = nsnull;
  }

  if (nsnull != mAlphaBits) {
    delete[] (PRUint8*)mAlphaBits;
    mAlphaBits = nsnull;
  }

  if (nsnull != mAlphaPixmap) {
    gdk_pixmap_unref(mAlphaPixmap);
    mAlphaPixmap = nsnull;
  }

  SetDecodedRect(0,0,0,0);  //init


  // mImagePixmap gets created once per unique image bits in Draw()
  // ImageUpdated(nsImageUpdateFlags_kBitsChanged) can cause the
  // image bits to change and mImagePixmap will be unrefed and nulled.
  if (nsnull != mImagePixmap) {
    gdk_pixmap_unref(mImagePixmap);
    mImagePixmap = nsnull;
  }

  if (24 == aDepth) {
    mNumBytesPixel = 3;
  } else {
    NS_ASSERTION(PR_FALSE, "unexpected image depth");
    return NS_ERROR_UNEXPECTED;
  }

  mWidth = aWidth;
  mHeight = aHeight;
  mDepth = aDepth;
  mIsTopToBottom = PR_TRUE;

#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::Init(this=%p,%d,%d,%d,%d)\n",
         this,
         aWidth,
         aHeight,
         aDepth,
         aMaskRequirements);
#endif

  // create the memory for the image
  ComputeMetrics();

  mImageBits = (PRUint8*) new PRUint8[mSizeImage];

  switch(aMaskRequirements)
  {
    case nsMaskRequirements_kNoMask:
      mAlphaBits = nsnull;
      mAlphaWidth = 0;
      mAlphaHeight = 0;
      break;

    case nsMaskRequirements_kNeeds1Bit:
      mAlphaRowBytes = (aWidth + 7) / 8;
      mAlphaDepth = 1;

      // 32-bit align each row
      mAlphaRowBytes = (mAlphaRowBytes + 3) & ~0x3;

      mAlphaBits = new PRUint8[mAlphaRowBytes * aHeight];
      mAlphaWidth = aWidth;
      mAlphaHeight = aHeight;
      break;

    case nsMaskRequirements_kNeeds8Bit:
      mAlphaRowBytes = aWidth;
      mAlphaDepth = 8;

      // 32-bit align each row
      mAlphaRowBytes = (mAlphaRowBytes + 3) & ~0x3;
      mAlphaBits = new PRUint8[mAlphaRowBytes * aHeight];
      mAlphaWidth = aWidth;
      mAlphaHeight = aHeight;
      g_print("TODO: want an 8bit mask for an image..\n");
      break;
  }

  return NS_OK;
}

//------------------------------------------------------------

PRInt32 nsImageGTK::GetHeight()
{
  return mHeight;
}

PRInt32 nsImageGTK::GetWidth()
{
  return mWidth;
}

PRUint8 *nsImageGTK::GetBits()
{
  return mImageBits;
}

void *nsImageGTK::GetBitInfo()
{
  return nsnull;
}

PRInt32 nsImageGTK::GetLineStride()
{
  return mRowBytes;
}

nsColorMap *nsImageGTK::GetColorMap()
{
  return nsnull;
}

PRBool nsImageGTK::IsOptimized()
{
  return PR_TRUE;
}

PRUint8 *nsImageGTK::GetAlphaBits()
{
  return mAlphaBits;
}

PRInt32 nsImageGTK::GetAlphaWidth()
{
  return mAlphaWidth;
}

PRInt32 nsImageGTK::GetAlphaHeight()
{
  return mAlphaHeight;
}

PRInt32
nsImageGTK::GetAlphaLineStride()
{
  return mAlphaRowBytes;
}

nsIImage *nsImageGTK::DuplicateImage()
{
  return nsnull;
}

void nsImageGTK::SetAlphaLevel(PRInt32 aAlphaLevel)
{
}

PRInt32 nsImageGTK::GetAlphaLevel()
{
  return 0;
}

void nsImageGTK::MoveAlphaMask(PRInt32 aX, PRInt32 aY)
{
}

//------------------------------------------------------------

// set up the palette to the passed in color array, RGB only in this array
void nsImageGTK::ImageUpdated(nsIDeviceContext *aContext,
                              PRUint8 aFlags,
                              nsRect *aUpdateRect)
{
#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::ImageUpdated(this=%p,%d)\n",
         this,
         aFlags);
#endif

  mFlags = aFlags; // this should be 0'd out by Draw()

}

#ifdef CHEAP_PERFORMANCE_MEASURMENT
static PRTime gConvertTime, gAlphaTime, gAlphaEnd, gStartTime, gPixmapTime, gEndTime;
#endif

// Draw the bitmap, this method has a source and destination coordinates
NS_IMETHODIMP
nsImageGTK::Draw(nsIRenderingContext &aContext, nsDrawingSurface aSurface,
                 PRInt32 aSX, PRInt32 aSY, PRInt32 aSWidth, PRInt32 aSHeight,
                 PRInt32 aDX, PRInt32 aDY, PRInt32 aDWidth, PRInt32 aDHeight)
{
  g_return_val_if_fail ((aSurface != nsnull), NS_ERROR_FAILURE);

  nsDrawingSurfaceGTK *drawing = (nsDrawingSurfaceGTK*)aSurface;

  gdk_draw_rgb_image (drawing->GetDrawable(),
                      ((nsRenderingContextGTK&)aContext).GetGC(),
                      aDX, aDY, aDWidth, aDHeight,
                      GDK_RGB_DITHER_MAX,
                      mImageBits + mRowBytes * aSY + 3 * aDX,
                      mRowBytes);

  return NS_OK;
}

//------------------------------------------------------------
// 8-bit alpha composite drawing...
void
nsImageGTK::DrawComposited(nsIRenderingContext &aContext,
                           nsDrawingSurface aSurface,
                           PRInt32 aX, PRInt32 aY,
                           PRInt32 aWidth, PRInt32 aHeight)
{
  if ((aWidth != mWidth) || (aHeight != mHeight)) {
    aWidth = mWidth;
    aHeight = mHeight;
  }

  nsDrawingSurfaceGTK* drawing = (nsDrawingSurfaceGTK*) aSurface;

  GdkImage *image;
  GdkColormap *colormap;
  GdkVisual *visual;
    
//  image = gdk_image_get(drawing->GetDrawable(), aX, aY, aWidth, aHeight);
//  XImage *ximage = GDK_IMAGE_XIMAGE(image);
  colormap = gdk_rgb_get_cmap();
  visual = gdk_rgb_get_visual();
    
  unsigned char *target, *readData = new unsigned char[3*aWidth*aHeight];
  target = readData;

  Display *dpy = GDK_WINDOW_XDISPLAY(drawing->GetDrawable());
  Drawable drawable = GDK_WINDOW_XWINDOW(drawing->GetDrawable());

  // I hate clipping...
  PRUint32 surfaceWidth, surfaceHeight;
  drawing->GetDimensions(&surfaceWidth, &surfaceHeight);
  
  XImage *ximage = XCreateImage(dpy, GDK_VISUAL_XVISUAL(visual),
                                visual->depth, ZPixmap, 0, 0,
                                aWidth, aHeight, 8, 0);
  unsigned char *data = (unsigned char *)malloc(ximage->bytes_per_line*aHeight);
  ximage->data = (char *)data;

  int readX, readY;
  unsigned readWidth, readHeight, destX, destY;

  readX = aX; readY = aY;
  readWidth = aWidth; readHeight = aHeight;
  destX = 0;
  destY = 0;
  if (readY<0) {
    destY = -readY;
    readY = 0;
  }
  if (readX<0) {
    destX = -readX;
    readX = 0;
  }
  if (aY+aHeight>surfaceHeight) {
    readHeight = surfaceHeight-readY;
  }
  if (aX+aWidth>surfaceWidth) {
    readWidth = surfaceWidth-readX;
  }

//  fprintf(stderr, "aX=%d aY=%d, aWidth=%u aHeight=%u\n", aX, aY, aWidth, aHeight);
//  fprintf(stderr, "surfaceWidth=%u surfaceHeight=%u\n", surfaceWidth, surfaceHeight);
//  fprintf(stderr, "readX=%u readY=%u readWidth=%u readHeight=%u destX=%u destY=%u\n",
//          readX, readY, readWidth, readHeight, destX, destY);
    
  XGetSubImage(dpy, drawable, readX, readY, readWidth, readHeight, AllPlanes,
               ZPixmap, ximage, destX, destY);

//  XImage *ximage = XGetImage(dpy, drawable, 
//                             aX, aY, aWidth, aHeight, AllPlanes, ZPixmap);
//
//  unsigned char *data = (unsigned char *)ximage->data;

  PRBool isLSB;

  unsigned test = 1;
  isLSB = (((char *)&test)[0]) ? 1 : 0;

#if 0
  // set flipBits appropriately...
  PRBool flipBits =
    ( isLSB && ximage->bit_order != LSBFirst) ||
    (!isLSB && ximage->bit_order == LSBFirst);

  // flip bits
  if (flipBits)
    for (int i=0; i<ximage->height*ximage->bytes_per_line; i++)
      data[i] = 255-data[i];
#endif

  // set flipBytes appropriately...
  PRBool flipBytes = 
    ( isLSB && ximage->byte_order != LSBFirst) ||
    (!isLSB && ximage->byte_order == LSBFirst);

  // flip bytes
  if (flipBytes && (ximage->bitmap_unit!=8)) {
    for (int row=0; row<ximage->height; row++) {
      unsigned char *ptr = 
        (unsigned char*)ximage->data + row*ximage->bytes_per_line;
      if (ximage->bits_per_pixel==24) {  // Aurgh....
        for (int col=0;
             col<ximage->bytes_per_line;
             col+=(ximage->bits_per_pixel/8)) {
          unsigned char tmp;
          tmp = *ptr;
          *ptr = *(ptr+2);
          *(ptr+2) = tmp;
          ptr+=3;
        }
        continue;
      }
      
      for (int col=0; 
               col<ximage->bytes_per_line;
               col+=(ximage->bitmap_unit/8)) {
        unsigned char tmp;
        switch (ximage->bitmap_unit) {
        case 16:
          tmp = *ptr;
          *ptr = *(ptr+1);
          *(ptr+1) = tmp;
          ptr+=2;
        case 32:
          tmp = *ptr;
          *ptr = *(ptr+3);
          *(ptr+3) = tmp;
          tmp = *(ptr+1);
          *(ptr+1) = *(ptr+2);
          *(ptr+2) = tmp;
          ptr+=4;
        }
      }
    }
  }
 
  for (int row=0; row<ximage->height; row++) {
    unsigned char *ptr = 
      (unsigned char *)ximage->data + row*ximage->bytes_per_line;
    for (int col=0; col<ximage->width; col++) {
      unsigned pix;
      switch (ximage->bits_per_pixel) {
      case 1:
        pix = (*ptr>>(col%8))&1;
        if ((col%8)==7)
          ptr++;
        break;
      case 4:
        pix = (col&1)?(*ptr>>4):(*ptr&0xf);
        if (col&1)
          ptr++;
        break;
      case 8:
        pix = *ptr++;
        break;
      case 16:
        pix = *((short *)ptr);
        ptr+=2;
        break;
      case 24:
        if (isLSB)
          pix = (*(ptr+2)<<16) | (*(ptr+1)<<8) | *ptr;
        else
          pix = (*ptr<<16) | (*(ptr+1)<<8) | *(ptr+2);
        ptr+=3;
        break;
      case 32:
        pix = *((unsigned *)ptr);
        ptr+=4;
        break;
      }
      switch (visual->type) {
      case GDK_VISUAL_STATIC_GRAY:
      case GDK_VISUAL_GRAYSCALE:
      case GDK_VISUAL_STATIC_COLOR:
      case GDK_VISUAL_PSEUDO_COLOR:
        *target++ = colormap->colors[pix].red   >>8;
        *target++ = colormap->colors[pix].green >>8;
        *target++ = colormap->colors[pix].blue  >>8;
        break;
        
      case GDK_VISUAL_DIRECT_COLOR:
        *target++ = 
          colormap->colors[(pix&visual->red_mask)>>visual->red_shift].red       >> 8;
        *target++ = 
          colormap->colors[(pix&visual->green_mask)>>visual->green_shift].green >> 8;
        *target++ =
          colormap->colors[(pix&visual->blue_mask)>>visual->blue_shift].blue    >> 8;
        break;
        
      case GDK_VISUAL_TRUE_COLOR:
        if (ximage->bits_per_pixel>=24) {
          *target++ = ((pix&visual->red_mask)>>visual->red_shift);
          *target++ = ((pix&visual->green_mask)>>visual->green_shift);
          *target++ = ((pix&visual->blue_mask)>>visual->blue_shift);
        } else {
          *target++ = 
            ((pix&visual->red_mask)>>visual->red_shift)<<(8-visual->red_prec);
          *target++ = 
            ((pix&visual->green_mask)>>visual->green_shift)<<(8-visual->green_prec);
          *target++ = 
            ((pix&visual->blue_mask)>>visual->blue_shift)<<(8-visual->blue_prec);
        }
        break;
      }
    }
  }

  // now composite

  for (int y=0; y<mHeight; y++) {
    for (int i=0; i<mWidth; i++) {
      unsigned char *targetRow = readData+3*y*mWidth;
      unsigned char *imageRow = mImageBits+y*mRowBytes;
      unsigned char *alphaRow = mAlphaBits+y*mAlphaRowBytes;

      targetRow[3*i] =   MIN(255, (targetRow[3*i]*(255-alphaRow[i]) +
                                   imageRow[3*i]*alphaRow[i])>>8);
      targetRow[3*i+1] = MIN(255, (targetRow[3*i+1]*(255-alphaRow[i]) +
                                   imageRow[3*i+1]*alphaRow[i])>>8);
      targetRow[3*i+2] = MIN(255, (targetRow[3*i+2]*(255-alphaRow[i]) +
                                   imageRow[3*i+2]*alphaRow[i])>>8);
    }
  }

  GdkGC *imageGC = gdk_gc_new(drawing->GetDrawable());
  gdk_draw_rgb_image(drawing->GetDrawable(), imageGC,
                     aX, aY, mWidth, mHeight,
                     GDK_RGB_DITHER_MAX,
                     readData, 3*mWidth);
  gdk_gc_unref(imageGC);

  XDestroyImage(ximage);
  delete [] readData;
}

void nsImageGTK::CreateAlphaBitmap(PRInt32 aWidth, PRInt32 aHeight)
{
  XImage *x_image = nsnull;
  Pixmap pixmap = 0;
  Display *dpy = nsnull;
  Visual *visual = nsnull;

  // Create gc clip-mask on demand
  if (mAlphaBits && IsFlagSet(nsImageUpdateFlags_kBitsChanged, mFlags)) {

    if (!mAlphaPixmap) {
      mAlphaPixmap = gdk_pixmap_new(nsnull, aWidth, aHeight, 1);
    }

    /* get the X primitives */
    dpy = GDK_WINDOW_XDISPLAY(mAlphaPixmap);

    /* this is the depth of the pixmap that we are going to draw to.
       It's always a bitmap.  We're doing alpha here folks. */
    visual = GDK_VISUAL_XVISUAL(gdk_rgb_get_visual());

    // Make an image out of the alpha-bits created by the image library
    x_image = XCreateImage(dpy, visual,
                           1, /* visual depth...1 for bitmaps */
                           XYPixmap,
                           0, /* x offset, XXX fix this */
                           (char *)mAlphaBits,  /* cast away our sign. */
                           aWidth,
                           aHeight,
                           32,/* bitmap pad */
                           mAlphaRowBytes); /* bytes per line */

    x_image->bits_per_pixel=1;

    /* Image library always places pixels left-to-right MSB to LSB */
    x_image->bitmap_bit_order = MSBFirst;

    /* This definition doesn't depend on client byte ordering
       because the image library ensures that the bytes in
       bitmask data are arranged left to right on the screen,
       low to high address in memory. */
    x_image->byte_order = MSBFirst;
#if defined(IS_LITTLE_ENDIAN)
    // no, it's still MSB XXX check on this!!
    //      x_image->byte_order = LSBFirst;
#elif defined (IS_BIG_ENDIAN)
    x_image->byte_order = MSBFirst;
#else
#error ERROR! Endianness is unknown;
#endif

    // Write into the pixemap that is underneath gdk's mAlphaPixmap
    // the image we just created.
    pixmap = GDK_WINDOW_XWINDOW(mAlphaPixmap);

    if (!s1bitGC) {
      GdkGCValues gcv;
      memset(&gcv, 0, sizeof(GdkGCValues));
      gcv.function = GDK_COPY;
      s1bitGC = gdk_gc_new_with_values(mAlphaPixmap, &gcv, GDK_GC_FUNCTION);
    }

    XPutImage(dpy, pixmap, GDK_GC_XGC(s1bitGC), x_image, 0, 0, 0, 0,
              aWidth, aHeight);

    // Now we are done with the temporary image
    x_image->data = 0;          /* Don't free the IL_Pixmap's bits. */
    XDestroyImage(x_image);
  }

}

void nsImageGTK::CreateOffscreenPixmap(PRInt32 aWidth, PRInt32 aHeight)
{
  // Render unique image bits onto an off screen pixmap only once
  // The image bits can change as a result of ImageUpdated() - for
  // example: animated GIFs.
  if (!mImagePixmap) {
#ifdef TRACE_IMAGE_ALLOCATION
    printf("nsImageGTK::Draw(this=%p) gdk_pixmap_new(nsnull,width=%d,height=%d,depth=%d)\n",
           this,
           aWidth, aHeight,
           mDepth);
#endif

    // Create an off screen pixmap to hold the image bits.
    mImagePixmap = gdk_pixmap_new(nsnull, aWidth, aHeight,
                                  gdk_rgb_get_visual()->depth);
  }
}


void nsImageGTK::DrawImageOffscreen(PRInt32 validX, PRInt32 validY, PRInt32 validWidth, PRInt32 validHeight)
{
  if (IsFlagSet(nsImageUpdateFlags_kBitsChanged, mFlags)) {

    if (!sXbitGC) {
      GdkGCValues gcv;
      memset(&gcv, 0, sizeof(GdkGCValues));
      gcv.function = GDK_COPY;
      sXbitGC = gdk_gc_new(mImagePixmap);
    }

    // Render the image bits into an off screen pixmap
    gdk_draw_rgb_image(mImagePixmap,
                       sXbitGC,
                       validX, validY, validWidth, validHeight,
                       GDK_RGB_DITHER_MAX,
                       mImageBits, mRowBytes);
  }

}

void nsImageGTK::SetupGCForAlpha(GdkGC *aGC, PRInt32 aX, PRInt32 aY)
{
  if (mAlphaPixmap)
  {
    // Setup gc to use the given alpha-pixmap for clipping
    XGCValues xvalues;
    unsigned long xvalues_mask = 0;
    xvalues.clip_x_origin = aX;
    xvalues.clip_y_origin = aY;
    xvalues.clip_mask = GDK_WINDOW_XWINDOW(mAlphaPixmap);
    xvalues_mask = GCClipXOrigin | GCClipYOrigin | GCClipMask;

    XChangeGC(GDK_DISPLAY(), GDK_GC_XGC(aGC), xvalues_mask, &xvalues);
  }
}

// Draw the bitmap, this draw just has destination coordinates
NS_IMETHODIMP
nsImageGTK::Draw(nsIRenderingContext &aContext,
                 nsDrawingSurface aSurface,
                 PRInt32 aX, PRInt32 aY,
                 PRInt32 aWidth, PRInt32 aHeight)
{
  g_return_val_if_fail ((aSurface != nsnull), NS_ERROR_FAILURE);

  if (mAlphaDepth==8) {
    DrawComposited(aContext, aSurface, aX, aY, aWidth, aHeight);
    return NS_OK;
  }

  // XXX kipp: this is temporary code until we eliminate the
  // width/height arguments from the draw method.
  if ((aWidth != mWidth) || (aHeight != mHeight)) {
    aWidth = mWidth;
    aHeight = mHeight;
  }


#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::Draw(this=%p,x=%d,y=%d,width=%d,height=%d)\n",
         this,
         aX, aY,
         aWidth, aHeight);
#endif

  nsDrawingSurfaceGTK* drawing = (nsDrawingSurfaceGTK*) aSurface;

  // make a copy of the GC so that we can completly restore the things we are about to change
  GdkGC *copyGC;
  copyGC = gdk_gc_new(drawing->GetDrawable());
  gdk_gc_copy(copyGC, ((nsRenderingContextGTK&)aContext).GetGC());

#ifdef CHEAP_PERFORMANCE_MEASURMENT
  gStartTime = gPixmapTime = PR_Now();
  gAlphaTime = PR_Now();
#endif

  CreateAlphaBitmap(aWidth, aHeight);

#ifdef CHEAP_PERFORMANCE_MEASURMENT
  gAlphaEnd = PR_Now();
  gPixmapTime = PR_Now();
#endif



  PRInt32
    validX = 0,
    validY = 0,
    validWidth  = aWidth,
    validHeight = aHeight;

  // limit the image rectangle to the size of the image data which
  // has been validated.
  if ((mDecodedY2 < aHeight)) {
    validHeight = mDecodedY2 - mDecodedY1;
  }
  if ((mDecodedX2 < aWidth)) {
    validWidth = mDecodedX2 - mDecodedX1;
  }
  if ((mDecodedY1 > 0)) {
    validHeight -= mDecodedY1;
    validY = mDecodedY1;
  }
  if ((mDecodedX1 > 0)) {
    validWidth -= mDecodedX1;
    validX = mDecodedX1;
  }

  CreateOffscreenPixmap(aWidth, aHeight);
  DrawImageOffscreen(validX, validY, validWidth, validHeight);

  SetupGCForAlpha(copyGC, aX, aY);

#ifdef TRACE_IMAGE_ALLOCATION
  printf("nsImageGTK::Draw(this=%p) gdk_draw_pixmap(x=%d,y=%d,width=%d,height=%d)\n",
         this,
         validX+aX,
         validY+aY,
         validWidth,                  
         validHeight);
#endif


  // copy our offscreen pixmap onto the window.
  gdk_window_copy_area(drawing->GetDrawable(),      // dest window
                       copyGC,                      // gc
                       validX+aX,                   // xsrc
                       validY+aY,                   // ysrc
                       mImagePixmap,                // source window
                       validX,                      // xdest
                       validY,                      // ydest
                       validWidth,                  // width
                       validHeight);                // height


  gdk_gc_unref(copyGC);


#ifdef CHEAP_PERFORMANCE_MEASURMENT
  gEndTime = PR_Now();
  printf("nsImageGTK::Draw(this=%p,w=%d,h=%d) total=%lld pixmap=%lld, alpha=%lld\n",
         this,
         aWidth, aHeight,
         gEndTime - gStartTime,
         gPixmapTime - gStartTime,
         gAlphaEnd - gAlphaTime);
#endif

  mFlags = 0;

  return NS_OK;
}


/** ---------------------------------------------------
 *  See documentation in nsRenderingContextImpl.h
 *	@update 3/29/00 dwc
 */
static void TileImage(GdkWindow *dest, GdkGC *gc, GdkWindow *src, nsRect &aSrcRect,
                      PRInt16 aWidth, PRInt16 aHeight)
{
  nsRect  destRect;
  
  if(aSrcRect.width < aWidth) {
    // width is less than double so double our source bitmap width
    destRect = aSrcRect;
    destRect.x += aSrcRect.width;
    gdk_window_copy_area(dest, gc, aSrcRect.x, aSrcRect.y, src, destRect.x, destRect.y, destRect.width, destRect.height);

    aSrcRect.width*=2; 
    TileImage(dest,gc,src,aSrcRect,aWidth,aHeight);
  } else if (aSrcRect.height < aHeight) {
    // height is less than double so double our source bitmap height
    destRect = aSrcRect;
    destRect.y += aSrcRect.height;
    gdk_window_copy_area(dest, gc, aSrcRect.x, aSrcRect.y, src, destRect.x, destRect.y, destRect.width, destRect.height);
    aSrcRect.height*=2;
    TileImage(dest,gc,src,aSrcRect,aWidth,aHeight);
  }
}


/** 
 * Draw a tiled version of the bitmap
 * @update - dwc 3/30/00
 * @param aSurface  the surface to blit to
 * @param aX0 starting x
 * @param aY0 starting y
 * @param aX1 ending x
 * @param aY1 ending y
 * @param aWidth The destination width of the pixelmap
 * @param aHeight The destination height of the pixelmap
 */
NS_IMETHODIMP nsImageGTK::DrawTile(nsIRenderingContext &aContext,
                                   nsDrawingSurface aSurface,
                                   nscoord aX0, nscoord aY0,
                                   nscoord aX1, nscoord aY1,
                                   nscoord aWidth, nscoord aHeight)
{
  mWidth = aX1 - aX0;
  mHeight = aY1 - aY0;

  nsDrawingSurfaceGTK *drawing = (nsDrawingSurfaceGTK*)aSurface;

  GdkGC *copyGC;
  copyGC = gdk_gc_new(drawing->GetDrawable());
  gdk_gc_copy(copyGC, ((nsRenderingContextGTK&)aContext).GetGC());

  CreateOffscreenPixmap(mWidth, mHeight);

  if (mAlphaBits) {
    // tile images...
    DrawImageOffscreen(aX0, aY0, mWidth, mHeight);

    SetupGCForAlpha(copyGC, aX0, aY0);

    nsRect srcRect(aX0, aY0, mWidth, mHeight);
    TileImage(drawing->GetDrawable(), copyGC, mImagePixmap, srcRect, aWidth, aHeight);
  }

  else {
    // XXX we should properly handle the image not being completly decoded here
    
    DrawImageOffscreen(aX0, aY0, mWidth, mHeight);

    XGCValues xvalues;
    unsigned long xvalues_mask = 0;
    xvalues.fill_style = FillTiled;
    xvalues.tile = GDK_WINDOW_XWINDOW(mImagePixmap);
    xvalues_mask = GCFillRule | GCTile;
    XChangeGC(GDK_DISPLAY(), GDK_GC_XGC(copyGC), xvalues_mask, &xvalues);

    // draw onscreen
    gdk_draw_rectangle(drawing->GetDrawable(), copyGC, PR_TRUE, aX0, aY0, aWidth, aHeight);
  }

 gdk_gc_unref(copyGC);

 return NS_OK;
}

//------------------------------------------------------------

nsresult nsImageGTK::Optimize(nsIDeviceContext* aContext)
{
  return NS_OK;
}

//------------------------------------------------------------
// lock the image pixels. nothing to do on gtk
NS_IMETHODIMP
nsImageGTK::LockImagePixels(PRBool aMaskPixels)
{
  return NS_OK;
}

//------------------------------------------------------------
// unlock the image pixels. nothing to do on gtk
NS_IMETHODIMP
nsImageGTK::UnlockImagePixels(PRBool aMaskPixels)
{
  return NS_OK;
} 

// ---------------------------------------------------
//	Set the decoded dimens of the image
//
NS_IMETHODIMP
nsImageGTK::SetDecodedRect(PRInt32 x1, PRInt32 y1, PRInt32 x2, PRInt32 y2 )
{
    
  mDecodedX1 = x1; 
  mDecodedY1 = y1; 
  mDecodedX2 = x2; 
  mDecodedY2 = y2; 
  return NS_OK;
}
