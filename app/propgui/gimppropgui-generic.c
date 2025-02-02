/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmapropgui-generic.c
 * Copyright (C) 2002-2014  Michael Natterer <mitch@ligma.org>
 *                          Sven Neumann <sven@ligma.org>
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
#include <stdlib.h>

#include <gegl.h>
#include <gegl-paramspecs.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "propgui-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "core/ligmacontext.h"

#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmapropgui.h"
#include "ligmapropgui-generic.h"

#include "ligma-intl.h"


#define HAS_KEY(p,k,v) ligma_gegl_param_spec_has_key (p, k, v)


static void   ligma_prop_gui_chain_toggled (LigmaChainButton *chain,
                                           GtkAdjustment   *x_adj);


/*  public functions  */

GtkWidget *
_ligma_prop_gui_new_generic (GObject                  *config,
                            GParamSpec              **param_specs,
                            guint                     n_param_specs,
                            GeglRectangle            *area,
                            LigmaContext              *context,
                            LigmaCreatePickerFunc      create_picker_func,
                            LigmaCreateControllerFunc  create_controller_func,
                            gpointer                  creator)
{
  GtkWidget    *main_vbox;
  GtkSizeGroup *label_group;
  GList        *chains = NULL;
  gint          i;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

  label_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  for (i = 0; i < n_param_specs; i++)
    {
      GParamSpec  *pspec      = param_specs[i];
      GParamSpec  *next_pspec = NULL;

      if (i < n_param_specs - 1)
        next_pspec = param_specs[i + 1];

      if (next_pspec                        &&
          HAS_KEY (pspec,      "axis", "x") &&
          HAS_KEY (next_pspec, "axis", "y"))
        {
          GtkWidget     *widget_x;
          GtkWidget     *widget_y;
          const gchar   *label_x;
          const gchar   *label_y;
          GtkAdjustment *adj_x;
          GtkAdjustment *adj_y;
          GtkWidget     *hbox;
          GtkWidget     *vbox;
          GtkWidget     *chain;

          i++;

          widget_x = ligma_prop_widget_new_from_pspec (config, pspec,
                                                      area, context,
                                                      create_picker_func,
                                                      create_controller_func,
                                                      creator,
                                                      &label_x);
          widget_y = ligma_prop_widget_new_from_pspec (config, next_pspec,
                                                      area, context,
                                                      create_picker_func,
                                                      create_controller_func,
                                                      creator,
                                                      &label_y);

          adj_x = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget_x));
          adj_y = gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (widget_y));

          hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
          gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
          gtk_widget_show (hbox);

          ligma_prop_gui_bind_container (widget_x, hbox);

          vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
          gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
          gtk_widget_show (vbox);

          gtk_box_pack_start (GTK_BOX (vbox), widget_x, FALSE, FALSE, 0);
          gtk_widget_show (widget_x);

          gtk_box_pack_start (GTK_BOX (vbox), widget_y, FALSE, FALSE, 0);
          gtk_widget_show (widget_y);

          chain = ligma_chain_button_new (LIGMA_CHAIN_RIGHT);
          gtk_box_pack_end (GTK_BOX (hbox), chain, FALSE, FALSE, 0);
          gtk_widget_show (chain);

          if (! HAS_KEY (pspec, "unit", "pixel-coordinate")    &&
              ! HAS_KEY (pspec, "unit", "relative-coordinate") &&
              gtk_adjustment_get_value (adj_x) ==
              gtk_adjustment_get_value (adj_y))
            {
              GBinding *binding;

              ligma_chain_button_set_active (LIGMA_CHAIN_BUTTON (chain), TRUE);

              binding = g_object_bind_property (adj_x, "value",
                                                adj_y, "value",
                                                G_BINDING_BIDIRECTIONAL);

              g_object_set_data (G_OBJECT (chain), "binding", binding);
            }

          g_object_set_data_full (G_OBJECT (chain), "x-property",
                                  g_strdup (pspec->name), g_free);
          g_object_set_data_full (G_OBJECT (chain), "y-property",
                                  g_strdup (next_pspec->name), g_free);

          chains = g_list_prepend (chains, chain);

          g_signal_connect (chain, "toggled",
                            G_CALLBACK (ligma_prop_gui_chain_toggled),
                            adj_x);

          g_object_set_data (G_OBJECT (adj_x), "y-adjustment", adj_y);

          if (create_picker_func &&
              (HAS_KEY (pspec, "unit", "pixel-coordinate") ||
               HAS_KEY (pspec, "unit", "relative-coordinate")))
            {
              GtkWidget *button;
              gchar     *pspec_name;

              pspec_name = g_strconcat (pspec->name, ":",
                                        next_pspec->name, NULL);

              button = create_picker_func (creator,
                                           pspec_name,
                                           LIGMA_ICON_CURSOR,
                                           _("Pick coordinates from the image"),
                                           /* pick_abyss = */ TRUE,
                                           NULL, NULL);
              gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
              gtk_widget_show (button);

              g_object_weak_ref (G_OBJECT (button),
                                 (GWeakNotify) g_free, pspec_name);
            }
        }
      else if (next_pspec                                  &&
               HAS_KEY (pspec,      "role", "range-start") &&
               HAS_KEY (next_pspec, "role", "range-end")   &&
               HAS_KEY (pspec,      "unit", "luminance"))
        {
          GtkWidget   *vbox;
          GtkWidget   *spin_scale;
          GtkWidget   *label;
          GtkWidget   *frame;
          GtkWidget   *range;
          const gchar *label_str;
          const gchar *range_label_str;
          gdouble      step_increment;
          gdouble      page_increment;
          gdouble      ui_lower;
          gdouble      ui_upper;

          i++;

          vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
          gtk_box_pack_start (GTK_BOX (main_vbox), vbox, FALSE, FALSE, 0);

          spin_scale = ligma_prop_widget_new_from_pspec (
            config, pspec,
            area, context,
            create_picker_func,
            create_controller_func,
            creator,
            &label_str);
          gtk_widget_show (spin_scale);

          g_object_set_data_full (G_OBJECT (vbox),
                                  "ligma-underlying-widget",
                                  g_object_ref_sink (spin_scale),
                                  g_object_unref);

          range_label_str = gegl_param_spec_get_property_key (pspec,
                                                              "range-label");

          if (range_label_str)
            label_str = range_label_str;

          gtk_spin_button_get_increments (GTK_SPIN_BUTTON (spin_scale),
                                          &step_increment, &page_increment);

          ligma_spin_scale_get_scale_limits (LIGMA_SPIN_SCALE (spin_scale),
                                            &ui_lower, &ui_upper);

          label = gtk_label_new_with_mnemonic (label_str);
          gtk_label_set_xalign (GTK_LABEL (label), 0.0);
          gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
          gtk_widget_show (label);

          if (! range_label_str)
            {
              g_object_bind_property (spin_scale, "label",
                                      label,      "label",
                                      G_BINDING_SYNC_CREATE);
            }

          frame = ligma_frame_new (NULL);
          gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
          gtk_widget_show (frame);

          range = ligma_prop_range_new (config,
                                       pspec->name, next_pspec->name,
                                       step_increment, page_increment,
                                       gtk_spin_button_get_digits (
                                         GTK_SPIN_BUTTON (spin_scale)),
                                       ! HAS_KEY (pspec,
                                                  "range-sorted", "false"));
          ligma_prop_range_set_ui_limits (range, ui_lower, ui_upper);
          gtk_container_add (GTK_CONTAINER (frame), range);
          gtk_widget_show (range);

          ligma_prop_gui_bind_container (spin_scale, vbox);
          ligma_prop_gui_bind_tooltip   (spin_scale, vbox);
        }
      else
        {
          GtkWidget   *widget;
          const gchar *label;
          gboolean     expand = FALSE;

          widget = ligma_prop_widget_new_from_pspec (config, pspec,
                                                    area, context,
                                                    create_picker_func,
                                                    create_controller_func,
                                                    creator,
                                                    &label);

          if (GTK_IS_SCROLLED_WINDOW (widget))
            expand = TRUE;

          if (widget && label)
            {
              GtkWidget *l;

              l = gtk_label_new_with_mnemonic (label);
              gtk_label_set_xalign (GTK_LABEL (l), 0.0);
              gtk_widget_show (l);

              ligma_prop_gui_bind_label (widget, l);

              if (GTK_IS_SCROLLED_WINDOW (widget))
                {
                  GtkWidget *frame;

                  /* don't set as frame title, it should not be bold */
                  gtk_box_pack_start (GTK_BOX (main_vbox), l, FALSE, FALSE, 0);

                  frame = ligma_frame_new (NULL);
                  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 0);
                  gtk_widget_show (frame);

                  gtk_container_add (GTK_CONTAINER (frame), widget);
                  gtk_widget_show (widget);

                  ligma_prop_gui_bind_container (widget, frame);
                }
              else
                {
                  GtkWidget *hbox;

                  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
                  gtk_box_pack_start (GTK_BOX (main_vbox), hbox,
                                      expand, expand, 0);
                  gtk_widget_show (hbox);

                  gtk_size_group_add_widget (label_group, l);
                  gtk_box_pack_start (GTK_BOX (hbox), l, FALSE, FALSE, 0);

                  gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
                  gtk_widget_show (widget);

                  ligma_prop_gui_bind_container (widget, hbox);
                }
            }
          else if (widget)
            {
              gtk_box_pack_start (GTK_BOX (main_vbox), widget,
                                  expand, expand, 0);
              gtk_widget_show (widget);
            }
        }
    }

  g_object_unref (label_group);

  g_object_set_data_full (G_OBJECT (main_vbox), "chains", chains,
                          (GDestroyNotify) g_list_free);

  gtk_widget_show (main_vbox);

  return main_vbox;
}


/*  private functions  */

static void
ligma_prop_gui_chain_toggled (LigmaChainButton *chain,
                             GtkAdjustment   *x_adj)
{
  GBinding      *binding;
  GtkAdjustment *y_adj;

  binding = g_object_get_data (G_OBJECT (chain), "binding");
  y_adj   = g_object_get_data (G_OBJECT (x_adj), "y-adjustment");

  if (ligma_chain_button_get_active (chain))
    {
      if (! binding)
        binding = g_object_bind_property (x_adj, "value",
                                          y_adj, "value",
                                          G_BINDING_BIDIRECTIONAL);
    }
  else
    {
      g_clear_object (&binding);
    }
  g_object_set_data (G_OBJECT (chain), "binding", binding);
}
