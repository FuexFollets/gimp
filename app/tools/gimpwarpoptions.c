/* LIGMA - The GNU Image Manipulation Program
 *
 * ligmawarpoptions.c
 * Copyright (C) 2011 Michael Muré <batolettre@gmail.com>
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "widgets/ligmapropwidgets.h"

#include "ligmawarpoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_BEHAVIOR,
  PROP_EFFECT_SIZE,
  PROP_EFFECT_HARDNESS,
  PROP_EFFECT_STRENGTH,
  PROP_STROKE_SPACING,
  PROP_INTERPOLATION,
  PROP_ABYSS_POLICY,
  PROP_HIGH_QUALITY_PREVIEW,
  PROP_REAL_TIME_PREVIEW,
  PROP_STROKE_DURING_MOTION,
  PROP_STROKE_PERIODICALLY,
  PROP_STROKE_PERIODICALLY_RATE,
  PROP_N_ANIMATION_FRAMES
};


static void       ligma_warp_options_set_property (GObject      *object,
                                                  guint         property_id,
                                                  const GValue *value,
                                                  GParamSpec   *pspec);
static void       ligma_warp_options_get_property (GObject      *object,
                                                  guint         property_id,
                                                  GValue       *value,
                                                  GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaWarpOptions, ligma_warp_options,
               LIGMA_TYPE_TOOL_OPTIONS)

#define parent_class ligma_warp_options_parent_class


static void
ligma_warp_options_class_init (LigmaWarpOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_warp_options_set_property;
  object_class->get_property = ligma_warp_options_get_property;

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_BEHAVIOR,
                         "behavior",
                         _("Behavior"),
                         _("Behavior"),
                         LIGMA_TYPE_WARP_BEHAVIOR,
                         LIGMA_WARP_BEHAVIOR_MOVE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_EFFECT_SIZE,
                           "effect-size",
                           _("Size"),
                           _("Effect Size"),
                           1.0, 10000.0, 40.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_EFFECT_HARDNESS,
                           "effect-hardness",
                           _("Hardness"),
                           _("Effect Hardness"),
                           0.0, 100.0, 50.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_EFFECT_STRENGTH,
                           "effect-strength",
                           _("Strength"),
                           _("Effect Strength"),
                           1.0, 100.0, 50.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_SPACING,
                           "stroke-spacing",
                           _("Spacing"),
                           _("Stroke Spacing"),
                           1.0, 100.0, 10.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_INTERPOLATION,
                         "interpolation",
                         _("Interpolation"),
                         _("Interpolation method"),
                         LIGMA_TYPE_INTERPOLATION_TYPE,
                         LIGMA_INTERPOLATION_CUBIC,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_ABYSS_POLICY,
                         "abyss-policy",
                         _("Abyss policy"),
                         _("Out-of-bounds sampling behavior"),
                         GEGL_TYPE_ABYSS_POLICY,
                         GEGL_ABYSS_NONE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_HIGH_QUALITY_PREVIEW,
                            "high-quality-preview",
                            _("High quality preview"),
                            _("Use an accurate but slower preview"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_REAL_TIME_PREVIEW,
                            "real-time-preview",
                            _("Real-time preview"),
                            _("Render preview in real time (slower)"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_STROKE_DURING_MOTION,
                            "stroke-during-motion",
                            _("During motion"),
                            _("Apply effect during motion"),
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_STROKE_PERIODICALLY,
                            "stroke-periodically",
                            _("Periodically"),
                            _("Apply effect periodically"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_STROKE_PERIODICALLY_RATE,
                           "stroke-periodically-rate",
                           _("Rate"),
                           _("Periodic stroke rate"),
                           0.0, 100.0, 50.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_INT (object_class, PROP_N_ANIMATION_FRAMES,
                        "n-animation-frames",
                        _("Frames"),
                        _("Number of animation frames"),
                        3, 1000, 10,
                        LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_warp_options_init (LigmaWarpOptions *options)
{
}

static void
ligma_warp_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaWarpOptions *options = LIGMA_WARP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_BEHAVIOR:
      options->behavior = g_value_get_enum (value);
      break;
    case PROP_EFFECT_SIZE:
      options->effect_size = g_value_get_double (value);
      break;
    case PROP_EFFECT_HARDNESS:
      options->effect_hardness = g_value_get_double (value);
      break;
    case PROP_EFFECT_STRENGTH:
      options->effect_strength = g_value_get_double (value);
      break;
    case PROP_STROKE_SPACING:
      options->stroke_spacing = g_value_get_double (value);
      break;
    case PROP_INTERPOLATION:
      options->interpolation = g_value_get_enum (value);
      break;
    case PROP_ABYSS_POLICY:
      options->abyss_policy = g_value_get_enum (value);
      break;
    case PROP_HIGH_QUALITY_PREVIEW:
      options->high_quality_preview = g_value_get_boolean (value);
      break;
    case PROP_REAL_TIME_PREVIEW:
      options->real_time_preview = g_value_get_boolean (value);
      break;
    case PROP_STROKE_DURING_MOTION:
      options->stroke_during_motion = g_value_get_boolean (value);
      break;
    case PROP_STROKE_PERIODICALLY:
      options->stroke_periodically = g_value_get_boolean (value);
      break;
    case PROP_STROKE_PERIODICALLY_RATE:
      options->stroke_periodically_rate = g_value_get_double (value);
      break;
    case PROP_N_ANIMATION_FRAMES:
      options->n_animation_frames = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_warp_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaWarpOptions *options = LIGMA_WARP_OPTIONS (object);

  switch (property_id)
    {
    case PROP_BEHAVIOR:
      g_value_set_enum (value, options->behavior);
      break;
    case PROP_EFFECT_SIZE:
      g_value_set_double (value, options->effect_size);
      break;
    case PROP_EFFECT_HARDNESS:
      g_value_set_double (value, options->effect_hardness);
      break;
    case PROP_EFFECT_STRENGTH:
      g_value_set_double (value, options->effect_strength);
      break;
    case PROP_STROKE_SPACING:
      g_value_set_double (value, options->stroke_spacing);
      break;
    case PROP_INTERPOLATION:
      g_value_set_enum (value, options->interpolation);
      break;
    case PROP_ABYSS_POLICY:
      g_value_set_enum (value, options->abyss_policy);
      break;
    case PROP_HIGH_QUALITY_PREVIEW:
      g_value_set_boolean (value, options->high_quality_preview);
      break;
    case PROP_REAL_TIME_PREVIEW:
      g_value_set_boolean (value, options->real_time_preview);
      break;
    case PROP_STROKE_DURING_MOTION:
      g_value_set_boolean (value, options->stroke_during_motion);
      break;
    case PROP_STROKE_PERIODICALLY:
      g_value_set_boolean (value, options->stroke_periodically);
      break;
    case PROP_STROKE_PERIODICALLY_RATE:
      g_value_set_double (value, options->stroke_periodically_rate);
      break;
    case PROP_N_ANIMATION_FRAMES:
      g_value_set_int (value, options->n_animation_frames);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
ligma_warp_options_gui (LigmaToolOptions *tool_options)
{
  LigmaWarpOptions *options = LIGMA_WARP_OPTIONS (tool_options);
  GObject         *config  = G_OBJECT (tool_options);
  GtkWidget       *vbox    = ligma_tool_options_gui (tool_options);
  GtkWidget       *frame;
  GtkWidget       *vbox2;
  GtkWidget       *button;
  GtkWidget       *combo;
  GtkWidget       *scale;

  combo = ligma_prop_enum_combo_box_new (config, "behavior", 0, 0);
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);

  options->behavior_combo = combo;

  scale = ligma_prop_spin_scale_new (config, "effect-size",
                                    0.01, 1.0, 2);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), 1.0, 1000.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = ligma_prop_spin_scale_new (config, "effect-hardness",
                                    1, 10, 1);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), 0.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = ligma_prop_spin_scale_new (config, "effect-strength",
                                    1, 10, 1);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), 1.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  scale = ligma_prop_spin_scale_new (config, "stroke-spacing",
                                    1, 10, 1);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), 1.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);

  combo = ligma_prop_enum_combo_box_new (config, "interpolation", 0, 0);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Interpolation"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);

  combo = ligma_prop_enum_combo_box_new (config, "abyss-policy",
                                        GEGL_ABYSS_NONE, GEGL_ABYSS_LOOP);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Abyss policy"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);

  button = ligma_prop_check_button_new (config, "high-quality-preview", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ligma_prop_check_button_new (config, "real-time-preview", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  /*  the stroke frame  */
  frame = ligma_frame_new (_("Stroke"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  options->stroke_frame = frame;

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  button = ligma_prop_check_button_new (config, "stroke-during-motion", NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);

  scale = ligma_prop_spin_scale_new (config, "stroke-periodically-rate",
                                    1, 10, 1);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), 0.0, 100.0);

  frame = ligma_prop_expanding_frame_new (config, "stroke-periodically", NULL,
                                         scale, NULL);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, FALSE, 0);

  /*  the animation frame  */
  frame = ligma_frame_new (_("Animate"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (vbox2);

  scale = ligma_prop_spin_scale_new (config, "n-animation-frames",
                                    1.0, 10.0, 0);
  ligma_spin_scale_set_scale_limits (LIGMA_SPIN_SCALE (scale), 3.0, 100.0);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);

  options->animate_button = gtk_button_new_with_label (_("Create Animation"));
  gtk_widget_set_sensitive (options->animate_button, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox2), options->animate_button,
                      FALSE, FALSE, 0);
  gtk_widget_show (options->animate_button);

  g_object_add_weak_pointer (G_OBJECT (options->animate_button),
                             (gpointer) &options->animate_button);

  return vbox;
}
