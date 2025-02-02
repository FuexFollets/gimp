/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacanvaslimit.c
 * Copyright (C) 2020 Ell
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

#include "ligmacanvaslimit.h"
#include "ligmadisplayshell.h"


#define DASH_LENGTH   4.0
#define HIT_DISTANCE 16.0


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_X,
  PROP_Y,
  PROP_RADIUS,
  PROP_ASPECT_RATIO,
  PROP_ANGLE,
  PROP_DASHED
};


typedef struct _LigmaCanvasLimitPrivate LigmaCanvasLimitPrivate;

struct _LigmaCanvasLimitPrivate
{
  LigmaLimitType type;

  gdouble       x;
  gdouble       y;
  gdouble       radius;
  gdouble       aspect_ratio;
  gdouble       angle;

  gboolean      dashed;
};

#define GET_PRIVATE(limit) \
        ((LigmaCanvasLimitPrivate *) ligma_canvas_limit_get_instance_private ((LigmaCanvasLimit *) (limit)))


/*  local function prototypes  */

static void             ligma_canvas_limit_set_property (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void             ligma_canvas_limit_get_property (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);

static void             ligma_canvas_limit_draw         (LigmaCanvasItem *item,
                                                        cairo_t        *cr);
static cairo_region_t * ligma_canvas_limit_get_extents  (LigmaCanvasItem *item);
static gboolean         ligma_canvas_limit_hit          (LigmaCanvasItem *item,
                                                        gdouble         x,
                                                        gdouble         y);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaCanvasLimit, ligma_canvas_limit,
                            LIGMA_TYPE_CANVAS_ITEM)

#define parent_class ligma_canvas_limit_parent_class


/*  private functions  */

static void
ligma_canvas_limit_class_init (LigmaCanvasLimitClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaCanvasItemClass *item_class   = LIGMA_CANVAS_ITEM_CLASS (klass);

  object_class->set_property = ligma_canvas_limit_set_property;
  object_class->get_property = ligma_canvas_limit_get_property;

  item_class->draw           = ligma_canvas_limit_draw;
  item_class->get_extents    = ligma_canvas_limit_get_extents;
  item_class->hit            = ligma_canvas_limit_hit;

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL, NULL,
                                                      LIGMA_TYPE_LIMIT_TYPE,
                                                      LIGMA_LIMIT_CIRCLE,
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_X,
                                   g_param_spec_double ("x", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y,
                                   g_param_spec_double ("y", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_RADIUS,
                                   g_param_spec_double ("radius", NULL, NULL,
                                                        0.0,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ASPECT_RATIO,
                                   g_param_spec_double ("aspect-ratio", NULL, NULL,
                                                        -1.0,
                                                        +1.0,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_ANGLE,
                                   g_param_spec_double ("angle", NULL, NULL,
                                                        -G_MAXDOUBLE,
                                                        +G_MAXDOUBLE,
                                                        0.0,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_DASHED,
                                   g_param_spec_boolean ("dashed", NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
}

static void
ligma_canvas_limit_init (LigmaCanvasLimit *limit)
{
}

static void
ligma_canvas_limit_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaCanvasLimitPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TYPE:
      priv->type = g_value_get_enum (value);
      break;

    case PROP_X:
      priv->x = g_value_get_double (value);
      break;

    case PROP_Y:
      priv->y = g_value_get_double (value);
      break;

    case PROP_RADIUS:
      priv->radius = g_value_get_double (value);
      break;

    case PROP_ASPECT_RATIO:
      priv->aspect_ratio = g_value_get_double (value);
      break;

    case PROP_ANGLE:
      priv->angle = g_value_get_double (value);
      break;

    case PROP_DASHED:
      priv->dashed = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_limit_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaCanvasLimitPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, priv->type);
      break;

    case PROP_X:
      g_value_set_double (value, priv->x);
      break;

    case PROP_Y:
      g_value_set_double (value, priv->y);
      break;

    case PROP_RADIUS:
      g_value_set_double (value, priv->radius);
      break;

    case PROP_ASPECT_RATIO:
      g_value_set_double (value, priv->aspect_ratio);
      break;

    case PROP_ANGLE:
      g_value_set_double (value, priv->angle);
      break;

    case PROP_DASHED:
      g_value_set_boolean (value, priv->dashed);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_canvas_limit_transform (LigmaCanvasItem *item,
                             gdouble        *x,
                             gdouble        *y,
                             gdouble        *rx,
                             gdouble        *ry)
{
  LigmaCanvasLimit        *limit      = LIGMA_CANVAS_LIMIT (item);
  LigmaCanvasLimitPrivate *priv       = GET_PRIVATE (item);
  gdouble                 x1, y1;
  gdouble                 x2, y2;
  gdouble                 min_radius = 0.0;

  ligma_canvas_limit_get_radii (limit, rx, ry);

  ligma_canvas_item_transform_xy_f (item,
                                   priv->x - *rx, priv->y - *ry,
                                   &x1,           &y1);
  ligma_canvas_item_transform_xy_f (item,
                                   priv->x + *rx, priv->y + *ry,
                                   &x2,           &y2);

  x1  = floor (x1) + 0.5;
  y1  = floor (y1) + 0.5;

  x2  = floor (x2) + 0.5;
  y2  = floor (y2) + 0.5;

  *x  = (x1 + x2) / 2.0;
  *y  = (y1 + y2) / 2.0;

  *rx = (x2 - x1) / 2.0;
  *ry = (y2 - y1) / 2.0;

  switch (priv->type)
    {
    case LIGMA_LIMIT_CIRCLE:
    case LIGMA_LIMIT_SQUARE:
      min_radius = 2.0;
      break;

    case LIGMA_LIMIT_DIAMOND:
      min_radius = 3.0;
      break;

    case LIGMA_LIMIT_HORIZONTAL:
    case LIGMA_LIMIT_VERTICAL:
      min_radius = 1.0;
      break;
    }

  *rx = MAX (*rx, min_radius);
  *ry = MAX (*ry, min_radius);
}

static void
ligma_canvas_limit_paint (LigmaCanvasItem *item,
                         cairo_t        *cr)
{
  LigmaCanvasLimitPrivate *priv  = GET_PRIVATE (item);
  LigmaDisplayShell       *shell = ligma_canvas_item_get_shell (item);
  gdouble                 x,  y;
  gdouble                 rx, ry;
  gdouble                 inf;

  ligma_canvas_limit_transform (item,
                               &x,  &y,
                               &rx, &ry);

  cairo_save (cr);

  cairo_translate (cr, x,  y);
  cairo_rotate    (cr, priv->angle);
  cairo_scale     (cr, rx, ry);

  inf = MAX (x, shell->disp_width  - x) +
        MAX (y, shell->disp_height - y);

  switch (priv->type)
    {
    case LIGMA_LIMIT_CIRCLE:
      cairo_arc (cr, 0.0, 0.0, 1.0, 0.0, 2.0 * G_PI);
      break;

    case LIGMA_LIMIT_SQUARE:
      cairo_rectangle (cr, -1.0, -1.0, 2.0, 2.0);
      break;

    case LIGMA_LIMIT_DIAMOND:
      cairo_move_to (cr,  0.0, -1.0);
      cairo_line_to (cr, +1.0,  0.0);
      cairo_line_to (cr,  0.0, +1.0);
      cairo_line_to (cr, -1.0,  0.0);
      cairo_close_path (cr);
      break;

    case LIGMA_LIMIT_HORIZONTAL:
      cairo_move_to (cr, -inf / rx, -1.0);
      cairo_line_to (cr, +inf / rx, -1.0);

      cairo_move_to (cr, -inf / rx, +1.0);
      cairo_line_to (cr, +inf / rx, +1.0);
      break;

    case LIGMA_LIMIT_VERTICAL:
      cairo_move_to (cr, -1.0, -inf / ry);
      cairo_line_to (cr, -1.0, +inf / ry);

      cairo_move_to (cr, +1.0, -inf / ry);
      cairo_line_to (cr, +1.0, +inf / ry);
      break;
    }

  cairo_restore (cr);
}

static void
ligma_canvas_limit_draw (LigmaCanvasItem *item,
                        cairo_t        *cr)
{
  LigmaCanvasLimitPrivate *priv = GET_PRIVATE (item);

  ligma_canvas_limit_paint (item, cr);

  cairo_save (cr);

  if (priv->dashed)
    cairo_set_dash (cr, (const gdouble[]) {DASH_LENGTH}, 1, 0.0);

  _ligma_canvas_item_stroke (item, cr);

  cairo_restore (cr);
}

static cairo_region_t *
ligma_canvas_limit_get_extents (LigmaCanvasItem *item)
{
  cairo_t               *cr;
  cairo_surface_t       *surface;
  cairo_rectangle_int_t  rectangle;
  gdouble                x1, y1;
  gdouble                x2, y2;

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);

  cr = cairo_create (surface);

  ligma_canvas_limit_paint (item, cr);

  cairo_path_extents (cr,
                      &x1, &y1,
                      &x2, &y2);

  cairo_destroy (cr);

  cairo_surface_destroy (surface);

  rectangle.x      = floor (x1 - 1.5);
  rectangle.y      = floor (y1 - 1.5);
  rectangle.width  = ceil  (x2 + 1.5) - rectangle.x;
  rectangle.height = ceil  (y2 + 1.5) - rectangle.y;

  return cairo_region_create_rectangle (&rectangle);
}

static gboolean
ligma_canvas_limit_hit (LigmaCanvasItem *item,
                       gdouble         x,
                       gdouble         y)
{
  LigmaCanvasLimit *limit = LIGMA_CANVAS_LIMIT (item);
  gdouble          bx, by;

  ligma_canvas_limit_boundary_point (limit,
                                    x,   y,
                                    &bx, &by);

  return ligma_canvas_item_transform_distance (item,
                                              x,  y,
                                              bx, by) <= HIT_DISTANCE;
}


/*  public functions  */

LigmaCanvasItem *
ligma_canvas_limit_new (LigmaDisplayShell *shell,
                       LigmaLimitType     type,
                       gdouble           x,
                       gdouble           y,
                       gdouble           radius,
                       gdouble           aspect_ratio,
                       gdouble           angle,
                       gboolean          dashed)
{
  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), NULL);

  return g_object_new (LIGMA_TYPE_CANVAS_LIMIT,
                       "shell",        shell,
                       "type",         type,
                       "x",            x,
                       "y",            y,
                       "radius",       radius,
                       "aspect-ratio", aspect_ratio,
                       "angle",        angle,
                       "dashed",       dashed,
                       NULL);
}

void
ligma_canvas_limit_get_radii (LigmaCanvasLimit  *limit,
                             gdouble          *rx,
                             gdouble          *ry)
{
  LigmaCanvasLimitPrivate *priv;

  g_return_if_fail (LIGMA_IS_CANVAS_LIMIT (limit));

  priv = GET_PRIVATE (limit);

  if (priv->aspect_ratio >= 0.0)
    {
      if (rx) *rx = priv->radius;
      if (ry) *ry = priv->radius * (1.0 - priv->aspect_ratio);
    }
  else
    {
      if (rx) *rx = priv->radius * (1.0 + priv->aspect_ratio);
      if (ry) *ry = priv->radius;
    }
}

gboolean
ligma_canvas_limit_is_inside (LigmaCanvasLimit *limit,
                             gdouble          x,
                             gdouble          y)
{
  LigmaCanvasLimitPrivate *priv;
  LigmaVector2             p;
  gdouble                 rx, ry;

  g_return_val_if_fail (LIGMA_IS_CANVAS_LIMIT (limit), FALSE);

  priv = GET_PRIVATE (limit);

  ligma_canvas_limit_get_radii (limit, &rx, &ry);

  if (rx == 0.0 || ry == 0.0)
    return FALSE;

  p.x = x - priv->x;
  p.y = y - priv->y;

  ligma_vector2_rotate (&p, +priv->angle);

  p.x = fabs (p.x / rx);
  p.y = fabs (p.y / ry);

  switch (priv->type)
    {
    case LIGMA_LIMIT_CIRCLE:
      return ligma_vector2_length (&p) < 1.0;

    case LIGMA_LIMIT_SQUARE:
      return p.x < 1.0 && p.y < 1.0;

    case LIGMA_LIMIT_DIAMOND:
      return p.x + p.y < 1.0;

    case LIGMA_LIMIT_HORIZONTAL:
      return p.y < 1.0;

    case LIGMA_LIMIT_VERTICAL:
      return p.x < 1.0;
    }

  g_return_val_if_reached (FALSE);
}

void
ligma_canvas_limit_boundary_point (LigmaCanvasLimit *limit,
                                  gdouble          x,
                                  gdouble          y,
                                  gdouble         *bx,
                                  gdouble         *by)
{
  LigmaCanvasLimitPrivate *priv;
  LigmaVector2             p;
  gdouble                 rx, ry;
  gboolean                flip_x = FALSE;
  gboolean                flip_y = FALSE;

  g_return_if_fail (LIGMA_IS_CANVAS_LIMIT (limit));
  g_return_if_fail (bx != NULL);
  g_return_if_fail (by != NULL);

  priv = GET_PRIVATE (limit);

  ligma_canvas_limit_get_radii (limit, &rx, &ry);

  p.x = x - priv->x;
  p.y = y - priv->y;

  ligma_vector2_rotate (&p, +priv->angle);

  if (p.x < 0.0)
    {
      p.x = -p.x;

      flip_x = TRUE;
    }

  if (p.y < 0.0)
    {
      p.y = -p.y;

      flip_y = TRUE;
    }

  switch (priv->type)
    {
    case LIGMA_LIMIT_CIRCLE:
      if (rx == ry)
        {
          ligma_vector2_normalize (&p);

          ligma_vector2_mul (&p, rx);
        }
      else
        {
          gdouble a0 = 0.0;
          gdouble a1 = G_PI / 2.0;
          gdouble a;
          gint    i;

          for (i = 0; i < 20; i++)
            {
              LigmaVector2 r;
              LigmaVector2 n;

              a = (a0 + a1) / 2.0;

              r.x = p.x - rx * cos (a);
              r.y = p.y - ry * sin (a);

              n.x = 1.0;
              n.y = tan (a) * rx / ry;

              if (ligma_vector2_cross_product (&r, &n).x >= 0.0)
                a1 = a;
              else
                a0 = a;
            }

          a = (a0 + a1) / 2.0;

          p.x = rx * cos (a);
          p.y = ry * sin (a);
        }
      break;

    case LIGMA_LIMIT_SQUARE:
      if (p.x <= rx || p.y <= ry)
        {
          if (rx - p.x <= ry - p.y)
            p.x = rx;
          else
            p.y = ry;
        }
      else
        {
          p.x = rx;
          p.y = ry;
        }
      break;

    case LIGMA_LIMIT_DIAMOND:
      {
        LigmaVector2 l;
        LigmaVector2 r;
        gdouble     t;

        l.x = rx;
        l.y = -ry;

        r.x = p.x;
        r.y = p.y - ry;

        t = ligma_vector2_inner_product (&r, &l) /
            ligma_vector2_inner_product (&l, &l);
        t = CLAMP (t, 0.0, 1.0);

        p.x = rx * t;
        p.y = ry * (1.0 - t);
      }
      break;

    case LIGMA_LIMIT_HORIZONTAL:
      p.y = ry;
      break;

    case LIGMA_LIMIT_VERTICAL:
      p.x = rx;
      break;
    }

  if (flip_x)
    p.x = -p.x;

  if (flip_y)
    p.y = -p.y;

  ligma_vector2_rotate (&p, -priv->angle);

  *bx = priv->x + p.x;
  *by = priv->y + p.y;
}

gdouble
ligma_canvas_limit_boundary_radius (LigmaCanvasLimit *limit,
                                   gdouble          x,
                                   gdouble          y)
{
  LigmaCanvasLimitPrivate *priv;
  LigmaVector2             p;

  g_return_val_if_fail (LIGMA_IS_CANVAS_LIMIT (limit), 0.0);

  priv = GET_PRIVATE (limit);

  p.x = x - priv->x;
  p.y = y - priv->y;

  ligma_vector2_rotate (&p, +priv->angle);

  p.x = fabs (p.x);
  p.y = fabs (p.y);

  if (priv->aspect_ratio >= 0.0)
    p.y /= 1.0 - priv->aspect_ratio;
  else
    p.x /= 1.0 + priv->aspect_ratio;

  switch (priv->type)
    {
    case LIGMA_LIMIT_CIRCLE:
      return ligma_vector2_length (&p);

    case LIGMA_LIMIT_SQUARE:
      return MAX (p.x, p.y);

    case LIGMA_LIMIT_DIAMOND:
      return p.x + p.y;

    case LIGMA_LIMIT_HORIZONTAL:
      return p.y;

    case LIGMA_LIMIT_VERTICAL:
      return p.x;
    }

  g_return_val_if_reached (0.0);
}

void
ligma_canvas_limit_center_point (LigmaCanvasLimit *limit,
                                gdouble          x,
                                gdouble          y,
                                gdouble         *cx,
                                gdouble         *cy)
{
  LigmaCanvasLimitPrivate *priv;
  LigmaVector2             p;

  g_return_if_fail (LIGMA_IS_CANVAS_LIMIT (limit));
  g_return_if_fail (cx != NULL);
  g_return_if_fail (cy != NULL);

  priv = GET_PRIVATE (limit);

  p.x = x - priv->x;
  p.y = y - priv->y;

  ligma_vector2_rotate (&p, +priv->angle);

  switch (priv->type)
    {
    case LIGMA_LIMIT_CIRCLE:
    case LIGMA_LIMIT_SQUARE:
    case LIGMA_LIMIT_DIAMOND:
      p.x = 0.0;
      p.y = 0.0;
      break;

    case LIGMA_LIMIT_HORIZONTAL:
      p.y = 0.0;
      break;

    case LIGMA_LIMIT_VERTICAL:
      p.x = 0.0;
      break;
    }

  ligma_vector2_rotate (&p, -priv->angle);

  *cx = priv->x + p.x;
  *cy = priv->y + p.y;
}
