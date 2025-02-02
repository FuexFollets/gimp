/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

/* NOTE: This file is auto-generated by pdbgen.pl. */

#include "config.h"

#include "stamp-pdbgen.h"

#include <string.h>

#include <gegl.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"

#include "pdb-types.h"

#include "core/ligma.h"
#include "core/ligmabuffer.h"
#include "core/ligmacontainer-filter.h"
#include "core/ligmacontainer.h"
#include "core/ligmaparamspecs.h"
#include "gegl/ligma-babl-compat.h"

#include "ligmapdb.h"
#include "ligmapdb-utils.h"
#include "ligmaprocedure.h"
#include "internal-procs.h"


static LigmaValueArray *
buffers_get_list_invoker (LigmaProcedure         *procedure,
                          Ligma                  *ligma,
                          LigmaContext           *context,
                          LigmaProgress          *progress,
                          const LigmaValueArray  *args,
                          GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  const gchar *filter;
  gchar **buffer_list = NULL;

  filter = g_value_get_string (ligma_value_array_index (args, 0));

  if (success)
    {
      buffer_list = ligma_container_get_filtered_name_array (ligma->named_buffers,
                                                            filter);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_take_boxed (ligma_value_array_index (return_vals, 1), buffer_list);

  return return_vals;
}

static LigmaValueArray *
buffer_rename_invoker (LigmaProcedure         *procedure,
                       Ligma                  *ligma,
                       LigmaContext           *context,
                       LigmaProgress          *progress,
                       const LigmaValueArray  *args,
                       GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  const gchar *buffer_name;
  const gchar *new_name;
  gchar *real_name = NULL;

  buffer_name = g_value_get_string (ligma_value_array_index (args, 0));
  new_name = g_value_get_string (ligma_value_array_index (args, 1));

  if (success)
    {
      LigmaBuffer *buffer = ligma_pdb_get_buffer (ligma, buffer_name, error);

      if (buffer)
        {
          ligma_object_set_name (LIGMA_OBJECT (buffer), new_name);
          real_name = g_strdup (ligma_object_get_name (buffer));
        }
      else
        success = FALSE;
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_take_string (ligma_value_array_index (return_vals, 1), real_name);

  return return_vals;
}

static LigmaValueArray *
buffer_delete_invoker (LigmaProcedure         *procedure,
                       Ligma                  *ligma,
                       LigmaContext           *context,
                       LigmaProgress          *progress,
                       const LigmaValueArray  *args,
                       GError               **error)
{
  gboolean success = TRUE;
  const gchar *buffer_name;

  buffer_name = g_value_get_string (ligma_value_array_index (args, 0));

  if (success)
    {
      LigmaBuffer *buffer = ligma_pdb_get_buffer (ligma, buffer_name, error);

      if (buffer)
        success = ligma_container_remove (ligma->named_buffers, LIGMA_OBJECT (buffer));
      else
        success = FALSE;
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static LigmaValueArray *
buffer_get_width_invoker (LigmaProcedure         *procedure,
                          Ligma                  *ligma,
                          LigmaContext           *context,
                          LigmaProgress          *progress,
                          const LigmaValueArray  *args,
                          GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  const gchar *buffer_name;
  gint width = 0;

  buffer_name = g_value_get_string (ligma_value_array_index (args, 0));

  if (success)
    {
      LigmaBuffer *buffer = ligma_pdb_get_buffer (ligma, buffer_name, error);

      if (buffer)
        width = ligma_buffer_get_width (buffer);
      else
        success = FALSE;
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_int (ligma_value_array_index (return_vals, 1), width);

  return return_vals;
}

static LigmaValueArray *
buffer_get_height_invoker (LigmaProcedure         *procedure,
                           Ligma                  *ligma,
                           LigmaContext           *context,
                           LigmaProgress          *progress,
                           const LigmaValueArray  *args,
                           GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  const gchar *buffer_name;
  gint height = 0;

  buffer_name = g_value_get_string (ligma_value_array_index (args, 0));

  if (success)
    {
      LigmaBuffer *buffer = ligma_pdb_get_buffer (ligma, buffer_name, error);

      if (buffer)
        height = ligma_buffer_get_height (buffer);
      else
        success = FALSE;
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_int (ligma_value_array_index (return_vals, 1), height);

  return return_vals;
}

static LigmaValueArray *
buffer_get_bytes_invoker (LigmaProcedure         *procedure,
                          Ligma                  *ligma,
                          LigmaContext           *context,
                          LigmaProgress          *progress,
                          const LigmaValueArray  *args,
                          GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  const gchar *buffer_name;
  gint bytes = 0;

  buffer_name = g_value_get_string (ligma_value_array_index (args, 0));

  if (success)
    {
      LigmaBuffer *buffer = ligma_pdb_get_buffer (ligma, buffer_name, error);

      if (buffer)
        {
          const Babl *format = ligma_buffer_get_format (buffer);

          bytes = babl_format_get_bytes_per_pixel (format);
        }
      else
        success = FALSE;
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_int (ligma_value_array_index (return_vals, 1), bytes);

  return return_vals;
}

static LigmaValueArray *
buffer_get_image_type_invoker (LigmaProcedure         *procedure,
                               Ligma                  *ligma,
                               LigmaContext           *context,
                               LigmaProgress          *progress,
                               const LigmaValueArray  *args,
                               GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  const gchar *buffer_name;
  gint image_type = 0;

  buffer_name = g_value_get_string (ligma_value_array_index (args, 0));

  if (success)
    {
      LigmaBuffer *buffer = ligma_pdb_get_buffer (ligma, buffer_name, error);

      if (buffer)
        image_type = ligma_babl_format_get_image_type (ligma_buffer_get_format (buffer));
      else
        success = FALSE;
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_enum (ligma_value_array_index (return_vals, 1), image_type);

  return return_vals;
}

void
register_buffer_procs (LigmaPDB *pdb)
{
  LigmaProcedure *procedure;

  /*
   * ligma-buffers-get-list
   */
  procedure = ligma_procedure_new (buffers_get_list_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-buffers-get-list");
  ligma_procedure_set_static_help (procedure,
                                  "Retrieve a complete listing of the available buffers.",
                                  "This procedure returns a complete listing of available named buffers.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "2005");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("filter",
                                                       "filter",
                                                       "An optional regular expression used to filter the list",
                                                       FALSE, TRUE, FALSE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_boxed ("buffer-list",
                                                       "buffer list",
                                                       "The list of buffer names",
                                                       G_TYPE_STRV,
                                                       LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-buffer-rename
   */
  procedure = ligma_procedure_new (buffer_rename_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-buffer-rename");
  ligma_procedure_set_static_help (procedure,
                                  "Renames a named buffer.",
                                  "This procedure renames a named buffer.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "2005");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("new-name",
                                                       "new name",
                                                       "The buffer's new name",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_string ("real-name",
                                                           "real name",
                                                           "The real name given to the buffer",
                                                           FALSE, FALSE, FALSE,
                                                           NULL,
                                                           LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-buffer-delete
   */
  procedure = ligma_procedure_new (buffer_delete_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-buffer-delete");
  ligma_procedure_set_static_help (procedure,
                                  "Deletes a named buffer.",
                                  "This procedure deletes a named buffer.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "David Gowers <neota@softhome.net>",
                                         "David Gowers <neota@softhome.net>",
                                         "2005");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-buffer-get-width
   */
  procedure = ligma_procedure_new (buffer_get_width_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-buffer-get-width");
  ligma_procedure_set_static_help (procedure,
                                  "Retrieves the specified buffer's width.",
                                  "This procedure retrieves the specified named buffer's width.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "2005");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_int ("width",
                                                     "width",
                                                     "The buffer width",
                                                     G_MININT32, G_MAXINT32, 0,
                                                     LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-buffer-get-height
   */
  procedure = ligma_procedure_new (buffer_get_height_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-buffer-get-height");
  ligma_procedure_set_static_help (procedure,
                                  "Retrieves the specified buffer's height.",
                                  "This procedure retrieves the specified named buffer's height.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "2005");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_int ("height",
                                                     "height",
                                                     "The buffer height",
                                                     G_MININT32, G_MAXINT32, 0,
                                                     LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-buffer-get-bytes
   */
  procedure = ligma_procedure_new (buffer_get_bytes_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-buffer-get-bytes");
  ligma_procedure_set_static_help (procedure,
                                  "Retrieves the specified buffer's bytes.",
                                  "This procedure retrieves the specified named buffer's bytes.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "2005");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_int ("bytes",
                                                     "bytes",
                                                     "The buffer bpp",
                                                     G_MININT32, G_MAXINT32, 0,
                                                     LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-buffer-get-image-type
   */
  procedure = ligma_procedure_new (buffer_get_image_type_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-buffer-get-image-type");
  ligma_procedure_set_static_help (procedure,
                                  "Retrieves the specified buffer's image type.",
                                  "This procedure retrieves the specified named buffer's image type.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "2005");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("buffer-name",
                                                       "buffer name",
                                                       "The buffer name",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_enum ("image-type",
                                                      "image type",
                                                      "The buffer image type",
                                                      LIGMA_TYPE_IMAGE_BASE_TYPE,
                                                      LIGMA_RGB,
                                                      LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
