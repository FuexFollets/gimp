/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaprogress.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/ligmacontainerentry.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaprogressbox.h"
#include "widgets/ligmawidgets-utils.h"

#include "file-open-location-dialog.h"

#include "ligma-intl.h"


static void      file_open_location_response   (GtkDialog          *dialog,
                                                gint                response_id,
                                                Ligma               *ligma);

static gboolean  file_open_location_completion (GtkEntryCompletion *completion,
                                                const gchar        *key,
                                                GtkTreeIter        *iter,
                                                gpointer            data);


/*  public functions  */

GtkWidget *
file_open_location_dialog_new (Ligma *ligma)
{
  LigmaContext        *context;
  GtkWidget          *dialog;
  GtkWidget          *hbox;
  GtkWidget          *vbox;
  GtkWidget          *image;
  GtkWidget          *label;
  GtkWidget          *entry;
  GtkEntryCompletion *completion;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  dialog = ligma_dialog_new (_("Open Location"),
                            "ligma-file-open-location",
                            NULL, 0,
                            ligma_standard_help_func,
                            LIGMA_HELP_FILE_OPEN_LOCATION,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Open"),   GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG(dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_open_location_response),
                    ligma);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_WEB, GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  label = gtk_label_new (_("Enter location (URI):"));
  gtk_label_set_xalign (GTK_LABEL (label), 0.0);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* we don't want the context to affect the entry, so create
   * a scratch one instead of using e.g. the user context
   */
  context = ligma_context_new (ligma, "file-open-location-dialog", NULL);
  entry = ligma_container_entry_new (ligma->documents, context,
                                    LIGMA_VIEW_SIZE_SMALL, 0);
  g_object_unref (context);

  completion = gtk_entry_get_completion (GTK_ENTRY (entry));
  gtk_entry_completion_set_match_func (completion,
                                       file_open_location_completion,
                                       NULL, NULL);

  gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
  gtk_widget_set_size_request (entry, 400, -1);
  gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  g_object_set_data (G_OBJECT (dialog), "location-entry", entry);

  return dialog;
}


/*  private functions  */

static void
file_open_location_response (GtkDialog *dialog,
                             gint       response_id,
                             Ligma      *ligma)
{
  GtkWidget   *entry;
  GtkWidget   *box;
  const gchar *text = NULL;

  box = g_object_get_data (G_OBJECT (dialog), "progress-box");

  if (response_id != GTK_RESPONSE_OK)
    {
      if (box && LIGMA_PROGRESS_BOX (box)->active)
        ligma_progress_cancel (LIGMA_PROGRESS (box));
      else
        gtk_widget_destroy (GTK_WIDGET (dialog));

      return;
    }

  entry = g_object_get_data (G_OBJECT (dialog), "location-entry");
  text = gtk_entry_get_text (GTK_ENTRY (entry));

  if (text && strlen (text))
    {
      LigmaImage         *image;
      gchar             *filename;
      GFile             *file;
      LigmaPDBStatusType  status;
      GError            *error = NULL;

      filename = g_filename_from_uri (text, NULL, NULL);

      if (filename)
        {
          file = g_file_new_for_uri (text);
          g_free (filename);
        }
      else
        {
          file = file_utils_filename_to_file (ligma, text, &error);
        }

      if (!box)
        {
          box = ligma_progress_box_new ();
          gtk_container_set_border_width (GTK_CONTAINER (box), 12);
          gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                            box, FALSE, FALSE, 0);

          g_object_set_data (G_OBJECT (dialog), "progress-box", box);
        }

      if (file)
        {
          gtk_widget_show (box);

          gtk_editable_set_editable (GTK_EDITABLE (entry), FALSE);
          gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_OK, FALSE);

          image = file_open_with_proc_and_display (ligma,
                                                   ligma_get_user_context (ligma),
                                                   LIGMA_PROGRESS (box),
                                                   file, FALSE, NULL,
                                                   G_OBJECT (ligma_widget_get_monitor (entry)),
                                                   &status, &error);

          gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_OK, TRUE);
          gtk_editable_set_editable (GTK_EDITABLE (entry), TRUE);

          if (image == NULL && status != LIGMA_PDB_CANCEL)
            {
              ligma_message (ligma, G_OBJECT (box), LIGMA_MESSAGE_ERROR,
                            _("Opening '%s' failed:\n\n%s"),
                            ligma_file_get_utf8_name (file), error->message);
              g_clear_error (&error);
            }

          g_object_unref (file);

          if (image != NULL)
            {
              gtk_widget_destroy (GTK_WIDGET (dialog));
              return;
            }
        }
      else
        {
          ligma_message (ligma, G_OBJECT (box), LIGMA_MESSAGE_ERROR,
                        _("Opening '%s' failed:\n\n%s"),
                        text,
                        /* error should never be NULL, also issue #3093 */
                        error ? error->message : _("Invalid URI"));
          g_clear_error (&error);
        }
    }
}

static gboolean
file_open_location_completion (GtkEntryCompletion *completion,
                               const gchar        *key,
                               GtkTreeIter        *iter,
                               gpointer            data)
{
  GtkTreeModel *model = gtk_entry_completion_get_model (completion);
  gchar        *name;
  gchar        *normalized;
  gchar        *case_normalized;
  gboolean      match;

  gtk_tree_model_get (model, iter,
                      1, &name,
                      -1);

  if (! name)
    return FALSE;

  normalized = g_utf8_normalize (name, -1, G_NORMALIZE_ALL);
  case_normalized = g_utf8_casefold (normalized, -1);

  match = (strncmp (key, case_normalized, strlen (key)) == 0);

  if (! match)
    {
      const gchar *colon = strchr (case_normalized, ':');

      if (colon && strlen (colon) > 2 && colon[1] == '/' && colon[2] == '/')
        match = (strncmp (key, colon + 3, strlen (key)) == 0);
    }

  g_free (normalized);
  g_free (case_normalized);
  g_free (name);

  return match;
}
