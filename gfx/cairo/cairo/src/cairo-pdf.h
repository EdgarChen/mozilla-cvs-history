/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 *
 * The Original Code is the cairo graphics library.
 *
 * The Initial Developer of the Original Code is University of Southern
 * California.
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@isi.edu>
 */

#include <cairo.h>

#ifndef CAIRO_PDF_H
#define CAIRO_PDF_H
#ifdef  CAIRO_HAS_PDF_SURFACE

#include <stdio.h>

void
cairo_set_target_pdf (cairo_t	*cr,
		      FILE	*file,
		      double	width_inches,
		      double	height_inches,
		      double	x_pixels_per_inch,
		      double	y_pixels_per_inch);


cairo_surface_t *
cairo_pdf_surface_create (FILE		*file,
			  double	width_inches,
			  double	height_inches,
			  double	x_pixels_per_inch,
			  double	y_pixels_per_inch);

#endif /* CAIRO_HAS_PDF_SURFACE */
#endif /* CAIRO_PDF_H */
