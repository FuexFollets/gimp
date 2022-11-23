/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmabrushselectbutton.c
 * Copyright (C) 1998 Andy Thomas
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "ligma.h"

#include "ligmauitypes.h"
#include "ligmabrushselectbutton.h"
#include "ligmauimarshal.h"

#include "libligma-intl.h"


/**
 * SECTION: ligmabrushselectbutton
 * @title: ligmabrushselectbutton
 * @short_description: A button that pops up a brush selection dialog.
 *
 * A button that pops up a brush selection dialog.
 **/


#define CELL_SIZE 20


enum
{
  BRUSH_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_BRUSH_NAME,
  PROP_BRUSH_OPACITY,
  PROP_BRUSH_SPACING,
  PROP_BRUSH_PAINT_MODE,
  N_PROPS
};


struct _LigmaBrushSelectButtonPrivate
{
  gchar         *title;

  gchar         *brush_name;      /* Local copy */
  gdouble        opacity;
  gint           spacing;
  LigmaLayerMode  paint_mode;
  gint           width;
  gint           height;
  guchar        *mask_data;       /* local copy */

  GtkWidget     *inside;
  GtkWidget     *preview;
  GtkWidget     *popup;
};


/*  local function prototypes  */

static void   ligma_brush_select_button_finalize     (GObject      *object);

static void   ligma_brush_select_button_set_property (GObject      *object,
                                                     guint         property_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);
static void   ligma_brush_select_button_get_property (GObject      *object,
                                                     guint         property_id,
                                                     GValue       *value,
                                                     GParamSpec   *pspec);

static void   ligma_brush_select_button_clicked  (LigmaBrushSelectButton *button);

static void   ligma_brush_select_button_callback (const gchar          *brush_name,
                                                 gdouble               opacity,
                                                 gint                  spacing,
                                                 LigmaLayerMode         paint_mode,
                                                 gint                  width,
                                                 gint                  height,
                                                 gint                  mask_size,
                                                 const guchar         *mask_data,
                                                 gboolean              dialog_closing,
                                                 gpointer              user_data);

static void     ligma_brush_select_preview_resize  (LigmaBrushSelectButton *button);
static gboolean ligma_brush_select_preview_events  (GtkWidget             *widget,
                                                   GdkEvent              *event,
                                                   LigmaBrushSelectButton *button);
static void     ligma_brush_select_preview_draw    (LigmaPreviewArea       *area,
                                                   gint                   x,
                                                   gint                   y,
                                                   gint                   width,
                                                   gint                   height,
                                                   const guchar          *mask_data,
                                                   gint                   rowstride);
static void     ligma_brush_select_preview_update  (GtkWidget             *preview,
                                                   gint                   brush_width,
                                                   gint                   brush_height,
                                                   const guchar          *mask_data);

static void     ligma_brush_select_button_open_popup  (LigmaBrushSelectButton *button,
                                                      gint                   x,
                                                      gint                   y);
static void     ligma_brush_select_button_close_popup (LigmaBrushSelectButton *button);

static void   ligma_brush_select_drag_data_received (LigmaBrushSelectButton *button,
                                                    GdkDragContext        *context,
                                                    gint                   x,
                                                    gint                   y,
                                                    GtkSelectionData      *selection,
                                                    guint                  info,
                                                    guint                  time);

static GtkWidget * ligma_brush_select_button_create_inside (LigmaBrushSelectButton *button);


static const GtkTargetEntry target = { "application/x-ligma-brush-name", 0 };

static guint brush_button_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *brush_button_props[N_PROPS] = { NULL, };


G_DEFINE_TYPE_WITH_PRIVATE (LigmaBrushSelectButton, ligma_brush_select_button,
                            LIGMA_TYPE_SELECT_BUTTON)


static void
ligma_brush_select_button_class_init (LigmaBrushSelectButtonClass *klass)
{
  GObjectClass          *object_class        = G_OBJECT_CLASS (klass);
  LigmaSelectButtonClass *select_button_class = LIGMA_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = ligma_brush_select_button_finalize;
  object_class->set_property = ligma_brush_select_button_set_property;
  object_class->get_property = ligma_brush_select_button_get_property;

  select_button_class->select_destroy = ligma_brush_select_destroy;

  klass->brush_set = NULL;

  /**
   * LigmaBrushSelectButton:title:
   *
   * The title to be used for the brush selection popup dialog.
   *
   * Since: 2.4
   */
  brush_button_props[PROP_TITLE] = g_param_spec_string ("title",
                                                        "Title",
                                                        "The title to be used for the brush selection popup dialog",
                                                        _("Brush Selection"),
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY);

  /**
   * LigmaBrushSelectButton:brush-name:
   *
   * The name of the currently selected brush.
   *
   * Since: 2.4
   */
  brush_button_props[PROP_BRUSH_NAME] = g_param_spec_string ("brush-name",
                                                             "Brush name",
                                                             "The name of the currently selected brush",
                                                             NULL,
                                                             LIGMA_PARAM_READWRITE);

  /**
   * LigmaBrushSelectButton:opacity:
   *
   * The opacity of the currently selected brush.
   *
   * Since: 2.4
   */
  brush_button_props[PROP_BRUSH_OPACITY] = g_param_spec_double ("brush-opacity",
                                                                "Brush opacity",
                                                                "The opacity of the currently selected brush",
                                                                -1.0, 100.0, -1.0,
                                                                LIGMA_PARAM_READWRITE);

  /**
   * LigmaBrushSelectButton:spacing:
   *
   * The spacing of the currently selected brush.
   *
   * Since: 2.4
   */
  brush_button_props[PROP_BRUSH_SPACING] = g_param_spec_int ("brush-spacing",
                                                             "Brush spacing",
                                                             "The spacing of the currently selected brush",
                                                             -G_MAXINT, 1000, -1,
                                                             LIGMA_PARAM_READWRITE);

  /**
   * LigmaBrushSelectButton:paint-mode:
   *
   * The paint mode.
   *
   * Since: 2.4
   */
  brush_button_props[PROP_BRUSH_PAINT_MODE] = g_param_spec_int ("brush-paint-mode",
                                                                "Brush paint mode",
                                                                "The paint mode of the currently selected brush",
                                                                -1, LIGMA_LAYER_MODE_LUMINANCE,
                                                                -1,
                                                                LIGMA_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, brush_button_props);

  /**
   * LigmaBrushSelectButton::brush-set:
   * @widget: the object which received the signal.
   * @brush_name: the name of the currently selected brush.
   * @opacity: opacity of the brush
   * @spacing: spacing of the brush
   * @paint_mode: paint mode of the brush
   * @width: width of the brush
   * @height: height of the brush
   * @mask_data: (array) (element-type guchar): brush mask data
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::brush-set signal is emitted when the user selects a brush.
   *
   * Since: 2.4
   */
  brush_button_signals[BRUSH_SET] =
    g_signal_new ("brush-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaBrushSelectButtonClass, brush_set),
                  NULL, NULL,
                  _ligmaui_marshal_VOID__STRING_DOUBLE_INT_INT_INT_INT_POINTER_BOOLEAN,
                  G_TYPE_NONE, 8,
                  G_TYPE_STRING,
                  G_TYPE_DOUBLE,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_INT,
                  G_TYPE_POINTER,
                  G_TYPE_BOOLEAN);
}

static void
ligma_brush_select_button_init (LigmaBrushSelectButton *button)
{
  LigmaBrushSelectButtonPrivate *priv;
  gint                          mask_bpp;
  gint                          mask_data_size;
  gint                          color_bpp;
  gint                          color_data_size;
  guint8                       *color_data;

  button->priv = ligma_brush_select_button_get_instance_private (button);

  priv = button->priv;

  priv->brush_name = ligma_context_get_brush ();
  ligma_brush_get_pixels (priv->brush_name,
                         &priv->width,
                         &priv->height,
                         &mask_bpp,
                         &mask_data_size,
                         &priv->mask_data,
                         &color_bpp,
                         &color_data_size,
                         &color_data);

  if (color_data)
    g_free (color_data);

  priv->inside = ligma_brush_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), priv->inside);
}

/**
 * ligma_brush_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL means to use the default
 *              title.
 * @brush_name: (nullable): Initial brush name or %NULL to use current selection.
 * @opacity:    Initial opacity. -1 means to use current opacity.
 * @spacing:    Initial spacing. -1 means to use current spacing.
 * @paint_mode: Initial paint mode.  -1 means to use current paint mode.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a brush. This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_brush_select_button_new (const gchar   *title,
                              const gchar   *brush_name,
                              gdouble        opacity,
                              gint           spacing,
                              LigmaLayerMode  paint_mode)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (LIGMA_TYPE_BRUSH_SELECT_BUTTON,
                           "title",            title,
                           "brush-name",       brush_name,
                           "brush-opacity",    opacity,
                           "brush-spacing",    spacing,
                           "brush-paint-mode", paint_mode,
                           NULL);
  else
    button = g_object_new (LIGMA_TYPE_BRUSH_SELECT_BUTTON,
                           "brush-name",       brush_name,
                           "brush-opacity",    opacity,
                           "brush-spacing",    spacing,
                           "brush-paint-mode", paint_mode,
                           NULL);

  return button;
}

/**
 * ligma_brush_select_button_get_brush:
 * @button:                        A #LigmaBrushSelectButton
 * @opacity:    (out) (optional):  Opacity of the selected brush.
 * @spacing:    (out) (optional):  Spacing of the selected brush.
 * @paint_mode: (out) (optional):  Paint mode of the selected brush.
 *
 * Retrieves the properties of currently selected brush.
 *
 * Returns: an internal copy of the brush name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
ligma_brush_select_button_get_brush (LigmaBrushSelectButton *button,
                                    gdouble               *opacity,
                                    gint                  *spacing,
                                    LigmaLayerMode         *paint_mode)
{
  g_return_val_if_fail (LIGMA_IS_BRUSH_SELECT_BUTTON (button), NULL);

  if (opacity)
    *opacity = button->priv->opacity;

  if (spacing)
    *spacing = button->priv->spacing;

  if (paint_mode)
    *paint_mode = button->priv->paint_mode;

  return button->priv->brush_name;
}

/**
 * ligma_brush_select_button_set_brush:
 * @button: A #LigmaBrushSelectButton
 * @brush_name: (nullable): Brush name to set; %NULL means no change.
 * @opacity:    Opacity to set. -1.0 means no change.
 * @spacing:    Spacing to set. -1 means no change.
 * @paint_mode: Paint mode to set.  -1 means no change.
 *
 * Sets the current brush and other values for the brush select
 * button.
 *
 * Since: 2.4
 */
void
ligma_brush_select_button_set_brush (LigmaBrushSelectButton *button,
                                    const gchar           *brush_name,
                                    gdouble                opacity,
                                    gint                   spacing,
                                    LigmaLayerMode          paint_mode)
{
  LigmaSelectButton *select_button;

  g_return_if_fail (LIGMA_IS_BRUSH_SELECT_BUTTON (button));

  select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      ligma_brushes_set_popup (select_button->temp_callback, brush_name,
                              opacity, spacing, paint_mode);
    }
  else
    {
      gchar  *name;
      gint    width;
      gint    height;
      gint    bytes;
      gint    mask_data_size;
      guint8 *mask_data;
      gint    color_bpp;
      gint    color_data_size;
      guint8 *color_data;

      if (brush_name && *brush_name)
        name = g_strdup (brush_name);
      else
        name = ligma_context_get_brush ();

      if (ligma_brush_get_pixels (name,
                                 &width,
                                 &height,
                                 &bytes,
                                 &mask_data_size,
                                 &mask_data,
                                 &color_bpp,
                                 &color_data_size,
                                 &color_data))
        {
          if (color_data)
            g_free (color_data);

          if (opacity < 0.0)
            opacity = ligma_context_get_opacity ();

          if (spacing == -1)
            ligma_brush_get_spacing (name, &spacing);

          if (paint_mode == -1)
            paint_mode = ligma_context_get_paint_mode ();

          ligma_brush_select_button_callback (name,
                                             opacity, spacing, paint_mode,
                                             width, height, width * height,
                                             mask_data,
                                             FALSE, button);

          g_free (mask_data);
        }

      g_free (name);
    }
}


/*  private functions  */

static void
ligma_brush_select_button_finalize (GObject *object)
{
  LigmaBrushSelectButton *button = LIGMA_BRUSH_SELECT_BUTTON (object);

  g_clear_pointer (&button->priv->brush_name, g_free);
  g_clear_pointer (&button->priv->mask_data,  g_free);
  g_clear_pointer (&button->priv->title,      g_free);

  G_OBJECT_CLASS (ligma_brush_select_button_parent_class)->finalize (object);
}

static void
ligma_brush_select_button_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  LigmaBrushSelectButton *button = LIGMA_BRUSH_SELECT_BUTTON (object);
  gdouble                opacity;
  gint32                 spacing;
  gint32                 paint_mode;

  switch (property_id)
    {
    case PROP_TITLE:
      button->priv->title = g_value_dup_string (value);
      break;

    case PROP_BRUSH_NAME:
      ligma_brush_select_button_set_brush (button,
                                          g_value_get_string (value),
                                          -1.0, -1, -1);
      break;

    case PROP_BRUSH_OPACITY:
      opacity = g_value_get_double (value);
      if (opacity >= 0.0)
        button->priv->opacity = opacity;
      break;

    case PROP_BRUSH_SPACING:
      spacing = g_value_get_int (value);
      if (spacing != -1)
        button->priv->spacing = spacing;
      break;

    case PROP_BRUSH_PAINT_MODE:
      paint_mode = g_value_get_int (value);
      if (paint_mode != -1)
        button->priv->paint_mode = paint_mode;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_brush_select_button_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  LigmaBrushSelectButton *button = LIGMA_BRUSH_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, button->priv->title);
      break;

    case PROP_BRUSH_NAME:
      g_value_set_string (value, button->priv->brush_name);
      break;

    case PROP_BRUSH_OPACITY:
      g_value_set_double (value, button->priv->opacity);
      break;

    case PROP_BRUSH_SPACING:
      g_value_set_int (value, button->priv->spacing);
      break;

    case PROP_BRUSH_PAINT_MODE:
      g_value_set_int (value, button->priv->paint_mode);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_brush_select_button_callback (const gchar   *name,
                                   gdouble        opacity,
                                   gint           spacing,
                                   LigmaLayerMode  paint_mode,
                                   gint           width,
                                   gint           height,
                                   gint           mask_size,
                                   const guchar  *mask_data,
                                   gboolean       dialog_closing,
                                   gpointer       data)
{
  LigmaBrushSelectButton *button        = LIGMA_BRUSH_SELECT_BUTTON (data);
  LigmaSelectButton      *select_button = LIGMA_SELECT_BUTTON (button);

  g_free (button->priv->brush_name);
  g_free (button->priv->mask_data);

  button->priv->brush_name = g_strdup (name);
  button->priv->width      = width;
  button->priv->height     = height;
  button->priv->mask_data  = g_memdup2 (mask_data, width * height);
  button->priv->opacity    = opacity;
  button->priv->spacing    = spacing;
  button->priv->paint_mode = paint_mode;

  ligma_brush_select_preview_update (button->priv->preview,
                                    width, height, mask_data);

  if (dialog_closing)
    select_button->temp_callback = NULL;

  g_signal_emit (button, brush_button_signals[BRUSH_SET], 0,
                 name, opacity, spacing, paint_mode, width, height, mask_data,
                 dialog_closing);
  g_object_notify_by_pspec (G_OBJECT (button), brush_button_props[PROP_BRUSH_NAME]);
}

static void
ligma_brush_select_button_clicked (LigmaBrushSelectButton *button)
{
  LigmaSelectButton *select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      /*  calling ligma_brushes_set_popup() raises the dialog  */
      ligma_brushes_set_popup (select_button->temp_callback,
                              button->priv->brush_name,
                              button->priv->opacity,
                              button->priv->spacing,
                              button->priv->paint_mode);
    }
  else
    {
      select_button->temp_callback =
        ligma_brush_select_new (button->priv->title,
                               button->priv->brush_name,
                               button->priv->opacity,
                               button->priv->spacing,
                               button->priv->paint_mode,
                               ligma_brush_select_button_callback,
                               button, NULL);
    }
}

static void
ligma_brush_select_preview_resize (LigmaBrushSelectButton *button)
{
  if (button->priv->width  > 0 &&
      button->priv->height > 0)
    ligma_brush_select_preview_update (button->priv->preview,
                                      button->priv->width,
                                      button->priv->height,
                                      button->priv->mask_data);
}

static gboolean
ligma_brush_select_preview_events (GtkWidget             *widget,
                                  GdkEvent              *event,
                                  LigmaBrushSelectButton *button)
{
  GdkEventButton *bevent;

  if (button->priv->mask_data)
    {
      switch (event->type)
        {
        case GDK_BUTTON_PRESS:
          bevent = (GdkEventButton *) event;

          if (bevent->button == 1)
            {
              gtk_grab_add (widget);
              ligma_brush_select_button_open_popup (button,
                                                   bevent->x, bevent->y);
            }
          break;

        case GDK_BUTTON_RELEASE:
          bevent = (GdkEventButton *) event;

          if (bevent->button == 1)
            {
              gtk_grab_remove (widget);
              ligma_brush_select_button_close_popup (button);
            }
          break;

        default:
          break;
        }
    }

  return FALSE;
}

static void
ligma_brush_select_preview_draw (LigmaPreviewArea *area,
                                gint             x,
                                gint             y,
                                gint             width,
                                gint             height,
                                const guchar    *mask_data,
                                gint             rowstride)
{
  const guchar *src;
  guchar       *dest;
  guchar       *buf;
  gint          i, j;

  buf = g_new (guchar, width * height);

  src  = mask_data;
  dest = buf;

  for (j = 0; j < height; j++)
    {
      const guchar *s = src;

      for (i = 0; i < width; i++, s++, dest++)
        *dest = 255 - *s;

      src += rowstride;
    }

  ligma_preview_area_draw (area,
                          x, y, width, height,
                          LIGMA_GRAY_IMAGE,
                          buf,
                          width);

  g_free (buf);
}

static void
ligma_brush_select_preview_update (GtkWidget    *preview,
                                  gint          brush_width,
                                  gint          brush_height,
                                  const guchar *mask_data)
{
  LigmaPreviewArea *area = LIGMA_PREVIEW_AREA (preview);
  GtkAllocation    allocation;
  gint             x, y;
  gint             width, height;

  gtk_widget_get_allocation (preview, &allocation);

  width  = MIN (brush_width,  allocation.width);
  height = MIN (brush_height, allocation.height);

  x = ((allocation.width  - width)  / 2);
  y = ((allocation.height - height) / 2);

  if (x || y)
    ligma_preview_area_fill (area,
                            0, 0,
                            allocation.width,
                            allocation.height,
                            0xFF, 0xFF, 0xFF);

  ligma_brush_select_preview_draw (area,
                                  x, y, width, height,
                                  mask_data, brush_width);
}

static void
ligma_brush_select_button_open_popup (LigmaBrushSelectButton *button,
                                     gint                   x,
                                     gint                   y)
{
  LigmaBrushSelectButtonPrivate *priv = button->priv;
  GtkWidget                    *frame;
  GtkWidget                    *preview;
  GdkMonitor                   *monitor;
  GdkRectangle                  workarea;
  gint                          x_org;
  gint                          y_org;

  if (priv->popup)
    ligma_brush_select_button_close_popup (button);

  if (priv->width <= CELL_SIZE && priv->height <= CELL_SIZE)
    return;

  priv->popup = gtk_window_new (GTK_WINDOW_POPUP);
  gtk_window_set_type_hint (GTK_WINDOW (priv->popup), GDK_WINDOW_TYPE_HINT_DND);
  gtk_window_set_screen (GTK_WINDOW (priv->popup),
                         gtk_widget_get_screen (GTK_WIDGET (button)));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (priv->popup), frame);
  gtk_widget_show (frame);

  preview = ligma_preview_area_new ();
  gtk_widget_set_size_request (preview, priv->width, priv->height);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  /* decide where to put the popup */
  gdk_window_get_origin (gtk_widget_get_window (priv->preview),
                         &x_org, &y_org);

  monitor = ligma_widget_get_monitor (GTK_WIDGET (button));
  gdk_monitor_get_workarea (monitor, &workarea);

  x = x_org + x - (priv->width  / 2);
  y = y_org + y - (priv->height / 2);

  x = CLAMP (x, workarea.x, workarea.x + workarea.width  - priv->width);
  y = CLAMP (y, workarea.y, workarea.y + workarea.height - priv->height);

  gtk_window_move (GTK_WINDOW (priv->popup), x, y);

  gtk_widget_show (priv->popup);

  /*  Draw the brush  */
  ligma_brush_select_preview_draw (LIGMA_PREVIEW_AREA (preview),
                                  0, 0, priv->width, priv->height,
                                  priv->mask_data, priv->width);
}

static void
ligma_brush_select_button_close_popup (LigmaBrushSelectButton *button)
{
  g_clear_pointer (&button->priv->popup, gtk_widget_destroy);
}

static void
ligma_brush_select_drag_data_received (LigmaBrushSelectButton *button,
                                      GdkDragContext        *context,
                                      gint                   x,
                                      gint                   y,
                                      GtkSelectionData      *selection,
                                      guint                  info,
                                      guint                  time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("%s: received invalid brush data", G_STRFUNC);
      return;
    }

  str = g_strndup ((const gchar *) gtk_selection_data_get_data (selection),
                   length);

  if (g_utf8_validate (str, -1, NULL))
    {
      gint     pid;
      gpointer unused;
      gint     name_offset = 0;

      if (sscanf (str, "%i:%p:%n", &pid, &unused, &name_offset) >= 2 &&
          pid == ligma_getpid () && name_offset > 0)
        {
          gchar *name = str + name_offset;

          ligma_brush_select_button_set_brush (button, name, -1.0, -1, -1);
        }
    }

  g_free (str);
}

static GtkWidget *
ligma_brush_select_button_create_inside (LigmaBrushSelectButton *brush_button)
{
  LigmaBrushSelectButtonPrivate *priv = brush_button->priv;
  GtkWidget                    *hbox;
  GtkWidget                    *frame;
  GtkWidget                    *button;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

  priv->preview = ligma_preview_area_new ();
  gtk_widget_add_events (priv->preview,
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  gtk_widget_set_size_request (priv->preview, CELL_SIZE, CELL_SIZE);
  gtk_container_add (GTK_CONTAINER (frame), priv->preview);

  g_signal_connect_swapped (priv->preview, "size-allocate",
                            G_CALLBACK (ligma_brush_select_preview_resize),
                            brush_button);
  g_signal_connect (priv->preview, "event",
                    G_CALLBACK (ligma_brush_select_preview_events),
                    brush_button);

  gtk_drag_dest_set (GTK_WIDGET (priv->preview),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (priv->preview, "drag-data-received",
                            G_CALLBACK (ligma_brush_select_drag_data_received),
                            brush_button);

  button = gtk_button_new_with_mnemonic (_("_Browse..."));
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 0);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ligma_brush_select_button_clicked),
                            brush_button);

  gtk_widget_show_all (hbox);

  return hbox;
}
