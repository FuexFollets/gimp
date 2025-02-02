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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "operations/ligmalevelsconfig.h"

#include "ligmadrawable.h"
#include "ligmadrawable-histogram.h"
#include "ligmadrawable-levels.h"
#include "ligmadrawable-operation.h"
#include "ligmahistogram.h"
#include "ligmaprogress.h"

#include "ligma-intl.h"


/*  public functions  */

void
ligma_drawable_levels_stretch (LigmaDrawable *drawable,
                              LigmaProgress *progress)
{
  LigmaLevelsConfig *config;
  LigmaHistogram    *histogram;
  GeglNode         *levels;

  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress));

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable), NULL, NULL, NULL, NULL))
    return;

  config = g_object_new (LIGMA_TYPE_LEVELS_CONFIG, NULL);

  histogram = ligma_histogram_new (FALSE);
  ligma_drawable_calculate_histogram (drawable, histogram, FALSE);

  ligma_levels_config_stretch (config, histogram,
                              ligma_drawable_is_rgb (drawable));

  g_object_unref (histogram);

  levels = g_object_new (GEGL_TYPE_NODE,
                         "operation", "ligma:levels",
                         NULL);

  gegl_node_set (levels,
                 "config", config,
                 NULL);

  ligma_drawable_apply_operation (drawable, progress, _("Levels"),
                                 levels);

  g_object_unref (levels);
  g_object_unref (config);
}
