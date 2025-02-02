/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationcurves.c
 * Copyright (C) 2007 Michael Natterer <mitch@ligma.org>
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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmamath/ligmamath.h"

#include "operations-types.h"

#include "core/ligmacurve.h"
#include "core/ligmacurve-map.h"

#include "ligmacurvesconfig.h"
#include "ligmaoperationcurves.h"

#include "ligma-intl.h"


static gboolean ligma_operation_curves_process (GeglOperation       *operation,
                                               void                *in_buf,
                                               void                *out_buf,
                                               glong                samples,
                                               const GeglRectangle *roi,
                                               gint                 level);


G_DEFINE_TYPE (LigmaOperationCurves, ligma_operation_curves,
               LIGMA_TYPE_OPERATION_POINT_FILTER)

#define parent_class ligma_operation_curves_parent_class


static void
ligma_operation_curves_class_init (LigmaOperationCurvesClass *klass)
{
  GObjectClass                  *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass            *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_class     = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  object_class->set_property   = ligma_operation_point_filter_set_property;
  object_class->get_property   = ligma_operation_point_filter_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:curves",
                                 "categories",  "color",
                                 "description", _("Adjust color curves"),
                                 NULL);

  point_class->process = ligma_operation_curves_process;

  g_object_class_install_property (object_class,
                                   LIGMA_OPERATION_POINT_FILTER_PROP_TRC,
                                   g_param_spec_enum ("trc",
                                                      "Linear/Percptual",
                                                      "What TRC to operate on",
                                                      LIGMA_TYPE_TRC_TYPE,
                                                      LIGMA_TRC_NON_LINEAR,
                                                      G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
                                   LIGMA_OPERATION_POINT_FILTER_PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The config object",
                                                        LIGMA_TYPE_CURVES_CONFIG,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_operation_curves_init (LigmaOperationCurves *self)
{
}

static gboolean
ligma_operation_curves_process (GeglOperation       *operation,
                               void                *in_buf,
                               void                *out_buf,
                               glong                samples,
                               const GeglRectangle *roi,
                               gint                 level)
{
  LigmaOperationPointFilter *point  = LIGMA_OPERATION_POINT_FILTER (operation);
  LigmaCurvesConfig         *config = LIGMA_CURVES_CONFIG (point->config);
  gfloat                   *src    = in_buf;
  gfloat                   *dest   = out_buf;

  if (! config)
    return FALSE;

  ligma_curve_map_pixels (config->curve[0],
                         config->curve[1],
                         config->curve[2],
                         config->curve[3],
                         config->curve[4], src, dest, samples);

  return TRUE;
}
