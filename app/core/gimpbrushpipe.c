/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1999 Adrian Likins and Tor Lillqvist
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

#include "libligmabase/ligmaparasiteio.h"
#include "libligmamath/ligmamath.h"

#include "core-types.h"

#include "ligmabrush-private.h"
#include "ligmabrushpipe.h"
#include "ligmabrushpipe-load.h"
#include "ligmabrushpipe-save.h"
#include "ligmatempbuf.h"


static void          ligma_brush_pipe_finalize         (GObject          *object);

static gint64        ligma_brush_pipe_get_memsize      (LigmaObject       *object,
                                                       gint64           *gui_size);

static gboolean      ligma_brush_pipe_get_popup_size   (LigmaViewable     *viewable,
                                                       gint              width,
                                                       gint              height,
                                                       gboolean          dot_for_dot,
                                                       gint             *popup_width,
                                                       gint             *popup_height);

static const gchar * ligma_brush_pipe_get_extension    (LigmaData         *data);
static void          ligma_brush_pipe_copy             (LigmaData         *data,
                                                       LigmaData         *src_data);

static void          ligma_brush_pipe_begin_use        (LigmaBrush        *brush);
static void          ligma_brush_pipe_end_use          (LigmaBrush        *brush);
static LigmaBrush   * ligma_brush_pipe_select_brush     (LigmaBrush        *brush,
                                                       const LigmaCoords *last_coords,
                                                       const LigmaCoords *current_coords);
static gboolean      ligma_brush_pipe_want_null_motion (LigmaBrush        *brush,
                                                       const LigmaCoords *last_coords,
                                                       const LigmaCoords *current_coords);


G_DEFINE_TYPE (LigmaBrushPipe, ligma_brush_pipe, LIGMA_TYPE_BRUSH);

#define parent_class ligma_brush_pipe_parent_class


static void
ligma_brush_pipe_class_init (LigmaBrushPipeClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass   *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaViewableClass *viewable_class    = LIGMA_VIEWABLE_CLASS (klass);
  LigmaDataClass     *data_class        = LIGMA_DATA_CLASS (klass);
  LigmaBrushClass    *brush_class       = LIGMA_BRUSH_CLASS (klass);

  object_class->finalize         = ligma_brush_pipe_finalize;

  ligma_object_class->get_memsize = ligma_brush_pipe_get_memsize;

  viewable_class->get_popup_size = ligma_brush_pipe_get_popup_size;

  data_class->save               = ligma_brush_pipe_save;
  data_class->get_extension      = ligma_brush_pipe_get_extension;
  data_class->copy               = ligma_brush_pipe_copy;

  brush_class->begin_use         = ligma_brush_pipe_begin_use;
  brush_class->end_use           = ligma_brush_pipe_end_use;
  brush_class->select_brush      = ligma_brush_pipe_select_brush;
  brush_class->want_null_motion  = ligma_brush_pipe_want_null_motion;
}

static void
ligma_brush_pipe_init (LigmaBrushPipe *pipe)
{
  pipe->current   = NULL;
  pipe->dimension = 0;
  pipe->rank      = NULL;
  pipe->stride    = NULL;
  pipe->n_brushes = 0;
  pipe->brushes   = NULL;
  pipe->select    = NULL;
  pipe->index     = NULL;
}

static void
ligma_brush_pipe_finalize (GObject *object)
{
  LigmaBrushPipe *pipe = LIGMA_BRUSH_PIPE (object);

  g_clear_pointer (&pipe->rank,   g_free);
  g_clear_pointer (&pipe->stride, g_free);
  g_clear_pointer (&pipe->select, g_free);
  g_clear_pointer (&pipe->index,  g_free);
  g_clear_pointer (&pipe->params, g_free);

  if (pipe->brushes)
    {
      gint i;

      for (i = 0; i < pipe->n_brushes; i++)
        if (pipe->brushes[i])
          g_object_unref (pipe->brushes[i]);

      g_clear_pointer (&pipe->brushes, g_free);
    }

  LIGMA_BRUSH (pipe)->priv->mask   = NULL;
  LIGMA_BRUSH (pipe)->priv->pixmap = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
ligma_brush_pipe_get_memsize (LigmaObject *object,
                             gint64     *gui_size)
{
  LigmaBrushPipe *pipe    = LIGMA_BRUSH_PIPE (object);
  gint64         memsize = 0;
  gint           i;

  memsize += pipe->dimension * (sizeof (gint) /* rank   */ +
                                sizeof (gint) /* stride */ +
                                sizeof (PipeSelectModes));

  for (i = 0; i < pipe->n_brushes; i++)
    memsize += ligma_object_get_memsize (LIGMA_OBJECT (pipe->brushes[i]),
                                        gui_size);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_brush_pipe_get_popup_size (LigmaViewable *viewable,
                                gint          width,
                                gint          height,
                                gboolean      dot_for_dot,
                                gint         *popup_width,
                                gint         *popup_height)
{
  return ligma_viewable_get_size (viewable, popup_width, popup_height);
}

static const gchar *
ligma_brush_pipe_get_extension (LigmaData *data)
{
  return LIGMA_BRUSH_PIPE_FILE_EXTENSION;
}

static void
ligma_brush_pipe_copy (LigmaData *data,
                      LigmaData *src_data)
{
  LigmaBrushPipe *pipe     = LIGMA_BRUSH_PIPE (data);
  LigmaBrushPipe *src_pipe = LIGMA_BRUSH_PIPE (src_data);
  gint           i;

  pipe->dimension = src_pipe->dimension;

  g_clear_pointer (&pipe->rank, g_free);
  pipe->rank = g_memdup2 (src_pipe->rank,
                          pipe->dimension * sizeof (gint));

  g_clear_pointer (&pipe->stride, g_free);
  pipe->stride = g_memdup2 (src_pipe->stride,
                            pipe->dimension * sizeof (gint));

  g_clear_pointer (&pipe->select, g_free);
  pipe->select = g_memdup2 (src_pipe->select,
                            pipe->dimension * sizeof (PipeSelectModes));

  g_clear_pointer (&pipe->index, g_free);
  pipe->index = g_memdup2 (src_pipe->index,
                           pipe->dimension * sizeof (gint));

  for (i = 0; i < pipe->n_brushes; i++)
    if (pipe->brushes[i])
      g_object_unref (pipe->brushes[i]);
  g_clear_pointer (&pipe->brushes, g_free);

  pipe->n_brushes = src_pipe->n_brushes;

  pipe->brushes = g_new0 (LigmaBrush *, pipe->n_brushes);
  for (i = 0; i < pipe->n_brushes; i++)
    if (src_pipe->brushes[i])
      {
        pipe->brushes[i] =
          LIGMA_BRUSH (ligma_data_duplicate (LIGMA_DATA (src_pipe->brushes[i])));
        ligma_object_set_name (LIGMA_OBJECT (pipe->brushes[i]),
                              ligma_object_get_name (src_pipe->brushes[i]));
      }

  g_clear_pointer (&pipe->params, g_free);
  pipe->params = g_strdup (src_pipe->params);

  pipe->current = pipe->brushes[0];

  LIGMA_BRUSH (pipe)->priv->spacing  = pipe->current->priv->spacing;
  LIGMA_BRUSH (pipe)->priv->x_axis   = pipe->current->priv->x_axis;
  LIGMA_BRUSH (pipe)->priv->y_axis   = pipe->current->priv->y_axis;
  LIGMA_BRUSH (pipe)->priv->mask     = pipe->current->priv->mask;
  LIGMA_BRUSH (pipe)->priv->pixmap   = pipe->current->priv->pixmap;

  ligma_data_dirty (data);
}

static void
ligma_brush_pipe_begin_use (LigmaBrush *brush)
{
  LigmaBrushPipe *pipe = LIGMA_BRUSH_PIPE (brush);
  gint           i;

  LIGMA_BRUSH_CLASS (parent_class)->begin_use (brush);

  for (i = 0; i < pipe->n_brushes; i++)
    if (pipe->brushes[i])
      ligma_brush_begin_use (pipe->brushes[i]);
}

static void
ligma_brush_pipe_end_use (LigmaBrush *brush)
{
  LigmaBrushPipe *pipe = LIGMA_BRUSH_PIPE (brush);
  gint           i;

  LIGMA_BRUSH_CLASS (parent_class)->end_use (brush);

  for (i = 0; i < pipe->n_brushes; i++)
    if (pipe->brushes[i])
      ligma_brush_end_use (pipe->brushes[i]);
}

static LigmaBrush *
ligma_brush_pipe_select_brush (LigmaBrush        *brush,
                              const LigmaCoords *last_coords,
                              const LigmaCoords *current_coords)
{
  LigmaBrushPipe *pipe = LIGMA_BRUSH_PIPE (brush);
  gint           i, brushix, ix;

  if (pipe->n_brushes == 1)
    return LIGMA_BRUSH (pipe->current);

  brushix = 0;
  for (i = 0; i < pipe->dimension; i++)
    {
      switch (pipe->select[i])
        {
        case PIPE_SELECT_INCREMENTAL:
          ix = (pipe->index[i] + 1) % pipe->rank[i];
          break;

        case PIPE_SELECT_ANGULAR:
          /* Coords angle is already nomalized,
           * offset by 90 degrees is still needed
           * because hoses were made PS compatible*/
          ix = (gint) RINT ((1.0 - current_coords->direction + 0.25) * pipe->rank[i]) % pipe->rank[i];
          break;

        case PIPE_SELECT_VELOCITY:
          ix = ROUND (current_coords->velocity * pipe->rank[i]);
          break;

        case PIPE_SELECT_RANDOM:
          /* This probably isn't the right way */
          ix = g_random_int_range (0, pipe->rank[i]);
          break;

        case PIPE_SELECT_PRESSURE:
          ix = RINT (current_coords->pressure * (pipe->rank[i] - 1));
          break;

        case PIPE_SELECT_TILT_X:
          ix = RINT (current_coords->xtilt / 2.0 * pipe->rank[i]) + pipe->rank[i] / 2;
          break;

        case PIPE_SELECT_TILT_Y:
          ix = RINT (current_coords->ytilt / 2.0 * pipe->rank[i]) + pipe->rank[i] / 2;
          break;

        case PIPE_SELECT_CONSTANT:
        default:
          ix = pipe->index[i];
          break;
        }

      pipe->index[i] = CLAMP (ix, 0, pipe->rank[i] - 1);
      brushix += pipe->stride[i] * pipe->index[i];
    }

  /* Make sure is inside bounds */
  brushix = CLAMP (brushix, 0, pipe->n_brushes - 1);

  pipe->current = pipe->brushes[brushix];

  return LIGMA_BRUSH (pipe->current);
}

static gboolean
ligma_brush_pipe_want_null_motion (LigmaBrush        *brush,
                                  const LigmaCoords *last_coords,
                                  const LigmaCoords *current_coords)
{
  LigmaBrushPipe *pipe = LIGMA_BRUSH_PIPE (brush);
  gint           i;

  if (pipe->n_brushes == 1)
    return TRUE;

  for (i = 0; i < pipe->dimension; i++)
    if (pipe->select[i] == PIPE_SELECT_ANGULAR)
      return FALSE;

  return TRUE;
}


/*  public functions  */

gboolean
ligma_brush_pipe_set_params (LigmaBrushPipe *pipe,
                            const gchar   *paramstring)
{
  gint totalcells;
  gint i;

  g_return_val_if_fail (LIGMA_IS_BRUSH_PIPE (pipe), FALSE);
  g_return_val_if_fail (pipe->dimension == 0, FALSE); /* only on a new pipe! */

  if (paramstring && *paramstring)
    {
      LigmaPixPipeParams params;

      ligma_pixpipe_params_init (&params);
      ligma_pixpipe_params_parse (paramstring, &params);

      pipe->dimension = params.dim;
      pipe->rank      = g_new0 (gint, pipe->dimension);
      pipe->select    = g_new0 (PipeSelectModes, pipe->dimension);
      pipe->index     = g_new0 (gint, pipe->dimension);
      /* placement is not used at all ?? */

      for (i = 0; i < pipe->dimension; i++)
        {
          pipe->rank[i] = MAX (1, params.rank[i]);

          if (strcmp (params.selection[i], "incremental") == 0)
            pipe->select[i] = PIPE_SELECT_INCREMENTAL;
          else if (strcmp (params.selection[i], "angular") == 0)
            pipe->select[i] = PIPE_SELECT_ANGULAR;
          else if (strcmp (params.selection[i], "velocity") == 0)
            pipe->select[i] = PIPE_SELECT_VELOCITY;
          else if (strcmp (params.selection[i], "random") == 0)
            pipe->select[i] = PIPE_SELECT_RANDOM;
          else if (strcmp (params.selection[i], "pressure") == 0)
            pipe->select[i] = PIPE_SELECT_PRESSURE;
          else if (strcmp (params.selection[i], "xtilt") == 0)
            pipe->select[i] = PIPE_SELECT_TILT_X;
          else if (strcmp (params.selection[i], "ytilt") == 0)
            pipe->select[i] = PIPE_SELECT_TILT_Y;
          else
            pipe->select[i] = PIPE_SELECT_CONSTANT;

          pipe->index[i] = 0;
        }

      ligma_pixpipe_params_free (&params);

      pipe->params = g_strdup (paramstring);
    }
  else
    {
      pipe->dimension = 1;
      pipe->rank      = g_new (gint, 1);
      pipe->rank[0]   = pipe->n_brushes;
      pipe->select    = g_new (PipeSelectModes, 1);
      pipe->select[0] = PIPE_SELECT_INCREMENTAL;
      pipe->index     = g_new (gint, 1);
      pipe->index[0]  = 0;
    }

  totalcells = 1; /* Not all necessarily present, maybe */
  for (i = 0; i < pipe->dimension; i++)
    totalcells *= pipe->rank[i];

  pipe->stride = g_new0 (gint, pipe->dimension);

  for (i = 0; i < pipe->dimension; i++)
    {
      if (i == 0)
        pipe->stride[i] = totalcells / pipe->rank[i];
      else
        pipe->stride[i] = pipe->stride[i-1] / pipe->rank[i];
    }

  if (pipe->stride[pipe->dimension - 1] != 1)
    return FALSE;

  return TRUE;
}
