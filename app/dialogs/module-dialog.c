/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * module-dialog.c
 * (C) 1999 Austin Donnelly <austin@ligma.org>
 * (C) 2008 Sven Neumann <sven@ligma.org>
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
#include "libligmamodule/ligmamodule.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligma.h"
#include "core/ligma-modules.h"

#include "widgets/ligmahelp-ids.h"

#include "module-dialog.h"

#include "ligma-intl.h"


#define RESPONSE_REFRESH  1

enum
{
  COLUMN_NAME,
  COLUMN_ENABLED,
  COLUMN_MODULE,
  N_COLUMNS
};

enum
{
  INFO_AUTHOR,
  INFO_VERSION,
  INFO_DATE,
  INFO_COPYRIGHT,
  INFO_LOCATION,
  N_INFOS
};

typedef struct _ModuleDialog ModuleDialog;

struct _ModuleDialog
{
  Ligma         *ligma;

  LigmaModule   *selected;
  GtkListStore *list;

  GtkWidget    *hint;
  GtkWidget    *grid;
  GtkWidget    *label[N_INFOS];
  GtkWidget    *error_box;
  GtkWidget    *error_label;
};


/*  local function prototypes  */

static void   dialog_response         (GtkWidget             *widget,
                                       gint                   response_id,
                                       ModuleDialog          *private);
static void   dialog_destroy_callback (GtkWidget             *widget,
                                       ModuleDialog          *private);
static void   dialog_select_callback  (GtkTreeSelection      *sel,
                                       ModuleDialog          *private);
static void   dialog_enabled_toggled  (GtkCellRendererToggle *celltoggle,
                                       const gchar           *path_string,
                                       ModuleDialog          *private);
static void   make_list_item          (gpointer               data,
                                       gpointer               user_data);
static void   dialog_info_add         (LigmaModuleDB          *db,
                                       LigmaModule            *module,
                                       ModuleDialog          *private);
static void   dialog_info_remove      (LigmaModuleDB          *db,
                                       LigmaModule            *module,
                                       ModuleDialog          *private);
static void   dialog_info_update      (LigmaModuleDB          *db,
                                       LigmaModule            *module,
                                       ModuleDialog          *private);
static void   dialog_info_init        (ModuleDialog          *private,
                                       GtkWidget             *grid);


/*  public functions  */

GtkWidget *
module_dialog_new (Ligma *ligma)
{
  ModuleDialog      *private;
  GtkWidget         *dialog;
  GtkWidget         *vbox;
  GtkWidget         *sw;
  GtkWidget         *view;
  GtkWidget         *image;
  GtkTreeSelection  *sel;
  GtkTreeIter        iter;
  GtkTreeViewColumn *col;
  GtkCellRenderer   *rend;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  private = g_slice_new0 (ModuleDialog);

  private->ligma = ligma;

  dialog = ligma_dialog_new (_("Module Manager"),
                            "ligma-modules", NULL, 0,
                            ligma_standard_help_func, LIGMA_HELP_MODULE_DIALOG,

                            _("_Refresh"), RESPONSE_REFRESH,
                            _("_Close"),   GTK_RESPONSE_CLOSE,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_CLOSE,
                                           RESPONSE_REFRESH,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (dialog_response),
                    private);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  private->hint = ligma_hint_box_new (_("You will have to restart LIGMA "
                                       "for the changes to take effect."));
  gtk_box_pack_start (GTK_BOX (vbox), private->hint, FALSE, FALSE, 0);

  if (ligma->write_modulerc)
    gtk_widget_show (private->hint);

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, 0);
  gtk_widget_set_size_request (sw, 124, 100);
  gtk_widget_show (sw);

  private->list = gtk_list_store_new (N_COLUMNS,
                                      G_TYPE_STRING,
                                      G_TYPE_BOOLEAN,
                                      LIGMA_TYPE_MODULE);
  view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (private->list));
  g_object_unref (private->list);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (view), FALSE);

  g_list_foreach (ligma_module_db_get_modules (ligma->module_db),
                  make_list_item, private);

  rend = gtk_cell_renderer_toggle_new ();

  g_signal_connect (rend, "toggled",
                    G_CALLBACK (dialog_enabled_toggled),
                    private);

  col = gtk_tree_view_column_new ();
  gtk_tree_view_column_pack_start (col, rend, FALSE);
  gtk_tree_view_column_add_attribute (col, rend, "active", COLUMN_ENABLED);

  gtk_tree_view_append_column (GTK_TREE_VIEW (view), col);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (view), 1,
                                               _("Module"),
                                               gtk_cell_renderer_text_new (),
                                               "text", COLUMN_NAME,
                                               NULL);

  gtk_container_add (GTK_CONTAINER (sw), view);
  gtk_widget_show (view);

  private->grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (private->grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), private->grid, FALSE, FALSE, 0);
  gtk_widget_show (private->grid);

  private->error_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), private->error_box, FALSE, FALSE, 0);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_DIALOG_WARNING,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (private->error_box), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  private->error_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (private->error_label), 0.0);
  gtk_box_pack_start (GTK_BOX (private->error_box),
                      private->error_label, TRUE, TRUE, 0);
  gtk_widget_show (private->error_label);

  dialog_info_init (private, private->grid);

  dialog_info_update (ligma->module_db, private->selected, private);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (view));

  g_signal_connect (sel, "changed",
                    G_CALLBACK (dialog_select_callback),
                    private);

  if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (private->list), &iter))
    gtk_tree_selection_select_iter (sel, &iter);

  /* hook the LigmaModuleDB signals so we can refresh the display
   * appropriately.
   */
  g_signal_connect (ligma->module_db, "add",
                    G_CALLBACK (dialog_info_add),
                    private);
  g_signal_connect (ligma->module_db, "remove",
                    G_CALLBACK (dialog_info_remove),
                    private);
  g_signal_connect (ligma->module_db, "module-modified",
                    G_CALLBACK (dialog_info_update),
                    private);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (dialog_destroy_callback),
                    private);

  return dialog;
}


/*  private functions  */

static void
dialog_response (GtkWidget    *widget,
                 gint          response_id,
                 ModuleDialog *private)
{
  if (response_id == RESPONSE_REFRESH)
    ligma_modules_refresh (private->ligma);
  else
    gtk_widget_destroy (widget);
}

static void
dialog_destroy_callback (GtkWidget    *widget,
                         ModuleDialog *private)
{
  g_signal_handlers_disconnect_by_func (private->ligma->module_db,
                                        dialog_info_add,
                                        private);
  g_signal_handlers_disconnect_by_func (private->ligma->module_db,
                                        dialog_info_remove,
                                        private);
  g_signal_handlers_disconnect_by_func (private->ligma->module_db,
                                        dialog_info_update,
                                        private);

  g_slice_free (ModuleDialog, private);
}

static void
dialog_select_callback (GtkTreeSelection *sel,
                        ModuleDialog     *private)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      LigmaModule *module;

      gtk_tree_model_get (GTK_TREE_MODEL (private->list), &iter,
                          COLUMN_MODULE, &module, -1);

      if (module)
        g_object_unref (module);

      if (private->selected == module)
        return;

      private->selected = module;

      dialog_info_update (private->ligma->module_db, private->selected, private);
    }
}

static void
dialog_enabled_toggled (GtkCellRendererToggle *celltoggle,
                        const gchar           *path_string,
                        ModuleDialog          *private)
{
  GtkTreePath *path;
  GtkTreeIter  iter;
  LigmaModule  *module = NULL;

  path = gtk_tree_path_new_from_string (path_string);

  if (! gtk_tree_model_get_iter (GTK_TREE_MODEL (private->list), &iter, path))
    {
      g_warning ("%s: bad tree path?", G_STRFUNC);
      return;
    }

  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (private->list), &iter,
                      COLUMN_MODULE, &module,
                      -1);

  if (module)
    {
      ligma_module_set_auto_load (module, ! ligma_module_get_auto_load (module));
      g_object_unref (module);

      private->ligma->write_modulerc = TRUE;
      gtk_widget_show (private->hint);
   }
}

static void
dialog_list_item_update (ModuleDialog *private,
                         GtkTreeIter  *iter,
                         LigmaModule   *module)
{
  const LigmaModuleInfo *info = ligma_module_get_info (module);
  GFile                *file = ligma_module_get_file (module);

  gtk_list_store_set (private->list, iter,
                      COLUMN_NAME,   (info ?
                                      gettext (info->purpose) :
                                      ligma_file_get_utf8_name (file)),
                      COLUMN_ENABLED, ligma_module_get_auto_load (module),
                      COLUMN_MODULE,  module,
                      -1);
}

static void
make_list_item (gpointer data,
                gpointer user_data)
{
  LigmaModule   *module = data;
  ModuleDialog *private = user_data;
  GtkTreeIter   iter;

  if (! private->selected)
    private->selected = module;

  gtk_list_store_append (private->list, &iter);

  dialog_list_item_update (private, &iter, module);
}

static void
dialog_info_add (LigmaModuleDB *db,
                 LigmaModule   *module,
                 ModuleDialog *private)
{
  make_list_item (module, private);
}

static void
dialog_info_remove (LigmaModuleDB *db,
                    LigmaModule   *module,
                    ModuleDialog *private)
{
  GtkTreeIter  iter;

  /* FIXME: Use gtk_list_store_foreach_remove when it becomes available */

  if (! gtk_tree_model_get_iter_first (GTK_TREE_MODEL (private->list), &iter))
    return;

  do
    {
      LigmaModule  *this;

      gtk_tree_model_get (GTK_TREE_MODEL (private->list), &iter,
                          COLUMN_MODULE, &this,
                          -1);

      if (this)
        g_object_unref (this);

      if (this == module)
        {
          gtk_list_store_remove (private->list, &iter);
          return;
        }
    }
  while (gtk_tree_model_iter_next (GTK_TREE_MODEL (private->list), &iter));

  g_warning ("%s: Tried to remove a module not in the dialog's list.",
             G_STRFUNC);
}

static void
dialog_info_update (LigmaModuleDB *db,
                    LigmaModule   *module,
                    ModuleDialog *private)
{
  GtkTreeModel         *model = GTK_TREE_MODEL (private->list);
  const LigmaModuleInfo *info;
  GtkTreeIter           iter;
  const gchar          *text[N_INFOS] = { NULL, };
  const gchar          *location      = NULL;
  gboolean              iter_valid;
  gint                  i;
  gboolean              show_error;

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      LigmaModule *this;

      gtk_tree_model_get (model, &iter,
                          COLUMN_MODULE, &this,
                          -1);
      if (this)
        g_object_unref (this);

      if (this == module)
        break;
    }

  if (iter_valid)
    dialog_list_item_update (private, &iter, module);

  /* only update the info if we're actually showing it */
  if (module != private->selected)
    return;

  if (! module)
    {
      for (i = 0; i < N_INFOS; i++)
        gtk_label_set_text (GTK_LABEL (private->label[i]), NULL);

      gtk_label_set_text (GTK_LABEL (private->error_label), NULL);
      gtk_widget_hide (private->error_box);

      return;
    }

  if (ligma_module_is_on_disk (module))
    location = ligma_file_get_utf8_name (ligma_module_get_file (module));

  info = ligma_module_get_info (module);

  if (info)
    {
      text[INFO_AUTHOR]    = info->author;
      text[INFO_VERSION]   = info->version;
      text[INFO_DATE]      = info->date;
      text[INFO_COPYRIGHT] = info->copyright;
      text[INFO_LOCATION]  = ligma_module_is_on_disk (module) ?
                             location : _("Only in memory");
    }
  else
    {
      text[INFO_LOCATION]  = ligma_module_is_on_disk (module) ?
                             location : _("No longer available");
    }

  for (i = 0; i < N_INFOS; i++)
    gtk_label_set_text (GTK_LABEL (private->label[i]),
                        text[i] ? text[i] : "--");

  /* Show errors */
  show_error = (ligma_module_get_state (module) == LIGMA_MODULE_STATE_ERROR &&
                ligma_module_get_last_error (module));
  gtk_label_set_text (GTK_LABEL (private->error_label),
                      show_error ? ligma_module_get_last_error (module) : NULL);
  gtk_widget_set_visible (private->error_box, show_error);
}

static void
dialog_info_init (ModuleDialog *private,
                  GtkWidget    *grid)
{
  GtkWidget *label;
  gint       i;

  const gchar * const text[] =
  {
    N_("Author:"),
    N_("Version:"),
    N_("Date:"),
    N_("Copyright:"),
    N_("Location:")
  };

  for (i = 0; i < G_N_ELEMENTS (text); i++)
    {
      label = gtk_label_new (gettext (text[i]));
      gtk_label_set_xalign (GTK_LABEL (label), 0.0);
      gtk_grid_attach (GTK_GRID (grid), label, 0, i, 1, 1);
      gtk_widget_show (label);

      private->label[i] = gtk_label_new ("");
      gtk_label_set_xalign (GTK_LABEL (private->label[i]), 0.0);
      gtk_label_set_ellipsize (GTK_LABEL (private->label[i]),
                               PANGO_ELLIPSIZE_END);
      gtk_grid_attach (GTK_GRID (grid), private->label[i], 1, i, 1, 1);
      gtk_widget_show (private->label[i]);
    }
}
