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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "tools-types.h"

#include "core/ligma.h"
#include "core/ligmaguide.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmachannel.h"

#include "vectors/ligmavectors.h"

#include "widgets/ligmapivotselector.h"
#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmaalignoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


#define ALIGN_VER_N_BUTTONS 3
#define ALIGN_HOR_N_BUTTONS 3
#define DISTR_VER_N_BUTTONS 2
#define DISTR_HOR_N_BUTTONS 2


enum
{
  ALIGN_BUTTON_CLICKED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_ALIGN_REFERENCE,
  PROP_ALIGN_LAYERS,
  PROP_ALIGN_VECTORS,
  PROP_ALIGN_CONTENTS,
  PROP_PIVOT_X,
  PROP_PIVOT_Y,
};

struct _LigmaAlignOptionsPrivate
{
  gboolean   align_layers;
  gboolean   align_vectors;
  gboolean   align_contents;
  gdouble    pivot_x;
  gdouble    pivot_y;

  GList     *selected_guides;
  GObject   *reference;

  GtkWidget *selected_guides_label;
  GtkWidget *reference_combo;
  GtkWidget *reference_box;
  GtkWidget *reference_label;
  GtkWidget *pivot_selector;

  GtkWidget *align_ver_button[ALIGN_VER_N_BUTTONS];
  GtkWidget *align_hor_button[ALIGN_HOR_N_BUTTONS];
  GtkWidget *distr_ver_button[DISTR_VER_N_BUTTONS];
  GtkWidget *distr_hor_button[DISTR_HOR_N_BUTTONS];
};


static void   ligma_align_options_finalize               (GObject           *object);
static void   ligma_align_options_set_property           (GObject           *object,
                                                         guint              property_id,
                                                         const GValue      *value,
                                                         GParamSpec        *pspec);
static void   ligma_align_options_get_property           (GObject           *object,
                                                         guint              property_id,
                                                         GValue            *value,
                                                         GParamSpec        *pspec);

static void   ligma_align_options_image_changed          (LigmaContext      *context,
                                                         LigmaImage        *image,
                                                         LigmaAlignOptions *options);

static void   ligma_align_options_update_area            (LigmaAlignOptions *options);
static void   ligma_align_options_guide_removed          (LigmaImage        *image,
                                                         LigmaGuide        *guide,
                                                         LigmaAlignOptions *options);
static void   ligma_align_options_reference_removed      (GObject          *object,
                                                         LigmaAlignOptions *options);
static void   ligma_align_options_pivot_changed          (LigmaPivotSelector *selector,
                                                         LigmaAlignOptions  *options);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaAlignOptions, ligma_align_options, LIGMA_TYPE_TOOL_OPTIONS)

#define parent_class ligma_selection_options_parent_class

static guint align_options_signals[LAST_SIGNAL] = { 0 };


static void
ligma_align_options_class_init (LigmaAlignOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize      = ligma_align_options_finalize;
  object_class->set_property  = ligma_align_options_set_property;
  object_class->get_property  = ligma_align_options_get_property;

  klass->align_button_clicked = NULL;

  align_options_signals[ALIGN_BUTTON_CLICKED] =
    g_signal_new ("align-button-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaAlignOptionsClass,
                                   align_button_clicked),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_ALIGNMENT_TYPE);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_ALIGN_REFERENCE,
                         "align-reference",
                         _("Relative to"),
                         _("Reference object targets will be aligned on"),
                         LIGMA_TYPE_ALIGN_REFERENCE_TYPE,
                         LIGMA_ALIGN_REFERENCE_IMAGE,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ALIGN_LAYERS,
                            "align-layers",
                            _("Selected layers"),
                            _("Selected layers will be aligned or distributed by the tool"),
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ALIGN_VECTORS,
                            "align-vectors",
                            _("Selected paths"),
                            _("Selected paths will be aligned or distributed by the tool"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ALIGN_CONTENTS,
                            "align-contents",
                            _("Use extents of layer contents"),
                            _("Instead of aligning or distributing on layer borders, use its content bounding box"),
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_PIVOT_X,
                           "pivot-x",
                           "X position of the point to align in objects",
                           NULL,
                           0.0, 1.0, 0.5,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_PIVOT_Y,
                           "pivot-y",
                           "Y position of the point to align in objects",
                           NULL,
                           0.0, 1.0, 0.5,
                           LIGMA_PARAM_STATIC_STRINGS);
}

static void
ligma_align_options_init (LigmaAlignOptions *options)
{
  options->priv = ligma_align_options_get_instance_private (options);

  options->priv->selected_guides = NULL;
}

static void
ligma_align_options_finalize (GObject *object)
{
  LigmaAlignOptions *options = LIGMA_ALIGN_OPTIONS (object);

  if (LIGMA_CONTEXT (options)->ligma)
    ligma_align_options_image_changed (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma),
                                      NULL, options);
}

static void
ligma_align_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaAlignOptions *options = LIGMA_ALIGN_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ALIGN_REFERENCE:
      options->align_reference = g_value_get_enum (value);
      ligma_align_options_update_area (options);
      break;

    case PROP_ALIGN_LAYERS:
      options->priv->align_layers = g_value_get_boolean (value);
      ligma_align_options_update_area (options);
      break;
    case PROP_ALIGN_VECTORS:
      options->priv->align_vectors = g_value_get_boolean (value);
      ligma_align_options_update_area (options);
      break;

    case PROP_ALIGN_CONTENTS:
      options->priv->align_contents = g_value_get_boolean (value);
      break;

    case PROP_PIVOT_X:
      options->priv->pivot_x = g_value_get_double (value);
      break;
    case PROP_PIVOT_Y:
      options->priv->pivot_y = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_align_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaAlignOptions *options = LIGMA_ALIGN_OPTIONS (object);

  switch (property_id)
    {
    case PROP_ALIGN_REFERENCE:
      g_value_set_enum (value, options->align_reference);
      break;

    case PROP_ALIGN_LAYERS:
      g_value_set_boolean (value, options->priv->align_layers);
      break;
    case PROP_ALIGN_VECTORS:
      g_value_set_boolean (value, options->priv->align_vectors);
      break;

    case PROP_ALIGN_CONTENTS:
      g_value_set_boolean (value, options->priv->align_contents);
      break;

    case PROP_PIVOT_X:
      g_value_set_double (value, options->priv->pivot_x);
      break;
    case PROP_PIVOT_Y:
      g_value_set_double (value, options->priv->pivot_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_align_options_button_clicked (GtkButton        *button,
                                   LigmaAlignOptions *options)
{
  LigmaAlignmentType action;

  action = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button),
                                               "align-action"));

  g_signal_emit (options, align_options_signals[ALIGN_BUTTON_CLICKED], 0,
                 action);
}

static GtkWidget *
ligma_align_options_button_new (LigmaAlignOptions  *options,
                               LigmaAlignmentType  action,
                               GtkWidget         *parent,
                               const gchar       *tooltip)
{
  GtkWidget   *button;
  GtkWidget   *image;
  const gchar *icon_name = NULL;

  switch (action)
    {
    case LIGMA_ALIGN_LEFT:
      icon_name = LIGMA_ICON_GRAVITY_WEST;
      break;
    case LIGMA_ALIGN_HCENTER:
      icon_name = LIGMA_ICON_CENTER_HORIZONTAL;
      break;
    case LIGMA_ALIGN_RIGHT:
      icon_name = LIGMA_ICON_GRAVITY_EAST;
      break;
    case LIGMA_ALIGN_TOP:
      icon_name = LIGMA_ICON_GRAVITY_NORTH;
      break;
    case LIGMA_ALIGN_VCENTER:
      icon_name = LIGMA_ICON_CENTER_VERTICAL;
      break;
    case LIGMA_ALIGN_BOTTOM:
      icon_name = LIGMA_ICON_GRAVITY_SOUTH;
      break;
    case LIGMA_ARRANGE_HFILL:
        icon_name = LIGMA_ICON_FILL_HORIZONTAL;
        break;
    case LIGMA_ARRANGE_VFILL:
        icon_name = LIGMA_ICON_FILL_VERTICAL;
        break;
    case LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP:
        icon_name = LIGMA_ICON_EVEN_HORIZONTAL_GAP;
        break;
    case LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP:
        icon_name = LIGMA_ICON_EVEN_VERTICAL_GAP;
        break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  button = gtk_button_new ();
  gtk_widget_set_sensitive (button, FALSE);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name (icon_name, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  gtk_box_pack_start (GTK_BOX (parent), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  ligma_help_set_help_data (button, tooltip, NULL);

  g_object_set_data (G_OBJECT (button), "align-action",
                     GINT_TO_POINTER (action));
  g_signal_connect (button, "clicked",
                    G_CALLBACK (ligma_align_options_button_clicked),
                    options);

  return button;
}

GtkWidget *
ligma_align_options_gui (LigmaToolOptions *tool_options)
{
  GObject          *config  = G_OBJECT (tool_options);
  LigmaAlignOptions *options = LIGMA_ALIGN_OPTIONS (tool_options);
  GtkWidget        *vbox    = ligma_tool_options_gui (tool_options);
  GtkWidget        *widget;
  GtkWidget        *section_vbox;
  GtkWidget        *items_grid;
  GtkWidget        *hbox;
  GtkWidget        *frame;
  GtkWidget        *combo;
  gchar            *text;
  gint              n = 0;

  /* Selected objects */
  frame = ligma_frame_new (_("Targets"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), section_vbox);
  gtk_widget_show (section_vbox);

  items_grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (section_vbox), items_grid);
  gtk_widget_show (items_grid);

  widget = ligma_prop_check_button_new (config, "align-contents", NULL);
  widget = ligma_prop_expanding_frame_new (config, "align-layers",
                                          NULL, widget, NULL);
  gtk_grid_attach (GTK_GRID (items_grid), widget, 0, 0, 1, 1);

  widget = ligma_prop_check_button_new (config, "align-vectors", NULL);
  gtk_grid_attach (GTK_GRID (items_grid), widget, 0, 1, 1, 1);

  options->priv->pivot_selector = ligma_pivot_selector_new (0.0, 0.0, 1.0, 1.0);
  gtk_widget_set_tooltip_text (options->priv->pivot_selector,
                               _("Set anchor point of targets"));
  ligma_pivot_selector_set_position (LIGMA_PIVOT_SELECTOR (options->priv->pivot_selector),
                                    options->priv->pivot_x, options->priv->pivot_y);
  gtk_grid_attach (GTK_GRID (items_grid), options->priv->pivot_selector, 1, 0, 1, 2);
  gtk_widget_show (options->priv->pivot_selector);

  g_signal_connect (options->priv->pivot_selector, "changed",
                    G_CALLBACK (ligma_align_options_pivot_changed),
                    options);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (section_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  widget = gtk_image_new_from_icon_name (LIGMA_ICON_CURSOR, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  /* TRANSLATORS: the %s strings are modifiers such as Shift, Alt or Cmd. */
  text = g_strdup_printf (_("%s-pick target guides (%s-%s to add more)"),
                          ligma_get_mod_string (GDK_MOD1_MASK),
                          ligma_get_mod_string (ligma_get_extend_selection_mask ()),
                          ligma_get_mod_string (GDK_MOD1_MASK));
  widget = gtk_label_new (text);
  gtk_label_set_line_wrap (GTK_LABEL (widget), TRUE);
  gtk_label_set_line_wrap_mode (GTK_LABEL (widget), PANGO_WRAP_WORD);
  g_free (text);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  options->priv->selected_guides_label = widget;

  /* Align frame */
  frame = ligma_frame_new (_("Align"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), section_vbox);
  gtk_widget_show (section_vbox);

  /* Align frame: reference */
  combo = ligma_prop_enum_combo_box_new (config, "align-reference", 0, 0);
  ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (combo), _("Relative to"));
  g_object_set (combo, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_box_pack_start (GTK_BOX (section_vbox), combo, FALSE, FALSE, 0);
  options->priv->reference_combo = combo;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (section_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  options->priv->reference_box = hbox;

  widget = gtk_image_new_from_icon_name (LIGMA_ICON_CURSOR, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gtk_label_new (_("Select the reference object"));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);

  widget = gtk_label_new (NULL);
  gtk_box_pack_start (GTK_BOX (section_vbox), widget, FALSE, FALSE, 0);
  gtk_widget_show (widget);
  options->priv->reference_label = widget;

  /* Align frame: buttons */

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (section_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  n = 0;
  options->priv->align_ver_button[n++] =
    ligma_align_options_button_new (options, LIGMA_ALIGN_LEFT, hbox,
                                   _("Align anchor points of targets on left edge of reference"));

  options->priv->align_ver_button[n++] =
    ligma_align_options_button_new (options, LIGMA_ALIGN_HCENTER, hbox,
                                   _("Align anchor points of targets on vertical middle of reference"));

  options->priv->align_ver_button[n++] =
    ligma_align_options_button_new (options, LIGMA_ALIGN_RIGHT, hbox,
                                   _("Align anchor points of targets on right edge of reference"));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (section_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  n = 0;
  options->priv->align_hor_button[n++] =
    ligma_align_options_button_new (options, LIGMA_ALIGN_TOP, hbox,
                                   _("Align anchor points of targets on top edge of reference"));

  options->priv->align_hor_button[n++] =
    ligma_align_options_button_new (options, LIGMA_ALIGN_VCENTER, hbox,
                                   _("Align anchor points of targets on horizontal middle of reference"));

  options->priv->align_hor_button[n++] =
    ligma_align_options_button_new (options, LIGMA_ALIGN_BOTTOM, hbox,
                                   _("Align anchor points of targets on bottom of reference"));

  /* Distribute frame */
  frame = ligma_frame_new (_("Distribute"));
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  section_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), section_vbox);
  gtk_widget_show (section_vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (section_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  n = 0;
  options->priv->distr_ver_button[n++] =
    ligma_align_options_button_new (options, LIGMA_ARRANGE_HFILL, hbox,
                                   _("Distribute anchor points of targets evenly in the horizontal"));
  options->priv->distr_ver_button[n++] =
    ligma_align_options_button_new (options, LIGMA_DISTRIBUTE_EVEN_HORIZONTAL_GAP, hbox,
                                   _("Distribute horizontally with even horizontal gaps"));

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (section_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  n = 0;
  options->priv->distr_hor_button[n++] =
    ligma_align_options_button_new (options, LIGMA_ARRANGE_VFILL, hbox,
                                   _("Distribute anchor points of targets evenly in the vertical"));
  options->priv->distr_hor_button[n++] =
    ligma_align_options_button_new (options, LIGMA_DISTRIBUTE_EVEN_VERTICAL_GAP, hbox,
                                   _("Distribute vertically with even vertical gaps"));

  g_signal_connect (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma),
                    "image-changed",
                    G_CALLBACK (ligma_align_options_image_changed),
                    tool_options);
  ligma_align_options_image_changed (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma),
                                    ligma_context_get_image (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma)),
                                    options);

  return vbox;
}

GList *
ligma_align_options_get_objects (LigmaAlignOptions *options)
{
  LigmaImage *image;
  GList     *objects = NULL;

  image = ligma_context_get_image (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma));

  if (image)
    {
      if (options->priv->align_layers)
        {
          GList *layers;

          layers = ligma_image_get_selected_layers (image);
          layers = g_list_copy (layers);
          objects = g_list_concat (objects, layers);
        }
      if (options->priv->align_vectors)
        {
          GList *vectors;

          vectors = ligma_image_get_selected_vectors (image);
          vectors = g_list_copy (vectors);
          objects = g_list_concat (objects, vectors);
        }

      if (options->priv->selected_guides)
        {
          GList *guides;

          guides = g_list_copy (options->priv->selected_guides);
          objects = g_list_concat (objects, guides);
        }
    }

  return objects;
}

void
ligma_align_options_get_pivot (LigmaAlignOptions *options,
                              gdouble          *x,
                              gdouble          *y)
{
  ligma_pivot_selector_get_position (LIGMA_PIVOT_SELECTOR (options->priv->pivot_selector),
                                    x, y);
}

void
ligma_align_options_pick_reference (LigmaAlignOptions *options,
                                   GObject          *object)
{
  if (options->priv->reference)
    g_signal_handlers_disconnect_by_func (options->priv->reference,
                                          G_CALLBACK (ligma_align_options_reference_removed),
                                          options);

  g_clear_object (&options->priv->reference);

  if (object)
    {
      options->priv->reference = g_object_ref (object);

      /* Both LigmaItem and LigmaGuide/LigmaAuxItem have a "removed" signal with
       * similar signature. */
      g_signal_connect_object (options->priv->reference,
                               "removed",
                               G_CALLBACK (ligma_align_options_reference_removed),
                               options, 0);
    }

  ligma_align_options_update_area (options);
}

GObject *
ligma_align_options_get_reference (LigmaAlignOptions *options,
                                  gboolean          blink_if_none)
{
  GObject   *reference = NULL;
  LigmaImage *image;

  image = ligma_context_get_image (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma));

  if (image)
    {
      switch (options->align_reference)
        {
        case LIGMA_ALIGN_REFERENCE_IMAGE:
          reference = G_OBJECT (image);
          break;
        case LIGMA_ALIGN_REFERENCE_SELECTION:
          reference = G_OBJECT (ligma_image_get_mask (image));
          break;
        case LIGMA_ALIGN_REFERENCE_PICK:
          reference = G_OBJECT (options->priv->reference);
          break;
        }

      if (reference == NULL && blink_if_none)
        {
          if (options->align_reference == LIGMA_ALIGN_REFERENCE_PICK)
            ligma_widget_blink (options->priv->reference_box);
          else
            ligma_widget_blink (options->priv->reference_combo);
        }
    }

  return reference;
}

gboolean
ligma_align_options_align_contents (LigmaAlignOptions *options)
{
  return options->priv->align_contents;
}

void
ligma_align_options_pick_guide (LigmaAlignOptions *options,
                               LigmaGuide        *guide,
                               gboolean          extend)
{
  if (! extend)
    g_clear_pointer (&options->priv->selected_guides, g_list_free);

  if (guide)
    {
      GList *list;

      if ((list = g_list_find (options->priv->selected_guides, guide)))
        options->priv->selected_guides = g_list_delete_link (options->priv->selected_guides, list);
      else
        options->priv->selected_guides = g_list_prepend (options->priv->selected_guides, guide);
    }

  ligma_align_options_update_area (options);
}


/*  Private functions  */

static void
ligma_align_options_image_changed (LigmaContext      *context,
                                  LigmaImage        *image,
                                  LigmaAlignOptions *options)
{
  LigmaImage *prev_image;

  prev_image = g_object_get_data (G_OBJECT (options), "ligma-align-options-image");

  if (image != prev_image)
    {
      /* We cannot keep track of selected guides across image changes. */
      g_clear_pointer (&options->priv->selected_guides, g_list_free);
      ligma_align_options_pick_reference (options, NULL);

      if (prev_image)
        {
          g_signal_handlers_disconnect_by_func (prev_image,
                                                G_CALLBACK (ligma_align_options_update_area),
                                                options);
          g_signal_handlers_disconnect_by_func (prev_image,
                                                G_CALLBACK (ligma_align_options_guide_removed),
                                                options);
        }
      if (image)
        {
          g_signal_connect_object (image, "selected-channels-changed",
                                   G_CALLBACK (ligma_align_options_update_area),
                                   options, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "selected-layers-changed",
                                   G_CALLBACK (ligma_align_options_update_area),
                                   options, G_CONNECT_SWAPPED);
          g_signal_connect_object (image, "guide-removed",
                                   G_CALLBACK (ligma_align_options_guide_removed),
                                   options, 0);
        }

      g_object_set_data (G_OBJECT (options), "ligma-align-options-image", image);
      ligma_align_options_update_area (options);
    }
}

static void
ligma_align_options_update_area (LigmaAlignOptions *options)
{
  LigmaImage *image;
  GList     *layers           = NULL;
  GList     *vectors          = NULL;
  gboolean   enable_ver_align = FALSE;
  gboolean   enable_hor_align = FALSE;
  gboolean   enable_ver_distr = FALSE;
  gboolean   enable_hor_distr = FALSE;
  gint       n_items = 0;
  gchar     *text;

  image = ligma_context_get_image (ligma_get_user_context (LIGMA_CONTEXT (options)->ligma));

  /* GUI not created yet. */
  if (! options->priv->reference_combo)
    return;

  if (image)
    {
      layers = ligma_image_get_selected_layers (image);
      vectors = ligma_image_get_selected_vectors (image);

      if (options->priv->align_layers)
        n_items += g_list_length (layers);
      if (options->priv->align_vectors)
        n_items += g_list_length (vectors);

      n_items += g_list_length (options->priv->selected_guides);
    }

  if (n_items > 0)
    {
      GObject *reference;

      reference = ligma_align_options_get_reference (options, FALSE);

      enable_ver_align = (reference != NULL &&
                          (! LIGMA_IS_GUIDE (reference) ||
                           ligma_guide_get_orientation (LIGMA_GUIDE (reference)) == LIGMA_ORIENTATION_VERTICAL));
      enable_hor_align = (reference != NULL &&
                          (! LIGMA_IS_GUIDE (reference) ||
                           ligma_guide_get_orientation (LIGMA_GUIDE (reference)) == LIGMA_ORIENTATION_HORIZONTAL));
      enable_ver_distr = enable_hor_distr = (n_items > 2);
    }
  for (gint i = 0; i < ALIGN_VER_N_BUTTONS; i++)
    gtk_widget_set_sensitive (options->priv->align_ver_button[i], enable_ver_align);
  for (gint i = 0; i < ALIGN_HOR_N_BUTTONS; i++)
    gtk_widget_set_sensitive (options->priv->align_hor_button[i], enable_hor_align);
  for (gint i = 0; i < DISTR_VER_N_BUTTONS; i++)
    gtk_widget_set_sensitive (options->priv->distr_ver_button[i], enable_ver_distr);
  for (gint i = 0; i < DISTR_HOR_N_BUTTONS; i++)
    gtk_widget_set_sensitive (options->priv->distr_hor_button[i], enable_hor_distr);

  /* Update the guide picking widgets. */
  if (options->priv->selected_guides)
    {
      gchar *tmp_txt;

      tmp_txt = g_strdup_printf (ngettext ("1 guide will be aligned or distributed",
                                           "%d guides will be aligned or distributed",
                                           g_list_length (options->priv->selected_guides)),
                                 g_list_length (options->priv->selected_guides));
      text = g_strdup_printf ("<i>%s</i>", tmp_txt);
      g_free (tmp_txt);
      gtk_widget_show (options->priv->selected_guides_label);
    }
  else
    {
      text = NULL;
      gtk_widget_hide (options->priv->selected_guides_label);
    }

  gtk_label_set_markup (GTK_LABEL (options->priv->selected_guides_label), text);
  g_free (text);

  /* Update the reference widgets. */
  text = NULL;
  if (options->align_reference == LIGMA_ALIGN_REFERENCE_PICK)
    {
      if (options->priv->reference)
        {
          gchar *tmp_txt;

          if (LIGMA_IS_LAYER (options->priv->reference))
            tmp_txt = g_strdup_printf (_("Reference layer: %s"),
                                       ligma_object_get_name (options->priv->reference));
          else if (LIGMA_IS_CHANNEL (options->priv->reference))
            tmp_txt = g_strdup_printf (_("Reference channel: %s"),
                                       ligma_object_get_name (options->priv->reference));
          else if (LIGMA_IS_VECTORS (options->priv->reference))
            tmp_txt = g_strdup_printf (_("Reference path: %s"),
                                       ligma_object_get_name (options->priv->reference));
          else if (LIGMA_IS_GUIDE (options->priv->reference))
            tmp_txt = g_strdup (_("Reference guide"));
          else
            g_return_if_reached ();

          text = g_strdup_printf ("<i>%s</i>", tmp_txt);
          g_free (tmp_txt);
        }
      gtk_widget_show (options->priv->reference_box);
    }
  else
    {
      gtk_widget_hide (options->priv->reference_box);
    }
  gtk_label_set_markup (GTK_LABEL (options->priv->reference_label), text);
  g_free (text);
}

static void
ligma_align_options_guide_removed (LigmaImage        *image,
                                  LigmaGuide        *guide,
                                  LigmaAlignOptions *options)
{
  GList *list;

  if ((list = g_list_find (options->priv->selected_guides, guide)))
    options->priv->selected_guides = g_list_delete_link (options->priv->selected_guides, list);

  if (G_OBJECT (guide) == options->priv->reference)
    ligma_align_options_pick_reference (options, NULL);

  ligma_align_options_update_area (options);
}

static void
ligma_align_options_reference_removed (GObject          *object,
                                      LigmaAlignOptions *options)
{
  if (G_OBJECT (object) == options->priv->reference)
    ligma_align_options_pick_reference (options, NULL);
}

static void
ligma_align_options_pivot_changed (LigmaPivotSelector *selector,
                                  LigmaAlignOptions  *options)
{
  gdouble x;
  gdouble y;

  ligma_pivot_selector_get_position (selector, &x, &y);
  g_object_set (options,
                "pivot-x", x,
                "pivot-y", y,
                NULL);
}
