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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "ligma.h"
#include "ligmacontext.h"
#include "ligmadrawable.h"
#include "ligmadrawable-offset.h"
#include "ligmadrawable-operation.h"

#include "ligma-intl.h"


void
ligma_drawable_offset (LigmaDrawable   *drawable,
                      LigmaContext    *context,
                      gboolean        wrap_around,
                      LigmaOffsetType  fill_type,
                      gint            offset_x,
                      gint            offset_y)
{
  GeglNode *node;
  gint      width;
  gint      height;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable),
                                  NULL, NULL, &width, &height))
    {
      return;
    }

  if (wrap_around)
    fill_type = LIGMA_OFFSET_WRAP_AROUND;

  if (fill_type == LIGMA_OFFSET_WRAP_AROUND)
    {
      offset_x %= width;
      offset_y %= height;
    }

  if (offset_x == 0 && offset_y == 0)
    return;

  node = gegl_node_new_child (NULL,
                              "operation", "ligma:offset",
                              "context", context,
                              "type",    fill_type,
                              "x",       offset_x,
                              "y",       offset_y,
                              NULL);

  ligma_drawable_apply_operation (drawable, NULL,
                                 C_("undo-type", "Offset Drawable"),
                                 node);

  g_object_unref (node);
}
