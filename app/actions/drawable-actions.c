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
#include "core/ligmaimage.h"
#include "core/ligmalayermask.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "drawable-actions.h"
#include "drawable-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry drawable_actions[] =
{
  { "drawable-equalize", NULL,
    NC_("drawable-action", "_Equalize"), NULL,
    NC_("drawable-action", "Automatic contrast enhancement"),
    drawable_equalize_cmd_callback,
    LIGMA_HELP_LAYER_EQUALIZE },

  { "drawable-levels-stretch", NULL,
    NC_("drawable-action", "_White Balance"), NULL,
    NC_("drawable-action", "Automatic white balance correction"),
    drawable_levels_stretch_cmd_callback,
    LIGMA_HELP_LAYER_WHITE_BALANCE }
};

static const LigmaToggleActionEntry drawable_toggle_actions[] =
{
  { "drawable-visible", LIGMA_ICON_VISIBLE,
    NC_("drawable-action", "Toggle Drawables _Visibility"), NULL, NULL,
    drawable_visible_cmd_callback,
    FALSE,
    LIGMA_HELP_LAYER_VISIBLE },

  { "drawable-lock-content", LIGMA_ICON_LOCK_CONTENT,
    NC_("drawable-action", "L_ock Pixels of Drawables"), NULL,
    NC_("drawable-action",
        "Keep the pixels on selected drawables from being modified"),
    drawable_lock_content_cmd_callback,
    FALSE,
    LIGMA_HELP_LAYER_LOCK_PIXELS },

  { "drawable-lock-position", LIGMA_ICON_LOCK_POSITION,
    NC_("drawable-action", "L_ock Position of Drawables"), NULL,
    NC_("drawable-action",
        "Keep the position on selected drawables from being modified"),
    drawable_lock_position_cmd_callback,
    FALSE,
    LIGMA_HELP_LAYER_LOCK_POSITION },
};

static const LigmaEnumActionEntry drawable_flip_actions[] =
{
  { "drawable-flip-horizontal", LIGMA_ICON_OBJECT_FLIP_HORIZONTAL,
    NC_("drawable-action", "Flip _Horizontally"), NULL,
    NC_("drawable-action", "Flip drawable horizontally"),
    LIGMA_ORIENTATION_HORIZONTAL, FALSE,
    LIGMA_HELP_LAYER_FLIP_HORIZONTAL },

  { "drawable-flip-vertical", LIGMA_ICON_OBJECT_FLIP_VERTICAL,
    NC_("drawable-action", "Flip _Vertically"), NULL,
    NC_("drawable-action", "Flip drawable vertically"),
    LIGMA_ORIENTATION_VERTICAL, FALSE,
    LIGMA_HELP_LAYER_FLIP_VERTICAL }
};

static const LigmaEnumActionEntry drawable_rotate_actions[] =
{
  { "drawable-rotate-90", LIGMA_ICON_OBJECT_ROTATE_90,
    NC_("drawable-action", "Rotate 90° _clockwise"), NULL,
    NC_("drawable-action", "Rotate drawable 90 degrees to the right"),
    LIGMA_ROTATE_90, FALSE,
    LIGMA_HELP_LAYER_ROTATE_90 },

  { "drawable-rotate-180", LIGMA_ICON_OBJECT_ROTATE_180,
    NC_("drawable-action", "Rotate _180°"), NULL,
    NC_("drawable-action", "Turn drawable upside-down"),
    LIGMA_ROTATE_180, FALSE,
    LIGMA_HELP_LAYER_ROTATE_180 },

  { "drawable-rotate-270", LIGMA_ICON_OBJECT_ROTATE_270,
    NC_("drawable-action", "Rotate 90° counter-clock_wise"), NULL,
    NC_("drawable-action", "Rotate drawable 90 degrees to the left"),
    LIGMA_ROTATE_270, FALSE,
    LIGMA_HELP_LAYER_ROTATE_270 }
};


void
drawable_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "drawable-action",
                                 drawable_actions,
                                 G_N_ELEMENTS (drawable_actions));

  ligma_action_group_add_toggle_actions (group, "drawable-action",
                                        drawable_toggle_actions,
                                        G_N_ELEMENTS (drawable_toggle_actions));

  ligma_action_group_add_enum_actions (group, "drawable-action",
                                      drawable_flip_actions,
                                      G_N_ELEMENTS (drawable_flip_actions),
                                      drawable_flip_cmd_callback);

  ligma_action_group_add_enum_actions (group, "drawable-action",
                                      drawable_rotate_actions,
                                      G_N_ELEMENTS (drawable_rotate_actions),
                                      drawable_rotate_cmd_callback);

#define SET_ALWAYS_SHOW_IMAGE(action,show) \
        ligma_action_group_set_action_always_show_image (group, action, show)

  SET_ALWAYS_SHOW_IMAGE ("drawable-rotate-90",  TRUE);
  SET_ALWAYS_SHOW_IMAGE ("drawable-rotate-180", TRUE);
  SET_ALWAYS_SHOW_IMAGE ("drawable-rotate-270", TRUE);

#undef SET_ALWAYS_SHOW_IMAGE
}

void
drawable_actions_update (LigmaActionGroup *group,
                         gpointer         data)
{
  LigmaImage *image;
  GList     *drawables     = NULL;
  GList     *iter;
  gboolean   has_visible   = FALSE;
  gboolean   locked        = TRUE;
  gboolean   can_lock      = FALSE;
  gboolean   locked_pos    = TRUE;
  gboolean   can_lock_pos  = FALSE;
  gboolean   all_rgb       = TRUE;
  gboolean   all_writable  = TRUE;
  gboolean   all_movable   = TRUE;
  gboolean   none_children = TRUE;

  image = action_data_get_image (data);

  if (image)
    {
      drawables = ligma_image_get_selected_drawables (image);

      for (iter = drawables; iter; iter = iter->next)
        {
          LigmaItem *item;

          if (ligma_item_get_visible (iter->data))
            has_visible = TRUE;

          if (ligma_item_can_lock_content (iter->data))
            {
              if (! ligma_item_get_lock_content (iter->data))
                locked = FALSE;
              can_lock = TRUE;
            }

          if (ligma_item_can_lock_position (iter->data))
            {
              if (! ligma_item_get_lock_position (iter->data))
                locked_pos = FALSE;
              can_lock_pos = TRUE;
            }

          if (ligma_viewable_get_children (LIGMA_VIEWABLE (iter->data)))
            none_children = FALSE;

          if (! ligma_drawable_is_rgb (iter->data))
            all_rgb = FALSE;

          if (LIGMA_IS_LAYER_MASK (iter->data))
            item = LIGMA_ITEM (ligma_layer_mask_get_layer (LIGMA_LAYER_MASK (iter->data)));
          else
            item = LIGMA_ITEM (iter->data);

          if (ligma_item_is_content_locked (item, NULL))
            all_writable = FALSE;

          if (ligma_item_is_position_locked (item, NULL))
            all_movable = FALSE;

          if (has_visible && ! locked && ! locked_pos &&
              ! none_children && ! all_rgb &&
              ! all_writable && ! all_movable)
            break;
        }
    }

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("drawable-equalize",       drawables && all_writable && none_children);
  SET_SENSITIVE ("drawable-levels-stretch", drawables && all_writable && none_children && all_rgb);

  SET_SENSITIVE ("drawable-visible",       drawables);
  SET_SENSITIVE ("drawable-lock-content",  can_lock);
  SET_SENSITIVE ("drawable-lock-position", can_lock_pos);

  SET_ACTIVE ("drawable-visible",       has_visible);
  SET_ACTIVE ("drawable-lock-content",  locked);
  SET_ACTIVE ("drawable-lock-position", locked_pos);

  SET_SENSITIVE ("drawable-flip-horizontal", drawables && all_writable && all_movable);
  SET_SENSITIVE ("drawable-flip-vertical",   drawables && all_writable && all_movable);

  SET_SENSITIVE ("drawable-rotate-90",  drawables && all_writable && all_movable);
  SET_SENSITIVE ("drawable-rotate-180", drawables && all_writable && all_movable);
  SET_SENSITIVE ("drawable-rotate-270", drawables && all_writable && all_movable);

#undef SET_SENSITIVE
#undef SET_ACTIVE

  g_list_free (drawables);
}
