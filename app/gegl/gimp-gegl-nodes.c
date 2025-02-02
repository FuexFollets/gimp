/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-gegl-nodes.h
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

#include <gegl.h>

#include "ligma-gegl-types.h"

#include "operations/layer-modes/ligma-layer-modes.h"

#include "ligma-gegl-nodes.h"
#include "ligma-gegl-utils.h"


GeglNode *
ligma_gegl_create_flatten_node (const LigmaRGB       *background,
                               const Babl          *space,
                               LigmaLayerColorSpace  composite_space)
{
  GeglNode  *node;
  GeglNode  *input;
  GeglNode  *output;
  GeglNode  *color;
  GeglNode  *mode;
  GeglColor *c;

  g_return_val_if_fail (background != NULL, NULL);
  g_return_val_if_fail (composite_space == LIGMA_LAYER_COLOR_SPACE_RGB_LINEAR ||
                        composite_space == LIGMA_LAYER_COLOR_SPACE_RGB_PERCEPTUAL,
                        NULL);

  node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  c = ligma_gegl_color_new (background, space);
  color = gegl_node_new_child (node,
                               "operation", "gegl:color",
                               "value",     c,
                               "format",    ligma_layer_mode_get_format (
                                              LIGMA_LAYER_MODE_NORMAL,
                                              LIGMA_LAYER_COLOR_SPACE_AUTO,
                                              composite_space,
                                              LIGMA_LAYER_COMPOSITE_AUTO,
                                              NULL),
                               NULL);
  g_object_unref (c);

  ligma_gegl_node_set_underlying_operation (node, color);

  mode = gegl_node_new_child (node,
                              "operation", "ligma:normal",
                              NULL);
  ligma_gegl_mode_node_set_mode (mode,
                                LIGMA_LAYER_MODE_NORMAL,
                                LIGMA_LAYER_COLOR_SPACE_AUTO,
                                composite_space,
                                LIGMA_LAYER_COMPOSITE_AUTO);

  gegl_node_connect_to (input,  "output",
                        mode,   "aux");
  gegl_node_connect_to (color,  "output",
                        mode,   "input");
  gegl_node_connect_to (mode,   "output",
                        output, "input");

  return node;
}

GeglNode *
ligma_gegl_create_apply_opacity_node (GeglBuffer *mask,
                                     gint        mask_offset_x,
                                     gint        mask_offset_y,
                                     gdouble     opacity)
{
  GeglNode  *node;
  GeglNode  *input;
  GeglNode  *output;
  GeglNode  *opacity_node;
  GeglNode  *mask_source;

  g_return_val_if_fail (GEGL_IS_BUFFER (mask), NULL);

  node = gegl_node_new ();

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  opacity_node = gegl_node_new_child (node,
                                      "operation", "gegl:opacity",
                                      "value",     opacity,
                                      NULL);

  ligma_gegl_node_set_underlying_operation (node, opacity_node);

  mask_source = ligma_gegl_add_buffer_source (node, mask,
                                             mask_offset_x,
                                             mask_offset_y);

  gegl_node_connect_to (input,        "output",
                        opacity_node, "input");
  gegl_node_connect_to (mask_source,  "output",
                        opacity_node, "aux");
  gegl_node_connect_to (opacity_node, "output",
                        output,       "input");

  return node;
}

GeglNode *
ligma_gegl_create_transform_node (const LigmaMatrix3 *matrix)
{
  GeglNode *node;

  g_return_val_if_fail (matrix != NULL, NULL);

  node = gegl_node_new_child (NULL,
                              "operation", "gegl:transform",
                              NULL);

  ligma_gegl_node_set_matrix (node, matrix);

  return node;
}

GeglNode *
ligma_gegl_add_buffer_source (GeglNode   *parent,
                             GeglBuffer *buffer,
                             gint        offset_x,
                             gint        offset_y)
{
  GeglNode *buffer_source;

  g_return_val_if_fail (GEGL_IS_NODE (parent), NULL);
  g_return_val_if_fail (GEGL_IS_BUFFER (buffer), NULL);

  buffer_source = gegl_node_new_child (parent,
                                       "operation", "gegl:buffer-source",
                                       "buffer",    buffer,
                                       NULL);

  if (offset_x != 0 || offset_y != 0)
    {
      GeglNode *translate =
        gegl_node_new_child (parent,
                             "operation", "gegl:translate",
                             "x",         (gdouble) offset_x,
                             "y",         (gdouble) offset_y,
                             NULL);

      gegl_node_connect_to (buffer_source, "output",
                            translate,     "input");

      buffer_source = translate;
    }

  return buffer_source;
}

void
ligma_gegl_mode_node_set_mode (GeglNode               *node,
                              LigmaLayerMode           mode,
                              LigmaLayerColorSpace     blend_space,
                              LigmaLayerColorSpace     composite_space,
                              LigmaLayerCompositeMode  composite_mode)
{
  gdouble opacity;

  g_return_if_fail (GEGL_IS_NODE (node));

  if (blend_space == LIGMA_LAYER_COLOR_SPACE_AUTO)
    blend_space = ligma_layer_mode_get_blend_space (mode);

  if (composite_space == LIGMA_LAYER_COLOR_SPACE_AUTO)
    composite_space = ligma_layer_mode_get_composite_space (mode);

  if (composite_mode == LIGMA_LAYER_COMPOSITE_AUTO)
    composite_mode = ligma_layer_mode_get_composite_mode (mode);

  gegl_node_get (node,
                 "opacity", &opacity,
                 NULL);

  /* setting the operation creates a new instance, so we have to set
   * all its properties
   */
  gegl_node_set (node,
                 "operation",       ligma_layer_mode_get_operation_name (mode),
                 "layer-mode",      mode,
                 "opacity",         opacity,
                 "blend-space",     blend_space,
                 "composite-space", composite_space,
                 "composite-mode",  composite_mode,
                 NULL);
}

void
ligma_gegl_mode_node_set_opacity (GeglNode *node,
                                 gdouble   opacity)
{
  g_return_if_fail (GEGL_IS_NODE (node));

  gegl_node_set (node,
                 "opacity", opacity,
                 NULL);
}

void
ligma_gegl_node_set_matrix (GeglNode          *node,
                           const LigmaMatrix3 *matrix)
{
  gchar *matrix_string;

  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (matrix != NULL);

  matrix_string = gegl_matrix3_to_string ((GeglMatrix3 *) matrix);

  gegl_node_set (node,
                 "transform", matrix_string,
                 NULL);

  g_free (matrix_string);
}

void
ligma_gegl_node_set_color (GeglNode      *node,
                          const LigmaRGB *color,
                          const Babl    *space)
{
  GeglColor *gegl_color;

  g_return_if_fail (GEGL_IS_NODE (node));
  g_return_if_fail (color != NULL);

  gegl_color = ligma_gegl_color_new (color, space);

  gegl_node_set (node,
                 "value", gegl_color,
                 NULL);

  g_object_unref (gegl_color);
}
