/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Screenshot plug-in
 * Copyright 1998-2007 Sven Neumann <sven@ligma.org>
 * Copyright 2003      Henrik Brix Andersen <brix@ligma.org>
 * Copyright 2012      Simone Karin Lehmann - OS X patches
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

#ifdef PLATFORM_OSX

#include <stdlib.h> /* for system() on OSX */
#include <string.h>

#include <glib.h>
#include <glib/gstdio.h> /* g_unlink() */

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "screenshot.h"
#include "screenshot-osx.h"


/*
 * Mac OS X uses a rootless X server. This won't let us use
 * gdk_pixbuf_get_from_drawable() and similar function on the root
 * window to get the entire screen contents. With a native OS X build
 * we have to do this without X as well.
 *
 * Since Mac OS X 10.2 a system utility for screencapturing is
 * included. We can safely use this, since it's available on every OS
 * X version LIGMA is running on.
 *
 * The main drawbacks are that it's not possible to shoot windows or
 * regions in scripts in noninteractive mode, and that windows always
 * include decorations, since decorations are different between X11
 * windows and native OS X app windows. But we can use this switch
 * to capture the shadow of a window, which is indeed very Mac-ish.
 *
 * This routines works well with X11 and as a native build.
 */

gboolean
screenshot_osx_available (void)
{
  return TRUE;
}

ScreenshotCapabilities
screenshot_osx_get_capabilities (void)
{
  return (SCREENSHOT_CAN_SHOOT_DECORATIONS |
          SCREENSHOT_CAN_SHOOT_POINTER     |
          SCREENSHOT_CAN_SHOOT_REGION      |
          SCREENSHOT_CAN_SHOOT_WINDOW      |
          SCREENSHOT_CAN_PICK_WINDOW       |
          SCREENSHOT_CAN_DELAY_WINDOW_SHOT);
}

LigmaPDBStatusType
screenshot_osx_shoot (ScreenshotValues  *shootvals,
                      GdkScreen         *screen,
                      LigmaImage        **image,
                      GError           **error)
{
  const gchar *mode    = " ";
  const gchar *cursor  = " ";
  gchar       *delay   = NULL;
  GFile       *tmpfile;
  gchar       *quoted;
  gchar       *command = NULL;

  switch (shootvals->shoot_type)
    {
    case SHOOT_REGION:
      if (shootvals->select_delay > 0)
        screenshot_delay (shootvals->select_delay);

      mode = "-is";
      break;

    case SHOOT_WINDOW:
      if (shootvals->select_delay > 0)
        screenshot_delay (shootvals->select_delay);

      if (shootvals->decorate)
        mode = "-iwo";
      else
        mode = "-iw";
      if (shootvals->show_cursor)
        cursor = "-C";
      break;

    case SHOOT_ROOT:
      mode = " ";
      if (shootvals->show_cursor)
        cursor = "-C";
      break;

    default:
      g_return_val_if_reached (LIGMA_PDB_CALLING_ERROR);
      break;
    }

  delay = g_strdup_printf ("-T %i", shootvals->screenshot_delay);

  tmpfile = ligma_temp_file ("png");
  quoted  = g_shell_quote (g_file_peek_path (tmpfile));

  command = g_strjoin (" ",
                       "/usr/sbin/screencapture",
                       mode,
                       cursor,
                       delay,
                       quoted,
                       NULL);

  g_free (quoted);
  g_free (delay);

  if (system ((const char *) command) == EXIT_SUCCESS)
    {
      /* don't attach a profile, screencapture attached one
       */

      *image = ligma_file_load (LIGMA_RUN_NONINTERACTIVE,
                               tmpfile);
      ligma_image_set_file (*image, g_file_new_for_uri ("screenshot.png"));

      g_file_delete (tmpfile, NULL, NULL);
      g_free (command);

      return LIGMA_PDB_SUCCESS;
   }

  g_free (command);

  return LIGMA_PDB_EXECUTION_ERROR;
}

#endif /* PLATFORM_OSX */
