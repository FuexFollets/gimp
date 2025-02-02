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

#include <gegl.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"

#include "pdb-types.h"

#include "core/ligma.h"
#include "core/ligmadatafactory.h"
#include "core/ligmaparamspecs.h"

#include "ligmapdb.h"
#include "ligmaprocedure.h"
#include "internal-procs.h"


static LigmaValueArray *
brushes_popup_invoker (LigmaProcedure         *procedure,
                       Ligma                  *ligma,
                       LigmaContext           *context,
                       LigmaProgress          *progress,
                       const LigmaValueArray  *args,
                       GError               **error)
{
  gboolean success = TRUE;
  const gchar *brush_callback;
  const gchar *popup_title;
  const gchar *initial_brush;
  gdouble opacity;
  gint spacing;
  gint paint_mode;

  brush_callback = g_value_get_string (ligma_value_array_index (args, 0));
  popup_title = g_value_get_string (ligma_value_array_index (args, 1));
  initial_brush = g_value_get_string (ligma_value_array_index (args, 2));
  opacity = g_value_get_double (ligma_value_array_index (args, 3));
  spacing = g_value_get_int (ligma_value_array_index (args, 4));
  paint_mode = g_value_get_enum (ligma_value_array_index (args, 5));

  if (success)
    {
      if (paint_mode == LIGMA_LAYER_MODE_OVERLAY_LEGACY)
        paint_mode = LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY;

      if (ligma->no_interface ||
          ! ligma_pdb_lookup_procedure (ligma->pdb, brush_callback) ||
          ! ligma_pdb_dialog_new (ligma, context, progress,
                                 ligma_data_factory_get_container (ligma->brush_factory),
                                 popup_title, brush_callback, initial_brush,
                                 "opacity",    opacity / 100.0,
                                 "paint-mode", paint_mode,
                                 "spacing",    spacing,
                                 NULL))
        success = FALSE;
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static LigmaValueArray *
brushes_close_popup_invoker (LigmaProcedure         *procedure,
                             Ligma                  *ligma,
                             LigmaContext           *context,
                             LigmaProgress          *progress,
                             const LigmaValueArray  *args,
                             GError               **error)
{
  gboolean success = TRUE;
  const gchar *brush_callback;

  brush_callback = g_value_get_string (ligma_value_array_index (args, 0));

  if (success)
    {
      if (ligma->no_interface ||
          ! ligma_pdb_lookup_procedure (ligma->pdb, brush_callback) ||
          ! ligma_pdb_dialog_close (ligma, ligma_data_factory_get_container (ligma->brush_factory),
                                   brush_callback))
        success = FALSE;
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static LigmaValueArray *
brushes_set_popup_invoker (LigmaProcedure         *procedure,
                           Ligma                  *ligma,
                           LigmaContext           *context,
                           LigmaProgress          *progress,
                           const LigmaValueArray  *args,
                           GError               **error)
{
  gboolean success = TRUE;
  const gchar *brush_callback;
  const gchar *brush_name;
  gdouble opacity;
  gint spacing;
  gint paint_mode;

  brush_callback = g_value_get_string (ligma_value_array_index (args, 0));
  brush_name = g_value_get_string (ligma_value_array_index (args, 1));
  opacity = g_value_get_double (ligma_value_array_index (args, 2));
  spacing = g_value_get_int (ligma_value_array_index (args, 3));
  paint_mode = g_value_get_enum (ligma_value_array_index (args, 4));

  if (success)
    {
      if (paint_mode == LIGMA_LAYER_MODE_OVERLAY_LEGACY)
        paint_mode = LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY;

      if (ligma->no_interface ||
          ! ligma_pdb_lookup_procedure (ligma->pdb, brush_callback) ||
          ! ligma_pdb_dialog_set (ligma, ligma_data_factory_get_container (ligma->brush_factory),
                                 brush_callback, brush_name,
                                 "opacity",    opacity / 100.0,
                                 "paint-mode", paint_mode,
                                 "spacing",    spacing,
                                 NULL))
        success = FALSE;
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

void
register_brush_select_procs (LigmaPDB *pdb)
{
  LigmaProcedure *procedure;

  /*
   * ligma-brushes-popup
   */
  procedure = ligma_procedure_new (brushes_popup_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-brushes-popup");
  ligma_procedure_set_static_help (procedure,
                                  "Invokes the Ligma brush selection.",
                                  "This procedure opens the brush selection dialog.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Andy Thomas",
                                         "Andy Thomas",
                                         "1998");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("brush-callback",
                                                       "brush callback",
                                                       "The callback PDB proc to call when brush selection is made",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("popup-title",
                                                       "popup title",
                                                       "Title of the brush selection dialog",
                                                       FALSE, FALSE, FALSE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("initial-brush",
                                                       "initial brush",
                                                       "The name of the brush to set as the first selected",
                                                       FALSE, TRUE, FALSE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_double ("opacity",
                                                    "opacity",
                                                    "The initial opacity of the brush",
                                                    0, 100, 0,
                                                    LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_int ("spacing",
                                                 "spacing",
                                                 "The initial spacing of the brush (if < 0 then use brush default spacing)",
                                                 G_MININT32, 1000, 0,
                                                 LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_enum ("paint-mode",
                                                  "paint mode",
                                                  "The initial paint mode",
                                                  LIGMA_TYPE_LAYER_MODE,
                                                  LIGMA_LAYER_MODE_NORMAL,
                                                  LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-brushes-close-popup
   */
  procedure = ligma_procedure_new (brushes_close_popup_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-brushes-close-popup");
  ligma_procedure_set_static_help (procedure,
                                  "Close the brush selection dialog.",
                                  "This procedure closes an opened brush selection dialog.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Andy Thomas",
                                         "Andy Thomas",
                                         "1998");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("brush-callback",
                                                       "brush callback",
                                                       "The name of the callback registered for this pop-up",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-brushes-set-popup
   */
  procedure = ligma_procedure_new (brushes_set_popup_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-brushes-set-popup");
  ligma_procedure_set_static_help (procedure,
                                  "Sets the current brush in a brush selection dialog.",
                                  "Sets the current brush in a brush selection dialog.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Andy Thomas",
                                         "Andy Thomas",
                                         "1998");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("brush-callback",
                                                       "brush callback",
                                                       "The name of the callback registered for this pop-up",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("brush-name",
                                                       "brush name",
                                                       "The name of the brush to set as selected",
                                                       FALSE, FALSE, FALSE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_double ("opacity",
                                                    "opacity",
                                                    "The initial opacity of the brush",
                                                    0, 100, 0,
                                                    LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_int ("spacing",
                                                 "spacing",
                                                 "The initial spacing of the brush (if < 0 then use brush default spacing)",
                                                 G_MININT32, 1000, 0,
                                                 LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_enum ("paint-mode",
                                                  "paint mode",
                                                  "The initial paint mode",
                                                  LIGMA_TYPE_LAYER_MODE,
                                                  LIGMA_LAYER_MODE_NORMAL,
                                                  LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
