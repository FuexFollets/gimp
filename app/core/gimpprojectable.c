/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaprojectable.c
 * Copyright (C) 2008  Michael Natterer <mitch@ligma.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "ligmamarshal.h"
#include "ligmaprojectable.h"
#include "ligmaviewable.h"


enum
{
  INVALIDATE,
  FLUSH,
  STRUCTURE_CHANGED,
  BOUNDS_CHANGED,
  LAST_SIGNAL
};


G_DEFINE_INTERFACE (LigmaProjectable, ligma_projectable, LIGMA_TYPE_VIEWABLE)


static guint projectable_signals[LAST_SIGNAL] = { 0 };


/*  private functions  */


static void
ligma_projectable_default_init (LigmaProjectableInterface *iface)
{
  projectable_signals[INVALIDATE] =
    g_signal_new ("invalidate",
                  G_TYPE_FROM_CLASS (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaProjectableInterface, invalidate),
                  NULL, NULL,
                  ligma_marshal_VOID__INT_INT_INT_INT,
                  G_TYPE_NONE, 4,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT);

  projectable_signals[FLUSH] =
    g_signal_new ("flush",
                  G_TYPE_FROM_CLASS (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaProjectableInterface, flush),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  projectable_signals[STRUCTURE_CHANGED] =
    g_signal_new ("structure-changed",
                  G_TYPE_FROM_CLASS (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaProjectableInterface, structure_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  projectable_signals[BOUNDS_CHANGED] =
    g_signal_new ("bounds-changed",
                  G_TYPE_FROM_CLASS (iface),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaProjectableInterface, bounds_changed),
                  NULL, NULL,
                  ligma_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2,
                  G_TYPE_INT,
                  G_TYPE_INT);
}


/*  public functions  */

void
ligma_projectable_invalidate (LigmaProjectable *projectable,
                             gint             x,
                             gint             y,
                             gint             width,
                             gint             height)
{
  g_return_if_fail (LIGMA_IS_PROJECTABLE (projectable));

  g_signal_emit (projectable, projectable_signals[INVALIDATE], 0,
                 x, y, width, height);
}

void
ligma_projectable_flush (LigmaProjectable *projectable,
                        gboolean         preview_invalidated)
{
  g_return_if_fail (LIGMA_IS_PROJECTABLE (projectable));

  g_signal_emit (projectable, projectable_signals[FLUSH], 0,
                 preview_invalidated);
}

void
ligma_projectable_structure_changed (LigmaProjectable *projectable)
{
  g_return_if_fail (LIGMA_IS_PROJECTABLE (projectable));

  g_signal_emit (projectable, projectable_signals[STRUCTURE_CHANGED], 0);
}

void
ligma_projectable_bounds_changed (LigmaProjectable *projectable,
                                 gint             old_x,
                                 gint             old_y)
{
  g_return_if_fail (LIGMA_IS_PROJECTABLE (projectable));

  g_signal_emit (projectable, projectable_signals[BOUNDS_CHANGED], 0,
                 old_x, old_y);
}

LigmaImage *
ligma_projectable_get_image (LigmaProjectable *projectable)
{
  LigmaProjectableInterface *iface;

  g_return_val_if_fail (LIGMA_IS_PROJECTABLE (projectable), NULL);

  iface = LIGMA_PROJECTABLE_GET_IFACE (projectable);

  if (iface->get_image)
    return iface->get_image (projectable);

  return NULL;
}

const Babl *
ligma_projectable_get_format (LigmaProjectable *projectable)
{
  LigmaProjectableInterface *iface;

  g_return_val_if_fail (LIGMA_IS_PROJECTABLE (projectable), NULL);

  iface = LIGMA_PROJECTABLE_GET_IFACE (projectable);

  if (iface->get_format)
    return iface->get_format (projectable);

  return 0;
}

void
ligma_projectable_get_offset (LigmaProjectable *projectable,
                             gint            *x,
                             gint            *y)
{
  LigmaProjectableInterface *iface;

  g_return_if_fail (LIGMA_IS_PROJECTABLE (projectable));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);

  iface = LIGMA_PROJECTABLE_GET_IFACE (projectable);

  *x = 0;
  *y = 0;

  if (iface->get_offset)
    iface->get_offset (projectable, x, y);
}

GeglRectangle
ligma_projectable_get_bounding_box (LigmaProjectable *projectable)
{
  LigmaProjectableInterface *iface;
  GeglRectangle             result = {};

  g_return_val_if_fail (LIGMA_IS_PROJECTABLE (projectable), result);

  iface = LIGMA_PROJECTABLE_GET_IFACE (projectable);

  if (iface->get_bounding_box)
    result = iface->get_bounding_box (projectable);

  return result;
}

GeglNode *
ligma_projectable_get_graph (LigmaProjectable *projectable)
{
  LigmaProjectableInterface *iface;

  g_return_val_if_fail (LIGMA_IS_PROJECTABLE (projectable), NULL);

  iface = LIGMA_PROJECTABLE_GET_IFACE (projectable);

  if (iface->get_graph)
    return iface->get_graph (projectable);

  return NULL;
}

void
ligma_projectable_begin_render (LigmaProjectable *projectable)
{
  LigmaProjectableInterface *iface;

  g_return_if_fail (LIGMA_IS_PROJECTABLE (projectable));

  iface = LIGMA_PROJECTABLE_GET_IFACE (projectable);

  if (iface->begin_render)
    iface->begin_render (projectable);
}

void
ligma_projectable_end_render (LigmaProjectable *projectable)
{
  LigmaProjectableInterface *iface;

  g_return_if_fail (LIGMA_IS_PROJECTABLE (projectable));

  iface = LIGMA_PROJECTABLE_GET_IFACE (projectable);

  if (iface->end_render)
    iface->end_render (projectable);
}

void
ligma_projectable_invalidate_preview (LigmaProjectable *projectable)
{
  LigmaProjectableInterface *iface;

  g_return_if_fail (LIGMA_IS_PROJECTABLE (projectable));

  iface = LIGMA_PROJECTABLE_GET_IFACE (projectable);

  if (iface->invalidate_preview)
    iface->invalidate_preview (projectable);
}
