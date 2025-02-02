/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamessagedialog.c
 * Copyright (C) 2004 Sven Neumann <sven@ligma.org>
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

#include "ligmamessagebox.h"
#include "ligmamessagedialog.h"


G_DEFINE_TYPE (LigmaMessageDialog, ligma_message_dialog, LIGMA_TYPE_DIALOG)


static void
ligma_message_dialog_class_init (LigmaMessageDialogClass *klass)
{
}

static void
ligma_message_dialog_init (LigmaMessageDialog *dialog)
{
}


/*  public functions  */

GtkWidget *
ligma_message_dialog_new (const gchar    *title,
                         const gchar    *icon_name,
                         GtkWidget      *parent,
                         GtkDialogFlags  flags,
                         LigmaHelpFunc    help_func,
                         const gchar    *help_id,
                         ...)
{
  LigmaMessageDialog *dialog;
  va_list            args;
  gboolean           use_header_bar;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (LIGMA_TYPE_MESSAGE_DIALOG,
                         "title",          title,
                         "role",           "ligma-message-dialog",
                         "modal",          (flags & GTK_DIALOG_MODAL),
                         "help-func",      help_func,
                         "help-id",        help_id,
                         "use-header-bar", use_header_bar,
                         NULL);

  if (parent)
    {
      if (! GTK_IS_WINDOW (parent))
        parent = gtk_widget_get_toplevel (parent);

      if (GTK_IS_WINDOW (parent))
        {
          gtk_window_set_transient_for (GTK_WINDOW (dialog),
                                        GTK_WINDOW (parent));

          if (flags & GTK_DIALOG_DESTROY_WITH_PARENT)
            gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
        }
      else
        {
          gtk_window_set_screen (GTK_WINDOW (dialog),
                                 gtk_widget_get_screen (parent));
        }
    }

  va_start (args, help_id);

  ligma_dialog_add_buttons_valist (LIGMA_DIALOG (dialog), args);

  va_end (args);

  dialog->box = g_object_new (LIGMA_TYPE_MESSAGE_BOX,
                              "icon-name", icon_name,
                              NULL);

  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      GTK_WIDGET (dialog->box), FALSE, FALSE, 0);
  gtk_widget_show (GTK_WIDGET (dialog->box));

  return GTK_WIDGET (dialog);
}
