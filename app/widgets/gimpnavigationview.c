/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaNavigationView Widget
 * Copyright (C) 2001-2002 Michael Natterer <mitch@ligma.org>
 *
 * partly based on app/nav_window
 * Copyright (C) 1999 Andy Thomas <alt@ligma.org>
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

#include <math.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmamath/ligmamath.h"

#include "widgets-types.h"

#include "core/ligmaimage.h"
#include "core/ligmamarshal.h"

#include "display/ligmacanvas-style.h"

#include "ligmanavigationview.h"
#include "ligmaviewrenderer.h"
#include "ligmawidgets-utils.h"


#define BORDER_WIDTH 2


enum
{
  MARKER_CHANGED,
  ZOOM,
  SCROLL,
  LAST_SIGNAL
};


struct _LigmaNavigationView
{
  LigmaView     parent_instance;

  /*  values in image coordinates  */
  gdouble      center_x;
  gdouble      center_y;
  gdouble      width;
  gdouble      height;
  gboolean     flip_horizontally;
  gboolean     flip_vertically;
  gdouble      rotate_angle;

  gboolean     canvas_visible;
  gdouble      canvas_x;
  gdouble      canvas_y;
  gdouble      canvas_width;
  gdouble      canvas_height;

  /*  values in view coordinates  */
  gint         p_center_x;
  gint         p_center_y;
  gint         p_width;
  gint         p_height;

  gint         p_canvas_x;
  gint         p_canvas_y;
  gint         p_canvas_width;
  gint         p_canvas_height;

  gint         motion_offset_x;
  gint         motion_offset_y;
  gboolean     has_grab;
};


static void     ligma_navigation_view_size_allocate   (GtkWidget      *widget,
                                                      GtkAllocation  *allocation);
static gboolean ligma_navigation_view_draw            (GtkWidget      *widget,
                                                      cairo_t        *cr);
static gboolean ligma_navigation_view_button_press    (GtkWidget      *widget,
                                                      GdkEventButton *bevent);
static gboolean ligma_navigation_view_button_release  (GtkWidget      *widget,
                                                      GdkEventButton *bevent);
static gboolean ligma_navigation_view_scroll          (GtkWidget      *widget,
                                                      GdkEventScroll *sevent);
static gboolean ligma_navigation_view_motion_notify   (GtkWidget      *widget,
                                                      GdkEventMotion *mevent);
static gboolean ligma_navigation_view_key_press       (GtkWidget      *widget,
                                                      GdkEventKey    *kevent);

static void     ligma_navigation_view_transform       (LigmaNavigationView *nav_view);
static void     ligma_navigation_view_draw_marker     (LigmaNavigationView *nav_view,
                                                      cairo_t            *cr);
static void     ligma_navigation_view_move_to         (LigmaNavigationView *nav_view,
                                                      gint                tx,
                                                      gint                ty);
static void     ligma_navigation_view_get_ratio       (LigmaNavigationView *nav_view,
                                                      gdouble            *ratiox,
                                                      gdouble            *ratioy);
static gboolean ligma_navigation_view_point_in_marker (LigmaNavigationView *nav_view,
                                                      gint                x,
                                                      gint                y);


G_DEFINE_TYPE (LigmaNavigationView, ligma_navigation_view, LIGMA_TYPE_VIEW)

#define parent_class ligma_navigation_view_parent_class

static guint view_signals[LAST_SIGNAL] = { 0 };


static void
ligma_navigation_view_class_init (LigmaNavigationViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  view_signals[MARKER_CHANGED] =
    g_signal_new ("marker-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaNavigationViewClass, marker_changed),
                  NULL, NULL,
                  ligma_marshal_VOID__DOUBLE_DOUBLE_DOUBLE_DOUBLE,
                  G_TYPE_NONE, 4,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE);

  view_signals[ZOOM] =
    g_signal_new ("zoom",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaNavigationViewClass, zoom),
                  NULL, NULL,
                  ligma_marshal_VOID__ENUM_DOUBLE,
                  G_TYPE_NONE, 2,
                  LIGMA_TYPE_ZOOM_TYPE,
                  G_TYPE_DOUBLE);

  view_signals[SCROLL] =
    g_signal_new ("scroll",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaNavigationViewClass, scroll),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  GDK_TYPE_EVENT);

  widget_class->size_allocate        = ligma_navigation_view_size_allocate;
  widget_class->draw                 = ligma_navigation_view_draw;
  widget_class->button_press_event   = ligma_navigation_view_button_press;
  widget_class->button_release_event = ligma_navigation_view_button_release;
  widget_class->scroll_event         = ligma_navigation_view_scroll;
  widget_class->motion_notify_event  = ligma_navigation_view_motion_notify;
  widget_class->key_press_event      = ligma_navigation_view_key_press;
}

static void
ligma_navigation_view_init (LigmaNavigationView *view)
{
  gtk_widget_set_can_focus (GTK_WIDGET (view), TRUE);
  gtk_widget_add_events (GTK_WIDGET (view),
                         GDK_SCROLL_MASK         |
                         GDK_SMOOTH_SCROLL_MASK  |
                         GDK_POINTER_MOTION_MASK |
                         GDK_KEY_PRESS_MASK);

  view->center_x          = 0.0;
  view->center_y          = 0.0;
  view->width             = 0.0;
  view->height            = 0.0;
  view->flip_horizontally = FALSE;
  view->flip_vertically   = FALSE;
  view->rotate_angle      = 0.0;

  view->canvas_visible    = FALSE;
  view->canvas_x          = 0.0;
  view->canvas_y          = 0.0;
  view->canvas_width      = 0.0;
  view->canvas_height     = 0.0;

  view->p_center_x      = 0;
  view->p_center_y      = 0;
  view->p_width         = 0;
  view->p_height        = 0;

  view->p_canvas_x      = 0;
  view->p_canvas_y      = 0;
  view->p_canvas_width  = 0;
  view->p_canvas_height = 0;

  view->motion_offset_x = 0;
  view->motion_offset_y = 0;
  view->has_grab        = FALSE;
}

static void
ligma_navigation_view_size_allocate (GtkWidget     *widget,
                                    GtkAllocation *allocation)
{
  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (LIGMA_VIEW (widget)->renderer->viewable)
    ligma_navigation_view_transform (LIGMA_NAVIGATION_VIEW (widget));
}

static gboolean
ligma_navigation_view_draw (GtkWidget *widget,
                           cairo_t   *cr)
{
  GTK_WIDGET_CLASS (parent_class)->draw (widget, cr);

  ligma_navigation_view_draw_marker (LIGMA_NAVIGATION_VIEW (widget), cr);

  return TRUE;
}

void
ligma_navigation_view_grab_pointer (LigmaNavigationView *nav_view,
                                   GdkEvent           *event)
{
  GtkWidget  *widget = GTK_WIDGET (nav_view);
  GdkDisplay *display;
  GdkCursor  *cursor;
  GdkWindow  *window;

  nav_view->has_grab = TRUE;

  gtk_grab_add (widget);

  display = gtk_widget_get_display (widget);
  cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);

  window = LIGMA_VIEW (nav_view)->event_window;

  gdk_seat_grab (gdk_event_get_seat (event),
                 window,
                 GDK_SEAT_CAPABILITY_ALL,
                 FALSE, cursor, event, NULL, NULL);

  g_object_unref (cursor);
}

static gboolean
ligma_navigation_view_button_press (GtkWidget      *widget,
                                   GdkEventButton *bevent)
{
  LigmaNavigationView *nav_view = LIGMA_NAVIGATION_VIEW (widget);
  gint                tx, ty;
  GdkDisplay         *display;

  tx = bevent->x;
  ty = bevent->y;

  if (bevent->type == GDK_BUTTON_PRESS && bevent->button == 1)
    {
      if (! ligma_navigation_view_point_in_marker (nav_view, tx, ty))
        {
          GdkCursor *cursor;

          nav_view->motion_offset_x = 0;
          nav_view->motion_offset_y = 0;

          ligma_navigation_view_move_to (nav_view, tx, ty);

          display = gtk_widget_get_display (widget);
          cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
          gdk_window_set_cursor (LIGMA_VIEW (widget)->event_window, cursor);
          g_object_unref (cursor);
        }
      else
        {
          nav_view->motion_offset_x = tx - nav_view->p_center_x;
          nav_view->motion_offset_y = ty - nav_view->p_center_y;
        }

      ligma_navigation_view_grab_pointer (nav_view, (GdkEvent *) bevent);
    }

  return TRUE;
}

static gboolean
ligma_navigation_view_button_release (GtkWidget      *widget,
                                     GdkEventButton *bevent)
{
  LigmaNavigationView *nav_view = LIGMA_NAVIGATION_VIEW (widget);

  if (bevent->button == 1 && nav_view->has_grab)
    {
      nav_view->has_grab = FALSE;

      gtk_grab_remove (widget);
      gdk_seat_ungrab (gdk_event_get_seat ((GdkEvent *) bevent));
    }

  return TRUE;
}

static gboolean
ligma_navigation_view_scroll (GtkWidget      *widget,
                             GdkEventScroll *sevent)
{
  if (sevent->state & ligma_get_toggle_behavior_mask ())
    {
      gdouble delta;

      switch (sevent->direction)
        {
        case GDK_SCROLL_UP:
          g_signal_emit (widget, view_signals[ZOOM], 0, LIGMA_ZOOM_IN, 0.0);
          break;

        case GDK_SCROLL_DOWN:
          g_signal_emit (widget, view_signals[ZOOM], 0, LIGMA_ZOOM_OUT, 0.0);
          break;

        case GDK_SCROLL_SMOOTH:
          gdk_event_get_scroll_deltas ((GdkEvent *) sevent, NULL, &delta);
          g_signal_emit (widget, view_signals[ZOOM], 0, LIGMA_ZOOM_SMOOTH, delta);
          break;

        default:
          break;
        }
    }
  else
    {
      g_signal_emit (widget, view_signals[SCROLL], 0, sevent);
    }

  return TRUE;
}

static gboolean
ligma_navigation_view_motion_notify (GtkWidget      *widget,
                                    GdkEventMotion *mevent)
{
  LigmaNavigationView *nav_view = LIGMA_NAVIGATION_VIEW (widget);
  LigmaView           *view     = LIGMA_VIEW (widget);

  if (! nav_view->has_grab)
    {
      GdkDisplay *display = gtk_widget_get_display (widget);
      GdkCursor  *cursor;

      if (nav_view->p_center_x == view->renderer->width  / 2 &&
          nav_view->p_center_y == view->renderer->height / 2 &&
          nav_view->p_width    == view->renderer->width      &&
          nav_view->p_height   == view->renderer->height)
        {
          gdk_window_set_cursor (view->event_window, NULL);
          return FALSE;
        }
      else if (ligma_navigation_view_point_in_marker (nav_view,
                                                     mevent->x, mevent->y))
        {
          cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
        }
      else
        {
          cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
        }

      gdk_window_set_cursor (view->event_window, cursor);
      g_object_unref (cursor);

      return FALSE;
    }

  ligma_navigation_view_move_to (nav_view,
                                mevent->x - nav_view->motion_offset_x,
                                mevent->y - nav_view->motion_offset_y);

  gdk_event_request_motions (mevent);

  return TRUE;
}

static gboolean
ligma_navigation_view_key_press (GtkWidget   *widget,
                                GdkEventKey *kevent)
{
  LigmaNavigationView *nav_view = LIGMA_NAVIGATION_VIEW (widget);
  gint                scroll_x = 0;
  gint                scroll_y = 0;

  switch (kevent->keyval)
    {
    case GDK_KEY_Up:
      scroll_y = -1;
      break;

    case GDK_KEY_Left:
      scroll_x = -1;
      break;

    case GDK_KEY_Right:
      scroll_x = 1;
      break;

    case GDK_KEY_Down:
      scroll_y = 1;
      break;

    default:
      break;
    }

  if (scroll_x || scroll_y)
    {
      ligma_navigation_view_move_to (nav_view,
                                    nav_view->p_center_x + scroll_x,
                                    nav_view->p_center_y + scroll_y);
      return TRUE;
    }

  return FALSE;
}


/*  public functions  */

void
ligma_navigation_view_set_marker (LigmaNavigationView *nav_view,
                                 gdouble             center_x,
                                 gdouble             center_y,
                                 gdouble             width,
                                 gdouble             height,
                                 gboolean            flip_horizontally,
                                 gboolean            flip_vertically,
                                 gdouble             rotate_angle)
{
  LigmaView *view;

  g_return_if_fail (LIGMA_IS_NAVIGATION_VIEW (nav_view));

  view = LIGMA_VIEW (nav_view);

  g_return_if_fail (view->renderer->viewable);

  nav_view->center_x          = center_x;
  nav_view->center_y          = center_y;
  nav_view->width             = MAX (1.0, width);
  nav_view->height            = MAX (1.0, height);
  nav_view->flip_horizontally = flip_horizontally ? TRUE : FALSE;
  nav_view->flip_vertically   = flip_vertically   ? TRUE : FALSE;
  nav_view->rotate_angle      = rotate_angle;

  ligma_navigation_view_transform (nav_view);

  /* Marker changed, redraw */
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
ligma_navigation_view_set_canvas (LigmaNavigationView *nav_view,
                                 gboolean            visible,
                                 gdouble             x,
                                 gdouble             y,
                                 gdouble             width,
                                 gdouble             height)
{
  LigmaView *view;

  g_return_if_fail (LIGMA_IS_NAVIGATION_VIEW (nav_view));

  view = LIGMA_VIEW (nav_view);

  g_return_if_fail (view->renderer->viewable);

  nav_view->canvas_visible = visible;
  nav_view->canvas_x       = x;
  nav_view->canvas_y       = y;
  nav_view->canvas_width   = MAX (1.0, width);
  nav_view->canvas_height  = MAX (1.0, height);

  ligma_navigation_view_transform (nav_view);

  /* Marker changed, redraw */
  gtk_widget_queue_draw (GTK_WIDGET (view));
}

void
ligma_navigation_view_set_motion_offset (LigmaNavigationView *view,
                                        gint                motion_offset_x,
                                        gint                motion_offset_y)
{
  g_return_if_fail (LIGMA_IS_NAVIGATION_VIEW (view));

  view->motion_offset_x = motion_offset_x;
  view->motion_offset_y = motion_offset_y;
}

void
ligma_navigation_view_get_local_marker (LigmaNavigationView *view,
                                       gint               *center_x,
                                       gint               *center_y,
                                       gint               *width,
                                       gint               *height)
{
  g_return_if_fail (LIGMA_IS_NAVIGATION_VIEW (view));

  if (center_x) *center_x = view->p_center_x;
  if (center_y) *center_y = view->p_center_y;
  if (width)    *width    = view->p_width;
  if (height)   *height   = view->p_height;
}


/*  private functions  */

static void
ligma_navigation_view_transform (LigmaNavigationView *nav_view)
{
  gdouble ratiox, ratioy;

  ligma_navigation_view_get_ratio (nav_view, &ratiox, &ratioy);

  nav_view->p_center_x = RINT (nav_view->center_x * ratiox);
  nav_view->p_center_y = RINT (nav_view->center_y * ratioy);

  nav_view->p_width  = ceil (nav_view->width  * ratiox);
  nav_view->p_height = ceil (nav_view->height * ratioy);

  nav_view->p_canvas_x = RINT (nav_view->canvas_x * ratiox);
  nav_view->p_canvas_y = RINT (nav_view->canvas_y * ratioy);

  nav_view->p_canvas_width  = ceil (nav_view->canvas_width  * ratiox);
  nav_view->p_canvas_height = ceil (nav_view->canvas_height * ratioy);
}

static void
ligma_navigation_view_draw_marker (LigmaNavigationView *nav_view,
                                  cairo_t            *cr)
{
  LigmaView *view = LIGMA_VIEW (nav_view);

  if (view->renderer->viewable && nav_view->width && nav_view->height)
    {
      GtkWidget      *widget = GTK_WIDGET (view);
      GtkAllocation   allocation;
      cairo_matrix_t  matrix;
      gint            p_width_2;
      gint            p_height_2;
      gdouble         angle;

      p_width_2  = nav_view->p_width  / 2;
      p_height_2 = nav_view->p_height / 2;

      angle = G_PI * nav_view->rotate_angle / 180.0;
      if (nav_view->flip_horizontally != nav_view->flip_vertically)
        angle = -angle;

      gtk_widget_get_allocation (widget, &allocation);

      cairo_get_matrix (cr, &matrix);

      cairo_rectangle (cr,
                       0, 0,
                       allocation.width, allocation.height);
      cairo_translate (cr, nav_view->p_center_x, nav_view->p_center_y);
      cairo_rotate (cr, -angle);
      cairo_rectangle (cr,
                       -p_width_2, -p_height_2,
                       nav_view->p_width, nav_view->p_height);

      cairo_set_source_rgba (cr, 0, 0, 0, 0.5);
      cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_fill (cr);

      if (nav_view->canvas_visible &&
          nav_view->canvas_width && nav_view->canvas_height)
        {
          cairo_save (cr);
          cairo_set_matrix (cr, &matrix);
          cairo_rectangle (cr,
                           nav_view->p_canvas_x      + 0.5,
                           nav_view->p_canvas_y      + 0.5,
                           nav_view->p_canvas_width  - 1.0,
                           nav_view->p_canvas_height - 1.0);
          ligma_canvas_set_canvas_style (GTK_WIDGET (nav_view), cr, 0, 0);
          cairo_stroke (cr);
          cairo_restore (cr);
        }

      cairo_rectangle (cr,
                       -p_width_2, -p_height_2,
                       nav_view->p_width, nav_view->p_height);

      cairo_set_source_rgb (cr, 1, 1, 1);
      cairo_set_line_width (cr, BORDER_WIDTH);
      cairo_stroke (cr);
    }
}

static void
ligma_navigation_view_move_to (LigmaNavigationView *nav_view,
                              gint                tx,
                              gint                ty)
{
  LigmaView  *view = LIGMA_VIEW (nav_view);
  gdouble    ratiox, ratioy;
  gdouble    x, y;

  if (! view->renderer->viewable)
    return;

  ligma_navigation_view_get_ratio (nav_view, &ratiox, &ratioy);

  x = tx / ratiox;
  y = ty / ratioy;

  g_signal_emit (view, view_signals[MARKER_CHANGED], 0,
                 x, y, nav_view->width, nav_view->height);
}

static void
ligma_navigation_view_get_ratio (LigmaNavigationView *nav_view,
                                gdouble            *ratiox,
                                gdouble            *ratioy)
{
  LigmaView  *view = LIGMA_VIEW (nav_view);
  gint       width;
  gint       height;

  ligma_viewable_get_size (view->renderer->viewable, &width, &height);

  *ratiox = (gdouble) view->renderer->width  / (gdouble) width;
  *ratioy = (gdouble) view->renderer->height / (gdouble) height;
}

static gboolean
ligma_navigation_view_point_in_marker (LigmaNavigationView *nav_view,
                                      gint                x,
                                      gint                y)
{
  gint    p_width_2, p_height_2;
  gdouble angle;
  gdouble tx, ty;

  p_width_2  = nav_view->p_width  / 2;
  p_height_2 = nav_view->p_height / 2;

  angle = G_PI * nav_view->rotate_angle / 180.0;
  if (nav_view->flip_horizontally != nav_view->flip_vertically)
    angle = -angle;

  x -= nav_view->p_center_x;
  y -= nav_view->p_center_y;

  tx = cos (angle) * x - sin (angle) * y;
  ty = sin (angle) * x + cos (angle) * y;

  return tx >= -p_width_2  && tx < p_width_2 &&
         ty >= -p_height_2 && ty < p_height_2;
}
