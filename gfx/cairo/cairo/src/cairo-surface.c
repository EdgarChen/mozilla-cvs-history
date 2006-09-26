/* cairo - a vector graphics library with display and print output
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
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
 *	Carl D. Worth <cworth@cworth.org>
 */

#include <stdlib.h>

#include "cairoint.h"
#include "cairo-surface-fallback-private.h"
#include "cairo-clip-private.h"

const cairo_surface_t _cairo_surface_nil = {
    &cairo_image_surface_backend,	/* backend */
    CAIRO_SURFACE_TYPE_IMAGE,
    CAIRO_CONTENT_COLOR,
    CAIRO_REF_COUNT_INVALID,		/* ref_count */
    CAIRO_STATUS_NO_MEMORY,		/* status */
    FALSE,				/* finished */
    { 0,	/* size */
      0,	/* num_elements */
      0,	/* element_size */
      NULL,	/* elements */
    },					/* user_data */
    { 1.0, 0.0,
      0.0, 1.0,
      0.0, 0.0
    },					/* device_transform */
    { 1.0, 0.0,
      0.0, 1.0,
      0.0, 0.0
    },					/* device_transform_inverse */
    0.0,				/* x_fallback_resolution */
    0.0,				/* y_fallback_resolution */
    NULL,				/* clip */
    0,					/* next_clip_serial */
    0,					/* current_clip_serial */
    FALSE,				/* is_snapshot */
    FALSE,				/* has_font_options */
    { CAIRO_ANTIALIAS_DEFAULT,
      CAIRO_SUBPIXEL_ORDER_DEFAULT,
      CAIRO_HINT_STYLE_DEFAULT,
      CAIRO_HINT_METRICS_DEFAULT
    }					/* font_options */
};

const cairo_surface_t _cairo_surface_nil_file_not_found = {
    &cairo_image_surface_backend,	/* backend */
    CAIRO_SURFACE_TYPE_IMAGE,
    CAIRO_CONTENT_COLOR,
    CAIRO_REF_COUNT_INVALID,		/* ref_count */
    CAIRO_STATUS_FILE_NOT_FOUND,	/* status */
    FALSE,				/* finished */
    { 0,	/* size */
      0,	/* num_elements */
      0,	/* element_size */
      NULL,	/* elements */
    },					/* user_data */
    { 1.0, 0.0,
      0.0, 1.0,
      0.0, 0.0
    },					/* device_transform */
    { 1.0, 0.0,
      0.0, 1.0,
      0.0, 0.0
    },					/* device_transform_inverse */
    0.0,				/* x_fallback_resolution */
    0.0,				/* y_fallback_resolution */
    NULL,				/* clip */
    0,					/* next_clip_serial */
    0,					/* current_clip_serial */
    FALSE,				/* is_snapshot */
    FALSE,				/* has_font_options */
    { CAIRO_ANTIALIAS_DEFAULT,
      CAIRO_SUBPIXEL_ORDER_DEFAULT,
      CAIRO_HINT_STYLE_DEFAULT,
      CAIRO_HINT_METRICS_DEFAULT
    }					/* font_options */
};

const cairo_surface_t _cairo_surface_nil_read_error = {
    &cairo_image_surface_backend,	/* backend */
    CAIRO_SURFACE_TYPE_IMAGE,
    CAIRO_CONTENT_COLOR,
    CAIRO_REF_COUNT_INVALID,		/* ref_count */
    CAIRO_STATUS_READ_ERROR,		/* status */
    FALSE,				/* finished */
    { 0,	/* size */
      0,	/* num_elements */
      0,	/* element_size */
      NULL,	/* elements */
    },					/* user_data */
    { 1.0, 0.0,
      0.0, 1.0,
      0.0, 0.0
    },					/* device_transform */
    { 1.0, 0.0,
      0.0, 1.0,
      0.0, 0.0
    },					/* device_transform_inverse */
    0.0,				/* x_fallback_resolution */
    0.0,				/* y_fallback_resolution */
    NULL,				/* clip */
    0,					/* next_clip_serial */
    0,					/* current_clip_serial */
    FALSE,				/* is_snapshot */
    FALSE,				/* has_font_options */
    { CAIRO_ANTIALIAS_DEFAULT,
      CAIRO_SUBPIXEL_ORDER_DEFAULT,
      CAIRO_HINT_STYLE_DEFAULT,
      CAIRO_HINT_METRICS_DEFAULT
    }					/* font_options */
};

static void
_cairo_surface_copy_pattern_for_destination (const cairo_pattern_t *pattern,
					     cairo_surface_t *destination,
					     cairo_pattern_t *pattern_out);

/**
 * _cairo_surface_set_error:
 * @surface: a surface
 * @status: a status value indicating an error, (eg. not
 * CAIRO_STATUS_SUCCESS)
 *
 * Sets surface->status to @status and calls _cairo_error;
 *
 * All assignments of an error status to surface->status should happen
 * through _cairo_surface_set_error() or else _cairo_error() should be
 * called immediately after the assignment.
 *
 * The purpose of this function is to allow the user to set a
 * breakpoint in _cairo_error() to generate a stack trace for when the
 * user causes cairo to detect an error.
 **/
void
_cairo_surface_set_error (cairo_surface_t *surface,
			  cairo_status_t status)
{
    /* Don't overwrite an existing error. This preserves the first
     * error, which is the most significant. It also avoids attempting
     * to write to read-only data (eg. from a nil surface). */
    if (surface->status == CAIRO_STATUS_SUCCESS)
	surface->status = status;

    _cairo_error (status);
}

/**
 * cairo_surface_get_type:
 * @surface: a #cairo_surface_t
 *
 * This function returns the type of the backend used to create
 * a surface. See #cairo_surface_type_t for available types.
 *
 * Return value: The type of @surface.
 *
 * Since: 1.2
 **/
cairo_surface_type_t
cairo_surface_get_type (cairo_surface_t *surface)
{
    /* We don't use surface->backend->type here so that some of the
     * special "wrapper" surfaces such as cairo_paginated_surface_t
     * can override surface->type with the type of the "child"
     * surface. */
    return surface->type;
}
slim_hidden_def (cairo_surface_get_type);

/**
 * cairo_surface_get_content:
 * @surface: a #cairo_surface_t
 *
 * Return value: The content type of @surface which indicates whether
 * the surface contains color and/or alpha information. See
 * #cairo_content_t.
 *
 * Since: 1.2
 **/
cairo_content_t
cairo_surface_get_content (cairo_surface_t *surface)
{
    return surface->content;
}
slim_hidden_def(cairo_surface_get_content);

/**
 * cairo_surface_status:
 * @surface: a #cairo_surface_t
 *
 * Checks whether an error has previously occurred for this
 * surface.
 *
 * Return value: %CAIRO_STATUS_SUCCESS, %CAIRO_STATUS_NULL_POINTER,
 * %CAIRO_STATUS_NO_MEMORY, %CAIRO_STATUS_READ_ERROR,
 * %CAIRO_STATUS_INVALID_CONTENT, %CAIRO_STATUS_INVALUE_FORMAT, or
 * %CAIRO_STATUS_INVALID_VISUAL.
 **/
cairo_status_t
cairo_surface_status (cairo_surface_t *surface)
{
    return surface->status;
}
slim_hidden_def (cairo_surface_status);

void
_cairo_surface_init (cairo_surface_t			*surface,
		     const cairo_surface_backend_t	*backend,
		     cairo_content_t			 content)
{
    surface->backend = backend;

    surface->content = content;

    surface->type = backend->type;

    surface->ref_count = 1;
    surface->status = CAIRO_STATUS_SUCCESS;
    surface->finished = FALSE;

    _cairo_user_data_array_init (&surface->user_data);

    cairo_matrix_init_identity (&surface->device_transform);
    cairo_matrix_init_identity (&surface->device_transform_inverse);

    surface->x_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;
    surface->y_fallback_resolution = CAIRO_SURFACE_FALLBACK_RESOLUTION_DEFAULT;

    surface->clip = NULL;
    surface->next_clip_serial = 0;
    surface->current_clip_serial = 0;

    surface->is_snapshot = FALSE;

    surface->has_font_options = FALSE;
}

cairo_surface_t *
_cairo_surface_create_similar_scratch (cairo_surface_t *other,
				       cairo_content_t	content,
				       int		width,
				       int		height)
{
    cairo_surface_t *surface = NULL;
    cairo_font_options_t options;

    cairo_format_t format = _cairo_format_from_content (content);

    if (other->status)
	return (cairo_surface_t*) &_cairo_surface_nil;

    if (other->backend->create_similar)
	surface = other->backend->create_similar (other, content, width, height);

    if (!surface)
	surface = cairo_image_surface_create (format, width, height);

    cairo_surface_get_font_options (other, &options);
    _cairo_surface_set_font_options (surface, &options);

    cairo_surface_set_fallback_resolution (surface,
					   other->x_fallback_resolution,
					   other->y_fallback_resolution);

    return surface;
}

/**
 * cairo_surface_create_similar:
 * @other: an existing surface used to select the backend of the new surface
 * @content: the content for the new surface
 * @width: width of the new surface, (in device-space units)
 * @height: height of the new surface (in device-space units)
 *
 * Create a new surface that is as compatible as possible with an
 * existing surface. For example the new surface will have the same
 * fallback resolution and font options as @other. Generally, the new
 * surface will also use the same backend as @other, unless that is
 * not possible for some reason. The type of the returned surface may
 * be examined with cairo_surface_get_type().
 *
 * Initially the surface contents are all 0 (transparent if contents
 * have transparency, black otherwise.)
 *
 * Return value: a pointer to the newly allocated surface. The caller
 * owns the surface and should call cairo_surface_destroy when done
 * with it.
 *
 * This function always returns a valid pointer, but it will return a
 * pointer to a "nil" surface if @other is already in an error state
 * or any other error occurs.
 **/
cairo_surface_t *
cairo_surface_create_similar (cairo_surface_t  *other,
			      cairo_content_t	content,
			      int		width,
			      int		height)
{
    if (other->status)
	return (cairo_surface_t*) &_cairo_surface_nil;

    if (! CAIRO_CONTENT_VALID (content)) {
	_cairo_error (CAIRO_STATUS_INVALID_CONTENT);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    return _cairo_surface_create_similar_solid (other, content,
						width, height,
						CAIRO_COLOR_TRANSPARENT);
}
slim_hidden_def (cairo_surface_create_similar);

cairo_surface_t *
_cairo_surface_create_similar_solid (cairo_surface_t	 *other,
				     cairo_content_t	  content,
				     int		  width,
				     int		  height,
				     const cairo_color_t *color)
{
    cairo_status_t status;
    cairo_surface_t *surface;
    cairo_pattern_t *source;

    surface = _cairo_surface_create_similar_scratch (other, content,
						     width, height);
    if (surface->status) {
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    source = _cairo_pattern_create_solid (color);
    if (source->status) {
	cairo_surface_destroy (surface);
	_cairo_error (CAIRO_STATUS_NO_MEMORY);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    status = _cairo_surface_paint (surface,
				   color == CAIRO_COLOR_TRANSPARENT ?
				   CAIRO_OPERATOR_CLEAR :
				   CAIRO_OPERATOR_SOURCE, source);

    cairo_pattern_destroy (source);

    if (status) {
	cairo_surface_destroy (surface);
	_cairo_error (status);
	return (cairo_surface_t*) &_cairo_surface_nil;
    }

    return surface;
}

cairo_clip_mode_t
_cairo_surface_get_clip_mode (cairo_surface_t *surface)
{
    if (surface->backend->intersect_clip_path != NULL)
	return CAIRO_CLIP_MODE_PATH;
    else if (surface->backend->set_clip_region != NULL)
	return CAIRO_CLIP_MODE_REGION;
    else
	return CAIRO_CLIP_MODE_MASK;
}

/**
 * cairo_surface_reference:
 * @surface: a #cairo_surface_t
 *
 * Increases the reference count on @surface by one. This prevents
 * @surface from being destroyed until a matching call to
 * cairo_surface_destroy() is made.
 *
 * Return value: the referenced #cairo_surface_t.
 **/
cairo_surface_t *
cairo_surface_reference (cairo_surface_t *surface)
{
    if (surface == NULL)
	return NULL;

    if (surface->ref_count == CAIRO_REF_COUNT_INVALID)
	return surface;

    assert (surface->ref_count > 0);

    surface->ref_count++;

    return surface;
}
slim_hidden_def (cairo_surface_reference);

/**
 * cairo_surface_destroy:
 * @surface: a #cairo_t
 *
 * Decreases the reference count on @surface by one. If the result is
 * zero, then @surface and all associated resources are freed.  See
 * cairo_surface_reference().
 **/
void
cairo_surface_destroy (cairo_surface_t *surface)
{
    if (surface == NULL)
	return;

    if (surface->ref_count == CAIRO_REF_COUNT_INVALID)
	return;

    assert (surface->ref_count > 0);

    surface->ref_count--;
    if (surface->ref_count)
	return;

    cairo_surface_finish (surface);

    _cairo_user_data_array_fini (&surface->user_data);

    free (surface);
}
slim_hidden_def(cairo_surface_destroy);

/**
 * cairo_surface_finish:
 * @surface: the #cairo_surface_t to finish
 *
 * This function finishes the surface and drops all references to
 * external resources.  For example, for the Xlib backend it means
 * that cairo will no longer access the drawable, which can be freed.
 * After calling cairo_surface_finish() the only valid operations on a
 * surface are getting and setting user data and referencing and
 * destroying it.  Further drawing to the surface will not affect the
 * surface but will instead trigger a CAIRO_STATUS_SURFACE_FINISHED
 * error.
 *
 * When the last call to cairo_surface_destroy() decreases the
 * reference count to zero, cairo will call cairo_surface_finish() if
 * it hasn't been called already, before freeing the resources
 * associated with the surface.
 **/
void
cairo_surface_finish (cairo_surface_t *surface)
{
    cairo_status_t status;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    if (surface->backend->finish == NULL) {
	surface->finished = TRUE;
	return;
    }

    if (!surface->status && surface->backend->flush) {
	status = surface->backend->flush (surface);
	if (status) {
	    _cairo_surface_set_error (surface, status);
	    return;
	}
    }

    status = surface->backend->finish (surface);
    if (status) {
	_cairo_surface_set_error (surface, status);
	return;
    }

    surface->finished = TRUE;
}
slim_hidden_def (cairo_surface_finish);

/**
 * cairo_surface_get_user_data:
 * @surface: a #cairo_surface_t
 * @key: the address of the #cairo_user_data_key_t the user data was
 * attached to
 *
 * Return user data previously attached to @surface using the specified
 * key.  If no user data has been attached with the given key this
 * function returns %NULL.
 *
 * Return value: the user data previously attached or %NULL.
 **/
void *
cairo_surface_get_user_data (cairo_surface_t		 *surface,
			     const cairo_user_data_key_t *key)
{
    return _cairo_user_data_array_get_data (&surface->user_data,
					    key);
}

/**
 * cairo_surface_set_user_data:
 * @surface: a #cairo_surface_t
 * @key: the address of a #cairo_user_data_key_t to attach the user data to
 * @user_data: the user data to attach to the surface
 * @destroy: a #cairo_destroy_func_t which will be called when the
 * surface is destroyed or when new user data is attached using the
 * same key.
 *
 * Attach user data to @surface.  To remove user data from a surface,
 * call this function with the key that was used to set it and %NULL
 * for @data.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or %CAIRO_STATUS_NO_MEMORY if a
 * slot could not be allocated for the user data.
 **/
cairo_status_t
cairo_surface_set_user_data (cairo_surface_t		 *surface,
			     const cairo_user_data_key_t *key,
			     void			 *user_data,
			     cairo_destroy_func_t	 destroy)
{
    if (surface->ref_count == CAIRO_REF_COUNT_INVALID)
	return CAIRO_STATUS_NO_MEMORY;

    return _cairo_user_data_array_set_data (&surface->user_data,
					    key, user_data, destroy);
}

/**
 * _cairo_surface_set_font_options:
 * @surface: a #cairo_surface_t
 * @options: a #cairo_font_options_t object that contains the
 *   options to use for this surface instead of backend's default
 *   font options.
 *
 * Sets the default font rendering options for the surface.
 * This is useful to correctly propagate default font options when
 * falling back to an image surface in a backend implementation.
 * This affects the options returned in cairo_surface_get_font_options().
 *
 * If @options is %NULL the surface options are reset to those of
 * the backend default.
 **/
void
_cairo_surface_set_font_options (cairo_surface_t       *surface,
				 cairo_font_options_t  *options)
{
    if (options) {
	surface->has_font_options = TRUE;
	_cairo_font_options_init_copy (&surface->font_options, options);
    } else {
	surface->has_font_options = FALSE;
    }
}

/**
 * cairo_surface_get_font_options:
 * @surface: a #cairo_surface_t
 * @options: a #cairo_font_options_t object into which to store
 *   the retrieved options. All existing values are overwritten
 *
 * Retrieves the default font rendering options for the surface.
 * This allows display surfaces to report the correct subpixel order
 * for rendering on them, print surfaces to disable hinting of
 * metrics and so forth. The result can then be used with
 * cairo_scaled_font_create().
 **/
void
cairo_surface_get_font_options (cairo_surface_t       *surface,
				cairo_font_options_t  *options)
{
    if (!surface->has_font_options) {
	surface->has_font_options = TRUE;

	if (!surface->finished && surface->backend->get_font_options) {
	    surface->backend->get_font_options (surface, &surface->font_options);
	} else {
	    _cairo_font_options_init_default (&surface->font_options);
	}
    }

    _cairo_font_options_init_copy (options, &surface->font_options);
}
slim_hidden_def (cairo_surface_get_font_options);

/**
 * cairo_surface_flush:
 * @surface: a #cairo_surface_t
 *
 * Do any pending drawing for the surface and also restore any
 * temporary modification's cairo has made to the surface's
 * state. This function must be called before switching from
 * drawing on the surface with cairo to drawing on it directly
 * with native APIs. If the surface doesn't support direct access,
 * then this function does nothing.
 **/
void
cairo_surface_flush (cairo_surface_t *surface)
{
    if (surface->status)
	return;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    if (surface->backend->flush) {
	cairo_status_t status;

	status = surface->backend->flush (surface);

	if (status)
	    _cairo_surface_set_error (surface, status);
    }
}

/**
 * cairo_surface_mark_dirty:
 * @surface: a #cairo_surface_t
 *
 * Tells cairo that drawing has been done to surface using means other
 * than cairo, and that cairo should reread any cached areas. Note
 * that you must call cairo_surface_flush() before doing such drawing.
 */
void
cairo_surface_mark_dirty (cairo_surface_t *surface)
{
    assert (! surface->is_snapshot);

    cairo_surface_mark_dirty_rectangle (surface, 0, 0, -1, -1);
}

/**
 * cairo_surface_mark_dirty_rectangle:
 * @surface: a #cairo_surface_t
 * @x: X coordinate of dirty rectangle
 * @y: Y coordinate of dirty rectangle
 * @width: width of dirty rectangle
 * @height: height of dirty rectangle
 *
 * Like cairo_surface_mark_dirty(), but drawing has been done only to
 * the specified rectangle, so that cairo can retain cached contents
 * for other parts of the surface.
 *
 * Any cached clip set on the surface will be reset by this function,
 * to make sure that future cairo calls have the clip set that they
 * expect.
 */
void
cairo_surface_mark_dirty_rectangle (cairo_surface_t *surface,
				    int              x,
				    int              y,
				    int              width,
				    int              height)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    /* Always reset the clip here, to avoid having external calls to
     * clip manipulation functions of the underlying device clip result
     * in a desync between the cairo clip and the backend clip, due to
     * the clip caching.
     */
    surface->current_clip_serial = -1;

    if (surface->backend->mark_dirty_rectangle) {
	cairo_status_t status;

	/* XXX: FRAGILE: We're ignoring the scaling component of
	 * device_transform here. I don't know what the right thing to
	 * do would actually be if there were some scaling here, but
	 * we avoid this since device_transfom scaling is not exported
	 * publicly and mark_dirty is not used internally. */
	status = surface->backend->mark_dirty_rectangle (surface,
                                                         x + surface->device_transform.x0,
                                                         y + surface->device_transform.y0,
							 width, height);

	if (status)
	    _cairo_surface_set_error (surface, status);
    }
}
slim_hidden_def (cairo_surface_mark_dirty_rectangle);

/**
 * _cairo_surface_set_device_scale:
 * @surface: a #cairo_surface_t
 * @sx: a scale factor in the X direction
 * @sy: a scale factor in the Y direction
 *
 * Private function for setting an extra scale factor to affect all
 * drawing to a surface. This is used, for example, when replaying a
 * meta surface to an image fallback intended for an eventual
 * vector-oriented backend. Since the meta surface will record
 * coordinates in one backend space, but the image fallback uses a
 * different backend space, (differing by the fallback resolution
 * scale factors), we need a scale factor correction.
 *
 * Caution: There is no guarantee that a surface with both a
 * device_scale and a device_offset will be treated in consistent
 * fashion. So, for now, just don't do that. (And we'll need to
 * examine this issue in more detail if we were to ever want to export
 * support for device scaling.)
 **/
void
_cairo_surface_set_device_scale (cairo_surface_t *surface,
				 double		  sx,
				 double		  sy)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    surface->device_transform.xx = sx;
    surface->device_transform.yy = sy;

    surface->device_transform_inverse.xx = 1.0 / sx;
    surface->device_transform_inverse.yy = 1.0 / sy;
}

/**
 * cairo_surface_set_device_offset:
 * @surface: a #cairo_surface_t
 * @x_offset: the offset in the X direction, in device units
 * @y_offset: the offset in the Y direction, in device units
 *
 * Sets an offset that is added to the device coordinates determined
 * by the CTM when drawing to @surface. One use case for this function
 * is when we want to create a #cairo_surface_t that redirects drawing
 * for a portion of an onscreen surface to an offscreen surface in a
 * way that is completely invisible to the user of the cairo
 * API. Setting a transformation via cairo_translate() isn't
 * sufficient to do this, since functions like
 * cairo_device_to_user() will expose the hidden offset.
 *
 * Note that the offset affects drawing to the surface as well as
 * using the surface in a source pattern.
 **/
void
cairo_surface_set_device_offset (cairo_surface_t *surface,
				 double           x_offset,
				 double           y_offset)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return;

    if (surface->finished) {
	_cairo_surface_set_error (surface, CAIRO_STATUS_SURFACE_FINISHED);
	return;
    }

    surface->device_transform.x0 = x_offset;
    surface->device_transform.y0 = y_offset;

    surface->device_transform_inverse.x0 = - x_offset;
    surface->device_transform_inverse.y0 = - y_offset;
}
slim_hidden_def (cairo_surface_set_device_offset);

/**
 * cairo_surface_get_device_offset:
 * @surface: a #cairo_surface_t
 * @x_offset: the offset in the X direction, in device units
 * @y_offset: the offset in the Y direction, in device units
 *
 * This function returns the previous device offset set by
 * cairo_surface_set_device_offset().
 *
 * Since: 1.2
 **/
void
cairo_surface_get_device_offset (cairo_surface_t *surface,
				 double          *x_offset,
				 double          *y_offset)
{
    *x_offset = surface->device_transform.x0;
    *y_offset = surface->device_transform.y0;
}
slim_hidden_def (cairo_surface_get_device_offset);

/**
 * cairo_surface_set_fallback_resolution:
 * @surface: a #cairo_surface_t
 * @x_pixels_per_inch: horizontal setting for pixels per inch
 * @y_pixels_per_inch: vertical setting for pixels per inch
 *
 * Set the horizontal and vertical resolution for image fallbacks.
 *
 * When certain operations aren't supported natively by a backend,
 * cairo will fallback by rendering operations to an image and then
 * overlaying that image onto the output. For backends that are
 * natively vector-oriented, this function can be used to set the
 * resolution used for these image fallbacks, (larger values will
 * result in more detailed images, but also larger file sizes).
 *
 * Some examples of natively vector-oriented backends are the ps, pdf,
 * and svg backends.
 *
 * For backends that are natively raster-oriented, image fallbacks are
 * still possible, but they are always performed at the native
 * device resolution. So this function has no effect on those
 * backends.
 *
 * NOTE: The fallback resolution only takes effect at the time of
 * completing a page (with cairo_show_page() or cairo_copy_page()) so
 * there is currently no way to have more than one fallback resolution
 * in effect on a single page.
 *
 * Since: 1.2
 **/
void
cairo_surface_set_fallback_resolution (cairo_surface_t	*surface,
				       double		 x_pixels_per_inch,
				       double		 y_pixels_per_inch)
{
    surface->x_fallback_resolution = x_pixels_per_inch;
    surface->y_fallback_resolution = y_pixels_per_inch;
}
slim_hidden_def (cairo_surface_set_fallback_resolution);

cairo_bool_t
_cairo_surface_has_device_transform (cairo_surface_t *surface)
{
    return ! _cairo_matrix_is_identity (&surface->device_transform);
}

/**
 * _cairo_surface_acquire_source_image:
 * @surface: a #cairo_surface_t
 * @image_out: location to store a pointer to an image surface that
 *    has identical contents to @surface. This surface could be @surface
 *    itself, a surface held internal to @surface, or it could be a new
 *    surface with a copy of the relevant portion of @surface.
 * @image_extra: location to store image specific backend data
 *
 * Gets an image surface to use when drawing as a fallback when drawing with
 * @surface as a source. _cairo_surface_release_source_image() must be called
 * when finished.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if a an image was stored in @image_out.
 * %CAIRO_INT_STATUS_UNSUPPORTED if an image cannot be retrieved for the specified
 * surface. Or %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_status_t
_cairo_surface_acquire_source_image (cairo_surface_t         *surface,
				     cairo_image_surface_t  **image_out,
				     void                   **image_extra)
{
    assert (!surface->finished);

    return surface->backend->acquire_source_image (surface,
						   image_out, image_extra);
}

/**
 * _cairo_surface_release_source_image:
 * @surface: a #cairo_surface_t
 * @image_extra: same as return from the matching _cairo_surface_acquire_source_image()
 *
 * Releases any resources obtained with _cairo_surface_acquire_source_image()
 **/
void
_cairo_surface_release_source_image (cairo_surface_t        *surface,
				     cairo_image_surface_t  *image,
				     void                   *image_extra)
{
    assert (!surface->finished);

    if (surface->backend->release_source_image)
	surface->backend->release_source_image (surface, image, image_extra);
}

/**
 * _cairo_surface_acquire_dest_image:
 * @surface: a #cairo_surface_t
 * @interest_rect: area of @surface for which fallback drawing is being done.
 *    A value of %NULL indicates that the entire surface is desired.
 *    XXXX I'd like to get rid of being able to pass NULL here (nothing seems to)
 * @image_out: location to store a pointer to an image surface that includes at least
 *    the intersection of @interest_rect with the visible area of @surface.
 *    This surface could be @surface itself, a surface held internal to @surface,
 *    or it could be a new surface with a copy of the relevant portion of @surface.
 *    If a new surface is created, it should have the same channels and depth
 *    as @surface so that copying to and from it is exact.
 * @image_rect: location to store area of the original surface occupied
 *    by the surface stored in @image.
 * @image_extra: location to store image specific backend data
 *
 * Retrieves a local image for a surface for implementing a fallback drawing
 * operation. After calling this function, the implementation of the fallback
 * drawing operation draws the primitive to the surface stored in @image_out
 * then calls _cairo_surface_release_dest_image(),
 * which, if a temporary surface was created, copies the bits back to the
 * main surface and frees the temporary surface.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or %CAIRO_STATUS_NO_MEMORY.
 *  %CAIRO_INT_STATUS_UNSUPPORTED can be returned but this will mean that
 *  the backend can't draw with fallbacks. It's possible for the routine
 *  to store NULL in @local_out and return %CAIRO_STATUS_SUCCESS;
 *  that indicates that no part of @interest_rect is visible, so no drawing
 *  is necessary. _cairo_surface_release_dest_image() should not be called in that
 *  case.
 **/
cairo_status_t
_cairo_surface_acquire_dest_image (cairo_surface_t         *surface,
				   cairo_rectangle_int16_t *interest_rect,
				   cairo_image_surface_t  **image_out,
				   cairo_rectangle_int16_t *image_rect,
				   void                   **image_extra)
{
    assert (!surface->finished);

    return surface->backend->acquire_dest_image (surface,
						 interest_rect,
						 image_out, image_rect, image_extra);
}

/**
 * _cairo_surface_release_dest_image:
 * @surface: a #cairo_surface_t
 * @interest_rect: same as passed to the matching _cairo_surface_acquire_dest_image()
 * @image: same as returned from the matching _cairo_surface_acquire_dest_image()
 * @image_rect: same as returned from the matching _cairo_surface_acquire_dest_image()
 * @image_extra: same as return from the matching _cairo_surface_acquire_dest_image()
 *
 * Finishes the operation started with _cairo_surface_acquire_dest_image(), by, if
 * necessary, copying the image from @image back to @surface and freeing any
 * resources that were allocated.
 **/
void
_cairo_surface_release_dest_image (cairo_surface_t         *surface,
				   cairo_rectangle_int16_t *interest_rect,
				   cairo_image_surface_t   *image,
				   cairo_rectangle_int16_t *image_rect,
				   void                    *image_extra)
{
    assert (!surface->finished);

    if (surface->backend->release_dest_image)
	surface->backend->release_dest_image (surface, interest_rect,
					      image, image_rect, image_extra);
}

/**
 * _cairo_surface_clone_similar:
 * @surface: a #cairo_surface_t
 * @src: the source image
 * @clone_out: location to store a surface compatible with @surface
 *   and with contents identical to @src. The caller must call
 *   cairo_surface_destroy() on the result.
 *
 * Creates a surface with contents identical to @src but that
 *   can be used efficiently with @surface. If @surface and @src are
 *   already compatible then it may return a new reference to @src.
 *
 * Return value: %CAIRO_STATUS_SUCCESS if a surface was created and stored
 *   in @clone_out. Otherwise %CAIRO_INT_STATUS_UNSUPPORTED or another
 *   error like %CAIRO_STATUS_NO_MEMORY.
 **/
cairo_status_t
_cairo_surface_clone_similar (cairo_surface_t  *surface,
			      cairo_surface_t  *src,
			      cairo_surface_t **clone_out)
{
    cairo_status_t status;
    cairo_image_surface_t *image;
    void *image_extra;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (surface->backend->clone_similar == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    status = surface->backend->clone_similar (surface, src, clone_out);
    if (status == CAIRO_STATUS_SUCCESS)
        (*clone_out)->device_transform = src->device_transform;

    if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	return status;

    status = _cairo_surface_acquire_source_image (src, &image, &image_extra);
    if (status != CAIRO_STATUS_SUCCESS)
	return status;

    status = surface->backend->clone_similar (surface, &image->base, clone_out);
    if (status == CAIRO_STATUS_SUCCESS)
        (*clone_out)->device_transform = src->device_transform;

    /* If the above failed point, we could implement a full fallback
     * using acquire_dest_image, but that's going to be very
     * inefficient compared to a backend-specific implementation of
     * clone_similar() with an image source. So we don't bother
     */

    _cairo_surface_release_source_image (src, image, image_extra);
    return status;
}

/* XXX: Shouldn't really need to do this here. */
#include "cairo-meta-surface-private.h"

/**
 * _cairo_surface_snapshot
 * @surface: a #cairo_surface_t
 *
 * Make an immutable copy of @surface. It is an error to call a
 * surface-modifying function on the result of this function.
 *
 * The caller owns the return value and should call
 * cairo_surface_destroy when finished with it. This function will not
 * return NULL, but will return a nil surface instead.
 *
 * Return value: The snapshot surface. Note that the return surface
 * may not necessarily be of the same type as @surface.
 **/
cairo_surface_t *
_cairo_surface_snapshot (cairo_surface_t *surface)
{
    if (surface->finished)
	return (cairo_surface_t *) &_cairo_surface_nil;

    if (surface->backend->snapshot)
	return surface->backend->snapshot (surface);

    return _cairo_surface_fallback_snapshot (surface);
}

cairo_status_t
_cairo_surface_composite (cairo_operator_t	op,
			  cairo_pattern_t	*src,
			  cairo_pattern_t	*mask,
			  cairo_surface_t	*dst,
			  int			src_x,
			  int			src_y,
			  int			mask_x,
			  int			mask_y,
			  int			dst_x,
			  int			dst_y,
			  unsigned int		width,
			  unsigned int		height)
{
    cairo_int_status_t status;

    assert (! dst->is_snapshot);

    if (mask) {
	/* These operators aren't interpreted the same way by the backends;
	 * they are implemented in terms of other operators in cairo-gstate.c
	 */
	assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);
    }

    if (dst->status)
	return dst->status;

    if (dst->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (dst->backend->composite) {
	status = dst->backend->composite (op,
					  src, mask, dst,
                                          src_x, src_y,
                                          mask_x, mask_y,
                                          dst_x, dst_y,
					  width, height);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    return _cairo_surface_fallback_composite (op,
					      src, mask, dst,
					      src_x, src_y,
					      mask_x, mask_y,
					      dst_x, dst_y,
					      width, height);
}

/**
 * _cairo_surface_fill_rectangle:
 * @surface: a #cairo_surface_t
 * @op: the operator to apply to the rectangle
 * @color: the source color
 * @x: X coordinate of rectangle, in backend coordinates
 * @y: Y coordinate of rectangle, in backend coordinates
 * @width: width of rectangle, in backend coordinates
 * @height: height of rectangle, in backend coordinates
 *
 * Applies an operator to a rectangle using a solid color as the source.
 * See _cairo_surface_fill_rectangles() for full details.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or the error that occurred
 **/
cairo_status_t
_cairo_surface_fill_rectangle (cairo_surface_t	   *surface,
			       cairo_operator_t	    op,
			       const cairo_color_t *color,
			       int		    x,
			       int		    y,
			       int		    width,
			       int		    height)
{
    cairo_rectangle_int16_t rect;

    assert (! surface->is_snapshot);

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    rect.x = x;
    rect.y = y;
    rect.width = width;
    rect.height = height;

    return _cairo_surface_fill_rectangles (surface, op, color, &rect, 1);
}

/**
 * _cairo_surface_fill_region:
 * @surface: a #cairo_surface_t
 * @op: the operator to apply to the region
 * @color: the source color
 * @region: the region to modify, in backend coordinates
 *
 * Applies an operator to a set of rectangles specified as a
 * #pixman_region16_t using a solid color as the source.
 * See _cairo_surface_fill_rectangles() for full details.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or the error that occurred
 **/
cairo_status_t
_cairo_surface_fill_region (cairo_surface_t	   *surface,
			    cairo_operator_t	    op,
			    const cairo_color_t    *color,
			    pixman_region16_t      *region)
{
    int num_rects = pixman_region_num_rects (region);
    pixman_box16_t *boxes = pixman_region_rects (region);
    cairo_rectangle_int16_t *rects;
    cairo_status_t status;
    int i;

    assert (! surface->is_snapshot);

    if (!num_rects)
	return CAIRO_STATUS_SUCCESS;

    rects = malloc (sizeof (pixman_rectangle_t) * num_rects);
    if (!rects)
	return CAIRO_STATUS_NO_MEMORY;

    for (i = 0; i < num_rects; i++) {
	rects[i].x = boxes[i].x1;
	rects[i].y = boxes[i].y1;
	rects[i].width = boxes[i].x2 - boxes[i].x1;
	rects[i].height = boxes[i].y2 - boxes[i].y1;
    }

    status =  _cairo_surface_fill_rectangles (surface, op,
					      color, rects, num_rects);

    free (rects);

    return status;
}

/**
 * _cairo_surface_fill_rectangles:
 * @surface: a #cairo_surface_t
 * @op: the operator to apply to the region
 * @color: the source color
 * @rects: the rectangles to modify, in backend coordinates
 * @num_rects: the number of rectangles in @rects
 *
 * Applies an operator to a set of rectangles using a solid color
 * as the source. Note that even if the operator is an unbounded operator
 * such as %CAIRO_OPERATOR_IN, only the given set of rectangles
 * is affected. This differs from _cairo_surface_composite_trapezoids()
 * where the entire destination rectangle is cleared.
 *
 * Return value: %CAIRO_STATUS_SUCCESS or the error that occurred
 **/
cairo_status_t
_cairo_surface_fill_rectangles (cairo_surface_t		*surface,
				cairo_operator_t         op,
				const cairo_color_t	*color,
				cairo_rectangle_int16_t	*rects,
				int			 num_rects)
{
    cairo_int_status_t status;

    assert (! surface->is_snapshot);

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (num_rects == 0)
	return CAIRO_STATUS_SUCCESS;

    if (surface->backend->fill_rectangles) {
	status = surface->backend->fill_rectangles (surface, op, color,
						    rects, num_rects);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    return _cairo_surface_fallback_fill_rectangles (surface, op, color,
						    rects, num_rects);
}

cairo_status_t
_cairo_surface_paint (cairo_surface_t	*surface,
		      cairo_operator_t	 op,
		      cairo_pattern_t	*source)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;

    assert (! surface->is_snapshot);

    if (source->type == CAIRO_PATTERN_TYPE_SURFACE &&
	(source->extend == CAIRO_EXTEND_REFLECT || source->extend == CAIRO_EXTEND_PAD))
    {
        return CAIRO_STATUS_NO_MEMORY;
    }

    _cairo_surface_copy_pattern_for_destination (source, surface, &dev_source.base);

    if (surface->backend->paint) {
	status = surface->backend->paint (surface, op, &dev_source.base);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_paint (surface, op, &dev_source.base);

 FINISH:
    _cairo_pattern_fini (&dev_source.base);

    return status;
}

cairo_status_t
_cairo_surface_mask (cairo_surface_t	*surface,
		     cairo_operator_t	 op,
		     cairo_pattern_t	*source,
		     cairo_pattern_t	*mask)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;
    cairo_pattern_union_t dev_mask;

    assert (! surface->is_snapshot);

    if (source->type == CAIRO_PATTERN_TYPE_SURFACE &&
	(source->extend == CAIRO_EXTEND_REFLECT || source->extend == CAIRO_EXTEND_PAD))
    {
        return CAIRO_STATUS_NO_MEMORY;
    }

    _cairo_surface_copy_pattern_for_destination (source, surface, &dev_source.base);
    _cairo_surface_copy_pattern_for_destination (mask, surface, &dev_mask.base);

    if (surface->backend->mask) {
	status = surface->backend->mask (surface, op, &dev_source.base, &dev_mask.base);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_mask (surface, op, &dev_source.base, &dev_mask.base);

 FINISH:
    _cairo_pattern_fini (&dev_mask.base);
    _cairo_pattern_fini (&dev_source.base);

    return status;
}

cairo_status_t
_cairo_surface_stroke (cairo_surface_t		*surface,
		       cairo_operator_t		 op,
		       cairo_pattern_t		*source,
		       cairo_path_fixed_t	*path,
		       cairo_stroke_style_t	*stroke_style,
		       cairo_matrix_t		*ctm,
		       cairo_matrix_t		*ctm_inverse,
		       double			 tolerance,
		       cairo_antialias_t	 antialias)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;
    cairo_path_fixed_t *dev_path = path;
    cairo_path_fixed_t real_dev_path;
    cairo_matrix_t dev_ctm = *ctm;
    cairo_matrix_t dev_ctm_inverse = *ctm_inverse;

    assert (! surface->is_snapshot);

    if (source->type == CAIRO_PATTERN_TYPE_SURFACE &&
	(source->extend == CAIRO_EXTEND_REFLECT || source->extend == CAIRO_EXTEND_PAD))
    {
        return CAIRO_STATUS_NO_MEMORY;
    }

    _cairo_surface_copy_pattern_for_destination (source, surface, &dev_source.base);

    if (surface->backend->stroke) {
	status = surface->backend->stroke (surface, op, &dev_source.base,
					   path, stroke_style,
					   &dev_ctm, &dev_ctm_inverse,
					   tolerance, antialias);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_stroke (surface, op, &dev_source.base,
                                             path, stroke_style,
                                             &dev_ctm, &dev_ctm_inverse,
                                             tolerance, antialias);

 FINISH:
    if (dev_path == &real_dev_path)
        _cairo_path_fixed_fini (&real_dev_path);
    _cairo_pattern_fini (&dev_source.base);

    return status;
}

cairo_status_t
_cairo_surface_fill (cairo_surface_t	*surface,
		     cairo_operator_t	 op,
		     cairo_pattern_t	*source,
		     cairo_path_fixed_t	*path,
		     cairo_fill_rule_t	 fill_rule,
		     double		 tolerance,
		     cairo_antialias_t	 antialias)
{
    cairo_status_t status;
    cairo_pattern_union_t dev_source;

    assert (! surface->is_snapshot);

    if (source->type == CAIRO_PATTERN_TYPE_SURFACE &&
	(source->extend == CAIRO_EXTEND_REFLECT || source->extend == CAIRO_EXTEND_PAD))
    {
        return CAIRO_STATUS_NO_MEMORY;
    }

    _cairo_surface_copy_pattern_for_destination (source, surface, &dev_source.base);

    if (surface->backend->fill) {
	status = surface->backend->fill (surface, op, &dev_source.base,
					 path, fill_rule,
					 tolerance, antialias);

	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_fill (surface, op, &dev_source.base,
                                           path, fill_rule,
                                           tolerance, antialias);

 FINISH:
    _cairo_pattern_fini (&dev_source.base);

    return status;
}

cairo_status_t
_cairo_surface_composite_trapezoids (cairo_operator_t		op,
				     cairo_pattern_t		*pattern,
				     cairo_surface_t		*dst,
				     cairo_antialias_t		antialias,
				     int			src_x,
				     int			src_y,
				     int			dst_x,
				     int			dst_y,
				     unsigned int		width,
				     unsigned int		height,
				     cairo_trapezoid_t		*traps,
				     int			num_traps)
{
    cairo_int_status_t status;

    assert (! dst->is_snapshot);

    /* These operators aren't interpreted the same way by the backends;
     * they are implemented in terms of other operators in cairo-gstate.c
     */
    assert (op != CAIRO_OPERATOR_SOURCE && op != CAIRO_OPERATOR_CLEAR);

    if (dst->status)
	return dst->status;

    if (dst->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (dst->backend->composite_trapezoids) {
	status = dst->backend->composite_trapezoids (op,
						     pattern, dst,
						     antialias,
						     src_x, src_y,
                                                     dst_x, dst_y,
						     width, height,
						     traps, num_traps);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
	    return status;
    }

    return  _cairo_surface_fallback_composite_trapezoids (op, pattern, dst,
							  antialias,
							  src_x, src_y,
							  dst_x, dst_y,
							  width, height,
							  traps, num_traps);
}

/* _copy_page and _show_page are unique among _cairo_surface functions
 * in that they will actually return CAIRO_INT_STATUS_UNSUPPORTED
 * rather than performing any fallbacks. */
cairo_int_status_t
_cairo_surface_copy_page (cairo_surface_t *surface)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (surface->backend->copy_page == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    return surface->backend->copy_page (surface);
}

/* _show_page and _copy_page are unique among _cairo_surface functions
 * in that they will actually return CAIRO_INT_STATUS_UNSUPPORTED
 * rather than performing any fallbacks. */
cairo_int_status_t
_cairo_surface_show_page (cairo_surface_t *surface)
{
    assert (! surface->is_snapshot);

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (surface->backend->show_page == NULL)
	return CAIRO_INT_STATUS_UNSUPPORTED;

    return surface->backend->show_page (surface);
}

/**
 * _cairo_surface_get_current_clip_serial:
 * @surface: the #cairo_surface_t to return the serial number for
 *
 * Returns the serial number associated with the current
 * clip in the surface.  All gstate functions must
 * verify that the correct clip is set in the surface before
 * invoking any surface drawing function
 */
unsigned int
_cairo_surface_get_current_clip_serial (cairo_surface_t *surface)
{
    return surface->current_clip_serial;
}

/**
 * _cairo_surface_allocate_clip_serial:
 * @surface: the #cairo_surface_t to allocate a serial number from
 *
 * Each surface has a separate set of clipping serial numbers, and
 * this function allocates one from the specified surface.  As zero is
 * reserved for the special no-clipping case, this function will not
 * return that except for an in-error surface, (ie. surface->status !=
 * CAIRO_STATUS_SUCCESS).
 */
unsigned int
_cairo_surface_allocate_clip_serial (cairo_surface_t *surface)
{
    unsigned int    serial;

    if (surface->status)
	return 0;

    if ((serial = ++(surface->next_clip_serial)) == 0)
	serial = ++(surface->next_clip_serial);
    return serial;
}

/**
 * _cairo_surface_reset_clip:
 * @surface: the #cairo_surface_t to reset the clip on
 *
 * This function sets the clipping for the surface to
 * None, which is to say that drawing is entirely
 * unclipped.  It also sets the clip serial number
 * to zero.
 */
cairo_status_t
_cairo_surface_reset_clip (cairo_surface_t *surface)
{
    cairo_status_t  status;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    surface->current_clip_serial = 0;

    if (surface->backend->intersect_clip_path) {
	status = surface->backend->intersect_clip_path (surface,
							NULL,
							CAIRO_FILL_RULE_WINDING,
							0,
							CAIRO_ANTIALIAS_DEFAULT);
	if (status)
	    return status;
    }

    if (surface->backend->set_clip_region != NULL) {
	status = surface->backend->set_clip_region (surface, NULL);
	if (status)
	    return status;
    }

    return CAIRO_STATUS_SUCCESS;
}

/**
 * _cairo_surface_set_clip_region:
 * @surface: the #cairo_surface_t to reset the clip on
 * @region: the #pixman_region16_t to use for clipping
 * @serial: the clip serial number associated with the region
 *
 * This function sets the clipping for the surface to
 * the specified region and sets the surface clipping
 * serial number to the associated serial number.
 */
cairo_status_t
_cairo_surface_set_clip_region (cairo_surface_t	    *surface,
				pixman_region16_t   *region,
				unsigned int	    serial)
{
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    assert (surface->backend->set_clip_region != NULL);

    surface->current_clip_serial = serial;

    status = surface->backend->set_clip_region (surface, region);

    return status;
}

cairo_int_status_t
_cairo_surface_intersect_clip_path (cairo_surface_t    *surface,
				    cairo_path_fixed_t *path,
				    cairo_fill_rule_t   fill_rule,
				    double		tolerance,
				    cairo_antialias_t	antialias)
{
    cairo_path_fixed_t *dev_path = path;
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    assert (surface->backend->intersect_clip_path != NULL);

    status = surface->backend->intersect_clip_path (surface,
						    dev_path,
						    fill_rule,
						    tolerance,
						    antialias);

    return status;
}

static cairo_status_t
_cairo_surface_set_clip_path_recursive (cairo_surface_t *surface,
					cairo_clip_path_t *clip_path)
{
    cairo_status_t status;

    if (clip_path == NULL)
	return CAIRO_STATUS_SUCCESS;

    status = _cairo_surface_set_clip_path_recursive (surface, clip_path->prev);
    if (status)
	return status;

    return _cairo_surface_intersect_clip_path (surface,
					       &clip_path->path,
					       clip_path->fill_rule,
					       clip_path->tolerance,
					       clip_path->antialias);
}

/**
 * _cairo_surface_set_clip_path:
 * @surface: the #cairo_surface_t to set the clip on
 * @clip_path: the clip path to set
 * @serial: the clip serial number associated with the clip path
 *
 * Sets the given clipping path for the surface and assigns the
 * clipping serial to the surface.
 **/
static cairo_status_t
_cairo_surface_set_clip_path (cairo_surface_t	*surface,
			      cairo_clip_path_t	*clip_path,
			      unsigned int	serial)
{
    cairo_status_t status;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    assert (surface->backend->intersect_clip_path != NULL);

    status = surface->backend->intersect_clip_path (surface,
						    NULL,
						    CAIRO_FILL_RULE_WINDING,
						    0,
						    CAIRO_ANTIALIAS_DEFAULT);
    if (status)
	return status;

    status = _cairo_surface_set_clip_path_recursive (surface, clip_path);
    if (status)
	return status;

    surface->current_clip_serial = serial;

    return CAIRO_STATUS_SUCCESS;
}

cairo_status_t
_cairo_surface_set_clip (cairo_surface_t *surface, cairo_clip_t *clip)
{
    unsigned int serial = 0;

    if (!surface)
	return CAIRO_STATUS_NULL_POINTER;

    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (clip) {
	serial = clip->serial;
	if (serial == 0)
	    clip = NULL;
    }

    surface->clip = clip;

    if (serial == _cairo_surface_get_current_clip_serial (surface))
	return CAIRO_STATUS_SUCCESS;

    if (clip) {
	if (clip->path)
	    return _cairo_surface_set_clip_path (surface,
						 clip->path,
						 clip->serial);

	if (clip->region)
	    return _cairo_surface_set_clip_region (surface,
						   clip->region,
						   clip->serial);
    }

    return _cairo_surface_reset_clip (surface);
}

/**
 * _cairo_surface_get_extents:
 * @surface: the #cairo_surface_t to fetch extents for
 *
 * This function returns a bounding box for the surface.  The surface
 * bounds are defined as a region beyond which no rendering will
 * possibly be recorded, in otherwords, it is the maximum extent of
 * potentially usable coordinates.
 *
 * For vector surfaces, (PDF, PS, SVG and meta-surfaces), the surface
 * might be conceived as unbounded, but we force the user to provide a
 * maximum size at the time of surface_create. So get_extents uses
 * that size.
 *
 * NOTE: The coordinates returned are in "backend" space rather than
 * "surface" space. That is, they are relative to the true (0,0)
 * origin rather than the device_transform origin. This might seem a
 * bit inconsistent with other cairo_surface interfaces, but all
 * current callers are within the surface layer where backend space is
 * desired.
 *
 * This behavior would have to be changed is we ever exported a public
 * variant of this function.
 */
cairo_status_t
_cairo_surface_get_extents (cairo_surface_t         *surface,
			    cairo_rectangle_int16_t *rectangle)
{
    if (surface->status)
	return surface->status;

    if (surface->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    return surface->backend->get_extents (surface, rectangle);
}

cairo_status_t
_cairo_surface_show_glyphs (cairo_surface_t	*surface,
			    cairo_operator_t	 op,
			    cairo_pattern_t	*source,
			    const cairo_glyph_t	*glyphs,
			    int			 num_glyphs,
			    cairo_scaled_font_t	*scaled_font)
{
    cairo_status_t status;
    cairo_scaled_font_t *dev_scaled_font = scaled_font;
    cairo_pattern_union_t dev_source;
    cairo_matrix_t font_matrix;

    assert (! surface->is_snapshot);

    if (source->type == CAIRO_PATTERN_TYPE_SURFACE &&
	(source->extend == CAIRO_EXTEND_REFLECT || source->extend == CAIRO_EXTEND_PAD))
    {
        return CAIRO_STATUS_NO_MEMORY;
    }

    _cairo_surface_copy_pattern_for_destination (source,
						 surface,
						 &dev_source.base);

    cairo_scaled_font_get_font_matrix (scaled_font, &font_matrix);

    if (_cairo_surface_has_device_transform (surface) &&
	! _cairo_matrix_is_integer_translation (&surface->device_transform, NULL, NULL))
    {
	cairo_font_options_t *font_options;
	cairo_matrix_t dev_ctm;

	font_options = cairo_font_options_create ();

	cairo_scaled_font_get_ctm (scaled_font, &dev_ctm);
	cairo_matrix_multiply (&dev_ctm, &dev_ctm, &surface->device_transform);
	cairo_scaled_font_get_font_options (scaled_font, font_options);
	dev_scaled_font = cairo_scaled_font_create (cairo_scaled_font_get_font_face (scaled_font),
						    &font_matrix,
						    &dev_ctm,
						    font_options);
	cairo_font_options_destroy (font_options);
    }

    if (surface->backend->show_glyphs) {
	status = surface->backend->show_glyphs (surface, op, &dev_source.base,
						glyphs, num_glyphs,
                                                dev_scaled_font);
	if (status != CAIRO_INT_STATUS_UNSUPPORTED)
            goto FINISH;
    }

    status = _cairo_surface_fallback_show_glyphs (surface, op, &dev_source.base,
                                                  glyphs, num_glyphs,
                                                  dev_scaled_font);

FINISH:
    if (dev_scaled_font != scaled_font)
	cairo_scaled_font_destroy (dev_scaled_font);

    _cairo_pattern_fini (&dev_source.base);

    return status;
}

/* XXX: Previously, we had a function named _cairo_surface_show_glyphs
 * with not-so-useful semantics. We've now got a new
 * _cairo_surface_show_glyphs with the proper semantics, and its
 * fallback still uses this old function (which still needs to be
 * cleaned up in terms of both semantics and naming). */
cairo_status_t
_cairo_surface_old_show_glyphs (cairo_scaled_font_t	*scaled_font,
				cairo_operator_t	 op,
				cairo_pattern_t		*pattern,
				cairo_surface_t		*dst,
				int			 source_x,
				int			 source_y,
				int			 dest_x,
				int			 dest_y,
				unsigned int		 width,
				unsigned int		 height,
				const cairo_glyph_t	*glyphs,
				int			 num_glyphs)
{
    cairo_status_t status;

    assert (! dst->is_snapshot);

    if (dst->status)
	return dst->status;

    if (dst->finished)
	return CAIRO_STATUS_SURFACE_FINISHED;

    if (dst->backend->old_show_glyphs) {
	status = dst->backend->old_show_glyphs (scaled_font,
						op, pattern, dst,
						source_x, source_y,
                                                dest_x, dest_y,
						width, height,
						glyphs, num_glyphs);
    } else
	status = CAIRO_INT_STATUS_UNSUPPORTED;

    return status;
}

static cairo_status_t
_cairo_surface_composite_fixup_unbounded_internal (cairo_surface_t         *dst,
						   cairo_rectangle_int16_t *src_rectangle,
						   cairo_rectangle_int16_t *mask_rectangle,
						   int			    dst_x,
						   int			    dst_y,
						   unsigned int		    width,
						   unsigned int		    height)
{
    cairo_rectangle_int16_t dst_rectangle;
    cairo_rectangle_int16_t drawn_rectangle;
    pixman_region16_t *drawn_region;
    pixman_region16_t *clear_region;
    cairo_status_t status = CAIRO_STATUS_SUCCESS;

    /* The area that was drawn is the area in the destination rectangle but not within
     * the source or the mask.
     */
    dst_rectangle.x = dst_x;
    dst_rectangle.y = dst_y;
    dst_rectangle.width = width;
    dst_rectangle.height = height;

    drawn_rectangle = dst_rectangle;

    if (src_rectangle)
	_cairo_rectangle_intersect (&drawn_rectangle, src_rectangle);

    if (mask_rectangle)
	_cairo_rectangle_intersect (&drawn_rectangle, mask_rectangle);

    /* Now compute the area that is in dst_rectangle but not in drawn_rectangle
     */
    drawn_region = _cairo_region_create_from_rectangle (&drawn_rectangle);
    clear_region = _cairo_region_create_from_rectangle (&dst_rectangle);
    if (!drawn_region || !clear_region) {
	status = CAIRO_STATUS_NO_MEMORY;
	goto CLEANUP_REGIONS;
    }

    if (pixman_region_subtract (clear_region, clear_region, drawn_region) != PIXMAN_REGION_STATUS_SUCCESS) {
	status = CAIRO_STATUS_NO_MEMORY;
	goto CLEANUP_REGIONS;
    }

    status = _cairo_surface_fill_region (dst, CAIRO_OPERATOR_SOURCE,
					 CAIRO_COLOR_TRANSPARENT,
					 clear_region);

 CLEANUP_REGIONS:
    if (drawn_region)
	pixman_region_destroy (drawn_region);
    if (clear_region)
	pixman_region_destroy (clear_region);

    return status;
}

/**
 * _cairo_surface_composite_fixup_unbounded:
 * @dst: the destination surface
 * @src_attr: source surface attributes (from _cairo_pattern_acquire_surface())
 * @src_width: width of source surface
 * @src_height: height of source surface
 * @mask_attr: mask surface attributes or %NULL if no mask
 * @mask_width: width of mask surface
 * @mask_height: height of mask surface
 * @src_x: @src_x from _cairo_surface_composite()
 * @src_y: @src_y from _cairo_surface_composite()
 * @mask_x: @mask_x from _cairo_surface_composite()
 * @mask_y: @mask_y from _cairo_surface_composite()
 * @dst_x: @dst_x from _cairo_surface_composite()
 * @dst_y: @dst_y from _cairo_surface_composite()
 * @width: @width from _cairo_surface_composite()
 * @height: @height_x from _cairo_surface_composite()
 *
 * Eeek! Too many parameters! This is a helper function to take care of fixing
 * up for bugs in libpixman and RENDER where, when asked to composite an
 * untransformed surface with an unbounded operator (like CLEAR or SOURCE)
 * only the region inside both the source and the mask is affected.
 * This function clears the region that should have been drawn but was wasn't.
 **/
cairo_status_t
_cairo_surface_composite_fixup_unbounded (cairo_surface_t            *dst,
					  cairo_surface_attributes_t *src_attr,
					  int                         src_width,
					  int                         src_height,
					  cairo_surface_attributes_t *mask_attr,
					  int                         mask_width,
					  int                         mask_height,
					  int			      src_x,
					  int			      src_y,
					  int			      mask_x,
					  int			      mask_y,
					  int			      dst_x,
					  int			      dst_y,
					  unsigned int		      width,
					  unsigned int		      height)
{
    cairo_rectangle_int16_t src_tmp, mask_tmp;
    cairo_rectangle_int16_t *src_rectangle = NULL;
    cairo_rectangle_int16_t *mask_rectangle = NULL;

    assert (! dst->is_snapshot);

    /* The RENDER/libpixman operators are clipped to the bounds of the untransformed,
     * non-repeating sources and masks. Other sources and masks can be ignored.
     */
    if (_cairo_matrix_is_integer_translation (&src_attr->matrix, NULL, NULL) &&
	src_attr->extend == CAIRO_EXTEND_NONE)
    {
	src_tmp.x = (dst_x - (src_x + src_attr->x_offset));
	src_tmp.y = (dst_y - (src_y + src_attr->y_offset));
	src_tmp.width = src_width;
	src_tmp.height = src_height;

	src_rectangle = &src_tmp;
    }

    if (mask_attr &&
	_cairo_matrix_is_integer_translation (&mask_attr->matrix, NULL, NULL) &&
	mask_attr->extend == CAIRO_EXTEND_NONE)
    {
	mask_tmp.x = (dst_x - (mask_x + mask_attr->x_offset));
	mask_tmp.y = (dst_y - (mask_y + mask_attr->y_offset));
	mask_tmp.width = mask_width;
	mask_tmp.height = mask_height;

	mask_rectangle = &mask_tmp;
    }

    return _cairo_surface_composite_fixup_unbounded_internal (dst, src_rectangle, mask_rectangle,
							      dst_x, dst_y, width, height);
}

/**
 * _cairo_surface_composite_shape_fixup_unbounded:
 * @dst: the destination surface
 * @src_attr: source surface attributes (from _cairo_pattern_acquire_surface())
 * @src_width: width of source surface
 * @src_height: height of source surface
 * @mask_width: width of mask surface
 * @mask_height: height of mask surface
 * @src_x: @src_x from _cairo_surface_composite()
 * @src_y: @src_y from _cairo_surface_composite()
 * @mask_x: @mask_x from _cairo_surface_composite()
 * @mask_y: @mask_y from _cairo_surface_composite()
 * @dst_x: @dst_x from _cairo_surface_composite()
 * @dst_y: @dst_y from _cairo_surface_composite()
 * @width: @width from _cairo_surface_composite()
 * @height: @height_x from _cairo_surface_composite()
 *
 * Like _cairo_surface_composite_fixup_unbounded(), but instead of
 * handling the case where we have a source pattern and a mask
 * pattern, handle the case where we are compositing a source pattern
 * using a mask we create ourselves, as in
 * _cairo_surface_composite_glyphs() or _cairo_surface_composite_trapezoids()
 **/
cairo_status_t
_cairo_surface_composite_shape_fixup_unbounded (cairo_surface_t            *dst,
						cairo_surface_attributes_t *src_attr,
						int                         src_width,
						int                         src_height,
						int                         mask_width,
						int                         mask_height,
						int			    src_x,
						int			    src_y,
						int			    mask_x,
						int			    mask_y,
						int			    dst_x,
						int			    dst_y,
						unsigned int		    width,
						unsigned int		    height)
{
    cairo_rectangle_int16_t src_tmp, mask_tmp;
    cairo_rectangle_int16_t *src_rectangle = NULL;
    cairo_rectangle_int16_t *mask_rectangle = NULL;

    assert (! dst->is_snapshot);

    /* The RENDER/libpixman operators are clipped to the bounds of the untransformed,
     * non-repeating sources and masks. Other sources and masks can be ignored.
     */
    if (_cairo_matrix_is_integer_translation (&src_attr->matrix, NULL, NULL) &&
	src_attr->extend == CAIRO_EXTEND_NONE)
    {
	src_tmp.x = (dst_x - (src_x + src_attr->x_offset));
	src_tmp.y = (dst_y - (src_y + src_attr->y_offset));
	src_tmp.width = src_width;
	src_tmp.height = src_height;

	src_rectangle = &src_tmp;
    }

    mask_tmp.x = dst_x - mask_x;
    mask_tmp.y = dst_y - mask_y;
    mask_tmp.width = mask_width;
    mask_tmp.height = mask_height;

    mask_rectangle = &mask_tmp;

    return _cairo_surface_composite_fixup_unbounded_internal (dst, src_rectangle, mask_rectangle,
							      dst_x, dst_y, width, height);
}

/**
 * _cairo_surface_copy_pattern_for_destination
 * @pattern: the pattern to copy
 * @destination: the destination surface for which the pattern is being copied
 * @pattern_out: the location to hold the copy
 *
 * Copies the given pattern, taking into account device scale and offsets
 * of the destination surface.
 */
void
_cairo_surface_copy_pattern_for_destination (const cairo_pattern_t *pattern,
                                             cairo_surface_t *destination,
                                             cairo_pattern_t *pattern_out)
{
    _cairo_pattern_init_copy (pattern_out, pattern);

    if (_cairo_surface_has_device_transform (destination)) {
	cairo_matrix_t device_to_surface = destination->device_transform;
	cairo_status_t status;

	status = cairo_matrix_invert (&device_to_surface);
	/* We only ever allow for scaling (under the implementation's
	 * control) or translation (under the user's control). So the
	 * matrix should always be invertible. */
	assert (status == CAIRO_STATUS_SUCCESS);

	_cairo_pattern_transform (pattern_out, &device_to_surface);
    }
}

/*  LocalWords:  rasterized
 */
