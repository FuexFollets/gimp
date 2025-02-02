/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmacurve.h"
#include "core/ligmacurve-map.h"

#include "ligmaclipboard.h"
#include "ligmacurveview.h"
#include "ligmawidgets-utils.h"


#define POINT_MAX_DISTANCE 16.0


enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_BASE_LINE,
  PROP_GRID_ROWS,
  PROP_GRID_COLUMNS,
  PROP_X_AXIS_LABEL,
  PROP_Y_AXIS_LABEL
};

enum
{
  SELECTION_CHANGED,
  CUT_CLIPBOARD,
  COPY_CLIPBOARD,
  PASTE_CLIPBOARD,
  LAST_SIGNAL
};


typedef struct
{
  LigmaCurve *curve;
  LigmaRGB    color;
  gboolean   color_set;
} BGCurve;


static void       ligma_curve_view_finalize              (GObject          *object);
static void       ligma_curve_view_dispose               (GObject          *object);
static void       ligma_curve_view_set_property          (GObject          *object,
                                                         guint             property_id,
                                                         const GValue     *value,
                                                         GParamSpec       *pspec);
static void       ligma_curve_view_get_property          (GObject          *object,
                                                         guint             property_id,
                                                         GValue           *value,
                                                         GParamSpec       *pspec);

static void       ligma_curve_view_style_updated         (GtkWidget        *widget);
static gboolean   ligma_curve_view_draw                  (GtkWidget        *widget,
                                                         cairo_t          *cr);
static gboolean   ligma_curve_view_button_press          (GtkWidget        *widget,
                                                         GdkEventButton   *bevent);
static gboolean   ligma_curve_view_button_release        (GtkWidget        *widget,
                                                         GdkEventButton   *bevent);
static gboolean   ligma_curve_view_motion_notify         (GtkWidget        *widget,
                                                         GdkEventMotion   *bevent);
static gboolean   ligma_curve_view_leave_notify          (GtkWidget        *widget,
                                                         GdkEventCrossing *cevent);
static gboolean   ligma_curve_view_key_press             (GtkWidget        *widget,
                                                         GdkEventKey      *kevent);

static void       ligma_curve_view_cut_clipboard         (LigmaCurveView    *view);
static void       ligma_curve_view_copy_clipboard        (LigmaCurveView    *view);
static void       ligma_curve_view_paste_clipboard       (LigmaCurveView    *view);

static void       ligma_curve_view_curve_dirty           (LigmaCurve        *curve,
                                                         LigmaCurveView    *view);
static void       ligma_curve_view_curve_notify_n_points (LigmaCurve        *curve,
                                                         GParamSpec       *pspec,
                                                         LigmaCurveView    *view);

static void       ligma_curve_view_set_cursor            (LigmaCurveView    *view,
                                                         gdouble           x,
                                                         gdouble           y);
static void       ligma_curve_view_unset_cursor          (LigmaCurveView *view);


G_DEFINE_TYPE (LigmaCurveView, ligma_curve_view,
               LIGMA_TYPE_HISTOGRAM_VIEW)

#define parent_class ligma_curve_view_parent_class

static guint curve_view_signals[LAST_SIGNAL] = { 0 };


static void
ligma_curve_view_class_init (LigmaCurveViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkBindingSet  *binding_set;

  object_class->finalize             = ligma_curve_view_finalize;
  object_class->dispose              = ligma_curve_view_dispose;
  object_class->set_property         = ligma_curve_view_set_property;
  object_class->get_property         = ligma_curve_view_get_property;

  widget_class->style_updated        = ligma_curve_view_style_updated;
  widget_class->draw                 = ligma_curve_view_draw;
  widget_class->button_press_event   = ligma_curve_view_button_press;
  widget_class->button_release_event = ligma_curve_view_button_release;
  widget_class->motion_notify_event  = ligma_curve_view_motion_notify;
  widget_class->leave_notify_event   = ligma_curve_view_leave_notify;
  widget_class->key_press_event      = ligma_curve_view_key_press;

  klass->selection_changed           = NULL;
  klass->cut_clipboard               = ligma_curve_view_cut_clipboard;
  klass->copy_clipboard              = ligma_curve_view_copy_clipboard;
  klass->paste_clipboard             = ligma_curve_view_paste_clipboard;

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_BASE_LINE,
                                   g_param_spec_boolean ("base-line",
                                                         NULL, NULL,
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_GRID_ROWS,
                                   g_param_spec_int ("grid-rows", NULL, NULL,
                                                     0, 100, 8,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_GRID_COLUMNS,
                                   g_param_spec_int ("grid-columns", NULL, NULL,
                                                     0, 100, 8,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_X_AXIS_LABEL,
                                   g_param_spec_string ("x-axis-label", NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_Y_AXIS_LABEL,
                                   g_param_spec_string ("y-axis-label", NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE));

  curve_view_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaCurveViewClass, selection_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  curve_view_signals[CUT_CLIPBOARD] =
    g_signal_new ("cut-clipboard",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (LigmaCurveViewClass, cut_clipboard),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  curve_view_signals[COPY_CLIPBOARD] =
    g_signal_new ("copy-clipboard",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (LigmaCurveViewClass, copy_clipboard),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  curve_view_signals[PASTE_CLIPBOARD] =
    g_signal_new ("paste-clipboard",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (LigmaCurveViewClass, paste_clipboard),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  binding_set = gtk_binding_set_by_class (klass);

  gtk_binding_entry_add_signal (binding_set, GDK_KEY_x, GDK_CONTROL_MASK,
                                "cut-clipboard", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_c, GDK_CONTROL_MASK,
                                "copy-clipboard", 0);
  gtk_binding_entry_add_signal (binding_set, GDK_KEY_v, GDK_CONTROL_MASK,
                                "paste-clipboard", 0);
}

static void
ligma_curve_view_init (LigmaCurveView *view)
{
  view->curve       = NULL;
  view->selected    = -1;
  view->offset_x    = 0.0;
  view->offset_y    = 0.0;
  view->last_x      = 0.0;
  view->last_y      = 0.0;
  view->cursor_type = -1;
  view->xpos        = -1.0;
  view->cursor_x    = -1.0;
  view->cursor_y    = -1.0;
  view->range_x_min = 0.0;
  view->range_x_max = 1.0;
  view->range_y_min = 0.0;
  view->range_y_max = 1.0;

  view->x_axis_label = NULL;
  view->y_axis_label = NULL;

  gtk_widget_set_can_focus (GTK_WIDGET (view), TRUE);
  gtk_widget_add_events (GTK_WIDGET (view),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_KEY_PRESS_MASK      |
                         GDK_LEAVE_NOTIFY_MASK);
}

static void
ligma_curve_view_finalize (GObject *object)
{
  LigmaCurveView *view = LIGMA_CURVE_VIEW (object);

  g_clear_object (&view->orig_curve);

  g_clear_object (&view->layout);
  g_clear_object (&view->cursor_layout);

  g_clear_pointer (&view->x_axis_label, g_free);
  g_clear_pointer (&view->y_axis_label, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_curve_view_dispose (GObject *object)
{
  LigmaCurveView *view = LIGMA_CURVE_VIEW (object);

  ligma_curve_view_set_curve (view, NULL, NULL);

  if (view->bg_curves)
    ligma_curve_view_remove_all_backgrounds (view);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_curve_view_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaCurveView *view = LIGMA_CURVE_VIEW (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      view->ligma = g_value_get_object (value); /* don't ref */
      break;
    case PROP_GRID_ROWS:
      view->grid_rows = g_value_get_int (value);
      break;
    case PROP_GRID_COLUMNS:
      view->grid_columns = g_value_get_int (value);
      break;
    case PROP_BASE_LINE:
      view->draw_base_line = g_value_get_boolean (value);
      break;
    case PROP_X_AXIS_LABEL:
      ligma_curve_view_set_x_axis_label (view, g_value_get_string (value));
      break;
    case PROP_Y_AXIS_LABEL:
      ligma_curve_view_set_y_axis_label (view, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_curve_view_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaCurveView *view = LIGMA_CURVE_VIEW (object);

  switch (property_id)
    {
    case PROP_LIGMA:
      g_value_set_object (value, view->ligma);
      break;
    case PROP_GRID_ROWS:
      g_value_set_int (value, view->grid_rows);
      break;
    case PROP_GRID_COLUMNS:
      g_value_set_int (value, view->grid_columns);
      break;
    case PROP_BASE_LINE:
      g_value_set_boolean (value, view->draw_base_line);
      break;
    case PROP_X_AXIS_LABEL:
      g_value_set_string (value, view->x_axis_label);
      break;
    case PROP_Y_AXIS_LABEL:
      g_value_set_string (value, view->y_axis_label);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_curve_view_style_updated (GtkWidget *widget)
{
  LigmaCurveView *view = LIGMA_CURVE_VIEW (widget);

  GTK_WIDGET_CLASS (parent_class)->style_updated (widget);

  g_clear_object (&view->layout);
  g_clear_object (&view->cursor_layout);
}

static void
ligma_curve_view_draw_grid (LigmaCurveView *view,
                           cairo_t       *cr,
                           gint           width,
                           gint           height,
                           gint           border)
{
  gint i;

  for (i = 1; i < view->grid_rows; i++)
    {
      gint y = i * (height - 1) / view->grid_rows;

      if ((view->grid_rows % 2) == 0 && (i == view->grid_rows / 2))
        continue;

      cairo_move_to (cr, border,             border + y);
      cairo_line_to (cr, border + width - 1, border + y);
    }

  for (i = 1; i < view->grid_columns; i++)
    {
      gint x = i * (width - 1) / view->grid_columns;

      if ((view->grid_columns % 2) == 0 && (i == view->grid_columns / 2))
        continue;

      cairo_move_to (cr, border + x, border);
      cairo_line_to (cr, border + x, border + height - 1);
    }

  if (view->draw_base_line)
    {
      cairo_move_to (cr, border, border + height - 1);
      cairo_line_to (cr, border + width - 1, border);
    }

  cairo_set_line_width (cr, 0.6);
  cairo_stroke (cr);

  if ((view->grid_rows % 2) == 0)
    {
      gint y = (height - 1) / 2;

      cairo_move_to (cr, border,             border + y);
      cairo_line_to (cr, border + width - 1, border + y);
    }

  if ((view->grid_columns % 2) == 0)
    {
      gint x = (width - 1) / 2;

      cairo_move_to (cr, border + x, border);
      cairo_line_to (cr, border + x, border + height - 1);
    }

  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);
}

static void
ligma_curve_view_draw_point (LigmaCurveView *view,
                            cairo_t       *cr,
                            gint           i,
                            gint           width,
                            gint           height,
                            gint           border)
{
  gdouble x, y;

  ligma_curve_get_point (view->curve, i, &x, &y);

  y = 1.0 - y;

#define CIRCLE_RADIUS  3
#define DIAMOND_RADIUS (G_SQRT2 * CIRCLE_RADIUS)

  switch (ligma_curve_get_point_type (view->curve, i))
    {
    case LIGMA_CURVE_POINT_SMOOTH:
      cairo_move_to (cr,
                     border + (gdouble) (width  - 1) * x + CIRCLE_RADIUS,
                     border + (gdouble) (height - 1) * y);
      cairo_arc     (cr,
                     border + (gdouble) (width  - 1) * x,
                     border + (gdouble) (height - 1) * y,
                     CIRCLE_RADIUS,
                     0, 2 * G_PI);
      break;

    case LIGMA_CURVE_POINT_CORNER:
      cairo_move_to    (cr,
                        border + (gdouble) (width  - 1) * x,
                        border + (gdouble) (height - 1) * y - DIAMOND_RADIUS);
      cairo_line_to    (cr,
                        border + (gdouble) (width  - 1) * x + DIAMOND_RADIUS,
                        border + (gdouble) (height - 1) * y);
      cairo_line_to    (cr,
                        border + (gdouble) (width  - 1) * x,
                        border + (gdouble) (height - 1) * y + DIAMOND_RADIUS);
      cairo_line_to    (cr,
                        border + (gdouble) (width  - 1) * x - DIAMOND_RADIUS,
                        border + (gdouble) (height - 1) * y);
      cairo_close_path (cr);
      break;
    }
}

static void
ligma_curve_view_draw_curve (LigmaCurveView *view,
                            cairo_t       *cr,
                            LigmaCurve     *curve,
                            gint           width,
                            gint           height,
                            gint           border)
{
  gdouble x, y;
  gint    i;

  x = 0.0;
  y = 1.0 - ligma_curve_map_value (curve, 0.0);

  cairo_move_to (cr,
                 border + (gdouble) (width  - 1) * x,
                 border + (gdouble) (height - 1)* y);

  for (i = 1; i < 256; i++)
    {
      x = (gdouble) i / 255.0;
      y = 1.0 - ligma_curve_map_value (curve, x);

      cairo_line_to (cr,
                     border + (gdouble) (width  - 1) * x,
                     border + (gdouble) (height - 1) * y);
    }

  cairo_stroke (cr);
}

static gboolean
ligma_curve_view_draw (GtkWidget *widget,
                      cairo_t   *cr)
{
  LigmaCurveView   *view  = LIGMA_CURVE_VIEW (widget);
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  GdkRGBA          grid_color;
  GdkRGBA          fg_color;
  GdkRGBA          bg_color;
  GList           *list;
  gint             border;
  gint             width;
  gint             height;
  gint             layout_x;
  gint             layout_y;
  gdouble          x, y;
  gint             i;

  cairo_save (cr);
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);
  cairo_restore (cr);

  if (! view->curve)
    return FALSE;

  gtk_style_context_save (style);
  gtk_style_context_add_class (style, "view");

  gtk_widget_get_allocation (widget, &allocation);

  border = LIGMA_HISTOGRAM_VIEW (view)->border_width;
  width  = allocation.width  - 2 * border;
  height = allocation.height - 2 * border;

  if (gtk_widget_has_focus (widget))
    {
      gtk_render_focus (style, cr,
                        border - 2, border - 2,
                        width + 4, height + 4);
    }

  cairo_set_line_width (cr, 1.0);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_translate (cr, 0.5, 0.5);

  gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                               &fg_color);
  bg_color       = fg_color;
  bg_color.red   = 1 - bg_color.red;
  bg_color.green = 1 - bg_color.green;
  bg_color.blue  = 1 - bg_color.blue;

  gtk_style_context_add_class (style, "grid");
  gtk_style_context_get_color (style, gtk_style_context_get_state (style),
                               &grid_color);
  gtk_style_context_remove_class (style, "grid");

  /*  Draw the grid lines  */
  gdk_cairo_set_source_rgba (cr, &grid_color);

  ligma_curve_view_draw_grid (view, cr, width, height, border);

  /*  Draw the axis labels  */

  if (view->x_axis_label)
    {
      if (! view->layout)
        view->layout = gtk_widget_create_pango_layout (widget, NULL);

      pango_layout_set_text (view->layout, view->x_axis_label, -1);
      pango_layout_get_pixel_size (view->layout, &layout_x, &layout_y);

      cairo_move_to (cr,
                     width - border - layout_x,
                     height - border - layout_y);

      pango_cairo_show_layout (cr, view->layout);
    }

  if (view->y_axis_label)
    {
      if (! view->layout)
        view->layout = gtk_widget_create_pango_layout (widget, NULL);

      pango_layout_set_text (view->layout, view->y_axis_label, -1);
      pango_layout_get_pixel_size (view->layout, &layout_x, &layout_y);

      cairo_save (cr);

      cairo_move_to (cr,
                     2 * border,
                     2 * border + layout_x);
      cairo_rotate (cr, - G_PI / 2);

      pango_cairo_show_layout (cr, view->layout);

      cairo_restore (cr);
    }


  /*  Draw the background curves  */
  for (list = view->bg_curves; list; list = g_list_next (list))
    {
      BGCurve *bg = list->data;

      if (bg->color_set)
        {
          cairo_set_source_rgba (cr,
                                 bg->color.r,
                                 bg->color.g,
                                 bg->color.b,
                                 0.5);
        }
      else
        {
          cairo_set_source_rgba (cr,
                                 fg_color.red,
                                 fg_color.green,
                                 fg_color.blue,
                                 0.5);
        }

      ligma_curve_view_draw_curve (view, cr, bg->curve,
                                  width, height, border);
    }

  /*  Draw the curve  */
  if (view->curve_color)
    ligma_cairo_set_source_rgb (cr, view->curve_color);
  else
    gdk_cairo_set_source_rgba (cr, &fg_color);

  ligma_curve_view_draw_curve (view, cr, view->curve,
                              width, height, border);

  /*  Draw the points  */
  if (ligma_curve_get_curve_type (view->curve) == LIGMA_CURVE_SMOOTH)
    {
      /*  Draw the unselected points  */
      for (i = 0; i < view->curve->n_points; i++)
        {
          if (i == view->selected)
            continue;

          ligma_curve_view_draw_point (view, cr, i, width, height, border);
        }

      cairo_stroke (cr);

      /*  Draw the selected point  */
      if (view->selected != -1)
        {
          ligma_curve_view_draw_point (view, cr, view->selected,
                                      width, height, border);
          cairo_fill (cr);
       }
    }

  if (view->xpos >= 0.0)
    {
      gchar buf[32];

      /* draw the color line */
      cairo_move_to (cr,
                     border + ROUND ((gdouble) (width - 1) * view->xpos),
                     border + 1);
      cairo_line_to (cr,
                     border + ROUND ((gdouble) (width - 1) * view->xpos),
                     border + height - 1);
      cairo_stroke (cr);

      if (view->range_x_max == 255.0)
        {
          /*  stupid heuristic: special-case for 0..255  */

          g_snprintf (buf, sizeof (buf), "x:%3d",
                      (gint) (view->xpos *
                              (view->range_x_max - view->range_x_min) +
                              view->range_x_min));
        }
      else if (view->range_x_max == 100.0)
        {
          /*  and for 0..100  */

          g_snprintf (buf, sizeof (buf), "x:%0.2f",
                      view->xpos *
                      (view->range_x_max - view->range_x_min) +
                      view->range_x_min);
        }
      else
        {
          g_snprintf (buf, sizeof (buf), "x:%0.3f",
                      view->xpos *
                      (view->range_x_max - view->range_x_min) +
                      view->range_x_min);
        }

      if (! view->layout)
        view->layout = gtk_widget_create_pango_layout (widget, NULL);

      pango_layout_set_text (view->layout, buf, -1);
      pango_layout_get_pixel_size (view->layout, &layout_x, &layout_y);

      if (view->xpos < 0.5)
        layout_x = border;
      else
        layout_x = -(layout_x + border);

      cairo_move_to (cr,
                     border + (gdouble) width * view->xpos + layout_x,
                     border + height - border - layout_y);
      pango_cairo_show_layout (cr, view->layout);
    }

  if (view->cursor_x >= 0.0 && view->cursor_x <= 1.0 &&
      view->cursor_y >= 0.0 && view->cursor_y <= 1.0)
    {
      gchar  buf[32];
      gint   w, h;

      if (! view->cursor_layout)
        view->cursor_layout = gtk_widget_create_pango_layout (widget, NULL);

      if (view->range_x_max == 255.0 &&
          view->range_y_max == 255.0)
        {
          /*  stupid heuristic: special-case for 0..255  */

          g_snprintf (buf, sizeof (buf), "x:%3d y:%3d",
                      (gint) round (view->cursor_x *
                                    (view->range_x_max - view->range_x_min) +
                                    view->range_x_min),
                      (gint) round ((1.0 - view->cursor_y) *
                                    (view->range_y_max - view->range_y_min) +
                                    view->range_y_min));
        }
      else if (view->range_x_max == 100.0 &&
               view->range_y_max == 100.0)
        {
          /*  and for 0..100  */

          g_snprintf (buf, sizeof (buf), "x:%0.2f y:%0.2f",
                      view->cursor_x *
                      (view->range_x_max - view->range_x_min) +
                      view->range_x_min,
                      (1.0 - view->cursor_y) *
                      (view->range_y_max - view->range_y_min) +
                      view->range_y_min);
        }
      else
        {
          g_snprintf (buf, sizeof (buf), "x:%0.3f y:%0.3f",
                      view->cursor_x *
                      (view->range_x_max - view->range_x_min) +
                      view->range_x_min,
                      (1.0 - view->cursor_y) *
                      (view->range_y_max - view->range_y_min) +
                      view->range_y_min);
        }

      pango_layout_set_text (view->cursor_layout, buf, -1);
      pango_layout_get_pixel_extents (view->cursor_layout,
                                      NULL, &view->cursor_rect);

      x = border * 2 + 3;
      y = border * 2 + 3;
      w = view->cursor_rect.width;
      h = view->cursor_rect.height;

      if (view->x_axis_label)
        x += border + view->cursor_rect.height; /* coincidentially the right value */

      cairo_push_group (cr);

      cairo_rectangle (cr, x + 0.5, y + 0.5, w, h);
      cairo_fill_preserve (cr);

      cairo_set_line_width (cr, 6);
      cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
      cairo_stroke (cr);

      gdk_cairo_set_source_rgba (cr, &bg_color);

      cairo_move_to (cr, x, y);
      pango_cairo_show_layout (cr, view->cursor_layout);

      cairo_pop_group_to_source (cr);
      cairo_paint_with_alpha (cr, 0.6);
    }

  gtk_style_context_restore (style);

  return FALSE;
}

static void
set_cursor (LigmaCurveView *view,
            GdkCursorType  new_cursor)
{
  if (new_cursor != view->cursor_type)
    {
      GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (view));
      GdkCursor  *cursor  = gdk_cursor_new_for_display (display, new_cursor);

      gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (view)), cursor);
      g_object_unref (cursor);

      view->cursor_type = new_cursor;
    }
}

static gboolean
ligma_curve_view_button_press (GtkWidget      *widget,
                              GdkEventButton *bevent)
{
  LigmaCurveView *view  = LIGMA_CURVE_VIEW (widget);
  LigmaCurve     *curve = view->curve;
  GtkAllocation  allocation;
  gint           border;
  gint           width, height;
  gdouble        x;
  gdouble        y;
  gint           point;
  gdouble        point_x;
  gdouble        point_y;

  if (! curve || bevent->button != 1)
    return TRUE;

  gtk_widget_get_allocation (widget, &allocation);

  border = LIGMA_HISTOGRAM_VIEW (view)->border_width;
  width  = allocation.width  - 2 * border;
  height = allocation.height - 2 * border;

  x = (gdouble) (bevent->x - border) / (gdouble) width;
  y = (gdouble) (bevent->y - border) / (gdouble) height;

  x = CLAMP (x, 0.0, 1.0);
  y = CLAMP (y, 0.0, 1.0);

  view->grabbed = TRUE;

  view->orig_curve = LIGMA_CURVE (ligma_data_duplicate (LIGMA_DATA (curve)));

  set_cursor (view, GDK_TCROSS);

  switch (ligma_curve_get_curve_type (curve))
    {
    case LIGMA_CURVE_SMOOTH:
      point = ligma_curve_get_closest_point (curve, x, 1.0 - y,
                                            POINT_MAX_DISTANCE /
                                            MAX (width, height));

      if (point < 0)
        {
          LigmaCurvePointType type = LIGMA_CURVE_POINT_SMOOTH;

          if (bevent->state & ligma_get_constrain_behavior_mask ())
            y = 1.0 - ligma_curve_map_value (view->orig_curve, x);

          if (view->selected >= 0)
            type = ligma_curve_get_point_type (curve, view->selected);

          point = ligma_curve_add_point (curve, x, 1.0 - y);

          ligma_curve_set_point_type (curve, point, type);
        }

      if (point > 0)
        ligma_curve_get_point (curve, point - 1, &view->leftmost, NULL);
      else
        view->leftmost = -1.0;

      if (point < ligma_curve_get_n_points (curve) - 1)
        ligma_curve_get_point (curve, point + 1, &view->rightmost, NULL);
      else
        view->rightmost = 2.0;

      ligma_curve_view_set_selected (view, point);

      ligma_curve_get_point (curve, point, &point_x, &point_y);

      view->offset_x = point_x         - x;
      view->offset_y = (1.0 - point_y) - y;

      view->point_type = ligma_curve_get_point_type (curve, point);
      break;

    case LIGMA_CURVE_FREE:
      view->last_x = x;
      view->last_y = y;

      ligma_curve_set_curve (curve, x, 1.0 - y);
      break;
    }

  if (! gtk_widget_has_focus (widget))
    gtk_widget_grab_focus (widget);

  return TRUE;
}

static gboolean
ligma_curve_view_button_release (GtkWidget      *widget,
                                GdkEventButton *bevent)
{
  LigmaCurveView *view = LIGMA_CURVE_VIEW (widget);

  if (bevent->button != 1)
    return TRUE;

  g_clear_object (&view->orig_curve);

  view->offset_x = 0.0;
  view->offset_y = 0.0;

  view->grabbed = FALSE;

  set_cursor (view, GDK_FLEUR);

  return TRUE;
}

static gboolean
ligma_curve_view_motion_notify (GtkWidget      *widget,
                               GdkEventMotion *mevent)
{
  LigmaCurveView  *view       = LIGMA_CURVE_VIEW (widget);
  LigmaCurve      *curve      = view->curve;
  GtkAllocation   allocation;
  GdkCursorType   new_cursor = GDK_X_CURSOR;
  gint            border;
  gint            width, height;
  gdouble         x;
  gdouble         y;
  gdouble         point_x;
  gdouble         point_y;
  gint            point;

  if (! curve)
    return TRUE;

  gtk_widget_get_allocation (widget, &allocation);

  border = LIGMA_HISTOGRAM_VIEW (view)->border_width;
  width  = allocation.width  - 2 * border;
  height = allocation.height - 2 * border;

  x = (gdouble) (mevent->x - border) / (gdouble) width;
  y = (gdouble) (mevent->y - border) / (gdouble) height;

  x += view->offset_x;
  y += view->offset_y;

  x = CLAMP (x, 0.0, 1.0);
  y = CLAMP (y, 0.0, 1.0);

  switch (ligma_curve_get_curve_type (curve))
    {
    case LIGMA_CURVE_SMOOTH:
      if (! view->grabbed) /*  If no point is grabbed...  */
        {
          point = ligma_curve_get_closest_point (curve, x, 1.0 - y,
                                                POINT_MAX_DISTANCE /
                                                MAX (width, height));

          if (point >= 0)
            {
              ligma_curve_get_point (curve, point, &point_x, &point_y);

              new_cursor = GDK_FLEUR;

              x = point_x;
              y = 1.0 - point_y;
            }
          else
            {
              new_cursor = GDK_TCROSS;

              if (mevent->state & ligma_get_constrain_behavior_mask ())
                y = 1.0 - ligma_curve_map_value (view->curve, x);
            }
        }
      else /*  Else, drag the grabbed point  */
        {
          new_cursor = GDK_TCROSS;

          if (mevent->state & ligma_get_constrain_behavior_mask ())
            y = 1.0 - ligma_curve_map_value (view->orig_curve, x);

          ligma_data_freeze (LIGMA_DATA (curve));

          if (x > view->leftmost && x < view->rightmost)
            {
              if (view->selected < 0)
                {
                  ligma_curve_view_set_selected (
                    view,
                    ligma_curve_add_point (curve, x, 1.0 - y));

                  ligma_curve_set_point_type (curve,
                                             view->selected, view->point_type);
                }
              else
                {
                  ligma_curve_set_point (curve, view->selected, x, 1.0 - y);
                }
            }
          else
            {
              if (view->selected >= 0)
                {
                  ligma_curve_delete_point (curve, view->selected);

                  ligma_curve_view_set_selected (view, -1);
                }
            }

          ligma_data_thaw (LIGMA_DATA (curve));
        }
      break;

    case LIGMA_CURVE_FREE:
      if (view->grabbed)
        {
          gint    n_samples = ligma_curve_get_n_samples (curve);
          gdouble x1, x2;
          gdouble y1, y2;

          if (view->last_x > x)
            {
              x1 = x;
              x2 = view->last_x;
              y1 = y;
              y2 = view->last_y;
            }
          else
            {
              x1 = view->last_x;
              x2 = x;
              y1 = view->last_y;
              y2 = y;
            }

          if (x2 != x1)
            {
              gint from = ROUND (x1 * (gdouble) (n_samples - 1));
              gint to   = ROUND (x2 * (gdouble) (n_samples - 1));
              gint i;

              ligma_data_freeze (LIGMA_DATA (curve));

              for (i = from; i <= to; i++)
                {
                  gdouble xpos = (gdouble) i / (gdouble) (n_samples - 1);
                  gdouble ypos = (y1 + ((y2 - y1) * (xpos - x1)) / (x2 - x1));

                  xpos = CLAMP (xpos, 0.0, 1.0);
                  ypos = CLAMP (ypos, 0.0, 1.0);

                  ligma_curve_set_curve (curve, xpos, 1.0 - ypos);
                }

              ligma_data_thaw (LIGMA_DATA (curve));
            }
          else
            {
              ligma_curve_set_curve (curve, x, 1.0 - y);
            }

          view->last_x = x;
          view->last_y = y;
        }

      if (mevent->state & GDK_BUTTON1_MASK)
        new_cursor = GDK_TCROSS;
      else
        new_cursor = GDK_PENCIL;

      break;
    }

  set_cursor (view, new_cursor);

  ligma_curve_view_set_cursor (view, x, y);

  return TRUE;
}

static gboolean
ligma_curve_view_leave_notify (GtkWidget        *widget,
                              GdkEventCrossing *cevent)
{
  LigmaCurveView *view = LIGMA_CURVE_VIEW (widget);

  ligma_curve_view_unset_cursor (view);

  return TRUE;
}

static gboolean
ligma_curve_view_key_press (GtkWidget   *widget,
                           GdkEventKey *kevent)
{
  LigmaCurveView *view    = LIGMA_CURVE_VIEW (widget);
  LigmaCurve     *curve   = view->curve;
  gboolean       handled = FALSE;

  if (! view->grabbed                                        &&
      curve                                                  &&
      ligma_curve_get_curve_type (curve) == LIGMA_CURVE_SMOOTH &&
      view->selected                    >= 0)
    {
      gint    i = view->selected;
      gdouble x, y;

      ligma_curve_get_point (curve, i, NULL, &y);

      switch (kevent->keyval)
        {
        case GDK_KEY_Left:
          for (i = i - 1; i >= 0 && ! handled; i--)
            {
              ligma_curve_get_point (curve, i, &x, NULL);

              if (x >= 0.0)
                {
                  ligma_curve_view_set_selected (view, i);

                  handled = TRUE;
                }
            }
          break;

        case GDK_KEY_Right:
          for (i = i + 1; i < curve->n_points && ! handled; i++)
            {
              ligma_curve_get_point (curve, i, &x, NULL);

              if (x >= 0.0)
                {
                  ligma_curve_view_set_selected (view, i);

                  handled = TRUE;
                }
            }
          break;

        case GDK_KEY_Up:
          if (y < 1.0)
            {
              y = y + (kevent->state & GDK_SHIFT_MASK ?
                       (16.0 / 255.0) : (1.0 / 255.0));

              ligma_curve_move_point (curve, i, CLAMP (y, 0.0, 1.0));

              handled = TRUE;
            }
          break;

        case GDK_KEY_Down:
          if (y > 0)
            {
              y = y - (kevent->state & GDK_SHIFT_MASK ?
                       (16.0 / 255.0) : (1.0 / 255.0));

              ligma_curve_move_point (curve, i, CLAMP (y, 0.0, 1.0));

              handled = TRUE;
            }
          break;

        case GDK_KEY_Delete:
          ligma_curve_delete_point (curve, i);
          break;

        default:
          break;
        }
    }

  if (handled)
    {
      set_cursor (view, GDK_TCROSS);

      return TRUE;
    }

  return GTK_WIDGET_CLASS (parent_class)->key_press_event (widget, kevent);
}

static void
ligma_curve_view_cut_clipboard (LigmaCurveView *view)
{
  g_printerr ("%s\n", G_STRFUNC);

  if (! view->curve || ! view->ligma)
    {
      gtk_widget_error_bell (GTK_WIDGET (view));
      return;
    }

  ligma_curve_view_copy_clipboard (view);

  ligma_curve_reset (view->curve, FALSE);
}

static void
ligma_curve_view_copy_clipboard (LigmaCurveView *view)
{
  LigmaCurve *copy;

  g_printerr ("%s\n", G_STRFUNC);

  if (! view->curve || ! view->ligma)
    {
      gtk_widget_error_bell (GTK_WIDGET (view));
      return;
    }

  copy = LIGMA_CURVE (ligma_data_duplicate (LIGMA_DATA (view->curve)));
  ligma_clipboard_set_curve (view->ligma, copy);
  g_object_unref (copy);
}

static void
ligma_curve_view_paste_clipboard (LigmaCurveView *view)
{
  LigmaCurve *copy;

  g_printerr ("%s\n", G_STRFUNC);

  if (! view->curve || ! view->ligma)
    {
      gtk_widget_error_bell (GTK_WIDGET (view));
      return;
    }

  copy = ligma_clipboard_get_curve (view->ligma);

  if (copy)
    {
      ligma_config_copy (LIGMA_CONFIG (copy),
                        LIGMA_CONFIG (view->curve), 0);
      g_object_unref (copy);
    }
}

static void
ligma_curve_view_curve_dirty (LigmaCurve     *curve,
                             LigmaCurveView *view)
{
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
ligma_curve_view_curve_notify_n_points (LigmaCurve     *curve,
                                       GParamSpec    *pspec,
                                       LigmaCurveView *view)
{
  ligma_curve_view_set_selected (view, -1);
}


/*  public functions  */

GtkWidget *
ligma_curve_view_new (void)
{
  return g_object_new (LIGMA_TYPE_CURVE_VIEW, NULL);
}

void
ligma_curve_view_set_curve (LigmaCurveView *view,
                           LigmaCurve     *curve,
                           const LigmaRGB *color)
{
  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));
  g_return_if_fail (curve == NULL || LIGMA_IS_CURVE (curve));

  if (view->curve == curve)
    return;

  if (view->curve)
    {
      g_signal_handlers_disconnect_by_func (view->curve,
                                            ligma_curve_view_curve_dirty,
                                            view);
      g_signal_handlers_disconnect_by_func (view->curve,
                                            ligma_curve_view_curve_notify_n_points,
                                            view);
      g_object_unref (view->curve);
    }

  view->curve = curve;

  if (view->curve)
    {
      g_object_ref (view->curve);
      g_signal_connect (view->curve, "dirty",
                        G_CALLBACK (ligma_curve_view_curve_dirty),
                        view);
      g_signal_connect (view->curve, "notify::n-points",
                        G_CALLBACK (ligma_curve_view_curve_notify_n_points),
                        view);
    }

  if (view->curve_color)
    g_free (view->curve_color);

  if (color)
    view->curve_color = g_memdup2 (color, sizeof (LigmaRGB));
  else
    view->curve_color = NULL;

  ligma_curve_view_set_selected (view, -1);

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

LigmaCurve *
ligma_curve_view_get_curve (LigmaCurveView *view)
{
  g_return_val_if_fail (LIGMA_IS_CURVE_VIEW (view), NULL);

  return view->curve;
}

void
ligma_curve_view_add_background (LigmaCurveView *view,
                                LigmaCurve     *curve,
                                const LigmaRGB *color)
{
  GList   *list;
  BGCurve *bg;

  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));
  g_return_if_fail (LIGMA_IS_CURVE (curve));

  for (list = view->bg_curves; list; list = g_list_next (list))
    {
      bg = list->data;

      g_return_if_fail (curve != bg->curve);
    }

  bg = g_slice_new0 (BGCurve);

  bg->curve = g_object_ref (curve);

  if (color)
    {
      bg->color     = *color;
      bg->color_set = TRUE;
    }

  g_signal_connect (bg->curve, "dirty",
                    G_CALLBACK (ligma_curve_view_curve_dirty),
                    view);

  view->bg_curves = g_list_append (view->bg_curves, bg);

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
ligma_curve_view_remove_background (LigmaCurveView *view,
                                   LigmaCurve     *curve)
{
  GList *list;

  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));
  g_return_if_fail (LIGMA_IS_CURVE (curve));

  for (list = view->bg_curves; list; list = g_list_next (list))
    {
      BGCurve *bg = list->data;

      if (bg->curve == curve)
        {
          g_signal_handlers_disconnect_by_func (bg->curve,
                                                ligma_curve_view_curve_dirty,
                                                view);
          g_object_unref (bg->curve);

          view->bg_curves = g_list_remove (view->bg_curves, bg);

          g_slice_free (BGCurve, bg);

          gtk_widget_queue_draw (GTK_WIDGET (view));

          break;
        }
    }

  if (! list)
    g_return_if_reached ();
}

void
ligma_curve_view_remove_all_backgrounds (LigmaCurveView *view)
{
  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));

  while (view->bg_curves)
    {
      BGCurve *bg = view->bg_curves->data;

      g_signal_handlers_disconnect_by_func (bg->curve,
                                            ligma_curve_view_curve_dirty,
                                            view);
      g_object_unref (bg->curve);

      view->bg_curves = g_list_remove (view->bg_curves, bg);

      g_slice_free (BGCurve, bg);
    }

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
ligma_curve_view_set_selected (LigmaCurveView *view,
                              gint           selected)
{
  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));

  if (selected != view->selected)
    {
      view->selected = selected;

      g_signal_emit (view, curve_view_signals[SELECTION_CHANGED], 0);

      gtk_widget_queue_draw (GTK_WIDGET (view));
    }
}

gint
ligma_curve_view_get_selected (LigmaCurveView *view)
{
  g_return_val_if_fail (LIGMA_IS_CURVE_VIEW (view), -1);

  if (view->curve && view->selected < ligma_curve_get_n_points (view->curve))
    return view->selected;
  else
    return -1;
}

void
ligma_curve_view_set_range_x (LigmaCurveView *view,
                             gdouble        min,
                             gdouble        max)
{
  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));

  view->range_x_min = min;
  view->range_x_max = max;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
ligma_curve_view_set_range_y (LigmaCurveView *view,
                             gdouble        min,
                             gdouble        max)
{
  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));

  view->range_y_min = min;
  view->range_y_max = max;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
ligma_curve_view_set_xpos (LigmaCurveView *view,
                          gdouble        x)
{
  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));

  view->xpos = x;

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
ligma_curve_view_set_x_axis_label (LigmaCurveView *view,
                                  const gchar   *label)
{
  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));

  if (view->x_axis_label)
    g_free (view->x_axis_label);

  view->x_axis_label = g_strdup (label);

  g_object_notify (G_OBJECT (view), "x-axis-label");

  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
ligma_curve_view_set_y_axis_label (LigmaCurveView *view,
                                  const gchar   *label)
{
  g_return_if_fail (LIGMA_IS_CURVE_VIEW (view));

  if (view->y_axis_label)
    g_free (view->y_axis_label);

  view->y_axis_label = g_strdup (label);

  g_object_notify (G_OBJECT (view), "y-axis-label");

  gtk_widget_queue_draw (GTK_WIDGET (view));
}


/*  private functions  */

static void
ligma_curve_view_set_cursor (LigmaCurveView *view,
                            gdouble        x,
                            gdouble        y)
{
  view->cursor_x = x;
  view->cursor_y = y;

  /* TODO: only invalidate the cursor label area */
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

static void
ligma_curve_view_unset_cursor (LigmaCurveView *view)
{
  view->cursor_x = -1.0;
  view->cursor_y = -1.0;

  /* TODO: only invalidate the cursor label area */
  gtk_widget_queue_draw (GTK_WIDGET (view));
}
