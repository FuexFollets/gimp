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

#include "widgets/ligmaactiongroup.h"

#include "debug-actions.h"
#include "debug-commands.h"


static const LigmaActionEntry debug_actions[] =
{
  { "debug-menu", NULL, "_Debug" },

  { "debug-gtk-inspector", NULL,
    "Start _GtkInspector", NULL, NULL,
    debug_gtk_inspector_cmd_callback,
    NULL },

  { "debug-mem-profile", NULL,
    "_Memory Profile", NULL, NULL,
    debug_mem_profile_cmd_callback,
    NULL },

  { "debug-benchmark-projection", NULL,
    "Benchmark _Projection", NULL,
    "Invalidates the entire projection, measures the time it takes to "
    "validate (render) the part that is visible in the active display, "
    "and print the result to stdout.",
    debug_benchmark_projection_cmd_callback,
    NULL },

  { "debug-show-image-graph", NULL,
    "Show Image _Graph", NULL,
    "Creates a new image showing the GEGL graph of this image",
    debug_show_image_graph_cmd_callback,
    NULL },

  { "debug-dump-items", NULL,
    "_Dump Items", NULL, NULL,
    debug_dump_menus_cmd_callback,
    NULL },

  { "debug-dump-managers", NULL,
    "Dump _UI Managers", NULL, NULL,
    debug_dump_managers_cmd_callback,
    NULL },

  { "debug-dump-keyboard-shortcuts", NULL,
    "Dump _Keyboard Shortcuts", NULL, NULL,
    debug_dump_keyboard_shortcuts_cmd_callback,
    NULL },

  { "debug-dump-attached-data", NULL,
    "Dump Attached Data", NULL, NULL,
    debug_dump_attached_data_cmd_callback,
    NULL }
};

void
debug_actions_setup (LigmaActionGroup *group)
{
  gint i;

  ligma_action_group_add_actions (group, NULL,
                                 debug_actions,
                                 G_N_ELEMENTS (debug_actions));

#define SET_VISIBLE(action,condition) \
        ligma_action_group_set_action_visible (group, action, (condition) != 0)

  for (i = 0; i < G_N_ELEMENTS (debug_actions); i++)
    SET_VISIBLE (debug_actions[i].name, group->ligma->show_debug_menu);

#undef SET_VISIBLE
}

void
debug_actions_update (LigmaActionGroup *group,
                      gpointer         data)
{
#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("debug-show-image-graph", gegl_has_operation ("gegl:introspect"));

#undef SET_SENSITIVE
}
