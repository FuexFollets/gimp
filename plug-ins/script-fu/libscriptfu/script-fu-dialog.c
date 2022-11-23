/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * script-fu-dialog.c
 * Copyright (C) 2022 Lloyd Konneker
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

#include <libligma/ligmaui.h>

#include "script-fu-types.h"    /* SFScript */
#include "script-fu-script.h"   /* get_title */
#include "script-fu-command.h"

#include "script-fu-dialog.h"


/* An informal class that shows a dialog for a script then runs the script.
 * It is internal to libscriptfu.
 *
 * The dialog is modal for the script:
 * OK button hides the dialog then runs the script once.
 *
 * The dialog is non-modal with respect to the LIGMA app GUI, which remains responsive.
 *
 * When called from plugin extension-script-fu, the dialog is modal on the extension:
 * although LIGMA app continues responsive, a user choosing a menu item
 * that is also implemented by a script and extension-script-fu
 * will not show a dialog until the first called script finishes.
 */

/* FUTURE: delete this after v3 is stable. */
#define DEBUG_CONFIG_PROPERTIES  FALSE

#if DEBUG_CONFIG_PROPERTIES
static void
dump_properties (LigmaProcedureConfig  *config)
{
  GParamSpec **pspecs;
  guint        n_pspecs;

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);
  for (guint i = 1; i < n_pspecs; i++)
    g_printerr ("%s %s\n", pspecs[i]->name, G_PARAM_SPEC_TYPE_NAME (pspecs[i]));
  g_free (pspecs);
}
#endif

/* Run a dialog for a procedure, then interpret the script.
 *
 * Run dialog: create config, create dialog for config, show dialog, and return a config.
 * Interpret: marshal config into Scheme text for function call, then interpret script.
 *
 * One widget per param of the procedure.
 * Require the procedure registered with params of GTypes
 * corresponding to SFType the author declared in script-fu-register call.
 *
 * Require initial_args is not NULL or empty.
 * A caller must ensure a dialog is needed because args is not empty.
 */
LigmaValueArray*
script_fu_dialog_run (LigmaProcedure        *procedure,
                      SFScript             *script,
                      LigmaImage            *image,
                      guint                 n_drawables,
                      LigmaDrawable        **drawables,
                      const LigmaValueArray *initial_args)

{
  LigmaValueArray      *result = NULL;
  LigmaProcedureDialog *dialog = NULL;
  LigmaProcedureConfig *config = NULL;
  gboolean             not_canceled;

  if ( (! G_IS_OBJECT (procedure)) || script == NULL)
    return ligma_procedure_new_return_values (procedure, LIGMA_PDB_EXECUTION_ERROR, NULL);

  if ( ligma_value_array_length (initial_args) < 1)
    return ligma_procedure_new_return_values (procedure, LIGMA_PDB_EXECUTION_ERROR, NULL);

  /* We don't prevent concurrent dialogs as in script-fu-interface.c.
   * For extension-script-fu, Ligma is already preventing concurrent dialogs.
   * For ligma-script-fu-interpreter, each plugin is a separate process
   * so concurrent dialogs CAN occur.
   */
  /* There is no progress widget in LigmaProcedureDialog.
   * Also, we don't need to update the progress in Ligma UI,
   * because Ligma shows progress: the name of all called PDB procedures.
   */

  /* Script's menu label */
  ligma_ui_init (script_fu_script_get_title (script));

  config = ligma_procedure_create_config (procedure);
#if DEBUG_CONFIG_PROPERTIES
  dump_properties (config);
  g_debug ("Len  of initial_args %i", ligma_value_array_length (initial_args) );
#endif

  /* Get saved settings (last values) into the config.
   * Since run mode is INTERACTIVE, initial_args is moot.
   * Instead, last used values or default values populate the config.
   */
  ligma_procedure_config_begin_run (config, NULL, LIGMA_RUN_INTERACTIVE, initial_args);

  /* Create a dialog having properties (describing arguments of the procedure)
   * taken from the config.
   *
   * Title dialog with the menu item, not the procedure name.
   * Assert menu item is localized.
   */
  dialog = (LigmaProcedureDialog*) ligma_procedure_dialog_new (
                                      procedure,
                                      config,
                                      script_fu_script_get_title (script));
  /* dialog has no widgets except standard buttons. */

  /* It is possible to create custom widget where the provided widget is not adequate.
   * Then ligma_procedure_dialog_fill_list will create the rest.
   * For now, the provided widgets should be adequate.
   */

  /* NULL means create widgets for all properties of the procedure
   * that we have not already created widgets for.
   */
  ligma_procedure_dialog_fill_list (dialog, NULL);

  not_canceled = ligma_procedure_dialog_run (dialog);
  /* Assert config holds validated arg values from a user interaction. */
  if (not_canceled)
    {
      /* initial_args is declared const.
       * To avoid compiler warning "discarding const"
       * copy initial_args to a writeable copy.
       */
      LigmaValueArray *final_args = (LigmaValueArray*) g_value_array_copy ((GValueArray*) initial_args);
      /* FIXME the above is deprecated.
       * Non-deprecated, but doesn't work:
       * LigmaValueArray *final_args = (LigmaValueArray*) g_array_copy ((GArray*) initial_args);
       * Maybe we need a ligma_value_array_copy method?
       */

      /* Store config's values into final_args. */
      ligma_procedure_config_get_values (config, final_args);

      result = script_fu_interpret_image_proc (procedure, script, image, n_drawables, drawables, final_args);
    }
  else
    {
      result = ligma_procedure_new_return_values (procedure, LIGMA_PDB_CANCEL, NULL);
    }

  gtk_widget_destroy ((GtkWidget*) dialog);

  /* Persist config aka settings for the next run of the plugin.
   * Passing the LigmaPDBStatus from result[0].
   * We must have a matching end_run for the begin_run, regardless of status.
   */
  ligma_procedure_config_end_run (config, g_value_get_enum (ligma_value_array_index (result, 0)));

  g_object_unref (config);

  return result;
}
