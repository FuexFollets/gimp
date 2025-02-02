/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacairo.c
 * Copyright (C) 2007      Sven Neumann <sven@ligma.org>
 *               2010-2012 Michael Natterer <mitch@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gio/gio.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "ligmacolortypes.h"

#include "ligmacairo.h"


/**
 * SECTION: ligmacairo
 * @title: LigmaCairo
 * @short_description: Color utility functions for cairo
 *
 * Utility functions that make cairo easier to use with LIGMA color
 * data types.
 **/


/**
 * ligma_cairo_set_source_rgb:
 * @cr:    Cairo context
 * @color: LigmaRGB color
 *
 * Sets the source pattern within @cr to the solid opaque color
 * described by @color.
 *
 * This function calls cairo_set_source_rgb() for you.
 *
 * Since: 2.6
 **/
void
ligma_cairo_set_source_rgb (cairo_t       *cr,
                           const LigmaRGB *color)
{
  cairo_set_source_rgb (cr, color->r, color->g, color->b);
}

/**
 * ligma_cairo_set_source_rgba:
 * @cr:    Cairo context
 * @color: LigmaRGB color
 *
 * Sets the source pattern within @cr to the solid translucent color
 * described by @color.
 *
 * This function calls cairo_set_source_rgba() for you.
 *
 * Since: 2.6
 **/
void
ligma_cairo_set_source_rgba (cairo_t       *cr,
                            const LigmaRGB *color)
{
  cairo_set_source_rgba (cr, color->r, color->g, color->b, color->a);
}

/**
 * ligma_cairo_checkerboard_create:
 * @cr:    Cairo context
 * @size:  check size
 * @light: light check color or %NULL to use the default light gray
 * @dark:  dark check color or %NULL to use the default dark gray
 *
 * Create a repeating checkerboard pattern.
 *
 * Returns: a new Cairo pattern that can be used as a source on @cr.
 *
 * Since: 2.6
 **/
cairo_pattern_t *
ligma_cairo_checkerboard_create (cairo_t       *cr,
                                gint           size,
                                const LigmaRGB *light,
                                const LigmaRGB *dark)
{
  cairo_t         *context;
  cairo_surface_t *surface;
  cairo_pattern_t *pattern;

  g_return_val_if_fail (cr != NULL, NULL);
  g_return_val_if_fail (size > 0, NULL);

  surface = cairo_surface_create_similar (cairo_get_target (cr),
                                          CAIRO_CONTENT_COLOR,
                                          2 * size, 2 * size);
  context = cairo_create (surface);

  if (light)
    ligma_cairo_set_source_rgb (context, light);
  else
    cairo_set_source_rgb (context,
                          LIGMA_CHECK_LIGHT, LIGMA_CHECK_LIGHT, LIGMA_CHECK_LIGHT);

  cairo_rectangle (context, 0,    0,    size, size);
  cairo_rectangle (context, size, size, size, size);
  cairo_fill (context);

  if (dark)
    ligma_cairo_set_source_rgb (context, dark);
  else
    cairo_set_source_rgb (context,
                          LIGMA_CHECK_DARK, LIGMA_CHECK_DARK, LIGMA_CHECK_DARK);

  cairo_rectangle (context, 0,    size, size, size);
  cairo_rectangle (context, size, 0,    size, size);
  cairo_fill (context);

  cairo_destroy (context);

  pattern = cairo_pattern_create_for_surface (surface);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);

  cairo_surface_destroy (surface);

  return pattern;
}

/**
 * ligma_cairo_surface_get_format:
 * @surface: a Cairo surface
 *
 * This function returns a #Babl format that corresponds to @surface's
 * pixel format.
 *
 * Returns: the #Babl format of @surface.
 *
 * Since: 2.10
 **/
const Babl *
ligma_cairo_surface_get_format (cairo_surface_t *surface)
{
  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (cairo_surface_get_type (surface) ==
                        CAIRO_SURFACE_TYPE_IMAGE, NULL);

  switch (cairo_image_surface_get_format (surface))
    {
    case CAIRO_FORMAT_RGB24:  return babl_format ("cairo-RGB24");
    case CAIRO_FORMAT_ARGB32: return babl_format ("cairo-ARGB32");
    case CAIRO_FORMAT_A8:     return babl_format ("cairo-A8");

    default:
      break;
    }

  g_return_val_if_reached (NULL);
}

/**
 * ligma_cairo_surface_create_buffer:
 * @surface: a Cairo surface
 *
 * This function returns a #GeglBuffer which wraps @surface's pixels.
 * It must only be called on image surfaces, calling it on other surface
 * types is an error.
 *
 * Returns: (transfer full): a #GeglBuffer
 *
 * Since: 2.10
 **/
GeglBuffer *
ligma_cairo_surface_create_buffer (cairo_surface_t *surface)
{
  const Babl *format;
  gint        width;
  gint        height;

  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (cairo_surface_get_type (surface) ==
                        CAIRO_SURFACE_TYPE_IMAGE, NULL);

  format = ligma_cairo_surface_get_format  (surface);
  width  = cairo_image_surface_get_width  (surface);
  height = cairo_image_surface_get_height (surface);

  return
    gegl_buffer_linear_new_from_data (cairo_image_surface_get_data (surface),
                                      format,
                                      GEGL_RECTANGLE (0, 0, width, height),
                                      cairo_image_surface_get_stride (surface),
                                      (GDestroyNotify) cairo_surface_destroy,
                                      cairo_surface_reference (surface));
}
