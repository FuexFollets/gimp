/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationbuffersourcevalidate.c
 * Copyright (C) 2017 Ell
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
#include <gegl-plugin.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "operations-types.h"

#include "gegl/ligmatilehandlervalidate.h"

#include "ligmaoperationbuffersourcevalidate.h"


enum
{
  PROP_0,
  PROP_BUFFER
};


static void           ligma_operation_buffer_source_validate_dispose          (GObject                           *object);
static void           ligma_operation_buffer_source_validate_get_property     (GObject                           *object,
                                                                              guint                              property_id,
                                                                              GValue                            *value,
                                                                              GParamSpec                        *pspec);
static void           ligma_operation_buffer_source_validate_set_property     (GObject                           *object,
                                                                              guint                              property_id,
                                                                              const GValue                      *value,
                                                                              GParamSpec                        *pspec);

static GeglRectangle  ligma_operation_buffer_source_validate_get_bounding_box (GeglOperation                     *operation);
static void           ligma_operation_buffer_source_validate_prepare          (GeglOperation                     *operation);
static gboolean       ligma_operation_buffer_source_validate_process          (GeglOperation                     *operation,
                                                                              GeglOperationContext              *context,
                                                                              const gchar                       *output_pad,
                                                                              const GeglRectangle               *result,
                                                                              gint                               level);

static void           ligma_operation_buffer_source_validate_invalidate       (gpointer                           object,
                                                                              const GeglRectangle               *rect,
                                                                              LigmaOperationBufferSourceValidate *buffer_source_validate);


G_DEFINE_TYPE (LigmaOperationBufferSourceValidate, ligma_operation_buffer_source_validate,
               GEGL_TYPE_OPERATION_SOURCE)

#define parent_class ligma_operation_buffer_source_validate_parent_class


static void
ligma_operation_buffer_source_validate_class_init (LigmaOperationBufferSourceValidateClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->dispose             = ligma_operation_buffer_source_validate_dispose;
  object_class->set_property        = ligma_operation_buffer_source_validate_set_property;
  object_class->get_property        = ligma_operation_buffer_source_validate_get_property;

  operation_class->get_bounding_box = ligma_operation_buffer_source_validate_get_bounding_box;
  operation_class->prepare          = ligma_operation_buffer_source_validate_prepare;
  operation_class->process          = ligma_operation_buffer_source_validate_process;

  operation_class->threaded         = FALSE;
  operation_class->cache_policy     = GEGL_CACHE_POLICY_NEVER;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:buffer-source-validate",
                                 "categories",  "ligma",
                                 "description", "LIGMA Buffer-Source Validate operation",
                                 NULL);

  g_object_class_install_property (object_class, PROP_BUFFER,
                                   g_param_spec_object ("buffer",
                                                        "Buffer",
                                                        "Input buffer",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READWRITE));
}

static void
ligma_operation_buffer_source_validate_init (LigmaOperationBufferSourceValidate *self)
{
}

static void
ligma_operation_buffer_source_validate_dispose (GObject *object)
{
  LigmaOperationBufferSourceValidate *buffer_source_validate = LIGMA_OPERATION_BUFFER_SOURCE_VALIDATE (object);

  if (buffer_source_validate->buffer)
    {
      LigmaTileHandlerValidate *validate_handler;

      validate_handler = ligma_tile_handler_validate_get_assigned (
        buffer_source_validate->buffer);

      if (validate_handler)
        {
          g_signal_connect (
            validate_handler,
            "invalidated",
            G_CALLBACK (ligma_operation_buffer_source_validate_invalidate),
            buffer_source_validate);
        }

      g_signal_handlers_disconnect_by_func (
        buffer_source_validate->buffer,
        ligma_operation_buffer_source_validate_invalidate,
        buffer_source_validate);

      g_clear_object (&buffer_source_validate->buffer);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_operation_buffer_source_validate_get_property (GObject    *object,
                                                    guint       property_id,
                                                    GValue     *value,
                                                    GParamSpec *pspec)
{
  LigmaOperationBufferSourceValidate *buffer_source_validate = LIGMA_OPERATION_BUFFER_SOURCE_VALIDATE (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      g_value_set_object (value, buffer_source_validate->buffer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_operation_buffer_source_validate_set_property (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec)
{
  LigmaOperationBufferSourceValidate *buffer_source_validate = LIGMA_OPERATION_BUFFER_SOURCE_VALIDATE (object);

  switch (property_id)
    {
    case PROP_BUFFER:
      {
        if (buffer_source_validate->buffer)
          {
            LigmaTileHandlerValidate *validate_handler;

            validate_handler = ligma_tile_handler_validate_get_assigned (
              buffer_source_validate->buffer);

            ligma_operation_buffer_source_validate_invalidate (
              buffer_source_validate->buffer,
              gegl_buffer_get_extent (buffer_source_validate->buffer),
              buffer_source_validate);

            g_signal_handlers_disconnect_by_func (
              buffer_source_validate->buffer,
              ligma_operation_buffer_source_validate_invalidate,
              buffer_source_validate);

            if (validate_handler)
              {
                g_signal_handlers_disconnect_by_func (
                  validate_handler,
                  ligma_operation_buffer_source_validate_invalidate,
                  buffer_source_validate);
              }

            g_clear_object (&buffer_source_validate->buffer);
          }

        buffer_source_validate->buffer = g_value_dup_object (value);

        if (buffer_source_validate->buffer)
          {
            LigmaTileHandlerValidate *validate_handler;

            validate_handler = ligma_tile_handler_validate_get_assigned (
              buffer_source_validate->buffer);

            if (validate_handler)
              {
                g_signal_connect (
                  validate_handler,
                  "invalidated",
                  G_CALLBACK (ligma_operation_buffer_source_validate_invalidate),
                  buffer_source_validate);
              }

            gegl_buffer_signal_connect (
              buffer_source_validate->buffer,
              "changed",
              G_CALLBACK (ligma_operation_buffer_source_validate_invalidate),
              buffer_source_validate);

            ligma_operation_buffer_source_validate_invalidate (
              buffer_source_validate->buffer,
              gegl_buffer_get_extent (buffer_source_validate->buffer),
              buffer_source_validate);
          }
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GeglRectangle
ligma_operation_buffer_source_validate_get_bounding_box (GeglOperation *operation)
{
  LigmaOperationBufferSourceValidate *buffer_source_validate = LIGMA_OPERATION_BUFFER_SOURCE_VALIDATE (operation);

  GeglRectangle result = {};

  if (buffer_source_validate->buffer)
    result = *gegl_buffer_get_extent (buffer_source_validate->buffer);

  return result;
}

static void
ligma_operation_buffer_source_validate_prepare (GeglOperation *operation)
{
  LigmaOperationBufferSourceValidate *buffer_source_validate = LIGMA_OPERATION_BUFFER_SOURCE_VALIDATE (operation);
  const Babl                        *format                 = NULL;

  if (buffer_source_validate->buffer)
    format = gegl_buffer_get_format (buffer_source_validate->buffer);

  gegl_operation_set_format (operation, "output", format);
}

static gboolean
ligma_operation_buffer_source_validate_process (GeglOperation        *operation,
                                               GeglOperationContext *context,
                                               const gchar          *output_pad,
                                               const GeglRectangle  *result,
                                               gint                  level)
{
  LigmaOperationBufferSourceValidate *buffer_source_validate = LIGMA_OPERATION_BUFFER_SOURCE_VALIDATE (operation);
  GeglBuffer                        *buffer                 = buffer_source_validate->buffer;

  if (buffer)
    {
      LigmaTileHandlerValidate *validate_handler;

      validate_handler = ligma_tile_handler_validate_get_assigned (buffer);

      if (validate_handler)
        {
          GeglRectangle rect;

          /* align the rectangle to the tile grid */
          gegl_rectangle_align_to_buffer (
            &rect, result, buffer_source_validate->buffer,
            GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

          ligma_tile_handler_validate_validate (validate_handler,
                                               buffer_source_validate->buffer,
                                               &rect,
                                               TRUE, FALSE);
        }

      gegl_operation_context_set_object (context, "output", G_OBJECT (buffer));

      gegl_object_set_has_forked (G_OBJECT (buffer));
    }

  return TRUE;
}

static void
ligma_operation_buffer_source_validate_invalidate (gpointer                           object,
                                                  const GeglRectangle               *rect,
                                                  LigmaOperationBufferSourceValidate *buffer_source_validate)
{
  gegl_operation_invalidate (GEGL_OPERATION (buffer_source_validate),
                             rect, FALSE);
}
