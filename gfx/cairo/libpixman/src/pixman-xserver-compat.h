/*
 * Copyright © 2005 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Red Hat,
 * Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Carl Worth, Red Hat, Inc.
 */

#ifndef _PIXMAN_XORG_COMPAT_H_
#define _PIXMAN_XORG_COMPAT_H_

/* This is a compatibility header file, designed to make it easier to
 * copy files directly from the xorg server into the implementation of
 * libpixman.
 *
 * The intended usage is that when copying files from the server, the
 * only change necessary should be to replace the block of include
 * directives with:
 *
 *	#include "pixman-xorg-compat.h"
 */

/* First, include the primary internal header file for libpixman. */

#include "icint.h"

/* Then, define any names that the server code will be expecting in
 * terms of libpixman names. */

typedef pixman_bits_t		FbBits;
typedef pixman_image_t*		PicturePtr;

typedef uint8_t			CARD8;
typedef uint16_t		CARD16;

typedef int16_t			INT16;

typedef pixman_point_fixed_t	xPointFixed;
typedef pixman_line_fixed_t	xLineFixed;
typedef pixman_trapezoid_t	xTrapezoid;
typedef pixman_triangle_t	xTriangle;

#define RENDER 1
#define FB_SHIFT IC_SHIFT 
#define FB_MASK IC_MASK
#define FB_ALLONES IC_ALLONES
#define FbMaskBits IcMaskBits

/* XXX: We changed some function and field names which makes for some
 * ugly hacks... */
#define pDrawable pixels
#define fbGetDrawable(pDrawable, pointer, stride, bpp, xoff, yoff) { \
    (buf) = (pDrawable)->data; \
    (stride) = ((int) pDrawable->stride) / sizeof (pixman_bits_t); \
    (bpp) = (pDrawable)->bpp; \
    (xoff) = 0; \
    (yoff) = 0; \
}

/* Then, include any header files that have been copied directly
 * from xserver. */

#include "renderedge.h"

/* And finally, this one prototype must come after the include of
 * renderedge.h, so it can't live alongside the other prototypes in
 * the horrible mess that is icint.h.
 */

pixman_private void
fbRasterizeEdges (pixman_bits_t		*buf,
		  int			bpp,
		  int			width,
		  int			stride,
		  RenderEdge		*l,
		  RenderEdge	 	*r,
		  pixman_fixed16_16_t	t,
		  pixman_fixed16_16_t	b);


#endif
