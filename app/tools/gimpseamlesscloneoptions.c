/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmaseamlesscloneoptions.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "widgets/ligmapropwidgets.h"

#include "ligmaseamlesscloneoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_MAX_REFINE_SCALE,
};


static void   ligma_seamless_clone_options_set_property (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
static void   ligma_seamless_clone_options_get_property (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaSeamlessCloneOptions, ligma_seamless_clone_options,
               LIGMA_TYPE_TOOL_OPTIONS)

#define parent_class ligma_seamless_clone_options_parent_class


static void
ligma_seamless_clone_options_class_init (LigmaSeamlessCloneOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_seamless_clone_options_set_property;
  object_class->get_property = ligma_seamless_clone_options_get_property;

  LIGMA_CONFIG_PROP_INT  (object_class, PROP_MAX_REFINE_SCALE,
                         "max-refine-scale",
                         _("Refinement scale"),
                         _("Maximal scale of refinement points to be "
                           "used for the interpolation mesh"),
                         0, 50, 5,
                         LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_seamless_clone_options_init (LigmaSeamlessCloneOptions *options)
{
}

static void
ligma_seamless_clone_options_set_property (GObject      *object,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  LigmaSeamlessCloneOptions *options = LIGMA_SEAMLESS_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MAX_REFINE_SCALE:
      options->max_refine_scale = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_seamless_clone_options_get_property (GObject    *object,
                                          guint       property_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  LigmaSeamlessCloneOptions *options = LIGMA_SEAMLESS_CLONE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MAX_REFINE_SCALE:
      g_value_set_int (value, options->max_refine_scale);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
ligma_seamless_clone_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_tool_options_gui (tool_options);
  GtkWidget *scale;

  scale = ligma_prop_spin_scale_new (config, "max-refine-scale",
                                    1.0, 10.0, 0);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), 0.0, 50.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return vbox;
}
