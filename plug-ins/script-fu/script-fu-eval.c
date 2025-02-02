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

#include "libligma/ligma.h"

#include "script-fu-eval.h"

#include "script-fu-lib.h"
#include "script-fu-intl.h"


LigmaValueArray *
script_fu_eval_run (LigmaProcedure        *procedure,
                    LigmaRunMode           run_mode,
                    const gchar          *code,
                    const LigmaValueArray *args)
{
  GString           *output = g_string_new (NULL);
  LigmaPDBStatusType  status = LIGMA_PDB_SUCCESS;

  script_fu_set_run_mode (run_mode);
  script_fu_redirect_output_to_gstr (output);

  switch (run_mode)
    {
    case LIGMA_RUN_NONINTERACTIVE:
      if (script_fu_interpret_string (code) != 0)
        status = LIGMA_PDB_EXECUTION_ERROR;
      break;

    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      status        = LIGMA_PDB_CALLING_ERROR;
      g_string_assign (output, _("Script-Fu evaluation mode only allows "
                                 "non-interactive invocation"));
      break;

    default:
      break;
    }

  if (status != LIGMA_PDB_SUCCESS && output->len > 0)
    {
      GError *error = g_error_new_literal (g_quark_from_string("scriptfu"), 0,
                                           g_string_free (output, FALSE));

      return ligma_procedure_new_return_values (procedure, status, error);
    }

  g_string_free (output, TRUE);

  return ligma_procedure_new_return_values (procedure, status, NULL);
}
