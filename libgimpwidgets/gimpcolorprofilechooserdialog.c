/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaColorProfileChooserDialog
 * Copyright (C) 2006-2014 Sven Neumann <sven@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
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

#ifdef PLATFORM_OSX
#include <AppKit/AppKit.h>
#endif

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmacolor/ligmacolor.h"

#include "ligmawidgetstypes.h"

#include "ligmacolorprofilechooserdialog.h"
#include "ligmacolorprofileview.h"
#include "ligmadialog.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmacolorprofilechooserdialog
 * @title: LigmaColorProfileChooserDialog
 * @short_description: A file chooser for selecting color profiles.
 *
 * A #GtkFileChooser subclass for selecting color profiles.
 **/


struct _LigmaColorProfileChooserDialogPrivate
{
  LigmaColorProfileView *profile_view;
};


static void     ligma_color_profile_chooser_dialog_constructed    (GObject                       *object);

static gboolean ligma_color_profile_chooser_dialog_delete_event   (GtkWidget                     *widget,
                                                                  GdkEventAny                   *event);

static void     ligma_color_profile_chooser_dialog_add_shortcut   (LigmaColorProfileChooserDialog *dialog);
static void     ligma_color_profile_chooser_dialog_update_preview (LigmaColorProfileChooserDialog *dialog);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaColorProfileChooserDialog,
                            ligma_color_profile_chooser_dialog,
                            GTK_TYPE_FILE_CHOOSER_DIALOG)

#define parent_class ligma_color_profile_chooser_dialog_parent_class


static void
ligma_color_profile_chooser_dialog_class_init (LigmaColorProfileChooserDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed  = ligma_color_profile_chooser_dialog_constructed;

  widget_class->delete_event = ligma_color_profile_chooser_dialog_delete_event;
}

static void
ligma_color_profile_chooser_dialog_init (LigmaColorProfileChooserDialog *dialog)
{
  dialog->priv =
    ligma_color_profile_chooser_dialog_get_instance_private (dialog);
}

static void
ligma_color_profile_chooser_dialog_constructed (GObject *object)
{
  LigmaColorProfileChooserDialog *dialog;
  GtkFileFilter                 *filter;
  GtkWidget                     *scrolled_window;
  GtkWidget                     *profile_view;

  dialog  = LIGMA_COLOR_PROFILE_CHOOSER_DIALOG (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gtk_window_set_role (GTK_WINDOW (dialog), "ligma-profile-chooser-dialog");

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files (*.*)"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("ICC color profile (*.icc, *.icm)"));
  gtk_file_filter_add_pattern (filter, "*.[Ii][Cc][Cc]");
  gtk_file_filter_add_pattern (filter, "*.[Ii][Cc][Mm]");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (dialog), filter);

  /*  the preview widget  */

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_size_request (scrolled_window, 300, -1);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  profile_view = ligma_color_profile_view_new ();
  gtk_container_add (GTK_CONTAINER (scrolled_window), profile_view);
  gtk_widget_show (profile_view);

  dialog->priv->profile_view = LIGMA_COLOR_PROFILE_VIEW (profile_view);

  gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (dialog),
                                       scrolled_window);

  g_signal_connect (dialog, "update-preview",
                    G_CALLBACK (ligma_color_profile_chooser_dialog_update_preview),
                    NULL);
}

static gboolean
ligma_color_profile_chooser_dialog_delete_event (GtkWidget   *widget,
                                                GdkEventAny *event)
{
  return TRUE;
}

GtkWidget *
ligma_color_profile_chooser_dialog_new (const gchar          *title,
                                       GtkWindow            *parent,
                                       GtkFileChooserAction  action)
{
  GtkWidget *dialog;

  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), NULL);

  dialog = g_object_new (LIGMA_TYPE_COLOR_PROFILE_CHOOSER_DIALOG,
                         "title",  title,
                         "action", action,
                         NULL);

  if (parent)
    gtk_window_set_transient_for (GTK_WINDOW (dialog), parent);

  if (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) ==
      GTK_FILE_CHOOSER_ACTION_SAVE)
    {
      gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                              _("_Save"),   GTK_RESPONSE_ACCEPT,
                              NULL);

      gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                      TRUE);
    }
  else
    {
      gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                              _("_Open"),   GTK_RESPONSE_ACCEPT,
                              NULL);
    }

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  ligma_color_profile_chooser_dialog_add_shortcut (LIGMA_COLOR_PROFILE_CHOOSER_DIALOG (dialog));

  return dialog;
}

/* Add shortcuts for default ICC profile locations */
static gboolean
add_shortcut (LigmaColorProfileChooserDialog *dialog,
              const gchar                   *folder)
{
  return (g_file_test (folder, G_FILE_TEST_IS_DIR) &&
          gtk_file_chooser_add_shortcut_folder (GTK_FILE_CHOOSER (dialog),
                                                folder, NULL));
}

static void
ligma_color_profile_chooser_dialog_add_shortcut (LigmaColorProfileChooserDialog *dialog)
{
  gboolean save = (gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) ==
                   GTK_FILE_CHOOSER_ACTION_SAVE);

#ifdef G_OS_WIN32
  {
    const gchar *prefix = g_getenv ("SystemRoot");
    gchar       *folder;

    if (! prefix)
      prefix = "c:\\windows";

    folder = g_strconcat (prefix, "\\system32\\spool\\drivers\\color", NULL);

    add_shortcut (dialog, folder);

    g_free (folder);
  }
#elif defined(PLATFORM_OSX)
  {
    NSAutoreleasePool *pool;
    NSArray           *path;
    NSString          *library_dir;
    gchar             *folder;
    gboolean           folder_set = FALSE;

    pool = [[NSAutoreleasePool alloc] init];

    if (save)
      {
        path = NSSearchPathForDirectoriesInDomains (NSLibraryDirectory,
                                                    NSUserDomainMask, YES);
        library_dir = [path objectAtIndex:0];

        folder = g_build_filename ([library_dir UTF8String],
                                   "ColorSync", "Profiles", NULL);

        folder_set = add_shortcut (dialog, folder);
        g_free (folder);
      }

    if (! folder_set)
      {
        path = NSSearchPathForDirectoriesInDomains (NSLibraryDirectory,
                                                    NSSystemDomainMask, YES);
        library_dir = [path objectAtIndex:0];

        folder = g_build_filename ([library_dir UTF8String],
                                   "ColorSync", "Profiles", NULL);

        add_shortcut (dialog, folder);
        g_free (folder);
      }

    [pool drain];
  }
#else
  {
    gboolean folder_set = FALSE;

    if (save)
      {
        gchar *folder = g_build_filename (g_get_user_data_dir (),
                                          "color", "icc", NULL);

        folder_set = add_shortcut (dialog, folder);

        if (! folder_set)
          {
            g_free (folder);

            /* Some software, like GNOME color, will save profiles in
             * $XDG_DATA_HOME/icc/
             */
            folder = g_build_filename (g_get_user_data_dir (),
                                       "icc", NULL);

            folder_set = add_shortcut (dialog, folder);
          }

        if (! folder_set)
          {
            g_free (folder);
            folder = g_build_filename (g_get_home_dir (),
                                       ".color", "icc", NULL);

            folder_set = add_shortcut (dialog, folder);
          }

        g_free (folder);
      }

    if (! folder_set)
      add_shortcut (dialog, COLOR_PROFILE_DIRECTORY);
  }
#endif
}

static void
ligma_color_profile_chooser_dialog_update_preview (LigmaColorProfileChooserDialog *dialog)
{
  LigmaColorProfile *profile;
  GFile            *file;
  GError           *error = NULL;

  file = gtk_file_chooser_get_preview_file (GTK_FILE_CHOOSER (dialog));

  if (! file)
    {
      ligma_color_profile_view_set_profile (dialog->priv->profile_view, NULL);
      return;
    }

  switch (g_file_query_file_type (file, G_FILE_QUERY_INFO_NONE, NULL))
    {
    case G_FILE_TYPE_REGULAR:
      profile = ligma_color_profile_new_from_file (file, &error);

      if (! profile)
        {
          ligma_color_profile_view_set_error (dialog->priv->profile_view,
                                             error->message);
          g_clear_error (&error);
        }
      else
        {
          ligma_color_profile_view_set_profile (dialog->priv->profile_view,
                                               profile);
          g_object_unref (profile);
        }
      break;

    case G_FILE_TYPE_DIRECTORY:
      ligma_color_profile_view_set_error (dialog->priv->profile_view,
                                         _("Folder"));
      break;

    default:
      ligma_color_profile_view_set_error (dialog->priv->profile_view,
                                         _("Not a regular file."));
      break;
    }

  g_object_unref (file);
}
