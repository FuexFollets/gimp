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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define SAVE_PROC      "file-header-save"
#define PLUG_IN_BINARY "file-header"
#define PLUG_IN_ROLE   "ligma-file-header"


typedef struct _Header      Header;
typedef struct _HeaderClass HeaderClass;

struct _Header
{
  LigmaPlugIn      parent_instance;
};

struct _HeaderClass
{
  LigmaPlugInClass parent_class;
};


#define HEADER_TYPE  (header_get_type ())
#define HEADER (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HEADER_TYPE, Header))

GType                   header_get_type         (void) G_GNUC_CONST;

static GList          * header_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * header_create_procedure (LigmaPlugIn           *plug_in,
                                                 const gchar          *name);

static LigmaValueArray * header_save             (LigmaProcedure        *procedure,
                                                 LigmaRunMode           run_mode,
                                                 LigmaImage            *image,
                                                 gint                  n_drawables,
                                                 LigmaDrawable        **drawables,
                                                 GFile                *file,
                                                 const LigmaValueArray *args,
                                                 gpointer              run_data);

static gboolean         save_image              (GFile                *file,
                                                 LigmaImage            *image,
                                                 LigmaDrawable         *drawable,
                                                 GError              **error);

static gboolean         print                   (GOutputStream        *output,
                                                 GError              **error,
                                                 const gchar          *format,
                                                 ...) G_GNUC_PRINTF (3, 4);


G_DEFINE_TYPE (Header, header, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (HEADER_TYPE)
DEFINE_STD_SET_I18N


static void
header_class_init (HeaderClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = header_query_procedures;
  plug_in_class->create_procedure = header_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
header_init (Header *header)
{
}

static GList *
header_query_procedures (LigmaPlugIn *plug_in)
{
  return  g_list_append (NULL, g_strdup (SAVE_PROC));
}

static LigmaProcedure *
header_create_procedure (LigmaPlugIn  *plug_in,
                         const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, SAVE_PROC))
    {
      procedure = ligma_save_procedure_new (plug_in, name,
                                           LIGMA_PDB_PROC_TYPE_PLUGIN,
                                           header_save, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "INDEXED, RGB");

      ligma_procedure_set_menu_label (procedure, _("C source code header"));

      ligma_procedure_set_documentation (procedure,
                                        "saves files as C unsigned character "
                                        "array",
                                        "FIXME: write help",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Spencer Kimball & Peter Mattis",
                                      "Spencer Kimball & Peter Mattis",
                                      "1997");

      ligma_file_procedure_set_handles_remote (LIGMA_FILE_PROCEDURE (procedure),
                                              TRUE);
      ligma_file_procedure_set_mime_types (LIGMA_FILE_PROCEDURE (procedure),
                                          "image/x-chdr");
      ligma_file_procedure_set_extensions (LIGMA_FILE_PROCEDURE (procedure),
                                          "h");
    }

  return procedure;
}

static LigmaValueArray *
header_save (LigmaProcedure        *procedure,
             LigmaRunMode           run_mode,
             LigmaImage            *image,
             gint                  n_drawables,
             LigmaDrawable        **drawables,
             GFile                *file,
             const LigmaValueArray *args,
             gpointer              run_data)
{
  LigmaPDBStatusType  status = LIGMA_PDB_SUCCESS;
  LigmaExportReturn   export = LIGMA_EXPORT_CANCEL;
  GError            *error  = NULL;

  gegl_init (NULL, NULL);

  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      ligma_ui_init (PLUG_IN_BINARY);

      export = ligma_export_image (&image, &n_drawables, &drawables, "Header",
                                  LIGMA_EXPORT_CAN_HANDLE_RGB |
                                  LIGMA_EXPORT_CAN_HANDLE_INDEXED);

      if (export == LIGMA_EXPORT_CANCEL)
        return ligma_procedure_new_return_values (procedure,
                                                 LIGMA_PDB_CANCEL,
                                                 NULL);
      break;

    default:
      break;
    }

  if (n_drawables != 1)
    {
      g_set_error (&error, G_FILE_ERROR, 0,
                   _("Header plug-in does not support multiple layers."));

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }

  if (! save_image (file, image, drawables[0],
                    &error))
    {
      status = LIGMA_PDB_EXECUTION_ERROR;
    }

  if (export == LIGMA_EXPORT_EXPORT)
    {
      ligma_image_delete (image);
      g_free (drawables);
    }

  return ligma_procedure_new_return_values (procedure, status, error);
}

static gboolean
save_image (GFile         *file,
            LigmaImage     *image,
            LigmaDrawable  *drawable,
            GError       **error)
{
  GeglBuffer    *buffer;
  const Babl    *format;
  LigmaImageType  drawable_type;
  GOutputStream *output;
  gint           x, y, b, c;
  const gchar   *backslash = "\\\\";
  const gchar   *quote     = "\\\"";
  const gchar   *newline   = "\"\n\t\"";
  gchar          buf[4];
  guchar        *d         = NULL;
  guchar        *data      = NULL;
  guchar        *cmap;
  GCancellable  *cancellable;
  gint           colors;
  gint           width;
  gint           height;

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, error));
  if (output)
    {
      GOutputStream *buffered;

      buffered = g_buffered_output_stream_new (output);
      g_object_unref (output);

      output = buffered;
    }
  else
    {
      return FALSE;
    }

  buffer = ligma_drawable_get_buffer (drawable);

  width  = gegl_buffer_get_width  (buffer);
  height = gegl_buffer_get_height (buffer);

  drawable_type = ligma_drawable_type (drawable);

  if (! print (output, error,
               "/*  LIGMA header image file format (%s): %s  */\n\n",
               LIGMA_RGB_IMAGE == drawable_type ? "RGB" : "INDEXED",
               ligma_file_get_utf8_name (file)) ||
      ! print (output, error,
               "static unsigned int width = %d;\n", width) ||
      ! print (output, error,
               "static unsigned int height = %d;\n\n", height) ||
      ! print (output, error,
               "/*  Call this macro repeatedly.  After each use, the pixel data can be extracted  */\n\n"))
    {
      goto fail;
    }

  switch (drawable_type)
    {
    case LIGMA_RGB_IMAGE:
      if (! print (output, error,
                   "#define HEADER_PIXEL(data,pixel) {\\\n"
                   "pixel[0] = (((data[0] - 33) << 2) | ((data[1] - 33) >> 4)); \\\n"
                   "pixel[1] = ((((data[1] - 33) & 0xF) << 4) | ((data[2] - 33) >> 2)); \\\n"
                   "pixel[2] = ((((data[2] - 33) & 0x3) << 6) | ((data[3] - 33))); \\\n"
                   "data += 4; \\\n}\n") ||
          ! print (output, error,
                   "static char *header_data =\n\t\""))
        {
          goto fail;
        }

      format = babl_format ("R'G'B' u8");

      data = g_new (guchar, width * babl_format_get_bytes_per_pixel (format));

      c = 0;
      for (y = 0; y < height; y++)
        {
          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, y, width, 1), 1.0,
                           format, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          for (x = 0; x < width; x++)
            {
              d = data + x * babl_format_get_bytes_per_pixel (format);

              buf[0] = ((d[0] >> 2) & 0x3F) + 33;
              buf[1] = ((((d[0] & 0x3) << 4) | (d[1] >> 4)) & 0x3F) + 33;
              buf[2] = ((((d[1] & 0xF) << 2) | (d[2] >> 6)) & 0x3F) + 33;
              buf[3] = (d[2] & 0x3F) + 33;

              for (b = 0; b < 4; b++)
                {
                  if (buf[b] == '"')
                    {
                      if (! print (output, error, "%s", quote))
                        goto fail;
                    }
                  else if (buf[b] == '\\')
                    {
                      if (! print (output, error, "%s", backslash))
                        goto fail;
                    }
                  else
                    {
                      if (! print (output, error, "%c", buf[b]))
                        goto fail;
                    }
                }

              c++;
              if (c >= 16)
                {
                  if (! print (output, error, "%s", newline))
                    goto fail;

                  c = 0;
                }
            }
        }

      if (! print (output, error, "\";\n"))
        goto fail;
      break;

    case LIGMA_INDEXED_IMAGE:
      if (! print (output, error,
                   "#define HEADER_PIXEL(data,pixel) {\\\n"
                   "pixel[0] = header_data_cmap[(unsigned char)data[0]][0]; \\\n"
                   "pixel[1] = header_data_cmap[(unsigned char)data[0]][1]; \\\n"
                   "pixel[2] = header_data_cmap[(unsigned char)data[0]][2]; \\\n"
                   "data ++; }\n\n"))
        {
          goto fail;
        }

      /* save colormap */
      cmap = ligma_image_get_colormap (image, &colors);

      if (! print (output, error,
                   "static unsigned char header_data_cmap[256][3] = {") ||
          ! print (output, error,
                   "\n\t{%3d,%3d,%3d}",
                   (gint) cmap[0], (gint) cmap[1], (gint) cmap[2]))
        {
          goto fail;
        }

      for (c = 1; c < colors; c++)
        {
          if (! print (output, error,
                       ",\n\t{%3d,%3d,%3d}",
                       (gint) cmap[3 * c],
                       (gint) cmap[3 * c + 1],
                       (gint) cmap[3 * c + 2]))
            {
              goto fail;
            }
        }

      /* fill the rest */
      for ( ; c < 256; c++)
        {
          if (! print (output, error, ",\n\t{255,255,255}"))
            goto fail;
        }

      /* close bracket */
      if (! print (output, error, "\n\t};\n"))
        goto fail;

      g_free (cmap);

      /* save image */
      if (! print (output, error, "static unsigned char header_data[] = {\n\t"))
        goto fail;

      data = g_new (guchar, width * 1);

      c = 0;
      for (y = 0; y < height; y++)
        {
          gegl_buffer_get (buffer, GEGL_RECTANGLE (0, y, width, 1), 1.0,
                           NULL, data,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          for (x = 0; x < width  -1; x++)
            {
              d = data + x * 1;

              if (! print (output, error, "%d,", (gint) d[0]))
                goto fail;

              c++;
              if (c >= 16)
                {
                  if (! print (output, error, "\n\t"))
                    goto fail;

                  c = 0;
                }
            }

          if (y != height - 1)
            {
              if (! print (output, error, "%d,\n\t", (gint) d[1]))
                goto fail;
            }
          else
            {
              if (! print (output, error, "%d\n\t", (gint) d[1]))
                goto fail;
            }

          c = 0; /* reset line counter */
        }

      if (! print (output, error, "};\n"))
        goto fail;
      break;

    default:
      g_warning ("unhandled drawable type (%d)", drawable_type);
      goto fail;
    }

  if (! g_output_stream_close (output, NULL, error))
    goto fail;

  g_free (data);
  g_object_unref (output);
  g_object_unref (buffer);

  return TRUE;

 fail:

  cancellable = g_cancellable_new ();
  g_cancellable_cancel (cancellable);
  g_output_stream_close (output, cancellable, NULL);

  g_free (data);
  g_object_unref (output);
  g_object_unref (buffer);
  g_object_unref (cancellable);

  return FALSE;
}

static gboolean
print (GOutputStream  *output,
       GError        **error,
       const gchar    *format,
       ...)
{
  va_list  args;
  gboolean success;

  va_start (args, format);
  success = g_output_stream_vprintf (output, NULL, NULL,
                                     error, format, args);
  va_end (args);

  return success;
}
