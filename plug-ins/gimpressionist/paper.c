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

#include <string.h>

#include <gtk/gtk.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "ligmaressionist.h"
#include "ppmtool.h"
#include "paper.h"

#include "libligma/stdplugins-intl.h"


static GtkWidget *paper_preview       = NULL;
static GtkWidget *paper_invert        = NULL;
static GtkWidget *paper_list          = NULL;
static GtkWidget *paper_relief_adjust = NULL;
static GtkWidget *paper_scale_adjust  = NULL;
static GtkWidget *paper_overlay       = NULL;

static void paper_update_preview (void)
{
  gint     i, j;
  guchar  *buf, *paper_preview_buffer;
  gdouble  sc;
  ppm_t    p = {0, 0, NULL};

  ppm_load (pcvals.selected_paper, &p);
  sc = p.width > p.height ? p.width : p.height;
  sc = 100.0 / sc;
  resize (&p, p.width*sc,p.height*sc);

  paper_preview_buffer = g_new0 (guchar, 100*100);

  for (i = 0, buf = paper_preview_buffer; i < 100; i++, buf += 100)
    {
      gint k = i * p.width * 3;

      if (i < p.height)
        {
          for (j = 0; j < p.width; j++)
            buf[j] = p.col[k + j * 3];
          if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (paper_invert)))
            for (j = 0; j < p.width; j++)
              buf[j] = 255 - buf[j];
        }
    }
  ligma_preview_area_draw (LIGMA_PREVIEW_AREA (paper_preview),
                          0, 0, 100, 100,
                          LIGMA_GRAY_IMAGE,
                          paper_preview_buffer,
                          100);

  ppm_kill (&p);
  g_free (paper_preview_buffer);

  gtk_widget_queue_draw (paper_preview);
}

static void
paper_select (GtkTreeSelection *selection,
              gpointer          data)
{
  GtkTreeIter   iter;
  GtkTreeModel *model;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gchar *paper;

      gtk_tree_model_get (model, &iter, 0, &paper, -1);

      if (paper)
        {
          gchar *fname = g_build_filename ("Paper", paper, NULL);

          g_strlcpy (pcvals.selected_paper,
                     fname, sizeof (pcvals.selected_paper));

          paper_update_preview ();

          g_free (fname);
          g_free (paper);
        }
    }
}

void
paper_restore (void)
{
  reselect (paper_list, pcvals.selected_paper);
  ligma_label_spin_set_value (LIGMA_LABEL_SPIN (paper_relief_adjust), pcvals.paper_relief);
  ligma_label_spin_set_value (LIGMA_LABEL_SPIN (paper_scale_adjust), pcvals.paper_scale);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (paper_invert), pcvals.paper_invert);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (paper_overlay), pcvals.paper_overlay);
}

void
paper_store (void)
{
  pcvals.paper_invert = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (paper_invert));
  pcvals.paper_overlay = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (paper_overlay));
}

void
create_paperpage (GtkNotebook *notebook)
{
  GtkWidget         *box1, *thispage, *box2;
  GtkWidget        *label, *tmpw, *grid;
  GtkWidget        *view;
  GtkWidget        *frame;
  GtkTreeSelection *selection;
  GtkTreeIter       iter;
  GtkListStore     *paper_store_list;

  label = gtk_label_new_with_mnemonic (_("P_aper"));

  thispage = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show (thispage);

  box1 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (thispage), box1, TRUE, TRUE, 0);
  gtk_widget_show (box1);

  paper_list = view = create_one_column_list (box1, paper_select);
  paper_store_list =
    GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (view)));
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  box2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (box2), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  paper_preview = tmpw = ligma_preview_area_new ();
  gtk_widget_set_size_request (tmpw, 100, 100);
  gtk_container_add (GTK_CONTAINER (frame), tmpw);
  gtk_widget_show (tmpw);

  paper_invert = tmpw = gtk_check_button_new_with_mnemonic ( _("_Invert"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_widget_show (tmpw);
  g_signal_connect_swapped (tmpw, "clicked",
                            G_CALLBACK (paper_select), selection);
  ligma_help_set_help_data (tmpw, _("Inverts the Papers texture"), NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw),
                                pcvals.paper_invert);

  paper_overlay = tmpw = gtk_check_button_new_with_mnemonic ( _("O_verlay"));
  gtk_box_pack_start (GTK_BOX (box2), tmpw, FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_widget_show (tmpw);
  ligma_help_set_help_data
    (tmpw, _("Applies the paper as it is (without embossing it)"), NULL);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw),
                                pcvals.paper_overlay);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (thispage), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  paper_scale_adjust =
    ligma_scale_entry_new (_("Scale:"), pcvals.paper_scale, 3.0, 150.0, 1);
  ligma_help_set_help_data (paper_scale_adjust,
                           _("Specifies the scale of the texture (in percent of original file)"),
                           NULL);
  g_signal_connect (paper_scale_adjust, "value-changed",
                    G_CALLBACK (ligmaressionist_scale_entry_update_double),
                    &pcvals.paper_scale);
  gtk_grid_attach (GTK_GRID (grid), paper_scale_adjust, 0, 0, 3, 1);
  gtk_widget_show (paper_scale_adjust);

  paper_relief_adjust =
    ligma_scale_entry_new (_("Relief:"), pcvals.paper_relief, 0.0, 100.0, 1);
  ligma_help_set_help_data (paper_relief_adjust,
                           _("Specifies the amount of embossing to apply to the image (in percent)"),
                           NULL);
  g_signal_connect (paper_relief_adjust, "value-changed",
                    G_CALLBACK (ligmaressionist_scale_entry_update_double),
                    &pcvals.paper_relief);
  gtk_grid_attach (GTK_GRID (grid), paper_relief_adjust, 0, 1, 3, 1);
  gtk_widget_show (paper_relief_adjust);


  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (paper_store_list), &iter))
    gtk_tree_selection_select_iter (selection, &iter);

  paper_select (selection, NULL);
  readdirintolist ("Paper", view, pcvals.selected_paper);
  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
