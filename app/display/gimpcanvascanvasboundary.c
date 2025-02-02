/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvascanvasboundary.c
 * Copyright (C) 2019 Ell
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

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "core/ligmaimage.h"

#include "ligmacanvas-style.h"
#include "ligmacanvascanvasboundary.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_IMAGE
};


typedef struct _LigmaCanvasCanvasBoundaryPrivate LigmaCanvasCanvasBoundaryPrivate;

struct _LigmaCanvasCanvasBoundaryPrivate
{
  LigmaImage *image;
};

#define GET_PRIVATE(canvas_boundary) \
        ((LigmaCanvasCanvasBoundaryPrivate *) ligma_canvas_canvas_boundary_get_instance_private ((LigmaCanvasCanvasBoundary *) (canvas_boundary)))


/*  local function prototypes  */

static void             ligma_canvas_canvas_boundary_set_property (GObject        *object,
                                                                  guint           property_id,
                                                                  const GValue   *value,
                                                                  GParamSpec     *pspec);
static void             ligma_canvas_canvas_boundary_get_property (GObject        *object,
                                                                  guint           property_id,
                                                                  GValue         *value,
                                                                  GParamSpec     *pspec);
static void             ligma_canvas_canvas_boundary_finalize     (GObject        *object);
static void             ligma_canvas_canvas_boundary_draw         (LigmaCanvasItem *item,
                                                                  cairo_t        *cr);
static cairo_region_t * ligma_canvas_canvas_boundary_get_extents  (LigmaCanvasItem *item);
static void             ligma_canvas_canvas_boundary_stroke       (LigmaCanvasItem *item,
                                                                  cairo_t        *cr);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasCanvasBoundary, ligma_canvas_canvas_boundary,
                            LIGMA_TYPE_CANVAS_RECTANGLE)

#define parent_class ligma_canvas_canvas_boundary_parent_class


static void
ligma_canvas_canvas_boundary_class_init (LigmaCanvasCanvasBoundaryClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = ligma_canvas_canvas_boundary_set_property;
  object_class->get_property = ligma_canvas_canvas_boundary_get_property;
  object_class->finalize     = ligma_canvas_canvas_boundary_finalize;

  item_class->draw           = ligma_canvas_canvas_boundary_draw;
  item_class->get_extents    = ligma_canvas_canvas_boundary_get_extents;
  item_class->stroke         = ligma_canvas_canvas_boundary_stroke;

  g_object_class_install_property (object_class, PROP_IMAGE,
                                   g_param_spec_object ("image", NULL, NULL,
                                                        LIGMA_TYPE_IMAGE,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_canvas_boundary_init (LigmaCanvasCanvasBoundary *canvas_boundary)
{
}

static void
ligma_canvas_canvas_boundary_finalize (GObject *object)
{
  LigmaCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  if (private->image)
    g_object_remove_weak_pointer (G_OBJECT (private->image),
                                  (gpointer) &private->image);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_canvas_canvas_boundary_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  LigmaCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      if (private->image)
        g_object_remove_weak_pointer (G_OBJECT (private->image),
                                      (gpointer) &private->image);
      private->image = g_value_get_object (value); /* don't ref */
      if (private->image)
        g_object_add_weak_pointer (G_OBJECT (private->image),
                                   (gpointer) &private->image);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_canvas_boundary_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  LigmaCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, private->image);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_canvas_boundary_draw (LigmaCanvasItem *item,
                                  cairo_t        *cr)
{
  LigmaCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (item);

  if (private->image)
    LIGMA_CANVAS_ITEM_CLASS (parent_class)->draw (item, cr);
}

static cairo_region_t *
ligma_canvas_canvas_boundary_get_extents (LigmaCanvasItem *item)
{
  LigmaCanvasCanvasBoundaryPrivate *private = GET_PRIVATE (item);

  if (private->image)
    return LIGMA_CANVAS_ITEM_CLASS (parent_class)->get_extents (item);

  return NULL;
}

static void
ligma_canvas_canvas_boundary_stroke (LigmaCanvasItem *item,
                                    cairo_t        *cr)
{
  LigmaDisplayShell *shell = ligma_canvas_item_get_shell (item);

  ligma_canvas_set_canvas_style (ligma_canvas_item_get_canvas (item), cr,
                                shell->offset_x, shell->offset_y);
  cairo_stroke (cr);
}

LigmaCanvasItem *
ligma_canvas_canvas_boundary_new (LigmaDisplayShell *shell)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_CANVAS_BOUNDARY,
                       "shell", shell,
                       NULL);
}

void
ligma_canvas_canvas_boundary_set_image (LigmaCanvasCanvasBoundary *boundary,
                                       LigmaImage                *image)
{
  LigmaCanvasCanvasBoundaryPrivate *private;

  g_return_if_fail (LIGMA_IS_CANVAS_CANVAS_BOUNDARY (boundary));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));

  private = GET_PRIVATE (boundary);

  if (image != private->image)
    {
      ligma_canvas_item_begin_change (LIGMA_CANVAS_ITEM (boundary));

      if (image)
        {
          g_object_set (boundary,
                        "x",      (gdouble) 0,
                        "y",      (gdouble) 0,
                        "width",  (gdouble) ligma_image_get_width  (image),
                        "height", (gdouble) ligma_image_get_height (image),
                        NULL);
        }

      g_object_set (boundary,
                    "image", image,
                    NULL);

      ligma_canvas_item_end_change (LIGMA_CANVAS_ITEM (boundary));
    }
  else if (image && image == private->image)
    {
      gint    lx, ly, lw, lh;
      gdouble x, y, w ,h;

      lx = 0;
      ly = 0;
      lw = ligma_image_get_width  (image);
      lh = ligma_image_get_height (image);

      g_object_get (boundary,
                    "x",      &x,
                    "y",      &y,
                    "width",  &w,
                    "height", &h,
                    NULL);

      if (lx != (gint) x ||
          ly != (gint) y ||
          lw != (gint) w ||
          lh != (gint) h)
        {
          ligma_canvas_item_begin_change (LIGMA_CANVAS_ITEM (boundary));

          g_object_set (boundary,
                        "x",      (gdouble) lx,
                        "y",      (gdouble) ly,
                        "width",  (gdouble) lw,
                        "height", (gdouble) lh,
                        NULL);

          ligma_canvas_item_end_change (LIGMA_CANVAS_ITEM (boundary));
        }
    }
}
