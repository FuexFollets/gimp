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

#include "ligmadodgeburnoptions.h"

#include "ligma-intl.h"


#define DODGE_BURN_DEFAULT_TYPE     LIGMA_DODGE_BURN_TYPE_DODGE
#define DODGE_BURN_DEFAULT_MODE     LIGMA_TRANSFER_MIDTONES
#define DODGE_BURN_DEFAULT_EXPOSURE 50.0


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_MODE,
  PROP_EXPOSURE
};


static void   ligma_dodge_burn_options_set_property (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void   ligma_dodge_burn_options_get_property (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaDodgeBurnOptions, ligma_dodge_burn_options,
               LIGMA_TYPE_PAINT_OPTIONS)


static void
ligma_dodge_burn_options_class_init (LigmaDodgeBurnOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_dodge_burn_options_set_property;
  object_class->get_property = ligma_dodge_burn_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_TYPE,
                         "type",
                         _("Type"),
                         NULL,
                         LIGMA_TYPE_DODGE_BURN_TYPE,
                         DODGE_BURN_DEFAULT_TYPE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "mode",
                         _("Range"),
                         NULL,
                         LIGMA_TYPE_TRANSFER_MODE,
                         DODGE_BURN_DEFAULT_MODE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_EXPOSURE,
                           "exposure",
                           _("Exposure"),
                           NULL,
                           0.0, 100.0, DODGE_BURN_DEFAULT_EXPOSURE,
                           LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_dodge_burn_options_init (LigmaDodgeBurnOptions *options)
{
}

static void
ligma_dodge_burn_options_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  LigmaDodgeBurnOptions *options = LIGMA_DODGE_BURN_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TYPE:
      options->type = g_value_get_enum (value);
      break;
    case PROP_MODE:
      options->mode = g_value_get_enum (value);
      break;
    case PROP_EXPOSURE:
      options->exposure = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_dodge_burn_options_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  LigmaDodgeBurnOptions *options = LIGMA_DODGE_BURN_OPTIONS (object);

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, options->type);
      break;
    case PROP_MODE:
      g_value_set_enum (value, options->mode);
      break;
    case PROP_EXPOSURE:
      g_value_set_double (value, options->exposure);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}
