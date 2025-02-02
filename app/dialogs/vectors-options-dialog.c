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

#include "vectors/ligmavectors.h"

#include "item-options-dialog.h"
#include "vectors-options-dialog.h"

#include "ligma-intl.h"


typedef struct _VectorsOptionsDialog VectorsOptionsDialog;

struct _VectorsOptionsDialog
{
  LigmaVectorsOptionsCallback  callback;
  gpointer                    user_data;
};


/*  local function prototypes  */

static void  vectors_options_dialog_free     (VectorsOptionsDialog *private);
static void  vectors_options_dialog_callback (GtkWidget            *dialog,
                                              LigmaImage            *image,
                                              LigmaItem             *item,
                                              LigmaContext          *context,
                                              const gchar          *item_name,
                                              gboolean              item_visible,
                                              LigmaColorTag          item_color_tag,
                                              gboolean              item_lock_content,
                                              gboolean              item_lock_position,
                                              gpointer              user_data);


/*  public functions  */

GtkWidget *
vectors_options_dialog_new (LigmaImage                  *image,
                            LigmaVectors                *vectors,
                            LigmaContext                *context,
                            GtkWidget                  *parent,
                            const gchar                *title,
                            const gchar                *role,
                            const gchar                *icon_name,
                            const gchar                *desc,
                            const gchar                *help_id,
                            const gchar                *vectors_name,
                            gboolean                    vectors_visible,
                            LigmaColorTag                vectors_color_tag,
                            gboolean                    vectors_lock_content,
                            gboolean                    vectors_lock_position,
                            LigmaVectorsOptionsCallback  callback,
                            gpointer                    user_data)
{
  VectorsOptionsDialog *private;
  GtkWidget            *dialog;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), NULL);
  g_return_val_if_fail (vectors == NULL || LIGMA_IS_VECTORS (vectors), NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (parent), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (icon_name != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);
  g_return_val_if_fail (help_id != NULL, NULL);
  g_return_val_if_fail (callback != NULL, NULL);

  private = g_slice_new0 (VectorsOptionsDialog);

  private->callback  = callback;
  private->user_data = user_data;

  dialog = item_options_dialog_new (image, LIGMA_ITEM (vectors), context,
                                    parent, title, role,
                                    icon_name, desc, help_id,
                                    _("Path _name:"),
                                    LIGMA_ICON_TOOL_PATH,
                                    _("Lock path _strokes"),
                                    _("Lock path _position"),
                                    vectors_name,
                                    vectors_visible,
                                    vectors_color_tag,
                                    vectors_lock_content,
                                    vectors_lock_position,
                                    vectors_options_dialog_callback,
                                    private);

  g_object_weak_ref (G_OBJECT (dialog),
                     (GWeakNotify) vectors_options_dialog_free, private);

  return dialog;
}


/*  private functions  */

static void
vectors_options_dialog_free (VectorsOptionsDialog *private)
{
  g_slice_free (VectorsOptionsDialog, private);
}

static void
vectors_options_dialog_callback (GtkWidget    *dialog,
                                 LigmaImage    *image,
                                 LigmaItem     *item,
                                 LigmaContext  *context,
                                 const gchar  *item_name,
                                 gboolean      item_visible,
                                 LigmaColorTag  item_color_tag,
                                 gboolean      item_lock_content,
                                 gboolean      item_lock_position,
                                 gpointer      user_data)
{
  VectorsOptionsDialog *private = user_data;

  private->callback (dialog,
                     image,
                     LIGMA_VECTORS (item),
                     context,
                     item_name,
                     item_visible,
                     item_color_tag,
                     item_lock_content,
                     item_lock_position,
                     private->user_data);
}
