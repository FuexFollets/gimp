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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"

#include "core-types.h"

#include "ligmapalette.h"
#include "ligmapalette-save.h"

#include "ligma-intl.h"


gboolean
ligma_palette_save (LigmaData       *data,
                   GOutputStream  *output,
                   GError        **error)
{
  LigmaPalette *palette = LIGMA_PALETTE (data);
  GString     *string;
  GList       *list;

  string = g_string_new ("LIGMA Palette\n");

  g_string_append_printf (string,
                          "Name: %s\n"
                          "Columns: %d\n"
                          "#\n",
                          ligma_object_get_name (palette),
                          CLAMP (ligma_palette_get_columns (palette), 0, 256));

  for (list = ligma_palette_get_colors (palette);
       list;
       list = g_list_next (list))
    {
      LigmaPaletteEntry *entry = list->data;
      guchar            r, g, b;

      ligma_rgb_get_uchar (&entry->color, &r, &g, &b);

      g_string_append_printf (string, "%3d %3d %3d\t%s\n",
                              r, g, b, entry->name);
    }

  if (! g_output_stream_write_all (output, string->str, string->len,
                                   NULL, NULL, error))
    {
      g_string_free (string, TRUE);

      return FALSE;
    }

  g_string_free (string, TRUE);

  return TRUE;
}
