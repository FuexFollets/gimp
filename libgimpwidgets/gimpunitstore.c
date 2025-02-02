/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaunitstore.c
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

#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "ligmawidgetstypes.h"

#include "ligmaunitstore.h"


/**
 * SECTION: ligmaunitstore
 * @title: LigmaUnitStore
 * @short_description: A model for units
 *
 * A model for #LigmaUnit views
 **/


enum
{
  PROP_0,
  PROP_NUM_VALUES,
  PROP_HAS_PIXELS,
  PROP_HAS_PERCENT,
  PROP_SHORT_FORMAT,
  PROP_LONG_FORMAT
};

struct _LigmaUnitStorePrivate
{
  gint      num_values;
  gboolean  has_pixels;
  gboolean  has_percent;

  gchar    *short_format;
  gchar    *long_format;

  gdouble  *values;
  gdouble  *resolutions;

  LigmaUnit  synced_unit;
};

#define GET_PRIVATE(obj) (((LigmaUnitStore *) (obj))->priv)


static void         ligma_unit_store_tree_model_init (GtkTreeModelIface *iface);

static void         ligma_unit_store_finalize        (GObject      *object);
static void         ligma_unit_store_set_property    (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void         ligma_unit_store_get_property    (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static GtkTreeModelFlags ligma_unit_store_get_flags  (GtkTreeModel *tree_model);
static gint         ligma_unit_store_get_n_columns   (GtkTreeModel *tree_model);
static GType        ligma_unit_store_get_column_type (GtkTreeModel *tree_model,
                                                     gint          index);
static gboolean     ligma_unit_store_get_iter        (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreePath  *path);
static GtkTreePath *ligma_unit_store_get_path        (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static void    ligma_unit_store_tree_model_get_value (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     gint          column,
                                                     GValue       *value);
static gboolean     ligma_unit_store_iter_next       (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gboolean     ligma_unit_store_iter_children   (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *parent);
static gboolean     ligma_unit_store_iter_has_child  (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gint         ligma_unit_store_iter_n_children (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter);
static gboolean     ligma_unit_store_iter_nth_child  (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *parent,
                                                     gint          n);
static gboolean     ligma_unit_store_iter_parent     (GtkTreeModel *tree_model,
                                                     GtkTreeIter  *iter,
                                                     GtkTreeIter  *child);


G_DEFINE_TYPE_WITH_CODE (LigmaUnitStore, ligma_unit_store, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (LigmaUnitStore)
                         G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
                                                ligma_unit_store_tree_model_init))

#define parent_class ligma_unit_store_parent_class


static GType column_types[LIGMA_UNIT_STORE_UNIT_COLUMNS] =
{
  G_TYPE_INVALID,
  G_TYPE_DOUBLE,
  G_TYPE_INT,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING,
  G_TYPE_STRING
};


static void
ligma_unit_store_class_init (LigmaUnitStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  column_types[LIGMA_UNIT_STORE_UNIT] = LIGMA_TYPE_UNIT;

  object_class->finalize     = ligma_unit_store_finalize;
  object_class->set_property = ligma_unit_store_set_property;
  object_class->get_property = ligma_unit_store_get_property;

  g_object_class_install_property (object_class, PROP_NUM_VALUES,
                                   g_param_spec_int ("num-values",
                                                     "Num Values",
                                                     "The number of values this store provides",
                                                     0, G_MAXINT, 0,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_HAS_PIXELS,
                                   g_param_spec_boolean ("has-pixels",
                                                         "Has Pixels",
                                                         "Whether the store has LIGMA_UNIT_PIXELS",
                                                         TRUE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_HAS_PERCENT,
                                   g_param_spec_boolean ("has-percent",
                                                         "Has Percent",
                                                         "Whether the store has LIGMA_UNIT_PERCENT",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_SHORT_FORMAT,
                                   g_param_spec_string ("short-format",
                                                        "Short Format",
                                                        "Format string for a short label",
                                                        "%a",
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_LONG_FORMAT,
                                   g_param_spec_string ("long-format",
                                                        "Long Format",
                                                        "Format string for a long label",
                                                        "%p",
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_unit_store_init (LigmaUnitStore *store)
{
  LigmaUnitStorePrivate *private;

  store->priv = ligma_unit_store_get_instance_private (store);

  private = store->priv;

  private->has_pixels   = TRUE;
  private->has_percent  = FALSE;
  private->short_format = g_strdup ("%a");
  private->long_format  = g_strdup ("%p");
  private->synced_unit  = ligma_unit_get_number_of_units () - 1;
}

static void
ligma_unit_store_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags       = ligma_unit_store_get_flags;
  iface->get_n_columns   = ligma_unit_store_get_n_columns;
  iface->get_column_type = ligma_unit_store_get_column_type;
  iface->get_iter        = ligma_unit_store_get_iter;
  iface->get_path        = ligma_unit_store_get_path;
  iface->get_value       = ligma_unit_store_tree_model_get_value;
  iface->iter_next       = ligma_unit_store_iter_next;
  iface->iter_children   = ligma_unit_store_iter_children;
  iface->iter_has_child  = ligma_unit_store_iter_has_child;
  iface->iter_n_children = ligma_unit_store_iter_n_children;
  iface->iter_nth_child  = ligma_unit_store_iter_nth_child;
  iface->iter_parent     = ligma_unit_store_iter_parent;
}

static void
ligma_unit_store_finalize (GObject *object)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (object);

  g_clear_pointer (&private->short_format, g_free);
  g_clear_pointer (&private->long_format,  g_free);

  g_clear_pointer (&private->values,      g_free);
  g_clear_pointer (&private->resolutions, g_free);
  private->num_values = 0;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_unit_store_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_NUM_VALUES:
      g_return_if_fail (private->num_values == 0);
      private->num_values = g_value_get_int (value);
      if (private->num_values)
        {
          private->values      = g_new0 (gdouble, private->num_values);
          private->resolutions = g_new0 (gdouble, private->num_values);
        }
      break;
    case PROP_HAS_PIXELS:
      ligma_unit_store_set_has_pixels (LIGMA_UNIT_STORE (object),
                                      g_value_get_boolean (value));
      break;
    case PROP_HAS_PERCENT:
      ligma_unit_store_set_has_percent (LIGMA_UNIT_STORE (object),
                                       g_value_get_boolean (value));
      break;
    case PROP_SHORT_FORMAT:
      g_free (private->short_format);
      private->short_format = g_value_dup_string (value);
      if (! private->short_format)
        private->short_format = g_strdup ("%a");
      break;
    case PROP_LONG_FORMAT:
      g_free (private->long_format);
      private->long_format = g_value_dup_string (value);
      if (! private->long_format)
        private->long_format = g_strdup ("%a");
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_unit_store_get_property (GObject      *object,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_NUM_VALUES:
      g_value_set_int (value, private->num_values);
      break;
    case PROP_HAS_PIXELS:
      g_value_set_boolean (value, private->has_pixels);
      break;
    case PROP_HAS_PERCENT:
      g_value_set_boolean (value, private->has_percent);
      break;
    case PROP_SHORT_FORMAT:
      g_value_set_string (value, private->short_format);
      break;
    case PROP_LONG_FORMAT:
      g_value_set_string (value, private->long_format);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GtkTreeModelFlags
ligma_unit_store_get_flags (GtkTreeModel *tree_model)
{
  return GTK_TREE_MODEL_ITERS_PERSIST | GTK_TREE_MODEL_LIST_ONLY;
}

static gint
ligma_unit_store_get_n_columns (GtkTreeModel *tree_model)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (tree_model);

  return LIGMA_UNIT_STORE_UNIT_COLUMNS + private->num_values;
}

static GType
ligma_unit_store_get_column_type (GtkTreeModel *tree_model,
                                 gint          index)
{
  g_return_val_if_fail (index >= 0, G_TYPE_INVALID);

  if (index < LIGMA_UNIT_STORE_UNIT_COLUMNS)
    return column_types[index];

  return G_TYPE_DOUBLE;
}

static gboolean
ligma_unit_store_get_iter (GtkTreeModel *tree_model,
                          GtkTreeIter  *iter,
                          GtkTreePath  *path)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (tree_model);
  gint                  index;
  LigmaUnit              unit;

  g_return_val_if_fail (gtk_tree_path_get_depth (path) > 0, FALSE);

  index = gtk_tree_path_get_indices (path)[0];

  unit = index;

  if (! private->has_pixels)
    unit++;

  if (private->has_percent)
    {
      unit--;

      if (private->has_pixels)
        {
          if (index == 0)
            unit = LIGMA_UNIT_PIXEL;
          else if (index == 1)
            unit = LIGMA_UNIT_PERCENT;
        }
      else
        {
          if (index == 0)
            unit = LIGMA_UNIT_PERCENT;
        }
    }

  if ((unit >= 0 && unit < ligma_unit_get_number_of_units ()) ||
      ((unit == LIGMA_UNIT_PERCENT && private->has_percent)))
    {
      iter->user_data = GINT_TO_POINTER (unit);
      return TRUE;
    }

  return FALSE;
}

static GtkTreePath *
ligma_unit_store_get_path (GtkTreeModel *tree_model,
                          GtkTreeIter  *iter)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (tree_model);
  GtkTreePath          *path    = gtk_tree_path_new ();
  LigmaUnit              unit    = GPOINTER_TO_INT (iter->user_data);
  gint                  index;

  index = unit;

  if (! private->has_pixels)
    index--;

  if (private->has_percent)
    {
      index++;

      if (private->has_pixels)
        {
          if (unit == LIGMA_UNIT_PIXEL)
            index = 0;
          else if (unit == LIGMA_UNIT_PERCENT)
            index = 1;
        }
      else
        {
          if (unit == LIGMA_UNIT_PERCENT)
            index = 0;
        }
    }

  gtk_tree_path_append_index (path, index);

  return path;
}

static void
ligma_unit_store_tree_model_get_value (GtkTreeModel *tree_model,
                                      GtkTreeIter  *iter,
                                      gint          column,
                                      GValue       *value)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (tree_model);
  LigmaUnit              unit;

  g_return_if_fail (column >= 0 &&
                    column < LIGMA_UNIT_STORE_UNIT_COLUMNS + private->num_values);

  g_value_init (value,
                column < LIGMA_UNIT_STORE_UNIT_COLUMNS ?
                column_types[column] :
                G_TYPE_DOUBLE);

  unit = GPOINTER_TO_INT (iter->user_data);

  if ((unit >= 0 && unit < ligma_unit_get_number_of_units ()) ||
      ((unit == LIGMA_UNIT_PERCENT && private->has_percent)))
    {
      switch (column)
        {
        case LIGMA_UNIT_STORE_UNIT:
          g_value_set_int (value, unit);
          break;
        case LIGMA_UNIT_STORE_UNIT_FACTOR:
          g_value_set_double (value, ligma_unit_get_factor (unit));
          break;
        case LIGMA_UNIT_STORE_UNIT_DIGITS:
          g_value_set_int (value, ligma_unit_get_digits (unit));
          break;
        case LIGMA_UNIT_STORE_UNIT_IDENTIFIER:
          g_value_set_static_string (value, ligma_unit_get_identifier (unit));
          break;
        case LIGMA_UNIT_STORE_UNIT_SYMBOL:
          g_value_set_static_string (value, ligma_unit_get_symbol (unit));
          break;
        case LIGMA_UNIT_STORE_UNIT_ABBREVIATION:
          g_value_set_static_string (value, ligma_unit_get_abbreviation (unit));
          break;
        case LIGMA_UNIT_STORE_UNIT_SINGULAR:
          g_value_set_static_string (value, ligma_unit_get_singular (unit));
          break;
        case LIGMA_UNIT_STORE_UNIT_PLURAL:
          g_value_set_static_string (value, ligma_unit_get_plural (unit));
          break;
        case LIGMA_UNIT_STORE_UNIT_SHORT_FORMAT:
          g_value_take_string (value,
                               ligma_unit_format_string (private->short_format,
                                                        unit));
          break;
        case LIGMA_UNIT_STORE_UNIT_LONG_FORMAT:
          g_value_take_string (value,
                               ligma_unit_format_string (private->long_format,
                                                        unit));
          break;

        default:
          column -= LIGMA_UNIT_STORE_UNIT_COLUMNS;
          if (unit == LIGMA_UNIT_PIXEL)
            {
              g_value_set_double (value, private->values[column]);
            }
          else if (private->resolutions[column])
            {
              g_value_set_double (value,
                                  private->values[column] *
                                  ligma_unit_get_factor (unit) /
                                  private->resolutions[column]);
            }
          break;
        }
    }
}

static gboolean
ligma_unit_store_iter_next (GtkTreeModel *tree_model,
                           GtkTreeIter  *iter)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (tree_model);
  LigmaUnit              unit    = GPOINTER_TO_INT (iter->user_data);

  if (unit == LIGMA_UNIT_PIXEL && private->has_percent)
    {
      unit = LIGMA_UNIT_PERCENT;
    }
  else if (unit == LIGMA_UNIT_PERCENT)
    {
      unit = LIGMA_UNIT_INCH;
    }
  else if (unit >= 0 && unit < ligma_unit_get_number_of_units () - 1)
    {
      unit++;
    }
  else
    {
      return FALSE;
    }

  iter->user_data = GINT_TO_POINTER (unit);

  return TRUE;
}

static gboolean
ligma_unit_store_iter_children (GtkTreeModel *tree_model,
                               GtkTreeIter  *iter,
                               GtkTreeIter  *parent)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (tree_model);
  LigmaUnit              unit;

  /* this is a list, nodes have no children */
  if (parent)
    return FALSE;

  if (private->has_pixels)
    {
      unit = LIGMA_UNIT_PIXEL;
    }
  else if (private->has_percent)
    {
      unit = LIGMA_UNIT_PERCENT;
    }
  else
    {
      unit = LIGMA_UNIT_INCH;
    }

  iter->user_data = GINT_TO_POINTER (unit);

  return TRUE;
}

static gboolean
ligma_unit_store_iter_has_child (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter)
{
  return FALSE;
}

static gint
ligma_unit_store_iter_n_children (GtkTreeModel *tree_model,
                                 GtkTreeIter  *iter)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (tree_model);
  gint                  n_children;

  if (iter)
    return 0;

  n_children = ligma_unit_get_number_of_units ();

  if (! private->has_pixels)
    n_children--;

  if (private->has_percent)
    n_children++;

  return n_children;
}

static gboolean
ligma_unit_store_iter_nth_child (GtkTreeModel *tree_model,
                                GtkTreeIter  *iter,
                                GtkTreeIter  *parent,
                                gint          n)
{
  LigmaUnitStorePrivate *private = GET_PRIVATE (tree_model);
  gint                  n_children;

  if (parent)
    return FALSE;

  n_children = ligma_unit_store_iter_n_children (tree_model, NULL);

  if (n >= 0 && n < n_children)
    {
      LigmaUnit unit = n;

      if (! private->has_pixels)
        unit++;

      if (private->has_percent)
        {
          unit--;

          if (private->has_pixels)
            {
              if (n == 0)
                unit = LIGMA_UNIT_PIXEL;
              else if (n == 1)
                unit = LIGMA_UNIT_PERCENT;
            }
          else
            {
              if (n == 0)
                unit = LIGMA_UNIT_PERCENT;
            }
        }

      iter->user_data = GINT_TO_POINTER (unit);

      return TRUE;
    }

  return FALSE;
}

static gboolean
ligma_unit_store_iter_parent (GtkTreeModel *tree_model,
                             GtkTreeIter  *iter,
                             GtkTreeIter  *child)
{
  return FALSE;
}


LigmaUnitStore *
ligma_unit_store_new (gint  num_values)
{
  return g_object_new (LIGMA_TYPE_UNIT_STORE,
                       "num-values", num_values,
                       NULL);
}

void
ligma_unit_store_set_has_pixels (LigmaUnitStore *store,
                                gboolean       has_pixels)
{
  LigmaUnitStorePrivate *private;

  g_return_if_fail (LIGMA_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  has_pixels = has_pixels ? TRUE : FALSE;

  if (has_pixels != private->has_pixels)
    {
      GtkTreeModel *model        = GTK_TREE_MODEL (store);
      GtkTreePath  *deleted_path = NULL;

      if (! has_pixels)
        {
          GtkTreeIter iter;

          gtk_tree_model_get_iter_first (model, &iter);
          deleted_path = gtk_tree_model_get_path (model, &iter);
        }

      private->has_pixels = has_pixels;

      if (has_pixels)
        {
          GtkTreePath *path;
          GtkTreeIter  iter;

          gtk_tree_model_get_iter_first (model, &iter);
          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_model_row_inserted (model, path, &iter);
          gtk_tree_path_free (path);
        }
      else if (deleted_path)
        {
          gtk_tree_model_row_deleted (model, deleted_path);
          gtk_tree_path_free (deleted_path);
        }

      g_object_notify (G_OBJECT (store), "has-pixels");
    }
}

gboolean
ligma_unit_store_get_has_pixels (LigmaUnitStore *store)
{
  LigmaUnitStorePrivate *private;

  g_return_val_if_fail (LIGMA_IS_UNIT_STORE (store), FALSE);

  private = GET_PRIVATE (store);

  return private->has_pixels;
}

void
ligma_unit_store_set_has_percent (LigmaUnitStore *store,
                                 gboolean       has_percent)
{
  LigmaUnitStorePrivate *private;

  g_return_if_fail (LIGMA_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  has_percent = has_percent ? TRUE : FALSE;

  if (has_percent != private->has_percent)
    {
      GtkTreeModel *model        = GTK_TREE_MODEL (store);
      GtkTreePath  *deleted_path = NULL;

      if (! has_percent)
        {
          GtkTreeIter iter;

          gtk_tree_model_get_iter_first (model, &iter);
          if (private->has_pixels)
            gtk_tree_model_iter_next (model, &iter);
          deleted_path = gtk_tree_model_get_path (model, &iter);
        }

      private->has_percent = has_percent;

      if (has_percent)
        {
          GtkTreePath *path;
          GtkTreeIter  iter;

          gtk_tree_model_get_iter_first (model, &iter);
          if (private->has_pixels)
            gtk_tree_model_iter_next (model, &iter);
          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_model_row_inserted (model, path, &iter);
          gtk_tree_path_free (path);
        }
      else if (deleted_path)
        {
          gtk_tree_model_row_deleted (model, deleted_path);
          gtk_tree_path_free (deleted_path);
        }

      g_object_notify (G_OBJECT (store), "has-percent");
    }
}

gboolean
ligma_unit_store_get_has_percent (LigmaUnitStore *store)
{
  LigmaUnitStorePrivate *private;

  g_return_val_if_fail (LIGMA_IS_UNIT_STORE (store), FALSE);

  private = GET_PRIVATE (store);

  return private->has_percent;
}

void
ligma_unit_store_set_pixel_value (LigmaUnitStore *store,
                                 gint           index,
                                 gdouble        value)
{
  LigmaUnitStorePrivate *private;

  g_return_if_fail (LIGMA_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  g_return_if_fail (index > 0 && index < private->num_values);

  private->values[index] = value;
}

void
ligma_unit_store_set_pixel_values (LigmaUnitStore *store,
                                  gdouble        first_value,
                                  ...)
{
  LigmaUnitStorePrivate *private;
  va_list               args;
  gint                  i;

  g_return_if_fail (LIGMA_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  va_start (args, first_value);

  for (i = 0; i < private->num_values; )
    {
      private->values[i] = first_value;

      if (++i < private->num_values)
        first_value = va_arg (args, gdouble);
    }

  va_end (args);
}

void
ligma_unit_store_set_resolution (LigmaUnitStore *store,
                                gint           index,
                                gdouble        resolution)
{
  LigmaUnitStorePrivate *private;

  g_return_if_fail (LIGMA_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  g_return_if_fail (index > 0 && index < private->num_values);

  private->resolutions[index] = resolution;
}

void
ligma_unit_store_set_resolutions  (LigmaUnitStore *store,
                                  gdouble        first_resolution,
                                  ...)
{
  LigmaUnitStorePrivate *private;
  va_list               args;
  gint                  i;

  g_return_if_fail (LIGMA_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  va_start (args, first_resolution);

  for (i = 0; i < private->num_values; )
    {
      private->resolutions[i] = first_resolution;

      if (++i < private->num_values)
        first_resolution = va_arg (args, gdouble);
    }

  va_end (args);
}

gdouble
ligma_unit_store_get_nth_value (LigmaUnitStore *store,
                               LigmaUnit       unit,
                               gint           index)
{
  LigmaUnitStorePrivate *private;
  GtkTreeIter          iter;
  GValue               value = G_VALUE_INIT;

  g_return_val_if_fail (LIGMA_IS_UNIT_STORE (store), 0.0);

  private = GET_PRIVATE (store);

  g_return_val_if_fail (index >= 0 && index < private->num_values, 0.0);

  iter.user_data = GINT_TO_POINTER (unit);

  ligma_unit_store_tree_model_get_value (GTK_TREE_MODEL (store),
                                        &iter,
                                        LIGMA_UNIT_STORE_FIRST_VALUE + index,
                                        &value);

  return g_value_get_double (&value);
}

void
ligma_unit_store_get_values (LigmaUnitStore *store,
                            LigmaUnit       unit,
                            gdouble       *first_value,
                            ...)
{
  LigmaUnitStorePrivate *private;
  va_list               args;
  gint                  i;

  g_return_if_fail (LIGMA_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);

  va_start (args, first_value);

  for (i = 0; i < private->num_values; )
    {
      if (first_value)
        *first_value = ligma_unit_store_get_nth_value (store, unit, i);

      if (++i < private->num_values)
        first_value = va_arg (args, gdouble *);
    }

  va_end (args);
}

void
_ligma_unit_store_sync_units (LigmaUnitStore *store)
{
  LigmaUnitStorePrivate *private;
  GtkTreeModel         *model;
  GtkTreeIter           iter;
  gboolean              iter_valid;

  g_return_if_fail (LIGMA_IS_UNIT_STORE (store));

  private = GET_PRIVATE (store);
  model   = GTK_TREE_MODEL (store);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gint unit;

      gtk_tree_model_get (model, &iter,
                          LIGMA_UNIT_STORE_UNIT, &unit,
                          -1);

      if (unit != LIGMA_UNIT_PERCENT &&
          unit > private->synced_unit)
        {
          GtkTreePath *path;

          path = gtk_tree_model_get_path (model, &iter);
          gtk_tree_model_row_inserted (model, path, &iter);
          gtk_tree_path_free (path);
        }
    }

  private->synced_unit = ligma_unit_get_number_of_units () - 1;
}
