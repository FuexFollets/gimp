/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include <string.h>

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "ligmawidgetstypes.h"

#include "ligmaruler.h"


/**
 * SECTION: ligmaruler
 * @title: LigmaRuler
 * @short_description: A ruler widget with configurable unit and orientation.
 *
 * A ruler widget with configurable unit and orientation.
 **/


#define MINIMUM_INCR                5
#define IMMEDIATE_REDRAW_THRESHOLD  20


enum
{
  PROP_0,
  PROP_ORIENTATION,
  PROP_UNIT,
  PROP_LOWER,
  PROP_UPPER,
  PROP_POSITION,
  PROP_MAX_SIZE,
  N_PROPS
};
static GParamSpec *object_props[N_PROPS] = { NULL, };


/* All distances below are in 1/72nd's of an inch. (According to
 * Adobe that's a point, but points are really 1/72.27 in.)
 */
struct _LigmaRulerPrivate
{
  GtkOrientation   orientation;
  LigmaUnit         unit;
  gdouble          lower;
  gdouble          upper;
  gdouble          position;
  gdouble          max_size;

  GdkWindow       *input_window;
  cairo_surface_t *backing_store;
  gboolean         backing_store_valid;
  GdkRectangle     last_pos_rect;
  guint            pos_redraw_idle_id;
  PangoLayout     *layout;

  GList           *track_widgets;
};

#define GET_PRIVATE(obj) (((LigmaRuler *) (obj))->priv)


typedef struct
{
  const gdouble  ruler_scale[16];
  const gint     subdivide[5];
} RulerMetric;

static const RulerMetric ruler_metric_decimal =
{
  { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },
  { 1, 5, 10, 50, 100 }
};

static const RulerMetric ruler_metric_inches =
{
  /* 12 inch = 1 foot; 36 inch = 1 yard; 72 inchs = 1 fathom */
  { 1, 2, 6, 12, 36, 72, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },

  /* Inches are divided by multiples of 2. */
  { 1, 2, 4, 8, 16 }
};

static const RulerMetric ruler_metric_feet =
{
  /* 3 feet = 1 yard; 6 feet = 1 fathom */
  { 1, 3, 6, 12, 36, 72, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },

  /* 1 foot = 12 inches, so let's divide up to 12, */
  { 1, 3, 6, 12,
  /* then divide the inch by 2. */
    24 }
};

static const RulerMetric ruler_metric_yards =
{
  /* 1 fathom = 2 yards. Should we go back to base-10 digits? */
  { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },

  /* 1 yard = 3 feet = 36 inches. */
  { 1, 3, 6, 12, 36 }
};


static void          ligma_ruler_dispose               (GObject        *object);
static void          ligma_ruler_set_property          (GObject        *object,
                                                       guint           prop_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);
static void          ligma_ruler_get_property          (GObject        *object,
                                                       guint           prop_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);

static void          ligma_ruler_realize               (GtkWidget      *widget);
static void          ligma_ruler_unrealize             (GtkWidget      *widget);
static void          ligma_ruler_map                   (GtkWidget      *widget);
static void          ligma_ruler_unmap                 (GtkWidget      *widget);
static void          ligma_ruler_size_allocate         (GtkWidget      *widget,
                                                       GtkAllocation  *allocation);
static void          ligma_ruler_get_preferred_width   (GtkWidget      *widget,
                                                       gint           *minimum_width,
                                                       gint           *natural_width);
static void          ligma_ruler_get_preferred_height  (GtkWidget      *widget,
                                                       gint           *minimum_height,
                                                       gint           *natural_height);
static void          ligma_ruler_style_updated         (GtkWidget      *widget);
static gboolean      ligma_ruler_motion_notify         (GtkWidget      *widget,
                                                       GdkEventMotion *event);
static gboolean      ligma_ruler_draw                  (GtkWidget      *widget,
                                                       cairo_t        *cr);

static void          ligma_ruler_draw_ticks            (LigmaRuler      *ruler);
static GdkRectangle  ligma_ruler_get_pos_rect          (LigmaRuler      *ruler,
                                                       gdouble         position);
static gboolean      ligma_ruler_idle_queue_pos_redraw (gpointer        data);
static void          ligma_ruler_queue_pos_redraw      (LigmaRuler      *ruler);
static void          ligma_ruler_draw_pos              (LigmaRuler      *ruler,
                                                       cairo_t        *cr);
static void          ligma_ruler_make_pixmap           (LigmaRuler      *ruler);
static PangoLayout * ligma_ruler_get_layout            (GtkWidget      *widget,
                                                       const gchar    *text);
static const RulerMetric *
                     ligma_ruler_get_metric            (LigmaUnit        unit);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaRuler, ligma_ruler, GTK_TYPE_WIDGET)

#define parent_class ligma_ruler_parent_class


static void
ligma_ruler_class_init (LigmaRulerClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose              = ligma_ruler_dispose;
  object_class->set_property         = ligma_ruler_set_property;
  object_class->get_property         = ligma_ruler_get_property;

  widget_class->realize              = ligma_ruler_realize;
  widget_class->unrealize            = ligma_ruler_unrealize;
  widget_class->map                  = ligma_ruler_map;
  widget_class->unmap                = ligma_ruler_unmap;
  widget_class->get_preferred_width  = ligma_ruler_get_preferred_width;
  widget_class->get_preferred_height = ligma_ruler_get_preferred_height;
  widget_class->size_allocate        = ligma_ruler_size_allocate;
  widget_class->style_updated        = ligma_ruler_style_updated;
  widget_class->motion_notify_event  = ligma_ruler_motion_notify;
  widget_class->draw                 = ligma_ruler_draw;

  object_props[PROP_ORIENTATION] = g_param_spec_enum ("orientation",
                                                      "Orientation",
                                                      "The orientation of the ruler",
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      LIGMA_PARAM_READWRITE);

  object_props[PROP_UNIT] = ligma_param_spec_unit ("unit",
                                                  "Unit",
                                                  "Unit of ruler",
                                                  TRUE, TRUE,
                                                  LIGMA_UNIT_PIXEL,
                                                  LIGMA_PARAM_READWRITE);

  object_props[PROP_LOWER] = g_param_spec_double ("lower",
                                                  "Lower",
                                                  "Lower limit of ruler",
                                                  -G_MAXDOUBLE,
                                                  G_MAXDOUBLE,
                                                  0.0,
                                                  LIGMA_PARAM_READWRITE);

  object_props[PROP_UPPER] = g_param_spec_double ("upper",
                                                  "Upper",
                                                  "Upper limit of ruler",
                                                  -G_MAXDOUBLE,
                                                  G_MAXDOUBLE,
                                                  0.0,
                                                  LIGMA_PARAM_READWRITE);

  object_props[PROP_POSITION] = g_param_spec_double ("position",
                                                     "Position",
                                                     "Position of mark on the ruler",
                                                     -G_MAXDOUBLE,
                                                     G_MAXDOUBLE,
                                                     0.0,
                                                     LIGMA_PARAM_READWRITE);

  object_props[PROP_MAX_SIZE] = g_param_spec_double ("max-size",
                                                     "Max Size",
                                                     "Maximum size of the ruler",
                                                     -G_MAXDOUBLE,
                                                     G_MAXDOUBLE,
                                                     0.0,
                                                     LIGMA_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, object_props);

  gtk_widget_class_set_css_name (widget_class, "LigmaRuler");
}

static void
ligma_ruler_init (LigmaRuler *ruler)
{
  LigmaRulerPrivate *priv;

  ruler->priv = ligma_ruler_get_instance_private (ruler);

  priv = ruler->priv;

  gtk_widget_set_has_window (GTK_WIDGET (ruler), FALSE);

  priv->orientation = GTK_ORIENTATION_HORIZONTAL;
  priv->unit        = LIGMA_UNIT_PIXEL;
}

static void
ligma_ruler_dispose (GObject *object)
{
  LigmaRuler        *ruler = LIGMA_RULER (object);
  LigmaRulerPrivate *priv  = GET_PRIVATE (ruler);

  while (priv->track_widgets)
    ligma_ruler_remove_track_widget (ruler, priv->track_widgets->data);

  if (priv->pos_redraw_idle_id)
    {
      g_source_remove (priv->pos_redraw_idle_id);
      priv->pos_redraw_idle_id = 0;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_ruler_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  LigmaRuler        *ruler = LIGMA_RULER (object);
  LigmaRulerPrivate *priv  = GET_PRIVATE (ruler);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      priv->orientation = g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (ruler));
      break;

    case PROP_UNIT:
      ligma_ruler_set_unit (ruler, g_value_get_int (value));
      break;

    case PROP_LOWER:
      ligma_ruler_set_range (ruler,
                            g_value_get_double (value),
                            priv->upper,
                            priv->max_size);
      break;
    case PROP_UPPER:
      ligma_ruler_set_range (ruler,
                            priv->lower,
                            g_value_get_double (value),
                            priv->max_size);
      break;

    case PROP_POSITION:
      ligma_ruler_set_position (ruler, g_value_get_double (value));
      break;

    case PROP_MAX_SIZE:
      ligma_ruler_set_range (ruler,
                            priv->lower,
                            priv->upper,
                            g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
ligma_ruler_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  LigmaRuler        *ruler = LIGMA_RULER (object);
  LigmaRulerPrivate *priv  = GET_PRIVATE (ruler);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;

    case PROP_UNIT:
      g_value_set_int (value, priv->unit);
      break;

    case PROP_LOWER:
      g_value_set_double (value, priv->lower);
      break;

    case PROP_UPPER:
      g_value_set_double (value, priv->upper);
      break;

    case PROP_POSITION:
      g_value_set_double (value, priv->position);
      break;

    case PROP_MAX_SIZE:
      g_value_set_double (value, priv->max_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * ligma_ruler_new:
 * @orientation: the ruler's orientation.
 *
 * Creates a new ruler.
 *
 * Returns: a new #LigmaRuler widget.
 *
 * Since: 2.8
 **/
GtkWidget *
ligma_ruler_new (GtkOrientation orientation)
{
  return g_object_new (LIGMA_TYPE_RULER,
                       "orientation", orientation,
                       NULL);
}

static void
ligma_ruler_update_position (LigmaRuler *ruler,
                            gdouble    x,
                            gdouble    y)
{
  LigmaRulerPrivate *priv = GET_PRIVATE (ruler);
  GtkAllocation     allocation;
  gdouble           lower;
  gdouble           upper;

  gtk_widget_get_allocation (GTK_WIDGET (ruler), &allocation);
  ligma_ruler_get_range (ruler, &lower, &upper, NULL);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      ligma_ruler_set_position (ruler,
                               lower +
                               (upper - lower) * x / allocation.width);
    }
  else
    {
      ligma_ruler_set_position (ruler,
                               lower +
                               (upper - lower) * y / allocation.height);
    }
}

/* Returns TRUE if a translation should be done */
static gboolean
gtk_widget_get_translation_to_window (GtkWidget *widget,
                                      GdkWindow *window,
                                      int       *x,
                                      int       *y)
{
  GdkWindow *w, *widget_window;

  if (! gtk_widget_get_has_window (widget))
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (widget, &allocation);

      *x = -allocation.x;
      *y = -allocation.y;
    }
  else
    {
      *x = 0;
      *y = 0;
    }

  widget_window = gtk_widget_get_window (widget);

  for (w = window;
       w && w != widget_window;
       w = gdk_window_get_effective_parent (w))
    {
      gdouble px, py;

      gdk_window_coords_to_parent (w, *x, *y, &px, &py);

      *x += px;
      *y += py;
    }

  if (w == NULL)
    {
      *x = 0;
      *y = 0;
      return FALSE;
    }

  return TRUE;
}

static void
ligma_ruler_event_to_widget_coords (GtkWidget *widget,
                                   GdkWindow *window,
                                   gdouble    event_x,
                                   gdouble    event_y,
                                   gint      *widget_x,
                                   gint      *widget_y)
{
  gint tx, ty;

  if (gtk_widget_get_translation_to_window (widget, window, &tx, &ty))
    {
      event_x += tx;
      event_y += ty;
    }

  *widget_x = event_x;
  *widget_y = event_y;
}

static gboolean
ligma_ruler_track_widget_motion_notify (GtkWidget      *widget,
                                       GdkEventMotion *mevent,
                                       LigmaRuler      *ruler)
{
  gint widget_x;
  gint widget_y;
  gint ruler_x;
  gint ruler_y;

  widget = gtk_get_event_widget ((GdkEvent *) mevent);

  ligma_ruler_event_to_widget_coords (widget, mevent->window,
                                     mevent->x, mevent->y,
                                     &widget_x, &widget_y);

  if (gtk_widget_translate_coordinates (widget, GTK_WIDGET (ruler),
                                        widget_x, widget_y,
                                        &ruler_x, &ruler_y))
    {
      ligma_ruler_update_position (ruler, ruler_x, ruler_y);
    }

  return FALSE;
}

/**
 * ligma_ruler_add_track_widget:
 * @ruler: a #LigmaRuler
 * @widget: the track widget to add
 *
 * Adds a "track widget" to the ruler. The ruler will connect to
 * GtkWidget:motion-notify-event: on the track widget and update its
 * position marker accordingly. The marker is correctly updated also
 * for the track widget's children, regardless of whether they are
 * ordinary children of off-screen children.
 *
 * Since: 2.8
 */
void
ligma_ruler_add_track_widget (LigmaRuler *ruler,
                             GtkWidget *widget)
{
  LigmaRulerPrivate *priv;

  g_return_if_fail (LIGMA_IS_RULER (ruler));
  g_return_if_fail (GTK_IS_WIDGET (ruler));

  priv = GET_PRIVATE (ruler);

  g_return_if_fail (g_list_find (priv->track_widgets, widget) == NULL);

  priv->track_widgets = g_list_prepend (priv->track_widgets, widget);

  g_signal_connect (widget, "motion-notify-event",
                    G_CALLBACK (ligma_ruler_track_widget_motion_notify),
                    ruler);
  g_signal_connect_swapped (widget, "destroy",
                            G_CALLBACK (ligma_ruler_remove_track_widget),
                            ruler);
}

/**
 * ligma_ruler_remove_track_widget:
 * @ruler: a #LigmaRuler
 * @widget: the track widget to remove
 *
 * Removes a previously added track widget from the ruler. See
 * ligma_ruler_add_track_widget().
 *
 * Since: 2.8
 */
void
ligma_ruler_remove_track_widget (LigmaRuler *ruler,
                                GtkWidget *widget)
{
  LigmaRulerPrivate *priv;

  g_return_if_fail (LIGMA_IS_RULER (ruler));
  g_return_if_fail (GTK_IS_WIDGET (ruler));

  priv = GET_PRIVATE (ruler);

  g_return_if_fail (g_list_find (priv->track_widgets, widget) != NULL);

  priv->track_widgets = g_list_remove (priv->track_widgets, widget);

  g_signal_handlers_disconnect_by_func (widget,
                                        ligma_ruler_track_widget_motion_notify,
                                        ruler);
  g_signal_handlers_disconnect_by_func (widget,
                                        ligma_ruler_remove_track_widget,
                                        ruler);
}

/**
 * ligma_ruler_set_unit:
 * @ruler: a #LigmaRuler
 * @unit:  the #LigmaUnit to set the ruler to
 *
 * This sets the unit of the ruler.
 *
 * Since: 2.8
 */
void
ligma_ruler_set_unit (LigmaRuler *ruler,
                     LigmaUnit   unit)
{
  LigmaRulerPrivate *priv;

  g_return_if_fail (LIGMA_IS_RULER (ruler));

  priv = GET_PRIVATE (ruler);

  if (priv->unit != unit)
    {
      priv->unit = unit;
      g_object_notify_by_pspec (G_OBJECT (ruler), object_props[PROP_UNIT]);

      priv->backing_store_valid = FALSE;
      gtk_widget_queue_draw (GTK_WIDGET (ruler));
    }
}

/**
 * ligma_ruler_get_unit:
 * @ruler: a #LigmaRuler
 *
 * Returns: (transfer none): the unit currently used in the @ruler widget.
 *
 * Since: 2.8
 **/
LigmaUnit
ligma_ruler_get_unit (LigmaRuler *ruler)
{
  g_return_val_if_fail (LIGMA_IS_RULER (ruler), 0);

  return GET_PRIVATE (ruler)->unit;
}

/**
 * ligma_ruler_set_position:
 * @ruler: a #LigmaRuler
 * @position: the position to set the ruler to
 *
 * This sets the position of the ruler.
 *
 * Since: 2.8
 */
void
ligma_ruler_set_position (LigmaRuler *ruler,
                         gdouble    position)
{
  LigmaRulerPrivate *priv;

  g_return_if_fail (LIGMA_IS_RULER (ruler));

  priv = GET_PRIVATE (ruler);

  if (priv->position != position)
    {
      GdkRectangle rect;
      gint xdiff, ydiff;

      priv->position = position;
      g_object_notify_by_pspec (G_OBJECT (ruler), object_props[PROP_POSITION]);

      rect = ligma_ruler_get_pos_rect (ruler, priv->position);

      xdiff = rect.x - priv->last_pos_rect.x;
      ydiff = rect.y - priv->last_pos_rect.y;

#ifndef GDK_WINDOWING_QUARTZ
      /*
       * If the position has changed far enough, queue a redraw immediately.
       * Otherwise, we only queue a redraw in a low priority idle handler, to
       * allow for other things (like updating the canvas) to run.
       *
       * TODO: This might not be necessary any more in GTK3 with the frame
       *       clock. Investigate this more after the port to GTK3.
       */
      if (priv->last_pos_rect.width  != 0 &&
          priv->last_pos_rect.height != 0 &&
          (ABS (xdiff) > IMMEDIATE_REDRAW_THRESHOLD ||
           ABS (ydiff) > IMMEDIATE_REDRAW_THRESHOLD))
        {
          if (priv->pos_redraw_idle_id)
            {
              g_source_remove (priv->pos_redraw_idle_id);
              priv->pos_redraw_idle_id = 0;
            }

          ligma_ruler_queue_pos_redraw (ruler);
        }
      else if (! priv->pos_redraw_idle_id)
#else
      /*
       * pos_redraw_idle_id being set can be counted on to mean
       * a redraw is needed (on mac only) since we will not do
       * a high priority draws due to the dramatic performance
       * they will have.
       */
      if (! priv->pos_redraw_idle_id)
#endif
        {
          priv->pos_redraw_idle_id =
            g_idle_add_full (G_PRIORITY_LOW,
                             ligma_ruler_idle_queue_pos_redraw,
                             ruler, NULL);
        }
    }
}

/**
 * ligma_ruler_get_position:
 * @ruler: a #LigmaRuler
 *
 * Returns: the current position of the @ruler widget.
 *
 * Since: 2.8
 **/
gdouble
ligma_ruler_get_position (LigmaRuler *ruler)
{
  g_return_val_if_fail (LIGMA_IS_RULER (ruler), 0.0);

  return GET_PRIVATE (ruler)->position;
}

/**
 * ligma_ruler_set_range:
 * @ruler: a #LigmaRuler
 * @lower: the lower limit of the ruler
 * @upper: the upper limit of the ruler
 * @max_size: the maximum size of the ruler used when calculating the space to
 * leave for the text
 *
 * This sets the range of the ruler.
 *
 * Since: 2.8
 */
void
ligma_ruler_set_range (LigmaRuler *ruler,
                      gdouble    lower,
                      gdouble    upper,
                      gdouble    max_size)
{
  LigmaRulerPrivate *priv;

  g_return_if_fail (LIGMA_IS_RULER (ruler));

  priv = GET_PRIVATE (ruler);

  g_object_freeze_notify (G_OBJECT (ruler));
  if (priv->lower != lower)
    {
      priv->lower = lower;
      g_object_notify_by_pspec (G_OBJECT (ruler), object_props[PROP_LOWER]);
    }
  if (priv->upper != upper)
    {
      priv->upper = upper;
      g_object_notify_by_pspec (G_OBJECT (ruler), object_props[PROP_UPPER]);
    }
  if (priv->max_size != max_size)
    {
      priv->max_size = max_size;
      g_object_notify_by_pspec (G_OBJECT (ruler), object_props[PROP_MAX_SIZE]);
    }
  g_object_thaw_notify (G_OBJECT (ruler));

  priv->backing_store_valid = FALSE;
  gtk_widget_queue_draw (GTK_WIDGET (ruler));
}

/**
 * ligma_ruler_get_range:
 * @ruler: a #LigmaRuler
 * @lower: (out) (optional): location to store lower limit of the ruler,
 *                           or %NULL
 * @upper: (out) (optional): location to store upper limit of the ruler,
 *                           or %NULL
 * @max_size: (out) (optional): location to store the maximum size of the ruler
 *                              used when calculating the space to leave for
 *                              the text, or %NULL.
 *
 * Retrieves values indicating the range and current position of a #LigmaRuler.
 * See ligma_ruler_set_range().
 *
 * Since: 2.8
 **/
void
ligma_ruler_get_range (LigmaRuler *ruler,
                      gdouble   *lower,
                      gdouble   *upper,
                      gdouble   *max_size)
{
  LigmaRulerPrivate *priv;

  g_return_if_fail (LIGMA_IS_RULER (ruler));

  priv = GET_PRIVATE (ruler);

  if (lower)
    *lower = priv->lower;
  if (upper)
    *upper = priv->upper;
  if (max_size)
    *max_size = priv->max_size;
}

static void
ligma_ruler_realize (GtkWidget *widget)
{
  LigmaRuler        *ruler = LIGMA_RULER (widget);
  LigmaRulerPrivate *priv  = GET_PRIVATE (ruler);
  GtkAllocation     allocation;
  GdkWindowAttr     attributes;
  gint              attributes_mask;

  GTK_WIDGET_CLASS (ligma_ruler_parent_class)->realize (widget);

  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x           = allocation.x;
  attributes.y           = allocation.y;
  attributes.width       = allocation.width;
  attributes.height      = allocation.height;
  attributes.wclass      = GDK_INPUT_ONLY;
  attributes.event_mask  = (gtk_widget_get_events (widget) |
                            GDK_EXPOSURE_MASK              |
                            GDK_POINTER_MOTION_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  priv->input_window = gdk_window_new (gtk_widget_get_window (widget),
                                       &attributes, attributes_mask);
  gdk_window_set_user_data (priv->input_window, ruler);

  ligma_ruler_make_pixmap (ruler);
}

static void
ligma_ruler_unrealize (GtkWidget *widget)
{
  LigmaRuler        *ruler = LIGMA_RULER (widget);
  LigmaRulerPrivate *priv  = GET_PRIVATE (ruler);

  g_clear_pointer (&priv->backing_store, cairo_surface_destroy);
  priv->backing_store_valid = FALSE;

  g_clear_object (&priv->layout);

  g_clear_pointer (&priv->input_window, gdk_window_destroy);

  GTK_WIDGET_CLASS (ligma_ruler_parent_class)->unrealize (widget);
}

static void
ligma_ruler_map (GtkWidget *widget)
{
  LigmaRulerPrivate *priv = GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (parent_class)->map (widget);

  if (priv->input_window)
    gdk_window_show (priv->input_window);
}

static void
ligma_ruler_unmap (GtkWidget *widget)
{
  LigmaRulerPrivate *priv = GET_PRIVATE (widget);

  if (priv->input_window)
    gdk_window_hide (priv->input_window);

  GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void
ligma_ruler_size_allocate (GtkWidget     *widget,
                          GtkAllocation *allocation)
{
  LigmaRuler        *ruler = LIGMA_RULER (widget);
  LigmaRulerPrivate *priv  = GET_PRIVATE (ruler);
  GtkAllocation     widget_allocation;
  gboolean          resized;

  gtk_widget_get_allocation (widget, &widget_allocation);

  resized = (widget_allocation.width  != allocation->width ||
             widget_allocation.height != allocation->height);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (gtk_widget_get_realized (widget))
    {
      gdk_window_move_resize (priv->input_window,
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);

      if (resized)
        ligma_ruler_make_pixmap (ruler);
    }
}

static void
ligma_ruler_size_request (GtkWidget      *widget,
                         GtkRequisition *requisition)
{
  LigmaRulerPrivate *priv    = GET_PRIVATE (widget);
  GtkStyleContext  *context = gtk_widget_get_style_context (widget);
  PangoLayout      *layout;
  PangoRectangle    ink_rect;
  GtkBorder         border;
  gint              size;

  layout = ligma_ruler_get_layout (widget, "0123456789");
  pango_layout_get_pixel_extents (layout, &ink_rect, NULL);

  size = 2 + ink_rect.height * 1.7;

  gtk_style_context_get_border (context, gtk_widget_get_state_flags (widget), &border);

  requisition->width  = border.left + border.right;
  requisition->height = border.top + border.bottom;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      requisition->width  += 1;
      requisition->height += size;
    }
  else
    {
      requisition->width  += size;
      requisition->height += 1;
    }
}

static void
ligma_ruler_get_preferred_width (GtkWidget *widget,
                                gint      *minimum_width,
                                gint      *natural_width)
{
  GtkRequisition requisition;

  ligma_ruler_size_request (widget, &requisition);

  *minimum_width = *natural_width = requisition.width;
}

static void
ligma_ruler_get_preferred_height (GtkWidget *widget,
                                 gint      *minimum_height,
                                 gint      *natural_height)
{
  GtkRequisition requisition;

  ligma_ruler_size_request (widget, &requisition);

  *minimum_height = *natural_height = requisition.height;
}

static void
ligma_ruler_style_updated (GtkWidget *widget)
{
  LigmaRulerPrivate *priv = GET_PRIVATE (widget);

  GTK_WIDGET_CLASS (ligma_ruler_parent_class)->style_updated (widget);

  priv->backing_store_valid = FALSE;

  g_clear_object (&priv->layout);
}

static gboolean
ligma_ruler_motion_notify (GtkWidget      *widget,
                          GdkEventMotion *event)
{
  LigmaRuler *ruler = LIGMA_RULER (widget);

  ligma_ruler_update_position (ruler, event->x, event->y);

  return FALSE;
}

static gboolean
ligma_ruler_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
  LigmaRuler        *ruler   = LIGMA_RULER (widget);
  LigmaRulerPrivate *priv    = GET_PRIVATE (ruler);
  GtkStyleContext  *context = gtk_widget_get_style_context (widget);
  GtkAllocation     allocation;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_render_background (context, cr, 0, 0, allocation.width, allocation.height);
  gtk_render_frame (context, cr, 0, 0, allocation.width, allocation.height);

  if (! priv->backing_store_valid)
    ligma_ruler_draw_ticks (ruler);

  cairo_set_source_surface (cr, priv->backing_store, 0, 0);
  cairo_paint (cr);

  ligma_ruler_draw_pos (ruler, cr);

  return FALSE;
}

static void
ligma_ruler_draw_ticks (LigmaRuler *ruler)
{
  GtkWidget         *widget  = GTK_WIDGET (ruler);
  GtkStyleContext   *context = gtk_widget_get_style_context (widget);
  LigmaRulerPrivate  *priv    = GET_PRIVATE (ruler);
  GtkAllocation      allocation;
  GtkBorder          border;
  GdkRGBA            color;
  cairo_t           *cr;
  gint               i;
  gint               width, height;
  gint               length, ideal_length;
  gdouble            lower, upper;  /* Upper and lower limits, in ruler units */
  gdouble            increment;     /* Number of pixels per unit */
  gint               scale;         /* Number of units per major unit */
  gdouble            start, end, cur;
  gchar              unit_str[32];
  gint               digit_height;
  gint               digit_offset;
  gint               text_size;
  gint               pos;
  gdouble            max_size;
  LigmaUnit           unit;
  PangoLayout       *layout;
  PangoRectangle     logical_rect, ink_rect;
  const RulerMetric *ruler_metric;

  if (! gtk_widget_is_drawable (widget))
    return;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_style_context_get_border (context, gtk_widget_get_state_flags (widget), &border);

  layout = ligma_ruler_get_layout (widget, "0123456789");
  pango_layout_get_extents (layout, &ink_rect, &logical_rect);

  digit_height = PANGO_PIXELS (ink_rect.height) + 2;
  digit_offset = ink_rect.y;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      width  = allocation.width;
      height = allocation.height - (border.top + border.bottom);
    }
  else
    {
      width  = allocation.height;
      height = allocation.width - (border.top + border.bottom);
    }

  cr = cairo_create (priv->backing_store);

  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

  gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget),
                               &color);
  gdk_cairo_set_source_rgba (cr, &color);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      cairo_rectangle (cr,
                       border.left,
                       height + border.top,
                       allocation.width - (border.left + border.right),
                       1);
    }
  else
    {
      cairo_rectangle (cr,
                       height + border.left,
                       border.top,
                       1,
                       allocation.height - (border.top + border.bottom));
    }

  ligma_ruler_get_range (ruler, &lower, &upper, &max_size);

  if ((upper - lower) == 0)
    goto out;

  increment = (gdouble) width / (upper - lower);

  /* determine the scale
   *   use the maximum extents of the ruler to determine the largest
   *   possible number to be displayed.  Calculate the height in pixels
   *   of this displayed text. Use this height to find a scale which
   *   leaves sufficient room for drawing the ruler.
   *
   *   We calculate the text size as for the vruler instead of
   *   actually measuring the text width, so that the result for the
   *   scale looks consistent with an accompanying vruler.
   */
  scale = ceil (max_size);
  g_snprintf (unit_str, sizeof (unit_str), "%d", scale);
  text_size = strlen (unit_str) * digit_height + 1;

  unit = ligma_ruler_get_unit (ruler);

  ruler_metric = ligma_ruler_get_metric (unit);

  for (scale = 0; scale < G_N_ELEMENTS (ruler_metric->ruler_scale); scale++)
    if (ruler_metric->ruler_scale[scale] * fabs (increment) > 2 * text_size)
      break;

  if (scale == G_N_ELEMENTS (ruler_metric->ruler_scale))
    scale = G_N_ELEMENTS (ruler_metric->ruler_scale) - 1;

  /* drawing starts here */
  length = 0;
  for (i = G_N_ELEMENTS (ruler_metric->subdivide) - 1; i >= 0; i--)
    {
      gdouble subd_incr;

      /* hack to get proper subdivisions at full pixels */
      if (unit == LIGMA_UNIT_PIXEL && scale == 1 && i == 1)
        subd_incr = 1.0;
      else
        subd_incr = ((gdouble) ruler_metric->ruler_scale[scale] /
                     (gdouble) ruler_metric->subdivide[i]);

      if (subd_incr * fabs (increment) <= MINIMUM_INCR)
        continue;

      /* don't subdivide pixels */
      if (unit == LIGMA_UNIT_PIXEL && subd_incr < 1.0)
        continue;

      /* Calculate the length of the tickmarks. Make sure that
       * this length increases for each set of ticks
       */
      ideal_length = height / (i + 1) - 1;
      if (ideal_length > ++length)
        length = ideal_length;

      if (lower < upper)
        {
          start = floor (lower / subd_incr) * subd_incr;
          end   = ceil  (upper / subd_incr) * subd_incr;
        }
      else
        {
          start = floor (upper / subd_incr) * subd_incr;
          end   = ceil  (lower / subd_incr) * subd_incr;
        }

      for (cur = start; cur <= end; cur += subd_incr)
        {
          pos = ROUND ((cur - lower) * increment);

          if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              cairo_rectangle (cr,
                               pos, height + border.top - length,
                               1,   length);
            }
          else
            {
              cairo_rectangle (cr,
                               height + border.left - length, pos,
                               length,                        1);
            }

          /* draw label */
          if (i == 0)
            {
              g_snprintf (unit_str, sizeof (unit_str), "%d", (int) cur);

              if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
                {
                  pango_layout_set_text (layout, unit_str, -1);
                  pango_layout_get_extents (layout, &logical_rect, NULL);

                  cairo_move_to (cr,
                                 pos + 2,
                                 border.top + PANGO_PIXELS (logical_rect.y - digit_offset));
                  pango_cairo_show_layout (cr, layout);
                }
              else
                {
                  gint j;

                  for (j = 0; j < (int) strlen (unit_str); j++)
                    {
                      pango_layout_set_text (layout, unit_str + j, 1);
                      pango_layout_get_extents (layout, NULL, &logical_rect);

                      cairo_move_to (cr,
                                     border.left + 1,
                                     pos + digit_height * j + 2 + PANGO_PIXELS (logical_rect.y - digit_offset));
                      pango_cairo_show_layout (cr, layout);
                    }
                }
            }
        }
    }

  cairo_fill (cr);

  priv->backing_store_valid = TRUE;

out:
  cairo_destroy (cr);
}

static GdkRectangle
ligma_ruler_get_pos_rect (LigmaRuler *ruler,
                         gdouble    position)
{
  GtkWidget        *widget  = GTK_WIDGET (ruler);
  GtkStyleContext  *context = gtk_widget_get_style_context (widget);
  LigmaRulerPrivate *priv    = GET_PRIVATE (ruler);
  GtkAllocation     allocation;
  GtkBorder         border;
  gint              width, height;
  gdouble           upper, lower;
  gdouble           increment;
  GdkRectangle      rect = { 0, };

  if (! gtk_widget_is_drawable (widget))
    return rect;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_style_context_get_border (context, gtk_widget_get_state_flags (widget), &border);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      width  = allocation.width;
      height = allocation.height - (border.top + border.bottom);

      rect.width = height / 2 + 2;
      rect.width |= 1;  /* make sure it's odd */
      rect.height = rect.width / 2 + 1;
    }
  else
    {
      width  = allocation.width - (border.left + border.right);
      height = allocation.height;

      rect.height = width / 2 + 2;
      rect.height |= 1;  /* make sure it's odd */
      rect.width = rect.height / 2 + 1;
    }

  ligma_ruler_get_range (ruler, &lower, &upper, NULL);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      increment = (gdouble) width / (upper - lower);

      rect.x = ROUND ((position - lower) * increment) + (border.left + border.right - rect.width) / 2 - 1;
      rect.y = (height + rect.height) / 2 + border.top;
    }
  else
    {
      increment = (gdouble) height / (upper - lower);

      rect.x = (width + rect.width) / 2 + border.left;
      rect.y = ROUND ((position - lower) * increment) + (border.top + border.bottom - rect.height) / 2 - 1;
    }

  return rect;
}

static gboolean
ligma_ruler_idle_queue_pos_redraw (gpointer data)
{
  LigmaRuler        *ruler     = data;
  LigmaRulerPrivate *priv      = GET_PRIVATE (ruler);

  ligma_ruler_queue_pos_redraw (ruler);

  /*
    * pos_redraw_idle_id being set can be counted on to mean
    * a redraw is needed (on mac only) since we will not do
    * a high priority draws due to the dramatic performance
    * they will have.
    */
  priv->pos_redraw_idle_id = 0;

  return G_SOURCE_REMOVE;
}

static void
ligma_ruler_queue_pos_redraw (LigmaRuler *ruler)
{
  LigmaRulerPrivate  *priv = GET_PRIVATE (ruler);
  const GdkRectangle rect = ligma_ruler_get_pos_rect (ruler, priv->position);
  GtkAllocation      allocation;

  gtk_widget_get_allocation (GTK_WIDGET(ruler), &allocation);

  gtk_widget_queue_draw_area (GTK_WIDGET (ruler),
                              rect.x + allocation.x,
                              rect.y + allocation.y,
                              rect.width,
                              rect.height);

  if (priv->last_pos_rect.width  != 0 &&
      priv->last_pos_rect.height != 0)
    {
      gtk_widget_queue_draw_area (GTK_WIDGET (ruler),
                                  priv->last_pos_rect.x + allocation.x,
                                  priv->last_pos_rect.y + allocation.y,
                                  priv->last_pos_rect.width,
                                  priv->last_pos_rect.height);

      priv->last_pos_rect.x      = 0;
      priv->last_pos_rect.y      = 0;
      priv->last_pos_rect.width  = 0;
      priv->last_pos_rect.height = 0;
    }
}

static void
ligma_ruler_draw_pos (LigmaRuler *ruler,
                     cairo_t   *cr)
{
  GtkWidget        *widget  = GTK_WIDGET (ruler);
  GtkStyleContext  *context = gtk_widget_get_style_context (widget);
  LigmaRulerPrivate *priv    = GET_PRIVATE (ruler);
  GdkRectangle      pos_rect;

  if (! gtk_widget_is_drawable (widget))
    return;

  pos_rect = ligma_ruler_get_pos_rect (ruler, ligma_ruler_get_position (ruler));

  if ((pos_rect.width > 0) && (pos_rect.height > 0))
    {
      GdkRGBA color;

      gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget),
                                   &color);
      gdk_cairo_set_source_rgba (cr, &color);

      cairo_move_to (cr, pos_rect.x, pos_rect.y);

      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          cairo_line_to (cr, pos_rect.x + pos_rect.width / 2.0,
                             pos_rect.y + pos_rect.height);
          cairo_line_to (cr, pos_rect.x + pos_rect.width,
                             pos_rect.y);
        }
      else
        {
          cairo_line_to (cr, pos_rect.x + pos_rect.width,
                             pos_rect.y + pos_rect.height / 2.0);
          cairo_line_to (cr, pos_rect.x,
                             pos_rect.y + pos_rect.height);
        }

      cairo_fill (cr);
    }

  if (priv->last_pos_rect.width  != 0 &&
      priv->last_pos_rect.height != 0)
    {
      gdk_rectangle_union (&priv->last_pos_rect,
                           &pos_rect,
                           &priv->last_pos_rect);
    }
  else
    {
      priv->last_pos_rect = pos_rect;
    }
}

static void
ligma_ruler_make_pixmap (LigmaRuler *ruler)
{
  GtkWidget        *widget = GTK_WIDGET (ruler);
  LigmaRulerPrivate *priv   = GET_PRIVATE (ruler);
  GtkAllocation     allocation;

  gtk_widget_get_allocation (widget, &allocation);

  if (priv->backing_store)
    cairo_surface_destroy (priv->backing_store);

  priv->backing_store =
    gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                       CAIRO_CONTENT_COLOR_ALPHA,
                                       allocation.width,
                                       allocation.height);

  priv->backing_store_valid = FALSE;
}

static PangoLayout *
ligma_ruler_get_layout (GtkWidget   *widget,
                       const gchar *text)
{
  LigmaRulerPrivate *priv = GET_PRIVATE (widget);

  if (priv->layout)
    {
      pango_layout_set_text (priv->layout, text, -1);
      return priv->layout;
    }

  priv->layout = gtk_widget_create_pango_layout (widget, text);

  return priv->layout;
}

#define FACTOR_EPSILON  0.0000001
#define FACTOR_EQUAL(u, f) (ABS (f - ligma_unit_get_factor (u)) < FACTOR_EPSILON)

static const RulerMetric *
ligma_ruler_get_metric (LigmaUnit unit)
{
  if (unit == LIGMA_UNIT_INCH)
    {
      return  &ruler_metric_inches;
    }
  /* XXX: recognizing feet or yard unit this way definitely sucks.
   * Actually the subdvision and rule scale rules should probably become
   * settable values in unitrc instead of hardcoded rules.
   * This way, people would be able to set how they want a unit to be
   * shown (we could definitely imagine someone wanting to see inches
   * with base-10 divisions).
   */
  else if (FACTOR_EQUAL (unit, 0.083333))
    {
      return  &ruler_metric_feet;
    }
  else if (FACTOR_EQUAL (unit, 0.027778))
    {
      return  &ruler_metric_yards;
    }

  return &ruler_metric_decimal;
}
