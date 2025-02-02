/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for LIGMA.
 *
 * Plugin to convert a selection to a path.
 *
 * Copyright (C) 1999 Andy Thomas  alt@ligma.org
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
 *
 */

/* Change log:-
 * 0.1 First version.
 */

#include "config.h"

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "types.h"

#include "selection-to-path.h"

#include "libligma/stdplugins-intl.h"


static GSList * adjust_widgets = NULL;

void scale_entry_update_double (LigmaLabelSpin *entry,
                                gdouble       *value);

/* Reset to recommended defaults */
void
reset_adv_dialog (void)
{
  GSList  *list;
  GObject *widget;
  gdouble *value;

  for (list = adjust_widgets; list; list = g_slist_next (list))
    {
      widget = G_OBJECT (list->data);
      value  = (gdouble *) g_object_get_data (G_OBJECT (widget),
                                              "default_value");

      if (GTK_IS_ADJUSTMENT (widget))
        {
          gtk_adjustment_set_value (GTK_ADJUSTMENT (widget),
                                    *value);
        }
      else if (GTK_IS_TOGGLE_BUTTON (widget))
        {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget),
                                        (gboolean)(*value));
        }
      else if (LIGMA_IS_SCALE_ENTRY (widget))
        {
          ligma_label_spin_set_value (LIGMA_LABEL_SPIN (widget),
                                     *value);
        }
      else
        g_warning ("Internal widget list error");
    }
}

static gpointer
def_val (gdouble default_value)
{
  gdouble *value = g_new0 (gdouble, 1);
  *value = default_value;
  return (value);
}

GtkWidget *
dialog_create_selection_area (SELVALS *sels)
{
  GtkWidget *scrolled_win;
  GtkWidget *grid;
  GtkWidget *check;
  GtkWidget *scale;
  gint       row;

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_win, -1, 400);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (scrolled_win),
                                             FALSE);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (scrolled_win), grid);
  gtk_widget_show (grid);
  row = 0;

  scale = ligma_scale_entry_new (_("Align Threshold:"), sels->align_threshold, 0.2, 2.0, 2);
  ligma_help_set_help_data (scale,
                           _("If two endpoints are closer than this, "
                             "they are made to be equal."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->align_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (0.5));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Corner Always Threshold:"), sels->corner_always_threshold, 30, 180, 2);
  ligma_help_set_help_data (scale,
                           _("If the angle defined by a point and its predecessors "
                             "and successors is smaller than this, it's a corner, "
                             "even if it's within 'corner_surround' pixels of a "
                             "point with a smaller angle."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->corner_always_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (60.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Corner Surround:"), sels->corner_surround, 3, 8, 0);
  ligma_help_set_help_data (scale,
                           _("Number of points to consider when determining if a "
                             "point is a corner or not."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->corner_surround);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (4.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Corner Threshold:"), sels->corner_threshold, 0, 180, 2);
  ligma_help_set_help_data (scale,
                           _("If a point, its predecessors, and its successors "
                             "define an angle smaller than this, it's a corner."),
                           NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->corner_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (100.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Error Threshold:"), sels->error_threshold, 0.2, 10, 2);
  ligma_help_set_help_data (scale,
                           _("Amount of error at which a fitted spline is "
                             "unacceptable. If any pixel is further away "
                             "than this from the fitted curve, we try again."),
                           NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->error_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (0.40));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Filter Alternative Surround:"), sels->filter_alternative_surround, 1, 10, 0);
  ligma_help_set_help_data (scale,
                           _("A second number of adjacent points to consider "
                             "when filtering."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->filter_alternative_surround);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (1.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Filter Epsilon:"), sels->filter_epsilon, 5, 40, 2);
  ligma_help_set_help_data (scale,
                           _("If the angles between the vectors produced by "
                             "filter_surround and filter_alternative_surround "
                             "points differ by more than this, use the one from "
                             "filter_alternative_surround."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->filter_epsilon);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (10.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Filter Iteration Count:"), sels->filter_iteration_count, 4, 70, 0);
  ligma_help_set_help_data (scale,
                           _("Number of times to smooth original data points.  "
                             "Increasing this number dramatically --- to 50 or "
                             "so --- can produce vastly better results. But if "
                             "any points that 'should' be corners aren't found, "
                             "the curve goes to hell around that point."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->filter_iteration_count);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (4.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Filter Percent:"), sels->filter_percent, 0, 1, 2);
  ligma_help_set_help_data (scale,
                           _("To produce the new point, use the old point plus "
                             "this times the neighbors."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->filter_percent);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (0.33));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Filter Secondary Surround:"), sels->filter_secondary_surround, 3, 10, 0);
  ligma_help_set_help_data (scale,
                           _("Number of adjacent points to consider if "
                             "'filter_surround' points defines a straight line."),
                           NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->filter_secondary_surround);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (3.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Filter Surround:"), sels->filter_surround, 2, 10, 0);
  ligma_help_set_help_data (scale,
                           _("Number of adjacent points to consider when filtering."),
                           NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->filter_surround);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (2.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  check = gtk_check_button_new_with_label (_("Keep Knees"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), sels->keep_knees);
  gtk_grid_attach (GTK_GRID (grid), check, 1, row, 2, 1);
  ligma_help_set_help_data (GTK_WIDGET (check),
                           _("Says whether or not to remove 'knee' "
                             "points after finding the outline."), NULL);
  g_signal_connect (check, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &sels->keep_knees);
  gtk_widget_show (check);
  adjust_widgets = g_slist_append (adjust_widgets, check);
  g_object_set_data (G_OBJECT (check), "default_value", def_val ((gdouble)FALSE));
  row++;

  scale = ligma_scale_entry_new (_("Line Reversion Threshold:"), sels->line_reversion_threshold, 0.01, 0.2, 3);
  ligma_help_set_help_data (scale,
                           _("If a spline is closer to a straight line than this, "
                             "it remains a straight line, even if it would otherwise "
                             "be changed back to a curve. This is weighted by the "
                             "square of the curve length, to make shorter curves "
                             "more likely to be reverted."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->line_reversion_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (0.01));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Line Threshold:"), sels->line_threshold, 0.2, 4, 2);
  ligma_help_set_help_data (scale,
                           _("How many pixels (on the average) a spline can "
                             "diverge from the line determined by its endpoints "
                             "before it is changed to a straight line."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->line_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (0.5));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Reparametrize Improvement:"), sels->reparameterize_improvement, 0, 1, 2);
  ligma_help_set_help_data (scale,
                           _("If reparameterization doesn't improve the fit by this "
                             "much percent, stop doing it. ""Amount of error at which "
                             "it is pointless to reparameterize."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->reparameterize_improvement);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (0.01));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Reparametrize Threshold:"), sels->reparameterize_threshold, 1, 50, 2);
  ligma_help_set_help_data (scale,
                           _("Amount of error at which it is pointless to reparameterize.  "
                             "This happens, for example, when we are trying to fit the "
                             "outline of the outside of an 'O' with a single spline. "
                             "The initial fit is not good enough for the Newton-Raphson "
                             "iteration to improve it.  It may be that it would be better "
                             "to detect the cases where we didn't find any corners."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->reparameterize_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (1.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Subdivide Search:"), sels->subdivide_search, 0.05, 1, 2);
  ligma_help_set_help_data (scale,
                           _("Percentage of the curve away from the worst point "
                             "to look for a better place to subdivide."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->subdivide_search);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (0.1));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Subdivide Surround:"), sels->subdivide_surround, 2, 10, 0);
  ligma_help_set_help_data (scale,
                           _("Number of points to consider when deciding whether "
                             "a given point is a better place to subdivide."),
                           NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->subdivide_surround);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (4.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Subdivide Threshold:"), sels->subdivide_threshold, 0.01, 1, 2);
  ligma_help_set_help_data (scale,
                           _("How many pixels a point can diverge from a straight "
                             "line and still be considered a better place to "
                             "subdivide."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->subdivide_threshold);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (0.03));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Tangent Surround:"), sels->tangent_surround, 2, 10, 0);
  ligma_help_set_help_data (scale,
                           _("Number of points to look at on either side of a "
                             "point when computing the approximation to the "
                             "tangent at that point."), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &sels->tangent_surround);
  adjust_widgets = g_slist_append (adjust_widgets, scale);
  g_object_set_data (G_OBJECT (scale), "default_value", def_val (3.0));
  gtk_grid_attach (GTK_GRID (grid), scale, 0, row++, 3, 1);
  gtk_widget_show (scale);

  return scrolled_win;
}

void
scale_entry_update_double (LigmaLabelSpin *entry,
                           gdouble       *value)
{
  *value = ligma_label_spin_get_value (entry);
}
