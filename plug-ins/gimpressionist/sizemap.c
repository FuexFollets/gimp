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
#include "ppmtool.h"
#include "size.h"
#include "infile.h"

#include "preview.h"

#include "libligma/stdplugins-intl.h"


#define RESPONSE_APPLY 1

#define MAPFILE "data.out"

static GtkWidget     *smwindow;
static GtkWidget     *smvectorprev;
static GtkWidget     *smpreviewprev;
static GtkWidget     *prev_button;
static GtkWidget     *next_button;
static GtkWidget     *add_button;
static GtkWidget     *kill_button;

static GtkAdjustment *smvectprevbrightadjust = NULL;

static GtkWidget     *sizadjust = NULL;
static GtkWidget     *smstradjust = NULL;
static GtkWidget     *smstrexpadjust = NULL;
static GtkWidget     *size_voronoi = NULL;

#define OMWIDTH 150
#define OMHEIGHT 150

static smvector_t smvector[MAXSIZEVECT];
static int numsmvect = 0;

static double
getsiz_from_gui (double x, double y)
{
  return getsiz_proto (x,y, numsmvect, smvector,
                       ligma_label_spin_get_value (LIGMA_LABEL_SPIN (smstrexpadjust)),
                       gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (size_voronoi)));
}

static void
updatesmpreviewprev (void)
{
  gint         x, y;
  static ppm_t nsbuffer;
  guchar       black[3] = {0, 0, 0};
  guchar       gray[3]  = {120, 120, 120};

  if (! PPM_IS_INITED (&nsbuffer))
    {
      ppm_new (&nsbuffer, OMWIDTH, OMHEIGHT);
    }
  fill (&nsbuffer, black);

  for (y = 6; y < OMHEIGHT-4; y += 10)
    {
      for (x = 6; x < OMWIDTH-4; x += 10)
         {
           gdouble siz = 5 * getsiz_from_gui (x / (double)OMWIDTH,
                                              y / (double)OMHEIGHT);
           ppm_drawline (&nsbuffer, x-siz, y-siz, x+siz, y-siz, gray);
           ppm_drawline (&nsbuffer, x+siz, y-siz, x+siz, y+siz, gray);
           ppm_drawline (&nsbuffer, x+siz, y+siz, x-siz, y+siz, gray);
           ppm_drawline (&nsbuffer, x-siz, y+siz, x-siz, y-siz, gray);
         }
    }


  ligma_preview_area_draw (LIGMA_PREVIEW_AREA (smpreviewprev),
                          0, 0, OMWIDTH, OMHEIGHT,
                          LIGMA_RGB_IMAGE,
                          nsbuffer.col,
                          OMWIDTH * 3);
}

static gint selectedsmvector = 0;
static ppm_t update_vector_preview_backup = {0,0,NULL};
static ppm_t update_vector_preview_sbuffer = {0,0,NULL};

static void
updatesmvectorprev (void)
{
  static int     ok = 0;
  gint           i, x, y;
  gdouble        val;
  static gdouble last_val = 0.0;
  guchar         gray[3]  = {120, 120, 120};
  guchar         red[3]   = {255, 0, 0};
  guchar         white[3] = {255, 255, 255};

  if (smvectprevbrightadjust)
    val = 1.0 - gtk_adjustment_get_value (smvectprevbrightadjust) / 100.0;
  else
    val = 0.5;

  if (!ok || (val != last_val))
    {
#if 0
      if (!PPM_IS_INITED (&infile))
         updatepreview (NULL, (void *)2); /* Force grabarea () */
      ppm_copy (&infile, &update_vector_preview_backup);
#else
      infile_copy_to_ppm (&update_vector_preview_backup);
#endif
      ppm_apply_brightness (&update_vector_preview_backup, val, 1,1,1);

      if (update_vector_preview_backup.width != OMWIDTH ||
          update_vector_preview_backup.height != OMHEIGHT)
         resize_fast (&update_vector_preview_backup, OMWIDTH, OMHEIGHT);

      ok = 1;
  }
  ppm_copy (&update_vector_preview_backup, &update_vector_preview_sbuffer);

  for (i = 0; i < numsmvect; i++)
    {
      x = smvector[i].x * OMWIDTH;
      y = smvector[i].y * OMHEIGHT;
      if (i == selectedsmvector)
      {
         ppm_drawline (&update_vector_preview_sbuffer, x-5, y, x+5, y, red);
         ppm_drawline (&update_vector_preview_sbuffer, x, y-5, x, y+5, red);
      }
      else
      {
         ppm_drawline (&update_vector_preview_sbuffer, x-5, y, x+5, y, gray);
         ppm_drawline (&update_vector_preview_sbuffer, x, y-5, x, y+5, gray);
      }
      ppm_put_rgb (&update_vector_preview_sbuffer, x, y, white);
  }

  ligma_preview_area_draw (LIGMA_PREVIEW_AREA (smvectorprev),
                          0, 0, OMWIDTH, OMHEIGHT,
                          LIGMA_RGB_IMAGE,
                          update_vector_preview_sbuffer.col,
                          OMWIDTH * 3);

  gtk_widget_set_sensitive (prev_button, (numsmvect > 1));
  gtk_widget_set_sensitive (next_button, (numsmvect > 1));
  gtk_widget_set_sensitive (add_button, (numsmvect < MAXORIENTVECT));
  gtk_widget_set_sensitive (kill_button, (numsmvect > 1));
}

void
size_map_free_resources (void)
{
  ppm_kill (&update_vector_preview_backup);
  ppm_kill (&update_vector_preview_sbuffer);
}

static gboolean smadjignore = FALSE;

static void
updatesmsliders (void)
{
  smadjignore = TRUE;
  ligma_label_spin_set_value (LIGMA_LABEL_SPIN (sizadjust), smvector[selectedsmvector].siz);
  ligma_label_spin_set_value (LIGMA_LABEL_SPIN (smstradjust), smvector[selectedsmvector].str);
  smadjignore = FALSE;
}

static void
smprevclick (GtkWidget *w, gpointer data)
{
  selectedsmvector--;
  if (selectedsmvector < 0)
    selectedsmvector = numsmvect-1;
  updatesmsliders ();
  updatesmvectorprev ();
}

static void
smnextclick (GtkWidget *w, gpointer data)
{
  selectedsmvector++;

  if (selectedsmvector == numsmvect)
    selectedsmvector = 0;
  updatesmsliders ();
  updatesmvectorprev ();
}

static void
smaddclick (GtkWidget *w, gpointer data)
{
  smvector[numsmvect].x = 0.5;
  smvector[numsmvect].y = 0.5;
  smvector[numsmvect].siz = 50.0;
  smvector[numsmvect].str = 1.0;
  selectedsmvector = numsmvect;
  numsmvect++;
  updatesmsliders ();
  updatesmvectorprev ();
  updatesmpreviewprev ();
}

static void
smdeleteclick (GtkWidget *w, gpointer data)
{
  int i;

  for (i = selectedsmvector; i < numsmvect-1; i++)
    {
      smvector[i] = smvector[i+1];
    }
  numsmvect--;
  if (selectedsmvector >= numsmvect)
    selectedsmvector = 0;
  updatesmsliders ();
  updatesmvectorprev ();
  updatesmpreviewprev ();
}

static void
smmapclick (GtkWidget *w, GdkEventButton *event)
{
  if (event->button == 1)
    {
      smvector[selectedsmvector].x = event->x / (double)OMWIDTH;
      smvector[selectedsmvector].y = event->y / (double)OMHEIGHT;
    }
  else if (event->button == 2)
    {
      if (numsmvect + 1 == MAXSIZEVECT)
        return;
      smvector[numsmvect].x = event->x / (double)OMWIDTH;
      smvector[numsmvect].y = event->y / (double)OMHEIGHT;
      smvector[numsmvect].siz = 0.0;
      smvector[numsmvect].str = 1.0;
      selectedsmvector = numsmvect;
      numsmvect++;
      updatesmsliders ();
    }
#if 0
  else if (event->button == 3)
    {
      double d;
      d = atan2 (OMWIDTH * smvector[selectedsmvector].x - event->x,
                 OMHEIGHT * smvector[selectedsmvector].y - event->y);
      smvector[selectedsmvector].dir = radtodeg (d);
      updatesmsliders ();
    */
  }
#endif
  updatesmvectorprev ();
  updatesmpreviewprev ();
}

static void
angsmadjmove (GtkWidget *w, gpointer data)
{
  if (!smadjignore)
    {
      smvector[selectedsmvector].siz = ligma_label_spin_get_value (LIGMA_LABEL_SPIN (sizadjust));
      updatesmvectorprev ();
      updatesmpreviewprev ();
    }
}

static void
strsmadjmove (GtkWidget *w, gpointer data)
{
  if (!smadjignore)
    {
      smvector[selectedsmvector].str = ligma_label_spin_get_value (LIGMA_LABEL_SPIN (smstradjust));
      updatesmvectorprev ();
      updatesmpreviewprev ();
    }
}

static void
smstrexpsmadjmove (GtkWidget *w, gpointer data)
{
  if (!smadjignore)
    {
      updatesmvectorprev ();
      updatesmpreviewprev ();
    }
}

static void
smresponse (GtkWidget *widget,
            gint       response_id,
            gpointer   data)
{
  switch (response_id)
    {
    case RESPONSE_APPLY:
    case GTK_RESPONSE_OK:
      {
        gint i;

        for (i = 0; i < numsmvect; i++)
          pcvals.size_vectors[i] = smvector[i];

        pcvals.num_size_vectors = numsmvect;
        pcvals.size_strength_exponent = ligma_label_spin_get_value (LIGMA_LABEL_SPIN (smstrexpadjust));
        pcvals.size_voronoi = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (size_voronoi));
      }
      break;
    }

  if (response_id != RESPONSE_APPLY)
    gtk_widget_hide (widget);
}

static void
initsmvectors (void)
{
  if (pcvals.num_size_vectors)
    {
      gint i;

      numsmvect = pcvals.num_size_vectors;
      for (i = 0; i < numsmvect; i++)
         {
           smvector[i] = pcvals.size_vectors[i];
         }
    }
  else
    {
      /* Shouldn't happen */
      numsmvect = 1;
      smvector[0].x = 0.5;
      smvector[0].y = 0.5;
      smvector[0].siz = 0.0;
      smvector[0].str = 1.0;
    }
  if (selectedsmvector >= numsmvect)
     selectedsmvector = numsmvect-1;
}

#if 0
static void
update_sizemap_dialog (void)
{
  if (smwindow)
    {
      initsmvectors ();

      gtk_adjustment_set_value (smstrexpadjust, pcvals.size_strength_exponent);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (size_voronoi),
                                    pcvals.size_voronoi);

      updatesmvectorprev ();
      updatesmpreviewprev ();
    }
}
#endif

void
create_sizemap_dialog (GtkWidget *parent)
{
  GtkWidget *tmpw, *tmpw2;
  GtkWidget *grid1;
  GtkWidget *grid2;
  GtkWidget *hbox;

  initsmvectors ();

  if (smwindow)
    {
      updatesmvectorprev ();
      updatesmpreviewprev ();
      gtk_window_present (GTK_WINDOW (smwindow));
      return;
    }

  smwindow = ligma_dialog_new (_("Size Map Editor"), PLUG_IN_ROLE,
                              gtk_widget_get_toplevel (parent), 0,
                              ligma_standard_help_func, PLUG_IN_PROC,

                              _("_Apply"),  RESPONSE_APPLY,
                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                              _("_OK"),     GTK_RESPONSE_OK,

                              NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (smwindow),
                                           GTK_RESPONSE_OK,
                                           RESPONSE_APPLY,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (smwindow, "response",
                    G_CALLBACK (smresponse),
                    NULL);
  g_signal_connect (smwindow, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &smwindow);

  grid1 = gtk_grid_new ();
  gtk_container_set_border_width (GTK_CONTAINER (grid1), 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (smwindow))),
                      grid1, TRUE, TRUE, 0);
  gtk_widget_show (grid1);

  tmpw2 = tmpw = gtk_frame_new (_("Smvectors"));
  gtk_container_set_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_grid_attach (GTK_GRID (grid1), tmpw, 0, 0, 1, 1);
  gtk_widget_show (tmpw);

  tmpw = hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,0);
  gtk_container_add (GTK_CONTAINER (tmpw2), tmpw);
  gtk_widget_show (tmpw);

  tmpw = gtk_event_box_new ();
  ligma_help_set_help_data (tmpw, _("The smvector-field. Left-click to move selected smvector, Right-click to point it towards mouse, Middle-click to add a new smvector."), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), tmpw, FALSE, FALSE, 0);
  tmpw2 = tmpw;

  tmpw = smvectorprev = ligma_preview_area_new ();
  gtk_widget_set_size_request (tmpw, OMWIDTH, OMHEIGHT);
  gtk_container_add (GTK_CONTAINER (tmpw2), tmpw);
  gtk_widget_show (tmpw);
  gtk_widget_add_events (tmpw2, GDK_BUTTON_PRESS_MASK);
  g_signal_connect (tmpw2, "button-press-event",
                    G_CALLBACK (smmapclick), NULL);
  gtk_widget_show (tmpw2);

  smvectprevbrightadjust =
    gtk_adjustment_new (50.0, 0.0, 100.0, 1.0, 1.0, 1.0);
  tmpw = gtk_scale_new (GTK_ORIENTATION_VERTICAL, smvectprevbrightadjust);
  gtk_scale_set_draw_value (GTK_SCALE (tmpw), FALSE);
  gtk_box_pack_start (GTK_BOX (hbox), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);
  g_signal_connect (smvectprevbrightadjust, "value-changed",
                    G_CALLBACK (updatesmvectorprev), NULL);
  ligma_help_set_help_data (tmpw, _("Adjust the preview's brightness"), NULL);

  tmpw2 = tmpw = gtk_frame_new (_("Preview"));
  gtk_container_set_border_width (GTK_CONTAINER (tmpw), 2);
  gtk_grid_attach (GTK_GRID (grid1), tmpw, 1, 0, 1, 1);
  gtk_widget_show (tmpw);

  tmpw = smpreviewprev = ligma_preview_area_new ();
  gtk_widget_set_size_request (tmpw, OMWIDTH, OMHEIGHT);
  gtk_container_add (GTK_CONTAINER (tmpw2), tmpw);
  gtk_widget_show (tmpw);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_grid_attach (GTK_GRID (grid1), hbox, 0, 1, 1, 1);
  gtk_widget_show (hbox);

  prev_button = tmpw = gtk_button_new_with_mnemonic ("_<<");
  gtk_box_pack_start (GTK_BOX (hbox), tmpw, FALSE, TRUE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
                    G_CALLBACK (smprevclick), NULL);
  ligma_help_set_help_data (tmpw, _("Select previous smvector"), NULL);

  next_button = tmpw = gtk_button_new_with_mnemonic ("_>>");
  gtk_box_pack_start (GTK_BOX (hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
                    G_CALLBACK (smnextclick), NULL);
  ligma_help_set_help_data (tmpw, _("Select next smvector"), NULL);

  add_button = tmpw = gtk_button_new_with_mnemonic ( _("A_dd"));
  gtk_box_pack_start (GTK_BOX (hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
                    G_CALLBACK (smaddclick), NULL);
  ligma_help_set_help_data (tmpw, _("Add new smvector"), NULL);

  kill_button = tmpw = gtk_button_new_with_mnemonic (_("_Kill"));
  gtk_box_pack_start (GTK_BOX (hbox),tmpw,FALSE,TRUE,0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
                    G_CALLBACK (smdeleteclick), NULL);
  ligma_help_set_help_data (tmpw, _("Delete selected smvector"), NULL);

  grid2 = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid2), 4);
  gtk_grid_attach (GTK_GRID (grid1), grid2, 0, 2, 2, 1);
  gtk_widget_show (grid2);

  sizadjust =
    ligma_scale_entry_new (_("_Size:"), 50.0, 0.0, 100.0, 1);
  ligma_help_set_help_data (sizadjust,
                           _("Change the angle of the selected smvector"),
                           NULL);
  g_signal_connect (sizadjust, "value-changed",
                    G_CALLBACK (angsmadjmove), NULL);
  gtk_grid_attach (GTK_GRID (grid2), sizadjust, 0, 0, 3, 1);
  gtk_widget_show (sizadjust);

  smstradjust =
    ligma_scale_entry_new (_("S_trength:"), 1.0, 0.1, 5.0, 1);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (smstradjust), 0.1, 0.5);
  ligma_help_set_help_data (smstradjust,
                           _("Change the strength of the selected smvector"),
                           NULL);
  g_signal_connect (smstradjust, "value-changed",
                    G_CALLBACK (strsmadjmove), NULL);
  gtk_grid_attach (GTK_GRID (grid2), smstradjust, 0, 1, 3, 1);
  gtk_widget_show (smstradjust);

  smstrexpadjust =
    ligma_scale_entry_new (_("St_rength exp.:"), 1.0, 0.1, 10.9, 1);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (smstradjust), 0.1, 0.5);
  ligma_help_set_help_data (smstrexpadjust,
                           _("Change the exponent of the strength"),
                           NULL);
  g_signal_connect (smstrexpadjust, "value-changed",
                    G_CALLBACK (smstrexpsmadjmove), NULL);
  gtk_grid_attach (GTK_GRID (grid2), smstrexpadjust, 0, 2, 3, 1);
  gtk_widget_show (smstrexpadjust);

  size_voronoi = tmpw = gtk_check_button_new_with_mnemonic ( _("_Voronoi"));
  gtk_grid_attach (GTK_GRID (grid2), tmpw, 3, 0, 1, 1);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw), FALSE);
  gtk_widget_show (tmpw);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw), pcvals.size_voronoi);
  g_signal_connect (tmpw, "clicked",
                    G_CALLBACK (smstrexpsmadjmove), NULL);
  ligma_help_set_help_data (tmpw, _("Voronoi-mode makes only the smvector closest to the given point have any influence"), NULL);

  gtk_widget_show (smwindow);

  updatesmvectorprev ();
  updatesmpreviewprev ();
}
