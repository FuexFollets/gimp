/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaenumwidgets.c
 * Copyright (C) 2002-2004  Sven Neumann <sven@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "ligmawidgetstypes.h"

#include "ligmaenumwidgets.h"
#include "ligmaframe.h"
#include "ligmahelpui.h"


/**
 * SECTION: ligmaenumwidgets
 * @title: LigmaEnumWidgets
 * @short_description: A set of utility functions to create widgets
 *                     based on enums.
 *
 * A set of utility functions to create widgets based on enums.
 **/


/**
 * ligma_enum_radio_box_new:
 * @enum_type:     the #GType of an enum.
 * @callback: (nullable): a callback to connect to the "toggled" signal of each
 *                        #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @callback_data_destroy: Destroy function for @callback_data.
 * @first_button: (out) (optional) (transfer none):
 *                Returns the first button in the created group.
 *
 * Creates a new group of #GtkRadioButtons representing the enum
 * values.  A group of radiobuttons is a good way to represent enums
 * with up to three or four values. Often it is better to use a
 * #LigmaEnumComboBox instead.
 *
 * Returns: (transfer full): a new #GtkBox holding a group of #GtkRadioButtons.
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_enum_radio_box_new (GType            enum_type,
                         GCallback        callback,
                         gpointer         callback_data,
                         GDestroyNotify   callback_data_destroy,
                         GtkWidget      **first_button)
{
  GEnumClass *enum_class;
  GtkWidget  *vbox;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  vbox = ligma_enum_radio_box_new_with_range (enum_type,
                                             enum_class->minimum,
                                             enum_class->maximum,
                                             callback, callback_data,
                                             callback_data_destroy,
                                             first_button);

  g_type_class_unref (enum_class);

  return vbox;
}

/**
 * ligma_enum_radio_box_new_with_range:
 * @minimum:       the minimum enum value
 * @maximum:       the maximum enum value
 * @enum_type:     the #GType of an enum.
 * @callback: (nullable): a callback to connect to the "toggled" signal of each
 *                 #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @callback_data_destroy: Destroy function for @callback_data.
 * @first_button: (out) (optional) (transfer none):
 *                Returns the first button in the created group.
 *
 * Just like ligma_enum_radio_box_new(), this function creates a group
 * of radio buttons, but additionally it supports limiting the range
 * of available enum values.
 *
 * Returns: (transfer full): a new vertical #GtkBox holding a group of
 *                           #GtkRadioButtons
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_enum_radio_box_new_with_range (GType            enum_type,
                                    gint             minimum,
                                    gint             maximum,
                                    GCallback        callback,
                                    gpointer         callback_data,
                                    GDestroyNotify   callback_data_destroy,
                                    GtkWidget      **first_button)
{
  GtkWidget  *vbox;
  GtkWidget  *button;
  GEnumClass *enum_class;
  GEnumValue *value;
  GSList     *group = NULL;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  g_object_weak_ref (G_OBJECT (vbox),
                     (GWeakNotify) g_type_class_unref, enum_class);

  if (callback_data_destroy)
    g_object_weak_ref (G_OBJECT (vbox),
                       (GWeakNotify) callback_data_destroy, callback_data);

  if (first_button)
    *first_button = NULL;

  for (value = enum_class->values; value->value_name; value++)
    {
      const gchar *desc;

      if (value->value < minimum || value->value > maximum)
        continue;

      desc = ligma_enum_value_get_desc (enum_class, value);

      button = gtk_radio_button_new_with_mnemonic (group, desc);

      if (first_button && *first_button == NULL)
        *first_button = button;

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_object_set_data (G_OBJECT (button), "ligma-item-data",
                         GINT_TO_POINTER (value->value));

      if (callback)
        g_signal_connect (button, "toggled",
                          callback,
                          callback_data);
    }

  return vbox;
}

/**
 * ligma_enum_radio_frame_new:
 * @enum_type:     the #GType of an enum.
 * @label_widget: (nullable): a #GtkWidget to use as label for the frame
 *                            that will hold the radio box.
 * @callback: (nullable): a callback to connect to the "toggled" signal of each
 *                        #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @callback_data_destroy: Destroy function for @callback_data.
 * @first_button: (out) (optional) (transfer none):
 *                Returns the first button in the created group.
 *
 * Calls ligma_enum_radio_box_new() and puts the resulting vbox into a
 * #GtkFrame.
 *
 * Returns: (transfer full): a new #GtkFrame holding a group of #GtkRadioButtons.
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_enum_radio_frame_new (GType            enum_type,
                           GtkWidget       *label_widget,
                           GCallback        callback,
                           gpointer         callback_data,
                           GDestroyNotify   callback_data_destroy,
                           GtkWidget      **first_button)
{
  GtkWidget *frame;
  GtkWidget *radio_box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (label_widget == NULL || GTK_IS_WIDGET (label_widget),
                        NULL);

  frame = ligma_frame_new (NULL);

  if (label_widget)
    {
      gtk_frame_set_label_widget (GTK_FRAME (frame), label_widget);
      gtk_widget_show (label_widget);
    }

  radio_box = ligma_enum_radio_box_new (enum_type,
                                       callback, callback_data,
                                       callback_data_destroy,
                                       first_button);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  return frame;
}

/**
 * ligma_enum_radio_frame_new_with_range:
 * @enum_type:     the #GType of an enum.
 * @minimum:       the minimum enum value
 * @maximum:       the maximum enum value
 * @label_widget: (nullable): a widget to put into the frame that will hold the radio box.
 * @callback: (nullable): a callback to connect to the "toggled" signal of each
 *                        #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @callback_data_destroy: Destroy function for @callback_data.
 * @first_button: (out) (optional) (transfer none):
 *                Returns the first button in the created group.
 *
 * Calls ligma_enum_radio_box_new_with_range() and puts the resulting
 * vertical box into a #GtkFrame.
 *
 * Returns: (transfer full): a new #GtkFrame holding a group of #GtkRadioButtons.
 *
 * Since: 2.4
 **/
GtkWidget *
ligma_enum_radio_frame_new_with_range (GType            enum_type,
                                      gint             minimum,
                                      gint             maximum,
                                      GtkWidget       *label_widget,
                                      GCallback        callback,
                                      gpointer         callback_data,
                                      GDestroyNotify   callback_data_destroy,
                                      GtkWidget      **first_button)
{
  GtkWidget *frame;
  GtkWidget *radio_box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (label_widget == NULL || GTK_IS_WIDGET (label_widget),
                        NULL);

  frame = ligma_frame_new (NULL);

  if (label_widget)
    {
      gtk_frame_set_label_widget (GTK_FRAME (frame), label_widget);
      gtk_widget_show (label_widget);
    }

  radio_box = ligma_enum_radio_box_new_with_range (enum_type,
                                                  minimum,
                                                  maximum,
                                                  callback, callback_data,
                                                  callback_data_destroy,
                                                  first_button);
  gtk_container_add (GTK_CONTAINER (frame), radio_box);
  gtk_widget_show (radio_box);

  return frame;
}

/**
 * ligma_enum_icon_box_new:
 * @enum_type:     the #GType of an enum.
 * @icon_prefix:   the prefix of the group of icon names to use.
 * @icon_size:     the icon size for the icons
 * @callback: (nullable): a callback to connect to the "toggled" signal of each
 *                        #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @callback_data_destroy: Destroy function for @callback_data.
 * @first_button: (out) (optional) (transfer none):
 *                Returns the first button in the created group.
 *
 * Creates a horizontal box of radio buttons with named icons. The
 * icon name for each icon is created by appending the enum_value's
 * nick to the given @icon_prefix.
 *
 * Returns: (transfer full): a new horizontal #GtkBox holding a group of #GtkRadioButtons.
 *
 * Since: 2.10
 **/
GtkWidget *
ligma_enum_icon_box_new (GType            enum_type,
                        const gchar     *icon_prefix,
                        GtkIconSize      icon_size,
                        GCallback        callback,
                        gpointer         callback_data,
                        GDestroyNotify   callback_data_destroy,
                        GtkWidget      **first_button)
{
  GEnumClass *enum_class;
  GtkWidget  *box;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);

  enum_class = g_type_class_ref (enum_type);

  box = ligma_enum_icon_box_new_with_range (enum_type,
                                           enum_class->minimum,
                                           enum_class->maximum,
                                           icon_prefix, icon_size,
                                           callback, callback_data,
                                           callback_data_destroy,
                                           first_button);

  g_type_class_unref (enum_class);

  return box;
}

/**
 * ligma_enum_icon_box_new_with_range:
 * @enum_type:     the #GType of an enum.
 * @minimum:       the minumim enum value
 * @maximum:       the maximum enum value
 * @icon_prefix:   the prefix of the group of icon names to use.
 * @icon_size:     the icon size for the icons
 * @callback: (nullable): a callback to connect to the "toggled" signal of each
 *                        #GtkRadioButton that is created.
 * @callback_data: data to pass to the @callback.
 * @callback_data_destroy: Destroy function for @callback_data.
 * @first_button: (out) (optional) (transfer none):
 *                Returns the first button in the created group.
 *
 * Just like ligma_enum_icon_box_new(), this function creates a group
 * of radio buttons, but additionally it supports limiting the range
 * of available enum values.
 *
 * Returns: (transfer full): a new horizontal #GtkBox holding a group of #GtkRadioButtons.
 *
 * Since: 2.10
 **/
GtkWidget *
ligma_enum_icon_box_new_with_range (GType            enum_type,
                                   gint             minimum,
                                   gint             maximum,
                                   const gchar     *icon_prefix,
                                   GtkIconSize      icon_size,
                                   GCallback        callback,
                                   gpointer         callback_data,
                                   GDestroyNotify   callback_data_destroy,
                                   GtkWidget      **first_button)
{
  GtkWidget  *hbox;
  GtkWidget  *button;
  GtkWidget  *image;
  GEnumClass *enum_class;
  GEnumValue *value;
  gchar      *icon_name;
  GSList     *group = NULL;

  g_return_val_if_fail (G_TYPE_IS_ENUM (enum_type), NULL);
  g_return_val_if_fail (icon_prefix != NULL, NULL);

  enum_class = g_type_class_ref (enum_type);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  g_object_weak_ref (G_OBJECT (hbox),
                     (GWeakNotify) g_type_class_unref, enum_class);

  if (callback_data_destroy)
    g_object_weak_ref (G_OBJECT (hbox),
                       (GWeakNotify) callback_data_destroy, callback_data);

  if (first_button)
    *first_button = NULL;

  for (value = enum_class->values; value->value_name; value++)
    {
      if (value->value < minimum || value->value > maximum)
        continue;

      button = gtk_radio_button_new (group);

      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

      if (first_button && *first_button == NULL)
        *first_button = button;

      icon_name = g_strconcat (icon_prefix, "-", value->value_nick, NULL);

      image = gtk_image_new_from_icon_name (icon_name, icon_size);

      g_free (icon_name);

      if (image)
        {
          gtk_container_add (GTK_CONTAINER (button), image);
          gtk_widget_show (image);
        }

      ligma_help_set_help_data (button,
                               ligma_enum_value_get_desc (enum_class, value),
                               NULL);

      group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_object_set_data (G_OBJECT (button), "ligma-item-data",
                         GINT_TO_POINTER (value->value));

      if (callback)
        g_signal_connect (button, "toggled",
                          callback,
                          callback_data);
    }

  return hbox;
}

/**
 * ligma_enum_icon_box_set_child_padding:
 * @icon_box: an icon box widget
 * @xpad:     horizontal padding
 * @ypad:     vertical padding
 *
 * Sets the padding of all buttons in a box created by
 * ligma_enum_icon_box_new().
 *
 * Since: 2.10
 **/
void
ligma_enum_icon_box_set_child_padding (GtkWidget *icon_box,
                                      gint       xpad,
                                      gint       ypad)
{
  GList *children;
  GList *list;

  g_return_if_fail (GTK_IS_CONTAINER (icon_box));

  children = gtk_container_get_children (GTK_CONTAINER (icon_box));

  for (list = children; list; list = g_list_next (list))
    {
      GtkWidget *child = gtk_bin_get_child (GTK_BIN (list->data));
      gint       start, end;
      gint       top, bottom;

      g_object_get (child,
                    "margin-start",  &start,
                    "margin-end",    &end,
                    "margin-top",    &top,
                    "margin-bottom", &bottom,
                    NULL);

      g_object_set (child,
                    "margin-start",  xpad < 0 ? start  : xpad,
                    "margin-end",    xpad < 0 ? end    : xpad,
                    "margin-top",    ypad < 0 ? top    : ypad,
                    "margin-bottom", ypad < 0 ? bottom : ypad,
                    NULL);
    }

  g_list_free (children);
}
