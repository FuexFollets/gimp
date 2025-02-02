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

#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligma.h"

#include "widgets/ligmadeviceeditor.h"
#include "widgets/ligmadevicemanager.h"
#include "widgets/ligmadevices.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmamessagebox.h"
#include "widgets/ligmamessagedialog.h"

#include "input-devices-dialog.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   input_devices_dialog_response (GtkWidget *dialog,
                                             guint      response_id,
                                             Ligma      *ligma);


/*  public functions  */

GtkWidget *
input_devices_dialog_new (Ligma *ligma)
{
  GtkWidget *dialog;
  GtkWidget *content_area;
  GtkWidget *editor;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  dialog = ligma_dialog_new (_("Configure Input Devices"),
                            "ligma-input-devices-dialog",
                            NULL, 0,
                            ligma_standard_help_func,
                            LIGMA_HELP_INPUT_DEVICES,

                            _("_Reset"),  GTK_RESPONSE_REJECT,
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                            GTK_RESPONSE_REJECT,
                                            GTK_RESPONSE_OK,
                                            GTK_RESPONSE_CANCEL,
                                            -1);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (input_devices_dialog_response),
                    ligma);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  editor = ligma_device_editor_new (ligma);
  gtk_container_set_border_width (GTK_CONTAINER (editor), 12);
  gtk_box_pack_start (GTK_BOX (content_area), editor, TRUE, TRUE, 0);
  gtk_widget_show (editor);

  return dialog;
}


/*  private functions  */

static void
input_devices_dialog_response (GtkWidget *dialog,
                               guint      response_id,
                               Ligma      *ligma)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      ligma_devices_save (ligma, TRUE);
      break;

    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_CANCEL:
      ligma_devices_restore (ligma);
      break;

    case GTK_RESPONSE_REJECT:
      {
        GtkWidget *confirm;

        confirm = ligma_message_dialog_new (_("Reset Input Device Configuration"),
                                           LIGMA_ICON_DIALOG_QUESTION,
                                           dialog,
                                           GTK_DIALOG_MODAL |
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           ligma_standard_help_func, NULL,

                                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                                           _("_Reset"),  GTK_RESPONSE_OK,

                                           NULL);

        ligma_dialog_set_alternative_button_order (GTK_DIALOG (confirm),
                                                  GTK_RESPONSE_OK,
                                                  GTK_RESPONSE_CANCEL,
                                                  -1);

        ligma_message_box_set_primary_text (LIGMA_MESSAGE_DIALOG (confirm)->box,
                                           _("Do you really want to reset all "
                                             "input devices to default configuration?"));

        if (ligma_dialog_run (LIGMA_DIALOG (confirm)) == GTK_RESPONSE_OK)
          {
            ligma_device_manager_reset (ligma_devices_get_manager (ligma));
            ligma_devices_save (ligma, TRUE);
            ligma_devices_restore (ligma);
          }
        gtk_widget_destroy (confirm);
      }
      return;

    default:
      break;
    }

  gtk_widget_destroy (dialog);
}
