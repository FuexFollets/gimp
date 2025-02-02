/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamodifierseditor.c
 * Copyright (C) 2022 Jehan
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
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "display/display-types.h"
#include "display/ligmamodifiersmanager.h"

#include "ligmaaction.h"
#include "ligmaactionview.h"
#include "ligmaactioneditor.h"
#include "ligmahelp-ids.h"
#include "ligmamodifierseditor.h"
#include "ligmashortcutbutton.h"
#include "ligmauimanager.h"
#include "ligmawidgets-utils.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_MANAGER,
};

struct _LigmaModifiersEditorPrivate
{
  GdkDevice            *device;
  guint                 button;

  GtkWidget            *header;
  GtkWidget            *warning;
  GtkWidget            *select_button;

  GtkWidget            *stack;
  GtkWidget            *current_settings;
  GtkWidget            *plus_button;

  GtkSizeGroup         *mod_size_group;
  GtkSizeGroup         *action_size_group;
  GtkSizeGroup         *action_action_size_group;
  GtkSizeGroup         *minus_size_group;

  LigmaModifiersManager *manager;
  GHashTable           *rows;

  GtkTreeSelection     *action_selection;
};


static void     ligma_modifiers_editor_constructed          (GObject             *object);
static void     ligma_modifiers_editor_finalize             (GObject             *object);
static void     ligma_modifiers_editor_set_property         (GObject             *object,
                                                            guint                property_id,
                                                            const GValue        *value,
                                                            GParamSpec          *pspec);
static void     ligma_modifiers_editor_get_property         (GObject             *object,
                                                            guint                property_id,
                                                            GValue              *value,
                                                            GParamSpec          *pspec);

static gboolean ligma_modifiers_editor_button_press_event   (GtkWidget           *editor,
                                                            GdkEventButton      *event,
                                                            gpointer             user_data);
static void     ligma_modifiers_editor_plus_button_clicked  (GtkButton           *button,
                                                            LigmaModifiersEditor *editor);
static void     ligma_modifiers_editor_minus_button_clicked (GtkButton           *minus_button,
                                                            LigmaModifiersEditor *editor);
static void     ligma_modifiers_editor_notify_accelerator   (GtkWidget           *widget,
                                                            const GParamSpec    *pspec,
                                                            LigmaModifiersEditor *editor);
static void     ligma_modifiers_editor_search_clicked       (GtkWidget           *button,
                                                            LigmaModifiersEditor *editor);

static void     ligma_modifiers_editor_show_settings         (LigmaModifiersEditor *editor,
                                                             GdkDevice           *device,
                                                             guint                button);
static void     ligma_modifiers_editor_add_mapping           (LigmaModifiersEditor *editor,
                                                             GdkModifierType      modifiers,
                                                             LigmaModifierAction   mod_action,
                                                             const gchar         *action_desc);

static void     ligma_controller_modifiers_action_activated  (GtkTreeView         *tv,
                                                             GtkTreePath         *path,
                                                             GtkTreeViewColumn   *column,
                                                             GtkWidget           *edit_dialog);
static void     ligma_modifiers_editor_search_response       (GtkWidget           *dialog,
                                                             gint                 response_id,
                                                             LigmaModifiersEditor *editor);

static gchar  * ligma_modifiers_editor_make_hash_key         (GdkDevice           *device,
                                                             guint                button,
                                                             GdkModifierType      modifiers);
static void     ligma_modifiers_editor_update_rows           (LigmaModifiersEditor *editor,
                                                             GdkModifierType      modifiers,
                                                             GtkWidget           *box_child);
static gboolean ligma_modifiers_editor_search_row            (gpointer             key,
                                                             gpointer             value,
                                                             gpointer             user_data);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaModifiersEditor, ligma_modifiers_editor,
                            LIGMA_TYPE_FRAME)

#define parent_class ligma_modifiers_editor_parent_class


static void
ligma_modifiers_editor_class_init (LigmaModifiersEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_modifiers_editor_constructed;
  object_class->finalize     = ligma_modifiers_editor_finalize;
  object_class->get_property = ligma_modifiers_editor_get_property;
  object_class->set_property = ligma_modifiers_editor_set_property;

  g_object_class_install_property (object_class, PROP_MANAGER,
                                   g_param_spec_object ("manager", NULL, NULL,
                                                        LIGMA_TYPE_MODIFIERS_MANAGER,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_modifiers_editor_init (LigmaModifiersEditor *editor)
{
  GtkWidget *grid;
  GtkWidget *hbox;
  GtkWidget *hint;
  GtkWidget *image;
  gchar     *text;

  editor->priv = ligma_modifiers_editor_get_instance_private (editor);
  editor->priv->device                   = NULL;
  editor->priv->plus_button              = NULL;
  editor->priv->current_settings         = NULL;
  editor->priv->mod_size_group           = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  editor->priv->action_size_group        = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  editor->priv->action_action_size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  editor->priv->minus_size_group         = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  editor->priv->rows                     = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  /* Setup the title. */
  gtk_frame_set_label_align (GTK_FRAME (editor), 0.5, 0.5);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 4);

  editor->priv->header = gtk_label_new (NULL);
  gtk_grid_attach (GTK_GRID (grid), editor->priv->header, 0, 0, 2, 1);
  gtk_widget_show (editor->priv->header);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_show (hbox);

  hint = gtk_label_new (NULL);
  text = g_strdup_printf ("<i>%s</i>", _("Click here to set a button's modifiers"));
  gtk_label_set_markup (GTK_LABEL (hint), text);
  g_free (text);
  gtk_box_pack_start (GTK_BOX (hbox), hint, TRUE, TRUE, 2);
  gtk_widget_show (hint);

  image = gtk_image_new_from_icon_name ("ligma-cursor", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 2);
  gtk_widget_show (image);

  editor->priv->warning = gtk_label_new (NULL);
  text = g_strdup_printf ("<b>%s</b>",
                          _("Modifiers cannot be customized on the primary button."));
  gtk_label_set_markup (GTK_LABEL (editor->priv->warning), text);
  g_free (text);
  gtk_grid_attach (GTK_GRID (grid), editor->priv->warning, 0, 2, 2, 1);
  gtk_widget_hide (editor->priv->warning);

  editor->priv->select_button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (editor->priv->select_button), hbox);
  gtk_grid_attach (GTK_GRID (grid), editor->priv->select_button, 0, 1, 1, 1);
  gtk_widget_show (editor->priv->select_button);

  gtk_frame_set_label_widget (GTK_FRAME (editor), grid);
  gtk_widget_show (grid);

  /* Setup the stack. */
  editor->priv->stack = gtk_stack_new ();
  gtk_container_add (GTK_CONTAINER (editor), editor->priv->stack);
  gtk_widget_show (editor->priv->stack);
}

static void
ligma_modifiers_editor_constructed (GObject *object)
{
  LigmaModifiersEditor *editor = LIGMA_MODIFIERS_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_signal_connect (editor->priv->select_button, "button-press-event",
                    G_CALLBACK (ligma_modifiers_editor_button_press_event),
                    object);
}

static void
ligma_modifiers_editor_finalize (GObject *object)
{
  LigmaModifiersEditor *editor = LIGMA_MODIFIERS_EDITOR (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);

  g_clear_object (&editor->priv->device);
  g_object_unref (editor->priv->mod_size_group);
  g_object_unref (editor->priv->action_size_group);
  g_object_unref (editor->priv->action_action_size_group);
  g_object_unref (editor->priv->minus_size_group);

  g_hash_table_unref (editor->priv->rows);
}

static void
ligma_modifiers_editor_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  LigmaModifiersEditor *editor = LIGMA_MODIFIERS_EDITOR (object);

  switch (property_id)
    {
    case PROP_MANAGER:
      editor->priv->manager = g_value_get_object (value);
      break;


   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_modifiers_editor_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  LigmaModifiersEditor *editor = LIGMA_MODIFIERS_EDITOR (object);

  switch (property_id)
    {
    case PROP_MANAGER:
      g_value_set_object (value, editor->priv->manager);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/*  public functions  */

GtkWidget *
ligma_modifiers_editor_new (LigmaModifiersManager *manager)
{
  LigmaModifiersEditor *editor;

  g_return_val_if_fail (LIGMA_IS_MODIFIERS_MANAGER (manager), NULL);

  editor = g_object_new (LIGMA_TYPE_MODIFIERS_EDITOR,
                         "manager", manager,
                         NULL);

  return GTK_WIDGET (editor);
}

void
ligma_modifiers_editor_clear (LigmaModifiersEditor *editor)
{
  ligma_modifiers_manager_clear (editor->priv->manager);
  g_hash_table_remove_all (editor->priv->rows);
  gtk_container_foreach (GTK_CONTAINER (editor->priv->stack),
                         (GtkCallback) gtk_widget_destroy,
                         NULL);
  ligma_modifiers_editor_show_settings (editor, editor->priv->device, editor->priv->button);
}

/*  private functions  */

static void
ligma_modifiers_editor_show_settings (LigmaModifiersEditor *editor,
                                     GdkDevice           *device,
                                     guint                button)
{
  const gchar *vendor_id;
  const gchar *product_id;
  gchar       *title;
  gchar       *text;

  vendor_id  = gdk_device_get_vendor_id (device);
  product_id = gdk_device_get_product_id (device);

  if (device != editor->priv->device)
    {
      g_clear_object (&editor->priv->device);
      editor->priv->device = g_object_ref (device);
    }
  editor->priv->button = button;

  /* Update header. */
  if (gdk_device_get_name (device) != NULL)
    text = g_strdup_printf (_("Editing modifiers for button %d of %s"),
                            editor->priv->button,
                            gdk_device_get_name (device));
  else
    text = g_strdup_printf (_("Editing modifiers for button %d"),
                            editor->priv->button);

  title = g_strdup_printf ("<b><big>%s</big></b>", text);
  gtk_label_set_markup (GTK_LABEL (editor->priv->header), title);

  g_free (title);
  g_free (text);

  /* Update modifier settings. */
  text = g_strdup_printf ("%s:%s-%d",
                          vendor_id ? vendor_id : "*",
                          product_id ? product_id : "*",
                          button);
  editor->priv->current_settings = gtk_stack_get_child_by_name (GTK_STACK (editor->priv->stack), text);

  if (! editor->priv->current_settings)
    {
      GtkWidget *plus_button;
      GList     *modifiers;
      GList     *iter;

      editor->priv->current_settings = gtk_list_box_new ();
      gtk_stack_add_named (GTK_STACK (editor->priv->stack), editor->priv->current_settings, text);

      modifiers = ligma_modifiers_manager_get_modifiers (editor->priv->manager,
                                                        device, editor->priv->button);
      for (iter = modifiers; iter; iter = iter->next)
        {
          GdkModifierType     mods = GPOINTER_TO_INT (iter->data);
          LigmaModifierAction  action;
          const gchar        *action_desc = NULL;

          action = ligma_modifiers_manager_get_action (editor->priv->manager, device,
                                                      editor->priv->button, mods,
                                                      &action_desc);
          ligma_modifiers_editor_add_mapping (editor, mods, action, action_desc);
        }

      plus_button = gtk_button_new_from_icon_name ("list-add", GTK_ICON_SIZE_LARGE_TOOLBAR);
      gtk_list_box_insert (GTK_LIST_BOX (editor->priv->current_settings), plus_button, -1);
      gtk_widget_show (plus_button);

      g_signal_connect (plus_button, "clicked",
                        G_CALLBACK (ligma_modifiers_editor_plus_button_clicked),
                        editor);
      g_object_set_data (G_OBJECT (editor->priv->current_settings), "plus-button", plus_button);

      if (g_list_length (modifiers) == 0)
        ligma_modifiers_editor_plus_button_clicked (GTK_BUTTON (plus_button), editor);

      gtk_widget_show (editor->priv->current_settings);
      g_list_free (modifiers);
    }

  gtk_stack_set_visible_child (GTK_STACK (editor->priv->stack), editor->priv->current_settings);

  g_free (text);
}

static gboolean
ligma_modifiers_editor_button_press_event (GtkWidget      *widget,
                                          GdkEventButton *event,
                                          gpointer        user_data)
{
  LigmaModifiersEditor *editor = LIGMA_MODIFIERS_EDITOR (user_data);
  GdkDevice           *device = gdk_event_get_source_device ((GdkEvent *) event);

  /* Update warning. */
  if (event->button == GDK_BUTTON_PRIMARY)
    {
      gtk_widget_show (editor->priv->warning);
      ligma_widget_blink (editor->priv->warning);
    }
  else
    {
      gtk_widget_hide (editor->priv->warning);
    }

  if (event->button != GDK_BUTTON_PRIMARY &&
      (event->button != editor->priv->button                       ||
       editor->priv->device == NULL                                ||
       g_strcmp0 (gdk_device_get_vendor_id (editor->priv->device),
                  gdk_device_get_vendor_id (device)) != 0          ||
       g_strcmp0 (gdk_device_get_product_id (editor->priv->device),
                  gdk_device_get_product_id (device)) != 0))
    {
      ligma_modifiers_editor_show_settings (editor, device, event->button);
    }

  return FALSE;
}

static void
ligma_modifiers_editor_add_mapping (LigmaModifiersEditor *editor,
                                   GdkModifierType      modifiers,
                                   LigmaModifierAction   mod_action,
                                   const gchar         *action_desc)
{
  GtkWidget   *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  GtkWidget   *box_row;
  GtkWidget   *combo_action_box;
  GtkWidget   *combo;
  GtkWidget   *shortcut;
  GtkWidget   *minus_button;
  GtkWidget   *action_button = NULL;
  GtkWidget   *plus_button;

  plus_button = g_object_get_data (G_OBJECT (editor->priv->current_settings), "plus-button");

  shortcut = ligma_shortcut_button_new (NULL);
  ligma_shortcut_button_accepts_modifier (LIGMA_SHORTCUT_BUTTON (shortcut),
                                         TRUE, FALSE);
  ligma_shortcut_button_set_accelerator (LIGMA_SHORTCUT_BUTTON (shortcut), NULL, 0, modifiers);
  gtk_box_pack_start (GTK_BOX (box), shortcut, FALSE, FALSE, 0);
  gtk_size_group_add_widget (editor->priv->mod_size_group, shortcut);
  gtk_widget_show (shortcut);

  combo_action_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_start (GTK_BOX (box), combo_action_box, FALSE, FALSE, 0);
  gtk_size_group_add_widget (editor->priv->action_action_size_group, combo_action_box);
  gtk_widget_show (combo_action_box);

  combo = ligma_enum_combo_box_new (LIGMA_TYPE_MODIFIER_ACTION);
  gtk_box_pack_start (GTK_BOX (combo_action_box), combo, FALSE, FALSE, 0);
  gtk_combo_box_set_active (GTK_COMBO_BOX (combo), mod_action);
  gtk_size_group_add_widget (editor->priv->action_size_group, combo);
  gtk_widget_show (combo);

  if (action_desc)
    {
      gchar *action_name = strchr (action_desc, '/');

      if (action_name)
        action_name++;

      if (strlen (action_name) > 0)
        action_button = gtk_button_new_with_label (action_name);
    }

  if (action_button == NULL)
    action_button = gtk_button_new_from_icon_name ("system-search", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_box_pack_start (GTK_BOX (combo_action_box), action_button, FALSE, FALSE, 0);
  gtk_widget_set_visible (action_button, mod_action == LIGMA_MODIFIER_ACTION_ACTION);

  minus_button = gtk_button_new_from_icon_name ("list-remove", GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_size_group_add_widget (editor->priv->minus_size_group, minus_button);
  gtk_box_pack_start (GTK_BOX (box), minus_button, FALSE, FALSE, 0);

  g_object_set_data (G_OBJECT (minus_button), "shortcut-button", shortcut);
  g_signal_connect (minus_button, "clicked",
                    G_CALLBACK (ligma_modifiers_editor_minus_button_clicked),
                    editor);
  gtk_widget_show (minus_button);

  g_object_set_data (G_OBJECT (shortcut), "shortcut-modifiers", GINT_TO_POINTER (modifiers));
  g_object_set_data (G_OBJECT (shortcut), "shortcut-button", shortcut);
  g_object_set_data (G_OBJECT (shortcut), "shortcut-action", combo);
  g_object_set_data (G_OBJECT (shortcut), "shortcut-action-action", action_button);
  g_object_set_data (G_OBJECT (combo),    "shortcut-button", shortcut);
  g_object_set_data (G_OBJECT (combo),    "shortcut-action", combo);
  g_object_set_data (G_OBJECT (combo),    "shortcut-action-action", action_button);
  g_signal_connect (shortcut, "notify::accelerator",
                    G_CALLBACK (ligma_modifiers_editor_notify_accelerator),
                    editor);
  g_signal_connect (combo, "notify::active",
                    G_CALLBACK (ligma_modifiers_editor_notify_accelerator),
                    editor);

  g_object_set_data (G_OBJECT (action_button), "shortcut-button", shortcut);
  g_signal_connect (action_button, "clicked",
                    G_CALLBACK (ligma_modifiers_editor_search_clicked),
                    editor);

  gtk_list_box_insert (GTK_LIST_BOX (editor->priv->current_settings), box, -1);

  if (mod_action != LIGMA_MODIFIER_ACTION_NONE)
    ligma_modifiers_editor_update_rows (editor, modifiers, shortcut);

  if (plus_button)
    {
      g_object_ref (plus_button);
      box_row = gtk_widget_get_parent (GTK_WIDGET (plus_button));
      gtk_container_remove (GTK_CONTAINER (box_row), GTK_WIDGET (plus_button));
      gtk_container_remove (GTK_CONTAINER (editor->priv->current_settings), box_row);
      gtk_list_box_insert (GTK_LIST_BOX (editor->priv->current_settings), GTK_WIDGET (plus_button), -1);
    }

  gtk_widget_show (box);
}

static void
ligma_modifiers_editor_plus_button_clicked (GtkButton           *plus_button,
                                           LigmaModifiersEditor *editor)
{
  ligma_modifiers_editor_add_mapping (editor, 0, LIGMA_MODIFIER_ACTION_NONE, NULL);
}

static void
ligma_modifiers_editor_minus_button_clicked (GtkButton           *minus_button,
                                            LigmaModifiersEditor *editor)
{
  GtkWidget       *shortcut = g_object_get_data (G_OBJECT (minus_button), "shortcut-button");
  GdkModifierType  modifiers;

  ligma_shortcut_button_get_keys (LIGMA_SHORTCUT_BUTTON (shortcut), NULL, &modifiers);
  ligma_modifiers_manager_remove (editor->priv->manager, editor->priv->device, editor->priv->button, modifiers);

  /* Always leave at least 1 row. Simply reset it instead. Since
   * GtkListBox doesn't have an API for length, I get the row at
   * position 2. If there is none, then there is just 1 row (+ 1 row for
   * the plus button).
   */
  if (gtk_list_box_get_row_at_index (GTK_LIST_BOX (editor->priv->current_settings), 2) == NULL)
    {
      GtkWidget *combo = g_object_get_data (G_OBJECT (minus_button), "shortcut-action");

      gtk_combo_box_set_active (GTK_COMBO_BOX (combo), LIGMA_MODIFIER_ACTION_NONE);
      ligma_shortcut_button_set_accelerator (LIGMA_SHORTCUT_BUTTON (shortcut), NULL, 0, 0);
    }
  else
    {
      GtkWidget *box_row;

      box_row = gtk_widget_get_parent (GTK_WIDGET (minus_button));
      box_row = gtk_widget_get_parent (box_row);
      ligma_modifiers_editor_update_rows (editor, modifiers, NULL);
    }
}

static void
ligma_modifiers_editor_notify_accelerator (GtkWidget           *widget,
                                          const GParamSpec    *pspec,
                                          LigmaModifiersEditor *editor)
{
  GtkWidget          *shortcut;
  GtkWidget          *combo;
  GtkWidget          *action_button;
  LigmaModifierAction  action = LIGMA_MODIFIER_ACTION_NONE;
  GdkModifierType     old_modifiers;
  GdkModifierType     modifiers;

  old_modifiers = (GdkModifierType) g_object_get_data (G_OBJECT (widget), "shortcut-modifiers");

  shortcut      = g_object_get_data (G_OBJECT (widget), "shortcut-button");
  combo         = g_object_get_data (G_OBJECT (widget), "shortcut-action");
  action_button = g_object_get_data (G_OBJECT (widget), "shortcut-action-action");

  ligma_shortcut_button_get_keys (LIGMA_SHORTCUT_BUTTON (shortcut), NULL, &modifiers);

  /* Delete the previous mapping. */
  if (old_modifiers != modifiers)
    ligma_modifiers_manager_remove (editor->priv->manager, editor->priv->device,
                                   editor->priv->button, old_modifiers);

  g_object_set_data (G_OBJECT (shortcut), "shortcut-modifiers", GINT_TO_POINTER (modifiers));

  if (ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (combo), (gint *) &action))
    {
      const gchar *action_desc;

      /* Check if the new mapping was on another row. */
      if (LIGMA_IS_SHORTCUT_BUTTON (widget) && old_modifiers != modifiers &&
          action != LIGMA_MODIFIER_ACTION_NONE)
        ligma_modifiers_editor_update_rows (editor, modifiers, shortcut);

      /* Finally set the new mapping. */
      action_desc = g_object_get_data (G_OBJECT (action_button), "shortcut-action-desc");
      ligma_modifiers_manager_set (editor->priv->manager, editor->priv->device,
                                  editor->priv->button, modifiers,
                                  action, action_desc);
      gtk_widget_set_visible (action_button, action == LIGMA_MODIFIER_ACTION_ACTION);
    }
}

static void
ligma_modifiers_editor_search_clicked (GtkWidget           *button,
                                      LigmaModifiersEditor *editor)
{
  gchar           *accel_name  = NULL;
  gchar           *action_name = "action name";

  GtkWidget       *shortcut;
  GdkModifierType  modifiers;

  shortcut = g_object_get_data (G_OBJECT (button), "shortcut-button");
  ligma_shortcut_button_get_keys (LIGMA_SHORTCUT_BUTTON (shortcut), NULL, &modifiers);
  accel_name = gtk_accelerator_name (0, modifiers);

  if (accel_name)
    {
      GtkWidget *view;
      GtkWidget *edit_dialog;
      gchar     *title;

      if (strlen (accel_name) > 0)
        {
          if (gdk_device_get_name (editor->priv->device) != NULL)
            /* TRANSLATORS: first %s is modifier keys, %d is button
             * number, last %s is an input device (e.g. a mouse) name.
             */
            title = g_strdup_printf (_("Select Action for %s button %d of %s"),
                                     accel_name, editor->priv->button,
                                     gdk_device_get_name (editor->priv->device));
          else
            /* TRANSLATORS: %s is modifiers key, %d is a button number. */
            title = g_strdup_printf (_("Editing modifiers for %s button %d"),
                                     accel_name, editor->priv->button);
        }
      else
        {
            /* TRANSLATORS: %d is a button number, %s is the device (e.g. a mouse) name. */
          if (gdk_device_get_name (editor->priv->device) != NULL)
            title = g_strdup_printf (_("Select Action for button %d of %s"),
                                     editor->priv->button,
                                     gdk_device_get_name (editor->priv->device));
          else
            /* TRANSLATORS: %d is an input device button number. */
            title = g_strdup_printf (_("Editing modifiers for button %d"),
                                     editor->priv->button);
        }

      edit_dialog =
        ligma_dialog_new (title,
                         "ligma-modifiers-action-dialog",
                         gtk_widget_get_toplevel (GTK_WIDGET (editor)),
                         GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                         ligma_standard_help_func,
                         LIGMA_HELP_PREFS_CANVAS_MODIFIERS,

                         _("_Cancel"), GTK_RESPONSE_CANCEL,
                         _("_OK"),     GTK_RESPONSE_OK,

                         NULL);
      g_free (title);

      /* Default height is very crappy because of the scrollbar so we
       * end up to resize manually each time. Let's have a minimum
       * height.
       */
      gtk_window_set_default_size (GTK_WINDOW (edit_dialog), -1, 400);
      ligma_dialog_set_alternative_button_order (GTK_DIALOG (edit_dialog),
                                                GTK_RESPONSE_OK,
                                                GTK_RESPONSE_CANCEL,
                                                -1);

      g_object_set_data (G_OBJECT (edit_dialog), "shortcut-button", shortcut);
      g_object_set_data (G_OBJECT (edit_dialog), "shortcut-action-action", button);
      g_signal_connect (edit_dialog, "response",
                        G_CALLBACK (ligma_modifiers_editor_search_response),
                        editor);

      view = ligma_action_editor_new (ligma_ui_managers_from_name ("<Image>")->data,
                                     action_name, FALSE);
      gtk_container_set_border_width (GTK_CONTAINER (view), 12);
      gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (edit_dialog))),
                          view, TRUE, TRUE, 0);
      gtk_widget_show (view);

      g_signal_connect_object (LIGMA_ACTION_EDITOR (view)->view, "row-activated",
                               G_CALLBACK (ligma_controller_modifiers_action_activated),
                               edit_dialog, 0);

      editor->priv->action_selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (LIGMA_ACTION_EDITOR (view)->view));

      g_object_add_weak_pointer (G_OBJECT (editor->priv->action_selection),
                                 (gpointer) &editor->priv->action_selection);

      gtk_widget_show (edit_dialog);

      g_free (accel_name);
    }
}

static void
ligma_controller_modifiers_action_activated (GtkTreeView       *view,
                                            GtkTreePath       *path,
                                            GtkTreeViewColumn *column,
                                            GtkWidget         *edit_dialog)
{
  gtk_dialog_response (GTK_DIALOG (edit_dialog), GTK_RESPONSE_OK);
}

static void
ligma_modifiers_editor_search_response (GtkWidget           *dialog,
                                       gint                 response_id,
                                       LigmaModifiersEditor *editor)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      GtkTreeModel *model;
      GtkTreeIter   iter;
      gchar        *icon_name   = NULL;
      LigmaAction   *action      = NULL;

      if (gtk_tree_selection_get_selected (editor->priv->action_selection, &model, &iter))
        gtk_tree_model_get (model, &iter,
                            LIGMA_ACTION_VIEW_COLUMN_ACTION,    &action,
                            LIGMA_ACTION_VIEW_COLUMN_ICON_NAME, &icon_name,
                            -1);

      if (action)
        {
          GtkActionGroup *group;

          g_object_get (action,
                        "action-group", &group,
                        NULL);

          if (group)
            {
              GtkWidget       *action_button;
              GtkWidget       *shortcut;
              GtkWidget       *label;
              gchar           *action_desc;
              GdkModifierType  modifiers;

              shortcut      = g_object_get_data (G_OBJECT (dialog), "shortcut-button");
              ligma_shortcut_button_get_keys (LIGMA_SHORTCUT_BUTTON (shortcut), NULL, &modifiers);

              action_button = g_object_get_data (G_OBJECT (dialog), "shortcut-action-action");
              action_desc = g_strdup_printf ("%s/%s",
                                             gtk_action_group_get_name (group),
                                             ligma_action_get_name (action));

              g_object_set_data_full (G_OBJECT (action_button), "shortcut-action-desc",
                                      action_desc, g_free);

              ligma_modifiers_manager_set (editor->priv->manager, editor->priv->device,
                                          editor->priv->button, modifiers,
                                          LIGMA_MODIFIER_ACTION_ACTION, action_desc);

              /* Change the button label. */
              gtk_container_foreach (GTK_CONTAINER (action_button),
                                     (GtkCallback) gtk_widget_destroy,
                                     NULL);
              label = gtk_label_new (ligma_action_get_name (action));
              gtk_container_add (GTK_CONTAINER (action_button), label);
              gtk_widget_show (label);

              g_object_unref (group);
            }
        }

      g_free (icon_name);
      g_clear_object (&action);
    }

  gtk_widget_destroy (dialog);
}

static gchar *
ligma_modifiers_editor_make_hash_key (GdkDevice        *device,
                                     guint             button,
                                     GdkModifierType   modifiers)
{
  const gchar *vendor_id;
  const gchar *product_id;

  g_return_val_if_fail (GDK_IS_DEVICE (device) || device == NULL, NULL);

  vendor_id  = device ? gdk_device_get_vendor_id (device) : NULL;
  product_id = device ? gdk_device_get_product_id (device) : NULL;
  modifiers  = modifiers & ligma_get_all_modifiers_mask ();

  return g_strdup_printf ("%s:%s-%d-%d",
                          vendor_id ? vendor_id : "(no-vendor-id)",
                          product_id ? product_id : "(no-product-id)",
                          button, modifiers);
}

static void
ligma_modifiers_editor_update_rows (LigmaModifiersEditor *editor,
                                   GdkModifierType      modifiers,
                                   GtkWidget           *box_child)
{
  GtkWidget *box_row;
  gchar     *hash_key;

  hash_key = ligma_modifiers_editor_make_hash_key (editor->priv->device,
                                                  editor->priv->button,
                                                  modifiers);
  if ((box_row = g_hash_table_lookup (editor->priv->rows, hash_key)) != NULL)
    {
      ligma_modifiers_manager_remove (editor->priv->manager, editor->priv->device,
                                     editor->priv->button, modifiers);
      gtk_container_remove (GTK_CONTAINER (editor->priv->current_settings), box_row);
    }

  if (box_child)
    {
      box_row = box_child;
      while (box_row && ! GTK_IS_LIST_BOX_ROW (box_row))
        box_row = gtk_widget_get_parent (box_row);

      g_return_if_fail (box_row != NULL && GTK_IS_LIST_BOX_ROW (box_row));

      g_hash_table_foreach_remove (editor->priv->rows,
                                   ligma_modifiers_editor_search_row,
                                   box_row);
      g_hash_table_insert (editor->priv->rows, hash_key, box_row);
    }
  else
    {
      g_free (hash_key);
    }
}

static gboolean
ligma_modifiers_editor_search_row (gpointer key,
                                  gpointer value,
                                  gpointer user_data)
{
  return (value == user_data);
}
