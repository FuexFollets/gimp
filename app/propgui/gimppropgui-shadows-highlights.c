/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmapropgui-shadows-highlights.c
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "propgui-types.h"

#include "core/ligmacontext.h"

#include "ligmapropgui.h"
#include "ligmapropgui-shadows-highlights.h"

#include "ligma-intl.h"


GtkWidget *
_ligma_prop_gui_new_shadows_highlights (GObject                  *config,
                                       GParamSpec              **param_specs,
                                       guint                     n_param_specs,
                                       GeglRectangle            *area,
                                       LigmaContext              *context,
                                       LigmaCreatePickerFunc      create_picker_func,
                                       LigmaCreateControllerFunc  create_controller_func,
                                       gpointer                  creator)
{
  GtkWidget   *main_vbox;
  GtkWidget   *frame;
  GtkWidget   *vbox;
  GtkWidget   *scale;
  const gchar *label;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 4);

  frame = ligma_frame_new (_("Shadows"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  scale = ligma_prop_widget_new (config, "shadows",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = ligma_prop_widget_new (config, "shadows-ccorrect",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  frame = ligma_frame_new (_("Highlights"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  scale = ligma_prop_widget_new (config, "highlights",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = ligma_prop_widget_new (config, "highlights-ccorrect",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  frame = ligma_frame_new (_("Common"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  scale = ligma_prop_widget_new (config, "whitepoint",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = ligma_prop_widget_new (config, "radius",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = ligma_prop_widget_new (config, "compress",
                                area, context, NULL, NULL, NULL, &label);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  return main_vbox;
}
