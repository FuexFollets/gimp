/* sample_colorize.c
 *      A LIGMA Plug-In by Wolfgang Hofer
 */

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

#include <stdlib.h>
#include <string.h>

#include <glib/gstdio.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


/* Some useful macros */
#define RESPONSE_RESET      1
#define RESPONSE_GET_COLORS 2

#define PLUG_IN_PROC        "plug-in-sample-colorize"
#define PLUG_IN_BINARY      "sample-colorize"
#define PLUG_IN_ROLE        "ligma-sample-colorize"
#define NUMBER_IN_ARGS      13

#define LUMINOSITY_0(X)      ((X[0] * 30 + X[1] * 59 + X[2] * 11))
#define LUMINOSITY_1(X)      ((X[0] * 30 + X[1] * 59 + X[2] * 11) / 100)
#define MIX_CHANNEL(a, b, m) (((a * m) + (b * (255 - m))) / 255)

#define SMP_GRADIENT         -444
#define SMP_INV_GRADIENT     -445


#define PREVIEW_BPP 3
#define PREVIEW_SIZE_X  256
#define PREVIEW_SIZE_Y  256
#define DA_WIDTH         256
#define DA_HEIGHT        25
#define GRADIENT_HEIGHT  15
#define CONTROL_HEIGHT   DA_HEIGHT - GRADIENT_HEIGHT
#define LEVELS_DA_MASK  (GDK_EXPOSURE_MASK       | \
                         GDK_ENTER_NOTIFY_MASK   | \
                         GDK_BUTTON_PRESS_MASK   | \
                         GDK_BUTTON_RELEASE_MASK | \
                         GDK_BUTTON1_MOTION_MASK | \
                         GDK_POINTER_MOTION_HINT_MASK)

#define LOW_INPUT          0x1
#define GAMMA              0x2
#define HIGH_INPUT         0x4
#define LOW_OUTPUT         0x8
#define HIGH_OUTPUT        0x10
#define INPUT_LEVELS       0x20
#define OUTPUT_LEVELS      0x40
#define INPUT_SLIDERS      0x80
#define OUTPUT_SLIDERS     0x100
#define DRAW               0x200
#define REFRESH_DST        0x400
#define ALL                0xFFF

#define MC_GET_SAMPLE_COLORS 1
#define MC_DST_REMAP         2
#define MC_ALL               (MC_GET_SAMPLE_COLORS | MC_DST_REMAP)

typedef struct
{
  LigmaDrawable *dst;
  gint32        sample_id;

  gint32 hold_inten;       /* TRUE or FALSE */
  gint32 orig_inten;       /* TRUE or FALSE */
  gint32 rnd_subcolors;    /* TRUE or FALSE */
  gint32 guess_missing;    /* TRUE or FALSE */
  gint32 lvl_in_min;       /* 0 up to 254 */
  gint32 lvl_in_max;       /* 1 up to 255 */
  float  lvl_in_gamma;     /* 0.1 up to 10.0  (1.0 == linear) */
  gint32 lvl_out_min;      /* 0 up to 254 */
  gint32 lvl_out_max;      /* 1 up to 255 */

  float  tol_col_err;      /* 0.0% up to 100.0%
                            * this is used to find out colors of the same
                            * colortone, while analyzing sample colors,
                            * It does not make much sense for the user to adjust this
                            * value. (I used a param file to find out a suitable value)
                            */
} t_values;

typedef struct
{
  GtkWidget     *dialog;
  GtkWidget     *sample_preview;
  GtkWidget     *dst_preview;
  GtkWidget     *sample_colortab_preview;
  GtkWidget     *sample_drawarea;
  GtkWidget     *in_lvl_gray_preview;
  GtkWidget     *in_lvl_drawarea;
  GtkAdjustment *adj_lvl_in_min;
  GtkAdjustment *adj_lvl_in_max;
  GtkAdjustment *adj_lvl_in_gamma;
  GtkAdjustment *adj_lvl_out_min;
  GtkAdjustment *adj_lvl_out_max;
  GtkWidget     *orig_inten_button;
  gint           active_slider;
  gint           slider_pos[5];  /*  positions for the five sliders  */

  gboolean       enable_preview_update;
  gboolean       sample_show_selection;
  gboolean       dst_show_selection;
  gboolean       sample_show_color;
  gboolean       dst_show_color;
} t_samp_interface;



typedef struct
{
  guchar  color[4];     /* R,G,B,A */
  gint32  sum_color;    /* nr. of sourcepixels with (nearly the same) color */
  void   *next;
} t_samp_color_elem;



typedef struct
{
  gint32              all_samples;  /* number of all source pixels with this luminosity */
  gint                from_sample;  /* TRUE: color found in sample, FALSE: interpolated color added */
  t_samp_color_elem  *col_ptr;       /* List of sample colors at same luminosity */
} t_samp_table_elem;


typedef struct
{
  LigmaDrawable *drawable;
  GeglBuffer   *buffer;
  const Babl   *format;
  gint          width;
  gint          height;
  void         *sel_gdrw;
  gint          x1;
  gint          y1;
  gint          x2;
  gint          y2;
  gint          index_alpha;   /* 0 == no alpha, 1 == GREYA, 3 == RGBA */
  gint          bpp;
  gint          tile_width;
  gint          tile_height;
  gint          shadow;
  gint32        seldeltax;
  gint32        seldeltay;
} t_GDRW;


typedef struct _Colorize      Colorize;
typedef struct _ColorizeClass ColorizeClass;

struct _Colorize
{
  LigmaPlugIn parent_instance;
};

struct _ColorizeClass
{
  LigmaPlugInClass parent_class;
};


#define COLORIZE_TYPE  (colorize_get_type ())
#define COLORIZE (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), COLORIZE_TYPE, Colorize))

GType                   colorize_get_type         (void) G_GNUC_CONST;

static GList          * colorize_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * colorize_create_procedure (LigmaPlugIn           *plug_in,
                                                   const gchar          *name);

static LigmaValueArray * colorize_run              (LigmaProcedure        *procedure,
                                                   LigmaRunMode           run_mode,
                                                   LigmaImage            *image,
                                                   gint                  n_drawables,
                                                   LigmaDrawable        **drawables,
                                                   const LigmaValueArray *args,
                                                   gpointer              run_data);

static gint             main_colorize             (gint          mc_flags);
static void             get_filevalues            (void);
static void             smp_dialog                (void);
static void             refresh_dst_preview       (GtkWidget    *preview,
                                                   guchar       *src_buffer);
static void             update_preview            (LigmaDrawable *drawable);
static void             clear_tables              (void);
static void             free_colors               (void);
static void             levels_update             (gint          update);
static gint             level_in_events           (GtkWidget    *widget,
                                                   GdkEvent     *event);
static gint             level_in_draw             (GtkWidget    *widget,
                                                   cairo_t      *cr);
static gint             level_out_events          (GtkWidget    *widget,
                                                   GdkEvent     *event);
static gint             level_out_draw            (GtkWidget    *widget,
                                                   cairo_t      *cr);
static void             calculate_level_transfers (void);
static void             get_pixel                 (t_GDRW       *gdrw,
                                                   gint32        x,
                                                   gint32        y,
                                                   guchar       *pixel);
static void             init_gdrw                 (t_GDRW       *gdrw,
                                                   LigmaDrawable *drawable,
                                                   gboolean      shadow);
static void             end_gdrw                  (t_GDRW       *gdrw);
static void             remap_pixel               (guchar       *pixel,
                                                   const guchar *original,
                                                   gint          bpp2);
static void             guess_missing_colors      (void);
static void             fill_missing_colors       (void);
static void             smp_get_colors            (GtkWidget    *dialog);
static void             get_gradient              (gint          mode);
static void             clear_preview             (GtkWidget    *preview);


G_DEFINE_TYPE (Colorize, colorize, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (COLORIZE_TYPE)
DEFINE_STD_SET_I18N


static t_samp_interface  g_di;  /* global dialog interface variables */
static t_values          g_values = { NULL, -1, 1, 1, 0, 1, 0, 255, 1.0, 0, 255, 5.5 };
static t_samp_table_elem g_lum_tab[256];
static guchar            g_lvl_trans_tab[256];
static guchar            g_out_trans_tab[256];
static guchar            g_sample_color_tab[256 * 3];
static guchar            g_dst_preview_buffer[PREVIEW_SIZE_X * PREVIEW_SIZE_Y * 4 ];  /* color copy with mask of dst in previewsize */

static gint32  g_tol_col_err;
static gint32  g_max_col_err;
static gint    g_Sdebug = FALSE;
static gint    g_show_progress = FALSE;


static void
colorize_class_init (ColorizeClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = colorize_query_procedures;
  plug_in_class->create_procedure = colorize_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
colorize_init (Colorize *colorize)
{
}

static GList *
colorize_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
colorize_create_procedure (LigmaPlugIn  *plug_in,
                          const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      static gchar *help_string =
        "This plug-in colorizes the contents of the specified (gray) layer"
        " with the help of a  sample (color) layer."
        " It analyzes all colors in the sample layer."
        " The sample colors are sorted by brightness (== intentisty) and amount"
        " and stored in a sample colortable (where brightness is the index)"
        " The pixels of the destination layer are remapped with the help of the"
        " sample colortable. If use_subcolors is TRUE, the remapping process uses"
        " all sample colors of the corresponding brightness-intensity and"
        " distributes the subcolors according to their amount in the sample"
        " (If the sample has 5 green, 3 yellow, and 1 red pixel of the "
        " intensity value 105, the destination pixels at intensity value 105"
        " are randomly painted in green, yellow and red in a relation of 5:3:1"
        " If use_subcolors is FALSE only one sample color per intensity is used."
        " (green will be used in this example)"
        " The brightness intensity value is transformed at the remapping process"
        " according to the levels: out_lo, out_hi, in_lo, in_high and gamma"
        " The in_low / in_high levels specify an initial mapping of the intensity."
        " The gamma value determines how intensities are interpolated between"
        " the in_lo and in_high levels. A gamma value of 1.0 results in linear"
        " interpolation. Higher gamma values results in more high-level intensities"
        " Lower gamma values results in more low-level intensities"
        " The out_low/out_high levels constrain the resulting intensity index"
        " The intensity index is used to pick the corresponding color"
        " in the sample colortable. If hold_inten is FALSE the picked color"
        " is used 1:1 as resulting remap_color."
        " If hold_inten is TRUE The brightness of the picked color is adjusted"
        " back to the original intensity value (only hue and saturation are"
        " taken from the picked sample color)"
        " (or to the input level, if orig_inten is set FALSE)"
        " Works on both Grayscale and RGB image with/without alpha channel."
        " (the image with the dst_drawable is converted to RGB if necessary)"
        " The sample_drawable should be of type RGB or RGBA";

      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            colorize_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("_Sample Colorize..."));
      ligma_procedure_add_menu_path (procedure, "<Image>/Colors/Map");

      ligma_procedure_set_documentation (procedure,
                                        _("Colorize image using a sample "
                                          "image as a guide"),
                                        help_string,
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Wolfgang Hofer",
                                      "hof@hotbot.com",
                                      "02/2000");

      LIGMA_PROC_ARG_DRAWABLE (procedure, "sample-drawable",
                              "Sample drawable",
                              "Sample drawable (should be of Type RGB or RGBA)",
                              TRUE,
                              G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "hold-inten",
                             "Hold inten",
                             "Hold brightness intensity levels",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "orig-inten",
                             "Orig inten",
                             "TRUE: hold brightness of original intensity "
                             "levels, "
                             "FALSE: Hold Intensity of input levels",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "rnd-subcolors",
                             "Rnd subcolors",
                             "TRUE: Use all subcolors of same intensity, "
                             "FALSE: Use only one color per intensity",
                             FALSE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "guess-missing",
                             "Guess missing",
                             "TRUE: guess samplecolors for the missing "
                             "intensity values, "
                             "FALSE: use only colors found in the sample",
                             TRUE,
                             G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "in-low",
                         "In low",
                         "Intensity of lowest input",
                         0, 254, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "in-high",
                         "In high",
                         "Intensity of highest input",
                         1, 255, 255,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_DOUBLE (procedure, "gamma",
                            "Gamma",
                            "Gamma adjustment factor, 1.0 is linear",
                            0.1, 1.0, 1.0,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "out-low",
                         "out low",
                         "Lowest sample color intensity",
                         0, 254, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "ouz-high",
                         "Out high",
                         "Highest sample color intensity",
                         1, 255, 255,
                         G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
colorize_run (LigmaProcedure        *procedure,
              LigmaRunMode           run_mode,
              LigmaImage            *image,
              gint                  n_drawables,
              LigmaDrawable        **drawables,
              const LigmaValueArray *args,
              gpointer              run_data)
{
  const gchar  *env;
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

  env = g_getenv ("SAMPLE_COLORIZE_DEBUG");
  if (env != NULL && (*env != 'n') && (*env != 'N'))
    g_Sdebug = TRUE;

  if (g_Sdebug)
    g_printf ("sample colorize run\n");
  g_show_progress = FALSE;

  g_values.lvl_out_min  = 0;
  g_values.lvl_out_max  = 255;
  g_values.lvl_in_gamma = 1.0;
  g_values.lvl_in_min   = 0;
  g_values.lvl_in_max   = 255;

  /* Possibly retrieve data from a previous run */
  ligma_get_data (PLUG_IN_PROC, &g_values);
  if (g_values.sample_id == SMP_GRADIENT ||
      g_values.sample_id == SMP_INV_GRADIENT)
    g_values.sample_id = -1;

  /* fix value */
  g_values.tol_col_err = 5.5;

  /*  Get the specified dst_drawable  */
  g_values.dst = drawable;

  clear_tables ();

  /*  Make sure that the drawable is gray or RGB color  */
  if (ligma_drawable_is_rgb  (drawable) ||
      ligma_drawable_is_gray (drawable))
    {
      switch (run_mode)
        {
        case LIGMA_RUN_INTERACTIVE:
          smp_dialog ();
          free_colors ();
          ligma_set_data (PLUG_IN_PROC, &g_values, sizeof (t_values));
          ligma_displays_flush ();
          break;

        case LIGMA_RUN_NONINTERACTIVE:
          g_values.sample_id     = LIGMA_VALUES_GET_DRAWABLE_ID (args, 0);
          g_values.hold_inten    = LIGMA_VALUES_GET_BOOLEAN     (args, 1);
          g_values.orig_inten    = LIGMA_VALUES_GET_BOOLEAN     (args, 2);
          g_values.rnd_subcolors = LIGMA_VALUES_GET_BOOLEAN     (args, 3);
          g_values.guess_missing = LIGMA_VALUES_GET_BOOLEAN     (args, 4);
          g_values.lvl_in_min    = LIGMA_VALUES_GET_INT         (args, 5);
          g_values.lvl_in_max    = LIGMA_VALUES_GET_INT         (args, 6);
          g_values.lvl_in_gamma  = LIGMA_VALUES_GET_DOUBLE      (args, 7);
          g_values.lvl_out_min   = LIGMA_VALUES_GET_INT         (args, 8);
          g_values.lvl_out_max   = LIGMA_VALUES_GET_INT         (args, 9);

          if (main_colorize (MC_GET_SAMPLE_COLORS) >= 0)
            {
              main_colorize (MC_DST_REMAP);
            }
          else
            {
              return ligma_procedure_new_return_values (procedure,
                                                       LIGMA_PDB_EXECUTION_ERROR,
                                                       NULL);
            }
          break;

        case LIGMA_RUN_WITH_LAST_VALS:
          break;
        }
    }
  else
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

/* ============================================================================
 * callback and constraint procedures for the dialog
 * ============================================================================
 */

static void
smp_response_callback (GtkWidget *widget,
                       gint       response_id)
{
  switch (response_id)
    {
    case RESPONSE_RESET:
      g_values.lvl_in_min    = 0;
      g_values.lvl_in_max    = 255;
      g_values.lvl_in_gamma  = 1.0;
      g_values.lvl_out_min   = 0;
      g_values.lvl_out_max   = 255;

      levels_update (ALL);
      break;

    case RESPONSE_GET_COLORS:
      smp_get_colors (widget);
      break;

    case GTK_RESPONSE_APPLY:
      g_show_progress = TRUE;
      if (main_colorize (MC_DST_REMAP) == 0)
        {
          ligma_displays_flush ();
          g_show_progress = FALSE;
          return;
        }
      gtk_dialog_set_response_sensitive (GTK_DIALOG (widget),
                                         GTK_RESPONSE_APPLY, FALSE);
      break;

    default:
      gtk_widget_destroy (widget);
      gtk_main_quit ();
      break;
    }
}

static void
smp_toggle_callback (GtkWidget *widget,
                     gpointer   data)
{
  gboolean *toggle_val = (gboolean *)data;

  *toggle_val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));

  if ((data == &g_di.sample_show_selection) ||
      (data == &g_di.sample_show_color))
    {
      update_preview (ligma_drawable_get_by_id (g_values.sample_id));
      return;
    }

  if ((data == &g_di.dst_show_selection) ||
      (data == &g_di.dst_show_color))
    {
       update_preview (g_values.dst);
       return;
    }

  if ((data == &g_values.hold_inten) ||
      (data == &g_values.orig_inten) ||
      (data == &g_values.rnd_subcolors))
    {
      if (g_di.orig_inten_button)
        gtk_widget_set_sensitive (g_di.orig_inten_button,g_values.hold_inten);
      refresh_dst_preview (g_di.dst_preview,  &g_dst_preview_buffer[0]);
    }

  if (data == &g_values.guess_missing)
    {
      if (g_values.guess_missing)
        guess_missing_colors ();
      else
        fill_missing_colors ();
      smp_get_colors (NULL);
    }
}

static void
smp_sample_combo_callback (GtkWidget *widget)
{
  gint value;

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget), &value);

  g_values.sample_id = value;

  if (value == SMP_GRADIENT || value == SMP_INV_GRADIENT)
    {
      get_gradient (value);
      smp_get_colors (NULL);

      if (g_di.sample_preview)
        clear_preview (g_di.sample_preview);

      gtk_dialog_set_response_sensitive (GTK_DIALOG (g_di.dialog),
                                         GTK_RESPONSE_APPLY, TRUE);
      gtk_dialog_set_response_sensitive (GTK_DIALOG (g_di.dialog),
                                         RESPONSE_GET_COLORS, FALSE);
    }
  else
    {
      update_preview (ligma_drawable_get_by_id (g_values.sample_id));

      gtk_dialog_set_response_sensitive (GTK_DIALOG (g_di.dialog),
                                         RESPONSE_GET_COLORS, TRUE);
    }
}

static void
smp_dest_combo_callback (GtkWidget *widget)
{
  gint id;

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget), &id);

  g_values.dst = ligma_drawable_get_by_id (id);

  update_preview (g_values.dst);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (g_di.dialog),
                                     RESPONSE_GET_COLORS, TRUE);
}

static gint
smp_constrain (LigmaImage *image,
               LigmaItem  *item,
               gpointer   data)
{
  /* don't accept layers from indexed images */
  if (ligma_drawable_is_indexed (LIGMA_DRAWABLE (item)))
    return FALSE;

  return TRUE;
}


static void
smp_adj_lvl_in_max_upd_callback (GtkAdjustment *adjustment)
{
  gint32 value;
  gint   upd_flags;

  value = CLAMP ((gtk_adjustment_get_value (adjustment)), 1, 255);

  if (value != g_values.lvl_in_max)
    {
      g_values.lvl_in_max = value;
      upd_flags = INPUT_SLIDERS | INPUT_LEVELS | DRAW | REFRESH_DST;
      if (g_values.lvl_in_max < g_values.lvl_in_min)
        {
          g_values.lvl_in_min = g_values.lvl_in_max;
          upd_flags |= LOW_INPUT;
        }
      levels_update (upd_flags);
    }
}

static void
smp_adj_lvl_in_min_upd_callback (GtkAdjustment *adjustment)
{
  double value;
  gint   upd_flags;

  value = CLAMP ((gtk_adjustment_get_value (adjustment)), 0, 254);

  if (value != g_values.lvl_in_min)
    {
      g_values.lvl_in_min = value;
      upd_flags = INPUT_SLIDERS | INPUT_LEVELS | DRAW | REFRESH_DST;
      if (g_values.lvl_in_min > g_values.lvl_in_max)
        {
          g_values.lvl_in_max = g_values.lvl_in_min;
          upd_flags |= HIGH_INPUT;
        }
      levels_update (upd_flags);
    }
}

static void
smp_text_gamma_upd_callback (GtkAdjustment *adjustment)
{
  double value;

  value = CLAMP ((gtk_adjustment_get_value (adjustment)), 0.1, 10.0);

  if (value != g_values.lvl_in_gamma)
    {
      g_values.lvl_in_gamma = value;
      levels_update (INPUT_SLIDERS | INPUT_LEVELS | DRAW | REFRESH_DST);
    }
}

static void
smp_adj_lvl_out_max_upd_callback (GtkAdjustment *adjustment)
{
  gint32 value;
  gint   upd_flags;

  value = CLAMP ((gtk_adjustment_get_value (adjustment)), 1, 255);

  if (value != g_values.lvl_out_max)
    {
      g_values.lvl_out_max = value;
      upd_flags = OUTPUT_SLIDERS | OUTPUT_LEVELS | DRAW | REFRESH_DST;
      if (g_values.lvl_out_max < g_values.lvl_out_min)
        {
          g_values.lvl_out_min = g_values.lvl_out_max;
          upd_flags |= LOW_OUTPUT;
        }
      levels_update (upd_flags);
    }
}

static void
smp_adj_lvl_out_min_upd_callback (GtkAdjustment *adjustment)
{
  double value;
  gint   upd_flags;

  value = CLAMP ((gtk_adjustment_get_value (adjustment)), 0, 254);

  if (value != g_values.lvl_out_min)
    {
      g_values.lvl_out_min = value;
      upd_flags = OUTPUT_SLIDERS | OUTPUT_LEVELS | DRAW  | REFRESH_DST;
      if (g_values.lvl_out_min > g_values.lvl_out_max)
        {
          g_values.lvl_out_max = g_values.lvl_out_min;
          upd_flags |= HIGH_OUTPUT;
        }
      levels_update (upd_flags);
    }
}

/* ============================================================================
 * DIALOG helper procedures
 *    (workers for the updates on the preview widgets)
 * ============================================================================
 */

static void
refresh_dst_preview (GtkWidget *preview,
                     guchar    *src_buffer)
{
  guchar  allrowsbuf[3 * PREVIEW_SIZE_X * PREVIEW_SIZE_Y];
  guchar *ptr;
  guchar *src_ptr;
  guchar  lum;
  guchar  maskbyte;
  gint    x, y;
  gint    preview_bpp;
  gint    src_bpp;

  preview_bpp = PREVIEW_BPP;
  src_bpp = PREVIEW_BPP +1;   /* 3 colors + 1 maskbyte */
  src_ptr = src_buffer;

  ptr = allrowsbuf;
  for (y = 0; y < PREVIEW_SIZE_Y; y++)
    {
      for (x = 0; x < PREVIEW_SIZE_X; x++)
        {
          if ((maskbyte = src_ptr[3]) == 0)
            {
              ptr[0] = src_ptr[0];
              ptr[1] = src_ptr[1];
              ptr[2] = src_ptr[2];
            }
          else
            {
              if (g_di.dst_show_color)
                {
                  remap_pixel (ptr, src_ptr, 3);
                }
              else
                {
                  /* lum = g_out_trans_tab[g_lvl_trans_tab[LUMINOSITY_1(src_ptr)]]; */
                  /* get brightness from (uncolorized) original */

                  lum = g_lvl_trans_tab[LUMINOSITY_1 (src_ptr)];
                  /* get brightness from (uncolorized) original */

                  ptr[0] = lum;
                  ptr[1] = lum;
                  ptr[2] = lum;
                }

              if (maskbyte < 255)
                {
                  ptr[0] = MIX_CHANNEL (ptr[0], src_ptr[0], maskbyte);
                  ptr[1] = MIX_CHANNEL (ptr[1], src_ptr[1], maskbyte);
                  ptr[2] = MIX_CHANNEL (ptr[2], src_ptr[2], maskbyte);
                }
            }
          ptr += preview_bpp;
          src_ptr += src_bpp;
        }
    }

  ligma_preview_area_draw (LIGMA_PREVIEW_AREA (preview),
                          0, 0, PREVIEW_SIZE_X, PREVIEW_SIZE_Y,
                          LIGMA_RGB_IMAGE,
                          allrowsbuf,
                          PREVIEW_SIZE_X * 3);
}

static void
clear_preview (GtkWidget *preview)
{
  ligma_preview_area_fill (LIGMA_PREVIEW_AREA (preview),
                          0, 0, PREVIEW_SIZE_X, PREVIEW_SIZE_Y,
                          170, 170, 170);
}

static void
update_pv (GtkWidget *preview,
           gboolean   show_selection,
           t_GDRW    *gdrw,
           guchar    *dst_buffer,
           gboolean   is_color)
{
  guchar  allrowsbuf[4 * PREVIEW_SIZE_X * PREVIEW_SIZE_Y];
  guchar  pixel[4];
  guchar *ptr;
  gint    x, y;
  gint    x2, y2;
  gint    ofx, ofy;
  gint    sel_width, sel_height;
  double  scale_x, scale_y;
  guchar *buf_ptr;
  guchar  dummy[4];
  guchar  maskbytes[4];
  gint    dstep;
  guchar  alpha;

  if (! preview)
    return;

  /* init gray pixel (if we are called without a sourceimage (gdwr == NULL) */
  pixel[0] = pixel[1] =pixel[2] = pixel[3] = 127;

  /* calculate scale factors and offsets */
  if (show_selection)
    {
      sel_width  = gdrw->x2 - gdrw->x1;
      sel_height = gdrw->y2 - gdrw->y1;

      if (sel_height > sel_width)
        {
          scale_y = (gfloat) sel_height / PREVIEW_SIZE_Y;
          scale_x = scale_y;
          ofx = (gdrw->x1 +
                   ((sel_width - (PREVIEW_SIZE_X * scale_x)) / 2));
          ofy = gdrw->y1;
        }
      else
        {
          scale_x = (gfloat) sel_width / PREVIEW_SIZE_X;
          scale_y = scale_x;
          ofx = gdrw->x1;
          ofy = (gdrw->y1 +
                   ((sel_height - (PREVIEW_SIZE_Y * scale_y)) / 2));
        }
    }
  else
    {
      if (gdrw->height > gdrw->width)
        {
          scale_y = (gfloat) gdrw->height / PREVIEW_SIZE_Y;
          scale_x = scale_y;
          ofx = (gdrw->width - (PREVIEW_SIZE_X * scale_x)) / 2;
          ofy = 0;
        }
      else
        {
          scale_x = (gfloat) gdrw->width / PREVIEW_SIZE_X;
          scale_y = scale_x;
          ofx = 0;
          ofy = (gdrw->height - (PREVIEW_SIZE_Y * scale_y)) / 2;
        }
    }

  /* check if output goes to previw widget or to dst_buffer */
  if (dst_buffer)
    {
      buf_ptr = dst_buffer;
      dstep = PREVIEW_BPP +1;
    }
  else
    {
      buf_ptr = &dummy[0];
      dstep = 0;
    }


  /* render preview */
  ptr = allrowsbuf;
  for (y = 0; y < PREVIEW_SIZE_Y; y++)
    {
      for (x = 0; x < PREVIEW_SIZE_X; x++)
        {
          if (gdrw->drawable)
            {
              x2 = ofx + (x * scale_x);
              y2 = ofy + (y * scale_y);
              get_pixel (gdrw, x2, y2, &pixel[0]);
              if (gdrw->sel_gdrw)
                {
                  get_pixel (gdrw->sel_gdrw,
                             x2 + gdrw->seldeltax,
                             y2 + gdrw->seldeltay,
                             &maskbytes[0]);
                }
              else
                {
                  maskbytes[0] = 255;
                }
            }

          alpha = pixel[gdrw->index_alpha];
          if (is_color && (gdrw->bpp > 2))
            {
              buf_ptr[0] = ptr[0] = pixel[0];
              buf_ptr[1] = ptr[1] = pixel[1];
              buf_ptr[2] = ptr[2] = pixel[2];
            }
          else
            {
              if (gdrw->bpp > 2)
                *ptr = LUMINOSITY_1 (pixel);
              else
                *ptr = pixel[0];

              *buf_ptr   = *ptr;
              buf_ptr[1] = ptr[1] = *ptr;
              buf_ptr[2] = ptr[2] = *ptr;
            }
          if (gdrw->index_alpha == 0)             /* has no alpha channel */
            buf_ptr[3] = ptr[3] = 255;
          else
            buf_ptr[3] = ptr[3] = MIN (maskbytes[0], alpha);
          buf_ptr += dstep;   /* advance (or stay at dummy byte) */
          ptr += 4;
        }

    }

  if (dst_buffer == NULL)
    {
      ligma_preview_area_draw (LIGMA_PREVIEW_AREA (preview),
                              0, 0, PREVIEW_SIZE_X, PREVIEW_SIZE_Y,
                              LIGMA_RGBA_IMAGE,
                              allrowsbuf,
                              PREVIEW_SIZE_X * 4);
      gtk_widget_queue_draw (preview);
    }
}

static void
update_preview (LigmaDrawable *drawable)
{
  t_GDRW   gdrw;
  gboolean is_drawable = FALSE;

  if (! drawable || !g_di.enable_preview_update)
    return;

  if (ligma_item_get_id (LIGMA_ITEM (drawable)) == g_values.sample_id)
    {
      is_drawable = TRUE;

      init_gdrw (&gdrw, drawable, FALSE);
      update_pv (g_di.sample_preview, g_di.sample_show_selection, &gdrw,
                 NULL, g_di.sample_show_color);
    }
  else if (ligma_item_get_id (LIGMA_ITEM (drawable)) ==
           ligma_item_get_id (LIGMA_ITEM (g_values.dst)))
    {
      is_drawable = TRUE;

      init_gdrw (&gdrw, drawable, FALSE);
      update_pv (g_di.dst_preview, g_di.dst_show_selection, &gdrw,
                 &g_dst_preview_buffer[0], g_di.dst_show_color);
      refresh_dst_preview (g_di.dst_preview,  &g_dst_preview_buffer[0]);
    }

  if (is_drawable)
    end_gdrw (&gdrw);
}

static void
levels_draw_slider (cairo_t *cr,
                    GdkRGBA *border_color,
                    GdkRGBA *fill_color,
                    gint     xpos)
{
  cairo_move_to (cr, xpos, 0);
  cairo_line_to (cr, xpos - (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);
  cairo_line_to (cr, xpos + (CONTROL_HEIGHT - 1) / 2, CONTROL_HEIGHT - 1);
  cairo_line_to (cr, xpos, 0);

  gdk_cairo_set_source_rgba (cr, fill_color);
  cairo_fill_preserve (cr);

  gdk_cairo_set_source_rgba (cr, border_color);
  cairo_stroke (cr);
}

static void
smp_get_colors (GtkWidget *dialog)
{
  gint   i;
  guchar buffer[3 * DA_WIDTH * GRADIENT_HEIGHT];

  update_preview (ligma_drawable_get_by_id (g_values.sample_id));

  if (dialog && main_colorize (MC_GET_SAMPLE_COLORS) >= 0)  /* do not colorize, just analyze sample colors */
    gtk_dialog_set_response_sensitive (GTK_DIALOG (g_di.dialog),
                                       GTK_RESPONSE_APPLY, TRUE);
  for (i = 0; i < GRADIENT_HEIGHT; i++)
    memcpy (buffer + i * 3 * DA_WIDTH, g_sample_color_tab, 3 * DA_WIDTH);

  update_preview (g_values.dst);

  ligma_preview_area_draw (LIGMA_PREVIEW_AREA (g_di.sample_colortab_preview),
                          0, 0, DA_WIDTH, GRADIENT_HEIGHT,
                          LIGMA_RGB_IMAGE,
                          buffer,
                          DA_WIDTH * 3);
}


static void
levels_update (gint update)
{
  gint i;

  if (g_Sdebug)
    g_printf ("levels_update: update request %x\n", update);

  /*  Recalculate the transfer array  */
  calculate_level_transfers ();
  if (update & REFRESH_DST)
    refresh_dst_preview (g_di.dst_preview,  &g_dst_preview_buffer[0]);

  /* update the spinbutton entry widgets */
  if (update & LOW_INPUT)
    gtk_adjustment_set_value (g_di.adj_lvl_in_min,
                              g_values.lvl_in_min);
  if (update & GAMMA)
    gtk_adjustment_set_value (g_di.adj_lvl_in_gamma,
                              g_values.lvl_in_gamma);
  if (update & HIGH_INPUT)
    gtk_adjustment_set_value (g_di.adj_lvl_in_max,
                              g_values.lvl_in_max);
  if (update & LOW_OUTPUT)
    gtk_adjustment_set_value (g_di.adj_lvl_out_min,
                              g_values.lvl_out_min);
  if (update & HIGH_OUTPUT)
    gtk_adjustment_set_value (g_di.adj_lvl_out_max,
                              g_values.lvl_out_max);
  if (update & INPUT_LEVELS)
    {
      guchar buffer[DA_WIDTH * GRADIENT_HEIGHT];
      for (i = 0; i < GRADIENT_HEIGHT; i++)
        memcpy (buffer + DA_WIDTH * i, g_lvl_trans_tab, DA_WIDTH);
      ligma_preview_area_draw (LIGMA_PREVIEW_AREA (g_di.in_lvl_gray_preview),
                              0, 0, DA_WIDTH, GRADIENT_HEIGHT,
                              LIGMA_GRAY_IMAGE,
                              buffer,
                              DA_WIDTH);
    }

  if (update & INPUT_SLIDERS)
    gtk_widget_queue_draw (g_di.in_lvl_drawarea);

  if (update & OUTPUT_SLIDERS)
    gtk_widget_queue_draw (g_di.sample_drawarea);
}

static gboolean
level_in_events (GtkWidget *widget,
                 GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gdouble         width, mid, tmp;
  gint            x, distance;
  gint            i;
  gint            update = FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (g_Sdebug)
        g_printf ("EVENT: GDK_BUTTON_PRESS\n");
      gtk_grab_add (widget);
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 0; i < 3; i++)
        {
          if (fabs (bevent->x - g_di.slider_pos[i]) < distance)
            {
              g_di.active_slider = i;
              distance = fabs (bevent->x - g_di.slider_pos[i]);
            }
        }
      x = bevent->x;
      update = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      if (g_Sdebug)
        g_printf ("EVENT: GDK_BUTTON_RELEASE\n");
      gtk_grab_remove (widget);
      switch (g_di.active_slider)
        {
        case 0:  /*  low input  */
          levels_update (LOW_INPUT | GAMMA | DRAW);
          break;

        case 1:  /*  gamma  */
          levels_update (GAMMA);
          break;

        case 2:  /*  high input  */
          levels_update (HIGH_INPUT | GAMMA | DRAW);
          break;
        }

      refresh_dst_preview (g_di.dst_preview,  &g_dst_preview_buffer[0]);
      break;

    case GDK_MOTION_NOTIFY:
      if (g_Sdebug)
        g_printf ("EVENT: GDK_MOTION_NOTIFY\n");
      mevent = (GdkEventMotion *) event;
      x = mevent->x;
      gdk_event_request_motions (mevent);
      update = TRUE;
      break;

    default:
      if (g_Sdebug)
        g_printf ("EVENT: default\n");
      break;
    }

  if (update)
    {
      if (g_Sdebug)
        g_printf ("EVENT: ** update **\n");
      switch (g_di.active_slider)
        {
        case 0:  /*  low input  */
          g_values.lvl_in_min = ((double) x / (double) DA_WIDTH) * 255.0;
          g_values.lvl_in_min = CLAMP (g_values.lvl_in_min,
                                       0, g_values.lvl_in_max);
          break;

        case 1:  /*  gamma  */
          width = (double) (g_di.slider_pos[2] - g_di.slider_pos[0]) / 2.0;
          mid = g_di.slider_pos[0] + width;

          x = CLAMP (x, g_di.slider_pos[0], g_di.slider_pos[2]);
          tmp = (double) (x - mid) / width;
          g_values.lvl_in_gamma = 1.0 / pow (10, tmp);

          /*  round the gamma value to the nearest 1/100th  */
          g_values.lvl_in_gamma =
            floor (g_values.lvl_in_gamma * 100 + 0.5) / 100.0;
          break;

        case 2:  /*  high input  */
          g_values.lvl_in_max = ((double) x / (double) DA_WIDTH) * 255.0;
          g_values.lvl_in_max = CLAMP (g_values.lvl_in_max,
                                       g_values.lvl_in_min, 255);
          break;
        }

      levels_update (INPUT_SLIDERS | INPUT_LEVELS | DRAW);
    }

  return FALSE;
}

static gboolean
level_in_draw (GtkWidget *widget,
               cairo_t   *cr)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  gdouble          width, mid, tmp;
  GdkRGBA          black = { 0.0, 0.0, 0.0, 1.0 };
  GdkRGBA          white = { 1.0, 1.0, 1.0, 1.0 };
  GdkRGBA          gray  = { 0.5, 0.5, 0.5, 1.0 };

  gtk_render_background (style, cr, 0, 0,
                         gtk_widget_get_allocated_width  (widget),
                         gtk_widget_get_allocated_height (widget));

  cairo_translate (cr, 0.5, 0.5);
  cairo_set_line_width (cr, 1.0);

  g_di.slider_pos[0] = DA_WIDTH * ((double) g_values.lvl_in_min / 255.0);
  g_di.slider_pos[2] = DA_WIDTH * ((double) g_values.lvl_in_max / 255.0);

  width = (double) (g_di.slider_pos[2] - g_di.slider_pos[0]) / 2.0;
  mid = g_di.slider_pos[0] + width;
  tmp = log10 (1.0 / g_values.lvl_in_gamma);
  g_di.slider_pos[1] = (int) (mid + width * tmp + 0.5);

  levels_draw_slider (cr,
                      &black, &gray,
                      g_di.slider_pos[1]);
  levels_draw_slider (cr,
                      &black, &black,
                      g_di.slider_pos[0]);
  levels_draw_slider (cr,
                      &black, &white,
                      g_di.slider_pos[2]);

  return FALSE;
}

static gboolean
level_out_events (GtkWidget *widget,
                  GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventMotion *mevent;
  gint            x, distance;
  gint            i;
  gint            update = FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      if (g_Sdebug)
        g_printf ("OUT_EVENT: GDK_BUTTON_PRESS\n");
      bevent = (GdkEventButton *) event;

      distance = G_MAXINT;
      for (i = 3; i < 5; i++)
        {
          if (fabs (bevent->x - g_di.slider_pos[i]) < distance)
            {
              g_di.active_slider = i;
              distance = fabs (bevent->x - g_di.slider_pos[i]);
            }
        }

      x = bevent->x;
      update = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      if (g_Sdebug)
        g_printf ("OUT_EVENT: GDK_BUTTON_RELEASE\n");
      switch (g_di.active_slider)
        {
        case 3:  /*  low output  */
          levels_update (LOW_OUTPUT | DRAW);
          break;

        case 4:  /*  high output  */
          levels_update (HIGH_OUTPUT | DRAW);
          break;
        }

      refresh_dst_preview (g_di.dst_preview,  &g_dst_preview_buffer[0]);
      break;

    case GDK_MOTION_NOTIFY:
      if (g_Sdebug)
        g_printf ("OUT_EVENT: GDK_MOTION_NOTIFY\n");
      mevent = (GdkEventMotion *) event;
      x = mevent->x;
      gdk_event_request_motions (mevent);
      update = TRUE;
      break;

    default:
      if (g_Sdebug)
        g_printf ("OUT_EVENT: default\n");
      break;
    }

  if (update)
    {
      if (g_Sdebug)
        g_printf ("OUT_EVENT: ** update **\n");
      switch (g_di.active_slider)
        {
        case 3:  /*  low output  */
          g_values.lvl_out_min = ((double) x / (double) DA_WIDTH) * 255.0;
          g_values.lvl_out_min = CLAMP (g_values.lvl_out_min,
                                        0, g_values.lvl_out_max);
          break;

        case 4:  /*  high output  */
          g_values.lvl_out_max = ((double) x / (double) DA_WIDTH) * 255.0;
          g_values.lvl_out_max = CLAMP (g_values.lvl_out_max,
                                        g_values.lvl_out_min, 255);
          break;
        }

      levels_update (OUTPUT_SLIDERS | OUTPUT_LEVELS | DRAW);
    }

  return FALSE;
}

static gboolean
level_out_draw (GtkWidget *widget,
                cairo_t   *cr)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GdkRGBA          black = { 0.0, 0.0, 0.0, 1.0 };

  gtk_render_background (style, cr, 0, 0,
                         gtk_widget_get_allocated_width  (widget),
                         gtk_widget_get_allocated_height (widget));

  cairo_translate (cr, 0.5, 0.5);
  cairo_set_line_width (cr, 1.0);

  g_di.slider_pos[3] = DA_WIDTH * ((double) g_values.lvl_out_min / 255.0);
  g_di.slider_pos[4] = DA_WIDTH * ((double) g_values.lvl_out_max / 255.0);

  levels_draw_slider (cr,
                      &black, &black,
                      g_di.slider_pos[3]);
  levels_draw_slider (cr,
                      &black, &black,
                      g_di.slider_pos[4]);

  return FALSE;
}



/* ============================================================================
 * smp_dialog
 *        The Interactive Dialog
 * ============================================================================
 */
static void
smp_dialog (void)
{
  GtkWidget     *dialog;
  GtkWidget     *hbox;
  GtkWidget     *vbox2;
  GtkWidget     *frame;
  GtkWidget     *grid;
  GtkWidget     *check_button;
  GtkWidget     *label;
  GtkWidget     *combo;
  GtkWidget     *spinbutton;
  GtkAdjustment *data;
  gint           ty;

  /* set flags for check buttons from mode value bits */
  if (g_Sdebug)
    g_print ("smp_dialog START\n");

  /* init some dialog variables */
  g_di.enable_preview_update = FALSE;
  g_di.sample_show_selection = TRUE;
  g_di.dst_show_selection    = TRUE;
  g_di.dst_show_color        = TRUE;
  g_di.sample_show_color     = TRUE;
  g_di.orig_inten_button     = NULL;

  /* Init GTK  */
  ligma_ui_init (PLUG_IN_BINARY);

  /* Main Dialog */
  g_di.dialog = dialog =
    ligma_dialog_new (_("Sample Colorize"), PLUG_IN_ROLE,
                     NULL, 0,
                     ligma_standard_help_func, PLUG_IN_PROC,

                     _("_Reset"),             RESPONSE_RESET,
                     _("Get _Sample Colors"), RESPONSE_GET_COLORS,
                     _("_Close"),             GTK_RESPONSE_CLOSE,
                     _("_Apply"),             GTK_RESPONSE_APPLY,

                     NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           RESPONSE_GET_COLORS,
                                           RESPONSE_RESET,
                                           GTK_RESPONSE_APPLY,
                                           GTK_RESPONSE_CLOSE,
                                           -1);

  ligma_window_set_transient (GTK_WINDOW (dialog));

  g_signal_connect (dialog, "response",
                    G_CALLBACK (smp_response_callback),
                    NULL);

  /* grid for values */
  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      grid, TRUE, TRUE, 0);

  ty = 0;
  /* layer combo_box (Dst) */
  label = gtk_label_new (_("Destination:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, ty, 1, 1);
  gtk_widget_show (label);

  combo = ligma_layer_combo_box_new (smp_constrain, NULL, NULL);
  ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                              ligma_item_get_id (LIGMA_ITEM (g_values.dst)),
                              G_CALLBACK (smp_dest_combo_callback),
                              NULL, NULL);

  gtk_grid_attach (GTK_GRID (grid), combo, 1, ty, 1, 1);
  gtk_widget_show (combo);

  /* layer combo_box (Sample) */
  label = gtk_label_new (_("Sample:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_grid_attach (GTK_GRID (grid), label, 3, ty, 1, 1);
  gtk_widget_show (label);

  combo = ligma_layer_combo_box_new (smp_constrain, NULL, NULL);

  ligma_int_combo_box_prepend (LIGMA_INT_COMBO_BOX (combo),
                              LIGMA_INT_STORE_VALUE,     SMP_INV_GRADIENT,
                              LIGMA_INT_STORE_LABEL,     _("From reverse gradient"),
                              LIGMA_INT_STORE_ICON_NAME, LIGMA_ICON_GRADIENT,
                              -1);
  ligma_int_combo_box_prepend (LIGMA_INT_COMBO_BOX (combo),
                              LIGMA_INT_STORE_VALUE,     SMP_GRADIENT,
                              LIGMA_INT_STORE_LABEL,     _("From gradient"),
                              LIGMA_INT_STORE_ICON_NAME, LIGMA_ICON_GRADIENT,
                              -1);

  ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo), g_values.sample_id,
                              G_CALLBACK (smp_sample_combo_callback),
                              NULL, NULL);

  gtk_grid_attach (GTK_GRID (grid), combo, 4, ty, 1, 1);
  gtk_widget_show (combo);

  ty++;


  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_grid_attach (GTK_GRID (grid), hbox, 0, ty, 2, 1);
  gtk_widget_show (hbox);

  /* check button */
  check_button = gtk_check_button_new_with_mnemonic (_("Sho_w selection"));
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);

  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (smp_toggle_callback),
                    &g_di.dst_show_selection);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                g_di.dst_show_selection);

  /* check button */
  check_button = gtk_check_button_new_with_mnemonic (_("Show co_lor"));
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);

  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (smp_toggle_callback),
                    &g_di.dst_show_color);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                g_di.dst_show_color);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_grid_attach (GTK_GRID (grid), hbox, 3, ty, 2, 1);
  gtk_widget_show (hbox);

  /* check button */
  check_button = gtk_check_button_new_with_mnemonic (_("Show selec_tion"));
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);

  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (smp_toggle_callback),
                    &g_di.sample_show_selection);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                g_di.sample_show_selection);

  /* check button */
  check_button = gtk_check_button_new_with_mnemonic (_("Show c_olor"));
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);

  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (smp_toggle_callback),
                    &g_di.sample_show_color);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                g_di.sample_show_color);

  ty++;

  /* Preview (Dst) */

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_grid_attach (GTK_GRID (grid), frame, 0, ty, 2, 1);
  gtk_widget_show (frame);

  g_di.dst_preview = ligma_preview_area_new ();
  gtk_widget_set_halign (g_di.dst_preview, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (g_di.dst_preview, TRUE);
  gtk_widget_set_size_request (g_di.dst_preview,
                               PREVIEW_SIZE_X, PREVIEW_SIZE_Y);
  gtk_container_add (GTK_CONTAINER (frame), g_di.dst_preview);
  gtk_widget_show (g_di.dst_preview);

  /* Preview (Sample)*/

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_grid_attach (GTK_GRID (grid), frame, 3, ty, 2, 1);
  gtk_widget_show (frame);

  g_di.sample_preview = ligma_preview_area_new ();
  gtk_widget_set_halign (g_di.sample_preview, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (g_di.sample_preview, TRUE);
  gtk_widget_set_size_request (g_di.sample_preview,
                               PREVIEW_SIZE_X, PREVIEW_SIZE_Y);
  gtk_container_add (GTK_CONTAINER (frame), g_di.sample_preview);
  gtk_widget_show (g_di.sample_preview);

  ty++;

  /*  The levels graylevel prevev  */
  frame = gtk_frame_new (NULL);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_grid_attach (GTK_GRID (grid), frame, 0, ty, 2, 1);

  g_di.in_lvl_gray_preview = ligma_preview_area_new ();
  gtk_widget_set_halign (g_di.in_lvl_gray_preview, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (g_di.in_lvl_gray_preview, TRUE);
  gtk_widget_set_size_request (g_di.in_lvl_gray_preview,
                               DA_WIDTH, GRADIENT_HEIGHT);
  gtk_widget_set_events (g_di.in_lvl_gray_preview, LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox2), g_di.in_lvl_gray_preview, FALSE, TRUE, 0);
  gtk_widget_show (g_di.in_lvl_gray_preview);

  g_signal_connect (g_di.in_lvl_gray_preview, "event",
                    G_CALLBACK (level_in_events),
                    NULL);

  /*  The levels drawing area  */
  g_di.in_lvl_drawarea = gtk_drawing_area_new ();
  gtk_widget_set_halign (g_di.in_lvl_drawarea, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (g_di.in_lvl_drawarea, TRUE);
  gtk_widget_set_size_request (g_di.in_lvl_drawarea,
                               DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (g_di.in_lvl_drawarea, LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox2), g_di.in_lvl_drawarea, FALSE, TRUE, 0);
  gtk_widget_show (g_di.in_lvl_drawarea);

  g_signal_connect (g_di.in_lvl_drawarea, "event",
                    G_CALLBACK (level_in_events),
                    NULL);
  g_signal_connect (g_di.in_lvl_drawarea, "draw",
                    G_CALLBACK (level_in_draw),
                    NULL);

  gtk_widget_show (vbox2);
  gtk_widget_show (frame);

  /*  The sample_colortable prevev  */
  frame = gtk_frame_new (NULL);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_grid_attach (GTK_GRID (grid), frame, 3, ty, 2, 1);

  g_di.sample_colortab_preview = ligma_preview_area_new ();
  gtk_widget_set_halign (g_di.sample_colortab_preview, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (g_di.sample_colortab_preview, TRUE);
  gtk_widget_set_size_request (g_di.sample_colortab_preview,
                               DA_WIDTH, GRADIENT_HEIGHT);
  gtk_box_pack_start (GTK_BOX (vbox2), g_di.sample_colortab_preview, FALSE, TRUE, 0);
  gtk_widget_show (g_di.sample_colortab_preview);

  /*  The levels drawing area  */
  g_di.sample_drawarea = gtk_drawing_area_new ();
  gtk_widget_set_halign (g_di.sample_drawarea, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (g_di.sample_drawarea, TRUE);
  gtk_widget_set_size_request (g_di.sample_drawarea,
                               DA_WIDTH, CONTROL_HEIGHT);
  gtk_widget_set_events (g_di.sample_drawarea, LEVELS_DA_MASK);
  gtk_box_pack_start (GTK_BOX (vbox2), g_di.sample_drawarea, FALSE, TRUE, 0);
  gtk_widget_show (g_di.sample_drawarea);

  g_signal_connect (g_di.sample_drawarea, "event",
                    G_CALLBACK (level_out_events),
                    NULL);
  g_signal_connect (g_di.sample_drawarea, "draw",
                    G_CALLBACK (level_out_draw),
                    NULL);

  gtk_widget_show (vbox2);
  gtk_widget_show (frame);


  ty++;

  /*  Horizontal box for INPUT levels text widget  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_grid_attach (GTK_GRID (grid), hbox, 0, ty, 2, 1);

  label = gtk_label_new (_("Input levels:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* min input spinbutton */
  data = gtk_adjustment_new ((gfloat)g_values.lvl_in_min, 0.0, 254.0, 1, 10, 0);
  g_di.adj_lvl_in_min = data;

  spinbutton = ligma_spin_button_new (g_di.adj_lvl_in_min, 0.5, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (g_di.adj_lvl_in_min, "value-changed",
                    G_CALLBACK (smp_adj_lvl_in_min_upd_callback),
                    &g_di);

  /* input gamma spinbutton */
  data = gtk_adjustment_new ((gfloat)g_values.lvl_in_gamma, 0.1, 10.0, 0.02, 0.2, 0);
  g_di.adj_lvl_in_gamma = data;

  spinbutton = ligma_spin_button_new (g_di.adj_lvl_in_gamma, 0.5, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (g_di.adj_lvl_in_gamma, "value-changed",
                    G_CALLBACK (smp_text_gamma_upd_callback),
                    &g_di);

  /* high input spinbutton */
  data = gtk_adjustment_new ((gfloat)g_values.lvl_in_max, 1.0, 255.0, 1, 10, 0);
  g_di.adj_lvl_in_max = data;

  spinbutton = ligma_spin_button_new (g_di.adj_lvl_in_max, 0.5, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (g_di.adj_lvl_in_max, "value-changed",
                    G_CALLBACK (smp_adj_lvl_in_max_upd_callback),
                    &g_di);

  gtk_widget_show (hbox);

  /*  Horizontal box for OUTPUT levels text widget  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_grid_attach (GTK_GRID (grid), hbox, 3, ty, 2, 1);

  label = gtk_label_new (_("Output levels:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /*  min output spinbutton */
  data = gtk_adjustment_new ((gfloat)g_values.lvl_out_min, 0.0, 254.0, 1, 10, 0);
  g_di.adj_lvl_out_min = data;

  spinbutton = ligma_spin_button_new (g_di.adj_lvl_out_min, 0.5, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (g_di.adj_lvl_out_min, "value-changed",
                    G_CALLBACK (smp_adj_lvl_out_min_upd_callback),
                    &g_di);

  /* high output spinbutton */
  data = gtk_adjustment_new ((gfloat)g_values.lvl_out_max, 0.0, 255.0, 1, 10, 0);
  g_di.adj_lvl_out_max = data;

  spinbutton = ligma_spin_button_new (g_di.adj_lvl_out_max, 0.5, 0);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  g_signal_connect (g_di.adj_lvl_out_max, "value-changed",
                    G_CALLBACK (smp_adj_lvl_out_max_upd_callback),
                    &g_di);

  gtk_widget_show (hbox);

  ty++;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_grid_attach (GTK_GRID (grid), hbox, 0, ty, 2, 1);
  gtk_widget_show (hbox);

  /* check button */
  check_button = gtk_check_button_new_with_mnemonic (_("Hold _intensity"));
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);

  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (smp_toggle_callback),
                    &g_values.hold_inten);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                g_values.hold_inten);

  /* check button */
  check_button = gtk_check_button_new_with_mnemonic (_("Original i_ntensity"));
  g_di.orig_inten_button = check_button;
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_set_sensitive (g_di.orig_inten_button, g_values.hold_inten);
  gtk_widget_show (check_button);

  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (smp_toggle_callback),
                    &g_values.orig_inten);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                g_values.orig_inten);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_grid_attach (GTK_GRID (grid), hbox, 3, ty, 2, 1);
  gtk_widget_show (hbox);

  /* check button */
  check_button = gtk_check_button_new_with_mnemonic (_("Us_e subcolors"));
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);

  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (smp_toggle_callback),
                    &g_values.rnd_subcolors);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                g_values.rnd_subcolors);

  /* check button */
  check_button = gtk_check_button_new_with_mnemonic (_("S_mooth samples"));
  gtk_box_pack_start (GTK_BOX (hbox), check_button, FALSE, FALSE, 0);
  gtk_widget_show (check_button);

  g_signal_connect (check_button, "toggled",
                    G_CALLBACK (smp_toggle_callback),
                    &g_values.guess_missing);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_button),
                                g_values.guess_missing);

  ty++;

  gtk_widget_show (grid);
  gtk_widget_show (frame);

  gtk_widget_show (dialog);

  /* set old_id's different (to force updates of the previews) */
  g_di.enable_preview_update = TRUE;
  smp_get_colors (NULL);
  update_preview (g_values.dst);
  levels_update (INPUT_SLIDERS | INPUT_LEVELS | DRAW);

  gtk_main ();
}

/* -----------------------------
 * DEBUG print procedures START
 * -----------------------------
 */

static void
print_ppm (const gchar *ppm_name)
{
  FILE              *fp;
  gint               idx;
  gint               cnt;
  gint               r;
  gint               g;
  gint               b;
  t_samp_color_elem *col_ptr;

  if (ppm_name == NULL)
    return;

  fp = g_fopen (ppm_name, "w");
  if (fp)
    {
      fprintf (fp, "P3\n# CREATOR: Ligma sample coloros\n256 256\n255\n");
      for (idx = 0; idx < 256; idx++)
        {
          col_ptr = g_lum_tab[idx].col_ptr;

          for (cnt = 0; cnt < 256; cnt++)
            {
              r = g = b = 0;
              if (col_ptr)
                {
                  if ((col_ptr->sum_color > 0) && (cnt != 20))
                    {
                      r = (gint) col_ptr->color[0];
                      g = (gint) col_ptr->color[1];
                      b = (gint) col_ptr->color[2];
                    }

                  if (cnt > 20)
                    col_ptr = col_ptr->next;
                }
              fprintf (fp, "%d %d %d\n", r, g, b);
            }
        }
      fclose (fp);
    }
}

static void
print_color_list (FILE              *fp,
                  t_samp_color_elem *col_ptr)
{
  if (fp == NULL)
    return;

  while (col_ptr)
    {
      fprintf (fp, "  RGBA: %03d %03d %03d %03d  sum: [%d]\n",
               (gint)col_ptr->color[0],
               (gint)col_ptr->color[1],
               (gint)col_ptr->color[2],
               (gint)col_ptr->color[3],
               (gint)col_ptr->sum_color);

      col_ptr = col_ptr->next;
    }
}

static void
print_table (FILE *fp)
{
  gint idx;

  if (fp == NULL)
    return;

  fprintf (fp, "---------------------------\n");
  fprintf (fp, "print_table\n");
  fprintf (fp, "---------------------------\n");

  for (idx = 0; idx < 256; idx++)
    {
      fprintf (fp, "LUM [%03d]  pixcount:%d\n",
               idx, (int)g_lum_tab[idx].all_samples);
      print_color_list (fp, g_lum_tab[idx].col_ptr);
    }
}

static void
print_transtable (FILE *fp)
{
  gint idx;

  if (fp == NULL)
    return;

  fprintf (fp, "---------------------------\n");
  fprintf (fp, "print_transtable\n");
  fprintf (fp, "---------------------------\n");

  for (idx = 0; idx < 256; idx++)
    {
      fprintf (fp, "LVL_TRANS [%03d]  in_lvl: %3d  out_lvl: %3d\n",
               idx, (int)g_lvl_trans_tab[idx], (int)g_out_trans_tab[idx]);
    }
}

static void
print_values (FILE *fp)
{
  if (fp == NULL)
    return;

  fprintf (fp, "sample_colorize: params\n");
  fprintf (fp, "g_values.hold_inten     :%d\n", (int)g_values.hold_inten);
  fprintf (fp, "g_values.orig_inten     :%d\n", (int)g_values.orig_inten);
  fprintf (fp, "g_values.rnd_subcolors  :%d\n", (int)g_values.rnd_subcolors);
  fprintf (fp, "g_values.guess_missing  :%d\n", (int)g_values.guess_missing);
  fprintf (fp, "g_values.lvl_in_min     :%d\n", (int)g_values.lvl_in_min);
  fprintf (fp, "g_values.lvl_in_max     :%d\n", (int)g_values.lvl_in_max);
  fprintf (fp, "g_values.lvl_in_gamma   :%f\n", g_values.lvl_in_gamma);
  fprintf (fp, "g_values.lvl_out_min    :%d\n", (int)g_values.lvl_out_min);
  fprintf (fp, "g_values.lvl_out_max    :%d\n", (int)g_values.lvl_out_max);

  fprintf (fp, "g_values.tol_col_err    :%f\n", g_values.tol_col_err);
}

/* -----------------------------
 * DEBUG print procedures END
 * -----------------------------
 */

/* DEBUG: read values from file */
static void
get_filevalues (void)
{
  FILE  *fp;
  gchar  buf[1000];

/*
  g_values.lvl_out_min = 0;
  g_values.lvl_out_max = 255;
  g_values.lvl_in_min = 0;
  g_values.lvl_in_max = 255;
  g_values.lvl_in_gamma = 1.0;
*/
  g_values.tol_col_err = 5.5;

  fp = g_fopen ("sample_colorize.values", "r");
  if (fp != NULL)
    {
      fgets (buf, 999, fp);
      sscanf (buf, "%f", &g_values.tol_col_err);
      fclose (fp);
    }

  g_printf ("g_values.tol_col_err    :%f\n", g_values.tol_col_err);
}

static gint32
color_error (guchar ref_red, guchar ref_green, guchar ref_blue,
             guchar cmp_red, guchar cmp_green, guchar cmp_blue)
{
  glong ff;
  glong fs;
  glong cmp_h, ref_h;

  /* 1. Brightness differences */
  cmp_h = (3 * cmp_red + 6 * cmp_green + cmp_blue) / 10;
  ref_h = (3 * ref_red + 6 * ref_green + ref_blue) / 10;

  fs = labs (ref_h - cmp_h);
  ff = fs * fs;

  /* 2. add Red Color differences */
  fs = abs (ref_red - cmp_red);
  ff += (fs * fs);

  /* 3. add  Green Color differences */
  fs = abs (ref_green - cmp_green);
  ff += (fs * fs);

  /* 4. add  Blue Color differences */
  fs = abs (ref_blue - cmp_blue);
  ff += (fs * fs);

  return ((gint32)(ff));
}

/* get pixel value
 *   return light gray transparent pixel if out of bounds
 *   (should occur in the previews only)
 */
static void
get_pixel (t_GDRW *gdrw,
           gint32  x,
           gint32  y,
           guchar *pixel)
{
  gegl_buffer_sample (gdrw->buffer, x, y, NULL, pixel, gdrw->format,
                      GEGL_SAMPLER_NEAREST, GEGL_ABYSS_NONE);
}

/* clear table */
static void
clear_tables (void)
{
  guint i;

  for (i = 0; i < 256; i++)
    {
      g_lum_tab[i].col_ptr = NULL;
      g_lum_tab[i].all_samples = 0;
      g_lvl_trans_tab[i] = i;
      g_out_trans_tab[i] = i;
      g_sample_color_tab[3 * i + 0] = i;
      g_sample_color_tab[3 * i + 1] = i;
      g_sample_color_tab[3 * i + 2] = i;
    }
}

/* free all allocated sample colors in table g_lum_tab */
static void
free_colors (void)
{
  gint               lum;
  t_samp_color_elem *col_ptr;
  t_samp_color_elem *next_ptr;

  for (lum = 0; lum < 256; lum++)
    {
      for (col_ptr = g_lum_tab[lum].col_ptr;
           col_ptr != NULL;
           col_ptr = next_ptr)
        {
          next_ptr = (t_samp_color_elem *)col_ptr->next;
          g_free (col_ptr);
        }

      g_lum_tab[lum].col_ptr = NULL;
      g_lum_tab[lum].all_samples = 0;
    }
}

/* setup lum transformer table according to input_levels, gamma and output levels
 * (uses sam algorithm as LIGMA Level Tool)
 */
static void
calculate_level_transfers (void)
{
  double inten;
  gint   i;
  gint   in_min, in_max;
  gint   out_min, out_max;

  if (g_values.lvl_in_max >= g_values.lvl_in_min)
    {
      in_max = g_values.lvl_in_max;
      in_min = g_values.lvl_in_min;
    }
  else
    {
      in_max = g_values.lvl_in_min;
      in_min = g_values.lvl_in_max;
    }
  if (g_values.lvl_out_max >= g_values.lvl_out_min)
    {
      out_max = g_values.lvl_out_max;
      out_min = g_values.lvl_out_min;
    }
  else
    {
      out_max = g_values.lvl_out_min;
      out_min = g_values.lvl_out_max;
    }

  /*  Recalculate the levels arrays  */
  for (i = 0; i < 256; i++)
    {
      /*  determine input intensity  */
      inten = (double) i / 255.0;
      if (g_values.lvl_in_gamma != 0.0)
        {
          inten = pow (inten, (1.0 / g_values.lvl_in_gamma));
        }
      inten = (double) (inten * (in_max - in_min) + in_min);
      inten = CLAMP (inten, 0.0, 255.0);
      g_lvl_trans_tab[i] = (guchar) (inten + 0.5);

      /*  determine the output intensity  */
      inten = (double) i / 255.0;
      inten = (double) (inten * (out_max - out_min) + out_min);
      inten = CLAMP (inten, 0.0, 255.0);
      g_out_trans_tab[i] = (guchar) (inten + 0.5);
    }
}



/* alloc and init new col Element */
static t_samp_color_elem *
new_samp_color (const guchar *color)
{
  t_samp_color_elem *col_ptr;

  col_ptr = g_new0 (t_samp_color_elem, 1);

  memcpy (&col_ptr->color[0], color, 4);

  col_ptr->sum_color = 1;
  col_ptr->next = NULL;

  return col_ptr;
}


/* store color in g_lum_tab  */
static void
add_color (const guchar *color)
{
  gint32             lum;
  t_samp_color_elem *col_ptr;

  lum = LUMINOSITY_1(color);

  g_lum_tab[lum].all_samples++;
  g_lum_tab[lum].from_sample = TRUE;

  /* check if exactly the same color is already in the list */
  for (col_ptr = g_lum_tab[lum].col_ptr;
       col_ptr != NULL;
       col_ptr = (t_samp_color_elem *)col_ptr->next)
    {
      if ((color[0] == col_ptr->color[0]) &&
          (color[1] == col_ptr->color[1]) &&
          (color[2] == col_ptr->color[2]))
        {
          col_ptr->sum_color++;
          return;
        }
    }

  /* alloc and init element for the new color  */
  col_ptr = new_samp_color (color);

  if (col_ptr != NULL)
    {
      /* add new color element as 1.st of the list */
      col_ptr->next = g_lum_tab[lum].col_ptr;
      g_lum_tab[lum].col_ptr = col_ptr;
    }
}

/* sort Sublists (color) by descending sum_color in g_lum_tab
 */
static void
sort_color (gint32 lum)
{
  t_samp_color_elem *col_ptr;
  t_samp_color_elem *next_ptr;
  t_samp_color_elem *prev_ptr;
  t_samp_color_elem *sorted_col_ptr;
  gint32             min;
  gint32             min_next;

  sorted_col_ptr = NULL;
  min_next           = 0;

  while (g_lum_tab[lum].col_ptr != NULL)
    {
      min = min_next;
      next_ptr = NULL;
      prev_ptr = NULL;

      for (col_ptr = g_lum_tab[lum].col_ptr;
           col_ptr != NULL;
           col_ptr = next_ptr)
        {
          next_ptr = col_ptr->next;
          if (col_ptr->sum_color > min)
            {
              /* check min value for next loop */
              if ((col_ptr->sum_color < min_next) || (min == min_next))
                {
                  min_next = col_ptr->sum_color;
                }
              prev_ptr = col_ptr;
            }
          else
            {
              /* add element at head of sorted list */
              col_ptr->next = sorted_col_ptr;
              sorted_col_ptr = col_ptr;

              /* remove element from list */
              if (prev_ptr == NULL)
                {
                  g_lum_tab[lum].col_ptr = next_ptr;  /* remove 1.st element */
                }
              else
                {
                  prev_ptr->next = next_ptr;
                }
            }
        }
    }

  g_lum_tab[lum].col_ptr = sorted_col_ptr;
}

static void
cnt_same_sample_colortones (t_samp_color_elem *ref_ptr,
                            guchar            *prev_color,
                            guchar            *color_tone,
                            gint              *csum)
{
  gint32             col_error, ref_error;
  t_samp_color_elem *col_ptr;

  ref_error = 0;
  if (prev_color != NULL)
    {
      ref_error = color_error (ref_ptr->color[0], ref_ptr->color[1], ref_ptr->color[2],
                                 prev_color[0],     prev_color[1],     prev_color[2]);
    }

  /* collect colors that are (nearly) the same */
  for (col_ptr = ref_ptr->next;
       col_ptr != NULL;
       col_ptr = (t_samp_color_elem *)col_ptr->next)
    {
      if (col_ptr->sum_color < 1)
        continue;
      col_error = color_error (ref_ptr->color[0], ref_ptr->color[1], ref_ptr->color[2],
                               col_ptr->color[0], col_ptr->color[1], col_ptr->color[2]);

      if (col_error <= g_tol_col_err)
        {
          /* cout color of the same colortone */
          *csum += col_ptr->sum_color;
          /* mark the already checked color with negative sum_color value */
          col_ptr->sum_color = 0 - col_ptr->sum_color;

          if (prev_color != NULL)
            {
              col_error = color_error (col_ptr->color[0], col_ptr->color[1], col_ptr->color[2],
                                       prev_color[0],     prev_color[1],     prev_color[2]);
              if (col_error < ref_error)
                {
                  /* use the color that is closest to prev_color */
                  memcpy (color_tone, &col_ptr->color[0], 3);
                  ref_error = col_error;
                }
            }
        }
    }
}

/* find the dominant colortones (out of all sample colors)
 * for each available brightness intensity value.
 * and store them in g_sample_color_tab
 */
static void
ideal_samples (void)
{
  gint32             lum;
  t_samp_color_elem *col_ptr;
  guchar            *color;

  guchar             color_tone[4];
  guchar             color_ideal[4];
  gint               csum, maxsum;

  color = NULL;
  for (lum = 0; lum < 256; lum++)
    {
      if (g_lum_tab[lum].col_ptr == NULL)
        continue;

      sort_color (lum);
      col_ptr = g_lum_tab[lum].col_ptr;
      memcpy (&color_ideal[0], &col_ptr->color[0], 3);

      maxsum = 0;

      /* collect colors that are (nearly) the same */
      for (;
           col_ptr != NULL;
           col_ptr = (t_samp_color_elem *)col_ptr->next)
        {
          csum = 0;
          if (col_ptr->sum_color > 0)
            {
              memcpy (&color_tone[0], &col_ptr->color[0], 3);
              cnt_same_sample_colortones (col_ptr, color,
                                          &color_tone[0], &csum);
              if (csum > maxsum)
                {
                  maxsum = csum;
                  memcpy (&color_ideal[0], &color_tone[0], 3);
                }
            }
          else
            col_ptr->sum_color = abs (col_ptr->sum_color);
        }

      /* store ideal color and keep track of the color */
      color = &g_sample_color_tab[3 * lum];
      memcpy (color, &color_ideal[0], 3);
    }
}

static void
guess_missing_colors (void)
{
  gint32  lum;
  gint32  idx;
  gfloat  div;

  guchar  lo_color[4];
  guchar  hi_color[4];
  guchar  new_color[4];

  lo_color[0] = 0;
  lo_color[1] = 0;
  lo_color[2] = 0;
  lo_color[3] = 255;

  hi_color[0] = 255;
  hi_color[1] = 255;
  hi_color[2] = 255;
  hi_color[3] = 255;

  new_color[0] = 0;
  new_color[1] = 0;
  new_color[2] = 0;
  new_color[3] = 255;

  for (lum = 0; lum < 256; lum++)
    {
      if ((g_lum_tab[lum].col_ptr == NULL) ||
          (g_lum_tab[lum].from_sample == FALSE))
        {
          if (lum > 0)
            {
              for (idx = lum; idx < 256; idx++)
                {
                  if ((g_lum_tab[idx].col_ptr != NULL) &&
                      g_lum_tab[idx].from_sample)
                    {
                      memcpy (&hi_color[0],
                              &g_sample_color_tab[3 * idx], 3);
                      break;
                    }
                  if (idx == 255)
                    {
                      hi_color[0] = 255;
                      hi_color[1] = 255;
                      hi_color[2] = 255;
                      break;
                    }
                }

              div = idx - (lum -1);
              new_color[0] = lo_color[0] + ((float)(hi_color[0] - lo_color[0]) / div);
              new_color[1] = lo_color[1] + ((float)(hi_color[1] - lo_color[1]) / div);
              new_color[2] = lo_color[2] + ((float)(hi_color[2] - lo_color[2]) / div);

/*
 *          printf ("LO: %03d %03d %03d HI: %03d %03d %03d   NEW: %03d %03d %03d\n",
 *                  (int)lo_color[0],  (int)lo_color[1],  (int)lo_color[2],
 *                  (int)hi_color[0],  (int)hi_color[1],  (int)hi_color[2],
 *                  (int)new_color[0], (int)new_color[1], (int)new_color[2]);
 */

            }
          g_lum_tab[lum].col_ptr = new_samp_color (&new_color[0]);
          g_lum_tab[lum].from_sample = FALSE;
          memcpy (&g_sample_color_tab [3 * lum], &new_color[0], 3);
        }
      memcpy (&lo_color[0], &g_sample_color_tab [3 * lum], 3);
    }
}

static void
fill_missing_colors (void)
{
  gint32 lum;
  gint32 idx;
  gint32 lo_idx;

  guchar lo_color[4];
  guchar hi_color[4];
  guchar new_color[4];

  lo_color[0] = 0;
  lo_color[1] = 0;
  lo_color[2] = 0;
  lo_color[3] = 255;

  hi_color[0] = 255;
  hi_color[1] = 255;
  hi_color[2] = 255;
  hi_color[3] = 255;

  new_color[0] = 0;
  new_color[1] = 0;
  new_color[2] = 0;
  new_color[3] = 255;

  lo_idx = 0;
  for (lum = 0; lum < 256; lum++)
    {
      if ((g_lum_tab[lum].col_ptr == NULL) ||
          (g_lum_tab[lum].from_sample == FALSE))
        {
          if (lum > 0)
            {
              for (idx = lum; idx < 256; idx++)
                {
                  if ((g_lum_tab[idx].col_ptr != NULL) &&
                      (g_lum_tab[idx].from_sample))
                    {
                      memcpy (&hi_color[0],
                              &g_sample_color_tab[3 * idx], 3);
                      break;
                    }

                  if (idx == 255)
                    {
/*
 *               hi_color[0] = 255;
 *               hi_color[1] = 255;
 *               hi_color[2] = 255;
 */
                      memcpy (&hi_color[0], &lo_color[0], 3);
                      break;
                    }
                }

              if ((lum > (lo_idx + ((idx - lo_idx ) / 2))) ||
                  (lo_idx == 0))
                {
                  new_color[0] = hi_color[0];
                  new_color[1] = hi_color[1];
                  new_color[2] = hi_color[2];
                }
              else
                {
                  new_color[0] = lo_color[0];
                  new_color[1] = lo_color[1];
                  new_color[2] = lo_color[2];
                }
            }

          g_lum_tab[lum].col_ptr = new_samp_color (&new_color[0]);
          g_lum_tab[lum].from_sample = FALSE;
          memcpy (&g_sample_color_tab[3 * lum], &new_color[0], 3);
        }
      else
        {
          lo_idx = lum;
          memcpy (&lo_color[0], &g_sample_color_tab[3 * lum], 3);
        }
    }
}

/* get 256 samples of active gradient (optional in inverse order) */
static void
get_gradient (gint mode)
{
  gchar   *name;
  gint     n_f_samples;
  gdouble *f_samples;
  gdouble *f_samp;        /* float samples */
  gint     lum;

  free_colors ();

  name = ligma_context_get_gradient ();

  ligma_gradient_get_uniform_samples (name, 256 /* n_samples */,
                                     mode == SMP_INV_GRADIENT,
                                     &n_f_samples, &f_samples);

  g_free (name);

  for (lum = 0; lum < 256; lum++)
    {
      f_samp = &f_samples[lum * 4];

      g_sample_color_tab[3 * lum + 0] = f_samp[0] * 255;
      g_sample_color_tab[3 * lum + 1] = f_samp[1] * 255;
      g_sample_color_tab[3 * lum + 2] = f_samp[2] * 255;

      g_lum_tab[lum].col_ptr =
        new_samp_color (&g_sample_color_tab[3 * lum]);
      g_lum_tab[lum].from_sample = TRUE;
      g_lum_tab[lum].all_samples = 1;
    }

  g_free (f_samples);
}

static void
end_gdrw (t_GDRW *gdrw)
{
  t_GDRW *sel_gdrw = (t_GDRW *) gdrw->sel_gdrw;

  if (sel_gdrw && sel_gdrw->buffer)
    {
      g_object_unref (sel_gdrw->buffer);
      sel_gdrw->buffer = NULL;
    }

  g_object_unref (gdrw->buffer);
  gdrw->buffer = NULL;
}

static void
init_gdrw (t_GDRW       *gdrw,
           LigmaDrawable *drawable,
           gboolean      shadow)
{
  LigmaImage    *image;
  LigmaDrawable *sel_channel;
  gint32        x1, x2, y1, y2;
  gint          offsetx, offsety;
  gint          w, h;
  gint          sel_offsetx, sel_offsety;
  t_GDRW       *sel_gdrw;
  gint32        non_empty;

  gdrw->drawable = drawable;

  if (shadow)
    gdrw->buffer = ligma_drawable_get_shadow_buffer (drawable);
  else
    gdrw->buffer = ligma_drawable_get_buffer (drawable);

  gdrw->width = ligma_drawable_get_width (drawable);
  gdrw->height = ligma_drawable_get_height (drawable);
  gdrw->tile_width = ligma_tile_width ();
  gdrw->tile_height = ligma_tile_height ();
  gdrw->shadow = shadow;
  gdrw->seldeltax = 0;
  gdrw->seldeltay = 0;
  /* get offsets within the image */
  ligma_drawable_get_offsets (gdrw->drawable, &offsetx, &offsety);

  if (! ligma_drawable_mask_intersect (gdrw->drawable,
                                      &gdrw->x1, &gdrw->y1, &w, &h))
    return;

  gdrw->x2 = gdrw->x1 + w;
  gdrw->y2 = gdrw->y1 + h;

  if (ligma_drawable_has_alpha (drawable))
    gdrw->format = babl_format ("R'G'B'A u8");
  else
    gdrw->format = babl_format ("R'G'B' u8");

  gdrw->bpp = babl_format_get_bytes_per_pixel (gdrw->format);

  if (ligma_drawable_has_alpha (drawable))
    {
      /* index of the alpha channelbyte {1|3} */
      gdrw->index_alpha = gdrw->bpp -1;
    }
  else
    {
      gdrw->index_alpha = 0;      /* there is no alpha channel */
    }

  image = ligma_item_get_image (LIGMA_ITEM (gdrw->drawable));

  /* check and see if we have a selection mask */
  sel_channel  = LIGMA_DRAWABLE (ligma_image_get_selection (image));

  ligma_selection_bounds (image, &non_empty, &x1, &y1, &x2, &y2);

  if (non_empty && sel_channel)
    {
      /* selection is TRUE */
      sel_gdrw = g_new0 (t_GDRW, 1);
      sel_gdrw->drawable = sel_channel;

      sel_gdrw->buffer = ligma_drawable_get_buffer (sel_channel);
      sel_gdrw->format = babl_format ("Y u8");

      sel_gdrw->width = ligma_drawable_get_width (sel_channel);
      sel_gdrw->height = ligma_drawable_get_height (sel_channel);

      sel_gdrw->tile_width = ligma_tile_width ();
      sel_gdrw->tile_height = ligma_tile_height ();
      sel_gdrw->shadow = shadow;
      sel_gdrw->x1 = x1;
      sel_gdrw->y1 = y1;
      sel_gdrw->x2 = x2;
      sel_gdrw->y2 = y2;
      sel_gdrw->seldeltax = 0;
      sel_gdrw->seldeltay = 0;
      sel_gdrw->bpp = babl_format_get_bytes_per_pixel (sel_gdrw->format);
      sel_gdrw->index_alpha = 0;   /* there is no alpha channel */
      sel_gdrw->sel_gdrw = NULL;

      /* offset delta between drawable and selection
       * (selection always has image size and should always have offsets of 0 )
       */
      ligma_drawable_get_offsets (sel_channel, &sel_offsetx, &sel_offsety);
      gdrw->seldeltax = offsetx - sel_offsetx;
      gdrw->seldeltay = offsety - sel_offsety;

      gdrw->sel_gdrw = (t_GDRW *) sel_gdrw;

      if (g_Sdebug)
        {
          g_printf ("init_gdrw: SEL_BOUNDS x1: %d y1: %d x2:%d y2: %d\n",
                    (int)sel_gdrw->x1, (int)sel_gdrw->y1,
                    (int)sel_gdrw->x2, (int)sel_gdrw->y2);
          g_printf ("init_gdrw: SEL_OFFS   x: %d y: %d\n",
                    (int)sel_offsetx, (int)sel_offsety );
          g_printf ("init_gdrw: SEL_DELTA  x: %d y: %d\n",
                    (int)gdrw->seldeltax, (int)gdrw->seldeltay );
        }
    }
  else
    gdrw->sel_gdrw = NULL;     /* selection is FALSE */
}

/* analyze the colors in the sample_drawable */
static int
sample_analyze (t_GDRW *sample_gdrw)
{
  gint32  sample_pixels;
  gint32  row, col;
  gint32  first_row, first_col, last_row, last_col;
  gint32  x, y;
  gint32  x2, y2;
  float   progress_step;
  float   progress_max;
  float   progress;
  guchar  color[4];
  FILE   *prot_fp;

  sample_pixels = 0;

  /* init progress */
  progress_max = (sample_gdrw->x2 - sample_gdrw->x1);
  progress_step = 1.0 / progress_max;
  progress = 0.0;
  if (g_show_progress)
    ligma_progress_init (_("Sample analyze"));

  prot_fp = NULL;
  if (g_Sdebug)
    prot_fp = g_fopen ("sample_colors.dump", "w");
  print_values (prot_fp);

  /* ------------------------------------------------
   * foreach pixel in the SAMPLE_drawable:
   *  calculate brightness intensity LUM
   * ------------------------------------------------
   * the inner loops (x/y) are designed to process
   * all pixels of one tile in the sample drawable, the outer loops (row/col) do step
   * to the next tiles. (this was done to reduce tile swapping)
   */

  first_row = sample_gdrw->y1 / sample_gdrw->tile_height;
  last_row  = (sample_gdrw->y2 / sample_gdrw->tile_height);
  first_col = sample_gdrw->x1 / sample_gdrw->tile_width;
  last_col  = (sample_gdrw->x2 / sample_gdrw->tile_width);

  for (row = first_row; row <= last_row; row++)
    {
      for (col = first_col; col <= last_col; col++)
        {
          if (col == first_col)
            x = sample_gdrw->x1;
          else
            x = col * sample_gdrw->tile_width;
          if (col == last_col)
            x2 = sample_gdrw->x2;
          else
            x2 = (col +1) * sample_gdrw->tile_width;

          for ( ; x < x2; x++)
            {
              if (row == first_row)
                y = sample_gdrw->y1;
              else
                y = row * sample_gdrw->tile_height;
              if (row == last_row)
                y2 = sample_gdrw->y2;
              else
                y2 = (row +1) * sample_gdrw->tile_height ;

              /* printf ("X: %4d Y:%4d Y2:%4d\n", (int)x, (int)y, (int)y2); */

              for ( ; y < y2; y++)
                {
                  /* check if the pixel is in the selection */
                  if (sample_gdrw->sel_gdrw)
                    {
                      get_pixel (sample_gdrw->sel_gdrw,
                                   (x + sample_gdrw->seldeltax),
                                   (y + sample_gdrw->seldeltay),
                                   &color[0]);

                      if (color[0] == 0)
                        continue;
                    }
                  get_pixel (sample_gdrw, x, y, &color[0]);

                  /* if this is a visible (non-transparent) pixel */
                  if ((sample_gdrw->index_alpha < 1) ||
                      (color[sample_gdrw->index_alpha] != 0))
                    {
                      /* store color in the sublists of g_lum_tab  */
                      add_color (&color[0]);
                      sample_pixels++;
                    }
                }

              if (g_show_progress)
                ligma_progress_update (progress += progress_step);
            }
        }
    }

  if (g_show_progress)
    ligma_progress_update (1.0);

  if (g_Sdebug)
    g_printf ("ROWS: %d - %d  COLS: %d - %d\n",
            (int)first_row, (int)last_row,
            (int)first_col, (int)last_col);

  print_table (prot_fp);

  if (g_Sdebug)
    print_ppm ("sample_color_all.ppm");

  /* find out ideal sample colors for each brightness intensity (lum)
   * and set g_sample_color_tab to the ideal colors.
   */
  ideal_samples ();
  calculate_level_transfers ();

  if (g_values.guess_missing)
    guess_missing_colors ();
  else
    fill_missing_colors ();

  print_table (prot_fp);
  if (g_Sdebug)
    print_ppm ("sample_color_2.ppm");
  print_transtable (prot_fp);
  if (prot_fp)
    fclose (prot_fp);

  /* check if there was at least one visible pixel */
  if (sample_pixels == 0)
    {
      g_printf ("Error: Source sample has no visible Pixel\n");
      return -1;
    }
  return 0;
}

static void
rnd_remap (gint32  lum,
           guchar *mapped_color)
{
  t_samp_color_elem *col_ptr;
  gint               rnd;
  gint               ct;
  gint               idx;

  if (g_lum_tab[lum].all_samples > 1)
    {
      rnd = g_random_int_range (0, g_lum_tab[lum].all_samples);
      ct  = 0;
      idx = 0;

      for (col_ptr = g_lum_tab[lum].col_ptr;
           col_ptr != NULL;
           col_ptr = (t_samp_color_elem *)col_ptr->next)
        {
          ct += col_ptr->sum_color;

          if (rnd < ct)
            {
              /* printf ("RND_remap: rnd: %d all:%d  ct:%d idx:%d\n",
               * rnd, (int)g_lum_tab[lum].all_samples, ct, idx);
               */
              memcpy (mapped_color, &col_ptr->color[0], 3);
              return;
            }
          idx++;
        }
    }

  memcpy (mapped_color, &g_sample_color_tab[lum + lum + lum], 3);
}

static void
remap_pixel (guchar       *pixel,
             const guchar *original,
             gint          bpp2)
{
  guchar mapped_color[4];
  gint   lum;
  double orig_lum, mapped_lum;
  double grn, red, blu;
  double mg,  mr,  mb;
  double dg,  dr,  db;
  double dlum;

  /* get brightness from (uncolorized) original */
  lum = g_out_trans_tab[g_lvl_trans_tab[LUMINOSITY_1 (original)]];
  if (g_values.rnd_subcolors)
    rnd_remap (lum, mapped_color);
  else
    memcpy (mapped_color, &g_sample_color_tab[3 * lum], 3);

  if (g_values.hold_inten)
    {
      if (g_values.orig_inten)
        orig_lum = LUMINOSITY_0(original);
      else
        orig_lum = 100.0 * g_lvl_trans_tab[LUMINOSITY_1 (original)];

      mapped_lum = LUMINOSITY_0 (mapped_color);

      if (mapped_lum == 0)
        {
          /* convert black to greylevel with desired brightness value */
          mapped_color[0] = orig_lum / 100.0;
          mapped_color[1] = mapped_color[0];
          mapped_color[2] = mapped_color[0];
        }
      else
        {
          /* Calculate theoretical RGB to reach given intensity LUM
           * value (orig_lum)
           */
          mr = mapped_color[0];
          mg = mapped_color[1];
          mb = mapped_color[2];

          if (mr > 0.0)
            {
              red =
                orig_lum / (30 + (59 * mg / mr) + (11 * mb / mr));
              grn = mg * red / mr;
              blu = mb * red / mr;
            }
          else if (mg > 0.0)
            {
              grn =
                orig_lum / ((30 * mr / mg) + 59 + (11 * mb / mg));
              red = mr * grn / mg;
              blu = mb * grn / mg;
            }
          else
            {
              blu =
                orig_lum / ((30 * mr / mb) + (59 * mg / mb) + 11);
              grn = mg * blu / mb;
              red = mr * blu / mb;
            }

          /* on overflow: Calculate real RGB values
           * (this may change the hue and saturation,
           * more and more into white)
           */

          if (red > 255)
            {
              if ((blu < 255) && (grn < 255))
                {
                  /* overflow in the red channel (compensate with green and blue)  */
                  dlum = (red - 255.0) * 30.0;
                  if (mg > 0)
                    {
                      dg = dlum / (59.0 + (11.0 * mb / mg));
                      db = dg * mb / mg;
                    }
                  else if (mb > 0)
                    {
                      db = dlum / (11.0 + (59.0 * mg / mb));
                      dg = db * mg / mb;
                    }
                  else
                    {
                      db = dlum / (11.0 + 59.0);
                      dg = dlum / (59.0 + 11.0);
                    }

                  grn += dg;
                  blu += db;
                }

              red = 255.0;

              if (grn > 255)
                {
                  grn = 255.0;
                  blu = (orig_lum - 22695) / 11;  /* 22695 =  (255 * 30) + (255 * 59)  */
                }
              if (blu > 255)
                {
                  blu = 255.0;
                  grn = (orig_lum - 10455) / 59;  /* 10455 =  (255 * 30) + (255 * 11)  */
                }
            }
          else if (grn > 255)
            {
              if ((blu < 255) && (red < 255))
                {
                  /* overflow in the green channel (compensate with red and blue)  */
                  dlum = (grn - 255.0) * 59.0;

                  if (mr > 0)
                    {
                      dr = dlum / (30.0 + (11.0 * mb / mr));
                      db = dr * mb / mr;
                    }
                  else if (mb > 0)
                    {
                      db = dlum / (11.0 + (30.0 * mr / mb));
                      dr = db * mr / mb;
                    }
                  else
                    {
                      db = dlum / (11.0 + 30.0);
                      dr = dlum / (30.0 + 11.0);
                    }

                  red += dr;
                  blu += db;
                }

              grn = 255.0;

              if (red > 255)
                {
                  red = 255.0;
                  blu = (orig_lum - 22695) / 11;  /* 22695 =  (255*59) + (255*30)  */
                }
              if (blu > 255)
                {
                  blu = 255.0;
                  red = (orig_lum - 17850) / 30;  /* 17850 =  (255*59) + (255*11)  */
                }
            }
          else if (blu > 255)
            {
              if ((red < 255) && (grn < 255))
                {
                  /* overflow in the blue channel (compensate with green and red)  */
                  dlum = (blu - 255.0) * 11.0;

                  if (mg > 0)
                    {
                      dg = dlum / (59.0 + (30.0 * mr / mg));
                      dr = dg * mr / mg;
                    }
                  else if (mr > 0)
                    {
                      dr = dlum / (30.0 + (59.0 * mg / mr));
                      dg = dr * mg / mr;
                    }
                  else
                    {
                      dr = dlum / (30.0 + 59.0);
                      dg = dlum / (59.0 + 30.0);
                    }

                  grn += dg;
                  red += dr;
                }

              blu = 255.0;

              if (grn > 255)
                {
                  grn = 255.0;
                  red = (orig_lum - 17850) / 30;  /* 17850 =  (255*11) + (255*59)  */
                }
              if (red > 255)
                {
                  red = 255.0;
                  grn = (orig_lum - 10455) / 59;  /* 10455 =  (255*11) + (255*30)  */
                }
            }

          mapped_color[0] = CLAMP0255 (red + 0.5);
          mapped_color[1] = CLAMP0255 (grn + 0.5);
          mapped_color[2] = CLAMP0255 (blu + 0.5);
        }
    }

  /* set colorized pixel in shadow pr */
  memcpy (pixel, &mapped_color[0], bpp2);
}

static void
colorize_func (const guchar *src,
               guchar       *dest,
               gint          bpp,
               gboolean      has_alpha)
{
  if (has_alpha)
    {
      bpp--;
      dest[bpp] = src[bpp];
    }

  remap_pixel (dest, src, bpp);
}

static void
colorize_drawable (LigmaDrawable *drawable)
{
  GeglBuffer         *src_buffer;
  GeglBuffer         *dest_buffer;
  GeglBufferIterator *iter;
  const Babl         *format;
  gboolean            has_alpha;
  gint                bpp;
  gint                x, y, w, h;
  gint                total_area;
  gint                area_so_far;

  if (! ligma_drawable_mask_intersect (drawable, &x, &y, &w, &h))
    return;

  src_buffer  = ligma_drawable_get_buffer (drawable);
  dest_buffer = ligma_drawable_get_shadow_buffer (drawable);

  has_alpha = ligma_drawable_has_alpha (drawable);

  if (has_alpha)
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  bpp = babl_format_get_bytes_per_pixel (format);

  if (g_show_progress)
    ligma_progress_init (_("Remap colorized"));


  total_area  = w * h;
  area_so_far = 0;

  if (total_area <= 0)
    goto out;

  iter = gegl_buffer_iterator_new (src_buffer,
                                   GEGL_RECTANGLE (x, y, w, h), 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);

  gegl_buffer_iterator_add (iter, dest_buffer,
                            GEGL_RECTANGLE (x, y, w, h), 0 , format,
                            GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (iter))
    {
      const guchar *src  = iter->items[0].data;
      guchar       *dest = iter->items[1].data;
      gint          row;

      for (row = 0; row < iter->items[0].roi.height; row++)
        {
          const guchar *s      = src;
          guchar       *d      = dest;
          gint          pixels = iter->items[0].roi.width;

          while (pixels--)
            {
              colorize_func (s, d, bpp, has_alpha);

              s += bpp;
              d += bpp;
            }

          src  += iter->items[0].roi.width * bpp;
          dest += iter->items[1].roi.width * bpp;
        }

      area_so_far += iter->items[0].roi.width * iter->items[0].roi.height;

      ligma_progress_update ((gdouble) area_so_far / (gdouble) total_area);
    }

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  ligma_drawable_merge_shadow (drawable, TRUE);
  ligma_drawable_update (drawable, x, y, w, h);

  out:
  if (g_show_progress)
    ligma_progress_update (0.0);
}

/* colorize dst_drawable like sample_drawable */
static int
main_colorize (gint mc_flags)
{
  t_GDRW   sample_gdrw;
  gboolean sample_drawable = FALSE;
  gint32   max;
  gint32   id;
  gint     rc;

  if (g_Sdebug)
    get_filevalues ();  /* for debugging: read values from file */

  /* calculate value of tolerable color error */
  max = color_error (0,0, 0, 255, 255, 255); /* 260100 */
  g_tol_col_err = (((float)max  * (g_values.tol_col_err * g_values.tol_col_err)) / (100.0 *100.0));
  g_max_col_err = max;

  rc = 0;

  if (mc_flags & MC_GET_SAMPLE_COLORS)
    {
      id = g_values.sample_id;
      if ((id == SMP_GRADIENT) || (id == SMP_INV_GRADIENT))
        {
          get_gradient (id);
        }
      else
        {
          sample_drawable = TRUE;

          init_gdrw (&sample_gdrw, ligma_drawable_get_by_id (id), FALSE);
          free_colors ();
          rc = sample_analyze (&sample_gdrw);
        }
    }

  if ((mc_flags & MC_DST_REMAP) && (rc == 0))
    {
      if (ligma_drawable_is_gray (g_values.dst) &&
          (mc_flags & MC_DST_REMAP))
        {
          ligma_image_convert_rgb (ligma_item_get_image (LIGMA_ITEM (g_values.dst)));
        }

      colorize_drawable (g_values.dst);
    }

  if (sample_drawable)
    end_gdrw (&sample_gdrw);

  return rc;
}
