/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmacolor/ligmacolor.h"

#include "ligmawidgetstypes.h"

#include "ligmapreviewarea.h"
#include "ligmawidgetsutils.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmapreviewarea
 * @title: LigmaPreviewArea
 * @short_description: A general purpose preview widget which caches
 *                     its pixel data.
 *
 * A general purpose preview widget which caches its pixel data.
 **/


enum
{
  PROP_0,
  PROP_CHECK_SIZE,
  PROP_CHECK_TYPE,
  PROP_CHECK_CUSTOM_COLOR1,
  PROP_CHECK_CUSTOM_COLOR2
};


#define DEFAULT_CHECK_SIZE  LIGMA_CHECK_SIZE_MEDIUM_CHECKS
#define DEFAULT_CHECK_TYPE  LIGMA_CHECK_TYPE_GRAY_CHECKS

#define CHECK_R(priv, row, col)        \
  (((((priv)->offset_y + (row)) & size) ^  \
    (((priv)->offset_x + (col)) & size)) ? r1 : r2)

#define CHECK_G(priv, row, col)        \
  (((((priv)->offset_y + (row)) & size) ^  \
    (((priv)->offset_x + (col)) & size)) ? g1 : g2)

#define CHECK_B(priv, row, col)        \
  (((((priv)->offset_y + (row)) & size) ^  \
    (((priv)->offset_x + (col)) & size)) ? b1 : b2)


struct _LigmaPreviewAreaPrivate
{
  LigmaCheckSize       check_size;
  LigmaCheckType       check_type;
  LigmaRGB             check_custom_color1;
  LigmaRGB             check_custom_color2;
  gint                width;
  gint                height;
  gint                rowstride;
  gint                offset_x;
  gint                offset_y;
  gint                max_width;
  gint                max_height;
  guchar             *buf;
  guchar             *colormap;

  LigmaColorConfig    *config;
  LigmaColorTransform *transform;
};

#define GET_PRIVATE(obj) (((LigmaPreviewArea *) (obj))->priv)


static void      ligma_preview_area_dispose           (GObject          *object);
static void      ligma_preview_area_finalize          (GObject          *object);
static void      ligma_preview_area_set_property      (GObject          *object,
                                                      guint             property_id,
                                                      const GValue     *value,
                                                      GParamSpec       *pspec);
static void      ligma_preview_area_get_property      (GObject          *object,
                                                      guint             property_id,
                                                      GValue           *value,
                                                      GParamSpec       *pspec);

static void      ligma_preview_area_size_allocate     (GtkWidget        *widget,
                                                      GtkAllocation    *allocation);
static gboolean  ligma_preview_area_widget_draw       (GtkWidget        *widget,
                                                      cairo_t          *cr);

static void      ligma_preview_area_queue_draw        (LigmaPreviewArea  *area,
                                                      gint              x,
                                                      gint              y,
                                                      gint              width,
                                                      gint              height);
static gint      ligma_preview_area_image_type_bytes  (LigmaImageType     type);

static void      ligma_preview_area_create_transform  (LigmaPreviewArea  *area);
static void      ligma_preview_area_destroy_transform (LigmaPreviewArea  *area);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaPreviewArea, ligma_preview_area,
                            GTK_TYPE_DRAWING_AREA)

#define parent_class ligma_preview_area_parent_class


static void
ligma_preview_area_class_init (LigmaPreviewAreaClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose       = ligma_preview_area_dispose;
  object_class->finalize      = ligma_preview_area_finalize;
  object_class->set_property  = ligma_preview_area_set_property;
  object_class->get_property  = ligma_preview_area_get_property;

  widget_class->size_allocate = ligma_preview_area_size_allocate;
  widget_class->draw          = ligma_preview_area_widget_draw;

  g_object_class_install_property (object_class, PROP_CHECK_SIZE,
                                   g_param_spec_enum ("check-size",
                                                      _("Check Size"),
                                                      "The size of the checkerboard pattern indicating transparency",
                                                      LIGMA_TYPE_CHECK_SIZE,
                                                      DEFAULT_CHECK_SIZE,
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CHECK_TYPE,
                                   g_param_spec_enum ("check-type",
                                                      _("Check Style"),
                                                      "The colors of the checkerboard pattern indicating transparency",
                                                      LIGMA_TYPE_CHECK_TYPE,
                                                      DEFAULT_CHECK_TYPE,
                                                      LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CHECK_CUSTOM_COLOR1,
                                   g_param_spec_boxed ("check-custom-color1",
                                                       _("Custom Checks Color 1"),
                                                       "The first color of the checkerboard pattern indicating transparency",
                                                       LIGMA_TYPE_RGB,
                                                       LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_CHECK_CUSTOM_COLOR2,
                                   g_param_spec_boxed ("check-custom-color2",
                                                       _("Custom Checks Color 2"),
                                                       "The second color of the checkerboard pattern indicating transparency",
                                                       LIGMA_TYPE_RGB,
                                                       LIGMA_PARAM_READWRITE));
}

static void
ligma_preview_area_init (LigmaPreviewArea *area)
{
  LigmaPreviewAreaPrivate *priv;

  area->priv = ligma_preview_area_get_instance_private (area);

  priv = area->priv;

  priv->check_size          = DEFAULT_CHECK_SIZE;
  priv->check_type          = DEFAULT_CHECK_TYPE;
  priv->check_custom_color1 = LIGMA_CHECKS_CUSTOM_COLOR1;
  priv->check_custom_color2 = LIGMA_CHECKS_CUSTOM_COLOR2;
  priv->max_width           = -1;
  priv->max_height          = -1;

  ligma_widget_track_monitor (GTK_WIDGET (area),
                             G_CALLBACK (ligma_preview_area_destroy_transform),
                             NULL, NULL);
}

static void
ligma_preview_area_dispose (GObject *object)
{
  LigmaPreviewArea *area = LIGMA_PREVIEW_AREA (object);

  ligma_preview_area_set_color_config (area, NULL);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_preview_area_finalize (GObject *object)
{
  LigmaPreviewAreaPrivate *priv = GET_PRIVATE (object);

  g_clear_pointer (&priv->buf,      g_free);
  g_clear_pointer (&priv->colormap, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_preview_area_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaPreviewAreaPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CHECK_SIZE:
      priv->check_size = g_value_get_enum (value);
      break;
    case PROP_CHECK_TYPE:
      priv->check_type = g_value_get_enum (value);
      break;
    case PROP_CHECK_CUSTOM_COLOR1:
      priv->check_custom_color1 = *(LigmaRGB *) g_value_get_boxed (value);
      break;
    case PROP_CHECK_CUSTOM_COLOR2:
      priv->check_custom_color2 = *(LigmaRGB *) g_value_get_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_preview_area_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaPreviewAreaPrivate *priv = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_CHECK_SIZE:
      g_value_set_enum (value, priv->check_size);
      break;
    case PROP_CHECK_TYPE:
      g_value_set_enum (value, priv->check_type);
      break;
    case PROP_CHECK_CUSTOM_COLOR1:
      g_value_set_boxed (value, &priv->check_custom_color1);
      break;
    case PROP_CHECK_CUSTOM_COLOR2:
      g_value_set_boxed (value, &priv->check_custom_color2);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_preview_area_size_allocate (GtkWidget     *widget,
                                 GtkAllocation *allocation)
{
  LigmaPreviewAreaPrivate *priv = GET_PRIVATE (widget);
  gint                    width;
  gint                    height;

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  width  = (priv->max_width > 0 ?
            MIN (allocation->width, priv->max_width) : allocation->width);
  height = (priv->max_height > 0 ?
            MIN (allocation->height, priv->max_height) : allocation->height);

  if (width  != priv->width || height != priv->height)
    {
      if (priv->buf)
        {
          g_free (priv->buf);

          priv->buf       = NULL;
          priv->rowstride = 0;
        }

      priv->width  = width;
      priv->height = height;
    }
}

static gboolean
ligma_preview_area_widget_draw (GtkWidget *widget,
                               cairo_t   *cr)
{
  LigmaPreviewArea        *area = LIGMA_PREVIEW_AREA (widget);
  LigmaPreviewAreaPrivate *priv = GET_PRIVATE (area);
  GtkAllocation           allocation;
  GdkPixbuf              *pixbuf;
  GdkRectangle            rect;

  if (! priv->buf)
    return FALSE;

  gtk_widget_get_allocation (widget, &allocation);

  rect.x      = (allocation.width  - priv->width)  / 2;
  rect.y      = (allocation.height - priv->height) / 2;
  rect.width  = priv->width;
  rect.height = priv->height;

  if (! priv->transform)
    ligma_preview_area_create_transform (area);

  if (priv->transform)
    {
      const Babl *format    = babl_format ("R'G'B' u8");
      gint        rowstride = ((priv->width * 3) + 3) & ~3;
      guchar     *buf       = g_new (guchar, rowstride * priv->height);
      guchar     *src       = priv->buf;
      guchar     *dest      = buf;
      gint        i;

      for (i = 0; i < priv->height; i++)
        {
          ligma_color_transform_process_pixels (priv->transform,
                                               format, src,
                                               format, dest,
                                               priv->width);

          src  += priv->rowstride;
          dest += rowstride;
        }

      pixbuf = gdk_pixbuf_new_from_data (buf,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         rect.width,
                                         rect.height,
                                         rowstride,
                                         (GdkPixbufDestroyNotify) g_free, NULL);
    }
  else
    {
      pixbuf = gdk_pixbuf_new_from_data (priv->buf,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         rect.width,
                                         rect.height,
                                         priv->rowstride,
                                         NULL, NULL);
    }

  gdk_cairo_set_source_pixbuf (cr, pixbuf, rect.x, rect.y);
  cairo_paint (cr);

  g_object_unref (pixbuf);

  return FALSE;
}

static void
ligma_preview_area_queue_draw (LigmaPreviewArea *area,
                              gint             x,
                              gint             y,
                              gint             width,
                              gint             height)
{
  LigmaPreviewAreaPrivate *priv   = GET_PRIVATE (area);
  GtkWidget              *widget = GTK_WIDGET (area);
  GtkAllocation           allocation;

  gtk_widget_get_allocation (widget, &allocation);

  x += (allocation.width  - priv->width)  / 2;
  y += (allocation.height - priv->height) / 2;

  gtk_widget_queue_draw_area (widget, x, y, width, height);
}

static gint
ligma_preview_area_image_type_bytes (LigmaImageType type)
{
  switch (type)
    {
    case LIGMA_GRAY_IMAGE:
    case LIGMA_INDEXED_IMAGE:
      return 1;

    case LIGMA_GRAYA_IMAGE:
    case LIGMA_INDEXEDA_IMAGE:
      return 2;

    case LIGMA_RGB_IMAGE:
      return 3;

    case LIGMA_RGBA_IMAGE:
      return 4;

    default:
      g_return_val_if_reached (0);
      break;
    }
}

static void
ligma_preview_area_create_transform (LigmaPreviewArea *area)
{
  LigmaPreviewAreaPrivate *priv = GET_PRIVATE (area);

  if (priv->config)
    {
      static LigmaColorProfile *profile = NULL;

      const Babl *format = babl_format ("R'G'B' u8");

      if (G_UNLIKELY (! profile))
        profile = ligma_color_profile_new_rgb_srgb ();

      priv->transform = ligma_widget_get_color_transform (GTK_WIDGET (area),
                                                         priv->config,
                                                         profile,
                                                         format,
                                                         format,
                                                         NULL,
                                                         LIGMA_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                         FALSE);
    }
}

static void
ligma_preview_area_destroy_transform (LigmaPreviewArea *area)
{
  LigmaPreviewAreaPrivate *priv = GET_PRIVATE (area);

  if (priv->transform)
    {
      g_object_unref (priv->transform);
      priv->transform = NULL;
    }

  gtk_widget_queue_draw (GTK_WIDGET (area));
}


/**
 * ligma_preview_area_new:
 *
 * Creates a new #LigmaPreviewArea widget.
 *
 * If the preview area is used to draw an image with transparency, you
 * might want to default the checkboard size and colors to user-set
 * Preferences. To do this, you may set the following properties on the
 * newly created #LigmaPreviewArea:
 *
 * |[<!-- language="C" -->
 * g_object_set (area,
 *               "check-size",          ligma_check_size (),
 *               "check-type",          ligma_check_type (),
 *               "check-custom-color1", ligma_check_custom_color1 (),
 *               "check-custom-color2", ligma_check_custom_color2 (),
 *               NULL);
 * ]|
 *
 * Returns: a new #LigmaPreviewArea widget.
 *
 * Since LIGMA 2.2
 **/
GtkWidget *
ligma_preview_area_new (void)
{
  return g_object_new (LIGMA_TYPE_PREVIEW_AREA, NULL);
}

/**
 * ligma_preview_area_draw:
 * @area:      a #LigmaPreviewArea widget.
 * @x:         x offset in preview
 * @y:         y offset in preview
 * @width:     buffer width
 * @height:    buffer height
 * @type:      the #LigmaImageType of @buf
 * @buf: (array): a #guchar buffer that contains the preview pixel data.
 * @rowstride: rowstride of @buf
 *
 * Draws @buf on @area and queues a redraw on the given rectangle.
 *
 * Since LIGMA 2.2
 **/
void
ligma_preview_area_draw (LigmaPreviewArea *area,
                        gint             x,
                        gint             y,
                        gint             width,
                        gint             height,
                        LigmaImageType    type,
                        const guchar    *buf,
                        gint             rowstride)
{
  LigmaPreviewAreaPrivate *priv;
  const guchar           *src;
  guchar                 *dest;
  guint                   size;
  LigmaRGB                 color1;
  LigmaRGB                 color2;
  guchar                  r1;
  guchar                  g1;
  guchar                  b1;
  guchar                  r2;
  guchar                  g2;
  guchar                  b2;
  gint                    row;
  gint                    col;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));
  g_return_if_fail (width >= 0 && height >= 0);

  priv = GET_PRIVATE (area);

  if (width == 0 || height == 0)
    return;

  g_return_if_fail (buf != NULL);
  g_return_if_fail (rowstride > 0);

  if (x + width < 0 || x >= priv->width)
    return;

  if (y + height < 0 || y >= priv->height)
    return;

  if (x < 0)
    {
      gint bpp = ligma_preview_area_image_type_bytes (type);

      buf -= x * bpp;
      width += x;

      x = 0;
    }

  if (x + width > priv->width)
    width = priv->width - x;

  if (y < 0)
    {
      buf -= y * rowstride;
      height += y;

      y = 0;
    }

  if (y + height > priv->height)
    height = priv->height - y;

  if (! priv->buf)
    {
      priv->rowstride = ((priv->width * 3) + 3) & ~3;
      priv->buf = g_new (guchar, priv->rowstride * priv->height);
    }

  size = 1 << (2 + priv->check_size);
  color1 = priv->check_custom_color1;
  color2 = priv->check_custom_color2;
  ligma_checks_get_colors (priv->check_type, &color1, &color2);
  ligma_rgb_get_uchar (&color1, &r1, &g1, &b1);
  ligma_rgb_get_uchar (&color2, &r2, &g2, &b2);

  src  = buf;
  dest = priv->buf + x * 3 + y * priv->rowstride;

  switch (type)
    {
    case LIGMA_RGB_IMAGE:
      for (row = 0; row < height; row++)
        {
          memcpy (dest, src, 3 * width);

          src  += rowstride;
          dest += priv->rowstride;
        }
      break;

    case LIGMA_RGBA_IMAGE:
       for (row = y; row < y + height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = x; col < x + width; col++, s += 4, d+= 3)
            {
              switch (s[3])
                {
                case 0:
                  d[0] = CHECK_R (priv, row, col);
                  d[1] = CHECK_G (priv, row, col);
                  d[2] = CHECK_B (priv, row, col);
                  break;

                case 255:
                  d[0] = s[0];
                  d[1] = s[1];
                  d[2] = s[2];
                  break;

                default:
                  {
                    register guint alpha   = s[3] + 1;
                    register guint check_r = CHECK_R (priv, row, col);
                    register guint check_g = CHECK_G (priv, row, col);
                    register guint check_b = CHECK_B (priv, row, col);

                    d[0] = ((check_r << 8) + (s[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (s[1] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (s[2] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src  += rowstride;
          dest += priv->rowstride;
        }
       break;

    case LIGMA_GRAY_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = 0; col < width; col++, s++, d += 3)
            {
              d[0] = d[1] = d[2] = s[0];
            }

          src  += rowstride;
          dest += priv->rowstride;
        }
      break;

    case LIGMA_GRAYA_IMAGE:
      for (row = y; row < y + height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = x; col < x + width; col++, s += 2, d+= 3)
            {
              switch (s[1])
                {
                case 0:
                  d[0] = CHECK_R (priv, row, col);
                  d[1] = CHECK_G (priv, row, col);
                  d[2] = CHECK_B (priv, row, col);
                  break;

                case 255:
                  d[0] = d[1] = d[2] = s[0];
                  break;

                default:
                  {
                    register guint alpha   = s[1] + 1;
                    register guint check_r = CHECK_R (priv, row, col);
                    register guint check_g = CHECK_G (priv, row, col);
                    register guint check_b = CHECK_B (priv, row, col);

                    d[0] = ((check_r << 8) + (s[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (s[0] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (s[0] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src  += rowstride;
          dest += priv->rowstride;
        }
      break;

    case LIGMA_INDEXED_IMAGE:
      g_return_if_fail (priv->colormap != NULL);
      for (row = 0; row < height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = 0; col < width; col++, s++, d += 3)
            {
              const guchar *colormap = priv->colormap + 3 * s[0];

              d[0] = colormap[0];
              d[1] = colormap[1];
              d[2] = colormap[2];
            }

          src  += rowstride;
          dest += priv->rowstride;
        }
      break;

    case LIGMA_INDEXEDA_IMAGE:
      g_return_if_fail (priv->colormap != NULL);
      for (row = y; row < y + height; row++)
        {
          const guchar *s = src;
          guchar       *d = dest;

          for (col = x; col < x + width; col++, s += 2, d += 3)
            {
              const guchar *colormap  = priv->colormap + 3 * s[0];

              switch (s[1])
                {
                case 0:
                  d[0] = CHECK_R (priv, row, col);
                  d[1] = CHECK_G (priv, row, col);
                  d[2] = CHECK_B (priv, row, col);
                  break;

                case 255:
                  d[0] = colormap[0];
                  d[1] = colormap[1];
                  d[2] = colormap[2];
                  break;

                default:
                  {
                    register guint alpha   = s[3] + 1;
                    register guint check_r = CHECK_R (priv, row, col);
                    register guint check_g = CHECK_G (priv, row, col);
                    register guint check_b = CHECK_B (priv, row, col);

                    d[0] = ((check_r << 8) + (colormap[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (colormap[1] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (colormap[2] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src  += rowstride;
          dest += priv->rowstride;
        }
      break;
    }

  ligma_preview_area_queue_draw (area, x, y, width, height);
}

/**
 * ligma_preview_area_blend:
 * @area:       a #LigmaPreviewArea widget.
 * @x:          x offset in preview
 * @y:          y offset in preview
 * @width:      buffer width
 * @height:     buffer height
 * @type:       the #LigmaImageType of @buf1 and @buf2
 * @buf1: (array): a #guchar buffer that contains the pixel data for
 *                 the lower layer
 * @rowstride1: rowstride of @buf1
 * @buf2: (array): a #guchar buffer that contains the pixel data for
 *                 the upper layer
 * @rowstride2: rowstride of @buf2
 * @opacity:    The opacity of the first layer.
 *
 * Composites @buf1 on @buf2 with the given @opacity, draws the result
 * to @area and queues a redraw on the given rectangle.
 *
 * Since LIGMA 2.2
 **/
void
ligma_preview_area_blend (LigmaPreviewArea *area,
                         gint             x,
                         gint             y,
                         gint             width,
                         gint             height,
                         LigmaImageType    type,
                         const guchar    *buf1,
                         gint             rowstride1,
                         const guchar    *buf2,
                         gint             rowstride2,
                         guchar           opacity)
{
  LigmaPreviewAreaPrivate *priv;
  const guchar           *src1;
  const guchar           *src2;
  guchar                 *dest;
  guint                   size;
  LigmaRGB                 color1;
  LigmaRGB                 color2;
  guchar                  r1;
  guchar                  g1;
  guchar                  b1;
  guchar                  r2;
  guchar                  g2;
  guchar                  b2;
  gint                    row;
  gint                    col;
  gint                    i;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));
  g_return_if_fail (width >= 0 && height >= 0);

  priv = GET_PRIVATE (area);

  if (width == 0 || height == 0)
    return;

  g_return_if_fail (buf1 != NULL);
  g_return_if_fail (buf2 != NULL);
  g_return_if_fail (rowstride1 > 0);
  g_return_if_fail (rowstride2 > 0);

  switch (opacity)
    {
    case 0:
      ligma_preview_area_draw (area, x, y, width, height,
                              type, buf1, rowstride1);
      return;

    case 255:
      ligma_preview_area_draw (area, x, y, width, height,
                              type, buf2, rowstride2);
      return;

    default:
      break;
    }

  if (x + width < 0 || x >= priv->width)
    return;

  if (y + height < 0 || y >= priv->height)
    return;

  if (x < 0)
    {
      gint bpp = ligma_preview_area_image_type_bytes (type);

      buf1 -= x * bpp;
      buf2 -= x * bpp;
      width += x;

      x = 0;
    }

  if (x + width > priv->width)
    width = priv->width - x;

  if (y < 0)
    {
      buf1 -= y * rowstride1;
      buf2 -= y * rowstride2;
      height += y;

      y = 0;
    }

  if (y + height > priv->height)
    height = priv->height - y;

  if (! priv->buf)
    {
      priv->rowstride = ((priv->width * 3) + 3) & ~3;
      priv->buf = g_new (guchar, priv->rowstride * priv->height);
    }

  size = 1 << (2 + priv->check_size);
  color1 = priv->check_custom_color1;
  color2 = priv->check_custom_color2;
  ligma_checks_get_colors (priv->check_type, &color1, &color2);
  ligma_rgb_get_uchar (&color1, &r1, &g1, &b1);
  ligma_rgb_get_uchar (&color2, &r2, &g2, &b2);

  src1 = buf1;
  src2 = buf2;
  dest = priv->buf + x * 3 + y * priv->rowstride;

  switch (type)
    {
    case LIGMA_RGB_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 3, s2 += 3, d+= 3)
            {
              d[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * opacity) >> 8;
              d[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * opacity) >> 8;
              d[2] = ((s1[2] << 8) + (s2[2] - s1[2]) * opacity) >> 8;
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += priv->rowstride;
        }
      break;

    case LIGMA_RGBA_IMAGE:
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 4, s2 += 4, d+= 3)
            {
              guchar inter[4];

              if (s1[3] == s2[3])
                {
                  inter[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * opacity) >> 8;
                  inter[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * opacity) >> 8;
                  inter[2] = ((s1[2] << 8) + (s2[2] - s1[2]) * opacity) >> 8;
                  inter[3] = s1[3];
                }
              else
                {
                  inter[3] = ((s1[3] << 8) + (s2[3] - s1[3]) * opacity) >> 8;

                  if (inter[3])
                    {
                      for (i = 0; i < 3; i++)
                        {
                          gushort a = s1[i] * s1[3];
                          gushort b = s2[i] * s2[3];

                          inter[i] =
                            (((a << 8) + (b  - a) * opacity) >> 8) / inter[3];
                        }
                    }
                }

              switch (inter[3])
                {
                case 0:
                  d[0] = CHECK_R (priv, row, col);
                  d[1] = CHECK_G (priv, row, col);
                  d[2] = CHECK_B (priv, row, col);
                  break;

                case 255:
                  d[0] = inter[0];
                  d[1] = inter[1];
                  d[2] = inter[2];
                  break;

                default:
                  {
                    register guint alpha   = inter[3] + 1;
                    register guint check_r = CHECK_R (priv, row, col);
                    register guint check_g = CHECK_G (priv, row, col);
                    register guint check_b = CHECK_B (priv, row, col);

                    d[0] = ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (inter[1] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (inter[2] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += priv->rowstride;
        }
      break;

    case LIGMA_GRAY_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = 0; col < width; col++, s1++, s2++, d += 3)
            {
              d[0] = d[1] = d[2] =
                ((s1[0] << 8) + (s2[0] - s1[0]) * opacity) >> 8;
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += priv->rowstride;
        }
      break;

    case LIGMA_GRAYA_IMAGE:
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 2, s2 += 2, d+= 3)
            {
              guchar inter[2] = { 0, };

              if (s1[1] == s2[1])
                {
                  inter[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * opacity) >> 8;
                  inter[1] = s1[1];
                }
              else
                {
                  inter[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * opacity) >> 8;

                  if (inter[1])
                    {
                      gushort a = s1[0] * s1[1];
                      gushort b = s2[0] * s2[1];

                      inter[0] =
                        (((a << 8) + (b  - a) * opacity) >> 8) / inter[1];
                    }
                }

              switch (inter[1])
                {
                case 0:
                  d[0] = CHECK_R (priv, row, col);
                  d[1] = CHECK_G (priv, row, col);
                  d[2] = CHECK_B (priv, row, col);
                  break;

                case 255:
                  d[0] = d[1] = d[2] = inter[0];
                  break;

                default:
                  {
                    register guint alpha   = inter[1] + 1;
                    register guint check_r = CHECK_R (priv, row, col);
                    register guint check_g = CHECK_G (priv, row, col);
                    register guint check_b = CHECK_B (priv, row, col);

                    d[0] = ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (inter[0] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (inter[0] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += priv->rowstride;
        }
      break;

    case LIGMA_INDEXED_IMAGE:
      g_return_if_fail (priv->colormap != NULL);
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = 0; col < width; col++, s1++, s2++, d += 3)
            {
              const guchar *cmap1 = priv->colormap + 3 * s1[0];
              const guchar *cmap2 = priv->colormap + 3 * s2[0];

              d[0] = ((cmap1[0] << 8) + (cmap2[0] - cmap1[0]) * opacity) >> 8;
              d[1] = ((cmap1[1] << 8) + (cmap2[1] - cmap1[1]) * opacity) >> 8;
              d[2] = ((cmap1[2] << 8) + (cmap2[2] - cmap1[2]) * opacity) >> 8;
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += priv->rowstride;
        }
      break;

    case LIGMA_INDEXEDA_IMAGE:
      g_return_if_fail (priv->colormap != NULL);
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 2, s2 += 2, d += 3)
            {
              const guchar *cmap1  = priv->colormap + 3 * s1[0];
              const guchar *cmap2  = priv->colormap + 3 * s2[0];
              guchar        inter[4];

              if (s1[1] == s2[1])
                {
                  inter[0] = (((cmap1[0] << 8) +
                               (cmap2[0] - cmap1[0]) * opacity) >> 8);
                  inter[1] = (((cmap1[1] << 8) +
                               (cmap2[1] - cmap1[1]) * opacity) >> 8);
                  inter[2] = (((cmap1[2] << 8) +
                               (cmap2[2] - cmap1[2]) * opacity) >> 8);
                  inter[3] = s1[1];
                }
              else
                {
                  inter[3] = ((s1[1] << 8) + (s2[1] - s1[1]) * opacity) >> 8;

                  if (inter[3])
                    {
                      for (i = 0; i < 3; i++)
                        {
                          gushort a = cmap1[i] * s1[1];
                          gushort b = cmap2[i] * s2[1];

                          inter[i] =
                            (((a << 8) + (b  - a) * opacity) >> 8) / inter[3];
                        }
                    }
                }

              switch (inter[3])
                {
                case 0:
                  d[0] = CHECK_R (priv, row, col);
                  d[1] = CHECK_G (priv, row, col);
                  d[2] = CHECK_B (priv, row, col);
                  break;

                case 255:
                  d[0] = inter[0];
                  d[1] = inter[1];
                  d[2] = inter[2];
                  break;

                default:
                  {
                    register guint alpha   = inter[3] + 1;
                    register guint check_r = CHECK_R (priv, row, col);
                    register guint check_g = CHECK_G (priv, row, col);
                    register guint check_b = CHECK_B (priv, row, col);

                    d[0] = ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                    d[1] = ((check_g << 8) + (inter[1] - check_g) * alpha) >> 8;
                    d[2] = ((check_b << 8) + (inter[2] - check_b) * alpha) >> 8;
                  }
                  break;
                }
            }

          src1 += rowstride1;
          src2 += rowstride2;
          dest += priv->rowstride;
        }
      break;
    }

  ligma_preview_area_queue_draw (area, x, y, width, height);
}

/**
 * ligma_preview_area_mask:
 * @area:           a #LigmaPreviewArea widget.
 * @x:              x offset in preview
 * @y:              y offset in preview
 * @width:          buffer width
 * @height:         buffer height
 * @type:           the #LigmaImageType of @buf1 and @buf2
 * @buf1: (array):  a #guchar buffer that contains the pixel data for
 *                  the lower layer
 * @rowstride1:     rowstride of @buf1
 * @buf2: (array):  a #guchar buffer that contains the pixel data for
 *                  the upper layer
 * @rowstride2:     rowstride of @buf2
 * @mask: (array):  a #guchar buffer representing the mask of the second
 *                  layer.
 * @rowstride_mask: rowstride for the mask.
 *
 * Composites @buf1 on @buf2 with the given @mask, draws the result on
 * @area and queues a redraw on the given rectangle.
 *
 * Since LIGMA 2.2
 **/
void
ligma_preview_area_mask (LigmaPreviewArea *area,
                        gint             x,
                        gint             y,
                        gint             width,
                        gint             height,
                        LigmaImageType    type,
                        const guchar    *buf1,
                        gint             rowstride1,
                        const guchar    *buf2,
                        gint             rowstride2,
                        const guchar    *mask,
                        gint             rowstride_mask)
{
  LigmaPreviewAreaPrivate *priv;
  const guchar           *src1;
  const guchar           *src2;
  const guchar           *src_mask;
  guchar                 *dest;
  guint                   size;
  LigmaRGB                 color1;
  LigmaRGB                 color2;
  guchar                  r1;
  guchar                  g1;
  guchar                  b1;
  guchar                  r2;
  guchar                  g2;
  guchar                  b2;
  gint                    row;
  gint                    col;
  gint                    i;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));
  g_return_if_fail (width >= 0 && height >= 0);

  priv = GET_PRIVATE (area);

  if (width == 0 || height == 0)
    return;

  g_return_if_fail (buf1 != NULL);
  g_return_if_fail (buf2 != NULL);
  g_return_if_fail (mask != NULL);
  g_return_if_fail (rowstride1 > 0);
  g_return_if_fail (rowstride2 > 0);
  g_return_if_fail (rowstride_mask > 0);

  if (x + width < 0 || x >= priv->width)
    return;

  if (y + height < 0 || y >= priv->height)
    return;

  if (x < 0)
    {
      gint bpp = ligma_preview_area_image_type_bytes (type);

      buf1 -= x * bpp;
      buf2 -= x * bpp;
      mask -= x;
      width += x;

      x = 0;
    }

  if (x + width > priv->width)
    width = priv->width - x;

  if (y < 0)
    {
      buf1 -= y * rowstride1;
      buf2 -= y * rowstride2;
      mask -= y * rowstride_mask;
      height += y;

      y = 0;
    }

  if (y + height > priv->height)
    height = priv->height - y;

  if (! priv->buf)
    {
      priv->rowstride = ((priv->width * 3) + 3) & ~3;
      priv->buf = g_new (guchar, priv->rowstride * priv->height);
    }

  size = 1 << (2 + priv->check_size);
  color1 = priv->check_custom_color1;
  color2 = priv->check_custom_color2;
  ligma_checks_get_colors (priv->check_type, &color1, &color2);
  ligma_rgb_get_uchar (&color1, &r1, &g1, &b1);
  ligma_rgb_get_uchar (&color2, &r2, &g2, &b2);

  src1     = buf1;
  src2     = buf2;
  src_mask = mask;
  dest     = priv->buf + x * 3 + y * priv->rowstride;

  switch (type)
    {
    case LIGMA_RGB_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 3, s2 += 3, m++, d+= 3)
            {
               d[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * m[0]) >> 8;
               d[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * m[0]) >> 8;
               d[2] = ((s1[2] << 8) + (s2[2] - s1[2]) * m[0]) >> 8;
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += priv->rowstride;
        }
      break;

    case LIGMA_RGBA_IMAGE:
       for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 4, s2 += 4, m++, d+= 3)
            {
              switch (m[0])
                {
                case 0:
                  switch (s1[3])
                    {
                    case 0:
                      d[0] = CHECK_R (priv, row, col);
                      d[1] = CHECK_G (priv, row, col);
                      d[2] = CHECK_B (priv, row, col);
                      break;

                    case 255:
                      d[0] = s1[0];
                      d[1] = s1[1];
                      d[2] = s1[2];
                      break;

                    default:
                      {
                        register guint alpha   = s1[3] + 1;
                        register guint check_r = CHECK_R (priv, row, col);
                        register guint check_g = CHECK_G (priv, row, col);
                        register guint check_b = CHECK_B (priv, row, col);

                        d[0] = ((check_r << 8) + (s1[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (s1[1] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (s1[2] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                case 255:
                  switch (s2[3])
                    {
                    case 0:
                      d[0] = CHECK_R (priv, row, col);
                      d[1] = CHECK_G (priv, row, col);
                      d[2] = CHECK_B (priv, row, col);
                      break;

                    case 255:
                      d[0] = s2[0];
                      d[1] = s2[1];
                      d[2] = s2[2];
                      break;

                    default:
                      {
                        register guint alpha   = s2[3] + 1;
                        register guint check_r = CHECK_R (priv, row, col);
                        register guint check_g = CHECK_G (priv, row, col);
                        register guint check_b = CHECK_B (priv, row, col);

                        d[0] = ((check_r << 8) + (s2[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (s2[1] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (s2[2] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                default:
                  {
                    guchar inter[4];

                    if (s1[3] == s2[3])
                      {
                        inter[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * m[0]) >> 8;
                        inter[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * m[0]) >> 8;
                        inter[2] = ((s1[2] << 8) + (s2[2] - s1[2]) * m[0]) >> 8;
                        inter[3] = s1[3];
                     }
                    else
                      {
                        inter[3] = ((s1[3] << 8) + (s2[3] - s1[3]) * m[0]) >> 8;

                        if (inter[3])
                          {
                            for (i = 0; i < 3; i++)
                             {
                               gushort a = s1[i] * s1[3];
                               gushort b = s2[i] * s2[3];

                               inter[i] =
                                 (((a << 8) + (b  - a) * m[0]) >> 8) / inter[3];
                             }
                         }
                      }

                    switch (inter[3])
                      {
                      case 0:
                        d[0] = CHECK_R (priv, row, col);
                        d[1] = CHECK_G (priv, row, col);
                        d[2] = CHECK_B (priv, row, col);
                        break;

                      case 255:
                        d[0] = inter[0];
                        d[1] = inter[1];
                        d[2] = inter[2];
                        break;

                      default:
                        {
                          register guint alpha   = inter[3] + 1;
                          register guint check_r = CHECK_R (priv, row, col);
                          register guint check_g = CHECK_G (priv, row, col);
                          register guint check_b = CHECK_B (priv, row, col);

                          d[0] = (((check_r << 8) +
                                   (inter[0] - check_r) * alpha) >> 8);
                          d[1] = (((check_g << 8) +
                                   (inter[1] - check_g) * alpha) >> 8);
                          d[2] = (((check_b << 8) +
                                   (inter[2] - check_b) * alpha) >> 8);
                        }
                        break;
                      }
                  }
                  break;
                }
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += priv->rowstride;
        }
       break;

    case LIGMA_GRAY_IMAGE:
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = 0; col < width; col++, s1++, s2++, m++, d += 3)
            d[0] = d[1] = d[2] = ((s1[0] << 8) + (s2[0] - s1[0]) * m[0]) >> 8;

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += priv->rowstride;
        }
      break;

    case LIGMA_GRAYA_IMAGE:
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 2, s2 += 2, m++, d+= 3)
            {
              switch (m[0])
                {
                case 0:
                  switch (s1[1])
                    {
                    case 0:
                      d[0] = CHECK_R (priv, row, col);
                      d[1] = CHECK_G (priv, row, col);
                      d[2] = CHECK_B (priv, row, col);
                      break;

                    case 255:
                      d[0] = d[1] = d[2] = s1[0];
                      break;

                    default:
                      {
                        register guint alpha   = s1[1] + 1;
                        register guint check_r = CHECK_R (priv, row, col);
                        register guint check_g = CHECK_G (priv, row, col);
                        register guint check_b = CHECK_B (priv, row, col);

                        d[0] = ((check_r << 8) + (s1[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (s1[0] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (s1[0] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                case 255:
                  switch (s2[1])
                    {
                    case 0:
                      d[0] = CHECK_R (priv, row, col);
                      d[1] = CHECK_G (priv, row, col);
                      d[2] = CHECK_B (priv, row, col);
                      break;

                    case 255:
                      d[0] = d[1] = d[2] = s2[0];
                      break;

                    default:
                      {
                        register guint alpha   = s2[1] + 1;
                        register guint check_r = CHECK_R (priv, row, col);
                        register guint check_g = CHECK_G (priv, row, col);
                        register guint check_b = CHECK_B (priv, row, col);

                        d[0] = ((check_r << 8) + (s2[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (s2[0] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (s2[0] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                default:
                  {
                    guchar inter[2] = { 0, };

                    if (s1[1] == s2[1])
                      {
                        inter[0] = ((s1[0] << 8) + (s2[0] - s1[0]) * m[0]) >> 8;
                        inter[1] = s1[1];
                      }
                    else
                      {
                        inter[1] = ((s1[1] << 8) + (s2[1] - s1[1]) * m[0]) >> 8;

                        if (inter[1])
                          {
                            gushort a = s1[0] * s1[1];
                            gushort b = s2[0] * s2[1];

                            inter[0] =
                              (((a << 8) + (b  - a) * m[0]) >> 8) / inter[1];
                          }
                      }

                    switch (inter[1])
                      {
                      case 0:
                        d[0] = CHECK_R (priv, row, col);
                        d[1] = CHECK_G (priv, row, col);
                        d[2] = CHECK_B (priv, row, col);
                        break;

                      case 255:
                        d[0] = d[1] = d[2] = inter[0];
                        break;

                      default:
                        {
                          register guint alpha   = inter[1] + 1;
                          register guint check_r = CHECK_R (priv, row, col);
                          register guint check_g = CHECK_G (priv, row, col);
                          register guint check_b = CHECK_B (priv, row, col);

                          d[0] = ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                          d[1] = ((check_g << 8) + (inter[0] - check_g) * alpha) >> 8;
                          d[2] = ((check_b << 8) + (inter[0] - check_b) * alpha) >> 8;
                        }
                        break;
                      }
                  }
                  break;
                }
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += priv->rowstride;
        }
      break;

    case LIGMA_INDEXED_IMAGE:
      g_return_if_fail (priv->colormap != NULL);
      for (row = 0; row < height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = 0; col < width; col++, s1++, s2++, m++, d += 3)
            {
              const guchar *cmap1 = priv->colormap + 3 * s1[0];
              const guchar *cmap2 = priv->colormap + 3 * s2[0];

              d[0] = ((cmap1[0] << 8) + (cmap2[0] - cmap1[0]) * m[0]) >> 8;
              d[1] = ((cmap1[1] << 8) + (cmap2[1] - cmap1[1]) * m[0]) >> 8;
              d[2] = ((cmap1[2] << 8) + (cmap2[2] - cmap1[2]) * m[0]) >> 8;
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += priv->rowstride;
        }
      break;

    case LIGMA_INDEXEDA_IMAGE:
      g_return_if_fail (priv->colormap != NULL);
      for (row = y; row < y + height; row++)
        {
          const guchar *s1 = src1;
          const guchar *s2 = src2;
          const guchar *m  = src_mask;
          guchar       *d  = dest;

          for (col = x; col < x + width; col++, s1 += 2, s2 += 2, m++, d += 3)
            {
              const guchar *cmap1  = priv->colormap + 3 * s1[0];
              const guchar *cmap2  = priv->colormap + 3 * s2[0];

              switch (m[0])
                {
                case 0:
                  switch (s1[1])
                    {
                    case 0:
                      d[0] = CHECK_R (priv, row, col);
                      d[1] = CHECK_G (priv, row, col);
                      d[2] = CHECK_B (priv, row, col);
                      break;

                    case 255:
                      d[0] = cmap1[0];
                      d[1] = cmap1[1];
                      d[2] = cmap1[2];
                      break;

                    default:
                      {
                        register guint alpha   = s1[1] + 1;
                        register guint check_r = CHECK_R (priv, row, col);
                        register guint check_g = CHECK_G (priv, row, col);
                        register guint check_b = CHECK_B (priv, row, col);

                        d[0] = ((check_r << 8) + (cmap1[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (cmap1[1] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (cmap1[2] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                case 255:
                  switch (s2[1])
                    {
                    case 0:
                      d[0] = CHECK_R (priv, row, col);
                      d[1] = CHECK_G (priv, row, col);
                      d[2] = CHECK_B (priv, row, col);
                      break;

                    case 255:
                      d[0] = cmap2[0];
                      d[1] = cmap2[1];
                      d[2] = cmap2[2];
                      break;

                    default:
                      {
                        register guint alpha   = s2[1] + 1;
                        register guint check_r = CHECK_R (priv, row, col);
                        register guint check_g = CHECK_G (priv, row, col);
                        register guint check_b = CHECK_B (priv, row, col);

                        d[0] = ((check_r << 8) + (cmap2[0] - check_r) * alpha) >> 8;
                        d[1] = ((check_g << 8) + (cmap2[1] - check_g) * alpha) >> 8;
                        d[2] = ((check_b << 8) + (cmap2[2] - check_b) * alpha) >> 8;
                      }
                      break;
                    }
                  break;

                default:
                  {
                    guchar inter[4];

                    if (s1[1] == s2[1])
                      {
                        inter[0] = (((cmap1[0] << 8) +
                                     (cmap2[0] - cmap1[0]) * m[0]) >> 8);
                        inter[1] = (((cmap1[1] << 8) +
                                     (cmap2[1] - cmap1[1]) * m[0]) >> 8);
                        inter[2] = (((cmap1[2] << 8) +
                                     (cmap2[2] - cmap1[2]) * m[0]) >> 8);
                        inter[3] = s1[1];
                      }
                    else
                      {
                        inter[3] = ((s1[1] << 8) + (s2[1] - s1[1]) * m[0]) >> 8;

                        if (inter[3])
                          {
                            for (i = 0 ; i < 3 ; i++)
                              {
                                gushort a = cmap1[i] * s1[1];
                                gushort b = cmap2[i] * s2[1];

                                inter[i] = ((((a << 8) + (b  - a) * m[0]) >> 8)
                                            / inter[3]);
                              }
                          }
                      }

                    switch (inter[3])
                      {
                      case 0:
                        d[0] = CHECK_R (priv, row, col);
                        d[1] = CHECK_G (priv, row, col);
                        d[2] = CHECK_B (priv, row, col);
                        break;

                      case 255:
                        d[0] = inter[0];
                        d[1] = inter[1];
                        d[2] = inter[2];
                        break;

                      default:
                        {
                          register guint alpha   = inter[3] + 1;
                          register guint check_r = CHECK_R (priv, row, col);
                          register guint check_g = CHECK_G (priv, row, col);
                          register guint check_b = CHECK_B (priv, row, col);

                          d[0] =
                            ((check_r << 8) + (inter[0] - check_r) * alpha) >> 8;
                          d[1] =
                            ((check_g << 8) + (inter[1] - check_g) * alpha) >> 8;
                          d[2] =
                            ((check_b << 8) + (inter[2] - check_b) * alpha) >> 8;
                        }
                        break;
                      }
                  }
                  break;
                }
            }

          src1     += rowstride1;
          src2     += rowstride2;
          src_mask += rowstride_mask;
          dest     += priv->rowstride;
        }
      break;
    }

  ligma_preview_area_queue_draw (area, x, y, width, height);
}

/**
 * ligma_preview_area_fill:
 * @area:   a #LigmaPreviewArea widget.
 * @x:      x offset in preview
 * @y:      y offset in preview
 * @width:  width of the rectangle to fill
 * @height: height of the rectangle to fill
 * @red:    red component of the fill color (0-255)
 * @green:  green component of the fill color (0-255)
 * @blue:   red component of the fill color (0-255)
 *
 * Fills the given rectangle of @area in the given color and queues a
 * redraw.
 *
 * Since LIGMA 2.2
 **/
void
ligma_preview_area_fill (LigmaPreviewArea *area,
                        gint             x,
                        gint             y,
                        gint             width,
                        gint             height,
                        guchar           red,
                        guchar           green,
                        guchar           blue)
{
  LigmaPreviewAreaPrivate *priv;
  guchar                 *dest;
  guchar                 *d;
  gint                    row;
  gint                    col;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));
  g_return_if_fail (width >= 0 && height >= 0);

  priv = GET_PRIVATE (area);

  if (width == 0 || height == 0)
    return;

  if (x + width < 0 || x >= priv->width)
    return;

  if (y + height < 0 || y >= priv->height)
    return;

  if (x < 0)
    {
      width += x;
      x = 0;
    }

  if (x + width > priv->width)
    width = priv->width - x;

  if (y < 0)
    {
      height += y;
      y = 0;
    }

  if (y + height > priv->height)
    height = priv->height - y;

  if (! priv->buf)
    {
      priv->rowstride = ((priv->width * 3) + 3) & ~3;
      priv->buf = g_new (guchar, priv->rowstride * priv->height);
    }

  dest = priv->buf + x * 3 + y * priv->rowstride;

  /*  colorize first row  */
  for (col = 0, d = dest; col < width; col++, d+= 3)
    {
      d[0] = red;
      d[1] = green;
      d[2] = blue;
    }

  /*  copy first row to remaining rows  */
  for (row = 1, d = dest; row < height; row++)
    {
      d += priv->rowstride;
      memcpy (d, dest, width * 3);
    }

  ligma_preview_area_queue_draw (area, x, y, width, height);
}

/**
 * ligma_preview_area_set_offsets:
 * @area: a #LigmaPreviewArea
 * @x:    horizontal offset
 * @y:    vertical offset
 *
 * Sets the offsets of the previewed area. This information is used
 * when drawing the checkerboard and to determine the dither offsets.
 *
 * Since: 2.2
 **/
void
ligma_preview_area_set_offsets (LigmaPreviewArea *area,
                               gint             x,
                               gint             y)
{
  LigmaPreviewAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));

  priv = GET_PRIVATE (area);

  priv->offset_x = x;
  priv->offset_y = y;
}

/**
 * ligma_preview_area_set_colormap:
 * @area:       a #LigmaPreviewArea
 * @colormap: (array): a #guchar buffer that contains the colormap
 * @num_colors: the number of colors in the colormap
 *
 * Sets the colormap for the #LigmaPreviewArea widget. You need to
 * call this function before you use ligma_preview_area_draw() with
 * an image type of %LIGMA_INDEXED_IMAGE or %LIGMA_INDEXEDA_IMAGE.
 *
 * Since LIGMA 2.2
 **/
void
ligma_preview_area_set_colormap (LigmaPreviewArea *area,
                                const guchar    *colormap,
                                gint             num_colors)
{
  LigmaPreviewAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));
  g_return_if_fail (colormap != NULL || num_colors == 0);
  g_return_if_fail (num_colors >= 0 && num_colors <= 256);

  priv = GET_PRIVATE (area);

  if (num_colors > 0)
    {
      if (priv->colormap)
        memset (priv->colormap, 0, 3 * 256);
      else
        priv->colormap = g_new0 (guchar, 3 * 256);

      memcpy (priv->colormap, colormap, 3 * num_colors);
    }
  else
    {
      g_free (priv->colormap);
      priv->colormap = NULL;
    }
}

/**
 * ligma_preview_area_set_color_config:
 * @area:   a #LigmaPreviewArea widget.
 * @config: a #LigmaColorConfig object.
 *
 * Sets the color management configuration to use with this preview area.
 *
 * Since: 2.10
 */
void
ligma_preview_area_set_color_config (LigmaPreviewArea *area,
                                    LigmaColorConfig *config)
{
  LigmaPreviewAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));
  g_return_if_fail (config == NULL || LIGMA_IS_COLOR_CONFIG (config));

  priv = GET_PRIVATE (area);

  if (config != priv->config)
    {
      if (priv->config)
        {
          g_signal_handlers_disconnect_by_func (priv->config,
                                                ligma_preview_area_destroy_transform,
                                                area);

          ligma_preview_area_destroy_transform (area);
        }

      g_set_object (&priv->config, config);

      if (priv->config)
        {
          g_signal_connect_swapped (priv->config, "notify",
                                    G_CALLBACK (ligma_preview_area_destroy_transform),
                                    area);
        }
    }
}

/**
 * ligma_preview_area_get_size:
 * @area:   a #LigmaPreviewArea widget.
 * @width: (out): The preview areay width
 * @height: (out): The preview areay height
 *
 * Gets the preview area size
 */
void
ligma_preview_area_get_size (LigmaPreviewArea *area,
                            gint            *width,
                            gint            *height)
{
  LigmaPreviewAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));

  priv = GET_PRIVATE (area);

  if (width)  *width  = priv->width;
  if (height) *height = priv->height;
}

/**
 * ligma_preview_area_set_max_size:
 * @area:   a #LigmaPreviewArea widget
 * @width:  the maximum width in pixels or -1 to unset the limit
 * @height: the maximum height in pixels or -1 to unset the limit
 *
 * Usually a #LigmaPreviewArea fills the size that it is
 * allocated. This function allows you to limit the preview area to a
 * maximum size. If a larger size is allocated for the widget, the
 * preview will draw itself centered into the allocated area.
 *
 * Since: 2.2
 **/
void
ligma_preview_area_set_max_size (LigmaPreviewArea *area,
                                gint             width,
                                gint             height)
{
  LigmaPreviewAreaPrivate *priv;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));

  priv = GET_PRIVATE (area);

  priv->max_width  = width;
  priv->max_height = height;
}



/*  popup menu  */

static void
ligma_preview_area_menu_toggled (GtkWidget       *item,
                                LigmaPreviewArea *area)
{
  gboolean active = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (item));

  if (active)
    {
      const gchar *name = g_object_get_data (G_OBJECT (item),
                                             "ligma-preview-area-prop-name");
      if (name)
        {
          gint  value =
            GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item),
                                                "ligma-preview-area-prop-value"));
          g_object_set (area,
                        name, value,
                        NULL);
        }
    }
}

static GtkWidget *
ligma_preview_area_menu_new (LigmaPreviewArea *area,
                            const gchar     *property)
{
  GParamSpec *pspec;
  GEnumClass *enum_class;
  GEnumValue *enum_value;
  GtkWidget  *menu;
  GtkWidget  *item;
  GSList     *group = NULL;
  gint        value;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (area), property);

  g_return_val_if_fail (G_IS_PARAM_SPEC_ENUM (pspec), NULL);

  g_object_get (area,
                property, &value,
                NULL);

  enum_class = G_PARAM_SPEC_ENUM (pspec)->enum_class;

  menu = gtk_menu_new ();

  for (enum_value = enum_class->values; enum_value->value_name; enum_value++)
    {
      const gchar *name = ligma_enum_value_get_desc (enum_class, enum_value);

      item = gtk_radio_menu_item_new_with_label (group, name);

      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      gtk_widget_show (item);

      g_object_set_data (G_OBJECT (item),
                         "ligma-preview-area-prop-name",
                         (gpointer) property);

      g_object_set_data (G_OBJECT (item),
                         "ligma-preview-area-prop-value",
                         GINT_TO_POINTER (enum_value->value));

      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                      (enum_value->value == value));

      g_signal_connect (item, "toggled",
                        G_CALLBACK (ligma_preview_area_menu_toggled),
                        area);

      group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
    }

  item = gtk_menu_item_new_with_label (g_param_spec_get_nick (pspec));

  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), menu);

  gtk_widget_show (item);

  return item;
}

/**
 * ligma_preview_area_menu_popup:
 * @area:  a #LigmaPreviewArea
 * @event: (nullable): the button event that causes the menu to popup or %NULL
 *
 * Creates a popup menu that allows one to configure the size and type of
 * the checkerboard pattern that the @area uses to visualize transparency.
 *
 * Since: 2.2
 **/
void
ligma_preview_area_menu_popup (LigmaPreviewArea *area,
                              GdkEventButton  *event)
{
  GtkWidget *menu;

  g_return_if_fail (LIGMA_IS_PREVIEW_AREA (area));

  menu = gtk_menu_new ();
  gtk_menu_set_screen (GTK_MENU (menu),
                       gtk_widget_get_screen (GTK_WIDGET (area)));

  gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                         ligma_preview_area_menu_new (area, "check-type"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                         ligma_preview_area_menu_new (area, "check-size"));
#if 0
  /* ligma_preview_area_menu_new() currently only handles enum types, and
   * in particular not color properties.
   */
  gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                         ligma_preview_area_menu_new (area, "check-custom-color1"));
  gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                         ligma_preview_area_menu_new (area, "check-custom-color2"));
#endif

  gtk_menu_popup_at_pointer (GTK_MENU (menu), (GdkEvent *) event);
}
