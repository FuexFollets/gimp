/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationgrow.c
 * Copyright (C) 2012 Michael Natterer <mitch@ligma.org>
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmacolor/ligmacolor.h"
#include "libligmamath/ligmamath.h"

#include "operations-types.h"

#include "ligmaoperationgrow.h"


enum
{
  PROP_0,
  PROP_RADIUS_X,
  PROP_RADIUS_Y
};


static void          ligma_operation_grow_get_property (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);
static void          ligma_operation_grow_set_property (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);

static void          ligma_operation_grow_prepare      (GeglOperation       *operation);
static GeglRectangle
          ligma_operation_grow_get_required_for_output (GeglOperation       *self,
                                                       const gchar         *input_pad,
                                                       const GeglRectangle *roi);
static GeglRectangle
                ligma_operation_grow_get_cached_region (GeglOperation       *self,
                                                       const GeglRectangle *roi);

static gboolean ligma_operation_grow_process           (GeglOperation       *operation,
                                                       GeglBuffer          *input,
                                                       GeglBuffer          *output,
                                                       const GeglRectangle *roi,
                                                       gint                 level);


G_DEFINE_TYPE (LigmaOperationGrow, ligma_operation_grow,
               GEGL_TYPE_OPERATION_FILTER)

#define parent_class ligma_operation_grow_parent_class


static void
ligma_operation_grow_class_init (LigmaOperationGrowClass *klass)
{
  GObjectClass             *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->set_property   = ligma_operation_grow_set_property;
  object_class->get_property   = ligma_operation_grow_get_property;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:grow",
                                 "categories",  "ligma",
                                 "description", "LIGMA Grow operation",
                                 NULL);

  operation_class->prepare                 = ligma_operation_grow_prepare;
  operation_class->get_required_for_output = ligma_operation_grow_get_required_for_output;
  operation_class->get_cached_region       = ligma_operation_grow_get_cached_region;
  operation_class->threaded                = FALSE;

  filter_class->process                    = ligma_operation_grow_process;

  g_object_class_install_property (object_class, PROP_RADIUS_X,
                                   g_param_spec_int ("radius-x",
                                                     "Radius X",
                                                     "Grow radius in X direction",
                                                     1, 2342, 1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_RADIUS_Y,
                                   g_param_spec_int ("radius-y",
                                                     "Radius Y",
                                                     "Grow radius in Y direction",
                                                     1, 2342, 1,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));
}

static void
ligma_operation_grow_init (LigmaOperationGrow *self)
{
}

static void
ligma_operation_grow_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
 LigmaOperationGrow *self = LIGMA_OPERATION_GROW (object);

  switch (property_id)
    {
    case PROP_RADIUS_X:
      g_value_set_int (value, self->radius_x);
      break;

    case PROP_RADIUS_Y:
      g_value_set_int (value, self->radius_y);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_operation_grow_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  LigmaOperationGrow *self = LIGMA_OPERATION_GROW (object);

  switch (property_id)
    {
    case PROP_RADIUS_X:
      self->radius_x = g_value_get_int (value);
      break;

    case PROP_RADIUS_Y:
      self->radius_y = g_value_get_int (value);
      break;

   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_operation_grow_prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input",  babl_format_with_space ("Y float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("Y float", space));
}

static GeglRectangle
ligma_operation_grow_get_required_for_output (GeglOperation       *self,
                                             const gchar         *input_pad,
                                             const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static GeglRectangle
ligma_operation_grow_get_cached_region (GeglOperation       *self,
                                       const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (self, "input");
}

static void
compute_border (gint16  *circ,
                guint16  xradius,
                guint16  yradius)
{
  gint32  i;
  gint32  diameter = xradius * 2 + 1;
  gdouble tmp;

  for (i = 0; i < diameter; i++)
    {
      if (i > xradius)
        tmp = (i - xradius) - 0.5;
      else if (i < xradius)
        tmp = (xradius - i) - 0.5;
      else
        tmp = 0.0;

      circ[i] = RINT (yradius /
                      (gdouble) xradius * sqrt (SQR (xradius) - SQR (tmp)));
    }
}

static inline void
rotate_pointers (gfloat  **p,
                 guint32   n)
{
  guint32  i;
  gfloat  *tmp;

  tmp = p[0];

  for (i = 0; i < n - 1; i++)
    p[i] = p[i + 1];

  p[i] = tmp;
}

static gboolean
ligma_operation_grow_process (GeglOperation       *operation,
                             GeglBuffer          *input,
                             GeglBuffer          *output,
                             const GeglRectangle *roi,
                             gint                 level)
{
  /* Any bugs in this function are probably also in thin_region.
   * Blame all bugs in this function on jaycox@ligma.org
   */
  LigmaOperationGrow *self          = LIGMA_OPERATION_GROW (operation);
  const Babl        *input_format  = gegl_operation_get_format (operation, "input");
  const Babl        *output_format = gegl_operation_get_format (operation, "output");
  gint32             i, j, x, y;
  gfloat           **buf;  /* caches the region's pixel data */
  gfloat            *out;  /* holds the new scan line we are computing */
  gfloat           **max;  /* caches the largest values for each column */
  gint16            *circ; /* holds the y coords of the filter's mask */
  gfloat             last_max;
  gint16             last_index;
  gfloat            *buffer;

  max = g_new (gfloat *, roi->width + 2 * self->radius_x);
  buf = g_new (gfloat *, self->radius_y + 1);

  for (i = 0; i < self->radius_y + 1; i++)
    buf[i] = g_new (gfloat, roi->width);

  buffer = g_new (gfloat,
                  (roi->width + 2 * self->radius_x) * (self->radius_y + 1));

  for (i = 0; i < roi->width + 2 * self->radius_x; i++)
    {
      if (i < self->radius_x)
        max[i] = buffer;
      else if (i < roi->width + self->radius_x)
        max[i] = &buffer[(self->radius_y + 1) * (i - self->radius_x)];
      else
        max[i] = &buffer[(self->radius_y + 1) * (roi->width + self->radius_x - 1)];

      for (j = 0; j < self->radius_y + 1; j++)
        max[i][j] = 0.0;
    }

  /* offset the max pointer by self->radius_x so the range of the
   * array is [-self->radius_x] to [roi->width + self->radius_x]
   */
  max += self->radius_x;

  out =  g_new (gfloat, roi->width);

  circ = g_new (gint16, 2 * self->radius_x + 1);
  compute_border (circ, self->radius_x, self->radius_y);

  /* offset the circ pointer by self->radius_x so the range of the
   * array is [-self->radius_x] to [self->radius_x]
   */
  circ += self->radius_x;

  memset (buf[0], 0, roi->width * sizeof (gfloat));

  for (i = 0; i < self->radius_y && i < roi->height; i++) /* load top of image */
    gegl_buffer_get (input,
                     GEGL_RECTANGLE (roi->x, roi->y + i,
                                     roi->width, 1),
                     1.0, input_format, buf[i + 1],
                     GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  for (x = 0; x < roi->width; x++) /* set up max for top of image */
    {
      max[x][0] = 0.0;       /* buf[0][x] is always 0 */
      max[x][1] = buf[1][x]; /* MAX (buf[1][x], max[x][0]) always = buf[1][x]*/

      for (j = 2; j < self->radius_y + 1; j++)
        max[x][j] = MAX (buf[j][x], max[x][j - 1]);
    }

  for (y = 0; y < roi->height; y++)
    {
      rotate_pointers (buf, self->radius_y + 1);

      if (y < roi->height - (self->radius_y))
        gegl_buffer_get (input,
                         GEGL_RECTANGLE (roi->x,  roi->y + y + self->radius_y,
                                         roi->width, 1),
                         1.0, input_format, buf[self->radius_y],
                         GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
      else
        memset (buf[self->radius_y], 0, roi->width * sizeof (gfloat));

      for (x = 0; x < roi->width; x++) /* update max array */
        {
          for (i = self->radius_y; i > 0; i--)
            max[x][i] = MAX (MAX (max[x][i - 1], buf[i - 1][x]), buf[i][x]);

          max[x][0] = buf[0][x];
        }

      last_max = max[0][circ[-1]];
      last_index = 1;

      for (x = 0; x < roi->width; x++) /* render scan line */
        {
          last_index--;

          if (last_index >= 0)
            {
              if (last_max >= 1.0)
                {
                  out[x] = 1.0;
                }
              else
                {
                  last_max = 0.0;

                  for (i = self->radius_x; i >= 0; i--)
                    if (last_max < max[x + i][circ[i]])
                      {
                        last_max = max[x + i][circ[i]];
                        last_index = i;
                      }

                  out[x] = last_max;
                }
            }
          else
            {
              last_index = self->radius_x;
              last_max = max[x + self->radius_x][circ[self->radius_x]];

              for (i = self->radius_x - 1; i >= -self->radius_x; i--)
                if (last_max < max[x + i][circ[i]])
                  {
                    last_max = max[x + i][circ[i]];
                    last_index = i;
                  }

              out[x] = last_max;
            }
        }

      gegl_buffer_set (output,
                       GEGL_RECTANGLE (roi->x, roi->y + y,
                                       roi->width, 1),
                       0, output_format, out,
                       GEGL_AUTO_ROWSTRIDE);
    }

  /* undo the offsets to the pointers so we can free the malloced memory */
  circ -= self->radius_x;
  max -= self->radius_x;

  g_free (circ);
  g_free (buffer);
  g_free (max);

  for (i = 0; i < self->radius_y + 1; i++)
    g_free (buf[i]);

  g_free (buf);
  g_free (out);

  return TRUE;
}
