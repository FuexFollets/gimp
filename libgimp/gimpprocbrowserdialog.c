/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaprocbrowserdialog.c
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "ligma.h"

#include "ligmauitypes.h"
#include "ligmaprocbrowserdialog.h"
#include "ligmaprocview.h"

#include "libligma-intl.h"


/**
 * SECTION: ligmaprocbrowserdialog
 * @title: LigmaProcBrowserDialog
 * @short_description: The dialog for the procedure and plugin browsers.
 *
 * The dialog for the procedure and plugin browsers.
 **/


#define DBL_LIST_WIDTH 250
#define DBL_WIDTH      (DBL_LIST_WIDTH + 400)
#define DBL_HEIGHT     250


enum
{
  SELECTION_CHANGED,
  ROW_ACTIVATED,
  LAST_SIGNAL
};

typedef enum
{
  SEARCH_TYPE_ALL,
  SEARCH_TYPE_NAME,
  SEARCH_TYPE_BLURB,
  SEARCH_TYPE_HELP,
  SEARCH_TYPE_AUTHORS,
  SEARCH_TYPE_COPYRIGHT,
  SEARCH_TYPE_DATE,
  SEARCH_TYPE_PROC_TYPE
} SearchType;

enum
{
  COLUMN_PROC_NAME,
  N_COLUMNS
};


struct _LigmaProcBrowserDialogPrivate
{
  GtkWidget    *browser;

  GtkListStore *store;
  GtkWidget    *tree_view;
};

#define GET_PRIVATE(obj) (((LigmaProcBrowserDialog *) (obj))->priv)


static void       browser_selection_changed (GtkTreeSelection      *sel,
                                             LigmaProcBrowserDialog *dialog);
static void       browser_row_activated     (GtkTreeView           *treeview,
                                             GtkTreePath           *path,
                                             GtkTreeViewColumn     *column,
                                             LigmaProcBrowserDialog *dialog);
static void       browser_search            (LigmaBrowser           *browser,
                                             const gchar           *query_text,
                                             gint                   search_type,
                                             LigmaProcBrowserDialog *dialog);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaProcBrowserDialog, ligma_proc_browser_dialog,
                            LIGMA_TYPE_DIALOG)

#define parent_class ligma_proc_browser_dialog_parent_class

static guint dialog_signals[LAST_SIGNAL] = { 0, };


static void
ligma_proc_browser_dialog_class_init (LigmaProcBrowserDialogClass *klass)
{
  /**
   * LigmaProcBrowserDialog::selection-changed:
   * @dialog: the object that received the signal
   *
   * Emitted when the selection in the contained #GtkTreeView changes.
   */
  dialog_signals[SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaProcBrowserDialogClass,
                                   selection_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * LigmaProcBrowserDialog::row-activated:
   * @dialog: the object that received the signal
   *
   * Emitted when one of the rows in the contained #GtkTreeView is activated.
   */
  dialog_signals[ROW_ACTIVATED] =
    g_signal_new ("row-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaProcBrowserDialogClass,
                                   row_activated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  klass->selection_changed = NULL;
  klass->row_activated     = NULL;
}

static void
ligma_proc_browser_dialog_init (LigmaProcBrowserDialog *dialog)
{
  LigmaProcBrowserDialogPrivate *priv;
  GtkWidget                    *scrolled_window;
  GtkCellRenderer              *renderer;
  GtkTreeSelection             *selection;
  GtkWidget                    *parent;

  dialog->priv = ligma_proc_browser_dialog_get_instance_private (dialog);

  priv = GET_PRIVATE (dialog);

  priv->browser = ligma_browser_new ();
  ligma_browser_add_search_types (LIGMA_BROWSER (priv->browser),
                                 _("by name"),        SEARCH_TYPE_NAME,
                                 _("by description"), SEARCH_TYPE_BLURB,
                                 _("by help"),        SEARCH_TYPE_HELP,
                                 _("by authors"),     SEARCH_TYPE_AUTHORS,
                                 _("by copyright"),   SEARCH_TYPE_COPYRIGHT,
                                 _("by date"),        SEARCH_TYPE_DATE,
                                 _("by type"),        SEARCH_TYPE_PROC_TYPE,
                                 NULL);
  gtk_container_set_border_width (GTK_CONTAINER (priv->browser), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      priv->browser, TRUE, TRUE, 0);
  gtk_widget_show (priv->browser);

  g_signal_connect (priv->browser, "search",
                    G_CALLBACK (browser_search),
                    dialog);

  /* list : list in a scrolled_win */

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (ligma_browser_get_left_vbox (LIGMA_BROWSER (priv->browser))),
                      scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  priv->tree_view = gtk_tree_view_new ();

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_text_set_fixed_height_from_font
    (GTK_CELL_RENDERER_TEXT (renderer), 1);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->tree_view),
                                               -1, NULL,
                                               renderer,
                                               "text", 0,
                                               NULL);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->tree_view), FALSE);

  g_signal_connect (priv->tree_view, "row_activated",
                    G_CALLBACK (browser_row_activated),
                    dialog);

  gtk_widget_set_size_request (priv->tree_view, DBL_LIST_WIDTH, DBL_HEIGHT);
  gtk_container_add (GTK_CONTAINER (scrolled_window), priv->tree_view);
  gtk_widget_show (priv->tree_view);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));

  g_signal_connect (selection, "changed",
                    G_CALLBACK (browser_selection_changed),
                    dialog);

  parent = gtk_widget_get_parent (ligma_browser_get_right_vbox (LIGMA_BROWSER (priv->browser)));
  parent = gtk_widget_get_parent (parent);

  gtk_widget_set_size_request (parent, DBL_WIDTH - DBL_LIST_WIDTH, -1);

  /* first search (all procedures) */
  browser_search (LIGMA_BROWSER (dialog->priv->browser),
                  "", SEARCH_TYPE_ALL, dialog);
}


/*  public functions  */

/**
 * ligma_proc_browser_dialog_new: (skip)
 * @title:     The dialog's title.
 * @role:      The dialog's role, see gtk_window_set_role().
 * @help_func: (scope async): The function which will be called if
 *             the user presses "F1".
 * @help_id:   The help_id which will be passed to @help_func.
 * @...:       A %NULL-terminated list destribing the action_area buttons.
 *
 * Create a new #LigmaProcBrowserDialog.
 *
 * Returns: a newly created #LigmaProcBrowserDialog.
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_proc_browser_dialog_new (const gchar  *title,
                              const gchar  *role,
                              LigmaHelpFunc  help_func,
                              const gchar  *help_id,
                              ...)
{
  LigmaProcBrowserDialog *dialog;
  va_list                args;
  gboolean               use_header_bar;

  va_start (args, help_id);

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (LIGMA_TYPE_PROC_BROWSER_DIALOG,
                         "title",          title,
                         "role",           role,
                         "help-func",      help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                         NULL);

  ligma_dialog_add_buttons_valist (LIGMA_DIALOG (dialog), args);

  va_end (args);

  return GTK_WIDGET (dialog);
}

/**
 * ligma_proc_browser_dialog_get_selected:
 * @dialog: a #LigmaProcBrowserDialog
 *
 * Retrieves the name of the currently selected procedure.
 *
 * Returns: (nullable): The name of the selected procedure of %NULL if no
 *               procedure is selected.
 *
 * Since: 2.4
 **/
gchar *
ligma_proc_browser_dialog_get_selected (LigmaProcBrowserDialog *dialog)
{
  GtkTreeSelection *sel;
  GtkTreeIter       iter;

  g_return_val_if_fail (LIGMA_IS_PROC_BROWSER_DIALOG (dialog), NULL);

  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->priv->tree_view));

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      gchar *proc_name;

      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter,
                          COLUMN_PROC_NAME, &proc_name,
                          -1);

      return proc_name;
    }

  return NULL;
}


/*  private functions  */

static void
browser_selection_changed (GtkTreeSelection      *sel,
                           LigmaProcBrowserDialog *dialog)
{
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      gchar *proc_name;

      gtk_tree_model_get (GTK_TREE_MODEL (dialog->priv->store), &iter,
                          COLUMN_PROC_NAME, &proc_name,
                          -1);

      ligma_browser_set_widget (LIGMA_BROWSER (dialog->priv->browser),
                               ligma_proc_view_new (proc_name));

      g_free (proc_name);
    }

  g_signal_emit (dialog, dialog_signals[SELECTION_CHANGED], 0);
}

static void
browser_row_activated (GtkTreeView           *treeview,
                       GtkTreePath           *path,
                       GtkTreeViewColumn     *column,
                       LigmaProcBrowserDialog *dialog)
{
  g_signal_emit (dialog, dialog_signals[ROW_ACTIVATED], 0);
}

static void
browser_search (LigmaBrowser           *browser,
                const gchar           *query_text,
                gint                   search_type,
                LigmaProcBrowserDialog *dialog)
{
  LigmaProcBrowserDialogPrivate  *priv      = GET_PRIVATE (dialog);
  LigmaPDB                       *pdb       = ligma_get_pdb ();
  gchar                        **proc_list = NULL;
  gint                           num_procs;
  gchar                         *str;
  GRegex                        *regex;

  /*  first check if the query is a valid regex  */
  regex = g_regex_new (query_text, 0, 0, NULL);

  if (! regex)
    {
      gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view), NULL);
      priv->store = NULL;

      ligma_browser_show_message (browser, _("No matches"));

      ligma_browser_set_search_summary (browser,
                                       _("Search term invalid or incomplete"));
      return;
    }

  g_regex_unref (regex);

  switch (search_type)
    {
    case SEARCH_TYPE_ALL:
      ligma_browser_show_message (browser, _("Searching"));

      proc_list = ligma_pdb_query_procedures (pdb,
                                             ".*", ".*", ".*", ".*", ".*",
                                             ".*", ".*", ".*");
      break;

    case SEARCH_TYPE_NAME:
      {
        GString     *query = g_string_new ("");
        const gchar *q     = query_text;

        ligma_browser_show_message (browser, _("Searching by name"));

        while (*q)
          {
            if ((*q == '_') || (*q == '-'))
              g_string_append (query, "-");
            else
              g_string_append_c (query, *q);

            q++;
          }

        proc_list = ligma_pdb_query_procedures (pdb,
                                               query->str, ".*", ".*", ".*", ".*",
                                               ".*", ".*", ".*");

        g_string_free (query, TRUE);
      }
      break;

    case SEARCH_TYPE_BLURB:
      ligma_browser_show_message (browser, _("Searching by description"));

      proc_list = ligma_pdb_query_procedures (pdb,
                                             ".*", query_text, ".*", ".*", ".*",
                                             ".*", ".*", ".*");
      break;

    case SEARCH_TYPE_HELP:
      ligma_browser_show_message (browser, _("Searching by help"));

      proc_list = ligma_pdb_query_procedures (pdb,
                                             ".*", ".*", query_text, ".*", ".*",
                                             ".*", ".*", ".*");
      break;

    case SEARCH_TYPE_AUTHORS:
      ligma_browser_show_message (browser, _("Searching by authors"));

      proc_list = ligma_pdb_query_procedures (pdb,
                                             ".*", ".*", ".*", ".*", query_text,
                                             ".*", ".*", ".*");
      break;

    case SEARCH_TYPE_COPYRIGHT:
      ligma_browser_show_message (browser, _("Searching by copyright"));

      proc_list = ligma_pdb_query_procedures (pdb,
                                             ".*", ".*", ".*", ".*", ".*",
                                             query_text, ".*", ".*");
      break;

    case SEARCH_TYPE_DATE:
      ligma_browser_show_message (browser, _("Searching by date"));

      proc_list = ligma_pdb_query_procedures (pdb,
                                             ".*", ".*", ".*", ".*", ".*",
                                             ".*", query_text, ".*");
      break;

    case SEARCH_TYPE_PROC_TYPE:
      ligma_browser_show_message (browser, _("Searching by type"));

      proc_list = ligma_pdb_query_procedures (pdb,
                                             ".*", ".*", ".*", ".*", ".*",
                                             ".*", ".*", query_text);
      break;
    }

  num_procs = g_strv_length (proc_list);

  if (! query_text || strlen (query_text) == 0)
    {
      str = g_strdup_printf (dngettext (GETTEXT_PACKAGE "-libligma",
                                        "%d procedure",
                                        "%d procedures",
                                        num_procs), num_procs);
    }
  else
    {
      switch (num_procs)
        {
        case 0:
          str = g_strdup (_("No matches for your query"));
          break;
        default:
          str = g_strdup_printf (dngettext (GETTEXT_PACKAGE "-libligma",
                                            "%d procedure matches your query",
                                            "%d procedures match your query",
                                            num_procs), num_procs);
          break;
        }
    }

  ligma_browser_set_search_summary (browser, str);
  g_free (str);

  if (num_procs > 0)
    {
      GtkTreeSelection *selection;
      GtkTreeIter       iter;
      gint              i;

      priv->store = gtk_list_store_new (N_COLUMNS,
                                          G_TYPE_STRING);
      gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view),
                               GTK_TREE_MODEL (priv->store));
      g_object_unref (priv->store);

      for (i = 0; i < num_procs; i++)
        {
          gtk_list_store_append (priv->store, &iter);
          gtk_list_store_set (priv->store, &iter,
                              COLUMN_PROC_NAME, proc_list[i],
                              -1);
        }

      g_strfreev (proc_list);

      gtk_tree_view_columns_autosize (GTK_TREE_VIEW (priv->tree_view));

      gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->store),
                                            COLUMN_PROC_NAME,
                                            GTK_SORT_ASCENDING);

      gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter);
      selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree_view));
      gtk_tree_selection_select_iter (selection, &iter);
    }
  else
    {
      gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree_view), NULL);
      priv->store = NULL;

      ligma_browser_show_message (browser, _("No matches"));
    }
}
