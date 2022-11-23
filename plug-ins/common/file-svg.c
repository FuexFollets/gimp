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

/* SVG loader plug-in
 * (C) Copyright 2003  Dom Lachowicz <cinamod@hotmail.com>
 *
 * Largely rewritten in September 2003 by Sven Neumann <sven@ligma.org>
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include <librsvg/rsvg.h>

#include "libligma/ligma.h"
#include "libligma/ligmaui.h"

#include "libligma/stdplugins-intl.h"


#define LOAD_PROC               "file-svg-load"
#define LOAD_THUMB_PROC         "file-svg-load-thumb"
#define PLUG_IN_BINARY          "file-svg"
#define PLUG_IN_ROLE            "ligma-file-svg"
#define SVG_VERSION             "2.5.0"
#define SVG_DEFAULT_RESOLUTION  90.0
#define SVG_DEFAULT_SIZE        500
#define SVG_PREVIEW_SIZE        128


typedef struct
{
  gdouble    resolution;
  gint       width;
  gint       height;
  gboolean   import;
  gboolean   merge;
} SvgLoadVals;


typedef struct _Svg      Svg;
typedef struct _SvgClass SvgClass;

struct _Svg
{
  LigmaPlugIn      parent_instance;
};

struct _SvgClass
{
  LigmaPlugInClass parent_class;
};


#define SVG_TYPE  (svg_get_type ())
#define SVG (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVG_TYPE, Svg))

GType                   svg_get_type         (void) G_GNUC_CONST;

static GList          * svg_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * svg_create_procedure (LigmaPlugIn           *plug_in,
                                              const gchar          *name);

static LigmaValueArray * svg_load             (LigmaProcedure        *procedure,
                                              LigmaRunMode           run_mode,
                                              GFile                *file,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);
static LigmaValueArray * svg_load_thumb       (LigmaProcedure        *procedure,
                                              GFile                *file,
                                              gint                  size,
                                              const LigmaValueArray *args,
                                              gpointer              run_data);

static LigmaImage         * load_image        (GFile                *file,
                                              RsvgHandleFlags       rsvg_flags,
                                              GError              **error);
static GdkPixbuf         * load_rsvg_pixbuf  (GFile                *file,
                                              SvgLoadVals          *vals,
                                              RsvgHandleFlags       rsvg_flags,
                                              gboolean             *allow_retry,
                                              GError              **error);
static gboolean            load_rsvg_size    (GFile                *file,
                                              SvgLoadVals          *vals,
                                              RsvgHandleFlags       rsvg_flags,
                                              GError              **error);
static LigmaPDBStatusType   load_dialog       (GFile                *file,
                                              RsvgHandleFlags      *rsvg_flags,
                                              GError              **error);



G_DEFINE_TYPE (Svg, svg, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (SVG_TYPE)
DEFINE_STD_SET_I18N


static SvgLoadVals load_vals =
{
  SVG_DEFAULT_RESOLUTION,
  0,
  0,
  FALSE,
  FALSE
};


static void
svg_class_init (SvgClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = svg_query_procedures;
  plug_in_class->create_procedure = svg_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
svg_init (Svg *svg)
{
}

static GList *
svg_query_procedures (LigmaPlugIn *plug_in)
{
  GList *list = NULL;

  list = g_list_append (list, g_strdup (LOAD_THUMB_PROC));
  list = g_list_append (list, g_strdup (LOAD_PROC));

  return list;
}

static LigmaProcedure *
svg_create_procedure (LigmaPlugIn  *plug_in,
                      const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, LOAD_PROC))
    {
      procedure = ligma_load_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           svg_load, NULL, NULL);

      ligma_procedure_set_menu_label (procedure, _("SVG image"));

      ligma_procedure_set_documentation (procedure,
                                        "Loads files in the SVG file format",
                                        "Renders SVG files to raster graphics "
                                        "using librsvg.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      SVG_VERSION);

      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/svg+xml");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "svg");
      ligma_file_procedure_set_magics (LIGMA_FILE_PROCEDURE (procedure),
                                      "0,string,<?xml,0,string,<svg");

      ligma_load_procedure_set_thumbnail_loader (LIGMA_LOAD_PROCEDURE (procedure),
                                                LOAD_THUMB_PROC);

      LIGMA_PROC_ARG_DOUBLE (procedure, "resolution",
                            "Resolution",
                            "Resolution to use for rendering the SVG",
                            LIGMA_MIN_RESOLUTION, LIGMA_MAX_RESOLUTION, 90,
                            LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "width",
                         "Width",
                         "Width (in pixels) to load the SVG in. "
                         "(0 for original width, a negative width to "
                         "specify a maximum width)",
                         -LIGMA_MAX_IMAGE_SIZE, LIGMA_MAX_IMAGE_SIZE, 0,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "height",
                         "Height",
                         "Height (in pixels) to load the SVG in. "
                         "(0 for original height, a negative height to "
                         "specify a maximum height)",
                         -LIGMA_MAX_IMAGE_SIZE, LIGMA_MAX_IMAGE_SIZE, 0,
                         LIGMA_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "paths",
                         "Paths",
                         "(0) don't import paths, (1) paths individually, "
                         "(2) paths merged",
                         0, 2, 0,
                         LIGMA_PARAM_READWRITE);
    }
  else if (! strcmp (name, LOAD_THUMB_PROC))
    {
      procedure = ligma_thumbnail_procedure_new (plug_in, name,
                                                LIGMA_PDB_PROC_TYPE_PLUGIN,
                                                svg_load_thumb, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Generates a thumbnail of an SVG image",
                                        "Renders a thumbnail of an SVG file "
                                        "using librsvg.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Dom Lachowicz, Sven Neumann",
                                      "Dom Lachowicz <cinamod@hotmail.com>",
                                      SVG_VERSION);
    }

  return procedure;
}

static LigmaValueArray *
svg_load (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          GFile                *file,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaValueArray    *return_vals;
  LigmaImage         *image;
  GError            *error      = NULL;
  RsvgHandleFlags    rsvg_flags = RSVG_HANDLE_FLAGS_NONE;
  LigmaPDBStatusType  status;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case LIGMA_RUN_NONINTERACTIVE:
      load_vals.resolution = LIGMA_VALUES_GET_DOUBLE (args, 0);
      load_vals.width      = LIGMA_VALUES_GET_INT    (args, 1);
      load_vals.height     = LIGMA_VALUES_GET_INT    (args, 2);
      load_vals.import     = LIGMA_VALUES_GET_INT    (args, 3) != FALSE;
      load_vals.merge      = LIGMA_VALUES_GET_INT    (args, 3) == 2;
      break;

    case LIGMA_RUN_INTERACTIVE:
      ligma_get_data (LOAD_PROC, &load_vals);
      status = load_dialog (file, &rsvg_flags, &error);
      if (status != LIGMA_PDB_SUCCESS)
        return ligma_procedure_new_return_values (procedure, status, error);
      break;

    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_get_data (LOAD_PROC, &load_vals);
      break;
    }

  image = load_image (file, rsvg_flags, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);
  if (load_vals.import)
    {
      LigmaVectors **vectors;
      gint          num_vectors;

      ligma_vectors_import_from_file (image, file,
                                     load_vals.merge, TRUE,
                                     &num_vectors, &vectors);
      if (num_vectors > 0)
        g_free (vectors);
    }

  if (run_mode != LIGMA_RUN_NONINTERACTIVE)
    ligma_set_data (LOAD_PROC, &load_vals, sizeof (load_vals));

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);

  return return_vals;
}

static LigmaValueArray *
svg_load_thumb (LigmaProcedure        *procedure,
                GFile                *file,
                gint                  size,
                const LigmaValueArray *args,
                gpointer              run_data)
{
  LigmaValueArray *return_vals;
  gint            width  = 0;
  gint            height = 0;
  LigmaImage      *image;
  GError         *error = NULL;

  gegl_init (NULL, NULL);

  if (load_rsvg_size (file, &load_vals, RSVG_HANDLE_FLAGS_NONE, NULL))
    {
      width  = load_vals.width;
      height = load_vals.height;
    }

  load_vals.resolution = SVG_DEFAULT_RESOLUTION;
  load_vals.width      = - size;
  load_vals.height     = - size;

  image = load_image (file, RSVG_HANDLE_FLAGS_NONE, &error);

  if (! image)
    return ligma_procedure_new_return_values (procedure,
                                             LIGMA_PDB_EXECUTION_ERROR,
                                             error);

  return_vals = ligma_procedure_new_return_values (procedure,
                                                  LIGMA_PDB_SUCCESS,
                                                  NULL);

  LIGMA_VALUES_SET_IMAGE (return_vals, 1, image);
  LIGMA_VALUES_SET_INT   (return_vals, 2, width);
  LIGMA_VALUES_SET_INT   (return_vals, 3, height);

  ligma_value_array_truncate (return_vals, 4);

  return return_vals;
}

static LigmaImage *
load_image (GFile            *file,
            RsvgHandleFlags   rsvg_flags,
            GError          **load_error)
{
  LigmaImage    *image;
  LigmaLayer    *layer;
  GdkPixbuf    *pixbuf;
  gint          width;
  gint          height;
  GError       *error = NULL;

  pixbuf = load_rsvg_pixbuf (file, &load_vals, rsvg_flags, NULL, &error);

  if (! pixbuf)
    {
      /*  Do not rely on librsvg setting GError on failure!  */
      g_set_error (load_error,
                   error ? error->domain : 0, error ? error->code : 0,
                   _("Could not open '%s' for reading: %s"),
                   ligma_file_get_utf8_name (file),
                   error ? error->message : _("Unknown reason"));
      g_clear_error (&error);

      return NULL;
    }

  ligma_progress_init (_("Rendering SVG"));

  width  = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  image = ligma_image_new (width, height, LIGMA_RGB);
  ligma_image_undo_disable (image);

  ligma_image_set_file (image, file);
  ligma_image_set_resolution (image,
                             load_vals.resolution, load_vals.resolution);

  layer = ligma_layer_new_from_pixbuf (image, _("Rendered SVG"), pixbuf,
                                      100,
                                      ligma_image_get_default_new_layer_mode (image),
                                      0.0, 1.0);
  ligma_image_insert_layer (image, layer, NULL, 0);

  ligma_image_undo_enable (image);

  return image;
}

/*  This is the callback used from load_rsvg_pixbuf().  */
static void
load_set_size_callback (gint     *width,
                        gint     *height,
                        gpointer  data)
{
  SvgLoadVals *vals = data;

  if (*width < 1 || *height < 1)
    {
      *width  = SVG_DEFAULT_SIZE;
      *height = SVG_DEFAULT_SIZE;
    }

  if (!vals->width || !vals->height)
    return;

  /*  either both arguments negative or none  */
  if ((vals->width * vals->height) < 0)
    return;

  if (vals->width > 0)
    {
      *width  = vals->width;
      *height = vals->height;
    }
  else
    {
      gdouble w      = *width;
      gdouble h      = *height;
      gdouble aspect = (gdouble) vals->width / (gdouble) vals->height;

      if (aspect > (w / h))
        {
          *height = abs (vals->height);
          *width  = (gdouble) abs (vals->width) * (w / h) + 0.5;
        }
      else
        {
          *width  = abs (vals->width);
          *height = (gdouble) abs (vals->height) / (w / h) + 0.5;
        }

      vals->width  = *width;
      vals->height = *height;
    }
}

/*  This function renders a pixbuf from an SVG file according to vals.  */
static GdkPixbuf *
load_rsvg_pixbuf (GFile            *file,
                  SvgLoadVals      *vals,
                  RsvgHandleFlags   rsvg_flags,
                  gboolean         *allow_retry,
                  GError          **error)
{
  GdkPixbuf  *pixbuf  = NULL;
  RsvgHandle *handle;

  handle = rsvg_handle_new_from_gfile_sync (file, rsvg_flags, NULL, error);

  if (! handle)
    {
      /* The "huge data" error will be seen from the handle creation
       * already. No need to retry when the error occurs later.
       */
      if (allow_retry)
        *allow_retry = TRUE;
      return NULL;
    }

  rsvg_handle_set_dpi (handle, vals->resolution);
  rsvg_handle_set_size_callback (handle, load_set_size_callback, vals, NULL);

  pixbuf = rsvg_handle_get_pixbuf (handle);

  g_object_unref (handle);

  return pixbuf;
}

static GtkWidget *size_label = NULL;

/*  This function retrieves the pixel size from an SVG file.  */
static gboolean
load_rsvg_size (GFile            *file,
                SvgLoadVals      *vals,
                RsvgHandleFlags   rsvg_flags,
                GError          **error)
{
  RsvgHandle        *handle;
  RsvgDimensionData  dim;
  gboolean           has_size;

  handle = rsvg_handle_new_from_gfile_sync (file, rsvg_flags, NULL, error);

  if (! handle)
    return FALSE;

  rsvg_handle_set_dpi (handle, vals->resolution);

  rsvg_handle_get_dimensions (handle, &dim);

  if (dim.width > 0 && dim.height > 0)
    {
      vals->width  = dim.width;
      vals->height = dim.height;
      has_size = TRUE;
    }
  else
    {
      vals->width  = SVG_DEFAULT_SIZE;
      vals->height = SVG_DEFAULT_SIZE;
      has_size = FALSE;
    }

    if (size_label)
      {
        if (has_size)
          {
            gchar *text = g_strdup_printf (_("%d × %d"),
                                           vals->width, vals->height);
            gtk_label_set_text (GTK_LABEL (size_label), text);
            g_free (text);
          }
        else
          {
            gtk_label_set_text (GTK_LABEL (size_label),
                                _("SVG file does not\nspecify a size!"));
          }
      }

  g_object_unref (handle);

  if (vals->width  < 1)  vals->width  = 1;
  if (vals->height < 1)  vals->height = 1;

  return TRUE;
}


/*  User interface  */

static LigmaSizeEntry *size       = NULL;
static GtkAdjustment *xadj       = NULL;
static GtkAdjustment *yadj       = NULL;
static GtkWidget     *constrain  = NULL;
static gdouble        ratio_x    = 1.0;
static gdouble        ratio_y    = 1.0;
static gint           svg_width  = 0;
static gint           svg_height = 0;

static void  load_dialog_set_ratio (gdouble x,
                                    gdouble y);


static void
load_dialog_size_callback (GtkWidget *widget,
                           gpointer   data)
{
  if (ligma_chain_button_get_active (LIGMA_CHAIN_BUTTON (constrain)))
    {
      gdouble x = ligma_size_entry_get_refval (size, 0) / (gdouble) svg_width;
      gdouble y = ligma_size_entry_get_refval (size, 1) / (gdouble) svg_height;

      if (x != ratio_x)
        {
          load_dialog_set_ratio (x, x);
        }
      else if (y != ratio_y)
        {
          load_dialog_set_ratio (y, y);
        }
    }
}

static void
load_dialog_ratio_callback (GtkAdjustment *adj,
                            gpointer       data)
{
  gdouble x = gtk_adjustment_get_value (xadj);
  gdouble y = gtk_adjustment_get_value (yadj);

  if (ligma_chain_button_get_active (LIGMA_CHAIN_BUTTON (constrain)))
    {
      if (x != ratio_x)
        y = x;
      else
        x = y;
    }

  load_dialog_set_ratio (x, y);
}

static void
load_dialog_resolution_callback (LigmaSizeEntry *res,
                                 GFile         *file)
{
  SvgLoadVals     vals = { 0.0, 0, 0 };
  RsvgHandleFlags rsvg_flags;

  rsvg_flags = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (file), "rsvg-flags"));

  load_vals.resolution = vals.resolution = ligma_size_entry_get_refval (res, 0);

  if (! load_rsvg_size (file, &vals, rsvg_flags, NULL))
    return;

  g_signal_handlers_block_by_func (size, load_dialog_size_callback, NULL);

  ligma_size_entry_set_resolution (size, 0, load_vals.resolution, FALSE);
  ligma_size_entry_set_resolution (size, 1, load_vals.resolution, FALSE);

  g_signal_handlers_unblock_by_func (size, load_dialog_size_callback, NULL);

  if (ligma_size_entry_get_unit (size) != LIGMA_UNIT_PIXEL)
    {
      ratio_x = ligma_size_entry_get_refval (size, 0) / vals.width;
      ratio_y = ligma_size_entry_get_refval (size, 1) / vals.height;
    }

  svg_width  = vals.width;
  svg_height = vals.height;

  load_dialog_set_ratio (ratio_x, ratio_y);
}

static void
load_dialog_set_ratio (gdouble x,
                       gdouble y)
{
  ratio_x = x;
  ratio_y = y;

  g_signal_handlers_block_by_func (size, load_dialog_size_callback, NULL);

  ligma_size_entry_set_refval (size, 0, svg_width  * x);
  ligma_size_entry_set_refval (size, 1, svg_height * y);

  g_signal_handlers_unblock_by_func (size, load_dialog_size_callback, NULL);

  g_signal_handlers_block_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_block_by_func (yadj, load_dialog_ratio_callback, NULL);

  gtk_adjustment_set_value (xadj, x);
  gtk_adjustment_set_value (yadj, y);

  g_signal_handlers_unblock_by_func (xadj, load_dialog_ratio_callback, NULL);
  g_signal_handlers_unblock_by_func (yadj, load_dialog_ratio_callback, NULL);
}

static LigmaPDBStatusType
load_dialog (GFile            *file,
             RsvgHandleFlags  *rsvg_flags,
             GError          **load_error)
{
  GtkWidget     *dialog;
  GtkWidget     *frame;
  GtkWidget     *hbox;
  GtkWidget     *vbox;
  GtkWidget     *image;
  GdkPixbuf     *preview;
  GtkWidget     *grid;
  GtkWidget     *grid2;
  GtkWidget     *res;
  GtkWidget     *label;
  GtkWidget     *spinbutton;
  GtkWidget     *toggle;
  GtkWidget     *toggle2;
  GtkAdjustment *adj;
  gboolean       run;
  GError        *error = NULL;
  gchar         *text;
  gboolean       allow_retry = FALSE;
  SvgLoadVals    vals  =
  {
    SVG_DEFAULT_RESOLUTION,
    - SVG_PREVIEW_SIZE,
    - SVG_PREVIEW_SIZE
  };

  preview = load_rsvg_pixbuf (file, &vals, *rsvg_flags, &allow_retry, &error);

  ligma_ui_init (PLUG_IN_BINARY);

  if (! preview && *rsvg_flags == RSVG_HANDLE_FLAGS_NONE && allow_retry)
    {
      /* We need to ask explicitly before using the "unlimited" size
       * option (XML_PARSE_HUGE in libxml) because it is considered
       * unsafe, possibly consumming too much memory with malicious XML
       * files.
       */
      dialog = ligma_dialog_new (_("Disable safety size limits?"),
                                PLUG_IN_ROLE,
                                NULL, 0,
                                ligma_standard_help_func, LOAD_PROC,

                                _("_No"),  GTK_RESPONSE_NO,
                                _("_Yes"), GTK_RESPONSE_YES,

                                NULL);

      gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_NO);
      gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
      ligma_window_set_transient (GTK_WINDOW (dialog));

      hbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                          hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      /* Unfortunately the error returned by librsvg is unclear. While
       * libxml explicitly returns a "parser error : internal error:
       * Huge input lookup", librsvg does not seem to relay this error.
       * It sends a further parsing error, false positive provoked by
       * the huge input error.
       * If we were able to single out the huge data error, we could
       * just directly return from the plug-in as a failure run in other
       * cases. Instead of this, we need to ask each and every time, in
       * case it might be the huge data error.
       */
      label = gtk_label_new (_("A parsing error occurred.\n"
                               "Disabling safety limits may help. "
                               "Malicious SVG files may use this to consume too much memory."));
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_label_set_line_wrap_mode (GTK_LABEL (label), PANGO_WRAP_WORD);
      gtk_label_set_max_width_chars (GTK_LABEL (label), 80);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      label = gtk_label_new (NULL);
      text = g_strdup_printf ("<b>%s</b>",
                              _("For security reasons, this should only be used for trusted input!"));
      gtk_label_set_markup (GTK_LABEL (label), text);
      g_free (text);
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      label = gtk_label_new (_("Retry without limits preventing to parse huge data?"));
      gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      gtk_widget_show (dialog);

      run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_YES);
      gtk_widget_destroy (dialog);

      if (run)
        {
          *rsvg_flags = RSVG_HANDLE_FLAG_UNLIMITED;
          g_clear_error (&error);
          preview = load_rsvg_pixbuf (file, &vals, *rsvg_flags, NULL, &error);
        }
    }

  if (! preview)
    {
      /*  Do not rely on librsvg setting GError on failure!  */
      g_set_error (load_error,
                   error ? error->domain : 0, error ? error->code : 0,
                   _("Could not open '%s' for reading: %s"),
                   ligma_file_get_utf8_name (file),
                   error ? error->message : _("Unknown reason"));
      g_clear_error (&error);

      return LIGMA_PDB_EXECUTION_ERROR;
    }

  /* Scalable Vector Graphics is SVG, should perhaps not be translated */
  dialog = ligma_dialog_new (_("Render Scalable Vector Graphics"),
                            PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, LOAD_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  ligma_window_set_transient (GTK_WINDOW (dialog));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /*  The SVG preview  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  image = gtk_image_new_from_pixbuf (preview);
  gtk_container_add (GTK_CONTAINER (frame), image);
  gtk_widget_show (image);

  size_label = gtk_label_new (NULL);
  gtk_label_set_justify (GTK_LABEL (size_label), GTK_JUSTIFY_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), size_label, TRUE, TRUE, 4);
  gtk_widget_show (size_label);

  /*  query the initial size after the size label is created  */
  vals.resolution = load_vals.resolution;

  load_rsvg_size (file, &vals, *rsvg_flags, NULL);

  svg_width  = vals.width;
  svg_height = vals.height;

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (hbox), grid, TRUE, TRUE, 0);
  gtk_widget_show (grid);

  /*  Width and Height  */
  label = gtk_label_new (_("Width:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  label = gtk_label_new (_("Height:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 0, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  adj = gtk_adjustment_new (1, 1, 1, 1, 10, 0);
  spinbutton = ligma_spin_button_new (adj, 1.0, 2);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_box_pack_start (GTK_BOX (hbox), spinbutton, FALSE, FALSE, 0);
  gtk_widget_show (spinbutton);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 1, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  size = LIGMA_SIZE_ENTRY (ligma_size_entry_new (1, LIGMA_UNIT_PIXEL, "%a",
                                               TRUE, FALSE, FALSE, 10,
                                               LIGMA_SIZE_ENTRY_UPDATE_SIZE));

  ligma_size_entry_add_field (size, GTK_SPIN_BUTTON (spinbutton), NULL);

  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (size), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (size));

  ligma_size_entry_set_refval_boundaries (size, 0,
                                         LIGMA_MIN_IMAGE_SIZE,
                                         LIGMA_MAX_IMAGE_SIZE);
  ligma_size_entry_set_refval_boundaries (size, 1,
                                         LIGMA_MIN_IMAGE_SIZE,
                                         LIGMA_MAX_IMAGE_SIZE);

  ligma_size_entry_set_refval (size, 0, svg_width);
  ligma_size_entry_set_refval (size, 1, svg_height);

  ligma_size_entry_set_resolution (size, 0, load_vals.resolution, FALSE);
  ligma_size_entry_set_resolution (size, 1, load_vals.resolution, FALSE);

  g_signal_connect (size, "value-changed",
                    G_CALLBACK (load_dialog_size_callback),
                    NULL);

  /*  Scale ratio  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 2, 1, 2);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (hbox);

  grid2 = gtk_grid_new ();
  gtk_box_pack_start (GTK_BOX (hbox), grid2, FALSE, FALSE, 0);

  xadj = gtk_adjustment_new (ratio_x,
                             (gdouble) LIGMA_MIN_IMAGE_SIZE / (gdouble) svg_width,
                             (gdouble) LIGMA_MAX_IMAGE_SIZE / (gdouble) svg_width,
                             0.01, 0.1, 0);
  spinbutton = ligma_spin_button_new (xadj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_grid_attach (GTK_GRID (grid2), spinbutton, 0, 0, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (xadj, "value-changed",
                    G_CALLBACK (load_dialog_ratio_callback),
                    NULL);

  label = gtk_label_new_with_mnemonic (_("_X ratio:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  yadj = gtk_adjustment_new (ratio_y,
                             (gdouble) LIGMA_MIN_IMAGE_SIZE / (gdouble) svg_height,
                             (gdouble) LIGMA_MAX_IMAGE_SIZE / (gdouble) svg_height,
                             0.01, 0.1, 0);
  spinbutton = ligma_spin_button_new (yadj, 0.01, 4);
  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 10);
  gtk_grid_attach (GTK_GRID (grid2), spinbutton, 0, 1, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (yadj, "value-changed",
                    G_CALLBACK (load_dialog_ratio_callback),
                    NULL);

  label = gtk_label_new_with_mnemonic (_("_Y ratio:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), spinbutton);
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  /*  the constrain ratio chainbutton  */
  constrain = ligma_chain_button_new (LIGMA_CHAIN_RIGHT);
  ligma_chain_button_set_active (LIGMA_CHAIN_BUTTON (constrain), TRUE);
  gtk_grid_attach (GTK_GRID (grid2), constrain, 1, 0, 1, 2);
  gtk_widget_show (constrain);

  ligma_help_set_help_data (ligma_chain_button_get_button (LIGMA_CHAIN_BUTTON (constrain)),
                           _("Constrain aspect ratio"), NULL);

  gtk_widget_show (grid2);

  /*  Resolution   */
  label = gtk_label_new (_("Resolution:"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 4, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (label);

  res = ligma_size_entry_new (1, LIGMA_UNIT_INCH, _("pixels/%a"),
                             FALSE, FALSE, FALSE, 10,
                             LIGMA_SIZE_ENTRY_UPDATE_RESOLUTION);

  gtk_grid_attach (GTK_GRID (grid), res, 1, 4, 1, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (res);

  /* don't let the resolution become too small, librsvg tends to
     crash with very small resolutions */
  ligma_size_entry_set_refval_boundaries (LIGMA_SIZE_ENTRY (res), 0,
                                         5.0, LIGMA_MAX_RESOLUTION);
  ligma_size_entry_set_refval (LIGMA_SIZE_ENTRY (res), 0, load_vals.resolution);

  g_object_set_data (G_OBJECT (file), "rsvg-flags", GINT_TO_POINTER (rsvg_flags));
  g_signal_connect (res, "value-changed",
                    G_CALLBACK (load_dialog_resolution_callback),
                    file);

  /*  Path Import  */
  toggle = gtk_check_button_new_with_mnemonic (_("Import _paths"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), load_vals.import);
  gtk_grid_attach (GTK_GRID (grid), toggle, 0, 5, 2, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (toggle);

  ligma_help_set_help_data (toggle,
                           _("Import path elements of the SVG so they "
                             "can be used with the LIGMA path tool"),
                           NULL);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &load_vals.import);

  toggle2 = gtk_check_button_new_with_mnemonic (_("Merge imported paths"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle2), load_vals.merge);
  gtk_grid_attach (GTK_GRID (grid), toggle2, 0, 6, 2, 1);
                    // GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (toggle2);

  g_signal_connect (toggle2, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &load_vals.merge);

  g_object_bind_property (toggle,  "active",
                          toggle2, "sensitive",
                          G_BINDING_SYNC_CREATE);

  gtk_widget_show (dialog);

  run = (ligma_dialog_run (LIGMA_DIALOG (dialog)) == GTK_RESPONSE_OK);

  if (run)
    {
      load_vals.width  = ROUND (ligma_size_entry_get_refval (size, 0));
      load_vals.height = ROUND (ligma_size_entry_get_refval (size, 1));
    }

  gtk_widget_destroy (dialog);

  return run ? LIGMA_PDB_SUCCESS : LIGMA_PDB_CANCEL;
}
