/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadisplayshell-grab.c
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

#include "display-types.h"

#include "widgets/ligmadevices.h"

#include "ligmadisplay.h"
#include "ligmadisplayshell.h"
#include "ligmadisplayshell-grab.h"


static GdkDevice *
get_associated_pointer (GdkDevice *device)
{
  switch (gdk_device_get_device_type (device))
    {
    case GDK_DEVICE_TYPE_SLAVE:
      device = gdk_device_get_associated_device (device);
      break;

    case GDK_DEVICE_TYPE_FLOATING:
      {
        GdkDisplay *display = gdk_device_get_display (device);
        GdkSeat    *seat    = gdk_display_get_default_seat (display);

        return gdk_seat_get_pointer (seat);
      }
      break;

    default:
      break;
    }

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    device = gdk_device_get_associated_device (device);

  return device;
}

gboolean
ligma_display_shell_pointer_grab (LigmaDisplayShell *shell,
                                 const GdkEvent   *event,
                                 GdkEventMask      event_mask)
{
  GdkDevice     *device;
  GdkDevice     *source_device;
  GdkGrabStatus  status;

  g_return_val_if_fail (LIGMA_IS_DISPLAY_SHELL (shell), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (shell->grab_pointer == NULL, FALSE);

  source_device = ligma_devices_get_from_event (shell->display->ligma,
                                               event, &device);

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    {
      device        = get_associated_pointer (device);
      source_device = NULL;
    }

  status = gdk_device_grab (device,
                            gtk_widget_get_window (shell->canvas),
                            GDK_OWNERSHIP_APPLICATION,
                            FALSE, event_mask, NULL,
                            gdk_event_get_time (event));

  if (status == GDK_GRAB_SUCCESS)
    {
      shell->grab_pointer        = device;
      shell->grab_pointer_source = source_device;
      shell->grab_pointer_time   = gdk_event_get_time (event);

      return TRUE;
    }

  g_printerr ("%s: gdk_device_grab(%s) failed with status %d\n",
              G_STRFUNC, gdk_device_get_name (device), status);

  return FALSE;
}

void
ligma_display_shell_pointer_ungrab (LigmaDisplayShell *shell,
                                   const GdkEvent   *event)
{
  g_return_if_fail (LIGMA_IS_DISPLAY_SHELL (shell));
  g_return_if_fail (event != NULL);
  g_return_if_fail (shell->grab_pointer != NULL);

  gdk_device_ungrab (shell->grab_pointer, shell->grab_pointer_time);

  shell->grab_pointer        = NULL;
  shell->grab_pointer_source = NULL;
  shell->grab_pointer_time   = 0;
}
