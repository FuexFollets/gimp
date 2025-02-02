/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmachannel.c
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "ligma.h"


G_DEFINE_TYPE (LigmaChannel, ligma_channel, LIGMA_TYPE_DRAWABLE)

#define parent_class ligma_drawable_parent_class


static void
ligma_channel_class_init (LigmaChannelClass *klass)
{
}

static void
ligma_channel_init (LigmaChannel *channel)
{
}

/**
 * ligma_channel_get_by_id:
 * @channel_id: The channel id.
 *
 * Returns a #LigmaChannel representing @channel_id. This function
 * calls ligma_item_get_by_id() and returns the item if it is channel
 * or %NULL otherwise.
 *
 * Returns: (nullable) (transfer none): a #LigmaChannel for @channel_id
 *          or %NULL if @channel_id does not represent a valid
 *          channel. The object belongs to libligma and you must not
 *          modify or unref it.
 *
 * Since: 3.0
 **/
LigmaChannel *
ligma_channel_get_by_id (gint32 channel_id)
{
  LigmaItem *item = ligma_item_get_by_id (channel_id);

  if (LIGMA_IS_CHANNEL (item))
    return (LigmaChannel *) item;

  return NULL;
}

/**
 * ligma_channel_new:
 * @image:   The image to which to add the channel.
 * @name:    The channel name.
 * @width:   The channel width.
 * @height:  The channel height.
 * @opacity: The channel opacity.
 * @color:   The channel compositing color.
 *
 * Create a new channel.
 *
 * This procedure creates a new channel with the specified width and
 * height. Name, opacity, and color are also supplied parameters. The
 * new channel still needs to be added to the image, as this is not
 * automatic. Add the new channel with the ligma_image_insert_channel()
 * command. Other attributes such as channel show masked, should be
 * set with explicit procedure calls. The channel's contents are
 * undefined initially.
 *
 * Returns: (transfer none): The newly created channel.
 *          The object belongs to libligma and you should not free it.
 */
LigmaChannel *
ligma_channel_new (LigmaImage     *image,
                  const gchar   *name,
                  guint          width,
                  guint          height,
                  gdouble        opacity,
                  const LigmaRGB *color)
{
  return _ligma_channel_new (image,
                            width,
                            height,
                            name,
                            opacity,
                            color);
}
