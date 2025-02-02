/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

/* NOTE: This file is auto-generated by pdbgen.pl. */

#include "config.h"

#include "stamp-pdbgen.h"

#include <gegl.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"

#include "pdb-types.h"

#include "core/ligma.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmaimage.h"
#include "core/ligmaparamspecs.h"
#include "plug-in/ligmaplugin-cleanup.h"
#include "plug-in/ligmaplugin.h"
#include "plug-in/ligmapluginmanager.h"

#include "ligmapdb.h"
#include "ligmaprocedure.h"
#include "internal-procs.h"


static LigmaValueArray *
image_undo_group_start_invoker (LigmaProcedure         *procedure,
                                Ligma                  *ligma,
                                LigmaContext           *context,
                                LigmaProgress          *progress,
                                const LigmaValueArray  *args,
                                GError               **error)
{
  gboolean success = TRUE;
  LigmaImage *image;

  image = g_value_get_object (ligma_value_array_index (args, 0));

  if (success)
    {
      LigmaPlugIn  *plug_in   = ligma->plug_in_manager->current_plug_in;
      const gchar *undo_desc = NULL;

      if (plug_in)
        {
          success = ligma_plug_in_cleanup_undo_group_start (plug_in, image);

          if (success)
            undo_desc = ligma_plug_in_get_undo_desc (plug_in);
        }

      if (success)
        ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_MISC, undo_desc);
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static LigmaValueArray *
image_undo_group_end_invoker (LigmaProcedure         *procedure,
                              Ligma                  *ligma,
                              LigmaContext           *context,
                              LigmaProgress          *progress,
                              const LigmaValueArray  *args,
                              GError               **error)
{
  gboolean success = TRUE;
  LigmaImage *image;

  image = g_value_get_object (ligma_value_array_index (args, 0));

  if (success)
    {
      LigmaPlugIn *plug_in = ligma->plug_in_manager->current_plug_in;

      if (plug_in)
        success = ligma_plug_in_cleanup_undo_group_end (plug_in, image);

      if (success)
        ligma_image_undo_group_end (image);
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static LigmaValueArray *
image_undo_is_enabled_invoker (LigmaProcedure         *procedure,
                               Ligma                  *ligma,
                               LigmaContext           *context,
                               LigmaProgress          *progress,
                               const LigmaValueArray  *args,
                               GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaImage *image;
  gboolean enabled = FALSE;

  image = g_value_get_object (ligma_value_array_index (args, 0));

  if (success)
    {
      enabled = ligma_image_undo_is_enabled (image);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_boolean (ligma_value_array_index (return_vals, 1), enabled);

  return return_vals;
}

static LigmaValueArray *
image_undo_disable_invoker (LigmaProcedure         *procedure,
                            Ligma                  *ligma,
                            LigmaContext           *context,
                            LigmaProgress          *progress,
                            const LigmaValueArray  *args,
                            GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaImage *image;
  gboolean disabled = FALSE;

  image = g_value_get_object (ligma_value_array_index (args, 0));

  if (success)
    {
    #if 0
      LigmaPlugIn *plug_in = ligma->plug_in_manager->current_plug_in;

      if (plug_in)
        success = ligma_plug_in_cleanup_undo_disable (plug_in, image);
    #endif

      if (success)
        disabled = ligma_image_undo_disable (image);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_boolean (ligma_value_array_index (return_vals, 1), disabled);

  return return_vals;
}

static LigmaValueArray *
image_undo_enable_invoker (LigmaProcedure         *procedure,
                           Ligma                  *ligma,
                           LigmaContext           *context,
                           LigmaProgress          *progress,
                           const LigmaValueArray  *args,
                           GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaImage *image;
  gboolean enabled = FALSE;

  image = g_value_get_object (ligma_value_array_index (args, 0));

  if (success)
    {
    #if 0
      LigmaPlugIn *plug_in = ligma->plug_in_manager->current_plug_in;

      if (plug_in)
        success = ligma_plug_in_cleanup_undo_enable (plug_in, image);
    #endif

      if (success)
        enabled = ligma_image_undo_enable (image);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_boolean (ligma_value_array_index (return_vals, 1), enabled);

  return return_vals;
}

static LigmaValueArray *
image_undo_freeze_invoker (LigmaProcedure         *procedure,
                           Ligma                  *ligma,
                           LigmaContext           *context,
                           LigmaProgress          *progress,
                           const LigmaValueArray  *args,
                           GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaImage *image;
  gboolean frozen = FALSE;

  image = g_value_get_object (ligma_value_array_index (args, 0));

  if (success)
    {
    #if 0
      LigmaPlugIn *plug_in = ligma->plug_in_manager->current_plug_in;

      if (plug_in)
        success = ligma_plug_in_cleanup_undo_freeze (plug_in, image);
    #endif

      if (success)
        frozen = ligma_image_undo_freeze (image);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_boolean (ligma_value_array_index (return_vals, 1), frozen);

  return return_vals;
}

static LigmaValueArray *
image_undo_thaw_invoker (LigmaProcedure         *procedure,
                         Ligma                  *ligma,
                         LigmaContext           *context,
                         LigmaProgress          *progress,
                         const LigmaValueArray  *args,
                         GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaImage *image;
  gboolean thawed = FALSE;

  image = g_value_get_object (ligma_value_array_index (args, 0));

  if (success)
    {
    #if 0
      LigmaPlugIn *plug_in = ligma->plug_in_manager->current_plug_in;

      if (plug_in)
        success = ligma_plug_in_cleanup_undo_thaw (plug_in, image);
    #endif

      if (success)
        thawed = ligma_image_undo_thaw (image);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_boolean (ligma_value_array_index (return_vals, 1), thawed);

  return return_vals;
}

void
register_image_undo_procs (LigmaPDB *pdb)
{
  LigmaProcedure *procedure;

  /*
   * ligma-image-undo-group-start
   */
  procedure = ligma_procedure_new (image_undo_group_start_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-image-undo-group-start");
  ligma_procedure_set_static_help (procedure,
                                  "Starts a group undo.",
                                  "This function is used to start a group undo--necessary for logically combining two or more undo operations into a single operation. This call must be used in conjunction with a 'ligma-image-undo-group-end' call.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1997");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "image",
                                                      "The ID of the image in which to open an undo group",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-image-undo-group-end
   */
  procedure = ligma_procedure_new (image_undo_group_end_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-image-undo-group-end");
  ligma_procedure_set_static_help (procedure,
                                  "Finish a group undo.",
                                  "This function must be called once for each 'ligma-image-undo-group-start' call that is made.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1997");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "image",
                                                      "The ID of the image in which to close an undo group",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-image-undo-is-enabled
   */
  procedure = ligma_procedure_new (image_undo_is_enabled_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-image-undo-is-enabled");
  ligma_procedure_set_static_help (procedure,
                                  "Check if the image's undo stack is enabled.",
                                  "This procedure checks if the image's undo stack is currently enabled or disabled. This is useful when several plug-ins or scripts call each other and want to check if their caller has already used 'ligma-image-undo-disable' or 'ligma-image-undo-freeze'.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Rapha\xc3\xabl Quinet <raphael@ligma.org>",
                                         "Rapha\xc3\xabl Quinet",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "image",
                                                      "The image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_boolean ("enabled",
                                                         "enabled",
                                                         "TRUE if undo is enabled for this image",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-image-undo-disable
   */
  procedure = ligma_procedure_new (image_undo_disable_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-image-undo-disable");
  ligma_procedure_set_static_help (procedure,
                                  "Disable the image's undo stack.",
                                  "This procedure disables the image's undo stack, allowing subsequent operations to ignore their undo steps. This is generally called in conjunction with 'ligma-image-undo-enable' to temporarily disable an image undo stack. This is advantageous because saving undo steps can be time and memory intensive.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1995-1996");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "image",
                                                      "The image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_boolean ("disabled",
                                                         "disabled",
                                                         "TRUE if the image undo has been disabled",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-image-undo-enable
   */
  procedure = ligma_procedure_new (image_undo_enable_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-image-undo-enable");
  ligma_procedure_set_static_help (procedure,
                                  "Enable the image's undo stack.",
                                  "This procedure enables the image's undo stack, allowing subsequent operations to store their undo steps. This is generally called in conjunction with 'ligma-image-undo-disable' to temporarily disable an image undo stack.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Spencer Kimball & Peter Mattis",
                                         "Spencer Kimball & Peter Mattis",
                                         "1995-1996");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "image",
                                                      "The image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_boolean ("enabled",
                                                         "enabled",
                                                         "TRUE if the image undo has been enabled",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-image-undo-freeze
   */
  procedure = ligma_procedure_new (image_undo_freeze_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-image-undo-freeze");
  ligma_procedure_set_static_help (procedure,
                                  "Freeze the image's undo stack.",
                                  "This procedure freezes the image's undo stack, allowing subsequent operations to ignore their undo steps. This is generally called in conjunction with 'ligma-image-undo-thaw' to temporarily disable an image undo stack. This is advantageous because saving undo steps can be time and memory intensive. 'ligma-image-undo-freeze' / 'ligma-image-undo-thaw' and 'ligma-image-undo-disable' / 'ligma-image-undo-enable' differ in that the former does not free up all undo steps when undo is thawed, so is more suited to interactive in-situ previews. It is important in this case that the image is back to the same state it was frozen in before thawing, else 'undo' behaviour is undefined.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Adam D. Moss",
                                         "Adam D. Moss",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "image",
                                                      "The image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_boolean ("frozen",
                                                         "frozen",
                                                         "TRUE if the image undo has been frozen",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-image-undo-thaw
   */
  procedure = ligma_procedure_new (image_undo_thaw_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-image-undo-thaw");
  ligma_procedure_set_static_help (procedure,
                                  "Thaw the image's undo stack.",
                                  "This procedure thaws the image's undo stack, allowing subsequent operations to store their undo steps. This is generally called in conjunction with 'ligma-image-undo-freeze' to temporarily freeze an image undo stack. 'ligma-image-undo-thaw' does NOT free the undo stack as 'ligma-image-undo-enable' does, so is suited for situations where one wishes to leave the undo stack in the same state in which one found it despite non-destructively playing with the image in the meantime. An example would be in-situ plug-in previews. Balancing freezes and thaws and ensuring image consistency is the responsibility of the caller.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Adam D. Moss",
                                         "Adam D. Moss",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_image ("image",
                                                      "image",
                                                      "The image",
                                                      FALSE,
                                                      LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_boolean ("thawed",
                                                         "thawed",
                                                         "TRUE if the image undo has been thawed",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
