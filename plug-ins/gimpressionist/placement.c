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

#include <gtk/gtk.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "ligmaressionist.h"
#include "placement.h"

#include "libligma/stdplugins-intl.h"


#define NUM_PLACE_RADIO 2

static GtkWidget *placement_radio[NUM_PLACE_RADIO];
static GtkWidget *placement_center     = NULL;
static GtkWidget *brush_density_adjust = NULL;

void
place_restore (void)
{
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (placement_radio[pcvals.place_type]), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (placement_center),
                                pcvals.placement_center);
  ligma_label_spin_set_value (LIGMA_LABEL_SPIN (brush_density_adjust),
                             pcvals.brush_density);
}

int
place_type_input (int in)
{
  return CLAMP_UP_TO (in, NUM_PLACE_RADIO);
}

void
place_store (void)
{
  pcvals.placement_center = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (placement_center));
}

void
create_placementpage (GtkNotebook *notebook)
{
  GtkWidget *vbox;
  GtkWidget *label, *tmpw, *frame;

  label = gtk_label_new_with_mnemonic (_("Pl_acement"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_show (vbox);

  frame = ligma_int_radio_group_new (TRUE, _("Placement"),
                                    G_CALLBACK (ligma_radio_button_update),
                                    &pcvals.place_type, NULL, 0,

                                    _("Randomly"),
                                    PLACEMENT_TYPE_RANDOM,
                                    &placement_radio[PLACEMENT_TYPE_RANDOM],

                                    _("Evenly distributed"),
                                    PLACEMENT_TYPE_EVEN_DIST,
                                    &placement_radio[PLACEMENT_TYPE_EVEN_DIST],

                                    NULL);

  ligma_help_set_help_data
    (placement_radio[PLACEMENT_TYPE_RANDOM],
     _("Place strokes randomly around the image"),
     NULL);
  ligma_help_set_help_data
    (placement_radio[PLACEMENT_TYPE_EVEN_DIST],
     _("The strokes are evenly distributed across the image"),
     NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (placement_radio[pcvals.place_type]), TRUE);

  placement_center = gtk_check_button_new_with_mnemonic ( _("Centered"));
  tmpw = placement_center;

  gtk_box_pack_start (GTK_BOX (vbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  ligma_help_set_help_data
    (tmpw, _("Focus the brush strokes around the center of the image"), NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw),
                                pcvals.placement_center);

  brush_density_adjust =
    ligma_scale_entry_new (_("Stroke _density:"), pcvals.brush_density, 1.0, 50.0, 0);
  ligma_help_set_help_data (brush_density_adjust,
                           _("The relative density of the brush strokes"),
                           NULL);
  g_signal_connect (brush_density_adjust, "value-changed",
                    G_CALLBACK (ligmaressionist_scale_entry_update_double),
                    &pcvals.brush_density);
  gtk_box_pack_start (GTK_BOX (vbox), brush_density_adjust, FALSE, FALSE, 6);
  gtk_widget_show (brush_density_adjust);

  gtk_notebook_append_page_menu (notebook, vbox, label, NULL);
}
