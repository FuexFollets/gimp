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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "paint-types.h"

#include "ligmaairbrushoptions.h"

#include "ligma-intl.h"


#define AIRBRUSH_DEFAULT_RATE        50.0
#define AIRBRUSH_DEFAULT_FLOW        10.0
#define AIRBRUSH_DEFAULT_MOTION_ONLY FALSE

enum
{
  PROP_0,
  PROP_RATE,
  PROP_MOTION_ONLY,
  PROP_FLOW,
  PROP_PRESSURE /*for backwards copatibility of tool options*/
};


static void   ligma_airbrush_options_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void   ligma_airbrush_options_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaAirbrushOptions, ligma_airbrush_options,
               LIGMA_TYPE_PAINT_OPTIONS)


static void
ligma_airbrush_options_class_init (LigmaAirbrushOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_airbrush_options_set_property;
  object_class->get_property = ligma_airbrush_options_get_property;

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_RATE,
                           "rate",
                           C_("airbrush-tool", "Rate"),
                           NULL,
                           0.0, 100.0, AIRBRUSH_DEFAULT_RATE,
                           LIGMA_PARAM_STATIC_STRINGS);


  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_MOTION_ONLY,
                            "motion-only",
                            _("Motion only"),
                            NULL,
                            AIRBRUSH_DEFAULT_MOTION_ONLY,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_FLOW,
                           "flow",
                           _("Flow"),
                           NULL,
                           0.0, 100.0, AIRBRUSH_DEFAULT_FLOW,
                           LIGMA_PARAM_STATIC_STRINGS);

  /* backwads-compadibility prop for flow fomerly known as pressure */
  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_PRESSURE,
                           "pressure",
                           NULL, NULL,
                           0.0, 100.0, AIRBRUSH_DEFAULT_FLOW,
                           LIGMA_CONFIG_PARAM_IGNORE);
}

static void
ligma_airbrush_options_init (LigmaAirbrushOptions *options)
{
}

static void
ligma_airbrush_options_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  LigmaAirbrushOptions *options = LIGMA_AIRBRUSH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RATE:
      options->rate = g_value_get_double (value);
      break;
    case PROP_MOTION_ONLY:
      options->motion_only = g_value_get_boolean (value);
      break;
    case PROP_PRESSURE:
    case PROP_FLOW:
      options->flow = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_airbrush_options_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  LigmaAirbrushOptions *options = LIGMA_AIRBRUSH_OPTIONS (object);

  switch (property_id)
    {
    case PROP_RATE:
      g_value_set_double (value, options->rate);
      break;
    case PROP_MOTION_ONLY:
      g_value_set_boolean (value, options->motion_only);
      break;
    case PROP_PRESSURE:
    case PROP_FLOW:
      g_value_set_double (value, options->flow);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
