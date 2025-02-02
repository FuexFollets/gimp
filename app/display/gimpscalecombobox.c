/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmascalecombobox.c
 * Copyright (C) 2004, 2008  Sven Neumann <sven@ligma.org>
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

#include "stdlib.h"

#include <gegl.h>
#include <gtk/gtk.h>
#include "gdk/gdkkeysyms.h"

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "display-types.h"

#include "ligmascalecombobox.h"


#define MAX_ITEMS  10

enum
{
  COLUMN_SCALE,
  COLUMN_LABEL,
  COLUMN_PERSISTENT,
  N_COLUMNS
};

enum
{
  ENTRY_ACTIVATED,
  LAST_SIGNAL
};


static void      ligma_scale_combo_box_constructed     (GObject           *object);
static void      ligma_scale_combo_box_finalize        (GObject           *object);

static void      ligma_scale_combo_box_changed         (LigmaScaleComboBox *combo_box);
static void      ligma_scale_combo_box_entry_activate  (GtkWidget         *entry,
                                                       LigmaScaleComboBox *combo_box);
static gboolean  ligma_scale_combo_box_entry_key_press (GtkWidget         *entry,
                                                       GdkEventKey       *event,
                                                       LigmaScaleComboBox *combo_box);

static void      ligma_scale_combo_box_scale_iter_set  (GtkListStore      *store,
                                                       GtkTreeIter       *iter,
                                                       gdouble            scale,
                                                       gboolean           persistent);


G_DEFINE_TYPE (LigmaScaleComboBox, ligma_scale_combo_box,
               GTK_TYPE_COMBO_BOX)

#define parent_class ligma_scale_combo_box_parent_class

static guint scale_combo_box_signals[LAST_SIGNAL] = { 0 };


static void
ligma_scale_combo_box_class_init (LigmaScaleComboBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  scale_combo_box_signals[ENTRY_ACTIVATED] =
    g_signal_new ("entry-activated",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaScaleComboBoxClass, entry_activated),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed = ligma_scale_combo_box_constructed;
  object_class->finalize    = ligma_scale_combo_box_finalize;
}

static void
ligma_scale_combo_box_init (LigmaScaleComboBox *combo_box)
{
  combo_box->scale     = 1.0;
  combo_box->last_path = NULL;
}

static void
ligma_scale_combo_box_constructed (GObject *object)
{
  LigmaScaleComboBox *combo_box = LIGMA_SCALE_COMBO_BOX (object);
  GtkWidget         *entry;
  GtkListStore      *store;
  GtkCellLayout     *layout;
  GtkCellRenderer   *cell;
  GtkTreeIter        iter;
  GtkBorder          border = { 0, 0, 0, 0 };
  gint               i;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  store = gtk_list_store_new (N_COLUMNS,
                              G_TYPE_DOUBLE,    /* SCALE       */
                              G_TYPE_STRING,    /* LABEL       */
                              G_TYPE_BOOLEAN);  /* PERSISTENT  */

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_box), GTK_TREE_MODEL (store));
  g_object_unref (store);

  gtk_combo_box_set_entry_text_column (GTK_COMBO_BOX (combo_box),
                                       COLUMN_LABEL);

  entry = gtk_bin_get_child (GTK_BIN (combo_box));

  gtk_widget_add_events (GTK_WIDGET (entry), GDK_SCROLL_MASK);

  g_object_set (entry,
                "xalign",             1.0,
                "width-chars",        5,
                "truncate-multiline", TRUE,
                "inner-border",       &border,
                NULL);

  layout = GTK_CELL_LAYOUT (combo_box);

  cell = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                       "xalign", 1.0,
                       NULL);

  gtk_cell_layout_clear (layout);
  gtk_cell_layout_pack_start (layout, cell, TRUE);
  gtk_cell_layout_set_attributes (layout, cell,
                                  "text", COLUMN_LABEL,
                                  NULL);

  for (i = 8; i > 0; i /= 2)
    {
      gtk_list_store_append (store, &iter);
      ligma_scale_combo_box_scale_iter_set (store, &iter, i, TRUE);
    }

  for (i = 2; i <= 8; i *= 2)
    {
      gtk_list_store_append (store, &iter);
      ligma_scale_combo_box_scale_iter_set (store, &iter, 1.0 / i, TRUE);
    }

  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (ligma_scale_combo_box_changed),
                    NULL);

  g_signal_connect (entry, "activate",
                    G_CALLBACK (ligma_scale_combo_box_entry_activate),
                    combo_box);
  g_signal_connect (entry, "key-press-event",
                    G_CALLBACK (ligma_scale_combo_box_entry_key_press),
                    combo_box);
}

static void
ligma_scale_combo_box_finalize (GObject *object)
{
  LigmaScaleComboBox *combo_box = LIGMA_SCALE_COMBO_BOX (object);

  if (combo_box->last_path)
    {
      gtk_tree_path_free (combo_box->last_path);
      combo_box->last_path = NULL;
    }

  if (combo_box->mru)
    {
      g_list_free_full (combo_box->mru,
                        (GDestroyNotify) gtk_tree_row_reference_free);
      combo_box->mru = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_scale_combo_box_changed (LigmaScaleComboBox *combo_box)
{
  GtkTreeIter iter;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (combo_box), &iter))
    {
      GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
      gdouble       scale;

      gtk_tree_model_get (model, &iter,
                          COLUMN_SCALE, &scale,
                          -1);
      if (scale > 0.0)
        {
          combo_box->scale = scale;

          if (combo_box->last_path)
            gtk_tree_path_free (combo_box->last_path);

          combo_box->last_path = gtk_tree_model_get_path (model, &iter);
        }
    }
}

static gboolean
ligma_scale_combo_box_parse_text (const gchar *text,
                                 gdouble     *scale)
{
  gchar   *end;
  gdouble  left_number;
  gdouble  right_number;

  /* try to parse a number */
  left_number = strtod (text, &end);

  if (end == text)
    return FALSE;
  else
    text = end;

  /* skip over whitespace */
  while (g_unichar_isspace (g_utf8_get_char (text)))
    text = g_utf8_next_char (text);

  if (*text == '\0' || *text == '%')
    {
      *scale = left_number / 100.0;
      return TRUE;
    }

  /* check for a valid separator */
  if (*text != '/' && *text != ':')
    {
      *scale = left_number;
      return TRUE;
    }

  text = g_utf8_next_char (text);

  /* skip over whitespace */
  while (g_unichar_isspace (g_utf8_get_char (text)))
    text = g_utf8_next_char (text);

  /* try to parse another number */
  right_number = strtod (text, &end);

  if (end == text)
    return FALSE;

  if (right_number == 0.0)
    return FALSE;

  *scale = left_number / right_number;
  return TRUE;
}

static void
ligma_scale_combo_box_entry_activate (GtkWidget         *entry,
                                     LigmaScaleComboBox *combo_box)
{
  const gchar *text = gtk_entry_get_text (GTK_ENTRY (entry));
  gdouble      scale;

  if (ligma_scale_combo_box_parse_text (text, &scale) &&
      scale >= 1.0 / 256.0                           &&
      scale <= 256.0)
    {
      ligma_scale_combo_box_set_scale (combo_box, scale);
    }
  else
    {
      gtk_widget_error_bell (entry);

      ligma_scale_combo_box_set_scale (combo_box, combo_box->scale);
    }

  g_signal_emit (combo_box, scale_combo_box_signals[ENTRY_ACTIVATED], 0);
}

static gboolean
ligma_scale_combo_box_entry_key_press (GtkWidget         *entry,
                                      GdkEventKey       *event,
                                      LigmaScaleComboBox *combo_box)
{
  if (event->keyval == GDK_KEY_Escape)
    {
      ligma_scale_combo_box_set_scale (combo_box, combo_box->scale);

      g_signal_emit (combo_box, scale_combo_box_signals[ENTRY_ACTIVATED], 0);

      return TRUE;
    }

  if (event->keyval == GDK_KEY_Tab    ||
      event->keyval == GDK_KEY_KP_Tab ||
      event->keyval == GDK_KEY_ISO_Left_Tab)
    {
      ligma_scale_combo_box_entry_activate (entry, combo_box);

      return TRUE;
    }

  return FALSE;
}

static void
ligma_scale_combo_box_scale_iter_set (GtkListStore *store,
                                     GtkTreeIter  *iter,
                                     gdouble       scale,
                                     gboolean      persistent)
{
  gchar label[32];

#ifdef G_OS_WIN32

  /*  use a normal space until pango's windows backend uses harfbuzz,
   *  see bug #735505
   */
#define PERCENT_SPACE " "

#else

  /*  use U+2009 THIN SPACE to separate the percent sign from the number */
#define PERCENT_SPACE "\342\200\211"

#endif

  if (scale > 1.0)
    g_snprintf (label, sizeof (label),
                "%d" PERCENT_SPACE "%%", (gint) ROUND (100.0 * scale));
  else
    g_snprintf (label, sizeof (label),
                "%.3g" PERCENT_SPACE "%%", 100.0 * scale);

  gtk_list_store_set (store, iter,
                      COLUMN_SCALE,      scale,
                      COLUMN_LABEL,      label,
                      COLUMN_PERSISTENT, persistent,
                      -1);
}

static void
ligma_scale_combo_box_mru_add (LigmaScaleComboBox *combo_box,
                              GtkTreeIter       *iter)
{
  GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
  GtkTreePath  *path  = gtk_tree_model_get_path (model, iter);
  GList        *list;
  gboolean      found;

  for (list = combo_box->mru, found = FALSE; list && !found; list = list->next)
    {
      GtkTreePath *this = gtk_tree_row_reference_get_path (list->data);

      if (gtk_tree_path_compare (this, path) == 0)
        {
          if (list->prev)
            {
              combo_box->mru = g_list_remove_link (combo_box->mru, list);
              combo_box->mru = g_list_concat (list, combo_box->mru);
            }

          found = TRUE;
        }

      gtk_tree_path_free (this);
    }

  if (! found)
    combo_box->mru = g_list_prepend (combo_box->mru,
                                     gtk_tree_row_reference_new (model, path));

  gtk_tree_path_free (path);
}

static void
ligma_scale_combo_box_mru_remove_last (LigmaScaleComboBox *combo_box)
{
  GtkTreeModel *model;
  GtkTreePath  *path;
  GList        *last;
  GtkTreeIter   iter;

  if (! combo_box->mru)
    return;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));

  last = g_list_last (combo_box->mru);
  path = gtk_tree_row_reference_get_path (last->data);

  if (gtk_tree_model_get_iter (model, &iter, path))
    {
      gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
      gtk_tree_row_reference_free (last->data);
      combo_box->mru = g_list_delete_link (combo_box->mru, last);
    }

  gtk_tree_path_free (path);
}


/**
 * ligma_scale_combo_box_new:
 *
 * Returns: a new #LigmaScaleComboBox.
 **/
GtkWidget *
ligma_scale_combo_box_new (void)
{
  return g_object_new (LIGMA_TYPE_SCALE_COMBO_BOX,
                       "has-entry", TRUE,
                       NULL);
}

void
ligma_scale_combo_box_set_scale (LigmaScaleComboBox *combo_box,
                                gdouble            scale)
{
  GtkTreeModel *model;
  GtkListStore *store;
  GtkWidget    *entry;
  GtkTreeIter   iter;
  gboolean      iter_valid;
  gboolean      persistent;
  gint          n_digits;

  g_return_if_fail (LIGMA_IS_SCALE_COMBO_BOX (combo_box));
  g_return_if_fail (scale > 0.0);

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo_box));
  store = GTK_LIST_STORE (model);

  for (iter_valid = gtk_tree_model_get_iter_first (model, &iter);
       iter_valid;
       iter_valid = gtk_tree_model_iter_next (model, &iter))
    {
      gdouble  this;

      gtk_tree_model_get (model, &iter,
                          COLUMN_SCALE, &this,
                          -1);

      if (fabs (this - scale) < 0.0001)
        break;
    }

  if (! iter_valid)
    {
      GtkTreeIter  sibling;

      for (iter_valid = gtk_tree_model_get_iter_first (model, &sibling);
           iter_valid;
           iter_valid = gtk_tree_model_iter_next (model, &sibling))
        {
          gdouble this;

          gtk_tree_model_get (model, &sibling,
                              COLUMN_SCALE, &this,
                              -1);

          if (this < scale)
            break;
        }

      gtk_list_store_insert_before (store, &iter, iter_valid ? &sibling : NULL);
      ligma_scale_combo_box_scale_iter_set (store, &iter, scale, FALSE);
    }

  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo_box), &iter);

  gtk_tree_model_get (model, &iter,
                      COLUMN_PERSISTENT, &persistent,
                      -1);
  if (! persistent)
    {
      ligma_scale_combo_box_mru_add (combo_box, &iter);

      if (gtk_tree_model_iter_n_children (model, NULL) > MAX_ITEMS)
        ligma_scale_combo_box_mru_remove_last (combo_box);
    }

  /* Update entry size appropriately. */
  entry = gtk_bin_get_child (GTK_BIN (combo_box));
  n_digits = (gint) floor (log10 (scale) + 1);

  g_object_set (entry,
                "width-chars", MAX (5, n_digits + 4),
                NULL);
}

gdouble
ligma_scale_combo_box_get_scale (LigmaScaleComboBox *combo_box)
{
  g_return_val_if_fail (LIGMA_IS_SCALE_COMBO_BOX (combo_box), 1.0);

  return combo_box->scale;
}
