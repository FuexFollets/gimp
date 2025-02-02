/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvasarc.c
 * Copyright (C) 2010 Michael Natterer <mitch@ligma.org>
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

#include "core/ligma-cairo.h"

#include "ligmacanvasarc.h"
#include "ligmadisplayshell.h"


enum
{
  PROP_0,
  PROP_CENTER_X,
  PROP_CENTER_Y,
  PROP_RADIUS_X,
  PROP_RADIUS_Y,
  PROP_START_ANGLE,
  PROP_SLICE_ANGLE,
  PROP_FILLED
};


typedef struct _LigmaCanvasArcPrivate LigmaCanvasArcPrivate;

struct _LigmaCanvasArcPrivate
{
  gdouble  center_x;
  gdouble  center_y;
  gdouble  radius_x;
  gdouble  radius_y;
  gdouble  start_angle;
  gdouble  slice_angle;
  gboolean filled;
};

#define GET_PRIVATE(arc) \
        ((LigmaCanvasArcPrivate *) ligma_canvas_arc_get_instance_private ((LigmaCanvasArc *) (arc)))


/*  local function prototypes  */

static void             ligma_canvas_arc_set_property (GObject        *object,
                                                      guint           property_id,
                                                      const GValue   *value,
                                                      GParamSpec     *pspec);
static void             ligma_canvas_arc_get_property (GObject        *object,
                                                      guint           property_id,
                                                      GValue         *value,
                                                      GParamSpec     *pspec);
static void             ligma_canvas_arc_draw         (LigmaCanvasItem *item,
                                                      cairo_t        *cr);
static cairo_region_t * ligma_canvas_arc_get_extents  (LigmaCanvasItem *item);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasArc, ligma_canvas_arc,
                            LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_arc_parent_class


static void
ligma_canvas_arc_class_init (LigmaCanvasArcClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = ligma_canvas_arc_set_property;
  object_class->get_property = ligma_canvas_arc_get_property;

  item_class->draw           = ligma_canvas_arc_draw;
  item_class->get_extents    = ligma_canvas_arc_get_extents;

  g_object_class_install_property (object_class, PROP_CENTER_X,
                                   g_param_spec_double ("center-x", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CENTER_Y,
                                   g_param_spec_double ("center-y", NULL, NULL,
                                                        -LIGMA_MAX_IMAGE_SIZE,
                                                        LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_RADIUS_X,
                                   g_param_spec_double ("radius-x", NULL, NULL,
                                                        0, LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_RADIUS_Y,
                                   g_param_spec_double ("radius-y", NULL, NULL,
                                                        0, LIGMA_MAX_IMAGE_SIZE, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_START_ANGLE,
                                   g_param_spec_double ("start-angle", NULL, NULL,
                                                        -1000, 1000, 0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SLICE_ANGLE,
                                   g_param_spec_double ("slice-angle", NULL, NULL,
                                                        -1000, 1000, 2 * G_PI,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FILLED,
                                   g_param_spec_boolean ("filled", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_arc_init (LigmaCanvasArc *arc)
{
}

static void
ligma_canvas_arc_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaCanvasArcPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CENTER_X:
      private->center_x = g_value_get_double (value);
      break;
    case PROP_CENTER_Y:
      private->center_y = g_value_get_double (value);
      break;
    case PROP_RADIUS_X:
      private->radius_x = g_value_get_double (value);
      break;
    case PROP_RADIUS_Y:
      private->radius_y = g_value_get_double (value);
      break;
    case PROP_START_ANGLE:
      private->start_angle = g_value_get_double (value);
      break;
    case PROP_SLICE_ANGLE:
      private->slice_angle = g_value_get_double (value);
      break;
    case PROP_FILLED:
      private->filled = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_arc_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaCanvasArcPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CENTER_X:
      g_value_set_double (value, private->center_x);
      break;
    case PROP_CENTER_Y:
      g_value_set_double (value, private->center_y);
      break;
    case PROP_RADIUS_X:
      g_value_set_double (value, private->radius_x);
      break;
    case PROP_RADIUS_Y:
      g_value_set_double (value, private->radius_y);
      break;
    case PROP_START_ANGLE:
      g_value_set_double (value, private->start_angle);
      break;
    case PROP_SLICE_ANGLE:
      g_value_set_double (value, private->slice_angle);
      break;
    case PROP_FILLED:
      g_value_set_boolean (value, private->filled);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_arc_transform (LigmaCanvasItem *item,
                           gdouble        *center_x,
                           gdouble        *center_y,
                           gdouble        *radius_x,
                           gdouble        *radius_y)
{
  LigmaCanvasArcPrivate *private = GET_PRIVATE (item);
  gdouble               x1, y1;
  gdouble               x2, y2;

  ligma_canvas_item_transform_xy_f (item,
                                   private->center_x - private->radius_x,
                                   private->center_y - private->radius_y,
                                   &x1, &y1);
  ligma_canvas_item_transform_xy_f (item,
                                   private->center_x + private->radius_x,
                                   private->center_y + private->radius_y,
                                   &x2, &y2);

  x1 = floor (x1);
  y1 = floor (y1);
  x2 = ceil (x2);
  y2 = ceil (y2);

  *center_x = (x1 + x2) / 2.0;
  *center_y = (y1 + y2) / 2.0;

  *radius_x = (x2 - x1) / 2.0;
  *radius_y = (y2 - y1) / 2.0;

  if (! private->filled)
    {
      *radius_x = MAX (*radius_x - 0.5, 0.0);
      *radius_y = MAX (*radius_y - 0.5, 0.0);
    }

  /* avoid cairo_scale (cr, 0.0, 0.0) */
  if (*radius_x == 0.0) *radius_x = 0.000001;
  if (*radius_y == 0.0) *radius_y = 0.000001;
}

static void
ligma_canvas_arc_draw (LigmaCanvasItem *item,
                      cairo_t        *cr)
{
  LigmaCanvasArcPrivate *private = GET_PRIVATE (item);
  gdouble               center_x, center_y;
  gdouble               radius_x, radius_y;

  ligma_canvas_arc_transform (item,
                             &center_x, &center_y,
                             &radius_x, &radius_y);

  cairo_save (cr);
  cairo_translate (cr, center_x, center_y);
  cairo_scale (cr, radius_x, radius_y);
  ligma_cairo_arc (cr, 0.0, 0.0, 1.0,
                  private->start_angle, private->slice_angle);
  cairo_restore (cr);

  if (private->filled)
    _ligma_canvas_item_fill (item, cr);
  else
    _ligma_canvas_item_stroke (item, cr);
}

static cairo_region_t *
ligma_canvas_arc_get_extents (LigmaCanvasItem *item)
{
  LigmaCanvasArcPrivate  *private = GET_PRIVATE (item);
  cairo_region_t        *region;
  cairo_rectangle_int_t  rectangle;
  gdouble                center_x, center_y;
  gdouble                radius_x, radius_y;

  ligma_canvas_arc_transform (item,
                             &center_x, &center_y,
                             &radius_x, &radius_y);

  rectangle.x      = floor (center_x - radius_x - 1.5);
  rectangle.y      = floor (center_y - radius_y - 1.5);
  rectangle.width  = ceil (center_x + radius_x + 1.5) - rectangle.x;
  rectangle.height = ceil (center_y + radius_y + 1.5) - rectangle.y;

  region = cairo_region_create_rectangle (&rectangle);

  if (! private->filled &&
      rectangle.width > 64 * 1.43 &&
      rectangle.height > 64 * 1.43)
    {
      radius_x *= 0.7;
      radius_y *= 0.7;

      rectangle.x      = ceil (center_x - radius_x + 1.5);
      rectangle.y      = ceil (center_y - radius_y + 1.5);
      rectangle.width  = floor (center_x + radius_x - 1.5) - rectangle.x;
      rectangle.height = floor (center_y + radius_y - 1.5) - rectangle.y;

      cairo_region_subtract_rectangle (region, &rectangle);
    }

  return region;
}

LigmaCanvasItem *
ligma_canvas_arc_new (LigmaDisplayShell *shell,
                     gdouble          center_x,
                     gdouble          center_y,
                     gdouble          radius_x,
                     gdouble          radius_y,
                     gdouble          start_angle,
                     gdouble          slice_angle,
                     gboolean         filled)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_ARC,
                       "shell",       shell,
                       "center-x",    center_x,
                       "center-y",    center_y,
                       "radius-x",    radius_x,
                       "radius-y",    radius_y,
                       "start-angle", start_angle,
                       "slice-angle", slice_angle,
                       "filled",      filled,
                       NULL);
}

void
ligma_canvas_arc_set (LigmaCanvasItem *arc,
                     gdouble         center_x,
                     gdouble         center_y,
                     gdouble         radius_x,
                     gdouble         radius_y,
                     gdouble         start_angle,
                     gdouble         slice_angle)
{
  g_return_if_fail (LIGMA_IS_CANVAS_ARC (arc));

  ligma_canvas_item_begin_change (arc);
  g_object_set (arc,
                "center-x",    center_x,
                "center-y",    center_y,
                "radius-x",    radius_x,
                "radius-y",    radius_y,
                "start-angle", start_angle,
                "slice-angle", slice_angle,
                NULL);
  ligma_canvas_item_end_change (arc);
}
