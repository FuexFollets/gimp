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

#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaitemstack.h"

#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaviewabledialog.h"

#include "image-merge-layers-dialog.h"

#include "ligma-intl.h"


typedef struct _ImageMergeLayersDialog ImageMergeLayersDialog;

struct _ImageMergeLayersDialog
{
  LigmaImage               *image;
  LigmaContext             *context;
  LigmaMergeType            merge_type;
  gboolean                 merge_active_group;
  gboolean                 discard_invisible;
  LigmaMergeLayersCallback  callback;
  gpointer                 user_data;
};


/*  local function prototypes  */

static void  image_merge_layers_dialog_free     (ImageMergeLayersDialog *private);
static void  image_merge_layers_dialog_response (GtkWidget              *dialog,
                                                 gint                    response_id,
                                                 ImageMergeLayersDialog *private);


/*  public functions  */

GtkWidget *
image_merge_layers_dialog_new (LigmaImage               *image,
                               LigmaContext             *context,
                               GtkWidget               *parent,
                               LigmaMergeType            merge_type,
                               gboolean                 merge_active_group,
                               gboolean                 discard_invisible,
                               LigmaMergeLayersCallback  callback,
                               gpointer                 user_data)
{
  ImageMergeLayersDialog *private;
  GtkWidget              *dialog;
  GtkWidget              *vbox;
  GtkWidget              *frame;
  GtkWidget              *button;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  private = g_slice_new0 (ImageMergeLayersDialog);

  private->image              = image;
  private->context            = context;
  private->merge_type         = merge_type;
  private->merge_active_group = merge_active_group;
  private->discard_invisible  = discard_invisible;
  private->callback           = callback;
  private->user_data          = user_data;

  dialog = ligma_viewable_dialog_new (g_list_prepend (NULL, image), context,
                                     _("Merge Layers"), "ligma-image-merge-layers",
                                     LIGMA_ICON_LAYER_MERGE_DOWN,
                                     _("Layers Merge Options"),
                                     parent,
                                     ligma_standard_help_func,
                                     LIGMA_HELP_IMAGE_MERGE_LAYERS,

                                     _("_Cancel"), GTK_RESPONSE_CANCEL,
                                     _("_Merge"),  GTK_RESPONSE_OK,

                                     NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) image_merge_layers_dialog_free, private);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (image_merge_layers_dialog_response),
                    private);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  frame =
    ligma_enum_radio_frame_new_with_range (LIGMA_TYPE_MERGE_TYPE,
                                          LIGMA_EXPAND_AS_NECESSARY,
                                          LIGMA_CLIP_TO_BOTTOM_LAYER,
                                          gtk_label_new (_("Final, Merged Layer should be:")),
                                          G_CALLBACK (ligma_radio_button_update),
                                          &private->merge_type, NULL,
                                          &button);
  ligma_int_radio_group_set_active (GTK_RADIO_BUTTON (button),
                                   private->merge_type);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  button = gtk_check_button_new_with_mnemonic (_("Merge within active _groups only"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->merge_active_group);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &private->merge_active_group);

  if (ligma_item_stack_is_flat (LIGMA_ITEM_STACK (ligma_image_get_layers (image))))
    gtk_widget_set_sensitive (button, FALSE);

  button = gtk_check_button_new_with_mnemonic (_("_Discard invisible layers"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button),
                                private->discard_invisible);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &private->discard_invisible);

  return dialog;
}


/*  private functions  */

static void
image_merge_layers_dialog_free (ImageMergeLayersDialog *private)
{
  g_slice_free (ImageMergeLayersDialog, private);
}

static void
image_merge_layers_dialog_response (GtkWidget              *dialog,
                                    gint                    response_id,
                                    ImageMergeLayersDialog *private)
{
  if (response_id == GTK_RESPONSE_OK)
    {
      private->callback (dialog,
                         private->image,
                         private->context,
                         private->merge_type,
                         private->merge_active_group,
                         private->discard_invisible,
                         private->user_data);
    }
  else
    {
      gtk_widget_destroy (dialog);
    }
}
