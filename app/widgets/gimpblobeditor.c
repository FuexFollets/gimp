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

#include <gtk/gtk.h>

#include "widgets-types.h"

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "paint/ligmaink-blob.h"

#include "ligmablobeditor.h"


enum
{
  PROP_0,
  PROP_TYPE,
  PROP_ASPECT,
  PROP_ANGLE
};


static void      ligma_blob_editor_set_property   (GObject        *object,
                                                  guint           property_id,
                                                  const GValue   *value,
                                                  GParamSpec     *pspec);
static void      ligma_blob_editor_get_property   (GObject        *object,
                                                  guint           property_id,
                                                  GValue         *value,
                                                  GParamSpec     *pspec);

static gboolean  ligma_blob_editor_draw           (GtkWidget      *widget,
                                                  cairo_t        *cr);
static gboolean  ligma_blob_editor_button_press   (GtkWidget      *widget,
                                                  GdkEventButton *event);
static gboolean  ligma_blob_editor_button_release (GtkWidget      *widget,
                                                  GdkEventButton *event);
static gboolean  ligma_blob_editor_motion_notify  (GtkWidget      *widget,
                                                  GdkEventMotion *event);

static void      ligma_blob_editor_get_handle     (LigmaBlobEditor *editor,
                                                  GdkRectangle   *rect);
static void      ligma_blob_editor_draw_blob      (LigmaBlobEditor *editor,
                                                  cairo_t        *cr,
                                                  gdouble         xc,
                                                  gdouble         yc,
                                                  gdouble         radius);


G_DEFINE_TYPE (LigmaBlobEditor, ligma_blob_editor, GTK_TYPE_DRAWING_AREA)

#define parent_class ligma_blob_editor_parent_class


static void
ligma_blob_editor_class_init (LigmaBlobEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property         = ligma_blob_editor_set_property;
  object_class->get_property         = ligma_blob_editor_get_property;

  widget_class->draw                 = ligma_blob_editor_draw;
  widget_class->button_press_event   = ligma_blob_editor_button_press;
  widget_class->button_release_event = ligma_blob_editor_button_release;
  widget_class->motion_notify_event  = ligma_blob_editor_motion_notify;

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("blob-type",
                                                      NULL, NULL,
                                                      LIGMA_TYPE_INK_BLOB_TYPE,
                                                      LIGMA_INK_BLOB_TYPE_CIRCLE,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_ASPECT,
                                   g_param_spec_double ("blob-aspect",
                                                        NULL, NULL,
                                                        1.0, 10.0, 1.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_ANGLE,
                                   g_param_spec_double ("blob-angle",
                                                        NULL, NULL,
                                                        -G_PI, G_PI, 0.0,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_blob_editor_init (LigmaBlobEditor *editor)
{
  editor->active = FALSE;

  gtk_widget_add_events (GTK_WIDGET (editor),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_EXPOSURE_MASK);
}

GtkWidget *
ligma_blob_editor_new (LigmaInkBlobType  type,
                      gdouble          aspect,
                      gdouble          angle)
{
  return g_object_new (LIGMA_TYPE_BLOB_EDITOR,
                       "blob-type",   type,
                       "blob-aspect", aspect,
                       "blob-angle",  angle,
                       NULL);
}

static void
ligma_blob_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaBlobEditor *editor = LIGMA_BLOB_EDITOR (object);

  switch (property_id)
    {
    case PROP_TYPE:
      editor->type = g_value_get_enum (value);
      break;
    case PROP_ASPECT:
      editor->aspect = g_value_get_double (value);
      break;
    case PROP_ANGLE:
      editor->angle = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  gtk_widget_queue_draw (GTK_WIDGET (editor));
}

static void
ligma_blob_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  LigmaBlobEditor *editor = LIGMA_BLOB_EDITOR (object);

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, editor->type);
      break;
    case PROP_ASPECT:
      g_value_set_double (value, editor->aspect);
      break;
    case PROP_ANGLE:
      g_value_set_double (value, editor->angle);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
ligma_blob_editor_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  LigmaBlobEditor  *editor = LIGMA_BLOB_EDITOR (widget);
  GtkStyleContext *style  = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  GdkRectangle     rect;
  gint             r0;

  gtk_widget_get_allocation (widget, &allocation);

  r0 = MIN (allocation.width, allocation.height) / 2;

  if (r0 < 2)
    return TRUE;

  ligma_blob_editor_draw_blob (editor, cr,
                              allocation.width  / 2.0,
                              allocation.height / 2.0,
                              0.9 * r0);

  ligma_blob_editor_get_handle (editor, &rect);

  gtk_style_context_save (style);

  gtk_style_context_add_class (style, GTK_STYLE_CLASS_BUTTON);

  gtk_style_context_set_state (style,
                               editor->in_handle ?
                               GTK_STATE_FLAG_PRELIGHT : 0);

  gtk_render_background (style, cr, rect.x, rect.y, rect.width, rect.height);
  gtk_render_frame (style, cr, rect.x, rect.y, rect.width, rect.height);

  gtk_style_context_restore (style);

  return TRUE;
}

static gboolean
ligma_blob_editor_button_press (GtkWidget      *widget,
                               GdkEventButton *event)
{
  LigmaBlobEditor *editor = LIGMA_BLOB_EDITOR (widget);

  if (editor->in_handle)
    editor->active = TRUE;

  return TRUE;
}

static gboolean
ligma_blob_editor_button_release (GtkWidget      *widget,
                                 GdkEventButton *event)
{
  LigmaBlobEditor *editor = LIGMA_BLOB_EDITOR (widget);

  editor->active = FALSE;

  return TRUE;
}

static gboolean
ligma_blob_editor_motion_notify (GtkWidget      *widget,
                                GdkEventMotion *event)
{
  LigmaBlobEditor *editor = LIGMA_BLOB_EDITOR (widget);

  if (editor->active)
    {
      GtkAllocation allocation;
      gint          x;
      gint          y;
      gint          rsquare;

      gtk_widget_get_allocation (widget, &allocation);

      x = event->x - allocation.width  / 2;
      y = event->y - allocation.height / 2;

      rsquare = SQR (x) + SQR (y);

      if (rsquare > 0)
        {
          gint     r0;
          gdouble  angle;
          gdouble  aspect;

          r0 = MIN (allocation.width, allocation.height) / 2;

          angle  = atan2 (y, x);
          aspect = 10.0 * sqrt ((gdouble) rsquare / (r0 * r0)) / 0.85;

          aspect = CLAMP (aspect, 1.0, 10.0);

          g_object_set (editor,
                        "blob-angle",  angle,
                        "blob-aspect", aspect,
                        NULL);
        }
    }
  else
    {
      GdkRectangle rect;
      gboolean     in_handle;

      ligma_blob_editor_get_handle (editor, &rect);

      if ((event->x >= rect.x) && (event->x - rect.x < rect.width) &&
          (event->y >= rect.y) && (event->y - rect.y < rect.height))
        {
          in_handle = TRUE;
        }
      else
        {
          in_handle = FALSE;
        }

      if (in_handle != editor->in_handle)
        {
          editor->in_handle = in_handle;

          gtk_widget_queue_draw (widget);
        }
    }

  return TRUE;
}

static void
ligma_blob_editor_get_handle (LigmaBlobEditor *editor,
                             GdkRectangle   *rect)
{
  GtkWidget     *widget = GTK_WIDGET (editor);
  GtkAllocation  allocation;
  gint           x, y;
  gint           r;

  gtk_widget_get_allocation (widget, &allocation);

  r = MIN (allocation.width, allocation.height) / 2;

  x = (allocation.width / 2 +
       0.85 * r *editor->aspect / 10.0 * cos (editor->angle));

  y = (allocation.height / 2 +
       0.85 * r * editor->aspect / 10.0 * sin (editor->angle));

  rect->x      = x - 5;
  rect->y      = y - 5;
  rect->width  = 10;
  rect->height = 10;
}

static void
ligma_blob_editor_draw_blob (LigmaBlobEditor *editor,
                            cairo_t        *cr,
                            gdouble         xc,
                            gdouble         yc,
                            gdouble         radius)
{
  GtkWidget       *widget   = GTK_WIDGET (editor);
  GtkStyleContext *style    = gtk_widget_get_style_context (widget);
  LigmaBlob        *blob;
  LigmaBlobFunc     function = ligma_blob_ellipse;
  GdkRGBA          color;
  gint             i;

  switch (editor->type)
    {
    case LIGMA_INK_BLOB_TYPE_CIRCLE:
      function = ligma_blob_ellipse;
      break;

    case LIGMA_INK_BLOB_TYPE_SQUARE:
      function = ligma_blob_square;
      break;

    case LIGMA_INK_BLOB_TYPE_DIAMOND:
      function = ligma_blob_diamond;
      break;
    }

  /*  to get a nice antialiased outline, render the blob at double size  */
  radius *= 2.0;
  blob = function (2.0 * xc, 2.0 * yc,
                   radius * cos (editor->angle),
                   radius * sin (editor->angle),
                   (- (radius / editor->aspect) * sin (editor->angle)),
                   (  (radius / editor->aspect) * cos (editor->angle)));

  for (i = 0; i < blob->height; i++)
    if (blob->data[i].left <= blob->data[i].right)
      {
        cairo_move_to (cr, blob->data[i].left / 2.0, (blob->y + i) / 2.0);
        break;
      }

  for (i = i + 1; i < blob->height; i++)
    {
      if (blob->data[i].left > blob->data[i].right)
        break;

      cairo_line_to (cr, blob->data[i].left / 2.0, (blob->y + i) / 2.0);
    }

  for (i = i - 1; i >= 0; i--)
    {
      if (blob->data[i].left > blob->data[i].right)
        break;

      cairo_line_to (cr, blob->data[i].right / 2.0, (blob->y + i) / 2.0);
    }

  cairo_close_path (cr);

  g_free (blob);

  gtk_style_context_get_color (style, gtk_widget_get_state_flags (widget),
                               &color);
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_fill (cr);
}
