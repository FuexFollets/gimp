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

#include "core/ligma.h"
#include "core/ligmacontext.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmadataeditor.h"
#include "widgets/ligmahelp-ids.h"

#include "data-editor-commands.h"
#include "tool-preset-editor-actions.h"
#include "tool-preset-editor-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry tool_preset_editor_actions[] =
{
  { "tool-preset-editor-popup", LIGMA_ICON_TOOL_PRESET,
    NC_("tool-preset-editor-action", "Tool Preset Editor Menu"), NULL, NULL, NULL,
    LIGMA_HELP_TOOL_PRESET_EDITOR_DIALOG },

  { "tool-preset-editor-save", LIGMA_ICON_DOCUMENT_SAVE,
    NC_("tool-preset-editor-action", "_Save Tool Options to Preset"), NULL,
    NC_("tool-preset-editor-action", "Save the active tool options to this "
        "tool preset"),
    tool_preset_editor_save_cmd_callback,
    LIGMA_HELP_TOOL_PRESET_SAVE },

  { "tool-preset-editor-restore", LIGMA_ICON_DOCUMENT_REVERT,
    NC_("tool-preset-editor-action", "_Restore Tool Preset"), NULL,
    NC_("tool-preset-editor-action", "Restore this tool preset"),
    tool_preset_editor_restore_cmd_callback,
    LIGMA_HELP_TOOL_PRESET_RESTORE }
};


static const LigmaToggleActionEntry tool_preset_editor_toggle_actions[] =
{
  { "tool-preset-editor-edit-active", LIGMA_ICON_LINKED,
    NC_("tool-preset-editor-action", "Edit Active Tool Preset"), NULL, NULL,
    data_editor_edit_active_cmd_callback,
    FALSE,
    LIGMA_HELP_TOOL_PRESET_EDITOR_EDIT_ACTIVE }
};


void
tool_preset_editor_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "tool-preset-editor-action",
                                 tool_preset_editor_actions,
                                 G_N_ELEMENTS (tool_preset_editor_actions));

  ligma_action_group_add_toggle_actions (group, "tool-preset-editor-action",
                                        tool_preset_editor_toggle_actions,
                                        G_N_ELEMENTS (tool_preset_editor_toggle_actions));

}

void
tool_preset_editor_actions_update (LigmaActionGroup *group,
                                   gpointer         user_data)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (user_data);
  gboolean        edit_active = FALSE;

  edit_active = ligma_data_editor_get_edit_active (data_editor);

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("tool-preset-editor-save",        data_editor->data);
  SET_SENSITIVE ("tool-preset-editor-restore",     data_editor->data);
  SET_ACTIVE    ("tool-preset-editor-edit-active", edit_active);

#undef SET_SENSITIVE
#undef SET_ACTIVE
}
