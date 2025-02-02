/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacellrenderercolor.c
 * Copyright (C) 2004,2007  Sven Neuman <sven1@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "ligmawidgetstypes.h"

#include "ligmacairo-utils.h"
#include "ligmacellrenderercolor.h"


/**
 * SECTION: ligmacellrenderercolor
 * @title: LigmaCellRendererColor
 * @short_description: A #GtkCellRenderer to display a #LigmaRGB color.
 *
 * A #GtkCellRenderer to display a #LigmaRGB color.
 **/


#define DEFAULT_ICON_SIZE  GTK_ICON_SIZE_MENU


enum
{
  PROP_0,
  PROP_COLOR,
  PROP_OPAQUE,
  PROP_SIZE
};


struct _LigmaCellRendererColorPrivate
{
  LigmaRGB     color;
  gboolean    opaque;
  GtkIconSize size;
  gint        border;
};

#define GET_PRIVATE(obj) (((LigmaCellRendererColor *) (obj))->priv)


static void ligma_cell_renderer_color_get_property (GObject            *object,
                                                   guint               param_id,
                                                   GValue             *value,
                                                   GParamSpec         *pspec);
static void ligma_cell_renderer_color_set_property (GObject            *object,
                                                   guint               param_id,
                                                   const GValue       *value,
                                                   GParamSpec         *pspec);
static void ligma_cell_renderer_color_get_size     (GtkCellRenderer    *cell,
                                                   GtkWidget          *widget,
                                                   const GdkRectangle *rectangle,
                                                   gint               *x_offset,
                                                   gint               *y_offset,
                                                   gint               *width,
                                                   gint               *height);
static void ligma_cell_renderer_color_render       (GtkCellRenderer    *cell,
                                                   cairo_t            *cr,
                                                   GtkWidget          *widget,
                                                   const GdkRectangle *background_area,
                                                   const GdkRectangle *cell_area,
                                                   GtkCellRendererState flags);



G_DEFINE_TYPE_WITH_PRIVATE (LigmaCellRendererColor, ligma_cell_renderer_color,
                            GTK_TYPE_CELL_RENDERER)

#define parent_class ligma_cell_renderer_color_parent_class


static void
ligma_cell_renderer_color_class_init (LigmaCellRendererColorClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  object_class->get_property = ligma_cell_renderer_color_get_property;
  object_class->set_property = ligma_cell_renderer_color_set_property;

  cell_class->get_size       = ligma_cell_renderer_color_get_size;
  cell_class->render         = ligma_cell_renderer_color_render;

  g_object_class_install_property (object_class, PROP_COLOR,
                                   g_param_spec_boxed ("color",
                                                       "Color",
                                                       "The displayed color",
                                                       LIGMA_TYPE_RGB,
                                                       LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_OPAQUE,
                                   g_param_spec_boolean ("opaque",
                                                         "Opaque",
                                                         "Whether to show transparency",
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SIZE,
                                   g_param_spec_int ("icon-size",
                                                     "Icon Size",
                                                     "The cell's size",
                                                     0, G_MAXINT,
                                                     DEFAULT_ICON_SIZE,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
ligma_cell_renderer_color_init (LigmaCellRendererColor *cell)
{
  cell->priv = ligma_cell_renderer_color_get_instance_private (cell);

  ligma_rgba_set (&cell->priv->color, 0.0, 0.0, 0.0, 1.0);
}

static void
ligma_cell_renderer_color_get_property (GObject    *object,
                                       guint       param_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  LigmaCellRendererColorPrivate *private = GET_PRIVATE (object);

  switch (param_id)
    {
    case PROP_COLOR:
      g_value_set_boxed (value, &private->color);
      break;
    case PROP_OPAQUE:
      g_value_set_boolean (value, private->opaque);
      break;
    case PROP_SIZE:
      g_value_set_int (value, private->size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
ligma_cell_renderer_color_set_property (GObject      *object,
                                       guint         param_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  LigmaCellRendererColorPrivate *private = GET_PRIVATE (object);
  LigmaRGB                      *color;

  switch (param_id)
    {
    case PROP_COLOR:
      color = g_value_get_boxed (value);
      private->color = *color;
      break;
    case PROP_OPAQUE:
      private->opaque = g_value_get_boolean (value);
      break;
    case PROP_SIZE:
      private->size = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
ligma_cell_renderer_color_get_size (GtkCellRenderer    *cell,
                                   GtkWidget          *widget,
                                   const GdkRectangle *cell_area,
                                   gint               *x_offset,
                                   gint               *y_offset,
                                   gint               *width,
                                   gint               *height)
{
  LigmaCellRendererColorPrivate *private = GET_PRIVATE (cell);
  gint                          calc_width;
  gint                          calc_height;
  gfloat                        xalign;
  gfloat                        yalign;
  gint                          xpad;
  gint                          ypad;

  gtk_icon_size_lookup (private->size, &calc_width, &calc_height);
  gtk_cell_renderer_get_alignment (cell, &xalign, &yalign);
  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  if (cell_area && calc_width > 0 && calc_height > 0)
    {
      if (x_offset)
        {
          *x_offset = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                        1.0 - xalign : xalign) *
                       (cell_area->width - calc_width));
          *x_offset = MAX (*x_offset, 0) + xpad;
        }
      if (y_offset)
        {
          *y_offset = (yalign * (cell_area->height - calc_height));
          *y_offset = MAX (*y_offset, 0) + ypad;
        }
    }
  else
    {
      if (x_offset)
        *x_offset = 0;
      if (y_offset)
        *y_offset = 0;
    }

  if (width)
    *width  = calc_width  + 2 * xpad;
  if (height)
    *height = calc_height + 2 * ypad;
}

static void
ligma_cell_renderer_color_render (GtkCellRenderer      *cell,
                                 cairo_t              *cr,
                                 GtkWidget            *widget,
                                 const GdkRectangle   *background_area,
                                 const GdkRectangle   *cell_area,
                                 GtkCellRendererState  flags)
{
  LigmaCellRendererColorPrivate *private = GET_PRIVATE (cell);
  GdkRectangle                  rect;
  gint                          xpad;
  gint                          ypad;

  ligma_cell_renderer_color_get_size (cell, widget, cell_area,
                                     &rect.x,
                                     &rect.y,
                                     &rect.width,
                                     &rect.height);

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  rect.x      += cell_area->x + xpad;
  rect.y      += cell_area->y + ypad;
  rect.width  -= 2 * xpad;
  rect.height -= 2 * ypad;

  if (rect.width > 2 && rect.height > 2)
    {
      GtkStyleContext *context = gtk_widget_get_style_context (widget);
      GtkStateFlags    state;
      GdkRGBA          color;

      cairo_rectangle (cr,
                       rect.x + 1, rect.y + 1,
                       rect.width - 2, rect.height - 2);

      ligma_cairo_set_source_rgb (cr, &private->color);
      cairo_fill (cr);

      if (! private->opaque && private->color.a < 1.0)
        {
          cairo_pattern_t *pattern;

          cairo_move_to (cr, rect.x + 1,              rect.y + rect.height - 1);
          cairo_line_to (cr, rect.x + rect.width - 1, rect.y + rect.height - 1);
          cairo_line_to (cr, rect.x + rect.width - 1, rect.y + 1);
          cairo_close_path (cr);

          pattern = ligma_cairo_checkerboard_create (cr,
                                                    LIGMA_CHECK_SIZE_SM,
                                                    NULL, NULL);
          cairo_set_source (cr, pattern);
          cairo_pattern_destroy (pattern);

          cairo_fill_preserve (cr);

          ligma_cairo_set_source_rgba (cr, &private->color);
          cairo_fill (cr);
        }

      /* draw border */
      cairo_rectangle (cr,
                       rect.x + 0.5, rect.y + 0.5,
                       rect.width - 1, rect.height - 1);

      state = gtk_cell_renderer_get_state (cell, widget, flags);

      cairo_set_line_width (cr, 1);
      gtk_style_context_get_color (context, state, &color);
      gdk_cairo_set_source_rgba (cr, &color);
      cairo_stroke (cr);
    }
}

/**
 * ligma_cell_renderer_color_new:
 *
 * Creates a #GtkCellRenderer that displays a color.
 *
 * Returns: a new #LigmaCellRendererColor
 *
 * Since: 2.2
 **/
GtkCellRenderer *
ligma_cell_renderer_color_new (void)
{
  return g_object_new (LIGMA_TYPE_CELL_RENDERER_COLOR, NULL);
}
