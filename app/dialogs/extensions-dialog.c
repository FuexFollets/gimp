/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * extension-dialog.c
 * Copyright (C) 2018 Jehan <jehan@ligma.org>
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

#include <cairo-gobject.h>
#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligma.h"
#include "core/ligmaextensionmanager.h"
#include "core/ligmaextension.h"

#include "widgets/ligmaextensiondetails.h"
#include "widgets/ligmaextensionlist.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaprefsbox.h"

#include "extensions-dialog.h"

#include "ligma-intl.h"

#define LIGMA_EXTENSION_LIST_STACK_CHILD    "extension-list"
#define LIGMA_EXTENSION_DETAILS_STACK_CHILD "extension-details"

static void extensions_dialog_response            (GtkWidget            *widget,
                                                   gint                  response_id,
                                                   GtkWidget            *dialog);
static void extensions_dialog_search_activate     (GtkEntry             *entry,
                                                   gpointer              user_data);
static void extensions_dialog_search_icon_pressed (GtkEntry             *entry,
                                                   GtkEntryIconPosition  icon_pos,
                                                   GdkEvent             *event,
                                                   gpointer              user_data);
static void extensions_dialog_extension_activated (LigmaExtensionList    *list,
                                                   LigmaExtension        *extension,
                                                   GtkStack             *stack);
static void extensions_dialog_back_button_clicked (GtkButton            *button,
                                                   GtkStack             *stack);

/*  public function  */

GtkWidget *
extensions_dialog_new (Ligma *ligma)
{
  GtkWidget   *dialog;
  GtkWidget   *stack;
  GtkWidget   *stacked;
  GtkWidget   *vbox;
  GtkWidget   *hbox;
  GtkWidget   *list;
  GtkWidget   *widget;
  GtkTreeIter  top_iter;

  dialog = ligma_dialog_new (_("Extensions"), "ligma-extensions",
                            NULL, 0, NULL,
                            LIGMA_HELP_EXTENSIONS_DIALOG,
                            _("_OK"), GTK_RESPONSE_OK,
                            NULL);

  widget = gtk_window_get_titlebar (GTK_WINDOW (dialog));
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (widget),
                                        FALSE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (extensions_dialog_response),
                    dialog);

  stack = gtk_stack_new ();
  gtk_stack_set_transition_type (GTK_STACK (stack),
                                 GTK_STACK_TRANSITION_TYPE_SLIDE_RIGHT);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      stack, TRUE, TRUE, 0);
  gtk_widget_show (stack);

  /* The extension lists. */

  stacked = ligma_prefs_box_new ();
  gtk_container_set_border_width (GTK_CONTAINER (stacked), 12);
  gtk_stack_add_named (GTK_STACK (stack), stacked,
                       LIGMA_EXTENSION_LIST_STACK_CHILD);
  gtk_widget_show (stacked);

  vbox = ligma_prefs_box_add_page (LIGMA_PREFS_BOX (stacked),
                                  "system-software-install",
                                  /*"ligma-extensions-installed",*/
                                  _("Installed Extensions"),
                                  _("Installed Extensions"),
                                  LIGMA_HELP_EXTENSIONS_INSTALLED,
                                  NULL,
                                  &top_iter);

  list = ligma_extension_list_new (ligma->extension_manager);
  g_signal_connect (list, "extension-activated",
                    G_CALLBACK (extensions_dialog_extension_activated),
                    stack);
  ligma_extension_list_show_user (LIGMA_EXTENSION_LIST (list));
  gtk_box_pack_start (GTK_BOX (vbox), list, TRUE, TRUE, 1);
  gtk_widget_show (list);

  vbox = ligma_prefs_box_add_page (LIGMA_PREFS_BOX (stacked),
                                  "system-software-install",
                                  _("System Extensions"),
                                  _("System Extensions"),
                                  LIGMA_HELP_EXTENSIONS_SYSTEM,
                                  NULL,
                                  &top_iter);

  list = ligma_extension_list_new (ligma->extension_manager);
  g_signal_connect (list, "extension-activated",
                    G_CALLBACK (extensions_dialog_extension_activated),
                    stack);
  ligma_extension_list_show_system (LIGMA_EXTENSION_LIST (list));
  gtk_box_pack_start (GTK_BOX (vbox), list, TRUE, TRUE, 1);
  gtk_widget_show (list);

  vbox = ligma_prefs_box_add_page (LIGMA_PREFS_BOX (stacked),
                                  "system-software-install",
                                  _("Install Extensions"),
                                  _("Install Extensions"),
                                  LIGMA_HELP_EXTENSIONS_INSTALL,
                                  NULL,
                                  &top_iter);

  list = ligma_extension_list_new (ligma->extension_manager);
  g_signal_connect (list, "extension-activated",
                    G_CALLBACK (extensions_dialog_extension_activated),
                    stack);
  ligma_extension_list_show_search (LIGMA_EXTENSION_LIST (list), NULL);
  gtk_box_pack_end (GTK_BOX (vbox), list, TRUE, TRUE, 1);
  gtk_widget_show (list);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 1);
  gtk_widget_show (hbox);

  widget = gtk_label_new (_("Search extension:"));
  gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 1);
  gtk_widget_show (widget);

  widget = gtk_entry_new ();
  gtk_entry_set_icon_from_icon_name (GTK_ENTRY (widget),
                                     GTK_ENTRY_ICON_SECONDARY,
                                     "edit-find");
  gtk_entry_set_icon_activatable (GTK_ENTRY (widget),
                                  GTK_ENTRY_ICON_SECONDARY,
                                  TRUE);
  gtk_entry_set_icon_sensitive (GTK_ENTRY (widget),
                                GTK_ENTRY_ICON_SECONDARY,
                                TRUE);
  gtk_entry_set_icon_tooltip_text (GTK_ENTRY (widget),
                                   GTK_ENTRY_ICON_SECONDARY,
                                   _("Search extensions matching these keywords"));
  g_signal_connect (widget, "activate",
                    G_CALLBACK (extensions_dialog_search_activate),
                    list);
  g_signal_connect (widget, "icon-press",
                    G_CALLBACK (extensions_dialog_search_icon_pressed),
                    list);

  gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 1);
  gtk_widget_show (widget);

  /* The extension details. */

  stacked = ligma_extension_details_new ();
  gtk_stack_add_named (GTK_STACK (stack), stacked,
                       LIGMA_EXTENSION_DETAILS_STACK_CHILD);
  gtk_widget_show (stacked);

  gtk_stack_set_visible_child_name (GTK_STACK (stack),
                                    LIGMA_EXTENSION_LIST_STACK_CHILD);
  return dialog;
}

static void
extensions_dialog_response (GtkWidget  *widget,
                            gint        response_id,
                            GtkWidget  *dialog)
{
  gtk_widget_destroy (dialog);
}

static void
extensions_dialog_search_activate (GtkEntry *entry,
                                   gpointer  user_data)
{
  LigmaExtensionList *list = user_data;

  ligma_extension_list_show_search  (list, gtk_entry_get_text (entry));
}

static void
extensions_dialog_search_icon_pressed (GtkEntry             *entry,
                                       GtkEntryIconPosition  icon_pos,
                                       GdkEvent             *event,
                                       gpointer              user_data)
{
  extensions_dialog_search_activate (entry, user_data);
}

static void
extensions_dialog_extension_activated (LigmaExtensionList *list,
                                       LigmaExtension     *extension,
                                       GtkStack          *stack)
{
  GtkWidget *dialog = gtk_widget_get_toplevel (GTK_WIDGET (stack));
  GtkWidget *header_bar;
  GtkWidget *widget;

  /* Add a back button to the header bar. */
  header_bar = gtk_window_get_titlebar (GTK_WINDOW (dialog));
  widget = gtk_button_new_from_icon_name ("go-previous", GTK_ICON_SIZE_SMALL_TOOLBAR);
  g_signal_connect (widget, "clicked",
                    G_CALLBACK (extensions_dialog_back_button_clicked),
                    stack);
  gtk_widget_show (widget);
  gtk_header_bar_pack_start (GTK_HEADER_BAR (header_bar), widget);

  /* Show the details of the extension. */
  widget = gtk_stack_get_child_by_name (stack, LIGMA_EXTENSION_DETAILS_STACK_CHILD);
  ligma_extension_details_set (LIGMA_EXTENSION_DETAILS (widget),
                              extension);

  gtk_stack_set_visible_child_name (stack,
                                    LIGMA_EXTENSION_DETAILS_STACK_CHILD);
}

static void
extensions_dialog_back_button_clicked (GtkButton *button,
                                       GtkStack  *stack)
{
  gtk_stack_set_visible_child_name (stack,
                                    LIGMA_EXTENSION_LIST_STACK_CHILD);
  gtk_widget_destroy (GTK_WIDGET (button));
}
