/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Gradient Map plug-in
 * Copyright (C) 1997 Eiichi Takamori <taka@ma1.seikyou.ne.jp>
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

#include <libligma/ligma.h>

#include "libligma/stdplugins-intl.h"


/* Some useful macros */
#define GRADMAP_PROC    "plug-in-gradmap"
#define PALETTEMAP_PROC "plug-in-palettemap"
#define PLUG_IN_BINARY  "gradient-map"
#define PLUG_IN_ROLE    "ligma-gradient-map"
#define NSAMPLES         2048

typedef enum
  {
    GRADIENT_MODE = 1,
    PALETTE_MODE
  } MapMode;


typedef struct _Map      Map;
typedef struct _MapClass MapClass;

struct _Map
{
  LigmaPlugIn parent_instance;
};

struct _MapClass
{
  LigmaPlugInClass parent_class;
};


#define MAP_TYPE  (map_get_type ())
#define MAP (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAP_TYPE, Map))

GType                   map_get_type         (void) G_GNUC_CONST;

static GList          * map_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * map_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * map_run              (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              LigmaImage            *image,
                                              gint                  n_drawables,
                                              LigmaDrawable        **drawables,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static void             map                  (GeglBuffer           *buffer,
                                              GeglBuffer           *shadow_buffer,
                                              LigmaDrawable         *drawable,
                                              MapMode               mode);
static gdouble        * get_samples_gradient (LigmaDrawable         *drawable);
static gdouble        * get_samples_palette  (LigmaDrawable         *drawable);


G_DEFINE_TYPE (Map, map, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (MAP_TYPE)
DEFINE_STD_SET_I18N


static void
map_class_init (MapClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = map_query_procedures;
  plug_in_class->create_procedure = map_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
map_init (Map *map)
{
}

static GList *
map_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (GRADMAP_PROC));
  list = g_list_append (list, g_strdup (PALETTEMAP_PROC));

  return list;
}

static LigmaProcedure *
map_create_procedure (LigmaPlugIn  *plug_in,
                           const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, GRADMAP_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            map_run,
                                            GINT_TO_POINTER (GRADIENT_MODE),
                                            NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("_Gradient Map"));
      ligma_procedure_add_menu_path (procedure, "<Image>/Colors/Map");

      ligma_procedure_set_documentation (procedure,
                                        _("Recolor the image using colors "
                                          "from the active gradient"),
                                        "This plug-in maps the contents of "
                                        "the specified drawable with active "
                                        "gradient. It calculates luminosity "
                                        "of each pixel and replaces the pixel "
                                        "by the sample of active gradient at "
                                        "the position proportional to that "
                                        "luminosity. Complete black pixel "
                                        "becomes the leftmost color of the "
                                        "gradient, and complete white becomes "
                                        "the rightmost. Works on both "
                                        "Grayscale and RGB image "
                                        "with/without alpha channel.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Eiichi Takamori",
                                      "Eiichi Takamori",
                                      "1997");
    }
  else if (! strcmp (name, PALETTEMAP_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            map_run,
                                            GINT_TO_POINTER (PALETTE_MODE),
                                            NULL);

      ligma_procedure_set_image_types (procedure, "RGB*, GRAY*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, _("_Palette Map"));
      ligma_procedure_add_menu_path (procedure, "<Image>/Colors/Map");

      ligma_procedure_set_documentation (procedure,
                                        _("Recolor the image using colors "
                                          "from the active palette"),
                                        "This plug-in maps the contents of "
                                        "the specified drawable with the "
                                        "active palette. It calculates "
                                        "luminosity of each pixel and "
                                        "replaces the pixel by the palette "
                                        "sample at the corresponding index. "
                                        "A complete black pixel becomes the "
                                        "lowest palette entry, and complete "
                                        "white becomes the highest. Works on "
                                        "both Grayscale and RGB image "
                                        "with/without alpha channel.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Bill Skaggs",
                                      "Bill Skaggs",
                                      "2004");
    }

  return procedure;
}

static LigmaValueArray *
map_run (LigmaProcedure         *procedure,
         LigmaRunMode            run_mode,
         LigmaImage             *image,
         gint                   n_drawables,
         LigmaDrawable         **drawables,
         const LigmaValueArray  *args,
         gpointer               run_data)
{
  MapMode       mode = GPOINTER_TO_INT (run_data);
  GeglBuffer   *shadow_buffer;
  GeglBuffer   *buffer;
  LigmaDrawable *drawable;

  gegl_init (NULL, NULL);

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   ligma_procedure_get_name (procedure));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  shadow_buffer = ligma_drawable_get_shadow_buffer (drawable);
  buffer        = ligma_drawable_get_buffer (drawable);

  /*  Make sure that the drawable is gray or RGB color  */
  if (ligma_drawable_is_rgb  (drawable) ||
      ligma_drawable_is_gray (drawable))
    {
      if (mode == GRADIENT_MODE)
        {
          ligma_progress_init (_("Gradient Map"));
        }
      else
        {
          ligma_progress_init (_("Palette Map"));
        }

      map (buffer, shadow_buffer, drawable, mode);
    }
  else
    {
      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  g_object_unref (buffer);
  g_object_unref (shadow_buffer);

  ligma_drawable_merge_shadow (drawable, TRUE);

  ligma_drawable_update (drawable, 0, 0,
                        ligma_drawable_get_width  (drawable),
                        ligma_drawable_get_height (drawable));

  if (run_mode != LIGMA_RUN_NONINTERACTIVE)
    ligma_displays_flush ();

  return ligma_procedure_new_return_values (procedure, LIGMA_PDB_SUCCESS, NULL);
}

static void
map (GeglBuffer   *buffer,
     GeglBuffer   *shadow_buffer,
     LigmaDrawable *drawable,
     MapMode       mode)
{
  GeglBufferIterator *gi;
  gint                nb_color_chan;
  gint                nb_chan;
  gint                nb_chan2;
  gint                nb_chan_samp;
  gint                index_iter;
  gboolean            interpolate;
  gdouble            *samples;
  gboolean            is_rgb;
  gboolean            has_alpha;
  const Babl         *format_shadow;
  const Babl         *format_buffer;

  is_rgb    = ligma_drawable_is_rgb (drawable);
  has_alpha = ligma_drawable_has_alpha (drawable);

  switch (mode)
    {
    case GRADIENT_MODE:
      samples = get_samples_gradient (drawable);
      interpolate = TRUE;
      break;
    case PALETTE_MODE:
      samples = get_samples_palette (drawable);
      interpolate = FALSE;
      break;
    default:
      g_error ("plug_in_gradmap: invalid mode");
    }

  if (is_rgb)
    {
      nb_color_chan = 3;
      nb_chan_samp = 4;
      if (has_alpha)
        format_shadow = babl_format ("R'G'B'A float");
      else
        format_shadow = babl_format ("R'G'B' float");
    }
  else
    {
      nb_color_chan = 1;
      nb_chan_samp = 2;
      if (has_alpha)
        format_shadow = babl_format ("Y'A float");
      else
        format_shadow = babl_format ("Y' float");
    }


  if (has_alpha)
    {
      nb_chan = nb_color_chan + 1;
      nb_chan2 = 2;
      format_buffer = babl_format ("Y'A float");
    }
  else
    {
      nb_chan = nb_color_chan;
      nb_chan2 = 1;
      format_buffer = babl_format ("Y' float");
    }

  gi = gegl_buffer_iterator_new (shadow_buffer, NULL, 0, format_shadow,
                                 GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 2);

  index_iter = gegl_buffer_iterator_add (gi, buffer, NULL,
                                         0, format_buffer,
                                         GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  while (gegl_buffer_iterator_next (gi))
    {
      guint   k;
      gfloat *data;
      gfloat *data2;

      data  = (gfloat*) gi->items[0].data;
      data2 = (gfloat*) gi->items[index_iter].data;

      if (interpolate)
        {
          for (k = 0; k < gi->length; k++)
            {
              gint      b, ind1, ind2;
              gdouble  *samp1, *samp2;
              gfloat    c1, c2, val;

              val = data2[0] * (NSAMPLES-1);

              ind1 = CLAMP (floor (val), 0, NSAMPLES-1);
              ind2 = CLAMP (ceil  (val), 0, NSAMPLES-1);

              c1 = 1.0 - (val - ind1);
              c2 = 1.0 - c1;

              samp1 = &(samples[ind1 * nb_chan_samp]);
              samp2 = &(samples[ind2 * nb_chan_samp]);

              for (b = 0; b < nb_color_chan; b++)
                data[b] = (samp1[b] * c1 + samp2[b] * c2);

              if (has_alpha)
                {
                  float alpha = (samp1[b] * c1 + samp2[b] * c2);
                  data[b] = alpha * data2[1];
                }

              data += nb_chan;
              data2 += nb_chan2;
            }
        }
      else
        {
          for (k = 0; k < gi->length; k++)
            {
              gint     b, ind;
              gdouble *samp;
              ind = CLAMP (data2[0] * (NSAMPLES-1), 0, NSAMPLES-1);

              samp = &(samples[ind * nb_chan_samp]);

              for (b = 0; b < nb_color_chan; b++)
                data[b] = samp[b];

              if (has_alpha)
                {
                  data[b] = samp[b] * data2[1];
                }

              data += nb_chan;
              data2 += nb_chan2;
            }
        }
    }

  g_free (samples);
}

/*
  Returns 2048 samples of the gradient.
  Each sample is (R'G'B'A float) or (Y'A float), depending on the drawable
 */
static gdouble *
get_samples_gradient (LigmaDrawable *drawable)
{
  gchar   *gradient_name;
  gint     n_d_samples;
  gdouble *d_samples = NULL;

  gradient_name = ligma_context_get_gradient ();

  /* FIXME: "reverse" hardcoded to FALSE. */
  ligma_gradient_get_uniform_samples (gradient_name, NSAMPLES, FALSE,
                                     &n_d_samples, &d_samples);
  g_free (gradient_name);

  if (! ligma_drawable_is_rgb (drawable))
    {
      const Babl *format_src = babl_format ("R'G'B'A double");
      const Babl *format_dst = babl_format ("Y'A double");
      const Babl *fish = babl_fish (format_src, format_dst);

      babl_process (fish, d_samples, d_samples, NSAMPLES);
    }

  return d_samples;
}

/*
  Returns 2048 samples of the palette.
  Each sample is (R'G'B'A float) or (Y'A float), depending on the drawable
 */
static gdouble *
get_samples_palette (LigmaDrawable *drawable)
{
  gchar      *palette_name;
  LigmaRGB     color_sample;
  gdouble    *d_samples, *d_samp;
  gboolean    is_rgb;
  gdouble     factor;
  gint        pal_entry, num_colors;
  gint        nb_color_chan, nb_chan, i;
  const Babl *format;

  palette_name = ligma_context_get_palette ();
  ligma_palette_get_info (palette_name, &num_colors);

  is_rgb = ligma_drawable_is_rgb (drawable);

  factor = ((double) num_colors) / NSAMPLES;
  format = is_rgb ? babl_format ("R'G'B'A double") : babl_format ("Y'A double");
  nb_color_chan = is_rgb ? 3 : 1;
  nb_chan = nb_color_chan + 1;

  d_samples = g_new (gdouble, NSAMPLES * nb_chan);

  for (i = 0; i < NSAMPLES; i++)
    {
      d_samp = &d_samples[i * nb_chan];
      pal_entry = CLAMP ((int)(i * factor), 0, num_colors - 1);

      ligma_palette_entry_get_color (palette_name, pal_entry, &color_sample);
      ligma_rgb_get_pixel (&color_sample,
                          format,
                          d_samp);
    }

  g_free (palette_name);
  return d_samples;
}
