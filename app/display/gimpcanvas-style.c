/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvas-style.c
 * Copyright (C) 2010  Michael Natterer <mitch@ligma.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmawidgets/ligmawidgets.h"

#include "display-types.h"

#include "core/ligma-cairo.h"
#include "core/ligmagrid.h"
#include "core/ligmalayer.h"

#include "ligmacanvas-style.h"

/* Styles for common and custom guides. */
static const LigmaRGB guide_normal_fg         = { 0.0, 0.0, 0.0, 1.0 };
static const LigmaRGB guide_normal_bg         = { 0.0, 0.8, 1.0, 1.0 };
static const LigmaRGB guide_active_fg         = { 0.0, 0.0, 0.0, 1.0 };
static const LigmaRGB guide_active_bg         = { 1.0, 0.0, 0.0, 1.0 };

static const LigmaRGB guide_mirror_normal_fg  = { 1.0, 1.0, 1.0, 1.0 };
static const LigmaRGB guide_mirror_normal_bg  = { 0.0, 1.0, 0.0, 1.0 };
static const LigmaRGB guide_mirror_active_fg  = { 0.0, 1.0, 0.0, 1.0 };
static const LigmaRGB guide_mirror_active_bg  = { 1.0, 0.0, 0.0, 1.0 };

static const LigmaRGB guide_mandala_normal_fg = { 1.0, 1.0, 1.0, 1.0 };
static const LigmaRGB guide_mandala_normal_bg = { 0.0, 1.0, 1.0, 1.0 };
static const LigmaRGB guide_mandala_active_fg = { 0.0, 1.0, 1.0, 1.0 };
static const LigmaRGB guide_mandala_active_bg = { 1.0, 0.0, 0.0, 1.0 };

static const LigmaRGB guide_split_normal_fg   = { 1.0, 1.0, 1.0, 1.0 };
static const LigmaRGB guide_split_normal_bg   = { 1.0, 0.0, 1.0, 1.0 };
static const LigmaRGB guide_split_active_fg   = { 1.0, 0.0, 1.0, 1.0 };
static const LigmaRGB guide_split_active_bg   = { 1.0, 0.0, 0.0, 1.0 };

/* Styles for other canvas items. */
static const LigmaRGB sample_point_normal = { 0.0, 0.8, 1.0, 1.0 };
static const LigmaRGB sample_point_active = { 1.0, 0.0, 0.0, 1.0 };

static const LigmaRGB layer_fg            = { 0.0, 0.0, 0.0, 1.0 };
static const LigmaRGB layer_bg            = { 1.0, 1.0, 0.0, 1.0 };

static const LigmaRGB layer_group_fg      = { 0.0, 0.0, 0.0, 1.0 };
static const LigmaRGB layer_group_bg      = { 0.0, 1.0, 1.0, 1.0 };

static const LigmaRGB layer_mask_fg       = { 0.0, 0.0, 0.0, 1.0 };
static const LigmaRGB layer_mask_bg       = { 0.0, 1.0, 0.0, 1.0 };

static const LigmaRGB canvas_fg           = { 0.0, 0.0, 0.0, 1.0 };
static const LigmaRGB canvas_bg           = { 1.0, 0.5, 0.0, 1.0 };

static const LigmaRGB selection_out_fg    = { 1.0, 1.0, 1.0, 1.0 };
static const LigmaRGB selection_out_bg    = { 0.5, 0.5, 0.5, 1.0 };

static const LigmaRGB selection_in_fg     = { 0.0, 0.0, 0.0, 1.0 };
static const LigmaRGB selection_in_bg     = { 1.0, 1.0, 1.0, 1.0 };

static const LigmaRGB vectors_normal_bg   = { 1.0, 1.0, 1.0, 0.6 };
static const LigmaRGB vectors_normal_fg   = { 0.0, 0.0, 1.0, 0.8 };

static const LigmaRGB vectors_active_bg   = { 1.0, 1.0, 1.0, 0.6 };
static const LigmaRGB vectors_active_fg   = { 1.0, 0.0, 0.0, 0.8 };

static const LigmaRGB outline_bg          = { 1.0, 1.0, 1.0, 0.6 };
static const LigmaRGB outline_fg          = { 0.0, 0.0, 0.0, 0.8 };

static const LigmaRGB passe_partout       = { 0.0, 0.0, 0.0, 1.0 };

static const LigmaRGB tool_bg             = { 0.0, 0.0, 0.0, 0.4 };
static const LigmaRGB tool_fg             = { 1.0, 1.0, 1.0, 0.8 };
static const LigmaRGB tool_fg_highlight   = { 1.0, 0.8, 0.2, 0.8 };


/*  public functions  */

void
ligma_canvas_set_guide_style (GtkWidget      *canvas,
                             cairo_t        *cr,
                             LigmaGuideStyle  style,
                             gboolean        active,
                             gdouble         offset_x,
                             gdouble         offset_y)
{
  cairo_pattern_t *pattern;
  LigmaRGB          normal_fg;
  LigmaRGB          normal_bg;
  LigmaRGB          active_fg;
  LigmaRGB          active_bg;
  gdouble          line_width;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  switch (style)
    {
    case LIGMA_GUIDE_STYLE_NORMAL:
      normal_fg  = guide_normal_fg;
      normal_bg  = guide_normal_bg;
      active_fg  = guide_active_fg;
      active_bg  = guide_active_bg;
      line_width = 1.0;
      break;

    case LIGMA_GUIDE_STYLE_MIRROR:
      normal_fg  = guide_mirror_normal_fg;
      normal_bg  = guide_mirror_normal_bg;
      active_fg  = guide_mirror_active_fg;
      active_bg  = guide_mirror_active_bg;
      line_width = 1.0;
      break;

    case LIGMA_GUIDE_STYLE_MANDALA:
      normal_fg  = guide_mandala_normal_fg;
      normal_bg  = guide_mandala_normal_bg;
      active_fg  = guide_mandala_active_fg;
      active_bg  = guide_mandala_active_bg;
      line_width = 1.0;
      break;

    case LIGMA_GUIDE_STYLE_SPLIT_VIEW:
      normal_fg  = guide_split_normal_fg;
      normal_bg  = guide_split_normal_bg;
      active_fg  = guide_split_active_fg;
      active_bg  = guide_split_active_bg;
      line_width = 1.0;
      break;

    default: /* LIGMA_GUIDE_STYLE_NONE */
      /* This should not happen. */
      g_return_if_reached ();
    }

  cairo_set_line_width (cr, line_width);

  if (active)
    pattern = ligma_cairo_pattern_create_stipple (&active_fg, &active_bg, 0,
                                                 offset_x, offset_y);
  else
    pattern = ligma_cairo_pattern_create_stipple (&normal_fg, &normal_bg, 0,
                                                 offset_x, offset_y);

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
ligma_canvas_set_sample_point_style (GtkWidget *canvas,
                                    cairo_t   *cr,
                                    gboolean   active)
{
  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);

  if (active)
    ligma_cairo_set_source_rgb (cr, &sample_point_active);
  else
    ligma_cairo_set_source_rgb (cr, &sample_point_normal);
}

void
ligma_canvas_set_grid_style (GtkWidget *canvas,
                            cairo_t   *cr,
                            LigmaGrid  *grid,
                            gdouble    offset_x,
                            gdouble    offset_y)
{
  LigmaRGB fg;
  LigmaRGB bg;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (LIGMA_IS_GRID (grid));

  cairo_set_line_width (cr, 1.0);

  ligma_grid_get_fgcolor (grid, &fg);

  switch (ligma_grid_get_style (grid))
    {
      cairo_pattern_t *pattern;

    case LIGMA_GRID_ON_OFF_DASH:
    case LIGMA_GRID_DOUBLE_DASH:
      if (grid->style == LIGMA_GRID_DOUBLE_DASH)
        {
          ligma_grid_get_bgcolor (grid, &bg);

          pattern = ligma_cairo_pattern_create_stipple (&fg, &bg, 0,
                                                       offset_x, offset_y);
        }
      else
        {
          ligma_rgba_set (&bg, 0.0, 0.0, 0.0, 0.0);

          pattern = ligma_cairo_pattern_create_stipple (&fg, &bg, 0,
                                                       offset_x, offset_y);
        }

      cairo_set_source (cr, pattern);
      cairo_pattern_destroy (pattern);
      break;

    case LIGMA_GRID_DOTS:
    case LIGMA_GRID_INTERSECTIONS:
    case LIGMA_GRID_SOLID:
      ligma_cairo_set_source_rgb (cr, &fg);
      break;
    }
}

void
ligma_canvas_set_pen_style (GtkWidget     *canvas,
                           cairo_t       *cr,
                           const LigmaRGB *color,
                           gint           width)
{
  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (color != NULL);

  cairo_set_antialias (cr, CAIRO_ANTIALIAS_NONE);
  cairo_set_line_width (cr, width);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  ligma_cairo_set_source_rgb (cr, color);
}

void
ligma_canvas_set_layer_style (GtkWidget *canvas,
                             cairo_t   *cr,
                             LigmaLayer *layer,
                             gdouble    offset_x,
                             gdouble    offset_y)
{
  cairo_pattern_t *pattern;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (LIGMA_IS_LAYER (layer));

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  if (ligma_layer_get_mask (layer) &&
      ligma_layer_get_edit_mask (layer))
    {
      pattern = ligma_cairo_pattern_create_stipple (&layer_mask_fg,
                                                   &layer_mask_bg,
                                                   0,
                                                   offset_x, offset_y);
    }
  else if (ligma_viewable_get_children (LIGMA_VIEWABLE (layer)))
    {
      pattern = ligma_cairo_pattern_create_stipple (&layer_group_fg,
                                                   &layer_group_bg,
                                                   0,
                                                   offset_x, offset_y);
    }
  else
    {
      pattern = ligma_cairo_pattern_create_stipple (&layer_fg,
                                                   &layer_bg,
                                                   0,
                                                   offset_x, offset_y);
    }

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
ligma_canvas_set_canvas_style (GtkWidget *canvas,
                              cairo_t   *cr,
                              gdouble    offset_x,
                              gdouble    offset_y)
{
  cairo_pattern_t *pattern;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  pattern = ligma_cairo_pattern_create_stipple (&canvas_fg,
                                               &canvas_bg,
                                               0,
                                               offset_x, offset_y);

  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
ligma_canvas_set_selection_out_style (GtkWidget *canvas,
                                     cairo_t   *cr,
                                     gdouble    offset_x,
                                     gdouble    offset_y)
{
  cairo_pattern_t *pattern;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  pattern = ligma_cairo_pattern_create_stipple (&selection_out_fg,
                                               &selection_out_bg,
                                               0,
                                               offset_x, offset_y);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
ligma_canvas_set_selection_in_style (GtkWidget *canvas,
                                    cairo_t   *cr,
                                    gint       index,
                                    gdouble    offset_x,
                                    gdouble    offset_y)
{
  cairo_pattern_t *pattern;

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  pattern = ligma_cairo_pattern_create_stipple (&selection_in_fg,
                                               &selection_in_bg,
                                               index,
                                               offset_x, offset_y);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
}

void
ligma_canvas_set_vectors_bg_style (GtkWidget *canvas,
                                  cairo_t   *cr,
                                  gboolean   active)
{
  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 3.0);

  if (active)
    ligma_cairo_set_source_rgba (cr, &vectors_active_bg);
  else
    ligma_cairo_set_source_rgba (cr, &vectors_normal_bg);
}

void
ligma_canvas_set_vectors_fg_style (GtkWidget *canvas,
                                  cairo_t   *cr,
                                  gboolean   active)
{
  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);

  if (active)
    ligma_cairo_set_source_rgba (cr, &vectors_active_fg);
  else
    ligma_cairo_set_source_rgba (cr, &vectors_normal_fg);
}

void
ligma_canvas_set_outline_bg_style (GtkWidget *canvas,
                                  cairo_t   *cr)
{
  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  ligma_cairo_set_source_rgba (cr, &outline_bg);
}

void
ligma_canvas_set_outline_fg_style (GtkWidget *canvas,
                                  cairo_t   *cr)
{
  static const double dashes[] = { 4.0, 4.0 };

  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  ligma_cairo_set_source_rgba (cr, &outline_fg);
  cairo_set_dash (cr, dashes, G_N_ELEMENTS (dashes), 0);
}

void
ligma_canvas_set_passe_partout_style (GtkWidget *canvas,
                                     cairo_t   *cr)
{
  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  ligma_cairo_set_source_rgba (cr, &passe_partout);
}

void
ligma_canvas_set_tool_bg_style (GtkWidget *canvas,
                               cairo_t   *cr)
{
  g_return_if_fail (GTK_IS_WIDGET (canvas));
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 3.0);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  ligma_cairo_set_source_rgba (cr, &tool_bg);
}

void
ligma_canvas_set_tool_fg_style (GtkWidget *canvas,
                               cairo_t   *cr,
                               gboolean   highlight)
{
  g_return_if_fail (cr != NULL);

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  if (highlight)
    ligma_cairo_set_source_rgba (cr, &tool_fg_highlight);
  else
    ligma_cairo_set_source_rgba (cr, &tool_fg);
}
