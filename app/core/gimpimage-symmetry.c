/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimage-symmetry.c
 * Copyright (C) 2015 Jehan <jehan@ligma.org>
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
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "core-types.h"

#include "ligmasymmetry.h"
#include "ligmaimage.h"
#include "ligmaimage-private.h"
#include "ligmaimage-symmetry.h"
#include "ligmasymmetry-mandala.h"
#include "ligmasymmetry-mirror.h"
#include "ligmasymmetry-tiling.h"


/**
 * ligma_image_symmetry_list:
 *
 * Returns a list of #GType of all existing symmetries.
 **/
GList *
ligma_image_symmetry_list (void)
{
  GList *list = NULL;

  list = g_list_prepend (list, GINT_TO_POINTER (LIGMA_TYPE_MIRROR));
  list = g_list_prepend (list, GINT_TO_POINTER (LIGMA_TYPE_TILING));
  list = g_list_prepend (list, GINT_TO_POINTER (LIGMA_TYPE_MANDALA));

  return list;
}

/**
 * ligma_image_symmetry_new:
 * @image: the #LigmaImage
 * @type:  the #GType of the symmetry
 *
 * Creates a new #LigmaSymmetry of @type attached to @image.
 * @type must be a subtype of `LIGMA_TYPE_SYMMETRY`.
 * Note that using the base @type `LIGMA_TYPE_SYMMETRY` creates an
 * identity transformation.
 *
 * Returns: the new #LigmaSymmetry.
 **/
LigmaSymmetry *
ligma_image_symmetry_new (LigmaImage *image,
                         GType      type)
{
  LigmaSymmetry *sym = NULL;

  g_return_val_if_fail (g_type_is_a (type, LIGMA_TYPE_SYMMETRY), NULL);

  sym = g_object_new (type,
                      "image", image,
                      NULL);

  return sym;
}

/**
 * ligma_image_symmetry_add:
 * @image: the #LigmaImage
 * @type:  the #GType of the symmetry
 *
 * Add a symmetry of type @type to @image and make it the
 * active transformation.
 **/
void
ligma_image_symmetry_add (LigmaImage    *image,
                         LigmaSymmetry *sym)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_IMAGE (image));
  g_return_if_fail (LIGMA_IS_SYMMETRY (sym));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  private->symmetries = g_list_prepend (private->symmetries,
                                        g_object_ref (sym));
}

/**
 * ligma_image_symmetry_remove:
 * @image:   the #LigmaImage
 * @sym: the #LigmaSymmetry
 *
 * Remove @sym from the list of symmetries of @image.
 * If it was the active transformation, unselect it first.
 **/
void
ligma_image_symmetry_remove (LigmaImage    *image,
                            LigmaSymmetry *sym)
{
  LigmaImagePrivate *private;

  g_return_if_fail (LIGMA_IS_SYMMETRY (sym));
  g_return_if_fail (LIGMA_IS_IMAGE (image));

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  if (private->active_symmetry == sym)
    ligma_image_set_active_symmetry (image, LIGMA_TYPE_SYMMETRY);

  private->symmetries = g_list_remove (private->symmetries, sym);
  g_object_unref (sym);
}

/**
 * ligma_image_symmetry_get:
 * @image: the #LigmaImage
 *
 * Returns: the list of #LigmaSymmetry set on @image.
 * The returned list belongs to @image and should not be freed.
 **/
GList *
ligma_image_symmetry_get (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return private->symmetries;
}

/**
 * ligma_image_set_active_symmetry:
 * @image: the #LigmaImage
 * @type:  the #GType of the symmetry
 *
 * Select the symmetry of type @type.
 * Using the GType allows to select a transformation without
 * knowing whether one of the same @type was already created.
 *
 * Returns TRUE on success, FALSE if no such symmetry was found.
 **/
gboolean
ligma_image_set_active_symmetry (LigmaImage *image,
                                GType      type)
{
  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  g_object_set (image,
                "symmetry", type,
                NULL);

  return TRUE;
}

/**
 * ligma_image_get_active_symmetry:
 * @image: the #LigmaImage
 *
 * Returns the #LigmaSymmetry transformation active on @image.
 **/
LigmaSymmetry *
ligma_image_get_active_symmetry (LigmaImage *image)
{
  LigmaImagePrivate *private;

  g_return_val_if_fail (LIGMA_IS_IMAGE (image), FALSE);

  private = LIGMA_IMAGE_GET_PRIVATE (image);

  return private->active_symmetry;
}
