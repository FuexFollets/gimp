/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include <glib-object.h>

#include "core/core-types.h"

#include "core/ligmaidtable.h"


#define ADD_TEST(function) \
  g_test_add ("/ligmaidtable/" #function, \
              LigmaTestFixture, \
              NULL, \
              ligma_test_id_table_setup, \
              ligma_test_id_table_ ## function, \
              ligma_test_id_table_teardown);


typedef struct
{
  LigmaIdTable *id_table;
} LigmaTestFixture;


static gpointer data1 = (gpointer) 0x00000001;
static gpointer data2 = (gpointer) 0x00000002;


static void
ligma_test_id_table_setup (LigmaTestFixture *fixture,
                          gconstpointer    data)
{
  fixture->id_table = ligma_id_table_new ();
}

static void
ligma_test_id_table_teardown (LigmaTestFixture *fixture,
                             gconstpointer    data)
{
  g_object_unref (fixture->id_table);
  fixture->id_table = NULL;
}

/**
 * ligma_test_id_table_insert_and_lookup:
 *
 * Test that insert and lookup works.
 **/
static void
ligma_test_id_table_insert_and_lookup (LigmaTestFixture *f,
                                      gconstpointer    data)
{
  gint     ret_id   = ligma_id_table_insert (f->id_table, data1);
  gpointer ret_data = ligma_id_table_lookup (f->id_table, ret_id);

  g_assert (ret_data == data1);
}

/**
 * ligma_test_id_table_insert_twice:
 *
 * Test that two consecutive inserts generates different IDs.
 **/
static void
ligma_test_id_table_insert_twice (LigmaTestFixture *f,
                                 gconstpointer    data)
{
  gint     ret_id    = ligma_id_table_insert (f->id_table, data1);
  gpointer ret_data  = ligma_id_table_lookup (f->id_table, ret_id);
  gint     ret_id2   = ligma_id_table_insert (f->id_table, data2);
  gpointer ret_data2 = ligma_id_table_lookup (f->id_table, ret_id2);

  g_assert (ret_id    != ret_id2);
  g_assert (ret_data  == data1);
  g_assert (ret_data2 == data2);
}

/**
 * ligma_test_id_table_insert_with_id:
 *
 * Test that it is possible to insert data with a specific ID.
 **/
static void
ligma_test_id_table_insert_with_id (LigmaTestFixture *f,
                                   gconstpointer    data)
{
  const int id = 10;

  int      ret_id   = ligma_id_table_insert_with_id (f->id_table, id, data1);
  gpointer ret_data = ligma_id_table_lookup (f->id_table, id);

  g_assert (ret_id   == id);
  g_assert (ret_data == data1);
}

/**
 * ligma_test_id_table_insert_with_id_existing:
 *
 * Test that it is not possible to insert data with a specific ID if
 * that ID already is inserted.
 **/
static void
ligma_test_id_table_insert_with_id_existing (LigmaTestFixture *f,
                                            gconstpointer    data)
{
  const int id = 10;

  int      ret_id    = ligma_id_table_insert_with_id (f->id_table, id, data1);
  gpointer ret_data  = ligma_id_table_lookup (f->id_table, ret_id);
  int      ret_id2   = ligma_id_table_insert_with_id (f->id_table, id, data2);
  gpointer ret_data2 = ligma_id_table_lookup (f->id_table, ret_id2);

  g_assert (id        == ret_id);
  g_assert (ret_id2   == -1);
  g_assert (ret_data  == data1);
  g_assert (ret_data2 == NULL);
}

/**
 * ligma_test_id_table_replace:
 *
 * Test that it is possible to replace data with a given ID with
 * different data.
 **/
static void
ligma_test_id_table_replace (LigmaTestFixture *f,
                            gconstpointer    data)
{
  int ret_id = ligma_id_table_insert (f->id_table, data1);
  gpointer ret_data;

  ligma_id_table_replace (f->id_table, ret_id, data2);
  ret_data = ligma_id_table_lookup (f->id_table, ret_id);

  g_assert (ret_data  == data2);
}

/**
 * ligma_test_id_table_replace_as_insert:
 *
 * Test that replace works like insert when there is no data to
 * replace.
 **/
static void
ligma_test_id_table_replace_as_insert (LigmaTestFixture *f,
                                      gconstpointer    data)
{
  const int id = 10;

  gpointer ret_data;

  ligma_id_table_replace (f->id_table, id, data1);
  ret_data = ligma_id_table_lookup (f->id_table, id);

  g_assert (ret_data  == data1);
}

/**
 * ligma_test_id_table_remove:
 *
 * Test that it is possible to remove data identified by the ID:
 **/
static void
ligma_test_id_table_remove (LigmaTestFixture *f,
                           gconstpointer    data)
{
  gint     ret_id            = ligma_id_table_insert (f->id_table, data1);
  void    *ret_data          = ligma_id_table_lookup (f->id_table, ret_id);
  gboolean remove_successful = ligma_id_table_remove (f->id_table, ret_id);
  void    *ret_data2         = ligma_id_table_lookup (f->id_table, ret_id);

  g_assert (remove_successful);
  g_assert (ret_data == data1);
  g_assert (ret_data2 == NULL);
}

/**
 * ligma_test_id_table_remove_non_existing:
 *
 * Tests that things work properly when trying to remove data with an
 * ID that doesn't exist.
 **/
static void
ligma_test_id_table_remove_non_existing (LigmaTestFixture *f,
                                        gconstpointer    data)
{
  const int id = 10;

  gboolean remove_successful = ligma_id_table_remove (f->id_table, id);
  void    *ret_data          = ligma_id_table_lookup (f->id_table, id);

  g_assert (! remove_successful);
  g_assert (ret_data == NULL);
}

int main(int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  ADD_TEST (insert_and_lookup);
  ADD_TEST (insert_twice);
  ADD_TEST (insert_with_id);
  ADD_TEST (insert_with_id_existing);
  ADD_TEST (replace);
  ADD_TEST (replace_as_insert);
  ADD_TEST (remove);
  ADD_TEST (remove_non_existing);

  return g_test_run ();
}
