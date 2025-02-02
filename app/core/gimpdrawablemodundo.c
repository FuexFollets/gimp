/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "gegl/ligma-gegl-utils.h"

#include "ligma-memsize.h"
#include "ligmaimage.h"
#include "ligmadrawable.h"
#include "ligmadrawablemodundo.h"


enum
{
  PROP_0,
  PROP_COPY_BUFFER
};


static void     ligma_drawable_mod_undo_constructed  (GObject             *object);
static void     ligma_drawable_mod_undo_set_property (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void     ligma_drawable_mod_undo_get_property (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);

static gint64   ligma_drawable_mod_undo_get_memsize  (LigmaObject          *object,
                                                     gint64              *gui_size);

static void     ligma_drawable_mod_undo_pop          (LigmaUndo            *undo,
                                                     LigmaUndoMode         undo_mode,
                                                     LigmaUndoAccumulator *accum);
static void     ligma_drawable_mod_undo_free         (LigmaUndo            *undo,
                                                     LigmaUndoMode         undo_mode);


G_DEFINE_TYPE (LigmaDrawableModUndo, ligma_drawable_mod_undo, LIGMA_TYPE_ITEM_UNDO)

#define parent_class ligma_drawable_mod_undo_parent_class


static void
ligma_drawable_mod_undo_class_init (LigmaDrawableModUndoClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaUndoClass   *undo_class        = LIGMA_UNDO_CLASS (klass);

  object_class->constructed      = ligma_drawable_mod_undo_constructed;
  object_class->set_property     = ligma_drawable_mod_undo_set_property;
  object_class->get_property     = ligma_drawable_mod_undo_get_property;

  ligma_object_class->get_memsize = ligma_drawable_mod_undo_get_memsize;

  undo_class->pop                = ligma_drawable_mod_undo_pop;
  undo_class->free               = ligma_drawable_mod_undo_free;

  g_object_class_install_property (object_class, PROP_COPY_BUFFER,
                                   g_param_spec_boolean ("copy-buffer",
                                                         NULL, NULL,
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_drawable_mod_undo_init (LigmaDrawableModUndo *undo)
{
}

static void
ligma_drawable_mod_undo_constructed (GObject *object)
{
  LigmaDrawableModUndo *drawable_mod_undo = LIGMA_DRAWABLE_MOD_UNDO (object);
  LigmaItem            *item;
  LigmaDrawable        *drawable;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma_assert (LIGMA_IS_DRAWABLE (LIGMA_ITEM_UNDO (object)->item));

  item     = LIGMA_ITEM_UNDO (object)->item;
  drawable = LIGMA_DRAWABLE (item);

  if (drawable_mod_undo->copy_buffer)
    {
      drawable_mod_undo->buffer =
        ligma_gegl_buffer_dup (ligma_drawable_get_buffer (drawable));
    }
  else
    {
      drawable_mod_undo->buffer =
        g_object_ref (ligma_drawable_get_buffer (drawable));
    }

  ligma_item_get_offset (item,
                        &drawable_mod_undo->offset_x,
                        &drawable_mod_undo->offset_y);
}

static void
ligma_drawable_mod_undo_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  LigmaDrawableModUndo *drawable_mod_undo = LIGMA_DRAWABLE_MOD_UNDO (object);

  switch (property_id)
    {
    case PROP_COPY_BUFFER:
      drawable_mod_undo->copy_buffer = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_drawable_mod_undo_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  LigmaDrawableModUndo *drawable_mod_undo = LIGMA_DRAWABLE_MOD_UNDO (object);

  switch (property_id)
    {
    case PROP_COPY_BUFFER:
      g_value_set_boolean (value, drawable_mod_undo->copy_buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_drawable_mod_undo_get_memsize (LigmaObject *object,
                                    gint64     *gui_size)
{
  LigmaDrawableModUndo *drawable_mod_undo = LIGMA_DRAWABLE_MOD_UNDO (object);
  gint64               memsize           = 0;

  memsize += ligma_gegl_buffer_get_memsize (drawable_mod_undo->buffer);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
ligma_drawable_mod_undo_pop (LigmaUndo            *undo,
                            LigmaUndoMode         undo_mode,
                            LigmaUndoAccumulator *accum)
{
  LigmaDrawableModUndo *drawable_mod_undo = LIGMA_DRAWABLE_MOD_UNDO (undo);
  LigmaDrawable        *drawable          = LIGMA_DRAWABLE (LIGMA_ITEM_UNDO (undo)->item);
  GeglBuffer          *buffer;
  gint                 offset_x;
  gint                 offset_y;

  LIGMA_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  buffer   = drawable_mod_undo->buffer;
  offset_x = drawable_mod_undo->offset_x;
  offset_y = drawable_mod_undo->offset_y;

  drawable_mod_undo->buffer = g_object_ref (ligma_drawable_get_buffer (drawable));

  ligma_item_get_offset (LIGMA_ITEM (drawable),
                        &drawable_mod_undo->offset_x,
                        &drawable_mod_undo->offset_y);

  ligma_drawable_set_buffer_full (drawable, FALSE, NULL,
                                 buffer,
                                 GEGL_RECTANGLE (offset_x, offset_y, 0, 0),
                                 TRUE);
  g_object_unref (buffer);
}

static void
ligma_drawable_mod_undo_free (LigmaUndo     *undo,
                             LigmaUndoMode  undo_mode)
{
  LigmaDrawableModUndo *drawable_mod_undo = LIGMA_DRAWABLE_MOD_UNDO (undo);

  g_clear_object (&drawable_mod_undo->buffer);

  LIGMA_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
