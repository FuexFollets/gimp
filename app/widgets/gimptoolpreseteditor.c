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

#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatooloptions.h"
#include "core/ligmatoolpreset.h"

#include "ligmadocked.h"
#include "ligmatoolpreseteditor.h"
#include "ligmamenufactory.h"
#include "ligmapropwidgets.h"

#include "ligma-intl.h"


struct _LigmaToolPresetEditorPrivate
{
  LigmaToolPreset *tool_preset_model;

  GtkWidget      *tool_icon;
  GtkWidget      *tool_label;

  GtkWidget      *fg_bg_toggle;
  GtkWidget      *opacity_paint_mode_toggle;
  GtkWidget      *brush_toggle;
  GtkWidget      *dynamics_toggle;
  GtkWidget      *mybrush_toggle;
  GtkWidget      *gradient_toggle;
  GtkWidget      *pattern_toggle;
  GtkWidget      *palette_toggle;
  GtkWidget      *font_toggle;
};


/*  local function prototypes  */

static void   ligma_tool_preset_editor_constructed  (GObject              *object);
static void   ligma_tool_preset_editor_finalize     (GObject              *object);

static void   ligma_tool_preset_editor_set_data     (LigmaDataEditor       *editor,
                                                    LigmaData             *data);

static void   ligma_tool_preset_editor_sync_data    (LigmaToolPresetEditor *editor);
static void   ligma_tool_preset_editor_notify_model (LigmaToolPreset       *options,
                                                    const GParamSpec     *pspec,
                                                    LigmaToolPresetEditor *editor);
static void   ligma_tool_preset_editor_notify_data  (LigmaToolPreset       *options,
                                                    const GParamSpec     *pspec,
                                                    LigmaToolPresetEditor *editor);



G_DEFINE_TYPE_WITH_CODE (LigmaToolPresetEditor, ligma_tool_preset_editor,
                         LIGMA_TYPE_DATA_EDITOR,
                         G_ADD_PRIVATE (LigmaToolPresetEditor)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_DOCKED, NULL))

#define parent_class ligma_tool_preset_editor_parent_class


static void
ligma_tool_preset_editor_class_init (LigmaToolPresetEditorClass *klass)
{
  GObjectClass        *object_class = G_OBJECT_CLASS (klass);
  LigmaDataEditorClass *editor_class = LIGMA_DATA_EDITOR_CLASS (klass);

  object_class->constructed = ligma_tool_preset_editor_constructed;
  object_class->finalize    = ligma_tool_preset_editor_finalize;

  editor_class->set_data    = ligma_tool_preset_editor_set_data;
  editor_class->title       = _("Tool Preset Editor");
}

static void
ligma_tool_preset_editor_init (LigmaToolPresetEditor *editor)
{
  editor->priv = ligma_tool_preset_editor_get_instance_private (editor);
}

static void
ligma_tool_preset_editor_constructed (GObject *object)
{
  LigmaToolPresetEditor *editor      = LIGMA_TOOL_PRESET_EDITOR (object);
  LigmaDataEditor       *data_editor = LIGMA_DATA_EDITOR (editor);
  LigmaToolPreset       *preset;
  GtkWidget            *hbox;
  GtkWidget            *label;
  GtkWidget            *button;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  preset = editor->priv->tool_preset_model =
    g_object_new (LIGMA_TYPE_TOOL_PRESET,
                  "ligma", data_editor->context->ligma,
                  NULL);

  g_signal_connect (preset, "notify",
                    G_CALLBACK (ligma_tool_preset_editor_notify_model),
                    editor);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (data_editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  editor->priv->tool_icon = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (hbox), editor->priv->tool_icon,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->priv->tool_icon);

  editor->priv->tool_label = gtk_label_new ("");
  ligma_label_set_attributes (GTK_LABEL (editor->priv->tool_label),
                             PANGO_ATTR_STYLE, PANGO_STYLE_ITALIC,
                             -1);
  gtk_box_pack_start (GTK_BOX (hbox), editor->priv->tool_label,
                      FALSE, FALSE, 0);
  gtk_widget_show (editor->priv->tool_label);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_box_pack_start (GTK_BOX (data_editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Icon:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  button = ligma_prop_icon_picker_new (LIGMA_VIEWABLE (preset),
                                      data_editor->context->ligma);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

  button = editor->priv->fg_bg_toggle =
    ligma_prop_check_button_new (G_OBJECT (preset), "use-fg-bg", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->opacity_paint_mode_toggle =
    ligma_prop_check_button_new (G_OBJECT (preset), "use-opacity-paint-mode", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->brush_toggle =
    ligma_prop_check_button_new (G_OBJECT (preset), "use-brush", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->dynamics_toggle =
    ligma_prop_check_button_new (G_OBJECT (preset), "use-dynamics", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->mybrush_toggle =
    ligma_prop_check_button_new (G_OBJECT (preset), "use-mypaint-brush", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->gradient_toggle =
    ligma_prop_check_button_new (G_OBJECT (preset), "use-gradient", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->pattern_toggle =
    ligma_prop_check_button_new (G_OBJECT (preset), "use-pattern", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->palette_toggle =
    ligma_prop_check_button_new (G_OBJECT (preset), "use-palette", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = editor->priv->font_toggle =
    ligma_prop_check_button_new (G_OBJECT (preset), "use-font", NULL);
  gtk_box_pack_start (GTK_BOX (data_editor), button, FALSE, FALSE, 0);

  button = ligma_editor_add_action_button (LIGMA_EDITOR (editor),
                                          "tool-preset-editor",
                                          "tool-preset-editor-save", NULL);

  button = ligma_editor_add_action_button (LIGMA_EDITOR (editor),
                                          "tool-preset-editor",
                                          "tool-preset-editor-restore", NULL);

  if (data_editor->data)
    ligma_tool_preset_editor_sync_data (editor);
}

static void
ligma_tool_preset_editor_finalize (GObject *object)
{
  LigmaToolPresetEditor *editor = LIGMA_TOOL_PRESET_EDITOR (object);

  g_clear_object (&editor->priv->tool_preset_model);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_tool_preset_editor_set_data (LigmaDataEditor *editor,
                                  LigmaData       *data)
{
  LigmaToolPresetEditor *preset_editor = LIGMA_TOOL_PRESET_EDITOR (editor);

  if (editor->data)
    g_signal_handlers_disconnect_by_func (editor->data,
                                          ligma_tool_preset_editor_notify_data,
                                          editor);

  LIGMA_DATA_EDITOR_CLASS (parent_class)->set_data (editor, data);

  if (editor->data)
    {
      g_signal_connect (editor->data, "notify",
                        G_CALLBACK (ligma_tool_preset_editor_notify_data),
                        editor);

      if (preset_editor->priv->tool_preset_model)
        ligma_tool_preset_editor_sync_data (preset_editor);
    }

  gtk_widget_set_sensitive (GTK_WIDGET (editor), editor->data_editable);
}


/*  public functions  */

GtkWidget *
ligma_tool_preset_editor_new (LigmaContext     *context,
                             LigmaMenuFactory *menu_factory)
{
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return g_object_new (LIGMA_TYPE_TOOL_PRESET_EDITOR,
                       "menu-factory",    menu_factory,
                       "menu-identifier", "<ToolPresetEditor>",
                       "ui-path",         "/tool-preset-editor-popup",
                       "data-factory",    context->ligma->tool_preset_factory,
                       "context",         context,
                       "data",            ligma_context_get_tool_preset (context),
                       NULL);
}


/*  private functions  */

static void
ligma_tool_preset_editor_sync_data (LigmaToolPresetEditor *editor)
{
  LigmaToolPresetEditorPrivate *priv        = editor->priv;
  LigmaDataEditor              *data_editor = LIGMA_DATA_EDITOR (editor);
  LigmaToolPreset              *preset;
  LigmaToolInfo                *tool_info;
  LigmaContextPropMask          serialize_props;
  const gchar                 *icon_name;
  gchar                       *label;

  g_signal_handlers_block_by_func (priv->tool_preset_model,
                                   ligma_tool_preset_editor_notify_model,
                                   editor);

  ligma_config_sync (G_OBJECT (data_editor->data),
                    G_OBJECT (priv->tool_preset_model),
                    LIGMA_CONFIG_PARAM_SERIALIZE);

  g_signal_handlers_unblock_by_func (priv->tool_preset_model,
                                     ligma_tool_preset_editor_notify_model,
                                     editor);

  if (! priv->tool_preset_model->tool_options)
    return;

  tool_info = priv->tool_preset_model->tool_options->tool_info;

  icon_name = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool_info));
  label     = g_strdup_printf (_("%s Preset"), tool_info->label);

  gtk_image_set_from_icon_name (GTK_IMAGE (priv->tool_icon),
                                icon_name, GTK_ICON_SIZE_MENU);
  gtk_label_set_text (GTK_LABEL (priv->tool_label), label);

  g_free (label);

  preset = LIGMA_TOOL_PRESET (data_editor->data);

  serialize_props =
    ligma_context_get_serialize_properties (LIGMA_CONTEXT (preset->tool_options));

  gtk_widget_set_sensitive (priv->fg_bg_toggle,
                            (serialize_props &
                             (LIGMA_CONTEXT_PROP_MASK_FOREGROUND |
                              LIGMA_CONTEXT_PROP_MASK_BACKGROUND)) != 0);
  gtk_widget_set_sensitive (priv->opacity_paint_mode_toggle,
                            (serialize_props &
                             (LIGMA_CONTEXT_PROP_MASK_OPACITY |
                              LIGMA_CONTEXT_PROP_MASK_PAINT_MODE)) != 0);
  gtk_widget_set_sensitive (priv->brush_toggle,
                            (serialize_props &
                             LIGMA_CONTEXT_PROP_MASK_BRUSH) != 0);
  gtk_widget_set_sensitive (priv->dynamics_toggle,
                            (serialize_props &
                             LIGMA_CONTEXT_PROP_MASK_DYNAMICS) != 0);
  gtk_widget_set_sensitive (priv->mybrush_toggle,
                            (serialize_props &
                             LIGMA_CONTEXT_PROP_MASK_MYBRUSH) != 0);
  gtk_widget_set_sensitive (priv->gradient_toggle,
                            (serialize_props &
                             LIGMA_CONTEXT_PROP_MASK_GRADIENT) != 0);
  gtk_widget_set_sensitive (priv->pattern_toggle,
                            (serialize_props &
                             LIGMA_CONTEXT_PROP_MASK_PATTERN) != 0);
  gtk_widget_set_sensitive (priv->palette_toggle,
                            (serialize_props &
                             LIGMA_CONTEXT_PROP_MASK_PALETTE) != 0);
  gtk_widget_set_sensitive (priv->font_toggle,
                            (serialize_props &
                             LIGMA_CONTEXT_PROP_MASK_FONT) != 0);
 }

static void
ligma_tool_preset_editor_notify_model (LigmaToolPreset       *options,
                                      const GParamSpec     *pspec,
                                      LigmaToolPresetEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);

  if (data_editor->data)
    {
      g_signal_handlers_block_by_func (data_editor->data,
                                       ligma_tool_preset_editor_notify_data,
                                       editor);

      ligma_config_sync (G_OBJECT (editor->priv->tool_preset_model),
                        G_OBJECT (data_editor->data),
                        LIGMA_CONFIG_PARAM_SERIALIZE);

      g_signal_handlers_unblock_by_func (data_editor->data,
                                         ligma_tool_preset_editor_notify_data,
                                         editor);
    }
}

static void
ligma_tool_preset_editor_notify_data (LigmaToolPreset       *options,
                                     const GParamSpec     *pspec,
                                     LigmaToolPresetEditor *editor)
{
  LigmaDataEditor *data_editor = LIGMA_DATA_EDITOR (editor);

  g_signal_handlers_block_by_func (editor->priv->tool_preset_model,
                                   ligma_tool_preset_editor_notify_model,
                                   editor);

  ligma_config_sync (G_OBJECT (data_editor->data),
                    G_OBJECT (editor->priv->tool_preset_model),
                    LIGMA_CONFIG_PARAM_SERIALIZE);

  g_signal_handlers_unblock_by_func (editor->priv->tool_preset_model,
                                     ligma_tool_preset_editor_notify_model,
                                     editor);
}
