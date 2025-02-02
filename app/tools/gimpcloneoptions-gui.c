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

#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmaitem.h"

#include "paint/ligmacloneoptions.h"

#include "widgets/ligmaviewablebox.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmacloneoptions-gui.h"
#include "ligmapaintoptions-gui.h"

#include "ligma-intl.h"


static gboolean ligma_clone_options_sync_source           (GBinding     *binding,
                                                          const GValue *source_value,
                                                          GValue       *target_value,
                                                          gpointer      user_data);

static void ligma_clone_options_gui_drawables_changed     (LigmaImage         *image,
                                                          LigmaSourceOptions *options);
static void ligma_clone_options_gui_src_changed           (LigmaSourceOptions *options,
                                                          GParamSpec        *pspec,
                                                          GtkWidget         *label);
static void ligma_clone_options_gui_context_image_changed (LigmaContext       *context,
                                                          LigmaImage         *image,
                                                          LigmaSourceOptions *options);

static gboolean ligma_clone_options_gui_update_src_label  (LigmaSourceOptions *options);


static gboolean
ligma_clone_options_sync_source (GBinding     *binding,
                                const GValue *source_value,
                                GValue       *target_value,
                                gpointer      user_data)
{
  LigmaCloneType type = g_value_get_enum (source_value);

  g_value_set_boolean (target_value,
                       type == GPOINTER_TO_INT (user_data));

  return TRUE;
}

static void
ligma_clone_options_gui_drawables_changed (LigmaImage         *image,
                                          LigmaSourceOptions *options)
{
  GList     *drawables;
  GtkWidget *button;

  drawables = ligma_image_get_selected_drawables (image);
  button    = g_object_get_data (G_OBJECT (options), "sample-merged-checkbox");

  gtk_widget_set_sensitive (button, (g_list_length (drawables) < 2));
  g_list_free (drawables);

  /* In case this was called several times in a row by threads. */
  g_idle_remove_by_data (options);

  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                   (GSourceFunc) ligma_clone_options_gui_update_src_label,
                   g_object_ref (options), (GDestroyNotify) g_object_unref);
}

static void
ligma_clone_options_gui_src_changed (LigmaSourceOptions *options,
                                    GParamSpec        *pspec,
                                    GtkWidget         *label)
{
  /* In case this was called several times in a row by threads. */
  g_idle_remove_by_data (options);

  /* Why we need absolutely to run this in a thread is that it updates
   * the GUI. And sometimes this src_changed callback may be called by
   * paint threads (see ligmapainttool-paint.c). This may cause crashes
   * as in recent GTK, all GTK/GDK calls should be main from the main
   * thread. Idle functions are ensured to be run in this main thread.
   */
  g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
                   (GSourceFunc) ligma_clone_options_gui_update_src_label,
                   g_object_ref (options), (GDestroyNotify) g_object_unref);
}

static void
ligma_clone_options_gui_context_image_changed (LigmaContext       *context,
                                              LigmaImage         *image,
                                              LigmaSourceOptions *options)
{
  LigmaImage *prev_image;
  GtkWidget *button;

  button     = g_object_get_data (G_OBJECT (options), "sample-merged-checkbox");
  prev_image = g_object_get_data (G_OBJECT (button), "ligma-clone-options-gui-image");

  if (image != prev_image)
    {
      if (prev_image)
        g_signal_handlers_disconnect_by_func (prev_image,
                                              G_CALLBACK (ligma_clone_options_gui_drawables_changed),
                                              options);
      if (image)
        {
          g_signal_connect_object (image, "selected-channels-changed",
                                   G_CALLBACK (ligma_clone_options_gui_drawables_changed),
                                   options, 0);
          g_signal_connect_object (image, "selected-layers-changed",
                                   G_CALLBACK (ligma_clone_options_gui_drawables_changed),
                                   options, 0);
          ligma_clone_options_gui_drawables_changed (image, options);
        }
      else
        {
          gtk_widget_set_sensitive (button, TRUE);
        }

      g_object_set_data (G_OBJECT (button), "ligma-clone-options-gui-image", image);
    }
}

static gboolean
ligma_clone_options_gui_update_src_label (LigmaSourceOptions *options)
{
  GtkWidget *label;
  gchar     *markup = NULL;

  label = g_object_get_data (G_OBJECT (options), "src-label");

  if (options->src_drawables == NULL)
    {
      markup = g_strdup_printf ("<i>%s</i>", _("No source selected"));
    }
  else
    {
      LigmaImage *image;
      GList     *drawables;
      gchar     *str = NULL;
      gboolean   sample_merged;

      image = ligma_context_get_image (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma));
      drawables = ligma_image_get_selected_drawables (image);

      sample_merged = options->sample_merged && g_list_length (drawables) == 1;

      if (g_list_length (drawables) > 1)
        {
          str = g_strdup_printf (ngettext ("Source: %d item to itself",
                                           "Source: %d items to themselves",
                                           g_list_length (drawables)),
                                 g_list_length (drawables));
        }
      else
        {
          LigmaImage *src_image = NULL;

          src_image = ligma_item_get_image (options->src_drawables->data);

          if (sample_merged)
            {
              if (image == src_image)
                str = g_strdup (_("All composited visible layers"));
              else
                str = g_strdup_printf (_("All composited visible layers from '%s'"),
                                       ligma_image_get_display_name (src_image));
            }
          else
            {
              if (image == src_image)
                str = g_strdup_printf (ngettext ("Source: %d item",
                                                 "Source: %d items",
                                                 g_list_length (options->src_drawables)),
                                       g_list_length (options->src_drawables));
              else
                str = g_strdup_printf (ngettext ("Source: %d item from '%s'",
                                                 "Source: %d items from '%s'",
                                                 g_list_length (options->src_drawables)),
                                       g_list_length (options->src_drawables),
                                       ligma_image_get_display_name (src_image));
            }
        }
      markup = g_strdup_printf ("<i>%s</i>", str);

      g_list_free (drawables);
      g_free (str);
    }

  gtk_label_set_markup (GTK_LABEL (label), markup);
  g_free (markup);

  return G_SOURCE_REMOVE;
}


/* Public functions. */


GtkWidget *
ligma_clone_options_gui (LigmaToolOptions *tool_options)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox   = ligma_paint_options_gui (tool_options);
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *combo;
  GtkWidget *source_vbox;
  GtkWidget *button;
  GtkWidget *hbox;
  gchar     *str;

  /*  the source frame  */
  frame = ligma_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), frame, 2);
  gtk_widget_show (frame);

  /*  the source type menu  */
  combo = ligma_prop_enum_combo_box_new (config, "clone-type", 0, 0);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Source"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_frame_set_label_widget (GTK_FRAME (frame), combo);

  source_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), source_vbox);
  gtk_widget_show (source_vbox);

  button = ligma_prop_check_button_new (config, "sample-merged", NULL);
  g_object_set_data (G_OBJECT (tool_options), "sample-merged-checkbox", button);
  gtk_box_pack_start (GTK_BOX (source_vbox), button, FALSE, FALSE, 0);

  g_object_bind_property_full (config, "clone-type",
                               button, "visible",
                               G_BINDING_SYNC_CREATE,
                               ligma_clone_options_sync_source,
                               NULL,
                               GINT_TO_POINTER (LIGMA_CLONE_IMAGE), NULL);

  label = gtk_label_new (NULL);
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  str = g_strdup_printf ("<i>%s</i>", _("No source selected"));
  gtk_label_set_markup (GTK_LABEL (label), str);
  g_object_set_data_full (G_OBJECT (tool_options), "src-label",
                          g_object_ref (label),
                          (GDestroyNotify) g_object_unref);
  g_free (str);

  gtk_box_pack_start (GTK_BOX (source_vbox), label, FALSE, FALSE, 0);

  g_object_bind_property_full (config, "clone-type",
                               label,  "visible",
                               G_BINDING_SYNC_CREATE,
                               ligma_clone_options_sync_source,
                               NULL,
                               GINT_TO_POINTER (LIGMA_CLONE_IMAGE), NULL);

  g_signal_connect (ligma_get_user_context (LIGMA_CONTEXT (tool_options)->ligma),
                    "image-changed",
                    G_CALLBACK (ligma_clone_options_gui_context_image_changed),
                    tool_options);

  hbox = ligma_prop_pattern_box_new (NULL, LIGMA_CONTEXT (tool_options),
                                    NULL, 2,
                                    "pattern-view-type", "pattern-view-size");
  gtk_box_pack_start (GTK_BOX (source_vbox), hbox, FALSE, FALSE, 0);

  g_object_bind_property_full (config, "clone-type",
                               hbox,   "visible",
                               G_BINDING_SYNC_CREATE,
                               ligma_clone_options_sync_source,
                               NULL,
                               GINT_TO_POINTER (LIGMA_CLONE_PATTERN), NULL);

  combo = ligma_prop_enum_combo_box_new (config, "align-mode", 0, 0);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Alignment"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, TRUE, TRUE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox), combo, 3);

  /* A few options which can trigger a change in the source label. */
  g_signal_connect (config, "notify::src-drawables",
                    G_CALLBACK (ligma_clone_options_gui_src_changed),
                    label);
  g_signal_connect (config, "notify::sample-merged",
                    G_CALLBACK (ligma_clone_options_gui_src_changed),
                    label);
  ligma_clone_options_gui_src_changed (LIGMA_SOURCE_OPTIONS (config), NULL, label);

  return vbox;
}
