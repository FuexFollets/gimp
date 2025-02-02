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
fonts_popup_invoker (LigmaProcedure         *procedure,
                     Ligma                  *ligma,
                     LigmaContext           *context,
                     LigmaProgress          *progress,
                     const LigmaValueArray  *args,
                     GError               **error)
{
  gboolean success = TRUE;
  const gchar *font_callback;
  const gchar *popup_title;
  const gchar *initial_font;

  font_callback = g_value_get_string (ligma_value_array_index (args, 0));
  popup_title = g_value_get_string (ligma_value_array_index (args, 1));
  initial_font = g_value_get_string (ligma_value_array_index (args, 2));

  if (success)
    {
      if (ligma->no_interface ||
          ! ligma_pdb_lookup_procedure (ligma->pdb, font_callback) ||
          ! ligma_data_factory_data_wait (ligma->font_factory)     ||
          ! ligma_pdb_dialog_new (ligma, context, progress,
                                 ligma_data_factory_get_container (ligma->font_factory),
                                 popup_title, font_callback, initial_font,
                                 NULL))
        success = FALSE;
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static LigmaValueArray *
fonts_close_popup_invoker (LigmaProcedure         *procedure,
                           Ligma                  *ligma,
                           LigmaContext           *context,
                           LigmaProgress          *progress,
                           const LigmaValueArray  *args,
                           GError               **error)
{
  gboolean success = TRUE;
  const gchar *font_callback;

  font_callback = g_value_get_string (ligma_value_array_index (args, 0));

  if (success)
    {
      if (ligma->no_interface ||
          ! ligma_pdb_lookup_procedure (ligma->pdb, font_callback) ||
          ! ligma_pdb_dialog_close (ligma,
                                   ligma_data_factory_get_container (ligma->font_factory),
                                   font_callback))
        success = FALSE;
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static LigmaValueArray *
fonts_set_popup_invoker (LigmaProcedure         *procedure,
                         Ligma                  *ligma,
                         LigmaContext           *context,
                         LigmaProgress          *progress,
                         const LigmaValueArray  *args,
                         GError               **error)
{
  gboolean success = TRUE;
  const gchar *font_callback;
  const gchar *font_name;

  font_callback = g_value_get_string (ligma_value_array_index (args, 0));
  font_name = g_value_get_string (ligma_value_array_index (args, 1));

  if (success)
    {
      if (ligma->no_interface ||
          ! ligma_pdb_lookup_procedure (ligma->pdb, font_callback) ||
          ! ligma_data_factory_data_wait (ligma->font_factory)     ||
          ! ligma_pdb_dialog_set (ligma,
                                 ligma_data_factory_get_container (ligma->font_factory),
                                 font_callback, font_name,
                                 NULL))
        success = FALSE;
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

void
register_font_select_procs (LigmaPDB *pdb)
{
  LigmaProcedure *procedure;

  /*
   * ligma-fonts-popup
   */
  procedure = ligma_procedure_new (fonts_popup_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-fonts-popup");
  ligma_procedure_set_static_help (procedure,
                                  "Invokes the Ligma font selection.",
                                  "This procedure opens the font selection dialog.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Sven Neumann <sven@ligma.org>",
                                         "Sven Neumann",
                                         "2003");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("font-callback",
                                                       "font callback",
                                                       "The callback PDB proc to call when font selection is made",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("popup-title",
                                                       "popup title",
                                                       "Title of the font selection dialog",
                                                       FALSE, FALSE, FALSE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("initial-font",
                                                       "initial font",
                                                       "The name of the font to set as the first selected",
                                                       FALSE, TRUE, FALSE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-fonts-close-popup
   */
  procedure = ligma_procedure_new (fonts_close_popup_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-fonts-close-popup");
  ligma_procedure_set_static_help (procedure,
                                  "Close the font selection dialog.",
                                  "This procedure closes an opened font selection dialog.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Sven Neumann <sven@ligma.org>",
                                         "Sven Neumann",
                                         "2003");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("font-callback",
                                                       "font callback",
                                                       "The name of the callback registered for this pop-up",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-fonts-set-popup
   */
  procedure = ligma_procedure_new (fonts_set_popup_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-fonts-set-popup");
  ligma_procedure_set_static_help (procedure,
                                  "Sets the current font in a font selection dialog.",
                                  "Sets the current font in a font selection dialog.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Sven Neumann <sven@ligma.org>",
                                         "Sven Neumann",
                                         "2003");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("font-callback",
                                                       "font callback",
                                                       "The name of the callback registered for this pop-up",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("font-name",
                                                       "font name",
                                                       "The name of the font to set as selected",
                                                       FALSE, FALSE, FALSE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
