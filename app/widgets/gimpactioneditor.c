/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaactioneditor.c
 * Copyright (C) 2008  Michael Natterer <mitch@ligma.org>
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

#include "widgets-types.h"

#include "ligmaactioneditor.h"
#include "ligmaactionview.h"
#include "ligmauimanager.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_action_editor_filter_clear   (GtkEntry         *entry);
static void   ligma_action_editor_filter_changed (GtkEntry         *entry,
                                                 LigmaActionEditor *editor);


G_DEFINE_TYPE (LigmaActionEditor, ligma_action_editor, GTK_TYPE_BOX)

#define parent_class ligma_action_editor_parent_class


static void
ligma_action_editor_class_init (LigmaActionEditorClass *klass)
{
}

static void
ligma_action_editor_init (LigmaActionEditor *editor)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;

  gtk_orientable_set_orientation (GTK_ORIENTABLE (editor),
                                  GTK_ORIENTATION_VERTICAL);

  gtk_box_set_spacing (GTK_BOX (editor), 12);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (editor), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new_with_mnemonic (_("_Search:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
  gtk_widget_show (entry);

  gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (entry),
                                     GTK_ENTRY_ICON_SECONDARY, "edit-clear");
  gtk_entry_set_icon_activatable (GTK_ENTRY (entry),
                                  GTK_ENTRY_ICON_SECONDARY, TRUE);
  gtk_entry_set_icon_sensitive (GTK_ENTRY (entry),
                                GTK_ENTRY_ICON_SECONDARY, FALSE);

  g_signal_connect (entry, "icon-press",
                    G_CALLBACK (ligma_action_editor_filter_clear),
                    NULL);
  g_signal_connect (entry, "changed",
                    G_CALLBACK (ligma_action_editor_filter_changed),
                    editor);
}

GtkWidget *
ligma_action_editor_new (LigmaUIManager *manager,
                        const gchar   *select_action,
                        gboolean       show_shortcuts)
{
  LigmaActionEditor *editor;
  GtkWidget        *scrolled_window;

  g_return_val_if_fail (LIGMA_IS_UI_MANAGER (manager), NULL);

  editor = g_object_new (LIGMA_TYPE_ACTION_EDITOR, NULL);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), scrolled_window, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_window);

  editor->view = ligma_action_view_new (manager, select_action, show_shortcuts);
  gtk_widget_set_size_request (editor->view, 300, 400);
  gtk_container_add (GTK_CONTAINER (scrolled_window), editor->view);
  gtk_widget_show (editor->view);

  return GTK_WIDGET (editor);
}


/*  private functions  */

static void
ligma_action_editor_filter_clear (GtkEntry *entry)
{
  gtk_entry_set_text (entry, "");
}

static void
ligma_action_editor_filter_changed (GtkEntry         *entry,
                                   LigmaActionEditor *editor)
{
  ligma_action_view_set_filter (LIGMA_ACTION_VIEW (editor->view),
                               gtk_entry_get_text (entry));
  gtk_entry_set_icon_sensitive (entry,
                                GTK_ENTRY_ICON_SECONDARY,
                                gtk_entry_get_text_length (entry) > 0);
}

