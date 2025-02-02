/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * ligmaimage.c
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

#include "config.h"

#include "ligma.h"

#include "libligmabase/ligmawire.h" /* FIXME kill this include */

#include "ligmapixbuf.h"
#include "ligmaplugin-private.h"
#include "ligmaprocedure-private.h"


enum
{
  PROP_0,
  PROP_ID,
  N_PROPS
};

struct _LigmaImage
{
  GObject parent_instance;
  gint    id;
};


static void   ligma_image_set_property  (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec);
static void   ligma_image_get_property  (GObject      *object,
                                        guint         property_id,
                                        GValue       *value,
                                        GParamSpec   *pspec);


G_DEFINE_TYPE (LigmaImage, ligma_image, G_TYPE_OBJECT)

#define parent_class ligma_image_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
ligma_image_class_init (LigmaImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = ligma_image_set_property;
  object_class->get_property = ligma_image_get_property;

  props[PROP_ID] =
    g_param_spec_int ("id",
                      "The image id",
                      "The image id for internal use",
                      0, G_MAXINT32, 0,
                      LIGMA_PARAM_READWRITE |
                      G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
ligma_image_init (LigmaImage *image)
{
}

static void
ligma_image_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  LigmaImage *image = LIGMA_IMAGE (object);

  switch (property_id)
    {
    case PROP_ID:
      image->id = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_image_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  LigmaImage *image = LIGMA_IMAGE (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_int (value, image->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/* Public API */

/**
 * ligma_image_get_id:
 * @image: The image.
 *
 * Returns: the image ID.
 *
 * Since: 3.0
 **/
gint32
ligma_image_get_id (LigmaImage *image)
{
  return image ? image->id : -1;
}

/**
 * ligma_image_get_by_id:
 * @image_id: The image id.
 *
 * Returns: (nullable) (transfer none): a #LigmaImage for @image_id or
 *          %NULL if @image_id does not represent a valid image.
 *          The object belongs to libligma and you must not modify
 *          or unref it.
 *
 * Since: 3.0
 **/
LigmaImage *
ligma_image_get_by_id (gint32 image_id)
{
  if (image_id > 0)
    {
      LigmaPlugIn    *plug_in   = ligma_get_plug_in ();
      LigmaProcedure *procedure = _ligma_plug_in_get_procedure (plug_in);

      return _ligma_procedure_get_image (procedure, image_id);
    }

  return NULL;
}

/**
 * ligma_image_is_valid:
 * @image: The image to check.
 *
 * Returns TRUE if the image is valid.
 *
 * This procedure checks if the given image is valid and refers to
 * an existing image.
 *
 * Returns: Whether the image is valid.
 *
 * Since: 2.4
 **/
gboolean
ligma_image_is_valid (LigmaImage *image)
{
  return ligma_image_id_is_valid (ligma_image_get_id (image));
}

/**
 * ligma_list_images:
 *
 * Returns the list of images currently open.
 *
 * This procedure returns the list of images currently open in LIGMA.
 *
 * Returns: (element-type LigmaImage) (transfer container):
 *          The list of images currently open.
 *          The returned list must be freed with g_list_free(). Image
 *          elements belong to libligma and must not be freed.
 *
 * Since: 3.0
 **/
GList *
ligma_list_images (void)
{
  LigmaImage **images;
  gint        num_images;
  GList      *list = NULL;
  gint        i;

  images = ligma_get_images (&num_images);

  for (i = 0; i < num_images; i++)
    list = g_list_prepend (list, images[i]);

  g_free (images);

  return g_list_reverse (list);
}

/**
 * ligma_image_list_layers:
 * @image: The image.
 *
 * Returns the list of layers contained in the specified image.
 *
 * This procedure returns the list of layers contained in the specified
 * image. The order of layers is from topmost to bottommost.
 *
 * Returns: (element-type LigmaLayer) (transfer container):
 *          The list of layers contained in the image.
 *          The returned list must be freed with g_list_free(). Layer
 *          elements belong to libligma and must not be freed.
 *
 * Since: 3.0
 **/
GList *
ligma_image_list_layers (LigmaImage *image)
{
  LigmaLayer **layers;
  gint        num_layers;
  GList      *list = NULL;
  gint        i;

  layers = ligma_image_get_layers (image, &num_layers);

  for (i = 0; i < num_layers; i++)
    list = g_list_prepend (list, layers[i]);

  g_free (layers);

  return g_list_reverse (list);
}

/**
 * ligma_image_list_selected_layers:
 * @image: The image.
 *
 * Returns the list of layers selected in the specified image.
 *
 * This procedure returns the list of layers selected in the specified
 * image.
 *
 * Returns: (element-type LigmaLayer) (transfer container):
 *          The list of selected layers in the image.
 *          The returned list must be freed with g_list_free(). Layer
 *          elements belong to libligma and must not be freed.
 *
 * Since: 3.0
 **/
GList *
ligma_image_list_selected_layers (LigmaImage *image)
{
  LigmaLayer **layers;
  gint        num_layers;
  GList      *list = NULL;
  gint        i;

  layers = ligma_image_get_selected_layers (image, &num_layers);

  for (i = 0; i < num_layers; i++)
    list = g_list_prepend (list, layers[i]);

  g_free (layers);

  return g_list_reverse (list);
}

/**
 * ligma_image_take_selected_layers:
 * @image: The image.
 * @layers: (transfer container) (element-type LigmaLayer): The list of layers to select.
 *
 * The layers are set as the selected layers in the image. Any previous
 * selected layers or channels are unselected. An exception is a previously
 * existing floating selection, in which case this procedure will return an
 * execution error.
 *
 * Returns: TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
ligma_image_take_selected_layers (LigmaImage *image,
                                 GList     *layers)
{
  LigmaLayer **sel_layers;
  GList      *list;
  gboolean    success;
  gint        i;

  sel_layers = g_new0 (LigmaLayer *, g_list_length (layers));
  for (list = layers, i = 0; list; list = list->next, i++)
    sel_layers[i] = list->data;

  success = ligma_image_set_selected_layers (image, g_list_length (layers),
                                            (const LigmaLayer **) sel_layers);
  g_list_free (layers);

  return success;
}

/**
 * ligma_image_list_selected_channels:
 * @image: The image.
 *
 * Returns the list of channels selected in the specified image.
 *
 * This procedure returns the list of channels selected in the specified
 * image.
 *
 * Returns: (element-type LigmaChannel) (transfer container):
 *          The list of selected channels in the image.
 *          The returned list must be freed with g_list_free(). Layer
 *          elements belong to libligma and must not be freed.
 *
 * Since: 3.0
 **/
GList *
ligma_image_list_selected_channels (LigmaImage *image)
{
  LigmaChannel **channels;
  gint          num_channels;
  GList        *list = NULL;
  gint          i;

  channels = ligma_image_get_selected_channels (image, &num_channels);

  for (i = 0; i < num_channels; i++)
    list = g_list_prepend (list, channels[i]);

  g_free (channels);

  return g_list_reverse (list);
}

/**
 * ligma_image_take_selected_channels:
 * @image: The image.
 * @channels: (transfer container) (element-type LigmaChannel): The list of channels to select.
 *
 * The channels are set as the selected channels in the image. Any previous
 * selected layers or channels are unselected. An exception is a previously
 * existing floating selection, in which case this procedure will return an
 * execution error.
 *
 * Returns: TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
ligma_image_take_selected_channels (LigmaImage *image,
                                   GList     *channels)
{
  LigmaChannel **sel_channels;
  GList        *list;
  gboolean      success;
  gint          i;

  sel_channels = g_new0 (LigmaChannel *, g_list_length (channels));
  for (list = channels, i = 0; list; list = list->next, i++)
    sel_channels[i] = list->data;

  success = ligma_image_set_selected_channels (image, g_list_length (channels),
                                              (const LigmaChannel **) sel_channels);
  g_list_free (channels);

  return success;
}

/**
 * ligma_image_list_selected_vectors:
 * @image: The image.
 *
 * Returns the list of paths selected in the specified image.
 *
 * This procedure returns the list of paths selected in the specified
 * image.
 *
 * Returns: (element-type LigmaVectors) (transfer container):
 *          The list of selected paths in the image.
 *          The returned list must be freed with g_list_free().
 *          Path elements belong to libligma and must not be freed.
 *
 * Since: 3.0
 **/
GList *
ligma_image_list_selected_vectors (LigmaImage *image)
{
  LigmaVectors **vectors;
  gint          num_vectors;
  GList        *list = NULL;
  gint          i;

  vectors = ligma_image_get_selected_vectors (image, &num_vectors);

  for (i = 0; i < num_vectors; i++)
    list = g_list_prepend (list, vectors[i]);

  g_free (vectors);

  return g_list_reverse (list);
}

/**
 * ligma_image_take_selected_vectors:
 * @image: The image.
 * @vectors: (transfer container) (element-type LigmaVectors): The list of paths to select.
 *
 * The paths are set as the selected paths in the image. Any previous
 * selected paths are unselected.
 *
 * Returns: TRUE on success.
 *
 * Since: 3.0
 **/
gboolean
ligma_image_take_selected_vectors (LigmaImage *image,
                                  GList     *vectors)
{
  LigmaVectors **sel_vectors;
  GList        *list;
  gboolean      success;
  gint          i;

  sel_vectors = g_new0 (LigmaVectors *, g_list_length (vectors));
  for (list = vectors, i = 0; list; list = list->next, i++)
    sel_vectors[i] = list->data;

  success = ligma_image_set_selected_vectors (image, g_list_length (vectors),
                                             (const LigmaVectors **) sel_vectors);
  g_list_free (vectors);

  return success;
}

/**
 * ligma_image_list_channels:
 * @image: The image.
 *
 * Returns the list of channels contained in the specified image.
 *
 * This procedure returns the list of channels contained in the
 * specified image. This does not include the selection mask, or layer
 * masks. The order is from topmost to bottommost. Note that
 * "channels" are custom channels and do not include the image's
 * color components.
 *
 * Returns: (element-type LigmaChannel) (transfer container):
 *          The list of channels contained in the image.
 *          The returned list must be freed with g_list_free(). Channel
 *          elements belong to libligma and must not be freed.
 *
 * Since: 3.0
 **/
GList *
ligma_image_list_channels (LigmaImage *image)
{
  LigmaChannel **channels;
  gint          num_channels;
  GList        *list = NULL;
  gint          i;

  channels = ligma_image_get_channels (image, &num_channels);

  for (i = 0; i < num_channels; i++)
    list = g_list_prepend (list, channels[i]);

  g_free (channels);

  return g_list_reverse (list);
}

/**
 * ligma_image_list_vectors:
 * @image: The image.
 *
 * Returns the list of vectors contained in the specified image.
 *
 * This procedure returns the list of vectors contained in the
 * specified image.
 *
 * Returns: (element-type LigmaVectors) (transfer container):
 *          The list of vectors contained in the image.
 *          The returned value must be freed with g_list_free(). Vectors
 *          elements belong to libligma and must not be freed.
 *
 * Since: 3.0
 **/
GList *
ligma_image_list_vectors (LigmaImage *image)
{
  LigmaVectors **vectors;
  gint          num_vectors;
  GList        *list = NULL;
  gint          i;

  vectors = ligma_image_get_vectors (image, &num_vectors);

  for (i = 0; i < num_vectors; i++)
    list = g_list_prepend (list, vectors[i]);

  g_free (vectors);

  return g_list_reverse (list);
}

/**
 * ligma_image_list_selected_drawables:
 * @image: The image.
 *
 * Returns the list of drawables selected in the specified image.
 *
 * This procedure returns the list of drawables selected in the specified
 * image.
 * These can be either a list of layers or a list of channels (a list mixing
 * layers and channels is not possible), or it can be a layer mask (a list
 * containing only a layer mask as single item), if a layer mask is in edit
 * mode.
 *
 * Returns: (element-type LigmaItem) (transfer container):
 *          The list of selected drawables in the image.
 *          The returned list must be freed with g_list_free(). Layer
 *          elements belong to libligma and must not be freed.
 *
 * Since: 3.0
 **/
GList *
ligma_image_list_selected_drawables (LigmaImage *image)
{
  LigmaItem **drawables;
  gint       num_drawables;
  GList     *list = NULL;
  gint       i;

  drawables = ligma_image_get_selected_drawables (image, &num_drawables);

  for (i = 0; i < num_drawables; i++)
    list = g_list_prepend (list, drawables[i]);

  g_free (drawables);

  return g_list_reverse (list);
}

/**
 * ligma_image_get_colormap:
 * @image:      The image.
 * @num_colors: (out): Returns the number of colors in the colormap array.
 *
 * Returns the image's colormap
 *
 * This procedure returns an actual pointer to the image's colormap, as
 * well as the number of colors contained in the colormap. If the image
 * is not of base type INDEXED, this pointer will be NULL.
 *
 * Returns: (array): The image's colormap.
 */
guchar *
ligma_image_get_colormap (LigmaImage *image,
                         gint      *num_colors)
{
  gint    num_bytes;
  guchar *cmap;

  cmap = _ligma_image_get_colormap (image, &num_bytes);

  if (num_colors)
    *num_colors = num_bytes / 3;

  return cmap;
}

/**
 * ligma_image_set_colormap:
 * @image:      The image.
 * @colormap: (array): The new colormap values.
 * @num_colors: Number of colors in the colormap array.
 *
 * Sets the entries in the image's colormap.
 *
 * This procedure sets the entries in the specified image's colormap.
 * The number of colors is specified by the "num_colors" parameter
 * and corresponds to the number of INT8 triples that must be contained
 * in the "cmap" array.
 *
 * Returns: TRUE on success.
 */
gboolean
ligma_image_set_colormap (LigmaImage    *image,
                         const guchar *colormap,
                         gint          num_colors)
{
  return _ligma_image_set_colormap (image, num_colors * 3, colormap);
}

/**
 * ligma_image_get_thumbnail_data:
 * @image:  The image.
 * @width:  (inout): The requested thumbnail width.
 * @height: (inout): The requested thumbnail height.
 * @bpp:    (out): The previews bpp.
 *
 * Get a thumbnail of an image.
 *
 * This function gets data from which a thumbnail of an image preview
 * can be created. Maximum x or y dimension is 1024 pixels. The pixels
 * are returned in RGB[A] or GRAY[A] format. The bpp return value
 * gives the number of bytes per pixel in the image.
 *
 * Returns: (array) (transfer full): the thumbnail data.
 **/
guchar *
ligma_image_get_thumbnail_data (LigmaImage *image,
                               gint      *width,
                               gint      *height,
                               gint      *bpp)
{
  gint    ret_width;
  gint    ret_height;
  guchar *image_data;
  gint    data_size;

  _ligma_image_thumbnail (image,
                         *width,
                         *height,
                         &ret_width,
                         &ret_height,
                         bpp,
                         &data_size,
                         &image_data);

  *width  = ret_width;
  *height = ret_height;

  return image_data;
}

/**
 * ligma_image_get_thumbnail:
 * @image:  the #LigmaImage
 * @width:  the requested thumbnail width  (<= 1024 pixels)
 * @height: the requested thumbnail height (<= 1024 pixels)
 * @alpha:  how to handle an alpha channel
 *
 * Retrieves a thumbnail pixbuf for @image.
 * The thumbnail will be not larger than the requested size.
 *
 * Returns: (transfer full): a new #GdkPixbuf
 *
 * Since: 2.2
 **/
GdkPixbuf *
ligma_image_get_thumbnail (LigmaImage              *image,
                          gint                    width,
                          gint                    height,
                          LigmaPixbufTransparency  alpha)
{
  gint    thumb_width  = width;
  gint    thumb_height = height;
  gint    thumb_bpp;
  guchar *data;

  g_return_val_if_fail (width  > 0 && width  <= 1024, NULL);
  g_return_val_if_fail (height > 0 && height <= 1024, NULL);

  data = ligma_image_get_thumbnail_data (image,
                                        &thumb_width,
                                        &thumb_height,
                                        &thumb_bpp);
  if (data)
    return _ligma_pixbuf_from_data (data,
                                   thumb_width, thumb_height, thumb_bpp,
                                   alpha);
  else
    return NULL;
}

/**
 * ligma_image_get_metadata:
 * @image: The image.
 *
 * Returns the image's metadata.
 *
 * Returns exif/iptc/xmp metadata from the image.
 *
 * Returns: (nullable) (transfer full): The exif/ptc/xmp metadata,
 *          or %NULL if there is none.
 *
 * Since: 2.10
 **/
LigmaMetadata *
ligma_image_get_metadata (LigmaImage *image)
{
  LigmaMetadata *metadata = NULL;
  gchar        *metadata_string;

  metadata_string = _ligma_image_get_metadata (image);
  if (metadata_string)
    {
      metadata = ligma_metadata_deserialize (metadata_string);
      g_free (metadata_string);
    }

  return metadata;
}

/**
 * ligma_image_set_metadata:
 * @image:    The image.
 * @metadata: The exif/ptc/xmp metadata.
 *
 * Set the image's metadata.
 *
 * Sets exif/iptc/xmp metadata on the image, or deletes it if
 * @metadata is %NULL.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_image_set_metadata (LigmaImage    *image,
                         LigmaMetadata *metadata)
{
  gchar    *metadata_string = NULL;
  gboolean  success;

  if (metadata)
    metadata_string = ligma_metadata_serialize (metadata);

  success = _ligma_image_set_metadata (image, metadata_string);

  if (metadata_string)
    g_free (metadata_string);

  return success;
}
