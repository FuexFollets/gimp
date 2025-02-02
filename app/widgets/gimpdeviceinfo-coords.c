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

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"

#include "ligmadeviceinfo.h"
#include "ligmadeviceinfo-coords.h"


static const LigmaCoords default_coords = LIGMA_COORDS_DEFAULT_VALUES;


/*  public functions  */

gboolean
ligma_device_info_get_event_coords (LigmaDeviceInfo *info,
                                   GdkWindow      *window,
                                   const GdkEvent *event,
                                   LigmaCoords     *coords)
{
  gdouble x;

  if (event && gdk_event_get_axis (event, GDK_AXIS_X, &x))
    {
      *coords = default_coords;

      coords->x = x;
      gdk_event_get_axis (event, GDK_AXIS_Y, &coords->y);

      /* translate event coordinates to window coordinates, only
       * happens if we drag a guide from a ruler
       */
      if (event->any.window &&
          event->any.window != window)
        {
          GtkWidget *src_widget;
          GtkWidget *dest_widget;

          src_widget = gtk_get_event_widget ((GdkEvent *) event);
          gdk_window_get_user_data (window, (gpointer) &dest_widget);

          if (src_widget && dest_widget)
            {
              gint offset_x;
              gint offset_y;

              if (gtk_widget_translate_coordinates (src_widget, dest_widget,
                                                    0, 0,
                                                    &offset_x, &offset_y))
                {
                  coords->x += offset_x;
                  coords->y += offset_y;
                }
            }
        }

      if (gdk_event_get_axis (event, GDK_AXIS_PRESSURE, &coords->pressure))
        {
          coords->pressure = ligma_device_info_map_axis (info,
                                                        GDK_AXIS_PRESSURE,
                                                        coords->pressure);
        }

      if (gdk_event_get_axis (event, GDK_AXIS_XTILT, &coords->xtilt))
        {
          coords->xtilt = ligma_device_info_map_axis (info,
                                                     GDK_AXIS_XTILT,
                                                     coords->xtilt);
        }

      if (gdk_event_get_axis (event, GDK_AXIS_YTILT, &coords->ytilt))
        {
          coords->ytilt = ligma_device_info_map_axis (info,
                                                     GDK_AXIS_YTILT,
                                                     coords->ytilt);
        }

      if (gdk_event_get_axis (event, GDK_AXIS_WHEEL, &coords->wheel))
        {
          coords->wheel = ligma_device_info_map_axis (info,
                                                     GDK_AXIS_WHEEL,
                                                     coords->wheel);
        }

      if (gdk_event_get_axis (event, GDK_AXIS_DISTANCE, &coords->distance))
        {
          coords->distance = ligma_device_info_map_axis (info,
                                                        GDK_AXIS_DISTANCE,
                                                        coords->distance);
        }

      if (gdk_event_get_axis (event, GDK_AXIS_ROTATION, &coords->rotation))
        {
          coords->rotation = ligma_device_info_map_axis (info,
                                                        GDK_AXIS_ROTATION,
                                                        coords->rotation);
        }

      if (gdk_event_get_axis (event, GDK_AXIS_SLIDER, &coords->slider))
        {
          coords->slider = ligma_device_info_map_axis (info,
                                                      GDK_AXIS_SLIDER,
                                                      coords->slider);
        }

      return TRUE;
    }

  ligma_device_info_get_device_coords (info, window, coords);

  return FALSE;
}

void
ligma_device_info_get_device_coords (LigmaDeviceInfo *info,
                                    GdkWindow      *window,
                                    LigmaCoords     *coords)
{
  GdkDevice *device = ligma_device_info_get_device (info, NULL);
  gdouble    axes[GDK_AXIS_LAST] = { 0, };

  if (gdk_device_get_device_type (device) == GDK_DEVICE_TYPE_SLAVE)
    device = gdk_device_get_associated_device (device);

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    device = gdk_device_get_associated_device (device);

  *coords = default_coords;

  gdk_device_get_state (device, window, axes, NULL);

#if 0
  gdk_device_get_axis (device, axes, GDK_AXIS_X, &coords->x);
  gdk_device_get_axis (device, axes, GDK_AXIS_Y, &coords->y);
#else
  {
    gint x, y;

    gdk_window_get_device_position (window, device, &x, &y, NULL);

    coords->x = x;
    coords->y = y;
  }
#endif

  if (gdk_device_get_axis (device,
                           axes, GDK_AXIS_PRESSURE, &coords->pressure))
    {
      coords->pressure = ligma_device_info_map_axis (info,
                                                    GDK_AXIS_PRESSURE,
                                                    coords->pressure);
    }

  if (gdk_device_get_axis (device,
                           axes, GDK_AXIS_XTILT, &coords->xtilt))
    {
      coords->xtilt = ligma_device_info_map_axis (info,
                                                 GDK_AXIS_XTILT,
                                                 coords->xtilt);
    }

  if (gdk_device_get_axis (device,
                           axes, GDK_AXIS_YTILT, &coords->ytilt))
    {
      coords->ytilt = ligma_device_info_map_axis (info,
                                                 GDK_AXIS_YTILT,
                                                 coords->ytilt);
    }

  if (gdk_device_get_axis (device,
                           axes, GDK_AXIS_WHEEL, &coords->wheel))
    {
      coords->wheel = ligma_device_info_map_axis (info,
                                                 GDK_AXIS_WHEEL,
                                                 coords->wheel);
    }

  if (gdk_device_get_axis (device,
                           axes, GDK_AXIS_DISTANCE, &coords->distance))
    {
      coords->distance = ligma_device_info_map_axis (info,
                                                    GDK_AXIS_DISTANCE,
                                                    coords->distance);
    }

  if (gdk_device_get_axis (device,
                           axes, GDK_AXIS_ROTATION, &coords->rotation))
    {
      coords->rotation = ligma_device_info_map_axis (info,
                                                    GDK_AXIS_ROTATION,
                                                    coords->rotation);
    }

  if (gdk_device_get_axis (device,
                           axes, GDK_AXIS_SLIDER, &coords->slider))
    {
      coords->slider = ligma_device_info_map_axis (info,
                                                  GDK_AXIS_SLIDER,
                                                  coords->slider);
    }
}

void
ligma_device_info_get_time_coords (LigmaDeviceInfo *info,
                                  GdkTimeCoord   *event,
                                  LigmaCoords     *coords)
{
  GdkDevice *device = ligma_device_info_get_device (info, NULL);

  *coords = default_coords;

  gdk_device_get_axis (device, event->axes, GDK_AXIS_X, &coords->x);
  gdk_device_get_axis (device, event->axes, GDK_AXIS_Y, &coords->y);

  /*  CLAMP() the return value of each *_get_axis() call to be safe
   *  against buggy XInput drivers.
   */

  if (gdk_device_get_axis (device,
                           event->axes, GDK_AXIS_PRESSURE, &coords->pressure))
    {
      coords->pressure = ligma_device_info_map_axis (info,
                                                    GDK_AXIS_PRESSURE,
                                                    coords->pressure);
    }

  if (gdk_device_get_axis (device,
                           event->axes, GDK_AXIS_XTILT, &coords->xtilt))
    {
      coords->xtilt = ligma_device_info_map_axis (info,
                                                 GDK_AXIS_XTILT,
                                                 coords->xtilt);
    }

  if (gdk_device_get_axis (device,
                           event->axes, GDK_AXIS_YTILT, &coords->ytilt))
    {
      coords->ytilt = ligma_device_info_map_axis (info,
                                                 GDK_AXIS_YTILT,
                                                 coords->ytilt);
    }

  if (gdk_device_get_axis (device,
                           event->axes, GDK_AXIS_WHEEL, &coords->wheel))
    {
      coords->wheel = ligma_device_info_map_axis (info,
                                                 GDK_AXIS_WHEEL,
                                                 coords->wheel);
    }

  if (gdk_device_get_axis (device,
                           event->axes, GDK_AXIS_DISTANCE, &coords->distance))
    {
      coords->distance = ligma_device_info_map_axis (info,
                                                    GDK_AXIS_DISTANCE,
                                                    coords->distance);
    }

  if (gdk_device_get_axis (device,
                           event->axes, GDK_AXIS_ROTATION, &coords->rotation))
    {
      coords->rotation = ligma_device_info_map_axis (info,
                                                    GDK_AXIS_ROTATION,
                                                    coords->rotation);
    }

  if (gdk_device_get_axis (device,
                           event->axes, GDK_AXIS_SLIDER, &coords->slider))
    {
      coords->slider = ligma_device_info_map_axis (info,
                                                  GDK_AXIS_SLIDER,
                                                  coords->slider);
    }
}

gboolean
ligma_device_info_get_event_state (LigmaDeviceInfo  *info,
                                  GdkWindow       *window,
                                  const GdkEvent  *event,
                                  GdkModifierType *state)
{
  if (gdk_event_get_state (event, state))
    return TRUE;

  ligma_device_info_get_device_state (info, window, state);

  return FALSE;
}

void
ligma_device_info_get_device_state (LigmaDeviceInfo  *info,
                                   GdkWindow       *window,
                                   GdkModifierType *state)
{
  GdkDevice *device = ligma_device_info_get_device (info, NULL);

  switch (gdk_device_get_device_type (device))
    {
    case GDK_DEVICE_TYPE_SLAVE:
      device = gdk_device_get_associated_device (device);
      break;

    case GDK_DEVICE_TYPE_FLOATING:
      {
        GdkDisplay *display = gdk_device_get_display (device);
        GdkSeat    *seat    = gdk_display_get_default_seat (display);

        device = gdk_seat_get_pointer (seat);
      }
      break;

    default:
      break;
    }

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    device = gdk_device_get_associated_device (device);

  gdk_device_get_state (device, window, NULL, state);
}
