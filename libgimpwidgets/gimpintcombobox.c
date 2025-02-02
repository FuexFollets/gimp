/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaintcombobox.c
 * Copyright (C) 2004  Sven Neumann <sven@ligma.org>
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

#include <libintl.h>

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "ligmawidgetstypes.h"

#include "ligmaintcombobox.h"
#include "ligmaintstore.h"


/**
 * SECTION: ligmaintcombobox
 * @title: LigmaIntComboBox
 * @short_description: A widget providing a popup menu of integer
 *                     values (e.g. enums).
 *
 * A widget providing a popup menu of integer values (e.g. enums).
 **/


enum
{
  PROP_0,
  PROP_ELLIPSIZE,
  PROP_LABEL,
  PROP_LAYOUT,
  PROP_VALUE
};


struct _LigmaIntComboBoxPrivate
{
  GtkCellRenderer        *text_renderer;

  PangoEllipsizeMode      ellipsize;
  gchar                  *label;
  LigmaIntComboBoxLayout   layout;

  LigmaIntSensitivityFunc  sensitivity_func;
  gpointer                sensitivity_data;
  GDestroyNotify          sensitivity_destroy;
};

#define GET_PRIVATE(obj) (((LigmaIntComboBox *) (obj))->priv)


static void  ligma_int_combo_box_constructed  (GObject         *object);
static void  ligma_int_combo_box_finalize     (GObject         *object);
static void  ligma_int_combo_box_set_property (GObject         *object,
                                              guint            property_id,
                                              const GValue    *value,
                                              GParamSpec      *pspec);
static void  ligma_int_combo_box_get_property (GObject         *object,
                                              guint            property_id,
                                              GValue          *value,
                                              GParamSpec      *pspec);

static void  ligma_int_combo_box_changed      (GtkComboBox     *combo_box,
                                              gpointer         user_data);

static void  ligma_int_combo_box_create_cells (LigmaIntComboBox *combo_box);
static void  ligma_int_combo_box_data_func    (GtkCellLayout   *layout,
                                              GtkCellRenderer *cell,
                                              GtkTreeModel    *model,
                                              GtkTreeIter     *iter,
                                              gpointer         data);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaIntComboBox, ligma_int_combo_box,
                            GTK_TYPE_COMBO_BOX)

#define parent_class ligma_int_combo_box_parent_class


static void
ligma_int_combo_box_class_init (LigmaIntComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_int_combo_box_constructed;
  object_class->finalize     = ligma_int_combo_box_finalize;
  object_class->set_property = ligma_int_combo_box_set_property;
  object_class->get_property = ligma_int_combo_box_get_property;

  /**
   * LigmaIntComboBox:ellipsize:
   *
   * Specifies the preferred place to ellipsize text in the combo-box,
   * if the cell renderer does not have enough room to display the
   * entire string.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class, PROP_ELLIPSIZE,
                                   g_param_spec_enum ("ellipsize",
                                                      "Ellipsize",
                                                      "Ellipsize mode for the used text cell renderer",
                                                      PANGO_TYPE_ELLIPSIZE_MODE,
                                                      PANGO_ELLIPSIZE_NONE,
                                                      LIGMA_PARAM_READWRITE));

  /**
   * LigmaIntComboBox:label:
   *
   * Sets a label on the combo-box, see ligma_int_combo_box_set_label().
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_LABEL,
                                   g_param_spec_string ("label",
                                                        "Label",
                                                        "An optional label to be displayed",
                                                        NULL,
                                                        LIGMA_PARAM_READWRITE));

  /**
   * LigmaIntComboBox:layout:
   *
   * Specifies the combo box layout.
   *
   * Since: 2.10
   */
  g_object_class_install_property (object_class, PROP_LAYOUT,
                                   g_param_spec_enum ("layout",
                                                      "Layout",
                                                      "Combo box layout",
                                                      LIGMA_TYPE_INT_COMBO_BOX_LAYOUT,
                                                      LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED,
                                                      LIGMA_PARAM_READWRITE));

  /**
   * LigmaIntComboBox:value:
   *
   * The active value (different from the "active" property of
   * GtkComboBox which is the active index).
   *
   * Since: 3.0
   */
  g_object_class_install_property (object_class, PROP_VALUE,
                                   g_param_spec_int ("value",
                                                     "Value",
                                                     "Value of active item",
                                                     G_MININT, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE));
}

static void
ligma_int_combo_box_init (LigmaIntComboBox *combo_box)
{
  LigmaIntComboBoxPrivate *priv;
  GtkListStore           *store;

  combo_box->priv = priv = ligma_int_combo_box_get_instance_private (combo_box);

  store = g_object_new (LIGMA_TYPE_INT_STORE, NULL);
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
  g_object_unref (store);

  priv->layout = LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED;

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (ligma_int_combo_box_changed),
                    NULL);
}

static void
ligma_int_combo_box_constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_int_combo_box_create_cells (LIGMA_INT_COMBO_BOX (object));
}

static void
ligma_int_combo_box_finalize (GObject *object)
{
  LigmaIntComboBoxPrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->label, g_free);

  if (priv->sensitivity_destroy)
    {
      GDestroyNotify d = priv->sensitivity_destroy;

      priv->sensitivity_destroy = NULL;
      d (priv->sensitivity_data);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_int_combo_box_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LigmaIntComboBoxPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ELLIPSIZE:
      priv->ellipsize = g_value_get_enum (value);
      if (priv->text_renderer)
        {
          g_object_set_property (G_OBJECT (priv->text_renderer),
                                 pspec->name, value);
        }
      break;
    case PROP_LABEL:
      ligma_int_combo_box_set_label (LIGMA_INT_COMBO_BOX (object),
                                    g_value_get_string (value));
      break;
    case PROP_LAYOUT:
      ligma_int_combo_box_set_layout (LIGMA_INT_COMBO_BOX (object),
                                     g_value_get_enum (value));
      break;
    case PROP_VALUE:
      ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (object),
                                     g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_int_combo_box_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LigmaIntComboBoxPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_ELLIPSIZE:
      g_value_set_enum (value, priv->ellipsize);
      break;
    case PROP_LABEL:
      g_value_set_string (value, priv->label);
      break;
    case PROP_LAYOUT:
      g_value_set_enum (value, priv->layout);
      break;
    case PROP_VALUE:
      {
        gint v;

        ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (object), &v);
        g_value_set_int (value, v);
      }
    break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/**
 * ligma_int_combo_box_new: (skip)
 * @first_label: the label of the first item
 * @first_value: the value of the first item
 * @...: a %NULL terminated list of more label, value pairs
 *
 * Creates a GtkComboBox that has integer values associated with each
 * item. The items to fill the combo box with are specified as a %NULL
 * terminated list of label/value pairs.
 *
 * If you need to construct an empty #LigmaIntComboBox, it's best to use
 * g_object_new (LIGMA_TYPE_INT_COMBO_BOX, NULL).
 *
 * Returns: a new #LigmaIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
ligma_int_combo_box_new (const gchar *first_label,
                        gint         first_value,
                        ...)
{
  GtkWidget *combo_box;
  va_list    args;

  va_start (args, first_value);

  combo_box = ligma_int_combo_box_new_valist (first_label, first_value, args);

  va_end (args);

  return combo_box;
}

/**
 * ligma_int_combo_box_new_valist: (skip)
 * @first_label: the label of the first item
 * @first_value: the value of the first item
 * @values: a va_list with more values
 *
 * A variant of ligma_int_combo_box_new() that takes a va_list of
 * label/value pairs.
 *
 * Returns: a new #LigmaIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
ligma_int_combo_box_new_valist (const gchar *first_label,
                               gint         first_value,
                               va_list      values)
{
  GtkWidget    *combo_box;
  GtkListStore *store;

  store = ligma_int_store_new_valist (first_label, first_value, values);

  combo_box = g_object_new (LIGMA_TYPE_INT_COMBO_BOX,
                            "model", store,
                            NULL);

  g_object_unref (store);

  return combo_box;
}

/**
 * ligma_int_combo_box_new_array: (rename-to ligma_int_combo_box_new)
 * @n_values: the number of values
 * @labels: (array length=n_values): an array of labels
 *
 * A variant of ligma_int_combo_box_new() that takes an array of labels.
 * The array indices are used as values.
 *
 * Returns: a new #LigmaIntComboBox.
 *
 * Since: 2.2
 **/
GtkWidget *
ligma_int_combo_box_new_array (gint         n_values,
                              const gchar *labels[])
{
  GtkWidget    *combo_box;
  GtkListStore *store;
  gint          i;

  g_return_val_if_fail (n_values >= 0, NULL);
  g_return_val_if_fail (labels != NULL || n_values == 0, NULL);

  combo_box = g_object_new (LIGMA_TYPE_INT_COMBO_BOX, NULL);

  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)));

  for (i = 0; i < n_values; i++)
    {
      GtkTreeIter  iter;

      if (labels[i])
        {
          gtk_list_store_append (store, &iter);
          gtk_list_store_set (store, &iter,
                              LIGMA_INT_STORE_VALUE, i,
                              LIGMA_INT_STORE_LABEL, gettext (labels[i]),
                              -1);
        }
    }

  return combo_box;
}

/**
 * ligma_int_combo_box_prepend: (skip)
 * @combo_box: a #LigmaIntComboBox
 * @...:       pairs of column number and value, terminated with -1
 *
 * This function provides a convenient way to prepend items to a
 * #LigmaIntComboBox. It prepends a row to the @combo_box's list store
 * and calls gtk_list_store_set() for you.
 *
 * The column number must be taken from the enum #LigmaIntStoreColumns.
 *
 * Since: 2.2
 **/
void
ligma_int_combo_box_prepend (LigmaIntComboBox *combo_box,
                            ...)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  va_list       args;

  g_return_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box));

  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)));

  va_start (args, combo_box);

  gtk_list_store_prepend (store, &iter);
  gtk_list_store_set_valist (store, &iter, args);

  va_end (args);
}

/**
 * ligma_int_combo_box_append: (skip)
 * @combo_box: a #LigmaIntComboBox
 * @...:       pairs of column number and value, terminated with -1
 *
 * This function provides a convenient way to append items to a
 * #LigmaIntComboBox. It appends a row to the @combo_box's list store
 * and calls gtk_list_store_set() for you.
 *
 * The column number must be taken from the enum #LigmaIntStoreColumns.
 *
 * Since: 2.2
 **/
void
ligma_int_combo_box_append (LigmaIntComboBox *combo_box,
                           ...)
{
  GtkListStore *store;
  GtkTreeIter   iter;
  va_list       args;

  g_return_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box));

  store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)));

  va_start (args, combo_box);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set_valist (store, &iter, args);

  va_end (args);
}

/**
 * ligma_int_combo_box_set_active:
 * @combo_box: a #LigmaIntComboBox
 * @value:     an integer value
 *
 * Looks up the item that belongs to the given @value and makes it the
 * selected item in the @combo_box.
 *
 * Returns: %TRUE on success (value changed or not) or %FALSE if there
 *          was no item for this value.
 *
 * Since: 2.2
 **/
gboolean
ligma_int_combo_box_set_active (LigmaIntComboBox *combo_box,
                               gint             value)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  gint          current_value;

  g_return_val_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box), FALSE);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  if (ligma_int_combo_box_get_active (combo_box, &current_value) &&
      value == current_value)
    {
      /* Guard for identical value to not loop forever between
       * LigmaIntComboBox "value" and GtkComboBox "active" properties.
       */
      return TRUE;
    }
  else if (ligma_int_store_lookup_by_value (model, value, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
      return TRUE;
    }

  return FALSE;
}

/**
 * ligma_int_combo_box_get_active:
 * @combo_box: a #LigmaIntComboBox
 * @value: (out): return location for the integer value
 *
 * Retrieves the value of the selected (active) item in the @combo_box.
 *
 * Returns: %TRUE if @value has been set or %FALSE if no item was
 *               active.
 *
 * Since: 2.2
 **/
gboolean
ligma_int_combo_box_get_active (LigmaIntComboBox *combo_box,
                               gint            *value)
{
  GtkTreeIter  iter;

  g_return_val_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box), FALSE);
  g_return_val_if_fail (value != NULL, FALSE);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)),
                          &iter,
                          LIGMA_INT_STORE_VALUE, value,
                          -1);
      return TRUE;
    }

  return FALSE;
}

/**
 * ligma_int_combo_box_set_active_by_user_data:
 * @combo_box: a #LigmaIntComboBox
 * @user_data: an integer value
 *
 * Looks up the item that has the given @user_data and makes it the
 * selected item in the @combo_box.
 *
 * Returns: %TRUE on success or %FALSE if there was no item for
 *               this user-data.
 *
 * Since: 2.10
 **/
gboolean
ligma_int_combo_box_set_active_by_user_data (LigmaIntComboBox *combo_box,
                                            gpointer         user_data)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  g_return_val_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box), FALSE);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  if (ligma_int_store_lookup_by_user_data (model, user_data, &iter))
    {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);
      return TRUE;
    }

  return FALSE;
}

/**
 * ligma_int_combo_box_get_active_user_data:
 * @combo_box: a #LigmaIntComboBox
 * @user_data: (out): return location for the gpointer value
 *
 * Retrieves the user-data of the selected (active) item in the @combo_box.
 *
 * Returns: %TRUE if @user_data has been set or %FALSE if no item was
 *               active.
 *
 * Since: 2.10
 **/
gboolean
ligma_int_combo_box_get_active_user_data (LigmaIntComboBox *combo_box,
                                         gpointer        *user_data)
{
  GtkTreeIter  iter;

  g_return_val_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box), FALSE);
  g_return_val_if_fail (user_data != NULL, FALSE);

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      gtk_tree_model_get (gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box)),
                          &iter,
                          LIGMA_INT_STORE_USER_DATA, user_data,
                          -1);
      return TRUE;
    }

  return FALSE;
}

/**
 * ligma_int_combo_box_connect:
 * @combo_box:    a #LigmaIntComboBox
 * @value:        the value to set
 * @callback:     a callback to connect to the @combo_box's "changed" signal
 * @data:         a pointer passed as data to g_signal_connect()
 * @data_destroy: Destroy function for @data.
 *
 * A convenience function that sets the initial @value of a
 * #LigmaIntComboBox and connects @callback to the "changed"
 * signal.
 *
 * This function also calls the @callback once after setting the
 * initial @value. This is often convenient when working with combo
 * boxes that select a default active item, like for example
 * ligma_drawable_combo_box_new(). If you pass an invalid initial
 * @value, the @callback will be called with the default item active.
 *
 * Returns: the signal handler ID as returned by g_signal_connect()
 *
 * Since: 2.2
 **/
gulong
ligma_int_combo_box_connect (LigmaIntComboBox *combo_box,
                            gint             value,
                            GCallback        callback,
                            gpointer         data,
                            GDestroyNotify   data_destroy)
{
  gulong handler = 0;

  g_return_val_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box), 0);

  if (callback)
    handler = g_signal_connect (combo_box, "changed", callback, data);

  if (data_destroy)
    g_object_weak_ref (G_OBJECT (combo_box), (GWeakNotify) data_destroy,
                       data);

  if (! ligma_int_combo_box_set_active (combo_box, value))
    g_signal_emit_by_name (combo_box, "changed", NULL);

  return handler;
}

/**
 * ligma_int_combo_box_set_label:
 * @combo_box: a #LigmaIntComboBox
 * @label:     a string to be shown as label
 *
 * Sets a caption on the @combo_box that will be displayed
 * left-aligned inside the box. When a label is set, the remaining
 * contents of the box will be right-aligned. This is useful for
 * places where screen estate is rare, like in tool options.
 *
 * Since: 2.10
 **/
void
ligma_int_combo_box_set_label (LigmaIntComboBox *combo_box,
                              const gchar     *label)
{
  LigmaIntComboBoxPrivate *priv;

  g_return_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box));

  priv = GET_PRIVATE (combo_box);

  if (label == priv->label)
    return;

  if (priv->label)
    {
      g_free (priv->label);
      priv->label = NULL;

      g_signal_handlers_disconnect_by_func (combo_box,
                                            ligma_int_combo_box_create_cells,
                                            NULL);
    }

  if (label)
    {
      priv->label = g_strdup (label);

      g_signal_connect (combo_box, "notify::popup-shown",
                        G_CALLBACK (ligma_int_combo_box_create_cells),
                        NULL);
    }

  ligma_int_combo_box_create_cells (combo_box);

  g_object_notify (G_OBJECT (combo_box), "label");
}

/**
 * ligma_int_combo_box_get_label:
 * @combo_box: a #LigmaIntComboBox
 *
 * Returns the label previously set with ligma_int_combo_box_set_label(),
 * or %NULL,
 *
 * Returns: the @combo_box' label.
 *
 * Since: 2.10
 **/
const gchar *
ligma_int_combo_box_get_label (LigmaIntComboBox *combo_box)
{
  g_return_val_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box), NULL);

  return GET_PRIVATE (combo_box)->label;
}

/**
 * ligma_int_combo_box_set_layout:
 * @combo_box: a #LigmaIntComboBox
 * @layout:    the combo box layout
 *
 * Sets the layout of @combo_box to @layout.
 *
 * Since: 2.10
 **/
void
ligma_int_combo_box_set_layout (LigmaIntComboBox       *combo_box,
                               LigmaIntComboBoxLayout  layout)
{
  LigmaIntComboBoxPrivate *priv;

  g_return_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box));

  priv = GET_PRIVATE (combo_box);

  if (layout == priv->layout)
    return;

  priv->layout = layout;

  ligma_int_combo_box_create_cells (combo_box);

  g_object_notify (G_OBJECT (combo_box), "layout");
}

/**
 * ligma_int_combo_box_get_layout:
 * @combo_box: a #LigmaIntComboBox
 *
 * Returns the layout of @combo_box
 *
 * Returns: the @combo_box's layout.
 *
 * Since: 2.10
 **/
LigmaIntComboBoxLayout
ligma_int_combo_box_get_layout (LigmaIntComboBox *combo_box)
{
  g_return_val_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box),
                        LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED);

  return GET_PRIVATE (combo_box)->layout;
}

/**
 * ligma_int_combo_box_set_sensitivity:
 * @combo_box: a #LigmaIntComboBox
 * @func: a function that returns a boolean value, or %NULL to unset
 * @data: data to pass to @func
 * @destroy: destroy notification for @data
 *
 * Sets a function that is used to decide about the sensitivity of
 * rows in the @combo_box. Use this if you want to set certain rows
 * insensitive.
 *
 * Calling gtk_widget_queue_draw() on the @combo_box will cause the
 * sensitivity to be updated.
 *
 * Since: 2.4
 **/
void
ligma_int_combo_box_set_sensitivity (LigmaIntComboBox        *combo_box,
                                    LigmaIntSensitivityFunc  func,
                                    gpointer                data,
                                    GDestroyNotify          destroy)
{
  LigmaIntComboBoxPrivate *priv;

  g_return_if_fail (LIGMA_IS_INT_COMBO_BOX (combo_box));

  priv = GET_PRIVATE (combo_box);

  if (priv->sensitivity_destroy)
    {
      GDestroyNotify d = priv->sensitivity_destroy;

      priv->sensitivity_destroy = NULL;
      d (priv->sensitivity_data);
    }

  priv->sensitivity_func    = func;
  priv->sensitivity_data    = data;
  priv->sensitivity_destroy = destroy;

  ligma_int_combo_box_create_cells (combo_box);
}


/*  private functions  */

static void
ligma_int_combo_box_changed (GtkComboBox *combo_box,
                            gpointer     user_data)
{
  gint value;

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (combo_box),
                                 &value);
  g_object_set (combo_box, "value", value, NULL);
}

static void
ligma_int_combo_box_create_cells (LigmaIntComboBox *combo_box)
{
  LigmaIntComboBoxPrivate *priv   = GET_PRIVATE (combo_box);
  GtkCellLayout          *layout = GTK_CELL_LAYOUT (combo_box);
  GtkCellRenderer        *text_renderer;
  GtkCellRenderer        *pixbuf_renderer;
  gboolean                popup_shown;

  g_object_get (combo_box, "popup-shown", &popup_shown, NULL);

  gtk_cell_layout_clear (layout);

  priv->text_renderer = NULL;

  if (popup_shown)
    {
      /*  menu layout  */

      pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();
      g_object_set (pixbuf_renderer,
                    "xpad", 2,
                    NULL);

      text_renderer = gtk_cell_renderer_text_new ();

      gtk_cell_layout_pack_start (layout, pixbuf_renderer, FALSE);
      gtk_cell_layout_pack_start (layout, text_renderer, TRUE);

      gtk_cell_layout_set_attributes (layout,
                                      pixbuf_renderer,
                                      "icon-name", LIGMA_INT_STORE_ICON_NAME,
                                      "pixbuf",    LIGMA_INT_STORE_PIXBUF,
                                      NULL);
      gtk_cell_layout_set_attributes (layout,
                                      text_renderer,
                                      "text", LIGMA_INT_STORE_LABEL,
                                      NULL);

      if (priv->sensitivity_func)
        {
          gtk_cell_layout_set_cell_data_func (layout,
                                              pixbuf_renderer,
                                              ligma_int_combo_box_data_func,
                                              priv, NULL);

          gtk_cell_layout_set_cell_data_func (layout,
                                              text_renderer,
                                              ligma_int_combo_box_data_func,
                                              priv, NULL);
        }
    }
  else
    {
      /*  combo box layout  */

      if (priv->layout != LIGMA_INT_COMBO_BOX_LAYOUT_ICON_ONLY)
        {
          priv->text_renderer = text_renderer = gtk_cell_renderer_text_new ();
          g_object_set (priv->text_renderer,
                        "ellipsize", priv->ellipsize,
                        NULL);
        }
      else
        {
          text_renderer = NULL;
        }

      pixbuf_renderer = gtk_cell_renderer_pixbuf_new ();

      if (text_renderer)
        {
          g_object_set (pixbuf_renderer,
                        "xpad", 2,
                        NULL);
        }

      if (priv->label)
        {
          GtkCellRenderer *label_renderer;

          label_renderer = gtk_cell_renderer_text_new ();
          g_object_set (label_renderer,
                        "text", priv->label,
                        NULL);

          gtk_cell_layout_pack_start (layout, label_renderer, FALSE);
          gtk_cell_layout_pack_end (layout, pixbuf_renderer, FALSE);

          if (text_renderer)
            {
              gtk_cell_layout_pack_end (layout, text_renderer, TRUE);

              g_object_set (text_renderer,
                            "xalign", 1.0,
                            NULL);
            }
        }
      else
        {
          gtk_cell_layout_pack_start (layout, pixbuf_renderer, FALSE);

          if (priv->text_renderer)
            gtk_cell_layout_pack_start (layout, text_renderer, TRUE);
        }

      gtk_cell_layout_set_attributes (layout,
                                      pixbuf_renderer,
                                      "icon-name", LIGMA_INT_STORE_ICON_NAME,
                                      NULL);

      if (text_renderer)
        gtk_cell_layout_set_attributes (layout,
                                        text_renderer,
                                        "text", LIGMA_INT_STORE_LABEL,
                                        NULL);

      if (priv->layout == LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED ||
          priv->sensitivity_func)
        {
          gtk_cell_layout_set_cell_data_func (layout,
                                              pixbuf_renderer,
                                              ligma_int_combo_box_data_func,
                                              priv, NULL);

          if (text_renderer)
            gtk_cell_layout_set_cell_data_func (layout,
                                                text_renderer,
                                                ligma_int_combo_box_data_func,
                                                priv, NULL);
        }
    }
}

static void
ligma_int_combo_box_data_func (GtkCellLayout   *layout,
                              GtkCellRenderer *cell,
                              GtkTreeModel    *model,
                              GtkTreeIter     *iter,
                              gpointer         data)
{
  LigmaIntComboBoxPrivate *priv = data;

  if (priv->layout == LIGMA_INT_COMBO_BOX_LAYOUT_ABBREVIATED &&
      cell == priv->text_renderer)
    {
      gchar *abbrev;

      gtk_tree_model_get (model, iter,
                          LIGMA_INT_STORE_ABBREV, &abbrev,
                          -1);

      if (abbrev)
        {
          g_object_set (cell,
                        "text", abbrev,
                        NULL);

          g_free (abbrev);
        }
    }

  if (priv->sensitivity_func)
    {
      gint      value;
      gboolean  sensitive;

      gtk_tree_model_get (model, iter,
                          LIGMA_INT_STORE_VALUE, &value,
                          -1);

      sensitive = priv->sensitivity_func (value, priv->sensitivity_data);

      g_object_set (cell,
                    "sensitive", sensitive,
                    NULL);
    }
}
