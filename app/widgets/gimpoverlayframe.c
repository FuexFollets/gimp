/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoverlayframe.c
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

#include <gtk/gtk.h>

#include "libligmamath/ligmamath.h"

#include "widgets-types.h"

#include "core/ligma-cairo.h"

#include "ligmaoverlayframe.h"
#include "ligmawidgets-utils.h"


static gboolean   ligma_overlay_frame_draw (GtkWidget *widget,
                                           cairo_t   *cr);


G_DEFINE_TYPE (LigmaOverlayFrame, ligma_overlay_frame, GTK_TYPE_BIN)

#define parent_class ligma_overlay_frame_parent_class


static void
ligma_overlay_frame_class_init (LigmaOverlayFrameClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->draw = ligma_overlay_frame_draw;

  gtk_widget_class_set_css_name (widget_class, "popover");
}

static void
ligma_overlay_frame_init (LigmaOverlayFrame *frame)
{
  gtk_widget_set_app_paintable (GTK_WIDGET (frame), TRUE);

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (frame)),
                               "background");
}

static gboolean
ligma_overlay_frame_draw (GtkWidget *widget,
                         cairo_t   *cr)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  gboolean         rgba;
  gint             border_radius;

  rgba = gdk_screen_get_rgba_visual (gtk_widget_get_screen (widget)) != NULL;

  if (rgba)
    {
      cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint (cr);
      cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    }

  gtk_widget_get_allocation (widget, &allocation);

  gtk_style_context_get (style, gtk_style_context_get_state (style),
                         "border-radius", &border_radius,
                         NULL);

  if (rgba)
    {
      ligma_cairo_rounded_rectangle (cr,
                                    0.0,              0.0,
                                    allocation.width, allocation.height,
                                    border_radius);
    }
  else
    {
      cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
    }

  cairo_clip (cr);

  gtk_render_background (style, cr,
                         0, 0,
                         allocation.width,
                         allocation.height);

  gtk_render_frame (style, cr,
                    0, 0,
                    allocation.width,
                    allocation.height);

  return GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
}

GtkWidget *
ligma_overlay_frame_new (void)
{
  return g_object_new (LIGMA_TYPE_OVERLAY_FRAME, NULL);
}
