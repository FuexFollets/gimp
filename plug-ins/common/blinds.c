/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for LIGMA.
 *
 * Blinds plug-in. Distort an image as though it was stuck to
 * window blinds and the blinds where opened/closed.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 * A fair proportion of this code was taken from the Whirl plug-in
 * which was copyrighted by Federico Mena Quintero (as below).
 *
 * Whirl plug-in --- distort an image into a whirlpool
 * Copyright (C) 1997 Federico Mena Quintero
 *
 */

#include "config.h"

#include <string.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"

/***** Magic numbers *****/

#define PLUG_IN_PROC   "plug-in-blinds"
#define PLUG_IN_BINARY "blinds"
#define PLUG_IN_ROLE   "ligma-blinds"

#define MAX_FANS       100

/* Variables set in dialog box */
typedef struct data
{
  gint                 angledsp;
  gint                 numsegs;
  LigmaOrientationType  orientation;
  gboolean bg_trans;
} BlindVals;


typedef struct _Blinds      Blinds;
typedef struct _BlindsClass BlindsClass;

struct _Blinds
{
  LigmaPlugIn parent_instance;
};

struct _BlindsClass
{
  LigmaPlugInClass parent_class;
};


#define BLINDS_TYPE  (blinds_get_type ())
#define BLINDS (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BLINDS_TYPE, Blinds))

GType                   blinds_get_type         (void) G_GNUC_CONST;

static GList          * blinds_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * blinds_create_procedure (LigmaPlugIn           *plug_in,
                                                 const gchar          *name);

static LigmaValueArray * blinds_run              (LigmaProcedure        *procedure,
                                                 LigmaRunMode           run_mode,
                                                 LigmaImage            *image,
                                                 gint                  n_drawables,
                                                 LigmaDrawable        **drawables,
                                                 const LigmaValueArray *args,
                                                 gpointer              run_data);

static gboolean         blinds_dialog           (LigmaDrawable         *drawable);
static void       blinds_scale_entry_update_int (LigmaLabelSpin        *entry,
                                                 gint                 *value);

static void             dialog_update_preview   (LigmaDrawable         *drawable,
                                                 LigmaPreview          *preview);
static void             apply_blinds            (LigmaDrawable         *drawable);


G_DEFINE_TYPE (Blinds, blinds, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (BLINDS_TYPE)
DEFINE_STD_SET_I18N


/* Array to hold each size of fans. And no there are not each the
 * same size (rounding errors...)
 */

static gint fanwidths[MAX_FANS];

/* Values when first invoked */
static BlindVals bvals =
{
  30,
  3,
  LIGMA_ORIENTATION_HORIZONTAL,
  FALSE
};


static void
blinds_class_init (BlindsClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = blinds_query_procedures;
  plug_in_class->create_procedure = blinds_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
blinds_init (Blinds *blinds)
{
}

static GList *
blinds_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
blinds_create_procedure (LigmaPlugIn  *plug_in,
                         const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            blinds_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("_Blinds..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Filters/Distorts");

      ligma_procedure_set_documentation (procedure,
                                        _("Simulate an image painted on "
                                          "window blinds"),
                                        "More here later",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Andy Thomas",
                                      "Andy Thomas",
                                      "1997");

      LIGMA_PROC_ARG_INT (procedure, "angle-displacement",
                         "Angle displacement",
                         "Angle of Displacement",
                         0, 360, 30,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "num-segments",
                         "Num segments",
                         "Number of segments in blinds",
                         1, 1024, 3,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_ENUM (procedure, "orientation",
                          "Orientation",
                          "The orientation",
                          LIGMA_TYPE_ORIENTATION_TYPE,
                          LIGMA_ORIENTATION_HORIZONTAL,
                          G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "bg-transparent",
                             "BG transparent",
                             "Background transparent",
                             FALSE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
blinds_run (LigmaProcedure        *procedure,
            LigmaRunMode           run_mode,
            LigmaImage            *image,
            gint                  n_drawables,
            LigmaDrawable        **drawables,
            const LigmaValueArray *args,
            gpointer              run_data)
{
  LigmaDrawable *drawable;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
      ligma_get_data (PLUG_IN_PROC, &bvals);

      if (! blinds_dialog (drawable))
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    case LIGMA_RUN_NONINTERACTIVE:
      bvals.angledsp    = LIGMA_VALUES_GET_INT     (args, 0);
      bvals.numsegs     = LIGMA_VALUES_GET_INT     (args, 1);
      bvals.orientation = LIGMA_VALUES_GET_ENUM    (args, 2);
      bvals.bg_trans    = LIGMA_VALUES_GET_BOOLEAN (args, 3);
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_get_data (PLUG_IN_PROC, &bvals);
      break;

    default:
      break;
    }

  if (ligma_drawable_is_rgb  (drawable) ||
      ligma_drawable_is_gray (drawable))
    {
      ligma_progress_init (_("Adding blinds"));

      apply_blinds (drawable);

      if (run_mode != LIGMA_RUN_NONINTERACTIVE)
        ligma_displays_flush ();

      if (run_mode == LIGMA_RUN_INTERACTIVE)
        ligma_set_data (PLUG_IN_PROC, &bvals, sizeof (BlindVals));
    }
  else
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}


static gboolean
blinds_dialog (LigmaDrawable *drawable)
{
  GtkWidget *dialog;
  GtkWidget *main_vbox;
  GtkWidget *preview;
  GtkWidget *hbox;
  GtkWidget *frame;
  GtkWidget *grid;
  GtkWidget *scale;
  GtkWidget *toggle;
  GtkWidget *horizontal;
  GtkWidget *vertical;
  gboolean   run;

  ligma_ui_init (PLUG_IN_BINARY);

  dialog = ligma_dialog_new (_("Blinds"), PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  ligma_window_set_transient (GTK_WINDOW (dialog));

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  preview = ligma_aspect_preview_new_from_drawable (drawable);
  gtk_box_pack_start (GTK_BOX (main_vbox), preview, TRUE, TRUE, 0);
  gtk_widget_show (preview);

  g_signal_connect_swapped (preview, "invalidated",
                            G_CALLBACK (dialog_update_preview),
                            drawable);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  frame =
    ligma_int_radio_group_new (TRUE, _("Orientation"),
                              G_CALLBACK (ligma_radio_button_update),
                              &bvals.orientation, NULL, bvals.orientation,

                              _("_Horizontal"), LIGMA_ORIENTATION_HORIZONTAL,
                              &horizontal,

                              _("_Vertical"),   LIGMA_ORIENTATION_VERTICAL,
                              &vertical,

                              NULL);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  g_signal_connect_swapped (horizontal, "toggled",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);
  g_signal_connect_swapped (vertical, "toggled",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  frame = ligma_frame_new (_("Background"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_check_button_new_with_mnemonic (_("_Transparent"));
  gtk_container_add (GTK_CONTAINER (frame), toggle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &bvals.bg_trans);
  g_signal_connect_swapped (toggle, "toggled",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), bvals.bg_trans);

  if (! ligma_drawable_has_alpha (drawable))
    {
      gtk_widget_set_sensitive (toggle, FALSE);
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), FALSE);
    }

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (main_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  scale = ligma_scale_entry_new (_("_Displacement:"), bvals.angledsp, 1, 90, 0);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 1.0, 15.0);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (blinds_scale_entry_update_int),
                    &bvals.angledsp);
  g_signal_connect_swapped (scale, "value-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 2);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("_Number of segments:"), bvals.numsegs, 1, MAX_FANS, 0);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (blinds_scale_entry_update_int),
                    &bvals.numsegs);
  g_signal_connect_swapped (scale, "value-changed",
                            G_CALLBACK (ligma_preview_invalidate),
                            preview);
  gtk_box_pack_start (GTK_BOX (main_vbox), scale, FALSE, FALSE, 2);
  gtk_widget_show (scale);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  gtk_widget_destroy (dialog);

  return run;
}

static void
blinds_scale_entry_update_int (LigmaLabelSpin *entry,
                               gint          *value)
{
  *value = (gint) ligma_label_spin_get_value (entry);
}

static void
blindsapply (guchar *srow,
             guchar *drow,
             gint    width,
             gint    bpp,
             guchar *bg)
{
  guchar  *src;
  guchar  *dst;
  gint     i,j,k;
  gdouble  ang;
  gint     available;

  /* Make the row 'shrink' around points along its length */
  /* The bvals.numsegs determines how many segments to slip it in to */
  /* The angle is the conceptual 'rotation' of each of these segments */

  /* Note the row is considered to be made up of a two dim array actual
   * pixel locations and the RGB color at these locations.
   */

  /* In the process copy the src row to the destination row */

  /* Fill in with background color ? */
  for (i = 0 ; i < width ; i++)
    {
      dst = &drow[i*bpp];

      for (j = 0 ; j < bpp; j++)
        {
          dst[j] = bg[j];
        }
    }

  /* Apply it */

  available = width;
  for (i = 0; i < bvals.numsegs; i++)
    {
      /* Width of segs are variable */
      fanwidths[i] = available / (bvals.numsegs - i);
      available -= fanwidths[i];
    }

  /* do center points  first - just for fun...*/
  available = 0;
  for (k = 1; k <= bvals.numsegs; k++)
    {
      int point;

      point = available + fanwidths[k - 1] / 2;

      available += fanwidths[k - 1];

      src = &srow[point * bpp];
      dst = &drow[point * bpp];

      /* Copy pixels across */
      for (j = 0 ; j < bpp; j++)
        {
          dst[j] = src[j];
        }
    }

  /* Disp for each point */
  ang = (bvals.angledsp * 2 * G_PI) / 360; /* Angle in rads */
  ang = (1 - fabs (cos (ang)));

  available = 0;
  for (k = 0 ; k < bvals.numsegs; k++)
    {
      int dx; /* Amount to move by */
      int fw;

      for (i = 0 ; i < (fanwidths[k]/2) ; i++)
        {
          /* Copy pixels across of left half of fan */
          fw = fanwidths[k] / 2;
          dx = (int) (ang * ((double) (fw - (double)(i % fw))));

          src = &srow[(available + i) * bpp];
          dst = &drow[(available + i + dx) * bpp];

          for (j = 0; j < bpp; j++)
            {
              dst[j] = src[j];
            }

          /* Right side */
          j = i + 1;
          src = &srow[(available + fanwidths[k] - j
                       - (fanwidths[k] % 2)) * bpp];
          dst = &drow[(available + fanwidths[k] - j
                       - (fanwidths[k] % 2) - dx) * bpp];

          for (j = 0; j < bpp; j++)
            {
              dst[j] = src[j];
            }
        }

      available += fanwidths[k];
    }
}

static void
dialog_update_preview (LigmaDrawable *drawable,
                       LigmaPreview  *preview)
{
  gint     y;
  guchar  *p, *buffer, *cache;
  LigmaRGB  background;
  guchar   bg[4];
  gint     width, height, bpp;

  ligma_preview_get_size (preview, &width, &height);
  cache = ligma_drawable_get_thumbnail_data (drawable,
                                            &width, &height, &bpp);
  p = cache;

  ligma_context_get_background (&background);

  if (bvals.bg_trans)
    ligma_rgb_set_alpha (&background, 0.0);

  if (ligma_drawable_is_gray (drawable))
    {
      bg[0] = ligma_rgb_luminance_uchar (&background);
      ligma_rgba_get_uchar (&background, NULL, NULL, NULL, bg + 3);
    }
  else
    {
      ligma_rgba_get_uchar (&background, bg, bg + 1, bg + 2, bg + 3);
    }

  buffer = g_new (guchar, width * height * bpp);

  if (bvals.orientation == LIGMA_ORIENTATION_VERTICAL)
    {
      for (y = 0; y < height; y++)
        {
          blindsapply (p,
                       buffer + y * width * bpp,
                       width,
                       bpp, bg);
          p += width * bpp;
        }
    }
  else
    {
      /* Horizontal blinds */
      /* Apply the blinds algo to a single column -
       * this act as a transformation matrix for the
       * rows. Make row 0 invalid so we can find it again!
       */
      gint i;
      guchar *sr = g_new (guchar, height * 4);
      guchar *dr = g_new0 (guchar, height * 4);
      guchar dummybg[4] = {0, 0, 0, 0};

      /* Fill in with background color ? */
      for (i = 0 ; i < width ; i++)
        {
          gint j;
          gint bd = bpp;
          guchar *dst;
          dst = &buffer[i * bd];

          for (j = 0 ; j < bd; j++)
            {
              dst[j] = bg[j];
            }
        }

      for ( y = 0 ; y < height; y++)
        {
          sr[y] = y+1;
        }

      /* Bit of a fiddle since blindsapply really works on an image
       * row not a set of bytes. - preview can't be > 255
       * or must make dr sr int rows.
       */
      blindsapply (sr, dr, height, 1, dummybg);

      for (y = 0; y < height; y++)
        {
          if (dr[y] == 0)
            {
              /* Draw background line */
              p = buffer;
            }
          else
            {
              /* Draw line from src */
              p = cache +
                (width * bpp * (dr[y] - 1));
            }
          memcpy (buffer + y * width * bpp,
                  p,
                  width * bpp);
        }
      g_free (sr);
      g_free (dr);
    }

  ligma_preview_draw_buffer (preview, buffer, width * bpp);

  g_free (buffer);
  g_free (cache);
}

/* STEP tells us how many rows/columns to gulp down in one go... */
/* Note all the "4" literals around here are to do with the depths
 * of the images. Makes it easier to deal with for my small brain.
 */

#define STEP 40

static void
apply_blinds (LigmaDrawable *drawable)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  guchar     *src_rows, *des_rows;
  gint        bytes;
  gint        x, y;
  LigmaRGB     background;
  guchar      bg[4];
  gint        sel_x1, sel_y1;
  gint        sel_width, sel_height;

  ligma_context_get_background (&background);

  if (bvals.bg_trans)
    ligma_rgb_set_alpha (&background, 0.0);

  ligma_rgba_get_uchar (&background, bg, bg + 1, bg + 2, bg + 3);

  if (! ligma_drawable_mask_intersect (drawable,
                                      &sel_x1, &sel_y1,
                                      &sel_width, &sel_height))
    return;

  if (ligma_drawable_has_alpha (drawable))
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  bytes = babl_format_get_bytes_per_pixel (format);

  src_buffer  = ligma_drawable_get_buffer (drawable);
  dest_buffer = ligma_drawable_get_shadow_buffer (drawable);

  src_rows = g_new (guchar, MAX (sel_width, sel_height) * bytes * STEP);
  des_rows = g_new (guchar, MAX (sel_width, sel_height) * bytes * STEP);

  if (bvals.orientation == LIGMA_ORIENTATION_VERTICAL)
    {
      for (y = 0; y < sel_height; y += STEP)
        {
          gint rr;
          gint step;

          if ((y + STEP) > sel_height)
            step = sel_height - y;
          else
            step = STEP;

          gegl_buffer_get (src_buffer,
                           GEGL_RECTANGLE (sel_x1, sel_y1 + y,
                                           sel_width, step), 1.0,
                           format, src_rows,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /* OK I could make this better */
          for (rr = 0; rr < STEP; rr++)
            blindsapply (src_rows + (sel_width * rr * bytes),
                         des_rows + (sel_width * rr * bytes),
                         sel_width, bytes, bg);

          gegl_buffer_set (dest_buffer,
                           GEGL_RECTANGLE (sel_x1, sel_y1 + y,
                                           sel_width, step), 0,
                           format, des_rows,
                           GEGL_AUTO_ROWSTRIDE);

          ligma_progress_update ((double) y / (double) sel_height);
        }
    }
  else
    {
      /* Horizontal blinds */
      /* Apply the blinds algo to a single column -
       * this act as a transformation matrix for the
       * rows. Make row 0 invalid so we can find it again!
       */
      gint    i;
      gint   *sr  = g_new (gint, sel_height * bytes);
      gint   *dr  = g_new (gint, sel_height * bytes);
      guchar *dst = g_new (guchar, STEP * bytes);
      guchar  dummybg[4];

      memset (dummybg, 0, 4);
      memset (dr, 0, sel_height * bytes); /* all dr rows are background rows */
      for (y = 0; y < sel_height; y++)
        {
          sr[y] = y+1;
        }

      /* Hmmm. does this work portably? */
      /* This "swaps" the integers around that are held in in the
       * sr & dr arrays.
       */
      blindsapply ((guchar *) sr, (guchar *) dr,
                   sel_height, sizeof (gint), dummybg);

      /* Fill in with background color ? */
      for (i = 0 ; i < STEP ; i++)
        {
          int     j;
          guchar *bgdst;
          bgdst = &dst[i * bytes];

          for (j = 0 ; j < bytes; j++)
            {
              bgdst[j] = bg[j];
            }
        }

      for (x = 0; x < sel_width; x += STEP)
        {
          int     rr;
          int     step;
          guchar *p;

          if((x + STEP) > sel_width)
            step = sel_width - x;
          else
            step = STEP;

          gegl_buffer_get (src_buffer,
                           GEGL_RECTANGLE (sel_x1 + x, sel_y1,
                                           step, sel_height), 1.0,
                           format, src_rows,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /* OK I could make this better */
          for (rr = 0; rr < sel_height; rr++)
            {
              if(dr[rr] == 0)
                {
                  /* Draw background line */
                  p = dst;
                }
              else
                {
                  /* Draw line from src */
                  p = src_rows + (step * bytes * (dr[rr] - 1));
                }
              memcpy (des_rows + (rr * step * bytes), p,
                      step * bytes);
            }

          gegl_buffer_set (dest_buffer,
                           GEGL_RECTANGLE (sel_x1 + x, sel_y1,
                                           step, sel_height), 0,
                           format, des_rows,
                           GEGL_AUTO_ROWSTRIDE);

          ligma_progress_update ((double) x / (double) sel_width);
        }

      g_free (dst);
      g_free (sr);
      g_free (dr);
    }

  g_free (src_rows);
  g_free (des_rows);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  ligma_progress_update (1.0);

  ligma_drawable_merge_shadow (drawable, TRUE);
  ligma_drawable_update (drawable,
                        sel_x1, sel_y1, sel_width, sel_height);
}
