/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmapropgui-diffraction-patterns.c
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
#include "ligmapropgui-diffraction-patterns.h"
#include "ligmapropgui-generic.h"

#include "ligma-intl.h"


GtkWidget *
_ligma_prop_gui_new_diffraction_patterns (GObject                  *config,
                                         GParamSpec              **param_specs,
                                         guint                     n_param_specs,
                                         GeglRectangle            *area,
                                         LigmaContext              *context,
                                         LigmaCreatePickerFunc      create_picker_func,
                                         LigmaCreateControllerFunc  create_controller_func,
                                         gpointer                  creator)
{
  GtkWidget *notebook;
  GtkWidget *vbox;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  notebook = gtk_notebook_new ();

  vbox = _ligma_prop_gui_new_generic (config,
                                     param_specs + 0, 3,
                                     area, context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Frequencies")));

  vbox = _ligma_prop_gui_new_generic (config,
                                     param_specs + 3, 3,
                                     area, context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Contours")));

  vbox = _ligma_prop_gui_new_generic (config,
                                     param_specs + 6, 3,
                                     area, context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Sharp Edges")));

  vbox = _ligma_prop_gui_new_generic (config,
                                     param_specs + 9, 3,
                                     area, context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox,
                            gtk_label_new (_("Other Options")));

  return notebook;
}
