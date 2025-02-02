/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaText
 * Copyright (C) 2002-2003  Sven Neumann <sven@ligma.org>
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

#include <pango/pangocairo.h>

#include "text-types.h"

#include "ligmatextlayout.h"
#include "ligmatextlayout-render.h"


void
ligma_text_layout_render (LigmaTextLayout    *layout,
                         cairo_t           *cr,
                         LigmaTextDirection  base_dir,
                         gboolean           path)
{
  PangoLayout    *pango_layout;
  cairo_matrix_t  trafo;
  gint            x, y;
  gint            width, height;

  g_return_if_fail (LIGMA_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (cr != NULL);

  cairo_save (cr);

  ligma_text_layout_get_offsets (layout, &x, &y);
  cairo_translate (cr, x, y);

  ligma_text_layout_get_transform (layout, &trafo);
  cairo_transform (cr, &trafo);

  if (base_dir == LIGMA_TEXT_DIRECTION_TTB_RTL ||
      base_dir == LIGMA_TEXT_DIRECTION_TTB_RTL_UPRIGHT)
    {
      ligma_text_layout_get_size (layout, &width, &height);
      cairo_translate (cr, width, 0);
      cairo_rotate (cr, G_PI_2);
    }

  if (base_dir == LIGMA_TEXT_DIRECTION_TTB_LTR ||
      base_dir == LIGMA_TEXT_DIRECTION_TTB_LTR_UPRIGHT)
    {
      ligma_text_layout_get_size (layout, &width, &height);
      cairo_translate (cr, 0, height);
      cairo_rotate (cr, -G_PI_2);
    }

  pango_layout = ligma_text_layout_get_pango_layout (layout);

  if (path)
    pango_cairo_layout_path (cr, pango_layout);
  else
    pango_cairo_show_layout (cr, pango_layout);

  cairo_restore (cr);
}
