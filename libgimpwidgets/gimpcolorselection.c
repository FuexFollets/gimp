/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacolorselection.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "ligmawidgetstypes.h"

#include "ligmacolorarea.h"
#include "ligmacolornotebook.h"
#include "ligmacolorscales.h"
#include "ligmacolorselect.h"
#include "ligmacolorselection.h"
#include "ligmahelpui.h"
#include "ligmaicons.h"
#include "ligmawidgets.h"
#include "ligmawidgets-private.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmacolorselection
 * @title: LigmaColorSelection
 * @short_description: Widget for doing a color selection.
 *
 * Widget for doing a color selection.
 **/


#define COLOR_AREA_SIZE  20


typedef enum
{
  UPDATE_NOTEBOOK  = 1 << 0,
  UPDATE_SCALES    = 1 << 1,
  UPDATE_ENTRY     = 1 << 2,
  UPDATE_COLOR     = 1 << 3
} UpdateType;

#define UPDATE_ALL (UPDATE_NOTEBOOK | \
                    UPDATE_SCALES   | \
                    UPDATE_ENTRY    | \
                    UPDATE_COLOR)

enum
{
  COLOR_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CONFIG
};


struct _LigmaColorSelectionPrivate
{
  gboolean                  show_alpha;

  LigmaHSV                   hsv;
  LigmaRGB                   rgb;
  LigmaColorSelectorChannel  channel;

  GtkWidget                *left_vbox;
  GtkWidget                *right_vbox;

  GtkWidget                *notebook;
  GtkWidget                *scales;

  GtkWidget                *new_color;
  GtkWidget                *old_color;
};

#define GET_PRIVATE(obj) (((LigmaColorSelection *) (obj))->priv)


static void   ligma_color_selection_set_property      (GObject            *object,
                                                      guint               property_id,
                                                      const GValue       *value,
                                                      GParamSpec         *pspec);

static void   ligma_color_selection_switch_page       (GtkWidget          *widget,
                                                      gpointer            page,
                                                      guint               page_num,
                                                      LigmaColorSelection *selection);
static void   ligma_color_selection_notebook_changed  (LigmaColorSelector  *selector,
                                                      const LigmaRGB      *rgb,
                                                      const LigmaHSV      *hsv,
                                                      LigmaColorSelection *selection);
static void   ligma_color_selection_scales_changed    (LigmaColorSelector  *selector,
                                                      const LigmaRGB      *rgb,
                                                      const LigmaHSV      *hsv,
                                                      LigmaColorSelection *selection);
static void   ligma_color_selection_color_picked      (GtkWidget          *widget,
                                                      const LigmaRGB      *rgb,
                                                      LigmaColorSelection *selection);
static void   ligma_color_selection_entry_changed     (LigmaColorHexEntry  *entry,
                                                      LigmaColorSelection *selection);
static void   ligma_color_selection_channel_changed   (LigmaColorSelector  *selector,
                                                      LigmaColorSelectorChannel channel,
                                                      LigmaColorSelection *selection);
static void   ligma_color_selection_new_color_changed (GtkWidget          *widget,
                                                      LigmaColorSelection *selection);

static void   ligma_color_selection_update            (LigmaColorSelection *selection,
                                                      UpdateType          update);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaColorSelection, ligma_color_selection,
                            GTK_TYPE_BOX)

#define parent_class ligma_color_selection_parent_class

static guint selection_signals[LAST_SIGNAL] = { 0, };


static void
ligma_color_selection_class_init (LigmaColorSelectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_color_selection_set_property;

  klass->color_changed       = NULL;

  g_object_class_install_property (object_class, PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        "Config",
                                                        "The color config used by this color selection",
                                                        LIGMA_TYPE_COLOR_CONFIG,
                                                        G_PARAM_WRITABLE));

  selection_signals[COLOR_CHANGED] =
    g_signal_new ("color-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaColorSelectionClass, color_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "LigmaColorSelection");
}

static void
ligma_color_selection_init (LigmaColorSelection *selection)
{
  LigmaColorSelectionPrivate *priv;
  GtkWidget                 *main_hbox;
  GtkWidget                 *hbox;
  GtkWidget                 *vbox;
  GtkWidget                 *frame;
  GtkWidget                 *label;
  GtkWidget                 *entry;
  GtkWidget                 *button;
  GtkSizeGroup              *new_group;
  GtkSizeGroup              *old_group;

  selection->priv = ligma_color_selection_get_instance_private (selection);

  priv = selection->priv;

  priv->show_alpha = TRUE;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (selection),
                                  GTK_ORIENTATION_VERTICAL);

  ligma_rgba_set (&priv->rgb, 0.0, 0.0, 0.0, 1.0);
  ligma_rgb_to_hsv (&priv->rgb, &priv->hsv);

  priv->channel = LIGMA_COLOR_SELECTOR_RED;

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (selection), main_hbox, TRUE, TRUE, 0);
  gtk_widget_show (main_hbox);

  /*  The left vbox with the notebook  */
  priv->left_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), priv->left_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (priv->left_vbox);

  if (_ligma_ensure_modules_func)
    {
      g_type_class_ref (LIGMA_TYPE_COLOR_SELECT);
      _ligma_ensure_modules_func ();
    }

  priv->notebook = ligma_color_selector_new (LIGMA_TYPE_COLOR_NOTEBOOK,
                                            &priv->rgb,
                                            &priv->hsv,
                                            priv->channel);

  if (_ligma_ensure_modules_func)
    g_type_class_unref (g_type_class_peek (LIGMA_TYPE_COLOR_SELECT));

  ligma_color_selector_set_toggles_visible
    (LIGMA_COLOR_SELECTOR (priv->notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (priv->left_vbox), priv->notebook,
                      TRUE, TRUE, 0);
  gtk_widget_show (priv->notebook);

  g_signal_connect (priv->notebook, "color-changed",
                    G_CALLBACK (ligma_color_selection_notebook_changed),
                    selection);
  g_signal_connect (ligma_color_notebook_get_notebook (LIGMA_COLOR_NOTEBOOK (priv->notebook)),
                    "switch-page",
                    G_CALLBACK (ligma_color_selection_switch_page),
                    selection);

  /*  The hbox for the color_areas  */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_end (GTK_BOX (priv->left_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The labels  */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Current:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  new_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (new_group, label);
  g_object_unref (new_group);

  label = gtk_label_new (_("Old:"));
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  old_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (old_group, label);
  g_object_unref (old_group);

  /*  The color areas  */
  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  priv->new_color = ligma_color_area_new (&priv->rgb,
                                         priv->show_alpha ?
                                         LIGMA_COLOR_AREA_SMALL_CHECKS :
                                         LIGMA_COLOR_AREA_FLAT,
                                         GDK_BUTTON1_MASK |
                                         GDK_BUTTON2_MASK);
  gtk_size_group_add_widget (new_group, priv->new_color);
  gtk_box_pack_start (GTK_BOX (vbox), priv->new_color, FALSE, FALSE, 0);
  gtk_widget_show (priv->new_color);

  g_signal_connect (priv->new_color, "color-changed",
                    G_CALLBACK (ligma_color_selection_new_color_changed),
                    selection);

  priv->old_color = ligma_color_area_new (&priv->rgb,
                                         priv->show_alpha ?
                                         LIGMA_COLOR_AREA_SMALL_CHECKS :
                                         LIGMA_COLOR_AREA_FLAT,
                                         GDK_BUTTON1_MASK |
                                         GDK_BUTTON2_MASK);
  gtk_drag_dest_unset (priv->old_color);
  gtk_size_group_add_widget (old_group, priv->old_color);
  gtk_box_pack_start (GTK_BOX (vbox), priv->old_color, FALSE, FALSE, 0);
  gtk_widget_show (priv->old_color);

  /*  The right vbox with color scales  */
  priv->right_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), priv->right_vbox,
                      TRUE, TRUE, 0);
  gtk_widget_show (priv->right_vbox);

  priv->scales = ligma_color_selector_new (LIGMA_TYPE_COLOR_SCALES,
                                          &priv->rgb,
                                          &priv->hsv,
                                          priv->channel);
  ligma_color_selector_set_toggles_visible
    (LIGMA_COLOR_SELECTOR (priv->scales), TRUE);
  ligma_color_selector_set_show_alpha (LIGMA_COLOR_SELECTOR (priv->scales),
                                      priv->show_alpha);
  gtk_box_pack_start (GTK_BOX (priv->right_vbox), priv->scales,
                      TRUE, TRUE, 0);
  gtk_widget_show (priv->scales);

  g_signal_connect (priv->scales, "channel-changed",
                    G_CALLBACK (ligma_color_selection_channel_changed),
                    selection);
  g_signal_connect (priv->scales, "color-changed",
                    G_CALLBACK (ligma_color_selection_scales_changed),
                    selection);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (priv->right_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /*  The color picker  */
  button = ligma_pick_button_new ();
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "color-picked",
                    G_CALLBACK (ligma_color_selection_color_picked),
                    selection);

  /* The hex triplet entry */
  entry = ligma_color_hex_entry_new ();
  gtk_box_pack_end (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  label = gtk_label_new_with_mnemonic (_("HTML _notation:"));
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  g_object_set_data (G_OBJECT (selection), "color-hex-entry", entry);

  g_signal_connect (entry, "color-changed",
                    G_CALLBACK (ligma_color_selection_entry_changed),
                    selection);
}

static void
ligma_color_selection_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaColorSelection *selection = LIGMA_COLOR_SELECTION (object);

  switch (property_id)
    {
    case PROP_CONFIG:
      ligma_color_selection_set_config (selection, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/**
 * ligma_color_selection_new:
 *
 * Creates a new #LigmaColorSelection widget.
 *
 * Returns: The new #LigmaColorSelection widget.
 **/
GtkWidget *
ligma_color_selection_new (void)
{
  return g_object_new (LIGMA_TYPE_COLOR_SELECTION, NULL);
}

/**
 * ligma_color_selection_set_show_alpha:
 * @selection:  A #LigmaColorSelection widget.
 * @show_alpha: The new @show_alpha setting.
 *
 * Sets the @show_alpha property of the @selection widget.
 **/
void
ligma_color_selection_set_show_alpha (LigmaColorSelection *selection,
                                     gboolean            show_alpha)
{
  LigmaColorSelectionPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTION (selection));

  priv = GET_PRIVATE (selection);

  if (show_alpha != priv->show_alpha)
    {
      priv->show_alpha = show_alpha ? TRUE : FALSE;

      ligma_color_selector_set_show_alpha
        (LIGMA_COLOR_SELECTOR (priv->notebook), priv->show_alpha);
      ligma_color_selector_set_show_alpha
        (LIGMA_COLOR_SELECTOR (priv->scales), priv->show_alpha);

      ligma_color_area_set_type (LIGMA_COLOR_AREA (priv->new_color),
                                priv->show_alpha ?
                                LIGMA_COLOR_AREA_SMALL_CHECKS :
                                LIGMA_COLOR_AREA_FLAT);
      ligma_color_area_set_type (LIGMA_COLOR_AREA (priv->old_color),
                                priv->show_alpha ?
                                LIGMA_COLOR_AREA_SMALL_CHECKS :
                                LIGMA_COLOR_AREA_FLAT);
    }
}

/**
 * ligma_color_selection_get_show_alpha:
 * @selection: A #LigmaColorSelection widget.
 *
 * Returns the @selection's @show_alpha property.
 *
 * Returns: %TRUE if the #LigmaColorSelection has alpha controls.
 **/
gboolean
ligma_color_selection_get_show_alpha (LigmaColorSelection *selection)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_SELECTION (selection), FALSE);

  return GET_PRIVATE (selection)->show_alpha;
}

/**
 * ligma_color_selection_set_color:
 * @selection: A #LigmaColorSelection widget.
 * @color:     The @color to set as current color.
 *
 * Sets the #LigmaColorSelection's current color to the new @color.
 **/
void
ligma_color_selection_set_color (LigmaColorSelection *selection,
                                const LigmaRGB      *color)
{
  LigmaColorSelectionPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (selection);

  priv->rgb = *color;
  ligma_rgb_to_hsv (&priv->rgb, &priv->hsv);

  ligma_color_selection_update (selection, UPDATE_ALL);

  ligma_color_selection_color_changed (selection);
}

/**
 * ligma_color_selection_get_color:
 * @selection: A #LigmaColorSelection widget.
 * @color:     (out caller-allocates): Return location for the
 *             @selection's current @color.
 *
 * This function returns the #LigmaColorSelection's current color.
 **/
void
ligma_color_selection_get_color (LigmaColorSelection *selection,
                                LigmaRGB            *color)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  *color = GET_PRIVATE (selection)->rgb;
}

/**
 * ligma_color_selection_set_old_color:
 * @selection: A #LigmaColorSelection widget.
 * @color:     The @color to set as old color.
 *
 * Sets the #LigmaColorSelection's old color.
 **/
void
ligma_color_selection_set_old_color (LigmaColorSelection *selection,
                                    const LigmaRGB      *color)
{
  LigmaColorSelectionPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (selection);

  ligma_color_area_set_color (LIGMA_COLOR_AREA (priv->old_color), color);
}

/**
 * ligma_color_selection_get_old_color:
 * @selection: A #LigmaColorSelection widget.
 * @color:     (out caller-allocates): Return location for the
 *             @selection's old @color.
 *
 * This function returns the #LigmaColorSelection's old color.
 **/
void
ligma_color_selection_get_old_color (LigmaColorSelection *selection,
                                    LigmaRGB            *color)
{
  LigmaColorSelectionPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTION (selection));
  g_return_if_fail (color != NULL);

  priv = GET_PRIVATE (selection);

  ligma_color_area_get_color (LIGMA_COLOR_AREA (priv->old_color), color);
}

/**
 * ligma_color_selection_reset:
 * @selection: A #LigmaColorSelection widget.
 *
 * Sets the #LigmaColorSelection's current color to its old color.
 **/
void
ligma_color_selection_reset (LigmaColorSelection *selection)
{
  LigmaColorSelectionPrivate *priv;
  LigmaRGB                    color;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTION (selection));

  priv = GET_PRIVATE (selection);

  ligma_color_area_get_color (LIGMA_COLOR_AREA (priv->old_color), &color);
  ligma_color_selection_set_color (selection, &color);
}

/**
 * ligma_color_selection_color_changed:
 * @selection: A #LigmaColorSelection widget.
 *
 * Emits the "color-changed" signal.
 **/
void
ligma_color_selection_color_changed (LigmaColorSelection *selection)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTION (selection));

  g_signal_emit (selection, selection_signals[COLOR_CHANGED], 0);
}

/**
 * ligma_color_selection_set_simulation:
 * @selection: A #LigmaColorSelection widget.
 * @profile:   A #LigmaColorProfile object.
 * @intent:    A #LigmaColorRenderingIntent enum.
 * @bpc:       A gboolean.
 *
 * Sets the simulation options to use with this color selection.
 *
 * Since: 3.0
 */
void
ligma_color_selection_set_simulation (LigmaColorSelection *selection,
                                     LigmaColorProfile   *profile,
                                     LigmaColorRenderingIntent intent,
                                     gboolean            bpc)
{
  g_return_if_fail (LIGMA_IS_COLOR_SELECTION (selection));

  ligma_color_notebook_set_simulation (LIGMA_COLOR_NOTEBOOK (selection->priv->notebook),
                                      profile,
                                      intent,
                                      bpc);

  g_signal_emit (selection, selection_signals[COLOR_CHANGED], 0);
}

/**
 * ligma_color_selection_set_config:
 * @selection: A #LigmaColorSelection widget.
 * @config:    A #LigmaColorConfig object.
 *
 * Sets the color management configuration to use with this color selection.
 *
 * Since: 2.4
 */
void
ligma_color_selection_set_config (LigmaColorSelection *selection,
                                 LigmaColorConfig    *config)
{
  LigmaColorSelectionPrivate *priv;

  g_return_if_fail (LIGMA_IS_COLOR_SELECTION (selection));
  g_return_if_fail (config == NULL || LIGMA_IS_COLOR_CONFIG (config));

  priv = GET_PRIVATE (selection);

  ligma_color_selector_set_config (LIGMA_COLOR_SELECTOR (priv->notebook),
                                  config);
  ligma_color_selector_set_config (LIGMA_COLOR_SELECTOR (priv->scales),
                                  config);
  ligma_color_area_set_color_config (LIGMA_COLOR_AREA (priv->old_color),
                                    config);
  ligma_color_area_set_color_config (LIGMA_COLOR_AREA (priv->new_color),
                                    config);
}

/**
 * ligma_color_selection_get_notebook:
 * @selection: A #LigmaColorSelection widget.
 *
 * Returns: (transfer none): The selection's #LigmaColorNotebook.
 *
 * Since: 3.0
 */
GtkWidget *
ligma_color_selection_get_notebook (LigmaColorSelection *selection)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_SELECTION (selection), NULL);

  return GET_PRIVATE (selection)->notebook;
}

/**
 * ligma_color_selection_get_right_vbox:
 * @selection: A #LigmaColorSelection widget.
 *
 * Returns: (transfer none) (type GtkBox): The selection's right #GtkBox which
 *          contains the color scales.
 *
 * Since: 3.0
 */
GtkWidget *
ligma_color_selection_get_right_vbox (LigmaColorSelection *selection)
{
  g_return_val_if_fail (LIGMA_IS_COLOR_SELECTION (selection), NULL);

  return GET_PRIVATE (selection)->right_vbox;
}


/*  private functions  */

static void
ligma_color_selection_switch_page (GtkWidget          *widget,
                                  gpointer            page,
                                  guint               page_num,
                                  LigmaColorSelection *selection)
{
  LigmaColorSelectionPrivate *priv     = GET_PRIVATE (selection);
  LigmaColorNotebook         *notebook = LIGMA_COLOR_NOTEBOOK (priv->notebook);
  LigmaColorSelector         *current;
  gboolean                   sensitive;

  current = ligma_color_notebook_get_current_selector (notebook);

  sensitive = (LIGMA_COLOR_SELECTOR_GET_CLASS (current)->set_channel != NULL);

  ligma_color_selector_set_toggles_sensitive
    (LIGMA_COLOR_SELECTOR (priv->scales), sensitive);
}

static void
ligma_color_selection_notebook_changed (LigmaColorSelector  *selector,
                                       const LigmaRGB      *rgb,
                                       const LigmaHSV      *hsv,
                                       LigmaColorSelection *selection)
{
  LigmaColorSelectionPrivate *priv = GET_PRIVATE (selection);

  priv->hsv = *hsv;
  priv->rgb = *rgb;

  ligma_color_selection_update (selection,
                               UPDATE_SCALES | UPDATE_ENTRY | UPDATE_COLOR);
  ligma_color_selection_color_changed (selection);
}

static void
ligma_color_selection_scales_changed (LigmaColorSelector  *selector,
                                     const LigmaRGB      *rgb,
                                     const LigmaHSV      *hsv,
                                     LigmaColorSelection *selection)
{
  LigmaColorSelectionPrivate *priv = GET_PRIVATE (selection);

  priv->rgb = *rgb;
  priv->hsv = *hsv;

  ligma_color_selection_update (selection,
                               UPDATE_ENTRY | UPDATE_NOTEBOOK | UPDATE_COLOR);
  ligma_color_selection_color_changed (selection);
}

static void
ligma_color_selection_color_picked (GtkWidget          *widget,
                                   const LigmaRGB      *rgb,
                                   LigmaColorSelection *selection)
{
  ligma_color_selection_set_color (selection, rgb);
}

static void
ligma_color_selection_entry_changed (LigmaColorHexEntry  *entry,
                                    LigmaColorSelection *selection)
{
  LigmaColorSelectionPrivate *priv = GET_PRIVATE (selection);

  ligma_color_hex_entry_get_color (entry, &priv->rgb);

  ligma_rgb_to_hsv (&priv->rgb, &priv->hsv);

  ligma_color_selection_update (selection,
                               UPDATE_NOTEBOOK | UPDATE_SCALES | UPDATE_COLOR);
  ligma_color_selection_color_changed (selection);
}

static void
ligma_color_selection_channel_changed (LigmaColorSelector        *selector,
                                      LigmaColorSelectorChannel  channel,
                                      LigmaColorSelection       *selection)
{
  LigmaColorSelectionPrivate *priv = GET_PRIVATE (selection);

  priv->channel = channel;

  ligma_color_selector_set_channel (LIGMA_COLOR_SELECTOR (priv->notebook),
                                   priv->channel);
}

static void
ligma_color_selection_new_color_changed (GtkWidget          *widget,
                                        LigmaColorSelection *selection)
{
  LigmaColorSelectionPrivate *priv = GET_PRIVATE (selection);

  ligma_color_area_get_color (LIGMA_COLOR_AREA (widget), &priv->rgb);
  ligma_rgb_to_hsv (&priv->rgb, &priv->hsv);

  ligma_color_selection_update (selection,
                               UPDATE_NOTEBOOK | UPDATE_SCALES | UPDATE_ENTRY);
  ligma_color_selection_color_changed (selection);
}

static void
ligma_color_selection_update (LigmaColorSelection *selection,
                             UpdateType          update)
{
  LigmaColorSelectionPrivate *priv = GET_PRIVATE (selection);

  if (update & UPDATE_NOTEBOOK)
    {
      g_signal_handlers_block_by_func (priv->notebook,
                                       ligma_color_selection_notebook_changed,
                                       selection);

      ligma_color_selector_set_color (LIGMA_COLOR_SELECTOR (priv->notebook),
                                     &priv->rgb,
                                     &priv->hsv);

      g_signal_handlers_unblock_by_func (priv->notebook,
                                         ligma_color_selection_notebook_changed,
                                         selection);
    }

  if (update & UPDATE_SCALES)
    {
      g_signal_handlers_block_by_func (priv->scales,
                                       ligma_color_selection_scales_changed,
                                       selection);

      ligma_color_selector_set_color (LIGMA_COLOR_SELECTOR (priv->scales),
                                     &priv->rgb,
                                     &priv->hsv);

      g_signal_handlers_unblock_by_func (priv->scales,
                                         ligma_color_selection_scales_changed,
                                         selection);
    }

  if (update & UPDATE_ENTRY)
    {
      LigmaColorHexEntry *entry;

      entry = g_object_get_data (G_OBJECT (selection), "color-hex-entry");

      g_signal_handlers_block_by_func (entry,
                                       ligma_color_selection_entry_changed,
                                       selection);

      ligma_color_hex_entry_set_color (entry, &priv->rgb);

      g_signal_handlers_unblock_by_func (entry,
                                         ligma_color_selection_entry_changed,
                                         selection);
    }

  if (update & UPDATE_COLOR)
    {
      g_signal_handlers_block_by_func (priv->new_color,
                                       ligma_color_selection_new_color_changed,
                                       selection);

      ligma_color_area_set_color (LIGMA_COLOR_AREA (priv->new_color),
                                 &priv->rgb);

      g_signal_handlers_unblock_by_func (priv->new_color,
                                         ligma_color_selection_new_color_changed,
                                         selection);
    }
}
