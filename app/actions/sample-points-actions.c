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
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmasamplepointeditor.h"

#include "sample-points-actions.h"
#include "sample-points-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry sample_points_actions[] =
{
  { "sample-points-popup", LIGMA_ICON_SAMPLE_POINT,
    NC_("sample-points-action", "Sample Point Menu"), NULL, NULL, NULL,
    LIGMA_HELP_SAMPLE_POINT_DIALOG }
};

static const LigmaToggleActionEntry sample_points_toggle_actions[] =
{
  { "sample-points-sample-merged", NULL,
    NC_("sample-points-action", "_Sample Merged"), "",
    NC_("sample-points-action",
        "Use the composite color of all visible layers"),
    sample_points_sample_merged_cmd_callback,
    TRUE,
    LIGMA_HELP_SAMPLE_POINT_SAMPLE_MERGED }
};


void
sample_points_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "sample-points-action",
                                 sample_points_actions,
                                 G_N_ELEMENTS (sample_points_actions));

  ligma_action_group_add_toggle_actions (group, "sample-points-action",
                                        sample_points_toggle_actions,
                                        G_N_ELEMENTS (sample_points_toggle_actions));
}

void
sample_points_actions_update (LigmaActionGroup *group,
                              gpointer         data)
{
  LigmaSamplePointEditor *editor = LIGMA_SAMPLE_POINT_EDITOR (data);

#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)

  SET_ACTIVE ("sample-points-sample-merged",
              ligma_sample_point_editor_get_sample_merged (editor));

#undef SET_ACTIVE
}
