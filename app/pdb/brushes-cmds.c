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
#include "core/ligmabrush.h"
#include "core/ligmacontainer-filter.h"
#include "core/ligmacontext.h"
#include "core/ligmadatafactory.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmatempbuf.h"

#include "ligmapdb.h"
#include "ligmapdb-utils.h"
#include "ligmaprocedure.h"
#include "internal-procs.h"


static LigmaValueArray *
brushes_refresh_invoker (LigmaProcedure         *procedure,
                         Ligma                  *ligma,
                         LigmaContext           *context,
                         LigmaProgress          *progress,
                         const LigmaValueArray  *args,
                         GError               **error)
{
  ligma_data_factory_data_refresh (ligma->brush_factory, context);

  return ligma_procedure_get_return_values (procedure, TRUE, NULL);
}

static LigmaValueArray *
brushes_get_list_invoker (LigmaProcedure         *procedure,
                          Ligma                  *ligma,
                          LigmaContext           *context,
                          LigmaProgress          *progress,
                          const LigmaValueArray  *args,
                          GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  const gchar *filter;
  gchar **brush_list = NULL;

  filter = g_value_get_string (ligma_value_array_index (args, 0));

  if (success)
    {
      brush_list = ligma_container_get_filtered_name_array (ligma_data_factory_get_container (ligma->brush_factory),
                                                           filter);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_take_boxed (ligma_value_array_index (return_vals, 1), brush_list);

  return return_vals;
}

void
register_brushes_procs (LigmaPDB *pdb)
{
  LigmaProcedure *procedure;

  /*
   * ligma-brushes-refresh
   */
  procedure = ligma_procedure_new (brushes_refresh_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-brushes-refresh");
  ligma_procedure_set_static_help (procedure,
                                  "Refresh current brushes. This function always succeeds.",
                                  "This procedure retrieves all brushes currently in the user's brush path and updates the brush dialogs accordingly.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Seth Burgess",
                                         "Seth Burgess",
                                         "1997");
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-brushes-get-list
   */
  procedure = ligma_procedure_new (brushes_get_list_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-brushes-get-list");
  ligma_procedure_set_static_help (procedure,
                                  "Retrieve a complete listing of the available brushes.",
                                  "This procedure returns a complete listing of available LIGMA brushes. Each name returned can be used as input to the 'ligma-context-set-brush' procedure.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1995-1996");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("filter",
                                                       "filter",
                                                       "An optional regular expression used to filter the list",
                                                       FALSE, TRUE, FALSE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_boxed ("brush-list",
                                                       "brush list",
                                                       "The list of brush names",
                                                       G_TYPE_STRV,
                                                       LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
