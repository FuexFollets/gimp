/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaactionview.c
 * Copyright (C) 2004-2005  Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligma.h"

#include "ligmaaction.h"
#include "ligmaactiongroup.h"
#include "ligmaactionview.h"
#include "ligmamessagebox.h"
#include "ligmamessagedialog.h"
#include "ligmauimanager.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void     ligma_action_view_dispose         (GObject         *object);
static void     ligma_action_view_finalize        (GObject         *object);

static void     ligma_action_view_select_path     (LigmaActionView  *view,
                                                  GtkTreePath     *path);
static gboolean ligma_action_view_accel_find_func (GtkAccelKey     *key,
                                                  GClosure        *closure,
                                                  gpointer         data);
static void     ligma_action_view_accel_changed   (GtkAccelGroup   *accel_group,
                                                  guint            unused1,
                                                  GdkModifierType  unused2,
                                                  GClosure        *accel_closure,
                                                  LigmaActionView  *view);
static void     ligma_action_view_accel_edited    (GtkCellRendererAccel *accel,
                                                  const char      *path_string,
                                                  guint            accel_key,
                                                  GdkModifierType  accel_mask,
                                                  guint            hardware_keycode,
                                                  LigmaActionView  *view);
static void     ligma_action_view_accel_cleared   (GtkCellRendererAccel *accel,
                                                  const char      *path_string,
                                                  LigmaActionView  *view);


G_DEFINE_TYPE (LigmaActionView, ligma_action_view, GTK_TYPE_TREE_VIEW)

#define parent_class ligma_action_view_parent_class


static void
ligma_action_view_class_init (LigmaActionViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose  = ligma_action_view_dispose;
  object_class->finalize = ligma_action_view_finalize;
}

static void
ligma_action_view_init (LigmaActionView *view)
{
}

static void
ligma_action_view_dispose (GObject *object)
{
  LigmaActionView *view = LIGMA_ACTION_VIEW (object);

  if (view->manager)
    {
      if (view->show_shortcuts)
        {
          GtkAccelGroup *group;

          group = ligma_ui_manager_get_accel_group (view->manager);

          g_signal_handlers_disconnect_by_func (group,
                                                ligma_action_view_accel_changed,
                                                view);
        }

      g_clear_object (&view->manager);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_action_view_finalize (GObject *object)
{
  LigmaActionView *view = LIGMA_ACTION_VIEW (object);

  g_clear_pointer (&view->filter, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
ligma_action_view_new (LigmaUIManager *manager,
                      const gchar   *select_action,
                      gboolean       show_shortcuts)
{
  GtkTreeView       *view;
  GtkTreeViewColumn *column;
  GtkCellRenderer   *cell;
  GtkTreeStore      *store;
  GtkTreeModel      *filter;
  GtkAccelGroup     *accel_group;
  GList             *list;
  GtkTreePath       *select_path = NULL;

  g_return_val_if_fail (LIGMA_IS_UI_MANAGER (manager), NULL);

  store = gtk_tree_store_new (LIGMA_ACTION_VIEW_N_COLUMNS,
                              G_TYPE_BOOLEAN,         /* COLUMN_VISIBLE        */
                              LIGMA_TYPE_ACTION,       /* COLUMN_ACTION         */
                              G_TYPE_STRING,          /* COLUMN_ICON_NAME      */
                              G_TYPE_STRING,          /* COLUMN_LABEL          */
                              G_TYPE_STRING,          /* COLUMN_LABEL_CASEFOLD */
                              G_TYPE_STRING,          /* COLUMN_NAME           */
                              G_TYPE_UINT,            /* COLUMN_ACCEL_KEY      */
                              GDK_TYPE_MODIFIER_TYPE, /* COLUMN_ACCEL_MASK     */
                              G_TYPE_CLOSURE);        /* COLUMN_ACCEL_CLOSURE  */

  accel_group = ligma_ui_manager_get_accel_group (manager);

  for (list = ligma_ui_manager_get_action_groups (manager);
       list;
       list = g_list_next (list))
    {
      LigmaActionGroup *group = list->data;
      GList           *actions;
      GList           *list2;
      GtkTreeIter      group_iter;

      gtk_tree_store_append (store, &group_iter, NULL);

      gtk_tree_store_set (store, &group_iter,
                          LIGMA_ACTION_VIEW_COLUMN_ICON_NAME, group->icon_name,
                          LIGMA_ACTION_VIEW_COLUMN_LABEL,     group->label,
                          -1);

      actions = ligma_action_group_list_actions (group);

      actions = g_list_sort (actions, (GCompareFunc) ligma_action_name_compare);

      for (list2 = actions; list2; list2 = g_list_next (list2))
        {
          LigmaAction      *action        = list2->data;
          const gchar     *name          = ligma_action_get_name (action);
          const gchar     *icon_name     = ligma_action_get_icon_name (action);
          gchar           *label;
          gchar           *label_casefold;
          guint            accel_key     = 0;
          GdkModifierType  accel_mask    = 0;
          GClosure        *accel_closure = NULL;
          GtkTreeIter      action_iter;

          if (ligma_action_is_gui_blacklisted (name))
            continue;

          label = ligma_strip_uline (ligma_action_get_label (action));

          if (! (label && strlen (label)))
            {
              g_free (label);
              label = g_strdup (name);
            }

          label_casefold = g_utf8_casefold (label, -1);

          if (show_shortcuts)
            {
              accel_closure = ligma_action_get_accel_closure (action);

              if (accel_closure)
                {
                  GtkAccelKey *key;

                  key = gtk_accel_group_find (accel_group,
                                              ligma_action_view_accel_find_func,
                                              accel_closure);

                  if (key            &&
                      key->accel_key &&
                      key->accel_flags & GTK_ACCEL_VISIBLE)
                    {
                      accel_key  = key->accel_key;
                      accel_mask = key->accel_mods;
                    }
                }
            }

          gtk_tree_store_append (store, &action_iter, &group_iter);

          gtk_tree_store_set (store, &action_iter,
                              LIGMA_ACTION_VIEW_COLUMN_VISIBLE,        TRUE,
                              LIGMA_ACTION_VIEW_COLUMN_ACTION,         action,
                              LIGMA_ACTION_VIEW_COLUMN_ICON_NAME,      icon_name,
                              LIGMA_ACTION_VIEW_COLUMN_LABEL,          label,
                              LIGMA_ACTION_VIEW_COLUMN_LABEL_CASEFOLD, label_casefold,
                              LIGMA_ACTION_VIEW_COLUMN_NAME,           name,
                              LIGMA_ACTION_VIEW_COLUMN_ACCEL_KEY,      accel_key,
                              LIGMA_ACTION_VIEW_COLUMN_ACCEL_MASK,     accel_mask,
                              LIGMA_ACTION_VIEW_COLUMN_ACCEL_CLOSURE,  accel_closure,
                              -1);

          g_free (label);
          g_free (label_casefold);

          if (select_action && ! strcmp (select_action, name))
            {
              select_path = gtk_tree_model_get_path (GTK_TREE_MODEL (store),
                                                     &action_iter);
            }
        }

      g_list_free (actions);
    }

  filter = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);

  g_object_unref (store);

  view = g_object_new (LIGMA_TYPE_ACTION_VIEW,
                       "model",      filter,
                       "rules-hint", TRUE,
                       NULL);

  g_object_unref (filter);

  gtk_tree_model_filter_set_visible_column (GTK_TREE_MODEL_FILTER (filter),
                                            LIGMA_ACTION_VIEW_COLUMN_VISIBLE);

  LIGMA_ACTION_VIEW (view)->manager        = g_object_ref (manager);
  LIGMA_ACTION_VIEW (view)->show_shortcuts = show_shortcuts;

  gtk_tree_view_set_search_column (GTK_TREE_VIEW (view),
                                   LIGMA_ACTION_VIEW_COLUMN_LABEL);

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Action"));

  cell = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, cell, FALSE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "icon-name",
                                       LIGMA_ACTION_VIEW_COLUMN_ICON_NAME,
                                       NULL);

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text",
                                       LIGMA_ACTION_VIEW_COLUMN_LABEL,
                                       NULL);

  gtk_tree_view_append_column (view, column);

  if (show_shortcuts)
    {
      g_signal_connect (accel_group, "accel-changed",
                        G_CALLBACK (ligma_action_view_accel_changed),
                        view);

      column = gtk_tree_view_column_new ();
      gtk_tree_view_column_set_title (column, _("Shortcut"));

      cell = gtk_cell_renderer_accel_new ();
      g_object_set (cell,
                    "mode",     GTK_CELL_RENDERER_MODE_EDITABLE,
                    "editable", TRUE,
                    NULL);
      gtk_tree_view_column_pack_start (column, cell, TRUE);
      gtk_tree_view_column_set_attributes (column, cell,
                                           "accel-key",
                                           LIGMA_ACTION_VIEW_COLUMN_ACCEL_KEY,
                                           "accel-mods",
                                           LIGMA_ACTION_VIEW_COLUMN_ACCEL_MASK,
                                           NULL);

      g_signal_connect (cell, "accel-edited",
                        G_CALLBACK (ligma_action_view_accel_edited),
                        view);
      g_signal_connect (cell, "accel-cleared",
                        G_CALLBACK (ligma_action_view_accel_cleared),
                        view);

      gtk_tree_view_append_column (view, column);
    }

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Name"));

  cell = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, cell, TRUE);
  gtk_tree_view_column_set_attributes (column, cell,
                                       "text",
                                       LIGMA_ACTION_VIEW_COLUMN_NAME,
                                       NULL);

  gtk_tree_view_append_column (view, column);

  if (select_path)
    {
      ligma_action_view_select_path (LIGMA_ACTION_VIEW (view), select_path);
      gtk_tree_path_free (select_path);
    }

  return GTK_WIDGET (view);
}

void
ligma_action_view_set_filter (LigmaActionView *view,
                             const gchar    *filter)
{
  GtkTreeSelection    *sel;
  GtkTreeModel        *filtered_model;
  GtkTreeModel        *model;
  GtkTreeIter          iter;
  gboolean             iter_valid;
  GtkTreeRowReference *selected_row = NULL;

  g_return_if_fail (LIGMA_IS_ACTION_VIEW (view));

  filtered_model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (filtered_model));

  if (filter && ! strlen (filter))
    filter = NULL;

  g_clear_pointer (&view->filter, g_free);

  if (filter)
    view->filter = g_utf8_casefold (filter, -1);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path (filtered_model, &iter);

      selected_row = gtk_tree_row_reference_new (filtered_model, path);
    }

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GtkTreeIter child_iter;
      gboolean    child_valid;
      gint        n_children = 0;

      for (child_valid = gtk_tree_model_iter_children (model, &child_iter,
                                                       &iter);
           child_valid;
           child_valid = gtk_tree_model_iter_next (model, &child_iter))
        {
          gboolean visible = TRUE;

          if (view->filter)
            {
              gchar *label;
              gchar *name;

              gtk_tree_model_get (model, &child_iter,
                                  LIGMA_ACTION_VIEW_COLUMN_LABEL_CASEFOLD, &label,
                                  LIGMA_ACTION_VIEW_COLUMN_NAME,           &name,
                                  -1);

              visible = label && name && (strstr (label, view->filter) != NULL ||
                                          strstr (name,  view->filter) != NULL);

              g_free (label);
              g_free (name);
            }

          gtk_tree_store_set (GTK_TREE_STORE (model), &child_iter,
                              LIGMA_ACTION_VIEW_COLUMN_VISIBLE, visible,
                              -1);

          if (visible)
            n_children++;
        }

      gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                          LIGMA_ACTION_VIEW_COLUMN_VISIBLE, n_children > 0,
                          -1);
    }

  if (view->filter)
    gtk_tree_view_expand_all (GTK_TREE_VIEW (view));
  else
    gtk_tree_view_collapse_all (GTK_TREE_VIEW (view));

  gtk_tree_view_columns_autosize (GTK_TREE_VIEW (view));

  if (selected_row)
    {
      if (gtk_tree_row_reference_valid (selected_row))
        {
          GtkTreePath *path = gtk_tree_row_reference_get_path (selected_row);

          ligma_action_view_select_path (view, path);
          gtk_tree_path_free (path);
        }

      gtk_tree_row_reference_free (selected_row);
    }
}


/*  private functions  */

static void
ligma_action_view_select_path (LigmaActionView *view,
                              GtkTreePath    *path)
{
  GtkTreeView *tv = GTK_TREE_VIEW (view);
  GtkTreePath *expand;

  expand = gtk_tree_path_copy (path);
  gtk_tree_path_up (expand);
  gtk_tree_view_expand_row (tv, expand, FALSE);
  gtk_tree_path_free (expand);

  gtk_tree_view_set_cursor (tv, path, NULL, FALSE);
  gtk_tree_view_scroll_to_cell (tv, path, NULL, TRUE, 0.5, 0.0);
}

static gboolean
ligma_action_view_accel_find_func (GtkAccelKey *key,
                                  GClosure    *closure,
                                  gpointer     data)
{
  return (GClosure *) data == closure;
}

static void
ligma_action_view_accel_changed (GtkAccelGroup   *accel_group,
                                guint            unused1,
                                GdkModifierType  unused2,
                                GClosure        *accel_closure,
                                LigmaActionView  *view)
{
  GtkTreeModel *model, *tmpmodel;
  GtkTreeIter   iter;
  gboolean      iter_valid;
  gchar        *pathstr = NULL;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  if (! model)
    return;

  model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));
  if (! model)
    return;

  if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (GTK_TREE_VIEW (view)),
                                       &tmpmodel, &iter))
    {
      GtkTreePath *path = gtk_tree_model_get_path (tmpmodel, &iter);

      pathstr = gtk_tree_path_to_string(path);
    }

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      GtkTreeIter child_iter;
      gboolean    child_valid;

      for (child_valid = gtk_tree_model_iter_children (model, &child_iter,
                                                       &iter);
           child_valid;
           child_valid = gtk_tree_model_iter_next (model, &child_iter))
        {
          GClosure *closure;

          gtk_tree_model_get (model, &child_iter,
                              LIGMA_ACTION_VIEW_COLUMN_ACCEL_CLOSURE, &closure,
                              -1);

          if (closure)
            g_closure_unref (closure);

          if (accel_closure == closure)
            {
              GtkAccelKey     *key;
              guint            accel_key  = 0;
              GdkModifierType  accel_mask = 0;

              key = gtk_accel_group_find (accel_group,
                                          ligma_action_view_accel_find_func,
                                          accel_closure);

              if (key            &&
                  key->accel_key &&
                  key->accel_flags & GTK_ACCEL_VISIBLE)
                {
                  accel_key  = key->accel_key;
                  accel_mask = key->accel_mods;
                }

              gtk_tree_store_set (GTK_TREE_STORE (model), &child_iter,
                                  LIGMA_ACTION_VIEW_COLUMN_ACCEL_KEY,  accel_key,
                                  LIGMA_ACTION_VIEW_COLUMN_ACCEL_MASK, accel_mask,
                                  -1);
              gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                                  LIGMA_ACTION_VIEW_COLUMN_VISIBLE, TRUE,
                                  -1);

              if (pathstr)
                {
                  GtkTreePath *path = gtk_tree_path_new_from_string (pathstr);

                  ligma_action_view_select_path (view, path);
                  gtk_tree_path_free (path);
                }
              g_free (pathstr);
              return;
            }
        }
    }
  g_free (pathstr);
}

typedef struct
{
  LigmaUIManager   *manager;
  gchar           *accel_path;
  guint            accel_key;
  GdkModifierType  accel_mask;
} ConfirmData;

static void
ligma_action_view_conflict_response (GtkWidget   *dialog,
                                    gint         response_id,
                                    ConfirmData *confirm_data)
{
  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      if (! gtk_accel_map_change_entry (confirm_data->accel_path,
                                        confirm_data->accel_key,
                                        confirm_data->accel_mask,
                                        TRUE))
        {
          ligma_message_literal (confirm_data->manager->ligma, G_OBJECT (dialog),
                                LIGMA_MESSAGE_ERROR,
                                _("Changing shortcut failed."));
        }
    }

  g_free (confirm_data->accel_path);

  g_slice_free (ConfirmData, confirm_data);
}

static void
ligma_action_view_conflict_confirm (LigmaActionView  *view,
                                   LigmaAction      *action,
                                   guint            accel_key,
                                   GdkModifierType  accel_mask,
                                   const gchar     *accel_path)
{
  LigmaActionGroup *group;
  gchar           *label;
  gchar           *accel_string;
  ConfirmData     *confirm_data;
  GtkWidget       *dialog;
  LigmaMessageBox  *box;

  g_object_get (action, "action-group", &group, NULL);

  label = ligma_strip_uline (ligma_action_get_label (action));

  accel_string = gtk_accelerator_get_label (accel_key, accel_mask);

  confirm_data = g_slice_new (ConfirmData);

  confirm_data->manager    = view->manager;
  confirm_data->accel_path = g_strdup (accel_path);
  confirm_data->accel_key  = accel_key;
  confirm_data->accel_mask = accel_mask;

  dialog =
    ligma_message_dialog_new (_("Conflicting Shortcuts"),
                             LIGMA_ICON_DIALOG_WARNING,
                             gtk_widget_get_toplevel (GTK_WIDGET (view)), 0,
                             ligma_standard_help_func, NULL,

                             _("_Cancel"),            GTK_RESPONSE_CANCEL,
                             _("_Reassign Shortcut"), GTK_RESPONSE_OK,

                             NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (ligma_action_view_conflict_response),
                    confirm_data);

  box = LIGMA_MESSAGE_DIALOG (dialog)->box;

  ligma_message_box_set_primary_text (box,
                                     _("Shortcut \"%s\" is already taken "
                                       "by \"%s\" from the \"%s\" group."),
                                     accel_string, label, group->label);
  ligma_message_box_set_text (box,
                             _("Reassigning the shortcut will cause it "
                               "to be removed from \"%s\"."),
                             label);

  g_free (label);
  g_free (accel_string);

  g_object_unref (group);

  gtk_widget_show (dialog);
}

static const gchar *
ligma_action_view_get_accel_action (LigmaActionView  *view,
                                   const gchar     *path_string,
                                   LigmaAction     **action_return,
                                   guint           *action_accel_key,
                                   GdkModifierType *action_accel_mask)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GtkTreeIter   iter;

  model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
  if (! model)
    return NULL;

  path = gtk_tree_path_new_from_string (path_string);

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      LigmaAction *action;

      gtk_tree_model_get (model, &iter,
                          LIGMA_ACTION_VIEW_COLUMN_ACTION,     &action,
                          LIGMA_ACTION_VIEW_COLUMN_ACCEL_KEY,  action_accel_key,
                          LIGMA_ACTION_VIEW_COLUMN_ACCEL_MASK, action_accel_mask,
                          -1);

      if (! action)
        goto done;

      gtk_tree_path_free (path);
      g_object_unref (action);

      *action_return = action;

      return ligma_action_get_accel_path (action);
    }

 done:
  gtk_tree_path_free (path);

  return NULL;
}

static void
ligma_action_view_accel_edited (GtkCellRendererAccel *accel,
                               const char           *path_string,
                               guint                 accel_key,
                               GdkModifierType       accel_mask,
                               guint                 hardware_keycode,
                               LigmaActionView       *view)
{
  LigmaAction      *action;
  guint            action_accel_key;
  GdkModifierType  action_accel_mask;
  const gchar     *accel_path;

  accel_path = ligma_action_view_get_accel_action (view, path_string,
                                                  &action,
                                                  &action_accel_key,
                                                  &action_accel_mask);

  if (! accel_path)
    return;

  if (accel_key  == action_accel_key &&
      accel_mask == action_accel_mask)
    return;

  if (! accel_key ||

      /* Don't allow arrow keys, they are all swallowed by the canvas
       * and cannot be invoked anyway, the same applies to space.
       */
      accel_key == GDK_KEY_Left  ||
      accel_key == GDK_KEY_Right ||
      accel_key == GDK_KEY_Up    ||
      accel_key == GDK_KEY_Down  ||
      accel_key == GDK_KEY_space ||
      accel_key == GDK_KEY_KP_Space)
    {
      ligma_message_literal (view->manager->ligma,
                            G_OBJECT (view), LIGMA_MESSAGE_ERROR,
                            _("Invalid shortcut."));
    }
  else if (accel_key        == GDK_KEY_F1 ||
           action_accel_key == GDK_KEY_F1)
    {
      ligma_message_literal (view->manager->ligma,
                            G_OBJECT (view), LIGMA_MESSAGE_ERROR,
                            _("F1 cannot be remapped."));
    }
  else if (accel_key  >= GDK_KEY_0 &&
           accel_key  <= GDK_KEY_9 &&
           accel_mask == GDK_MOD1_MASK)
    {
      ligma_message (view->manager->ligma,
                    G_OBJECT (view), LIGMA_MESSAGE_ERROR,
                    _("Alt+%d is used to switch to display %d and "
                      "cannot be remapped."),
                    accel_key - GDK_KEY_0,
                    accel_key == GDK_KEY_0 ? 10 : accel_key - GDK_KEY_0);
    }
  else if (! gtk_accel_map_change_entry (accel_path,
                                         accel_key, accel_mask, FALSE))
    {
      GtkTreeModel *model;
      LigmaAction   *conflict_action = NULL;
      GtkTreeIter   iter;
      gboolean      iter_valid;

      model = gtk_tree_view_get_model (GTK_TREE_VIEW (view));
      model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));

      for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (model, &iter))
        {
          GtkTreeIter child_iter;
          gboolean    child_valid;

          for (child_valid = gtk_tree_model_iter_children (model,
                                                           &child_iter,
                                                           &iter);
               child_valid;
               child_valid = gtk_tree_model_iter_next (model, &child_iter))
            {
              guint           child_accel_key;
              GdkModifierType child_accel_mask;

              gtk_tree_model_get (model, &child_iter,
                                  LIGMA_ACTION_VIEW_COLUMN_ACCEL_KEY,
                                  &child_accel_key,
                                  LIGMA_ACTION_VIEW_COLUMN_ACCEL_MASK,
                                  &child_accel_mask,
                                  -1);

              if (accel_key  == child_accel_key &&
                  accel_mask == child_accel_mask)
                {
                  gtk_tree_model_get (model, &child_iter,
                                      LIGMA_ACTION_VIEW_COLUMN_ACTION,
                                      &conflict_action,
                                      -1);
                  break;
                }
            }

          if (conflict_action)
            break;
        }

      if (conflict_action != action)
        {
          if (conflict_action)
            {
              ligma_action_view_conflict_confirm (view, conflict_action,
                                                 accel_key,
                                                 accel_mask,
                                                 accel_path);
              g_object_unref (conflict_action);
            }
          else
            {
              ligma_message_literal (view->manager->ligma,
                                    G_OBJECT (view), LIGMA_MESSAGE_ERROR,
                                    _("Changing shortcut failed."));
            }
        }
    }
}

static void
ligma_action_view_accel_cleared (GtkCellRendererAccel *accel,
                                const char           *path_string,
                                LigmaActionView       *view)
{
  LigmaAction      *action;
  guint            action_accel_key;
  GdkModifierType  action_accel_mask;
  const gchar     *accel_path;

  accel_path = ligma_action_view_get_accel_action (view, path_string,
                                                  &action,
                                                  &action_accel_key,
                                                  &action_accel_mask);

  if (! accel_path)
    return;

  if (action_accel_key == GDK_KEY_F1)
    {
      ligma_message_literal (view->manager->ligma,
                            G_OBJECT (view), LIGMA_MESSAGE_ERROR,
                            _("F1 cannot be remapped."));
      return;
    }

  if (! gtk_accel_map_change_entry (accel_path, 0, 0, FALSE))
    {
      ligma_message_literal (view->manager->ligma,
                            G_OBJECT (view), LIGMA_MESSAGE_ERROR,
                            _("Removing shortcut failed."));
    }
}
