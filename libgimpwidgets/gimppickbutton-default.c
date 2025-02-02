/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmapickbutton.c
 * Copyright (C) 2002 Michael Natterer <mitch@ligma.org>
 *
 * based on gtk+/gtk/gtkcolorsel.c
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
#include <gdk/gdkkeysyms.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "libligmacolor/ligmacolor.h"

#include "ligmawidgetstypes.h"

#include "ligmacairo-utils.h"
#include "ligmahelpui.h"
#include "ligmaicons.h"
#include "ligmapickbutton.h"
#include "ligmapickbutton-default.h"
#include "ligmapickbutton-private.h"
#include "ligmawidgetsutils.h"

#include "libligma/libligma-intl.h"


static gboolean   ligma_pick_button_mouse_press   (GtkWidget      *invisible,
                                                  GdkEventButton *event,
                                                  LigmaPickButton *button);
static gboolean   ligma_pick_button_key_press     (GtkWidget      *invisible,
                                                  GdkEventKey    *event,
                                                  LigmaPickButton *button);
static gboolean   ligma_pick_button_mouse_motion  (GtkWidget      *invisible,
                                                  GdkEventMotion *event,
                                                  LigmaPickButton *button);
static gboolean   ligma_pick_button_mouse_release (GtkWidget      *invisible,
                                                  GdkEventButton *event,
                                                  LigmaPickButton *button);
static void       ligma_pick_button_shutdown      (LigmaPickButton *button);
static void       ligma_pick_button_pick          (LigmaPickButton *button,
                                                  GdkEvent       *event);


static GdkCursor *
make_cursor (GdkDisplay *display)
{
  GdkPixbuf *pixbuf;
  GError    *error = NULL;

  pixbuf = gdk_pixbuf_new_from_resource ("/org/ligma/color-picker-cursors/cursor-color-picker.png",
                                         &error);

  if (pixbuf)
    {
      GdkCursor *cursor = gdk_cursor_new_from_pixbuf (display, pixbuf, 1, 30);

      g_object_unref (pixbuf);

      return cursor;
    }
  else
    {
      g_critical ("Failed to create cursor image: %s", error->message);
      g_clear_error (&error);
    }

  return NULL;
}

static gboolean
ligma_pick_button_mouse_press (GtkWidget      *invisible,
                              GdkEventButton *event,
                              LigmaPickButton *button)
{
  if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
      g_signal_connect (invisible, "motion-notify-event",
                        G_CALLBACK (ligma_pick_button_mouse_motion),
                        button);
      g_signal_connect (invisible, "button-release-event",
                        G_CALLBACK (ligma_pick_button_mouse_release),
                        button);

      g_signal_handlers_disconnect_by_func (invisible,
                                            ligma_pick_button_mouse_press,
                                            button);
      g_signal_handlers_disconnect_by_func (invisible,
                                            ligma_pick_button_key_press,
                                            button);

      ligma_pick_button_pick (button, (GdkEvent *) event);

      return TRUE;
    }

  return FALSE;
}

static gboolean
ligma_pick_button_key_press (GtkWidget      *invisible,
                            GdkEventKey    *event,
                            LigmaPickButton *button)
{
  if (event->keyval == GDK_KEY_Escape)
    {
      ligma_pick_button_shutdown (button);

      g_signal_handlers_disconnect_by_func (invisible,
                                            ligma_pick_button_mouse_press,
                                            button);
      g_signal_handlers_disconnect_by_func (invisible,
                                            ligma_pick_button_key_press,
                                            button);

      return TRUE;
    }

  return FALSE;
}

static gboolean
ligma_pick_button_mouse_motion (GtkWidget      *invisible,
                               GdkEventMotion *event,
                               LigmaPickButton *button)
{
  ligma_pick_button_pick (button, (GdkEvent *) event);

  return TRUE;
}

static gboolean
ligma_pick_button_mouse_release (GtkWidget      *invisible,
                                GdkEventButton *event,
                                LigmaPickButton *button)
{
  if (event->button != 1)
    return FALSE;

  ligma_pick_button_pick (button, (GdkEvent *) event);

  ligma_pick_button_shutdown (button);

  g_signal_handlers_disconnect_by_func (invisible,
                                        ligma_pick_button_mouse_motion,
                                        button);
  g_signal_handlers_disconnect_by_func (invisible,
                                        ligma_pick_button_mouse_release,
                                        button);

  return TRUE;
}

static void
ligma_pick_button_shutdown (LigmaPickButton *button)
{
  GdkDisplay *display = gtk_widget_get_display (button->priv->grab_widget);

  gtk_grab_remove (button->priv->grab_widget);

  gdk_seat_ungrab (gdk_display_get_default_seat (display));
}

static void
ligma_pick_button_pick (LigmaPickButton *button,
                       GdkEvent       *event)
{
  GdkScreen        *screen = gdk_event_get_screen (event);
  LigmaColorProfile *monitor_profile;
  GdkMonitor       *monitor;
  LigmaRGB           rgb;
  gint              x_root;
  gint              y_root;
  gdouble           x_win;
  gdouble           y_win;

  gdk_window_get_origin (gdk_event_get_window (event), &x_root, &y_root);
  gdk_event_get_coords (event, &x_win, &y_win);
  x_root += x_win;
  y_root += y_win;

#ifdef G_OS_WIN32

  {
    HDC      hdc;
    RECT     rect;
    COLORREF win32_color;

    /* For MS Windows, use native GDI functions to get the pixel, as
     * cairo does not handle the case where you have multiple monitors
     * with a monitor on the left or above the primary monitor.  That
     * scenario create a cairo primary surface with negative extent,
     * which is not handled properly (bug 740634).
     */

    hdc = GetDC (HWND_DESKTOP);
    GetClipBox (hdc, &rect);
    win32_color = GetPixel (hdc, x_root + rect.left, y_root + rect.top);
    ReleaseDC (HWND_DESKTOP, hdc);

    ligma_rgba_set_uchar (&rgb,
                         GetRValue (win32_color),
                         GetGValue (win32_color),
                         GetBValue (win32_color),
                         255);
  }

#else

  {
    GdkWindow       *window;
    gint             x_window;
    gint             y_window;
    cairo_surface_t *image;
    cairo_t         *cr;
    guchar          *data;
    guchar           color[3];

    /* we try to pick from the local window under the cursor, and fall
     * back to picking from the root window if this fails (i.e., if
     * the cursor is not under a local window).  on wayland, picking
     * from the root window is not supported, so this at least allows
     * us to pick from local windows.  see bug #780375.
     */
    window = gdk_device_get_window_at_position (gdk_event_get_device (event),
                                                &x_window, &y_window);
    if (! window)
      {
        window   = gdk_screen_get_root_window (screen);
        x_window = x_root;
        y_window = y_root;
      }

    image = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 1, 1);

    cr = cairo_create (image);

    gdk_cairo_set_source_window (cr, window, -x_window, -y_window);
    cairo_paint (cr);

    cairo_destroy (cr);

    data = cairo_image_surface_get_data (image);
    LIGMA_CAIRO_RGB24_GET_PIXEL (data, color[0], color[1], color[2]);

    cairo_surface_destroy (image);

    ligma_rgba_set_uchar (&rgb, color[0], color[1], color[2], 255);
  }

#endif

  monitor = gdk_display_get_monitor_at_point (gdk_screen_get_display (screen),
                                              x_root, y_root);
  monitor_profile = ligma_monitor_get_color_profile (monitor);

  if (monitor_profile)
    {
      LigmaColorProfile        *srgb_profile;
      LigmaColorTransform      *transform;
      const Babl              *format;
      LigmaColorTransformFlags  flags = 0;

      format = babl_format ("R'G'B'A double");

      flags |= LIGMA_COLOR_TRANSFORM_FLAGS_NOOPTIMIZE;
      flags |= LIGMA_COLOR_TRANSFORM_FLAGS_BLACK_POINT_COMPENSATION;

      srgb_profile = ligma_color_profile_new_rgb_srgb ();
      transform = ligma_color_transform_new (monitor_profile, format,
                                            srgb_profile,    format,
                                            LIGMA_COLOR_RENDERING_INTENT_PERCEPTUAL,
                                            flags);
      g_object_unref (srgb_profile);

      if (transform)
        {
          ligma_color_transform_process_pixels (transform,
                                               format, &rgb,
                                               format, &rgb,
                                               1);
          ligma_rgb_clamp (&rgb);

          g_object_unref (transform);
        }
    }

  g_signal_emit_by_name (button, "color-picked", &rgb);
}

/* entry point to this file, called from ligmapickbutton.c */
void
_ligma_pick_button_default_pick (LigmaPickButton *button)
{
  GdkDisplay *display;
  GtkWidget  *widget;

  if (! button->priv->cursor)
    button->priv->cursor =
      make_cursor (gtk_widget_get_display (GTK_WIDGET (button)));

  if (! button->priv->grab_widget)
    {
      button->priv->grab_widget = gtk_invisible_new ();

      gtk_widget_add_events (button->priv->grab_widget,
                             GDK_BUTTON_PRESS_MASK   |
                             GDK_BUTTON_RELEASE_MASK |
                             GDK_BUTTON1_MOTION_MASK);

      gtk_widget_show (button->priv->grab_widget);
    }

  widget = button->priv->grab_widget;

  display = gtk_widget_get_display (widget);

  if (gdk_seat_grab (gdk_display_get_default_seat (display),
                     gtk_widget_get_window (widget),
                     GDK_SEAT_CAPABILITY_ALL,
                     FALSE,
                     button->priv->cursor,
                     NULL,
                     NULL, NULL) != GDK_GRAB_SUCCESS)
    {
      g_warning ("Failed to grab seat to do eyedropper");
      return;
    }

  gtk_grab_add (widget);

  g_signal_connect (widget, "button-press-event",
                    G_CALLBACK (ligma_pick_button_mouse_press),
                    button);
  g_signal_connect (widget, "key-press-event",
                    G_CALLBACK (ligma_pick_button_key_press),
                    button);
}
