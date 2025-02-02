/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaprogress_pdb.c
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

/* NOTE: This file is auto-generated by pdbgen.pl */

#include "config.h"

#include "stamp-pdbgen.h"

#include "ligma.h"


/**
 * SECTION: ligmaprogress
 * @title: ligmaprogress
 * @short_description: Functions for embedding the progress bar into a plug-in's GUI.
 *
 * Functions for embedding the progress bar into a plug-in's GUI.
 **/


/**
 * _ligma_progress_init:
 * @message: Message to use in the progress dialog.
 * @gdisplay: (nullable): LigmaDisplay to update progressbar in, or %NULL for a separate window.
 *
 * Initializes the progress bar for the current plug-in.
 *
 * Initializes the progress bar for the current plug-in. It is only
 * valid to call this procedure from a plug-in.
 *
 * Returns: TRUE on success.
 **/
gboolean
_ligma_progress_init (const gchar *message,
                     LigmaDisplay *gdisplay)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, message,
                                          LIGMA_TYPE_DISPLAY, gdisplay,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-progress-init",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * _ligma_progress_update:
 * @percentage: Percentage of progress completed which must be between 0.0 and 1.0.
 *
 * Updates the progress bar for the current plug-in.
 *
 * Updates the progress bar for the current plug-in. It is only valid
 * to call this procedure from a plug-in.
 *
 * Returns: TRUE on success.
 **/
gboolean
_ligma_progress_update (gdouble percentage)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          G_TYPE_DOUBLE, percentage,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-progress-update",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_progress_pulse:
 *
 * Pulses the progress bar for the current plug-in.
 *
 * Updates the progress bar for the current plug-in. It is only valid
 * to call this procedure from a plug-in. Use this function instead of
 * ligma_progress_update() if you cannot tell how much progress has been
 * made. This usually causes the the progress bar to enter \"activity
 * mode\", where a block bounces back and forth.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
ligma_progress_pulse (void)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-progress-pulse",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_progress_set_text:
 * @message: Message to use in the progress dialog.
 *
 * Changes the text in the progress bar for the current plug-in.
 *
 * This function changes the text in the progress bar for the current
 * plug-in. Unlike ligma_progress_init() it does not change the
 * displayed value.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
ligma_progress_set_text (const gchar *message)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, message,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-progress-set-text",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_progress_end:
 *
 * Ends the progress bar for the current plug-in.
 *
 * Ends the progress display for the current plug-in. Most plug-ins
 * don't need to call this, they just exit when the work is done. It is
 * only valid to call this procedure from a plug-in.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.4
 **/
gboolean
ligma_progress_end (void)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-progress-end",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_progress_get_window_handle:
 *
 * Returns the native window ID of the toplevel window this plug-in's
 * progress is displayed in.
 *
 * This function returns the native window ID of the toplevel window
 * this plug-in\'s progress is displayed in.
 *
 * Returns: The progress bar's toplevel window.
 *
 * Since: 2.2
 **/
gint
ligma_progress_get_window_handle (void)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gint window = 0;

  args = ligma_value_array_new_from_types (NULL,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-progress-get-window-handle",
                                              args);
  ligma_value_array_unref (args);

  if (LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS)
    window = LIGMA_VALUES_GET_INT (return_vals, 1);

  ligma_value_array_unref (return_vals);

  return window;
}

/**
 * _ligma_progress_install:
 * @progress_callback: The callback PDB proc to call.
 *
 * Installs a progress callback for the current plug-in.
 *
 * This function installs a temporary PDB procedure which will handle
 * all progress calls made by this plug-in and any procedure it calls.
 * Calling this function multiple times simply replaces the old
 * progress callbacks.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.2
 **/
gboolean
_ligma_progress_install (const gchar *progress_callback)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, progress_callback,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-progress-install",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * _ligma_progress_uninstall:
 * @progress_callback: The name of the callback registered for this progress.
 *
 * Uninstalls the progress callback for the current plug-in.
 *
 * This function uninstalls any progress callback installed with
 * ligma_progress_install() before.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.2
 **/
gboolean
_ligma_progress_uninstall (const gchar *progress_callback)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, progress_callback,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-progress-uninstall",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_progress_cancel:
 * @progress_callback: The name of the callback registered for this progress.
 *
 * Cancels a running progress.
 *
 * This function cancels the currently running progress.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.2
 **/
gboolean
ligma_progress_cancel (const gchar *progress_callback)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          G_TYPE_STRING, progress_callback,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-progress-cancel",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}
