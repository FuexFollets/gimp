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

#include "widgets/ligmawidgets-utils.h"

#include "ligmamoveoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_MOVE_TYPE,
  PROP_MOVE_CURRENT
};


static void   ligma_move_options_set_property (GObject      *object,
                                              guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static void   ligma_move_options_get_property (GObject      *object,
                                              guint         property_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaMoveOptions, ligma_move_options, LIGMA_TYPE_TOOL_OPTIONS)


static void
ligma_move_options_class_init (LigmaMoveOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_move_options_set_property;
  object_class->get_property = ligma_move_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_MOVE_TYPE,
                         "move-type",
                         NULL, NULL,
                         LIGMA_TYPE_TRANSFORM_TYPE,
                         LIGMA_TRANSFORM_TYPE_LAYER,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_MOVE_CURRENT,
                            "move-current",
                            NULL, NULL,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_move_options_init (LigmaMoveOptions *options)
{
}

static void
ligma_move_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaMoveOptions *options = LIGMA_MOVE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MOVE_TYPE:
      options->move_type = g_value_get_enum (value);
      break;
    case PROP_MOVE_CURRENT:
      options->move_current = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_move_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaMoveOptions *options = LIGMA_MOVE_OPTIONS (object);

  switch (property_id)
    {
    case PROP_MOVE_TYPE:
      g_value_set_enum (value, options->move_type);
      break;
    case PROP_MOVE_CURRENT:
      g_value_set_boolean (value, options->move_current);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_move_options_notify_type (LigmaMoveOptions *move_options,
                               GParamSpec      *pspec,
                               GtkWidget       *frame)
{
  if (move_options->move_type == LIGMA_TRANSFORM_TYPE_SELECTION)
    {
      gtk_widget_hide (gtk_bin_get_child (GTK_BIN (frame)));
      gtk_frame_set_label (GTK_FRAME (frame), _("Move selection"));
    }
  else
    {
      const gchar *false_label = NULL;
      const gchar *true_label  = NULL;
      GtkWidget   *button;
      GSList      *group;
      gchar       *title;

      title = g_strdup_printf (_("Tool Toggle  (%s)"),
                               ligma_get_mod_string (ligma_get_extend_selection_mask ()));
      gtk_frame_set_label (GTK_FRAME (frame), title);
      g_free (title);

      switch (move_options->move_type)
        {
        case LIGMA_TRANSFORM_TYPE_LAYER:
          false_label = _("Pick a layer or guide");
          true_label  = _("Move the selected layers");
          break;

        case LIGMA_TRANSFORM_TYPE_PATH:
          false_label = _("Pick a path");
          true_label  = _("Move the active path");
          break;

        default: /* LIGMA_TRANSFORM_TYPE_SELECTION */
          g_return_if_reached ();
        }

      button = g_object_get_data (G_OBJECT (frame), "radio-button");

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_button_set_label (GTK_BUTTON (group->data), true_label);

      group = g_slist_next (group);
      gtk_button_set_label (GTK_BUTTON (group->data), false_label);

      gtk_widget_show (gtk_bin_get_child (GTK_BIN (frame)));
    }
}

GtkWidget *
ligma_move_options_gui (LigmaToolOptions *tool_options)
{
  GObject         *config  = G_OBJECT (tool_options);
  LigmaMoveOptions *options = LIGMA_MOVE_OPTIONS (tool_options);
  GtkWidget       *vbox    = ligma_tool_options_gui (tool_options);
  GtkWidget       *hbox;
  GtkWidget       *box;
  GtkWidget       *label;
  GtkWidget       *frame;
  gchar           *title;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  options->type_box = hbox;

  label = gtk_label_new (_("Move:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  box = ligma_prop_enum_icon_box_new (config, "move-type", "ligma",
                                     LIGMA_TRANSFORM_TYPE_LAYER,
                                     LIGMA_TRANSFORM_TYPE_PATH);
  gtk_box_pack_start (GTK_BOX (hbox), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  /*  tool toggle  */
  title =
    g_strdup_printf (_("Tool Toggle  (%s)"),
                     ligma_get_mod_string (ligma_get_extend_selection_mask ()));

  frame = ligma_prop_boolean_radio_frame_new (config, "move-current",
                                             title, "true", "false");

  ligma_move_options_notify_type (LIGMA_MOVE_OPTIONS (config), NULL, frame);

  g_signal_connect_object (config, "notify::move-type",
                           G_CALLBACK (ligma_move_options_notify_type),
                           frame, 0);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_free (title);

  return vbox;
}
