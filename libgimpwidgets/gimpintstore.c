/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaintstore.c
 * Copyright (C) 2004-2007  Sven Neumann <sven@ligma.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "ligmawidgetstypes.h"

#include "ligmaintstore.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmaintstore
 * @title: LigmaIntStore
 * @short_description: A model for integer based name-value pairs
 *                     (e.g. enums)
 *
 * A model for integer based name-value pairs (e.g. enums)
 **/


enum
{
  PROP_0,
  PROP_USER_DATA_TYPE
};


struct _LigmaIntStorePrivate
{
  GtkTreeIter *empty_iter;
  GType        user_data_type;
};

#define GET_PRIVATE(obj) (((LigmaIntStore *) (obj))->priv)


static void  ligma_int_store_tree_model_init (GtkTreeModelIface *iface);

static void  ligma_int_store_constructed     (GObject           *object);
static void  ligma_int_store_finalize        (GObject           *object);
static void  ligma_int_store_set_property    (GObject           *object,
                                             guint              property_id,
                                             const GValue      *value,
                                             GParamSpec        *pspec);
static void  ligma_int_store_get_property    (GObject           *object,
                                             guint              property_id,
                                             GValue            *value,
                                             GParamSpec        *pspec);

static void  ligma_int_store_row_inserted    (GtkTreeModel      *model,
                                             GtkTreePath       *path,
                                             GtkTreeIter       *iter);
static void  ligma_int_store_row_deleted     (GtkTreeModel      *model,
                                             GtkTreePath       *path);
static void  ligma_int_store_add_empty       (LigmaIntStore      *store);


G_DEFINE_TYPE_WITH_CODE (LigmaIntStore, ligma_int_store, GTK_TYPE_LIST_STORE,
                         G_ADD_PRIVATE (LigmaIntStore)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                ligma_int_store_tree_model_init))

#define parent_class ligma_int_store_parent_class

static GtkTreeModelIface *parent_iface = NULL;


static void
ligma_int_store_class_init (LigmaIntStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_int_store_constructed;
  object_class->finalize     = ligma_int_store_finalize;
  object_class->set_property = ligma_int_store_set_property;
  object_class->get_property = ligma_int_store_get_property;

  /**
   * LigmaIntStore:user-data-type:
   *
   * Sets the #GType for the LIGMA_INT_STORE_USER_DATA column.
   *
   * You need to set this property when constructing the store if you want
   * to use the LIGMA_INT_STORE_USER_DATA column and want to have the store
   * handle ref-counting of your user data.
   *
   * Since: 2.4
   */
  g_object_class_install_property (object_class,
                                   PROP_USER_DATA_TYPE,
                                   g_param_spec_gtype ("user-data-type",
                                                       "User Data Type",
                                                       "The GType of the user_data column",
                                                       G_TYPE_NONE,
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       LIGMA_PARAM_READWRITE));
}

static void
ligma_int_store_tree_model_init (GtkTreeModelIface *iface)
{
  parent_iface = g_type_interface_peek_parent (iface);

  iface->row_inserted = ligma_int_store_row_inserted;
  iface->row_deleted  = ligma_int_store_row_deleted;
}

static void
ligma_int_store_init (LigmaIntStore *store)
{
  store->priv = ligma_int_store_get_instance_private (store);
}

static void
ligma_int_store_constructed (GObject *object)
{
  LigmaIntStore        *store = LIGMA_INT_STORE (object);
  LigmaIntStorePrivate *priv  = GET_PRIVATE (store);
  GType                types[LIGMA_INT_STORE_NUM_COLUMNS];

  G_OBJECT_CLASS (parent_class)->constructed (object);

  types[LIGMA_INT_STORE_VALUE]     = G_TYPE_INT;
  types[LIGMA_INT_STORE_LABEL]     = G_TYPE_STRING;
  types[LIGMA_INT_STORE_ABBREV]    = G_TYPE_STRING;
  types[LIGMA_INT_STORE_ICON_NAME] = G_TYPE_STRING;
  types[LIGMA_INT_STORE_PIXBUF]    = GDK_TYPE_PIXBUF;
  types[LIGMA_INT_STORE_USER_DATA] = (priv->user_data_type != G_TYPE_NONE ?
                                     priv->user_data_type : G_TYPE_POINTER);

  gtk_list_store_set_column_types (GTK_LIST_STORE (store),
                                   LIGMA_INT_STORE_NUM_COLUMNS, types);

  ligma_int_store_add_empty (store);
}

static void
ligma_int_store_finalize (GObject *object)
{
  LigmaIntStorePrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->empty_iter, gtk_tree_iter_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_int_store_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaIntStorePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_USER_DATA_TYPE:
      priv->user_data_type = g_value_get_gtype (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_int_store_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaIntStorePrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_USER_DATA_TYPE:
      g_value_set_gtype (value, priv->user_data_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_int_store_row_inserted (GtkTreeModel *model,
                             GtkTreePath  *path,
                             GtkTreeIter  *iter)
{
  LigmaIntStore        *store = LIGMA_INT_STORE (model);
  LigmaIntStorePrivate *priv  = GET_PRIVATE (store);

  if (parent_iface->row_inserted)
    parent_iface->row_inserted (model, path, iter);

  if (priv->empty_iter &&
      memcmp (iter, priv->empty_iter, sizeof (GtkTreeIter)))
    {
      gtk_list_store_remove (GTK_LIST_STORE (store), priv->empty_iter);
      gtk_tree_iter_free (priv->empty_iter);
      priv->empty_iter = NULL;
    }
}

static void
ligma_int_store_row_deleted (GtkTreeModel *model,
                            GtkTreePath  *path)
{
  if (parent_iface->row_deleted)
    parent_iface->row_deleted (model, path);
}

static void
ligma_int_store_add_empty (LigmaIntStore *store)
{
  LigmaIntStorePrivate *priv = GET_PRIVATE (store);
  GtkTreeIter          iter = { 0, };

  g_return_if_fail (priv->empty_iter == NULL);

  gtk_list_store_prepend (GTK_LIST_STORE (store), &iter);
  gtk_list_store_set (GTK_LIST_STORE (store), &iter,
                      LIGMA_INT_STORE_VALUE, -1,
                      /* This string appears in an empty menu as in
                       * "nothing selected and nothing to select"
                       */
                      LIGMA_INT_STORE_LABEL, (_("(Empty)")),
                      -1);

  priv->empty_iter = gtk_tree_iter_copy (&iter);
}

/**
 * ligma_int_store_new: (skip)
 * @first_label: the label of the first item
 * @first_value: the value of the first item
 * @...:         a %NULL terminated list of more label, value pairs
 *
 * Creates a #GtkListStore with a number of useful columns.
 * #LigmaIntStore is especially useful if the items you want to store
 * are identified using an integer value.
 *
 * If you need to construct an empty #LigmaIntStore, it's best to use
 * g_object_new (LIGMA_TYPE_INT_STORE, NULL).
 *
 * Returns: a new #LigmaIntStore.
 *
 * Since: 2.2
 **/
GtkListStore *
ligma_int_store_new (const gchar *first_label,
                    gint         first_value,
                    ...)
{
  GtkListStore *store;
  va_list       args;

  va_start (args, first_value);

  store = ligma_int_store_new_valist (first_label, first_value, args);

  va_end (args);

  return store;
}

/**
 * ligma_int_store_new_valist:
 * @first_label: the label of the first item
 * @first_value: the value of the first item
 * @values:      a va_list with more values
 *
 * A variant of ligma_int_store_new() that takes a va_list of
 * label/value pairs.
 *
 * Returns: a new #LigmaIntStore.
 *
 * Since: 3.0
 **/
GtkListStore *
ligma_int_store_new_valist (const gchar *first_label,
                           gint         first_value,
                           va_list      values)
{
  GtkListStore *store;
  const gchar  *label;
  gint          value;

  store = g_object_new (LIGMA_TYPE_INT_STORE, NULL);

  for (label = first_label, value = first_value;
       label;
       label = va_arg (values, const gchar *), value = va_arg (values, gint))
    {
      GtkTreeIter iter = { 0, };

      gtk_list_store_append (store, &iter);
      gtk_list_store_set (store, &iter,
                          LIGMA_INT_STORE_VALUE, value,
                          LIGMA_INT_STORE_LABEL, label,
                          -1);
    }

  return store;
}

/**
 * ligma_int_store_lookup_by_value:
 * @model: a #LigmaIntStore
 * @value: an integer value to lookup in the @model
 * @iter: (out):  return location for the iter of the given @value
 *
 * Iterate over the @model looking for @value.
 *
 * Returns: %TRUE if the value has been located and @iter is
 *               valid, %FALSE otherwise.
 *
 * Since: 2.2
 **/
gboolean
ligma_int_store_lookup_by_value (GtkTreeModel *model,
                                gint          value,
                                GtkTreeIter  *iter)
{
  gboolean  iter_valid;

  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gint  this;

      gtk_tree_model_get (model, iter,
                          LIGMA_INT_STORE_VALUE, &this,
                          -1);
      if (this == value)
        break;
    }

  return iter_valid;
}

/**
 * ligma_int_store_lookup_by_user_data:
 * @model: a #LigmaIntStore
 * @user_data: a gpointer "user-data" to lookup in the @model
 * @iter: (out): return location for the iter of the given @user_data
 *
 * Iterate over the @model looking for @user_data.
 *
 * Returns: %TRUE if the user-data has been located and @iter is
 *               valid, %FALSE otherwise.
 *
 * Since: 2.10
 **/
gboolean
ligma_int_store_lookup_by_user_data (GtkTreeModel *model,
                                    gpointer      user_data,
                                    GtkTreeIter  *iter)
{
  gboolean iter_valid = FALSE;

  g_return_val_if_fail (GTK_IS_TREE_MODEL (model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  for (iter_valid = gtk_tree_model_get_iter_first (model, iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, iter))
    {
      gpointer this;

      gtk_tree_model_get (model, iter,
                          LIGMA_INT_STORE_USER_DATA, &this,
                          -1);
      if (this == user_data)
        break;
    }

  return (gboolean) iter_valid;
}
