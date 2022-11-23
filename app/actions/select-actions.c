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
#include "core/ligmachannel.h"
#include "core/ligmaimage.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "select-actions.h"
#include "select-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry select_actions[] =
{
  { "selection-popup", LIGMA_ICON_SELECTION,
    NC_("select-action", "Selection Editor Menu"), NULL, NULL, NULL,
    LIGMA_HELP_SELECTION_DIALOG },

  { "select-menu", NULL, NC_("select-action", "_Select") },

  { "select-all", LIGMA_ICON_SELECTION_ALL,
    NC_("select-action", "_All"), "<primary>A",
    NC_("select-action", "Select everything"),
    select_all_cmd_callback,
    LIGMA_HELP_SELECTION_ALL },

  { "select-none", LIGMA_ICON_SELECTION_NONE,
    NC_("select-action", "_None"), "<primary><shift>A",
    NC_("select-action", "Dismiss the selection"),
    select_none_cmd_callback,
    LIGMA_HELP_SELECTION_NONE },

  { "select-invert", LIGMA_ICON_INVERT,
    NC_("select-action", "_Invert"), "<primary>I",
    NC_("select-action", "Invert the selection"),
    select_invert_cmd_callback,
    LIGMA_HELP_SELECTION_INVERT },

  { "select-float", LIGMA_ICON_LAYER_FLOATING_SELECTION,
    NC_("select-action", "_Float"), "<primary><shift>L",
    NC_("select-action", "Create a floating selection"),
    select_float_cmd_callback,
    LIGMA_HELP_SELECTION_FLOAT },

  { "select-feather", NULL,
    NC_("select-action", "Fea_ther..."), NULL,
    NC_("select-action",
        "Blur the selection border so that it fades out smoothly"),
    select_feather_cmd_callback,
    LIGMA_HELP_SELECTION_FEATHER },

  { "select-sharpen", NULL,
    NC_("select-action", "_Sharpen"), NULL,
    NC_("select-action", "Remove fuzziness from the selection"),
    select_sharpen_cmd_callback,
    LIGMA_HELP_SELECTION_SHARPEN },

  { "select-shrink", LIGMA_ICON_SELECTION_SHRINK,
    NC_("select-action", "S_hrink..."), NULL,
    NC_("select-action", "Contract the selection"),
    select_shrink_cmd_callback,
    LIGMA_HELP_SELECTION_SHRINK },

  { "select-grow", LIGMA_ICON_SELECTION_GROW,
    NC_("select-action", "_Grow..."), NULL,
    NC_("select-action", "Enlarge the selection"),
    select_grow_cmd_callback,
    LIGMA_HELP_SELECTION_GROW },

  { "select-border", LIGMA_ICON_SELECTION_BORDER,
    NC_("select-action", "Bo_rder..."), NULL,
    NC_("select-action", "Replace the selection by its border"),
    select_border_cmd_callback,
    LIGMA_HELP_SELECTION_BORDER },

  { "select-flood", NULL,
    NC_("select-action", "Re_move Holes"), NULL,
    NC_("select-action", "Remove holes from the selection"),
    select_flood_cmd_callback,
    LIGMA_HELP_SELECTION_FLOOD },

  { "select-save", LIGMA_ICON_SELECTION_TO_CHANNEL,
    NC_("select-action", "Save to _Channel"), NULL,
    NC_("select-action", "Save the selection to a channel"),
    select_save_cmd_callback,
    LIGMA_HELP_SELECTION_TO_CHANNEL },

  { "select-fill", LIGMA_ICON_TOOL_BUCKET_FILL,
    NC_("select-action", "_Fill Selection Outline..."), NULL,
    NC_("select-action", "Fill the selection outline"),
    select_fill_cmd_callback,
    LIGMA_HELP_SELECTION_FILL },

  { "select-fill-last-values", LIGMA_ICON_TOOL_BUCKET_FILL,
    NC_("select-action", "_Fill Selection Outline with last values"), NULL,
    NC_("select-action", "Fill the selection outline with last used values"),
    select_fill_last_vals_cmd_callback,
    LIGMA_HELP_SELECTION_FILL },

  { "select-stroke", LIGMA_ICON_SELECTION_STROKE,
    NC_("select-action", "_Stroke Selection..."), NULL,
    NC_("select-action", "Paint along the selection outline"),
    select_stroke_cmd_callback,
    LIGMA_HELP_SELECTION_STROKE },

  { "select-stroke-last-values", LIGMA_ICON_SELECTION_STROKE,
    NC_("select-action", "_Stroke Selection with last values"), NULL,
    NC_("select-action", "Stroke the selection with last used values"),
    select_stroke_last_vals_cmd_callback,
    LIGMA_HELP_SELECTION_STROKE }
};


void
select_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "select-action",
                                 select_actions,
                                 G_N_ELEMENTS (select_actions));
}

void
select_actions_update (LigmaActionGroup *group,
                       gpointer         data)
{
  LigmaImage    *image    = action_data_get_image (data);
  gboolean      fs       = FALSE;
  gboolean      sel      = FALSE;

  GList        *drawables    = NULL;
  GList        *iter;
  gboolean      all_writable = TRUE;
  gboolean      no_groups    = TRUE;

  if (image)
    {
      drawables = ligma_image_get_selected_drawables (image);

      for (iter = drawables; iter; iter = iter->next)
        {
          if (ligma_item_is_content_locked (iter->data, NULL))
            all_writable = FALSE;

          if (ligma_viewable_get_children (iter->data))
            no_groups = FALSE;

          if (! all_writable && ! no_groups)
            break;
        }

      fs  = (ligma_image_get_floating_selection (image) != NULL);
      sel = ! ligma_channel_is_empty (ligma_image_get_mask (image));
    }

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)

  SET_SENSITIVE ("select-all",    image);
  SET_SENSITIVE ("select-none",   image && sel);
  SET_SENSITIVE ("select-invert", image);
  SET_SENSITIVE ("select-float",  g_list_length (drawables) == 1 && sel                 &&
                                  ! ligma_item_is_content_locked (drawables->data, NULL) &&
                                  ! ligma_viewable_get_children (drawables->data));

  SET_SENSITIVE ("select-feather", image && sel);
  SET_SENSITIVE ("select-sharpen", image && sel);
  SET_SENSITIVE ("select-shrink",  image && sel);
  SET_SENSITIVE ("select-grow",    image && sel);
  SET_SENSITIVE ("select-border",  image && sel);
  SET_SENSITIVE ("select-flood",   image && sel);

  SET_SENSITIVE ("select-save",               image && !fs);
  SET_SENSITIVE ("select-fill",               drawables && all_writable && no_groups && sel);
  SET_SENSITIVE ("select-fill-last-values",   drawables && all_writable && no_groups && sel);
  SET_SENSITIVE ("select-stroke",             drawables && all_writable && no_groups && sel);
  SET_SENSITIVE ("select-stroke-last-values", drawables && all_writable && no_groups && sel);

#undef SET_SENSITIVE

  g_list_free (drawables);
}
