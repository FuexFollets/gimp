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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmaerrorconsole.h"
#include "widgets/ligmahelp-ids.h"

#include "error-console-actions.h"
#include "error-console-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry error_console_actions[] =
{
  { "error-console-popup", LIGMA_ICON_DIALOG_WARNING,
    NC_("error-console-action", "Error Console Menu"), NULL, NULL, NULL,
    LIGMA_HELP_ERRORS_DIALOG },

  { "error-console-clear", LIGMA_ICON_EDIT_CLEAR,
    NC_("error-console-action", "_Clear"), NULL,
    NC_("error-console-action", "Clear error console"),
    error_console_clear_cmd_callback,
    LIGMA_HELP_ERRORS_CLEAR },

  { "error-console-select-all", NULL,
    NC_("error-console-action", "Select _All"), "",
    NC_("error-console-action", "Select all error messages"),
    error_console_select_all_cmd_callback,
    LIGMA_HELP_ERRORS_SELECT_ALL },

  { "error-console-highlight", NULL,
    NC_("error-console-action", "_Highlight"), NULL, NULL, NULL,
    LIGMA_HELP_ERRORS_HIGHLIGHT }
};

static const LigmaEnumActionEntry error_console_save_actions[] =
{
  { "error-console-save-all", LIGMA_ICON_DOCUMENT_SAVE_AS,
    NC_("error-console-action", "_Save Error Log to File..."), NULL,
    NC_("error-console-action", "Write all error messages to a file"),
    FALSE, FALSE,
    LIGMA_HELP_ERRORS_SAVE },

  { "error-console-save-selection", LIGMA_ICON_DOCUMENT_SAVE_AS,
    NC_("error-console-action", "Save S_election to File..."), NULL,
    NC_("error-console-action", "Write the selected error messages to a file"),
    TRUE, FALSE,
    LIGMA_HELP_ERRORS_SAVE }
};

static const LigmaToggleActionEntry error_console_highlight_actions[] =
{
  { "error-console-highlight-error", NULL,
    NC_("error-console-action", "_Errors"), NULL,
    NC_("error-console-action", "Highlight error console on errors"),
    error_console_highlight_error_cmd_callback,
    FALSE,
    LIGMA_HELP_ERRORS_HIGHLIGHT },

  { "error-console-highlight-warning", NULL,
    NC_("error-console-action", "_Warnings"), NULL,
    NC_("error-console-action", "Highlight error console on warnings"),
    error_console_highlight_warning_cmd_callback,
    FALSE,
    LIGMA_HELP_ERRORS_HIGHLIGHT },

  { "error-console-highlight-info", NULL,
    NC_("error-console-action", "_Messages"), NULL,
    NC_("error-console-action", "Highlight error console on messages"),
    error_console_highlight_info_cmd_callback,
    FALSE,
    LIGMA_HELP_ERRORS_HIGHLIGHT }
};


void
error_console_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "error-console-action",
                                 error_console_actions,
                                 G_N_ELEMENTS (error_console_actions));

  ligma_action_group_add_enum_actions (group, "error-console-action",
                                      error_console_save_actions,
                                      G_N_ELEMENTS (error_console_save_actions),
                                      error_console_save_cmd_callback);

  ligma_action_group_add_toggle_actions (group, "error-console-action",
                                        error_console_highlight_actions,
                                        G_N_ELEMENTS (error_console_highlight_actions));
}

void
error_console_actions_update (LigmaActionGroup *group,
                              gpointer         data)
{
  LigmaErrorConsole *console = LIGMA_ERROR_CONSOLE (data);
  gboolean          selection;

  selection = gtk_text_buffer_get_selection_bounds (console->text_buffer,
                                                    NULL, NULL);

#define SET_ACTIVE(action,condition)                                           \
        ligma_action_group_set_action_active (group, action, (condition) != 0)
#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("error-console-clear",          TRUE);
  SET_SENSITIVE ("error-console-select-all",     TRUE);
  SET_SENSITIVE ("error-console-save-all",       TRUE);
  SET_SENSITIVE ("error-console-save-selection", selection);
  SET_SENSITIVE ("error-console-highlight",      TRUE);

  SET_SENSITIVE ("error-console-highlight-error", TRUE);
  SET_ACTIVE ("error-console-highlight-error",
              console->highlight[LIGMA_MESSAGE_ERROR]);

  SET_SENSITIVE ("error-console-highlight-warning", TRUE);
  SET_ACTIVE ("error-console-highlight-warning",
              console->highlight[LIGMA_MESSAGE_WARNING]);

  SET_SENSITIVE ("error-console-highlight-info", TRUE);
  SET_ACTIVE ("error-console-highlight-info",
              console->highlight[LIGMA_MESSAGE_INFO]);

#undef SET_ACTIVE
#undef SET_SENSITIVE
}
