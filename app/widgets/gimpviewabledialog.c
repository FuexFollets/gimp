/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaviewabledialog.c
 * Copyright (C) 2000 Michael Natterer <mitch@ligma.org>
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
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmaitem.h"

#include "ligmaview.h"
#include "ligmaviewabledialog.h"
#include "ligmaviewrenderer.h"


enum
{
  PROP_0,
  PROP_VIEWABLES,
  PROP_CONTEXT,
  PROP_ICON_NAME,
  PROP_DESC
};


static void   ligma_viewable_dialog_dispose      (GObject            *object);
static void   ligma_viewable_dialog_set_property (GObject            *object,
                                                 guint               property_id,
                                                 const GValue       *value,
                                                 GParamSpec         *pspec);
static void   ligma_viewable_dialog_get_property (GObject            *object,
                                                 guint               property_id,
                                                 GValue             *value,
                                                 GParamSpec         *pspec);

static void   ligma_viewable_dialog_name_changed (LigmaObject         *object,
                                                 LigmaViewableDialog *dialog);
static void   ligma_viewable_dialog_close        (LigmaViewableDialog *dialog);


G_DEFINE_TYPE (LigmaViewableDialog, ligma_viewable_dialog, LIGMA_TYPE_DIALOG)

#define parent_class ligma_viewable_dialog_parent_class


static void
ligma_viewable_dialog_class_init (LigmaViewableDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = ligma_viewable_dialog_dispose;
  object_class->get_property = ligma_viewable_dialog_get_property;
  object_class->set_property = ligma_viewable_dialog_set_property;

  g_object_class_install_property (object_class, PROP_VIEWABLES,
                                   g_param_spec_pointer ("viewables", NULL, NULL,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CONTEXT,
                                   g_param_spec_object ("context", NULL, NULL,
                                                        LIGMA_TYPE_CONTEXT,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name", NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_DESC,
                                   g_param_spec_string ("description", NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_WRITABLE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_viewable_dialog_init (LigmaViewableDialog *dialog)
{
  GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (content_area), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  dialog->icon = gtk_image_new ();
  gtk_widget_set_valign (dialog->icon, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (hbox), dialog->icon, FALSE, FALSE, 0);
  gtk_widget_show (dialog->icon);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);
  gtk_widget_show (vbox);

  dialog->desc_label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (dialog->desc_label), 0.0);
  ligma_label_set_attributes (GTK_LABEL (dialog->desc_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->desc_label, FALSE, FALSE, 0);
  gtk_widget_show (dialog->desc_label);

  dialog->viewable_label = g_object_new (GTK_TYPE_LABEL,
                                         "xalign",    0.0,
                                         "yalign",    0.5,
                                         "ellipsize", PANGO_ELLIPSIZE_END,
                                         NULL);
  ligma_label_set_attributes (GTK_LABEL (dialog->viewable_label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->viewable_label, FALSE, FALSE, 0);
  gtk_widget_show (dialog->viewable_label);
}

static void
ligma_viewable_dialog_dispose (GObject *object)
{
  LigmaViewableDialog *dialog = LIGMA_VIEWABLE_DIALOG (object);

  if (dialog->view)
    ligma_viewable_dialog_set_viewables (dialog, NULL, NULL);

  g_list_free (dialog->viewables);
  dialog->viewables = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_viewable_dialog_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaViewableDialog *dialog = LIGMA_VIEWABLE_DIALOG (object);

  switch (property_id)
    {
    case PROP_VIEWABLES:
      ligma_viewable_dialog_set_viewables (dialog,
                                          g_value_get_pointer (value),
                                          dialog->context);
      break;

    case PROP_CONTEXT:
      ligma_viewable_dialog_set_viewables (dialog, g_list_copy (dialog->viewables),
                                          g_value_get_object (value));
      break;

    case PROP_ICON_NAME:
      gtk_image_set_from_icon_name (GTK_IMAGE (dialog->icon),
                                    g_value_get_string (value),
                                    GTK_ICON_SIZE_LARGE_TOOLBAR);
      break;

    case PROP_DESC:
      gtk_label_set_text (GTK_LABEL (dialog->desc_label),
                          g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_viewable_dialog_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaViewableDialog *dialog = LIGMA_VIEWABLE_DIALOG (object);

  switch (property_id)
    {
    case PROP_VIEWABLES:
      g_value_set_pointer (value, dialog->viewables);
      break;

    case PROP_CONTEXT:
      g_value_set_object (value, dialog->context);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
ligma_viewable_dialog_new (GList        *viewables,
                          LigmaContext  *context,
                          const gchar  *title,
                          const gchar  *role,
                          const gchar  *icon_name,
                          const gchar  *desc,
                          GtkWidget    *parent,
                          LigmaHelpFunc  help_func,
                          const gchar  *help_id,
                          ...)
{
  LigmaViewableDialog *dialog;
  va_list             args;
  gboolean            use_header_bar;

  g_return_val_if_fail (context == NULL || LIGMA_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (role != NULL, NULL);
  g_return_val_if_fail (parent == NULL || GTK_IS_WIDGET (parent), NULL);

  if (! viewables)
    g_warning ("Use of LigmaViewableDialog with an empty viewable list is deprecated!");

  g_object_get (gtk_settings_get_default (),
                "gtk-dialogs-use-header", &use_header_bar,
                NULL);

  dialog = g_object_new (LIGMA_TYPE_VIEWABLE_DIALOG,
                         "viewables",       viewables,
                         "context",        context,
                         "title",          title,
                         "role",           role,
                         "help-func",      help_func,
                         "help-id",        help_id,
                         "icon-name",      icon_name,
                         "description",    desc,
                         "parent",         parent,
                         "use-header-bar", use_header_bar,
                         NULL);

  va_start (args, help_id);
  ligma_dialog_add_buttons_valist (LIGMA_DIALOG (dialog), args);
  va_end (args);

  return GTK_WIDGET (dialog);
}

/*
 * ligma_viewable_dialog_set_viewables:
 * @dialog:
 * @viewables:
 * @context:
 *
 * Sets @dialog to display contents related to the list of #LigmaViewable
 * @viewables. If this list contains a single viewable, a small preview
 * is also shown.
 * @dialog takes ownership of @viewables and will free the list upon
 * destruction.
 */
void
ligma_viewable_dialog_set_viewables (LigmaViewableDialog *dialog,
                                    GList              *viewables,
                                    LigmaContext        *context)
{
  g_return_if_fail (LIGMA_IS_VIEWABLE_DIALOG (dialog));
  g_return_if_fail (context == NULL || LIGMA_IS_CONTEXT (context));

  dialog->context   = context;
  g_list_free (dialog->viewables);
  dialog->viewables = viewables;

  if (dialog->view)
    {
      LigmaViewable *old_viewable = LIGMA_VIEW (dialog->view)->viewable;

      if (g_list_length (viewables) == 1 && viewables->data == old_viewable)
        {
          ligma_view_renderer_set_context (LIGMA_VIEW (dialog->view)->renderer,
                                          context);
          return;
        }

      gtk_widget_destroy (dialog->view);

      if (old_viewable)
        {
          g_signal_handlers_disconnect_by_func (old_viewable,
                                                ligma_viewable_dialog_name_changed,
                                                dialog);

          g_signal_handlers_disconnect_by_func (old_viewable,
                                                ligma_viewable_dialog_close,
                                                dialog);
        }
    }

  if (g_list_length (viewables) == 1 && viewables->data)
    {
      LigmaViewable *viewable = viewables->data;
      GtkWidget    *box;

      g_return_if_fail (LIGMA_IS_VIEWABLE (viewable));
      g_signal_connect_object (viewable,
                               LIGMA_VIEWABLE_GET_CLASS (viewable)->name_changed_signal,
                               G_CALLBACK (ligma_viewable_dialog_name_changed),
                               dialog,
                               0);

      box = gtk_widget_get_parent (dialog->icon);

      dialog->view = ligma_view_new (context, viewable, 32, 1, TRUE);
      gtk_box_pack_end (GTK_BOX (box), dialog->view, FALSE, FALSE, 2);
      gtk_widget_show (dialog->view);

      g_object_add_weak_pointer (G_OBJECT (dialog->view),
                                 (gpointer) &dialog->view);

      ligma_viewable_dialog_name_changed (LIGMA_OBJECT (viewable), dialog);

      if (LIGMA_IS_ITEM (viewable))
        {
          g_signal_connect_object (viewable, "removed",
                                   G_CALLBACK (ligma_viewable_dialog_close),
                                   dialog,
                                   G_CONNECT_SWAPPED);
        }
      else
        {
          g_signal_connect_object (viewable, "disconnect",
                                   G_CALLBACK (ligma_viewable_dialog_close),
                                   dialog,
                                   G_CONNECT_SWAPPED);
        }
    }
}


/*  private functions  */

static void
ligma_viewable_dialog_name_changed (LigmaObject         *object,
                                   LigmaViewableDialog *dialog)
{
  gchar *name;

  name = ligma_viewable_get_description (LIGMA_VIEWABLE (object), NULL);

  if (LIGMA_IS_ITEM (object))
    {
      LigmaImage *image = ligma_item_get_image (LIGMA_ITEM (object));
      gchar     *tmp;

      tmp = name;
      name = g_strdup_printf ("%s-%d (%s)",
                              tmp,
                              ligma_item_get_id (LIGMA_ITEM (object)),
                              ligma_image_get_display_name (image));
      g_free (tmp);
    }

  gtk_label_set_text (GTK_LABEL (dialog->viewable_label), name);
  g_free (name);
}

static void
ligma_viewable_dialog_close (LigmaViewableDialog *dialog)
{
  g_signal_emit_by_name (dialog, "close");
}
