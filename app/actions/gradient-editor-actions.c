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
#include "core/ligmadatafactory.h"
#include "core/ligmagradient.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmagradienteditor.h"
#include "widgets/ligmahelp-ids.h"

#include "data-editor-commands.h"
#include "gradient-editor-actions.h"
#include "gradient-editor-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry gradient_editor_actions[] =
{
  { "gradient-editor-popup", LIGMA_ICON_GRADIENT,
    NC_("gradient-editor-action", "Gradient Editor Menu"), NULL, NULL, NULL,
    LIGMA_HELP_GRADIENT_EDITOR_DIALOG },

  { "gradient-editor-left-color-type", NULL,
    NC_("gradient-editor-action", "Left Color Type") },
  { "gradient-editor-load-left-color", LIGMA_ICON_DOCUMENT_REVERT,
    NC_("gradient-editor-action", "_Load Left Color From") },
  { "gradient-editor-save-left-color", LIGMA_ICON_DOCUMENT_SAVE,
    NC_("gradient-editor-action", "_Save Left Color To") },

  { "gradient-editor-right-color-type", NULL,
    NC_("gradient-editor-action", "Right Color Type") },
  { "gradient-editor-load-right-color", LIGMA_ICON_DOCUMENT_REVERT,
    NC_("gradient-editor-action", "Load Right Color Fr_om") },
  { "gradient-editor-save-right-color", LIGMA_ICON_DOCUMENT_SAVE,
    NC_("gradient-editor-action", "Sa_ve Right Color To") },

  { "gradient-editor-blending-func", NULL, "blending-function" },
  { "gradient-editor-coloring-type", NULL, "coloring-type"     },

  { "gradient-editor-left-color", NULL,
    NC_("gradient-editor-action", "L_eft Endpoint's Color..."), NULL, NULL,
    gradient_editor_left_color_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_COLOR },

  { "gradient-editor-right-color", NULL,
    NC_("gradient-editor-action", "R_ight Endpoint's Color..."), NULL, NULL,
    gradient_editor_right_color_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_COLOR },

  { "gradient-editor-flip", LIGMA_ICON_OBJECT_FLIP_HORIZONTAL,
    "flip", NULL, NULL,
    gradient_editor_flip_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_FLIP },

  { "gradient-editor-replicate", LIGMA_ICON_OBJECT_DUPLICATE,
    "replicate", NULL, NULL,
    gradient_editor_replicate_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_FLIP },

  { "gradient-editor-split-midpoint", NULL,
    "splitmidpoint", NULL, NULL,
    gradient_editor_split_midpoint_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_SPLIT_MIDPOINT },

  { "gradient-editor-split-uniform", NULL,
    "splituniform", NULL, NULL,
    gradient_editor_split_uniformly_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_SPLIT_UNIFORM },

  { "gradient-editor-delete", LIGMA_ICON_EDIT_DELETE,
    "delete", "", NULL,
    gradient_editor_delete_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_DELETE },

  { "gradient-editor-recenter", NULL,
    "recenter", NULL, NULL,
    gradient_editor_recenter_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_RECENTER },

  { "gradient-editor-redistribute", NULL,
    "redistribute", NULL, NULL,
    gradient_editor_redistribute_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_REDISTRIBUTE },

  { "gradient-editor-blend-color", NULL,
    NC_("gradient-editor-action", "Ble_nd Endpoints' Colors"), NULL, NULL,
    gradient_editor_blend_color_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_BLEND_COLOR },

  { "gradient-editor-blend-opacity", NULL,
    NC_("gradient-editor-action", "Blend Endpoints' Opacit_y"), NULL, NULL,
    gradient_editor_blend_opacity_cmd_callback,
    LIGMA_HELP_GRADIENT_EDITOR_BLEND_OPACITY }
};

static const LigmaToggleActionEntry gradient_editor_toggle_actions[] =
{
  { "gradient-editor-edit-active", LIGMA_ICON_LINKED,
    NC_("gradient-editor-action", "Edit Active Gradient"), NULL, NULL,
    data_editor_edit_active_cmd_callback,
    FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_EDIT_ACTIVE }
};


#define LOAD_LEFT_FROM(num,magic) \
  { "gradient-editor-load-left-" num, NULL, \
    num, NULL, NULL, \
    (magic), FALSE, \
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_LOAD }
#define SAVE_LEFT_TO(num,magic) \
  { "gradient-editor-save-left-" num, NULL, \
    num, NULL, NULL, \
    (magic), FALSE, \
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_SAVE }
#define LOAD_RIGHT_FROM(num,magic) \
  { "gradient-editor-load-right-" num, NULL, \
    num, NULL, NULL, \
    (magic), FALSE, \
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_LOAD }
#define SAVE_RIGHT_TO(num,magic) \
  { "gradient-editor-save-right-" num, NULL, \
    num, NULL, NULL, \
    (magic), FALSE, \
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_SAVE }

static const LigmaEnumActionEntry gradient_editor_load_left_actions[] =
{
  { "gradient-editor-load-left-left-neighbor", NULL,
    NC_("gradient-editor-action", "_Left Neighbor's Right Endpoint"), NULL, NULL,
    GRADIENT_EDITOR_COLOR_NEIGHBOR_ENDPOINT, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_LOAD },

  { "gradient-editor-load-left-right-endpoint", NULL,
    NC_("gradient-editor-action", "_Right Endpoint"), NULL, NULL,
    GRADIENT_EDITOR_COLOR_OTHER_ENDPOINT, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_LOAD },

  { "gradient-editor-load-left-fg", NULL,
    NC_("gradient-editor-action", "_Foreground Color"), NULL, NULL,
    GRADIENT_EDITOR_COLOR_FOREGROUND, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_LOAD },

  { "gradient-editor-load-left-bg", NULL,
    NC_("gradient-editor-action", "_Background Color"), NULL, NULL,
    GRADIENT_EDITOR_COLOR_BACKGROUND, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_LOAD },

  LOAD_LEFT_FROM ("01", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 0),
  LOAD_LEFT_FROM ("02", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 1),
  LOAD_LEFT_FROM ("03", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 2),
  LOAD_LEFT_FROM ("04", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 3),
  LOAD_LEFT_FROM ("05", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 4),
  LOAD_LEFT_FROM ("06", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 5),
  LOAD_LEFT_FROM ("07", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 6),
  LOAD_LEFT_FROM ("08", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 7),
  LOAD_LEFT_FROM ("09", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 8),
  LOAD_LEFT_FROM ("10", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 9)
};

static const LigmaEnumActionEntry gradient_editor_save_left_actions[] =
{
  SAVE_LEFT_TO ("01", 0),
  SAVE_LEFT_TO ("02", 1),
  SAVE_LEFT_TO ("03", 2),
  SAVE_LEFT_TO ("04", 3),
  SAVE_LEFT_TO ("05", 4),
  SAVE_LEFT_TO ("06", 5),
  SAVE_LEFT_TO ("07", 6),
  SAVE_LEFT_TO ("08", 7),
  SAVE_LEFT_TO ("09", 8),
  SAVE_LEFT_TO ("10", 9)
};

static const LigmaEnumActionEntry gradient_editor_load_right_actions[] =
{
  { "gradient-editor-load-right-right-neighbor", NULL,
    NC_("gradient-editor-action", "_Right Neighbor's Left Endpoint"), NULL, NULL,
    GRADIENT_EDITOR_COLOR_NEIGHBOR_ENDPOINT, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_LOAD },

  { "gradient-editor-load-right-left-endpoint", NULL,
    NC_("gradient-editor-action", "_Left Endpoint"), NULL, NULL,
    GRADIENT_EDITOR_COLOR_OTHER_ENDPOINT, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_LOAD },

  { "gradient-editor-load-right-fg", NULL,
    NC_("gradient-editor-action", "_Foreground Color"), NULL, NULL,
    GRADIENT_EDITOR_COLOR_FOREGROUND, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_LOAD },

  { "gradient-editor-load-right-bg", NULL,
    NC_("gradient-editor-action", "_Background Color"), NULL, NULL,
    GRADIENT_EDITOR_COLOR_BACKGROUND, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_LOAD },

  LOAD_RIGHT_FROM ("01", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 0),
  LOAD_RIGHT_FROM ("02", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 1),
  LOAD_RIGHT_FROM ("03", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 2),
  LOAD_RIGHT_FROM ("04", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 3),
  LOAD_RIGHT_FROM ("05", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 4),
  LOAD_RIGHT_FROM ("06", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 5),
  LOAD_RIGHT_FROM ("07", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 6),
  LOAD_RIGHT_FROM ("08", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 7),
  LOAD_RIGHT_FROM ("09", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 8),
  LOAD_RIGHT_FROM ("10", GRADIENT_EDITOR_COLOR_FIRST_CUSTOM + 9)
};

static const LigmaEnumActionEntry gradient_editor_save_right_actions[] =
{
  SAVE_RIGHT_TO ("01", 0),
  SAVE_RIGHT_TO ("02", 1),
  SAVE_RIGHT_TO ("03", 2),
  SAVE_RIGHT_TO ("04", 3),
  SAVE_RIGHT_TO ("05", 4),
  SAVE_RIGHT_TO ("06", 5),
  SAVE_RIGHT_TO ("07", 6),
  SAVE_RIGHT_TO ("08", 7),
  SAVE_RIGHT_TO ("09", 8),
  SAVE_RIGHT_TO ("10", 9)
};

#undef LOAD_LEFT_FROM
#undef SAVE_LEFT_TO
#undef LOAD_RIGHT_FROM
#undef SAVE_RIGHT_TO


static const LigmaRadioActionEntry gradient_editor_left_color_type_actions[] =
{
  { "gradient-editor-left-color-fixed", NULL,
    NC_("gradient-editor-color-type", "_Fixed"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_FIXED,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_COLOR },

  { "gradient-editor-left-color-foreground", NULL,
    NC_("gradient-editor-color-type", "F_oreground Color"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_FOREGROUND,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_COLOR },

  { "gradient-editor-left-color-foreground-transparent", NULL,
    NC_("gradient-editor-color-type",
        "Fo_reground Color (Transparent)"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_COLOR },

  { "gradient-editor-left-color-background", NULL,
    NC_("gradient-editor-color-type", "_Background Color"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_BACKGROUND,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_COLOR },

  { "gradient-editor-left-color-background-transparent", NULL,
    NC_("gradient-editor-color-type",
        "B_ackground Color (Transparent)"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_BACKGROUND_TRANSPARENT,
    LIGMA_HELP_GRADIENT_EDITOR_LEFT_COLOR }
};

static const LigmaRadioActionEntry gradient_editor_right_color_type_actions[] =
{
  { "gradient-editor-right-color-fixed", NULL,
    NC_("gradient-editor-color-type", "_Fixed"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_FIXED,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_COLOR },

  { "gradient-editor-right-color-foreground", NULL,
    NC_("gradient-editor-color-type", "F_oreground Color"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_FOREGROUND,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_COLOR },

  { "gradient-editor-right-color-foreground-transparent", NULL,
    NC_("gradient-editor-color-type",
        "Fo_reground Color (Transparent)"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_COLOR },

  { "gradient-editor-right-color-background", NULL,
    NC_("gradient-editor-color-type", "_Background Color"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_BACKGROUND,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_COLOR },

  { "gradient-editor-right-color-background-transparent", NULL,
    NC_("gradient-editor-color-type",
        "B_ackground Color (Transparent)"), NULL, NULL,
    LIGMA_GRADIENT_COLOR_BACKGROUND_TRANSPARENT,
    LIGMA_HELP_GRADIENT_EDITOR_RIGHT_COLOR }
};

static const LigmaRadioActionEntry gradient_editor_blending_actions[] =
{
  { "gradient-editor-blending-linear", NULL,
    NC_("gradient-editor-blending", "_Linear"), NULL, NULL,
    LIGMA_GRADIENT_SEGMENT_LINEAR,
    LIGMA_HELP_GRADIENT_EDITOR_BLENDING },

  { "gradient-editor-blending-curved", NULL,
    NC_("gradient-editor-blending", "_Curved"), NULL, NULL,
    LIGMA_GRADIENT_SEGMENT_CURVED,
    LIGMA_HELP_GRADIENT_EDITOR_BLENDING },

  { "gradient-editor-blending-sine", NULL,
    NC_("gradient-editor-blending", "_Sinusoidal"), NULL, NULL,
    LIGMA_GRADIENT_SEGMENT_SINE,
    LIGMA_HELP_GRADIENT_EDITOR_BLENDING },

  { "gradient-editor-blending-sphere-increasing", NULL,
    NC_("gradient-editor-blending", "Spherical (i_ncreasing)"), NULL, NULL,
    LIGMA_GRADIENT_SEGMENT_SPHERE_INCREASING,
    LIGMA_HELP_GRADIENT_EDITOR_BLENDING },

  { "gradient-editor-blending-sphere-decreasing", NULL,
    NC_("gradient-editor-blending", "Spherical (_decreasing)"), NULL, NULL,
    LIGMA_GRADIENT_SEGMENT_SPHERE_DECREASING,
    LIGMA_HELP_GRADIENT_EDITOR_BLENDING },

  { "gradient-editor-blending-step", NULL,
    NC_("gradient-editor-blending", "S_tep"), NULL, NULL,
    LIGMA_GRADIENT_SEGMENT_STEP,
    LIGMA_HELP_GRADIENT_EDITOR_BLENDING },

  { "gradient-editor-blending-varies", NULL,
    NC_("gradient-editor-blending", "(Varies)"), NULL, NULL,
    -1,
    LIGMA_HELP_GRADIENT_EDITOR_BLENDING }
};

static const LigmaRadioActionEntry gradient_editor_coloring_actions[] =
{
  { "gradient-editor-coloring-rgb", NULL,
    NC_("gradient-editor-coloring", "_RGB"), NULL, NULL,
    LIGMA_GRADIENT_SEGMENT_RGB,
    LIGMA_HELP_GRADIENT_EDITOR_COLORING },

  { "gradient-editor-coloring-hsv-ccw", NULL,
    NC_("gradient-editor-coloring", "HSV (_counter-clockwise hue)"), NULL, NULL,
    LIGMA_GRADIENT_SEGMENT_HSV_CCW,
    LIGMA_HELP_GRADIENT_EDITOR_COLORING },

  { "gradient-editor-coloring-hsv-cw", NULL,
    NC_("gradient-editor-coloring", "HSV (clockwise _hue)"), NULL, NULL,
    LIGMA_GRADIENT_SEGMENT_HSV_CW,
    LIGMA_HELP_GRADIENT_EDITOR_COLORING },

  { "gradient-editor-coloring-varies", NULL,
    NC_("gradient-editor-coloring", "(Varies)"), NULL, NULL,
    -1,
    LIGMA_HELP_GRADIENT_EDITOR_COLORING }
};

static const LigmaEnumActionEntry gradient_editor_zoom_actions[] =
{
  { "gradient-editor-zoom-in", LIGMA_ICON_ZOOM_IN,
    N_("Zoom In"), NULL,
    N_("Zoom in"),
    LIGMA_ZOOM_IN, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_ZOOM_IN },

  { "gradient-editor-zoom-out", LIGMA_ICON_ZOOM_OUT,
    N_("Zoom Out"), NULL,
    N_("Zoom out"),
    LIGMA_ZOOM_OUT, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_ZOOM_OUT },

  { "gradient-editor-zoom-all", LIGMA_ICON_ZOOM_FIT_BEST,
    N_("Zoom All"), NULL,
    N_("Zoom all"),
    LIGMA_ZOOM_OUT_MAX, FALSE,
    LIGMA_HELP_GRADIENT_EDITOR_ZOOM_ALL }
};


void
gradient_editor_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "gradient-editor-action",
                                 gradient_editor_actions,
                                 G_N_ELEMENTS (gradient_editor_actions));

  ligma_action_group_add_toggle_actions (group, "gradient-editor-action",
                                        gradient_editor_toggle_actions,
                                        G_N_ELEMENTS (gradient_editor_toggle_actions));

  ligma_action_group_add_enum_actions (group, "gradient-editor-action",
                                      gradient_editor_load_left_actions,
                                      G_N_ELEMENTS (gradient_editor_load_left_actions),
                                      gradient_editor_load_left_cmd_callback);

  ligma_action_group_add_enum_actions (group, "gradient-editor-action",
                                      gradient_editor_save_left_actions,
                                      G_N_ELEMENTS (gradient_editor_save_left_actions),
                                      gradient_editor_save_left_cmd_callback);

  ligma_action_group_add_enum_actions (group, "gradient-editor-action",
                                      gradient_editor_load_right_actions,
                                      G_N_ELEMENTS (gradient_editor_load_right_actions),
                                      gradient_editor_load_right_cmd_callback);


  ligma_action_group_add_enum_actions (group, "gradient-editor-action",
                                      gradient_editor_save_right_actions,
                                      G_N_ELEMENTS (gradient_editor_save_right_actions),
                                      gradient_editor_save_right_cmd_callback);

  ligma_action_group_add_radio_actions (group, "gradient-editor-color-type",
                                       gradient_editor_left_color_type_actions,
                                       G_N_ELEMENTS (gradient_editor_left_color_type_actions),
                                       NULL,
                                       0,
                                       gradient_editor_left_color_type_cmd_callback);

  ligma_action_group_add_radio_actions (group, "gradient-editor-color-type",
                                       gradient_editor_right_color_type_actions,
                                       G_N_ELEMENTS (gradient_editor_right_color_type_actions),
                                       NULL,
                                       0,
                                       gradient_editor_right_color_type_cmd_callback);

  ligma_action_group_add_radio_actions (group, "gradient-editor-blending",
                                       gradient_editor_blending_actions,
                                       G_N_ELEMENTS (gradient_editor_blending_actions),
                                       NULL,
                                       0,
                                       gradient_editor_blending_func_cmd_callback);

  ligma_action_group_add_radio_actions (group, "gradient-editor-coloring",
                                       gradient_editor_coloring_actions,
                                       G_N_ELEMENTS (gradient_editor_coloring_actions),
                                       NULL,
                                       0,
                                       gradient_editor_coloring_type_cmd_callback);

  ligma_action_group_add_enum_actions (group, NULL,
                                      gradient_editor_zoom_actions,
                                      G_N_ELEMENTS (gradient_editor_zoom_actions),
                                      gradient_editor_zoom_cmd_callback);
}

void
gradient_editor_actions_update (LigmaActionGroup *group,
                                gpointer         data)
{
  LigmaGradientEditor  *editor         = LIGMA_GRADIENT_EDITOR (data);
  LigmaDataEditor      *data_editor    = LIGMA_DATA_EDITOR (data);
  LigmaGradient        *gradient;
  gboolean             editable       = FALSE;
  LigmaRGB              left_color;
  LigmaRGB              right_color;
  LigmaRGB              left_seg_color;
  LigmaRGB              right_seg_color;
  LigmaRGB              fg;
  LigmaRGB              bg;
  gboolean             blending_equal = TRUE;
  gboolean             coloring_equal = TRUE;
  gboolean             left_editable  = TRUE;
  gboolean             right_editable = TRUE;
  gboolean             selection      = FALSE;
  gboolean             delete         = FALSE;
  gboolean             edit_active    = FALSE;

  gradient = LIGMA_GRADIENT (data_editor->data);

  if (gradient)
    {
      LigmaGradientSegmentType  type;
      LigmaGradientSegmentColor color;
      LigmaGradientSegment     *left_seg;
      LigmaGradientSegment     *right_seg;
      LigmaGradientSegment     *seg, *aseg;

      if (data_editor->data_editable)
        editable = TRUE;

      ligma_gradient_segment_get_left_flat_color (gradient,
                                                 data_editor->context,
                                                 editor->control_sel_l,
                                                 &left_color);

      if (editor->control_sel_l->prev)
        left_seg = editor->control_sel_l->prev;
      else
        left_seg = ligma_gradient_segment_get_last (editor->control_sel_l);

      ligma_gradient_segment_get_right_flat_color (gradient,
                                                  data_editor->context,
                                                  left_seg,
                                                  &left_seg_color);

      ligma_gradient_segment_get_right_flat_color (gradient,
                                                  data_editor->context,
                                                  editor->control_sel_r,
                                                  &right_color);

      if (editor->control_sel_r->next)
        right_seg = editor->control_sel_r->next;
      else
        right_seg = ligma_gradient_segment_get_first (editor->control_sel_r);

      ligma_gradient_segment_get_left_flat_color (gradient,
                                                 data_editor->context,
                                                 right_seg,
                                                 &right_seg_color);

      left_editable  = (editor->control_sel_l->left_color_type ==
                        LIGMA_GRADIENT_COLOR_FIXED);
      right_editable = (editor->control_sel_r->right_color_type ==
                        LIGMA_GRADIENT_COLOR_FIXED);

      type  = editor->control_sel_l->type;
      color = editor->control_sel_l->color;

      seg = editor->control_sel_l;

      do
        {
          blending_equal = blending_equal && (seg->type == type);
          coloring_equal = coloring_equal && (seg->color == color);

          aseg = seg;
          seg  = seg->next;
        }
      while (aseg != editor->control_sel_r);

      selection = (editor->control_sel_l != editor->control_sel_r);
      delete    = (editor->control_sel_l->prev || editor->control_sel_r->next);
    }

  if (data_editor->context)
    {
      ligma_context_get_foreground (data_editor->context, &fg);
      ligma_context_get_background (data_editor->context, &bg);
    }

  /*  pretend the gradient not being editable while the dialog is
   *  insensitive. prevents the gradient from being modified while a
   *  dialog is running. bug #161411 --mitch
   */
  if (! gtk_widget_is_sensitive (GTK_WIDGET (editor)))
    editable = FALSE;

  if (! editable)
    {
      left_editable  = FALSE;
      right_editable = FALSE;
    }

  edit_active = ligma_data_editor_get_edit_active (data_editor);

#define SET_ACTIVE(action,condition) \
        ligma_action_group_set_action_active (group, action, (condition) != 0)
#define SET_COLOR(action,color,set_label) \
        ligma_action_group_set_action_color (group, action, (color), (set_label))
#define SET_LABEL(action,label) \
        ligma_action_group_set_action_label (group, action, (label))
#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_VISIBLE(action,condition) \
        ligma_action_group_set_action_visible (group, action, (condition) != 0)

  SET_SENSITIVE ("gradient-editor-left-color-fixed",                  editable);
  SET_SENSITIVE ("gradient-editor-left-color-foreground",             editable);
  SET_SENSITIVE ("gradient-editor-left-color-foreground-transparent", editable);
  SET_SENSITIVE ("gradient-editor-left-color-background",             editable);
  SET_SENSITIVE ("gradient-editor-left-color-background-transparent", editable);

  if (gradient)
    {
      switch (editor->control_sel_l->left_color_type)
        {
        case LIGMA_GRADIENT_COLOR_FIXED:
          SET_ACTIVE ("gradient-editor-left-color-fixed", TRUE);
          break;
        case LIGMA_GRADIENT_COLOR_FOREGROUND:
          SET_ACTIVE ("gradient-editor-left-color-foreground", TRUE);
          break;
        case LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT:
          SET_ACTIVE ("gradient-editor-left-color-foreground-transparent", TRUE);
          break;
        case LIGMA_GRADIENT_COLOR_BACKGROUND:
          SET_ACTIVE ("gradient-editor-left-color-background", TRUE);
          break;
        case LIGMA_GRADIENT_COLOR_BACKGROUND_TRANSPARENT:
          SET_ACTIVE ("gradient-editor-left-color-background-transparent", TRUE);
          break;
        }
    }

  SET_SENSITIVE ("gradient-editor-left-color",               left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-left-neighbor",  editable);
  SET_SENSITIVE ("gradient-editor-load-left-right-endpoint", editable);

  if (gradient)
    {
      SET_COLOR ("gradient-editor-left-color",
                 &left_color, FALSE);
      SET_COLOR ("gradient-editor-load-left-left-neighbor",
                 &left_seg_color, FALSE);
      SET_COLOR ("gradient-editor-load-left-right-endpoint",
                 &right_color, FALSE);
    }

  SET_SENSITIVE ("gradient-editor-load-left-fg", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-bg", left_editable);

  SET_COLOR ("gradient-editor-load-left-fg",
             data_editor->context ? &fg : NULL, FALSE);
  SET_COLOR ("gradient-editor-load-left-bg",
             data_editor->context ? &bg : NULL, FALSE);

  SET_SENSITIVE ("gradient-editor-load-left-01", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-02", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-03", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-04", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-05", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-06", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-07", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-08", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-09", left_editable);
  SET_SENSITIVE ("gradient-editor-load-left-10", left_editable);

  SET_COLOR ("gradient-editor-load-left-01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("gradient-editor-load-left-02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("gradient-editor-load-left-03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("gradient-editor-load-left-04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("gradient-editor-load-left-05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("gradient-editor-load-left-06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("gradient-editor-load-left-07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("gradient-editor-load-left-08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("gradient-editor-load-left-09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("gradient-editor-load-left-10", &editor->saved_colors[9], TRUE);

  SET_SENSITIVE ("gradient-editor-save-left-01", gradient);
  SET_SENSITIVE ("gradient-editor-save-left-02", gradient);
  SET_SENSITIVE ("gradient-editor-save-left-03", gradient);
  SET_SENSITIVE ("gradient-editor-save-left-04", gradient);
  SET_SENSITIVE ("gradient-editor-save-left-05", gradient);
  SET_SENSITIVE ("gradient-editor-save-left-06", gradient);
  SET_SENSITIVE ("gradient-editor-save-left-07", gradient);
  SET_SENSITIVE ("gradient-editor-save-left-08", gradient);
  SET_SENSITIVE ("gradient-editor-save-left-09", gradient);
  SET_SENSITIVE ("gradient-editor-save-left-10", gradient);

  SET_COLOR ("gradient-editor-save-left-01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("gradient-editor-save-left-02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("gradient-editor-save-left-03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("gradient-editor-save-left-04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("gradient-editor-save-left-05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("gradient-editor-save-left-06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("gradient-editor-save-left-07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("gradient-editor-save-left-08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("gradient-editor-save-left-09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("gradient-editor-save-left-10", &editor->saved_colors[9], TRUE);

  SET_SENSITIVE ("gradient-editor-right-color-fixed",                  editable);
  SET_SENSITIVE ("gradient-editor-right-color-foreground",             editable);
  SET_SENSITIVE ("gradient-editor-right-color-foreground-transparent", editable);
  SET_SENSITIVE ("gradient-editor-right-color-background",             editable);
  SET_SENSITIVE ("gradient-editor-right-color-background-transparent", editable);

  if (gradient)
    {
      switch (editor->control_sel_r->right_color_type)
        {
        case LIGMA_GRADIENT_COLOR_FIXED:
          SET_ACTIVE ("gradient-editor-right-color-fixed", TRUE);
          break;
        case LIGMA_GRADIENT_COLOR_FOREGROUND:
          SET_ACTIVE ("gradient-editor-right-color-foreground", TRUE);
          break;
        case LIGMA_GRADIENT_COLOR_FOREGROUND_TRANSPARENT:
          SET_ACTIVE ("gradient-editor-right-color-foreground-transparent", TRUE);
          break;
        case LIGMA_GRADIENT_COLOR_BACKGROUND:
          SET_ACTIVE ("gradient-editor-right-color-background", TRUE);
          break;
        case LIGMA_GRADIENT_COLOR_BACKGROUND_TRANSPARENT:
          SET_ACTIVE ("gradient-editor-right-color-background-transparent", TRUE);
          break;
        }
    }

  SET_SENSITIVE ("gradient-editor-right-color",               right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-right-neighbor", editable);
  SET_SENSITIVE ("gradient-editor-load-right-left-endpoint",  editable);

  if (gradient)
    {
      SET_COLOR ("gradient-editor-right-color",
                 &right_color, FALSE);
      SET_COLOR ("gradient-editor-load-right-right-neighbor",
                 &right_seg_color, FALSE);
      SET_COLOR ("gradient-editor-load-right-left-endpoint",
                 &left_color, FALSE);
    }

  SET_SENSITIVE ("gradient-editor-load-right-fg", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-bg", right_editable);

  SET_COLOR ("gradient-editor-load-right-fg",
             data_editor->context ? &fg : NULL, FALSE);
  SET_COLOR ("gradient-editor-load-right-bg",
             data_editor->context ? &bg : NULL, FALSE);

  SET_SENSITIVE ("gradient-editor-load-right-01", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-02", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-03", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-04", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-05", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-06", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-07", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-08", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-09", right_editable);
  SET_SENSITIVE ("gradient-editor-load-right-10", right_editable);

  SET_COLOR ("gradient-editor-load-right-01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("gradient-editor-load-right-02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("gradient-editor-load-right-03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("gradient-editor-load-right-04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("gradient-editor-load-right-05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("gradient-editor-load-right-06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("gradient-editor-load-right-07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("gradient-editor-load-right-08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("gradient-editor-load-right-09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("gradient-editor-load-right-10", &editor->saved_colors[9], TRUE);

  SET_SENSITIVE ("gradient-editor-save-right-01", gradient);
  SET_SENSITIVE ("gradient-editor-save-right-02", gradient);
  SET_SENSITIVE ("gradient-editor-save-right-03", gradient);
  SET_SENSITIVE ("gradient-editor-save-right-04", gradient);
  SET_SENSITIVE ("gradient-editor-save-right-05", gradient);
  SET_SENSITIVE ("gradient-editor-save-right-06", gradient);
  SET_SENSITIVE ("gradient-editor-save-right-07", gradient);
  SET_SENSITIVE ("gradient-editor-save-right-08", gradient);
  SET_SENSITIVE ("gradient-editor-save-right-09", gradient);
  SET_SENSITIVE ("gradient-editor-save-right-10", gradient);

  SET_COLOR ("gradient-editor-save-right-01", &editor->saved_colors[0], TRUE);
  SET_COLOR ("gradient-editor-save-right-02", &editor->saved_colors[1], TRUE);
  SET_COLOR ("gradient-editor-save-right-03", &editor->saved_colors[2], TRUE);
  SET_COLOR ("gradient-editor-save-right-04", &editor->saved_colors[3], TRUE);
  SET_COLOR ("gradient-editor-save-right-05", &editor->saved_colors[4], TRUE);
  SET_COLOR ("gradient-editor-save-right-06", &editor->saved_colors[5], TRUE);
  SET_COLOR ("gradient-editor-save-right-07", &editor->saved_colors[6], TRUE);
  SET_COLOR ("gradient-editor-save-right-08", &editor->saved_colors[7], TRUE);
  SET_COLOR ("gradient-editor-save-right-09", &editor->saved_colors[8], TRUE);
  SET_COLOR ("gradient-editor-save-right-10", &editor->saved_colors[9], TRUE);

  SET_SENSITIVE ("gradient-editor-flip",           editable);
  SET_SENSITIVE ("gradient-editor-replicate",      editable);
  SET_SENSITIVE ("gradient-editor-split-midpoint", editable);
  SET_SENSITIVE ("gradient-editor-split-uniform",  editable);
  SET_SENSITIVE ("gradient-editor-delete",         editable && delete);
  SET_SENSITIVE ("gradient-editor-recenter",       editable);
  SET_SENSITIVE ("gradient-editor-redistribute",   editable);

  if (! selection)
    {
      SET_LABEL ("gradient-editor-blending-func",
                 _("_Blending Function for Segment"));
      SET_LABEL ("gradient-editor-coloring-type",
                 _("Coloring _Type for Segment"));

      SET_LABEL ("gradient-editor-flip",
                 _("_Flip Segment"));
      SET_LABEL ("gradient-editor-replicate",
                 _("_Replicate Segment..."));
      SET_LABEL ("gradient-editor-split-midpoint",
                 _("Split Segment at _Midpoint"));
      SET_LABEL ("gradient-editor-split-uniform",
                 _("Split Segment _Uniformly..."));
      SET_LABEL ("gradient-editor-delete",
                 _("_Delete Segment"));
      SET_LABEL ("gradient-editor-recenter",
                 _("Re-_center Segment's Midpoint"));
      SET_LABEL ("gradient-editor-redistribute",
                 _("Re-distribute _Handles in Segment"));
    }
  else
    {
      SET_LABEL ("gradient-editor-blending-func",
                 _("_Blending Function for Selection"));
      SET_LABEL ("gradient-editor-coloring-type",
                 _("Coloring _Type for Selection"));

      SET_LABEL ("gradient-editor-flip",
                 _("_Flip Selection"));
      SET_LABEL ("gradient-editor-replicate",
                 _("_Replicate Selection..."));
      SET_LABEL ("gradient-editor-split-midpoint",
                 _("Split Segments at _Midpoints"));
      SET_LABEL ("gradient-editor-split-uniform",
                 _("Split Segments _Uniformly..."));
      SET_LABEL ("gradient-editor-delete",
                 _("_Delete Selection"));
      SET_LABEL ("gradient-editor-recenter",
                 _("Re-_center Midpoints in Selection"));
      SET_LABEL ("gradient-editor-redistribute",
                 _("Re-distribute _Handles in Selection"));
    }

  SET_SENSITIVE ("gradient-editor-blending-varies", FALSE);
  SET_VISIBLE   ("gradient-editor-blending-varies", ! blending_equal);

  SET_SENSITIVE ("gradient-editor-blending-linear",            editable);
  SET_SENSITIVE ("gradient-editor-blending-curved",            editable);
  SET_SENSITIVE ("gradient-editor-blending-sine",              editable);
  SET_SENSITIVE ("gradient-editor-blending-sphere-increasing", editable);
  SET_SENSITIVE ("gradient-editor-blending-sphere-decreasing", editable);
  SET_SENSITIVE ("gradient-editor-blending-step",              editable);

  if (blending_equal && gradient)
    {
      switch (editor->control_sel_l->type)
        {
        case LIGMA_GRADIENT_SEGMENT_LINEAR:
          SET_ACTIVE ("gradient-editor-blending-linear", TRUE);
          break;
        case LIGMA_GRADIENT_SEGMENT_CURVED:
          SET_ACTIVE ("gradient-editor-blending-curved", TRUE);
          break;
        case LIGMA_GRADIENT_SEGMENT_SINE:
          SET_ACTIVE ("gradient-editor-blending-sine", TRUE);
          break;
        case LIGMA_GRADIENT_SEGMENT_SPHERE_INCREASING:
          SET_ACTIVE ("gradient-editor-blending-sphere-increasing", TRUE);
          break;
        case LIGMA_GRADIENT_SEGMENT_SPHERE_DECREASING:
          SET_ACTIVE ("gradient-editor-blending-sphere-decreasing", TRUE);
          break;
        case LIGMA_GRADIENT_SEGMENT_STEP:
          SET_ACTIVE ("gradient-editor-blending-step", TRUE);
          break;
        }
    }
  else
    {
      SET_ACTIVE ("gradient-editor-blending-varies", TRUE);
    }

  SET_SENSITIVE ("gradient-editor-coloring-varies", FALSE);
  SET_VISIBLE   ("gradient-editor-coloring-varies", ! coloring_equal);

  SET_SENSITIVE ("gradient-editor-coloring-rgb",     editable);
  SET_SENSITIVE ("gradient-editor-coloring-hsv-ccw", editable);
  SET_SENSITIVE ("gradient-editor-coloring-hsv-cw",  editable);

  if (coloring_equal && gradient)
    {
      switch (editor->control_sel_l->color)
        {
        case LIGMA_GRADIENT_SEGMENT_RGB:
          SET_ACTIVE ("gradient-editor-coloring-rgb", TRUE);
          break;
        case LIGMA_GRADIENT_SEGMENT_HSV_CCW:
          SET_ACTIVE ("gradient-editor-coloring-hsv-ccw", TRUE);
          break;
        case LIGMA_GRADIENT_SEGMENT_HSV_CW:
          SET_ACTIVE ("gradient-editor-coloring-hsv-cw", TRUE);
          break;
        }
    }
  else
    {
      SET_ACTIVE ("gradient-editor-coloring-varies", TRUE);
    }

  SET_SENSITIVE ("gradient-editor-blend-color",   editable && selection);
  SET_SENSITIVE ("gradient-editor-blend-opacity", editable && selection);

  SET_SENSITIVE ("gradient-editor-zoom-out", gradient);
  SET_SENSITIVE ("gradient-editor-zoom-in",  gradient);
  SET_SENSITIVE ("gradient-editor-zoom-all", gradient);

  SET_ACTIVE ("gradient-editor-edit-active", edit_active);

#undef SET_ACTIVE
#undef SET_COLOR
#undef SET_LABEL
#undef SET_SENSITIVE
#undef SET_VISIBLE
}
