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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmatoolinfo.h"

#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmatransform3doptions.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_MODE,
  PROP_UNIFIED,
  PROP_CONSTRAIN_AXIS,
  PROP_Z_AXIS,
  PROP_LOCAL_FRAME
};


static void   ligma_transform_3d_options_set_property (GObject      *object,
                                                      guint         property_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);
static void   ligma_transform_3d_options_get_property (GObject      *object,
                                                      guint         property_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaTransform3DOptions, ligma_transform_3d_options,
               LIGMA_TYPE_TRANSFORM_GRID_OPTIONS)

#define parent_class ligma_transform_3d_options_parent_class


static void
ligma_transform_3d_options_class_init (LigmaTransform3DOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_transform_3d_options_set_property;
  object_class->get_property = ligma_transform_3d_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_MODE,
                         "mode",
                         _("Mode"),
                         _("Transform mode"),
                         LIGMA_TYPE_TRANSFORM_3D_MODE,
                         LIGMA_TRANSFORM_3D_MODE_ROTATE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_UNIFIED,
                            "unified",
                            _("Unified interaction"),
                            _("Combine all interaction modes"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_CONSTRAIN_AXIS,
                            "constrain-axis",
                            NULL,
                            _("Constrain transformation to a single axis"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_Z_AXIS,
                            "z-axis",
                            NULL,
                            _("Transform along the Z axis"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_LOCAL_FRAME,
                            "local-frame",
                            NULL,
                            _("Transform in the local frame of reference"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_transform_3d_options_init (LigmaTransform3DOptions *options)
{
}

static void
ligma_transform_3d_options_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  LigmaTransform3DOptions *options = LIGMA_TRANSFORM_3D_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MODE:
      options->mode = g_value_get_enum (value);
      break;
    case PROP_UNIFIED:
      options->unified = g_value_get_boolean (value);
      break;

    case PROP_CONSTRAIN_AXIS:
      options->constrain_axis = g_value_get_boolean (value);
      break;
    case PROP_Z_AXIS:
      options->z_axis = g_value_get_boolean (value);
      break;
    case PROP_LOCAL_FRAME:
      options->local_frame = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_transform_3d_options_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  LigmaTransform3DOptions *options = LIGMA_TRANSFORM_3D_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, options->mode);
      break;
    case PROP_UNIFIED:
      g_value_set_boolean (value, options->unified);
      break;

    case PROP_CONSTRAIN_AXIS:
      g_value_set_boolean (value, options->constrain_axis);
      break;
    case PROP_Z_AXIS:
      g_value_set_boolean (value, options->z_axis);
      break;
    case PROP_LOCAL_FRAME:
      g_value_set_boolean (value, options->local_frame);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
ligma_transform_3d_options_gui (LigmaToolOptions *tool_options)
{
  GObject         *config = G_OBJECT (tool_options);
  GtkWidget       *vbox   = ligma_transform_grid_options_gui (tool_options);
  GtkWidget       *button;
  gchar           *label;
  GdkModifierType  extend_mask    = ligma_get_extend_selection_mask ();
  GdkModifierType  constrain_mask = ligma_get_constrain_behavior_mask ();

  button = ligma_prop_check_button_new (config, "unified", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  label = g_strdup_printf (_("Constrain axis (%s)"),
                           ligma_get_mod_string (extend_mask));

  button = ligma_prop_check_button_new (config, "constrain-axis", label);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_free (label);

  label = g_strdup_printf (_("Z axis (%s)"),
                           ligma_get_mod_string (constrain_mask));

  button = ligma_prop_check_button_new (config, "z-axis", label);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_free (label);

  label = g_strdup_printf (_("Local frame (%s)"),
                           ligma_get_mod_string (GDK_MOD1_MASK));

  button = ligma_prop_check_button_new (config, "local-frame", label);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_free (label);

  return vbox;
}
