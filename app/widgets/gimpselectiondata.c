/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligma-utils.h"
#include "core/ligmabrush.h"
#include "core/ligmacontainer.h"
#include "core/ligmacurve.h"
#include "core/ligmadatafactory.h"
#include "core/ligmagradient.h"
#include "core/ligmaimage.h"
#include "core/ligmaimagefile.h"
#include "core/ligmaitem.h"
#include "core/ligmapalette.h"
#include "core/ligmapattern.h"
#include "core/ligmatoolinfo.h"

#include "text/ligmafont.h"

#include "xcf/xcf.h"

#include "ligmaselectiondata.h"

#include "ligma-log.h"
#include "ligma-intl.h"


/*  local function prototypes  */

static const gchar * ligma_selection_data_get_name   (GtkSelectionData *selection,
                                                     const gchar      *strfunc);
static LigmaObject  * ligma_selection_data_get_object (GtkSelectionData *selection,
                                                     LigmaContainer    *container,
                                                     LigmaObject       *additional);
static gchar       * ligma_unescape_uri_string       (const char       *escaped,
                                                     int               len,
                                                     const char       *illegal_escaped_characters,
                                                     gboolean          ascii_must_not_be_escaped);


/*  public functions  */

void
ligma_selection_data_set_uri_list (GtkSelectionData *selection,
                                  GList            *uri_list)
{
  GList *list;
  gchar *vals = NULL;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (uri_list != NULL);

  for (list = uri_list; list; list = g_list_next (list))
    {
      if (vals)
        {
          gchar *tmp = g_strconcat (vals,
                                    list->data,
                                    list->next ? "\n" : NULL,
                                    NULL);
          g_free (vals);
          vals = tmp;
        }
      else
        {
          vals = g_strconcat (list->data,
                              list->next ? "\n" : NULL,
                              NULL);
        }
    }

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          8, (guchar *) vals, strlen (vals));

  g_free (vals);
}

GList *
ligma_selection_data_get_uri_list (GtkSelectionData *selection)
{
  GList       *crap_list = NULL;
  GList       *uri_list  = NULL;
  GList       *list;
  gint         length;
  const gchar *data;
  const gchar *buffer;

  g_return_val_if_fail (selection != NULL, NULL);

  length = gtk_selection_data_get_length (selection);

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("Received invalid file data!");
      return NULL;
    }

  data = buffer = (const gchar *) gtk_selection_data_get_data (selection);

  LIGMA_LOG (DND, "raw buffer >>%s<<", buffer);

  {
    gchar name_buffer[1024];

    while (*buffer && (buffer - data < length))
      {
        gchar *name = name_buffer;
        gint   len  = 0;

        while (len < sizeof (name_buffer) && *buffer && *buffer != '\n')
          {
            *name++ = *buffer++;
            len++;
          }
        if (len == 0)
          break;

        if (*(name - 1) == 0xd)   /* gmc uses RETURN+NEWLINE as delimiter */
          len--;

        if (len > 2)
          crap_list = g_list_prepend (crap_list, g_strndup (name_buffer, len));

        if (*buffer)
          buffer++;
      }
  }

  if (! crap_list)
    return NULL;

  /*  do various checks because file drag sources send all kinds of
   *  arbitrary crap...
   */
  for (list = crap_list; list; list = g_list_next (list))
    {
      const gchar *dnd_crap = list->data;
      gchar       *filename;
      gchar       *hostname;
      gchar       *uri   = NULL;
      GError      *error = NULL;

      LIGMA_LOG (DND, "trying to convert \"%s\" to an uri", dnd_crap);

      filename = g_filename_from_uri (dnd_crap, &hostname, NULL);

      if (filename)
        {
          /*  if we got a correctly encoded "file:" uri...
           *
           *  (for GLib < 2.4.4, this is escaped UTF-8,
           *   for GLib > 2.4.4, this is escaped local filename encoding)
           */

          uri = g_filename_to_uri (filename, hostname, NULL);

          g_free (hostname);
          g_free (filename);
        }
      else if (g_file_test (dnd_crap, G_FILE_TEST_EXISTS))
        {
          /*  ...else if we got a valid local filename...  */

          uri = g_filename_to_uri (dnd_crap, NULL, NULL);
        }
      else
        {
          /*  ...otherwise do evil things...  */

          const gchar *start = dnd_crap;

          if (g_str_has_prefix (dnd_crap, "file://"))
            {
              start += strlen ("file://");
            }
          else if (g_str_has_prefix (dnd_crap, "file:"))
            {
              start += strlen ("file:");
            }

          if (start != dnd_crap)
            {
              /*  try if we got a "file:" uri in the wrong encoding...
               *
               *  (for GLib < 2.4.4, this is escaped local filename encoding,
               *   for GLib > 2.4.4, this is escaped UTF-8)
               */
              gchar *unescaped_filename;

              if (strstr (dnd_crap, "%"))
                {
                  gchar *local_filename;

                  unescaped_filename = ligma_unescape_uri_string (start, -1,
                                                                 "/", FALSE);

                  /*  check if we got a drop from an application that
                   *  encodes file: URIs as UTF-8 (apps linked against
                   *  GLib < 2.4.4)
                   */
                  local_filename = g_filename_from_utf8 (unescaped_filename,
                                                         -1, NULL, NULL,
                                                         NULL);

                  if (local_filename)
                    {
                      g_free (unescaped_filename);
                      unescaped_filename = local_filename;
                    }
                }
              else
                {
                  unescaped_filename = g_strdup (start);
                }

              uri = g_filename_to_uri (unescaped_filename, NULL, &error);

              if (! uri)
                {
                  gchar *escaped_filename = g_strescape (unescaped_filename,
                                                         NULL);

                  g_message (_("The filename '%s' couldn't be converted to a "
                               "valid URI:\n\n%s"),
                             escaped_filename,
                             error->message ?
                             error->message : _("Invalid UTF-8"));
                  g_free (escaped_filename);
                  g_clear_error (&error);

                  g_free (unescaped_filename);
                  continue;
                }

              g_free (unescaped_filename);
            }
          else
            {
              /*  otherwise try the crap passed anyway, in case it's
               *  a "http:" or whatever uri a plug-in might handle
               */
              uri = g_strdup (dnd_crap);
            }
        }

      uri_list = g_list_prepend (uri_list, uri);
    }

  g_list_free_full (crap_list, (GDestroyNotify) g_free);

  return uri_list;
}

void
ligma_selection_data_set_color (GtkSelectionData *selection,
                               const LigmaRGB    *color)
{
  guint16  vals[4];
  guchar   r, g, b, a;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (color != NULL);

  ligma_rgba_get_uchar (color, &r, &g, &b, &a);

  vals[0] = r + (r << 8);
  vals[1] = g + (g << 8);
  vals[2] = b + (b << 8);
  vals[3] = a + (a << 8);

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          16, (const guchar *) vals, 8);
}

gboolean
ligma_selection_data_get_color (GtkSelectionData *selection,
                               LigmaRGB          *color)
{
  const guint16 *color_vals;

  g_return_val_if_fail (selection != NULL, FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  if (gtk_selection_data_get_format (selection) != 16 ||
      gtk_selection_data_get_length (selection) != 8)
    {
      g_warning ("Received invalid color data!");
      return FALSE;
    }

  color_vals = (const guint16 *) gtk_selection_data_get_data (selection);

  ligma_rgba_set_uchar (color,
                       (guchar) (color_vals[0] >> 8),
                       (guchar) (color_vals[1] >> 8),
                       (guchar) (color_vals[2] >> 8),
                       (guchar) (color_vals[3] >> 8));

  return TRUE;
}

void
ligma_selection_data_set_xcf (GtkSelectionData *selection,
                             LigmaImage        *image)
{
  GMemoryOutputStream *output;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  output = G_MEMORY_OUTPUT_STREAM (g_memory_output_stream_new_resizable ());

  xcf_save_stream (image->ligma, image, G_OUTPUT_STREAM (output), NULL,
                   NULL, NULL);

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          8,
                          g_memory_output_stream_get_data (output),
                          g_memory_output_stream_get_data_size (output));

  g_object_unref (output);
}

LigmaImage *
ligma_selection_data_get_xcf (GtkSelectionData *selection,
                             Ligma             *ligma)
{
  GInputStream *input;
  LigmaImage    *image;
  gsize         length;
  const guchar *data;
  GError       *error = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  length = gtk_selection_data_get_length (selection);

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("Received invalid data stream!");
      return NULL;
    }

  data = gtk_selection_data_get_data (selection);

  input = g_memory_input_stream_new_from_data (data, length, NULL);

  image = xcf_load_stream (ligma, input, NULL, NULL, &error);

  if (image)
    {
      /*  don't keep clipboard images in the image list  */
      ligma_container_remove (ligma->images, LIGMA_OBJECT (image));
    }
  else
    {
      g_warning ("Received invalid XCF data: %s", error->message);
      g_clear_error (&error);
    }

  g_object_unref (input);

  return image;
}

void
ligma_selection_data_set_stream (GtkSelectionData *selection,
                                const guchar     *stream,
                                gsize             stream_length)
{
  g_return_if_fail (selection != NULL);
  g_return_if_fail (stream != NULL);
  g_return_if_fail (stream_length > 0);

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          8, (guchar *) stream, stream_length);
}

const guchar *
ligma_selection_data_get_stream (GtkSelectionData *selection,
                                gsize            *stream_length)
{
  gint length;

  g_return_val_if_fail (selection != NULL, NULL);
  g_return_val_if_fail (stream_length != NULL, NULL);

  length = gtk_selection_data_get_length (selection);

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("Received invalid data stream!");
      return NULL;
    }

  *stream_length = length;

  return (const guchar *) gtk_selection_data_get_data (selection);
}

void
ligma_selection_data_set_curve (GtkSelectionData *selection,
                               LigmaCurve        *curve)
{
  gchar *str;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (LIGMA_IS_CURVE (curve));

  str = ligma_config_serialize_to_string (LIGMA_CONFIG (curve), NULL);

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          8, (guchar *) str, strlen (str));

  g_free (str);
}

LigmaCurve *
ligma_selection_data_get_curve (GtkSelectionData *selection)
{
  LigmaCurve *curve;
  gint       length;
  GError    *error = NULL;

  g_return_val_if_fail (selection != NULL, NULL);

  length = gtk_selection_data_get_length (selection);

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("Received invalid curve data!");
      return NULL;
    }

  curve = LIGMA_CURVE (ligma_curve_new ("pasted curve"));

  if (! ligma_config_deserialize_string (LIGMA_CONFIG (curve),
                                        (const gchar *)
                                        gtk_selection_data_get_data (selection),
                                        length,
                                        NULL,
                                        &error))
    {
      g_warning ("Received invalid curve data: %s", error->message);
      g_clear_error (&error);
      g_object_unref (curve);
      return NULL;
    }

  return curve;
}

void
ligma_selection_data_set_image (GtkSelectionData *selection,
                               LigmaImage        *image)
{
  gchar *str;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  str = g_strdup_printf ("%d:%d", ligma_get_pid (), ligma_image_get_id (image));

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          8, (guchar *) str, strlen (str));

  g_free (str);
}

LigmaImage *
ligma_selection_data_get_image (GtkSelectionData *selection,
                               Ligma             *ligma)
{
  const gchar *str;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  str = ligma_selection_data_get_name (selection, G_STRFUNC);

  if (str)
    {
      gint pid;
      gint ID;

      if (sscanf (str, "%i:%i", &pid, &ID) == 2 &&
          pid == ligma_get_pid ())
        {
          return ligma_image_get_by_id (ligma, ID);
        }
    }

  return NULL;
}

void
ligma_selection_data_set_component (GtkSelectionData *selection,
                                   LigmaImage        *image,
                                   LigmaChannelType   channel)
{
  gchar *str;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  str = g_strdup_printf ("%d:%d:%d", ligma_get_pid (), ligma_image_get_id (image),
                         (gint) channel);

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          8, (guchar *) str, strlen (str));

  g_free (str);
}

LigmaImage *
ligma_selection_data_get_component (GtkSelectionData *selection,
                                   Ligma             *ligma,
                                   LigmaChannelType  *channel)
{
  const gchar *str;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  if (channel)
    *channel = 0;

  str = ligma_selection_data_get_name (selection, G_STRFUNC);

  if (str)
    {
      gint pid;
      gint ID;
      gint ch;

      if (sscanf (str, "%i:%i:%i", &pid, &ID, &ch) == 3 &&
          pid == ligma_get_pid ())
        {
          LigmaImage *image = ligma_image_get_by_id (ligma, ID);

          if (image && channel)
            *channel = ch;

          return image;
        }
    }

  return NULL;
}

void
ligma_selection_data_set_item (GtkSelectionData *selection,
                              LigmaItem         *item)
{
  gchar *str;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (LIGMA_IS_ITEM (item));

  str = g_strdup_printf ("%d:%d", ligma_get_pid (), ligma_item_get_id (item));

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          8, (guchar *) str, strlen (str));

  g_free (str);
}

LigmaItem *
ligma_selection_data_get_item (GtkSelectionData *selection,
                              Ligma             *ligma)
{
  const gchar *str;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  str = ligma_selection_data_get_name (selection, G_STRFUNC);

  if (str)
    {
      gint pid;
      gint ID;

      if (sscanf (str, "%i:%i", &pid, &ID) == 2 &&
          pid == ligma_get_pid ())
        {
          return ligma_item_get_by_id (ligma, ID);
        }
    }

  return NULL;
}

void
ligma_selection_data_set_item_list (GtkSelectionData *selection,
                                   GList            *items)
{
  GString *str;
  GList   *iter;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (items);

  for (iter = items; iter; iter = iter->next)
    g_return_if_fail (LIGMA_IS_ITEM (iter->data));

  str = g_string_new (NULL);
  g_string_printf (str, "%d", ligma_get_pid ());
  for (iter = items; iter; iter = iter->next)
    g_string_append_printf (str, ":%d", ligma_item_get_id (iter->data));

  gtk_selection_data_set (selection,
                          gtk_selection_data_get_target (selection),
                          8, (guchar *) str->str, str->len);

  g_string_free (str, TRUE);
}

GList *
ligma_selection_data_get_item_list (GtkSelectionData *selection,
                                   Ligma             *ligma)
{
  const gchar  *str;
  GList        *items = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  str = ligma_selection_data_get_name (selection, G_STRFUNC);

  if (str)
    {
      gchar **tokens;
      gint64  pid;

      tokens = g_strsplit (str, ":", -1);
      g_return_val_if_fail (tokens[0] != NULL && tokens[1] != NULL, NULL);

      pid = g_ascii_strtoll (tokens[0], NULL, 10);
      if (pid == ligma_get_pid ())
        {
          gint i = 1;

          while (tokens[i])
            {
              gint64 id = g_ascii_strtoll (tokens[i], NULL, 10);

              items = g_list_prepend (items, ligma_item_get_by_id (ligma, id));
              i++;
            }
          items = g_list_reverse (items);
        }

      g_strfreev (tokens);
    }

  return items;
}

void
ligma_selection_data_set_object (GtkSelectionData *selection,
                                LigmaObject       *object)
{
  const gchar *name;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (LIGMA_IS_OBJECT (object));

  name = ligma_object_get_name (object);

  if (name)
    {
      gchar *str;

      str = g_strdup_printf ("%d:%p:%s", ligma_get_pid (), object, name);

      gtk_selection_data_set (selection,
                              gtk_selection_data_get_target (selection),
                              8, (guchar *) str, strlen (str));

      g_free (str);
    }
}

LigmaBrush *
ligma_selection_data_get_brush (GtkSelectionData *selection,
                               Ligma             *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  return (LigmaBrush *)
    ligma_selection_data_get_object (selection,
                                    ligma_data_factory_get_container (ligma->brush_factory),
                                    LIGMA_OBJECT (ligma_brush_get_standard (ligma_get_user_context (ligma))));
}

LigmaPattern *
ligma_selection_data_get_pattern (GtkSelectionData *selection,
                                 Ligma             *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  return (LigmaPattern *)
    ligma_selection_data_get_object (selection,
                                    ligma_data_factory_get_container (ligma->pattern_factory),
                                    LIGMA_OBJECT (ligma_pattern_get_standard (ligma_get_user_context (ligma))));
}

LigmaGradient *
ligma_selection_data_get_gradient (GtkSelectionData *selection,
                                  Ligma             *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  return (LigmaGradient *)
    ligma_selection_data_get_object (selection,
                                    ligma_data_factory_get_container (ligma->gradient_factory),
                                    LIGMA_OBJECT (ligma_gradient_get_standard (ligma_get_user_context (ligma))));
}

LigmaPalette *
ligma_selection_data_get_palette (GtkSelectionData *selection,
                                 Ligma             *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  return (LigmaPalette *)
    ligma_selection_data_get_object (selection,
                                    ligma_data_factory_get_container (ligma->palette_factory),
                                    LIGMA_OBJECT (ligma_palette_get_standard (ligma_get_user_context (ligma))));
}

LigmaFont *
ligma_selection_data_get_font (GtkSelectionData *selection,
                              Ligma             *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  return (LigmaFont *)
    ligma_selection_data_get_object (selection,
                                    ligma_data_factory_get_container (ligma->font_factory),
                                    LIGMA_OBJECT (ligma_font_get_standard ()));
}

LigmaBuffer *
ligma_selection_data_get_buffer (GtkSelectionData *selection,
                                Ligma             *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  return (LigmaBuffer *)
    ligma_selection_data_get_object (selection,
                                    ligma->named_buffers,
                                    LIGMA_OBJECT (ligma_get_clipboard_buffer (ligma)));
}

LigmaImagefile *
ligma_selection_data_get_imagefile (GtkSelectionData *selection,
                                   Ligma             *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  return (LigmaImagefile *) ligma_selection_data_get_object (selection,
                                                           ligma->documents,
                                                           NULL);
}

LigmaTemplate *
ligma_selection_data_get_template (GtkSelectionData *selection,
                                  Ligma             *ligma)
{
  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  return (LigmaTemplate *) ligma_selection_data_get_object (selection,
                                                          ligma->templates,
                                                          NULL);
}

LigmaToolItem *
ligma_selection_data_get_tool_item (GtkSelectionData *selection,
                                   Ligma             *ligma)
{
  LigmaToolItem *tool_item;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (selection != NULL, NULL);

  tool_item = (LigmaToolItem *)
    ligma_selection_data_get_object (selection,
                                    ligma->tool_info_list,
                                    LIGMA_OBJECT (ligma_tool_info_get_standard (ligma)));

  if (! tool_item)
    {
      tool_item = (LigmaToolItem *)
        ligma_selection_data_get_object (selection,
                                        ligma->tool_item_list,
                                        NULL);
    }

  return tool_item;
}


/*  private functions  */

static const gchar *
ligma_selection_data_get_name (GtkSelectionData *selection,
                              const gchar      *strfunc)
{
  const gchar *name;

  if (gtk_selection_data_get_format (selection) != 8 ||
      gtk_selection_data_get_length (selection) < 1)
    {
      g_warning ("%s: received invalid selection data", strfunc);
      return NULL;
    }

  name = (const gchar *) gtk_selection_data_get_data (selection);

  if (! g_utf8_validate (name, -1, NULL))
    {
      g_warning ("%s: received invalid selection data "
                 "(doesn't validate as UTF-8)", strfunc);
      return NULL;
    }

  LIGMA_LOG (DND, "name = '%s'", name);

  return name;
}

static LigmaObject *
ligma_selection_data_get_object (GtkSelectionData *selection,
                                LigmaContainer    *container,
                                LigmaObject       *additional)
{
  const gchar *str;

  str = ligma_selection_data_get_name (selection, G_STRFUNC);

  if (str)
    {
      gint     pid;
      gpointer object_addr;
      gint     name_offset = 0;

      if (sscanf (str, "%i:%p:%n", &pid, &object_addr, &name_offset) >= 2 &&
          pid == ligma_get_pid () && name_offset > 0)
        {
          const gchar *name = str + name_offset;

          LIGMA_LOG (DND, "pid = %d, addr = %p, name = '%s'",
                    pid, object_addr, name);

          if (additional &&
              strcmp (name, ligma_object_get_name (additional)) == 0 &&
              object_addr == (gpointer) additional)
            {
              return additional;
            }
          else
            {
              LigmaObject *object;

              object = ligma_container_get_child_by_name (container, name);

              if (object_addr == (gpointer) object)
                return object;
            }
        }
    }

  return NULL;
}

/*  the next two functions are straight cut'n'paste from glib/glib/gconvert.c,
 *  except that ligma_unescape_uri_string() does not try to UTF-8 validate
 *  the unescaped result.
 */
static int
unescape_character (const char *scanner)
{
  int first_digit;
  int second_digit;

  first_digit = g_ascii_xdigit_value (scanner[0]);
  if (first_digit < 0)
    return -1;

  second_digit = g_ascii_xdigit_value (scanner[1]);
  if (second_digit < 0)
    return -1;

  return (first_digit << 4) | second_digit;
}

static gchar *
ligma_unescape_uri_string (const char *escaped,
                          int         len,
                          const char *illegal_escaped_characters,
                          gboolean    ascii_must_not_be_escaped)
{
  const gchar *in, *in_end;
  gchar *out, *result;
  int c;

  if (escaped == NULL)
    return NULL;

  if (len < 0)
    len = strlen (escaped);

  result = g_malloc (len + 1);

  out = result;
  for (in = escaped, in_end = escaped + len; in < in_end; in++)
    {
      c = *in;

      if (c == '%')
        {
          /* catch partial escape sequences past the end of the substring */
          if (in + 3 > in_end)
            break;

          c = unescape_character (in + 1);

          /* catch bad escape sequences and NUL characters */
          if (c <= 0)
            break;

          /* catch escaped ASCII */
          if (ascii_must_not_be_escaped && c <= 0x7F)
            break;

          /* catch other illegal escaped characters */
          if (strchr (illegal_escaped_characters, c) != NULL)
            break;

          in += 2;
        }

      *out++ = c;
    }

  ligma_assert (out - result <= len);
  *out = '\0';

  if (in != in_end)
    {
      g_free (result);
      return NULL;
    }

  return result;
}
