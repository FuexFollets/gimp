/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoverlaydialog.c
 * Copyright (C) 2009-2010  Michael Natterer <mitch@ligma.org>
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
#include <gdk/gdkkeysyms.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmatoolinfo.h"

#include "ligmaoverlaydialog.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_TITLE,
  PROP_ICON_NAME
};

enum
{
  RESPONSE,
  DETACH,
  CLOSE,
  LAST_SIGNAL
};


typedef struct _ResponseData ResponseData;

struct _ResponseData
{
  gint response_id;
};


static void       ligma_overlay_dialog_constructed   (GObject           *object);
static void       ligma_overlay_dialog_dispose       (GObject           *object);
static void       ligma_overlay_dialog_finalize      (GObject           *object);
static void       ligma_overlay_dialog_set_property  (GObject           *object,
                                                     guint              property_id,
                                                     const GValue      *value,
                                                     GParamSpec        *pspec);
static void       ligma_overlay_dialog_get_property  (GObject           *object,
                                                     guint              property_id,
                                                     GValue            *value,
                                                     GParamSpec        *pspec);

static void       ligma_overlay_dialog_get_preferred_width
                                                    (GtkWidget         *widget,
                                                     gint              *minimum_width,
                                                     gint              *natural_width);
static void       ligma_overlay_dialog_get_preferred_height
                                                    (GtkWidget         *widget,
                                                     gint              *minimum_height,
                                                     gint              *natural_height);
static void       ligma_overlay_dialog_get_preferred_width_for_height
                                                    (GtkWidget         *widget,
                                                     gint               height,
                                                     gint              *minimum_width,
                                                     gint              *natural_width);
static void       ligma_overlay_dialog_get_preferred_height_for_width
                                                    (GtkWidget         *widget,
                                                     gint               width,
                                                     gint              *minimum_height,
                                                     gint              *natural_height);
static void       ligma_overlay_dialog_size_allocate (GtkWidget         *widget,
                                                     GtkAllocation     *allocation);

static void       ligma_overlay_dialog_forall        (GtkContainer      *container,
                                                     gboolean           include_internals,
                                                     GtkCallback        callback,
                                                     gpointer           callback_data);

static void       ligma_overlay_dialog_detach        (LigmaOverlayDialog *dialog);
static void       ligma_overlay_dialog_real_detach   (LigmaOverlayDialog *dialog);

static void       ligma_overlay_dialog_close         (LigmaOverlayDialog *dialog);
static void       ligma_overlay_dialog_real_close    (LigmaOverlayDialog *dialog);

static ResponseData * get_response_data                    (GtkWidget         *widget,
                                                            gboolean          create);


G_DEFINE_TYPE (LigmaOverlayDialog, ligma_overlay_dialog,
               LIGMA_TYPE_OVERLAY_FRAME)

static guint signals[LAST_SIGNAL] = { 0, };

#define parent_class ligma_overlay_dialog_parent_class


static void
ligma_overlay_dialog_class_init (LigmaOverlayDialogClass *klass)
{
  GObjectClass      *object_class    = G_OBJECT_CLASS (klass);
  GtkWidgetClass    *widget_class    = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  object_class->constructed   = ligma_overlay_dialog_constructed;
  object_class->dispose       = ligma_overlay_dialog_dispose;
  object_class->finalize      = ligma_overlay_dialog_finalize;
  object_class->get_property  = ligma_overlay_dialog_get_property;
  object_class->set_property  = ligma_overlay_dialog_set_property;

  widget_class->get_preferred_width  = ligma_overlay_dialog_get_preferred_width;
  widget_class->get_preferred_height = ligma_overlay_dialog_get_preferred_height;
  widget_class->get_preferred_width_for_height = ligma_overlay_dialog_get_preferred_width_for_height;
  widget_class->get_preferred_height_for_width = ligma_overlay_dialog_get_preferred_height_for_width;
  widget_class->size_allocate        = ligma_overlay_dialog_size_allocate;

  container_class->forall            = ligma_overlay_dialog_forall;

  klass->detach               = ligma_overlay_dialog_real_detach;
  klass->close                = ligma_overlay_dialog_real_close;

  g_object_class_install_property (object_class, PROP_TITLE,
                                   g_param_spec_string ("title",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name",
                                                        NULL, NULL,
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));

  signals[RESPONSE] =
    g_signal_new ("response",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (LigmaOverlayDialogClass, response),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  signals[DETACH] =
    g_signal_new ("detach",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (LigmaOverlayDialogClass, detach),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  signals[CLOSE] =
    g_signal_new ("close",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (LigmaOverlayDialogClass, close),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gtk_binding_entry_add_signal (gtk_binding_set_by_class (klass),
                                GDK_KEY_Escape, 0, "close", 0);

  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "LigmaOverlayDialog");
}

static void
ligma_overlay_dialog_init (LigmaOverlayDialog *dialog)
{
  dialog->header = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_parent (dialog->header, GTK_WIDGET (dialog));
  gtk_widget_show (dialog->header);

  dialog->action_area = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog->action_area),
                             GTK_BUTTONBOX_END);
  gtk_widget_set_parent (dialog->action_area, GTK_WIDGET (dialog));
  gtk_widget_show (dialog->action_area);
}

static void
ligma_overlay_dialog_constructed (GObject *object)
{
  LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (object);
  GtkWidget         *label;
  GtkWidget         *button;
  GtkWidget         *image;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->icon_image = image = gtk_image_new_from_icon_name (dialog->icon_name,
                                                             GTK_ICON_SIZE_MENU);
  gtk_box_pack_start (GTK_BOX (dialog->header), image, FALSE, FALSE, 0);
  gtk_widget_show (image);

  dialog->title_label = label = gtk_label_new (dialog->title);
  ligma_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (dialog->header), label, TRUE, TRUE, 0);
  gtk_widget_show (label);

  dialog->close_button = button = gtk_button_new ();
  gtk_widget_set_can_focus (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_end (GTK_BOX (dialog->header), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_CLOSE, GTK_ICON_SIZE_MENU);
  gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect_object (button, "clicked",
                           G_CALLBACK (ligma_overlay_dialog_close),
                           G_OBJECT (dialog),
                           G_CONNECT_SWAPPED);

  dialog->detach_button = button = gtk_button_new ();
  gtk_widget_set_can_focus (button, FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_box_pack_end (GTK_BOX (dialog->header), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  ligma_help_set_help_data (dialog->detach_button,
                           _("Detach dialog from canvas"), NULL);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_DETACH,
                                        GTK_ICON_SIZE_MENU);
  gtk_image_set_pixel_size (GTK_IMAGE (image), 12);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect_object (button, "clicked",
                           G_CALLBACK (ligma_overlay_dialog_detach),
                           G_OBJECT (dialog),
                           G_CONNECT_SWAPPED);
}

static void
ligma_overlay_dialog_dispose (GObject *object)
{
  LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (object);

  if (dialog->header)
    {
      gtk_widget_unparent (dialog->header);
      dialog->header = NULL;
    }

  if (dialog->action_area)
    {
      gtk_widget_unparent (dialog->action_area);
      dialog->action_area = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_overlay_dialog_finalize (GObject *object)
{
  LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (object);

  g_clear_pointer (&dialog->title,     g_free);
  g_clear_pointer (&dialog->icon_name, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_overlay_dialog_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_free (dialog->title);
      dialog->title = g_value_dup_string (value);
      if (dialog->title_label)
        gtk_label_set_text (GTK_LABEL (dialog->title_label), dialog->title);
      break;

    case PROP_ICON_NAME:
      g_free (dialog->icon_name);
      dialog->icon_name = g_value_dup_string (value);
      if (dialog->icon_image)
        gtk_image_set_from_icon_name (GTK_IMAGE (dialog->icon_image),
                                      dialog->icon_name, GTK_ICON_SIZE_MENU);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_overlay_dialog_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, dialog->title);
      break;

    case PROP_ICON_NAME:
      g_value_set_string (value, dialog->icon_name);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_overlay_dialog_get_preferred_width (GtkWidget *widget,
                                         gint      *minimum_width,
                                         gint      *natural_width)
{
  LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (widget);
  gint               header_minimum;
  gint               header_natural;
  gint               action_minimum;
  gint               action_natural;

  GTK_WIDGET_CLASS (parent_class)->get_preferred_width (widget,
                                                        minimum_width,
                                                        natural_width);

  gtk_widget_get_preferred_width (dialog->header,
                                  &header_minimum, &header_natural);
  gtk_widget_get_preferred_width (dialog->action_area,
                                  &action_minimum, &action_natural);

  *minimum_width = MAX (MAX (*minimum_width, action_minimum), header_minimum);
  *natural_width = MAX (MAX (*natural_width, action_natural), header_natural);
}

static void
ligma_overlay_dialog_get_preferred_height (GtkWidget *widget,
                                          gint      *minimum_height,
                                          gint      *natural_height)
{
  LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (widget);
  gint               header_minimum;
  gint               header_natural;
  gint               action_minimum;
  gint               action_natural;
  gint               border_width;

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  GTK_WIDGET_CLASS (parent_class)->get_preferred_height (widget,
                                                         minimum_height,
                                                         natural_height);

  gtk_widget_get_preferred_height (dialog->header,
                                   &header_minimum, &header_natural);
  gtk_widget_get_preferred_height (dialog->action_area,
                                   &action_minimum, &action_natural);

  *minimum_height += header_minimum + action_minimum + 2 * border_width;
  *natural_height += header_natural + action_natural + 2 * border_width;
}

static void
ligma_overlay_dialog_get_preferred_width_for_height (GtkWidget *widget,
                                                    gint       height,
                                                    gint      *minimum_width,
                                                    gint      *natural_width)
{
  LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (widget);
  gint               header_minimum;
  gint               header_natural;
  gint               action_minimum;
  gint               action_natural;

  GTK_WIDGET_CLASS (parent_class)->get_preferred_width_for_height (widget,
                                                                   height,
                                                                   minimum_width,
                                                                   natural_width);

  gtk_widget_get_preferred_width (dialog->header,
                                  &header_minimum, &header_natural);
  gtk_widget_get_preferred_width (dialog->action_area,
                                  &action_minimum, &action_natural);

  *minimum_width = MAX (MAX (*minimum_width, action_minimum), header_minimum);
  *natural_width = MAX (MAX (*natural_width, action_natural), header_natural);
}

static void
ligma_overlay_dialog_get_preferred_height_for_width (GtkWidget *widget,
                                                    gint       width,
                                                    gint      *minimum_height,
                                                    gint      *natural_height)
{
  LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (widget);
  gint               header_minimum;
  gint               header_natural;
  gint               action_minimum;
  gint               action_natural;
  gint               border_width;

  border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

  GTK_WIDGET_CLASS (parent_class)->get_preferred_height_for_width (widget,
                                                                   width,
                                                                   minimum_height,
                                                                   natural_height);

  gtk_widget_get_preferred_height_for_width (dialog->header,
                                             width - 2 * border_width,
                                             &header_minimum, &header_natural);
  gtk_widget_get_preferred_height_for_width (dialog->action_area,
                                             width - 2 * border_width,
                                             &action_minimum, &action_natural);

  *minimum_height += header_minimum + action_minimum + 2 * border_width;
  *natural_height += header_natural + action_natural + 2 * border_width;
}

static void
ligma_overlay_dialog_size_allocate (GtkWidget     *widget,
                                   GtkAllocation *allocation)
{
  GtkContainer      *container = GTK_CONTAINER (widget);
  LigmaOverlayDialog *dialog    = LIGMA_OVERLAY_DIALOG (widget);
  GtkWidget         *child     = gtk_bin_get_child (GTK_BIN (widget));
  GtkRequisition     header_requisition;
  GtkRequisition     action_requisition;
  GtkAllocation      child_allocation = { 0, };
  GtkAllocation      header_allocation;
  GtkAllocation      action_allocation;
  gint               border_width;

  gtk_widget_set_allocation (widget, allocation);

  border_width = gtk_container_get_border_width (container);

  gtk_widget_get_preferred_size (dialog->header,      &header_requisition, NULL);
  gtk_widget_get_preferred_size (dialog->action_area, &action_requisition, NULL);

  if (child && gtk_widget_get_visible (child))
    {
      child_allocation.x      = allocation->x + border_width;
      child_allocation.y      = (allocation->y + 2 * border_width +
                                 header_requisition.height);
      child_allocation.width  = MAX (allocation->width  - 2 * border_width, 0);
      child_allocation.height = MAX (allocation->height -
                                     4 * border_width -
                                     header_requisition.height -
                                     action_requisition.height, 0);

      gtk_widget_size_allocate (child, &child_allocation);
    }

  header_allocation.x = allocation->x + border_width;
  header_allocation.y = allocation->y + border_width;
  header_allocation.width  = MAX (allocation->width  - 2 * border_width, 0);
  header_allocation.height = header_requisition.height;

  gtk_widget_size_allocate (dialog->header, &header_allocation);

  action_allocation.x = allocation->x + border_width;
  action_allocation.y = (child_allocation.y + child_allocation.height +
                         border_width);
  action_allocation.width  = MAX (allocation->width  - 2 * border_width, 0);
  action_allocation.height = action_requisition.height;

  gtk_widget_size_allocate (dialog->action_area, &action_allocation);
}

static void
ligma_overlay_dialog_forall (GtkContainer *container,
                            gboolean      include_internals,
                            GtkCallback   callback,
                            gpointer      callback_data)
{
  GTK_CONTAINER_CLASS (parent_class)->forall (container, include_internals,
                                              callback, callback_data);

  if (include_internals)
    {
      LigmaOverlayDialog *dialog = LIGMA_OVERLAY_DIALOG (container);

      if (dialog->header)
        (* callback) (dialog->header, callback_data);

      if (dialog->action_area)
        (* callback) (dialog->action_area, callback_data);
    }
}

static void
ligma_overlay_dialog_detach (LigmaOverlayDialog *dialog)
{
  g_signal_emit (dialog, signals[DETACH], 0);
}

static void
ligma_overlay_dialog_real_detach (LigmaOverlayDialog *dialog)
{
  ligma_overlay_dialog_response (dialog, LIGMA_RESPONSE_DETACH);
}

static void
ligma_overlay_dialog_close (LigmaOverlayDialog *dialog)
{
  g_signal_emit (dialog, signals[CLOSE], 0);
}

static void
ligma_overlay_dialog_real_close (LigmaOverlayDialog *dialog)
{
  ligma_overlay_dialog_response (dialog, GTK_RESPONSE_DELETE_EVENT);
}

GtkWidget *
ligma_overlay_dialog_new (LigmaToolInfo *tool_info,
                         const gchar  *desc,
                         ...)
{
  LigmaOverlayDialog *dialog;
  const gchar       *icon_name;
  va_list            args;

  g_return_val_if_fail (LIGMA_IS_TOOL_INFO (tool_info), NULL);

  icon_name = ligma_viewable_get_icon_name (LIGMA_VIEWABLE (tool_info));

  dialog = g_object_new (LIGMA_TYPE_OVERLAY_DIALOG,
                         "title",     tool_info->label,
                         "icon-name", icon_name,
                         NULL);

  va_start (args, desc);
  ligma_overlay_dialog_add_buttons_valist (dialog, args);
  va_end (args);

  return GTK_WIDGET (dialog);
}

void
ligma_overlay_dialog_response (LigmaOverlayDialog *dialog,
                              gint               response_id)
{
  g_return_if_fail (LIGMA_IS_OVERLAY_DIALOG (dialog));

  g_signal_emit (dialog, signals[RESPONSE], 0,
                 response_id);
}

void
ligma_overlay_dialog_add_buttons_valist (LigmaOverlayDialog *dialog,
                                        va_list            args)
{
  const gchar *button_text;
  gint         response_id;

  g_return_if_fail (LIGMA_IS_OVERLAY_DIALOG (dialog));

  while ((button_text = va_arg (args, const gchar *)))
    {
      response_id = va_arg (args, gint);

      ligma_overlay_dialog_add_button (dialog, button_text, response_id);
    }
}

static void
action_widget_activated (GtkWidget         *widget,
                         LigmaOverlayDialog *dialog)
{
  ResponseData *ad = get_response_data (widget, FALSE);

  ligma_overlay_dialog_response (dialog, ad->response_id);
}

GtkWidget *
ligma_overlay_dialog_add_button (LigmaOverlayDialog *dialog,
                                const gchar       *button_text,
                                gint               response_id)
{
  GtkWidget    *button;
  ResponseData *ad;
  guint         signal_id;
  GClosure     *closure;

  g_return_val_if_fail (LIGMA_IS_OVERLAY_DIALOG (dialog), NULL);
  g_return_val_if_fail (button_text != NULL, NULL);

  if (response_id == GTK_RESPONSE_CANCEL ||
      response_id == GTK_RESPONSE_CLOSE  ||
      response_id == LIGMA_RESPONSE_DETACH)
    return NULL;

  button = gtk_button_new_with_mnemonic (button_text);
  gtk_widget_set_can_default (button, TRUE);
  gtk_widget_show (button);

  ad = get_response_data (button, TRUE);

  ad->response_id = response_id;

  signal_id = g_signal_lookup ("clicked", GTK_TYPE_BUTTON);

  closure = g_cclosure_new_object (G_CALLBACK (action_widget_activated),
                                   G_OBJECT (dialog));
  g_signal_connect_closure_by_id (button, signal_id, 0,
                                  closure, FALSE);

  gtk_box_pack_end (GTK_BOX (dialog->action_area), button, FALSE, TRUE, 0);

  if (response_id == GTK_RESPONSE_HELP)
    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (dialog->action_area),
                                        button, TRUE);

  return button;
}

void
ligma_overlay_dialog_set_alternative_button_order (LigmaOverlayDialog *overlay,
                                                  gint               n_ids,
                                                  gint              *ids)
{
  /* TODO */
}

void
ligma_overlay_dialog_set_default_response (LigmaOverlayDialog *overlay,
                                          gint               response_id)
{
  /* TODO */
}

void
ligma_overlay_dialog_set_response_sensitive (LigmaOverlayDialog *overlay,
                                            gint               response_id,
                                            gboolean           sensitive)
{
  GList *children;
  GList *list;

  g_return_if_fail (LIGMA_IS_OVERLAY_DIALOG (overlay));

  if (response_id == GTK_RESPONSE_CANCEL ||
      response_id == GTK_RESPONSE_CLOSE)
    {
      gtk_widget_set_sensitive (overlay->close_button, sensitive);
    }

  if (response_id == LIGMA_RESPONSE_DETACH)
    {
      gtk_widget_set_sensitive (overlay->detach_button, sensitive);
    }

  children = gtk_container_get_children (GTK_CONTAINER (overlay->action_area));

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget    *child = list->data;
      ResponseData *ad    = get_response_data (child, FALSE);

      if (ad && ad->response_id == response_id)
        {
          gtk_widget_set_sensitive (child, sensitive);
          break;
        }
    }

  g_list_free (children);
}

static void
response_data_free (gpointer data)
{
  g_slice_free (ResponseData, data);
}

static ResponseData *
get_response_data (GtkWidget *widget,
                   gboolean   create)
{
  ResponseData *ad = g_object_get_data (G_OBJECT (widget),
                                        "ligma-overlay-dialog-response-data");

  if (! ad && create)
    {
      ad = g_slice_new (ResponseData);

      g_object_set_data_full (G_OBJECT (widget),
                              "ligma-overlay-dialog-response-data",
                              ad, response_data_free);
    }

  return ad;
}
