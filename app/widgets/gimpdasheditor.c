/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadasheditor.c
 * Copyright (C) 2003 Simon Budig  <simon@ligma.org>
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
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"

#include "widgets-types.h"

#include "core/ligmadashpattern.h"
#include "core/ligmastrokeoptions.h"

#include "ligmadasheditor.h"


#define MIN_WIDTH          64
#define MIN_HEIGHT         20

#define DEFAULT_N_SEGMENTS 24


enum
{
  PROP_0,
  PROP_STROKE_OPTIONS,
  PROP_N_SEGMENTS,
  PROP_LENGTH
};


static void     ligma_dash_editor_finalize             (GObject        *object);
static void     ligma_dash_editor_set_property         (GObject        *object,
                                                       guint           property_id,
                                                       const GValue   *value,
                                                       GParamSpec     *pspec);
static void     ligma_dash_editor_get_property         (GObject        *object,
                                                       guint           property_id,
                                                       GValue         *value,
                                                       GParamSpec     *pspec);

static void     ligma_dash_editor_get_preferred_width  (GtkWidget      *widget,
                                                       gint           *minimum_width,
                                                       gint           *natural_width);
static void     ligma_dash_editor_get_preferred_height (GtkWidget      *widget,
                                                       gint           *minimum_height,
                                                       gint           *natural_height);
static gboolean ligma_dash_editor_draw                 (GtkWidget      *widget,
                                                       cairo_t        *cr);
static gboolean ligma_dash_editor_button_press         (GtkWidget      *widget,
                                                       GdkEventButton *bevent);
static gboolean ligma_dash_editor_button_release       (GtkWidget      *widget,
                                                       GdkEventButton *bevent);
static gboolean ligma_dash_editor_motion_notify        (GtkWidget      *widget,
                                                       GdkEventMotion *bevent);

/* helper function */
static void     update_segments_from_options          (LigmaDashEditor *editor);
static void     update_options_from_segments          (LigmaDashEditor *editor);
static void     update_blocksize                      (LigmaDashEditor *editor);
static gint     dash_x_to_index                       (LigmaDashEditor *editor,
                                                       gint            x);


G_DEFINE_TYPE (LigmaDashEditor, ligma_dash_editor, GTK_TYPE_DRAWING_AREA)

#define parent_class ligma_dash_editor_parent_class


static void
ligma_dash_editor_class_init (LigmaDashEditorClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize     = ligma_dash_editor_finalize;
  object_class->get_property = ligma_dash_editor_get_property;
  object_class->set_property = ligma_dash_editor_set_property;

  widget_class->get_preferred_width  = ligma_dash_editor_get_preferred_width;
  widget_class->get_preferred_height = ligma_dash_editor_get_preferred_height;
  widget_class->draw                 = ligma_dash_editor_draw;
  widget_class->button_press_event   = ligma_dash_editor_button_press;
  widget_class->button_release_event = ligma_dash_editor_button_release;
  widget_class->motion_notify_event  = ligma_dash_editor_motion_notify;

  g_object_class_install_property (object_class, PROP_STROKE_OPTIONS,
                                   g_param_spec_object ("stroke-options",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_STROKE_OPTIONS,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, PROP_N_SEGMENTS,
                                   g_param_spec_int ("n-segments",
                                                     NULL, NULL,
                                                     2, 120, DEFAULT_N_SEGMENTS,
                                                     LIGMA_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LENGTH,
                                   g_param_spec_double ("dash-length",
                                                        NULL, NULL,
                                                        0.0, 2000.0,
                                                        0.5 * DEFAULT_N_SEGMENTS,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT));
}

static void
ligma_dash_editor_init (LigmaDashEditor *editor)
{
  editor->segments       = NULL;
  editor->block_width    = 6;
  editor->block_height   = 6;
  editor->edit_mode      = TRUE;
  editor->edit_button_x0 = 0;

  gtk_widget_add_events (GTK_WIDGET (editor),
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON1_MOTION_MASK);
}

static void
ligma_dash_editor_finalize (GObject *object)
{
  LigmaDashEditor *editor = LIGMA_DASH_EDITOR (object);

  g_clear_object (&editor->stroke_options);
  g_clear_pointer (&editor->segments, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_dash_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  LigmaDashEditor *editor = LIGMA_DASH_EDITOR (object);

  switch (property_id)
    {
    case PROP_STROKE_OPTIONS:
      g_return_if_fail (editor->stroke_options == NULL);

      editor->stroke_options = g_value_dup_object (value);
      g_signal_connect_object (editor->stroke_options, "notify::dash-info",
                               G_CALLBACK (update_segments_from_options),
                               editor, G_CONNECT_SWAPPED);
      break;

    case PROP_N_SEGMENTS:
      editor->n_segments = g_value_get_int (value);

      if (editor->segments)
        g_free (editor->segments);
      editor->segments = g_new0 (gboolean, editor->n_segments);
      break;

    case PROP_LENGTH:
      editor->dash_length = g_value_get_double (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  update_segments_from_options (editor);
}

static void
ligma_dash_editor_get_property (GObject      *object,
                               guint         property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  LigmaDashEditor *editor = LIGMA_DASH_EDITOR (object);

  switch (property_id)
    {
    case PROP_STROKE_OPTIONS:
      g_value_set_object (value, editor->stroke_options);
      break;
    case PROP_N_SEGMENTS:
      g_value_set_int (value, editor->n_segments);
      break;
    case PROP_LENGTH:
      g_value_set_double (value, editor->dash_length);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_dash_editor_get_preferred_width (GtkWidget *widget,
                                      gint      *minimum_width,
                                      gint      *natural_width)
{
  LigmaDashEditor *editor = LIGMA_DASH_EDITOR (widget);

  *minimum_width = *natural_width = MAX (editor->block_width *
                                         editor->n_segments + 20,
                                         MIN_WIDTH);
}

static void
ligma_dash_editor_get_preferred_height (GtkWidget *widget,
                                       gint      *minimum_height,
                                       gint      *natural_height)
{
  LigmaDashEditor *editor = LIGMA_DASH_EDITOR (widget);

  *minimum_height = *natural_height = MAX (editor->block_height + 10,
                                           MIN_HEIGHT);
}

static gboolean
ligma_dash_editor_draw (GtkWidget *widget,
                       cairo_t   *cr)
{
  LigmaDashEditor  *editor = LIGMA_DASH_EDITOR (widget);
  GtkStyleContext *style  = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;
  GdkRGBA          fg_color;
  GdkRGBA          mid_color;
  gint             x;
  gint             w, h;

  gtk_widget_get_allocation (widget, &allocation);

  gtk_style_context_save (style);

  gtk_style_context_add_class (style, GTK_STYLE_CLASS_ENTRY);

  update_blocksize (editor);

  gtk_style_context_get_color (style,
                               gtk_widget_get_state_flags (widget),
                               &fg_color);

  mid_color        = fg_color;
  mid_color.alpha *= 0.5;

  /*  draw the background  */

  gtk_render_background (style, cr,
                         allocation.x, allocation.y,
                         allocation.width, allocation.height);

  w = editor->block_width;
  h = editor->block_height;

  editor->x0 = (allocation.width - w * editor->n_segments) / 2;
  editor->y0 = (allocation.height - h) / 2;

  /*  draw the dash segments  */

  x = editor->x0 % w;

  if (x > 0)
    x -= w;

  for (; x < editor->x0; x += w)
    {
      gint index = dash_x_to_index (editor, x);

      if (editor->segments[index])
        cairo_rectangle (cr, x, editor->y0, w, h);
    }

  gdk_cairo_set_source_rgba (cr, &mid_color);
  cairo_fill (cr);

  for (; x < editor->x0 + editor->n_segments * w; x += w)
    {
      gint index = dash_x_to_index (editor, x);

      if (editor->segments[index])
        cairo_rectangle (cr, x, editor->y0, w, h);
    }

  gdk_cairo_set_source_rgba (cr, &fg_color);
  cairo_fill (cr);

  for (; x < allocation.width + w; x += w)
    {
      gint index = dash_x_to_index (editor, x);

      if (editor->segments[index])
        cairo_rectangle (cr, x, editor->y0, w, h);
    }

  gdk_cairo_set_source_rgba (cr, &mid_color);
  cairo_fill (cr);

  /*  draw rulers  */

  x = editor->x0 % w;

  if (x > 0)
    x -= w;

  for (; x < allocation.width + w; x += w)
    {
      gint index = dash_x_to_index (editor, x);

      if (editor->n_segments % 4 == 0 &&
          (index + 1) % (editor->n_segments / 4) == 0)
        {
          cairo_move_to (cr, x + w - 0.5, editor->y0 - 2);
          cairo_line_to (cr, x + w - 0.5, editor->y0 + h + 2);
        }
      else if (index % 2 == 1)
        {
          cairo_move_to (cr, x + w - 0.5, editor->y0 + 1);
          cairo_line_to (cr, x + w - 0.5, editor->y0 + h - 1);
        }
      else
        {
          cairo_move_to (cr, x + w - 0.5, editor->y0 + h / 2 - 1);
          cairo_line_to (cr, x + w - 0.5, editor->y0 + h / 2 + 1);
        }
    }

  cairo_move_to (cr, editor->x0 - 0.5, editor->y0 - 1);
  cairo_move_to (cr, editor->x0 - 0.5, editor->y0 + h);

  gdk_cairo_set_source_rgba (cr, &mid_color);
  cairo_set_line_width (cr, 1.0);
  cairo_stroke (cr);

  gtk_style_context_restore (style);

  return FALSE;
}

static gboolean
ligma_dash_editor_button_press (GtkWidget      *widget,
                               GdkEventButton *bevent)
{
  LigmaDashEditor *editor = LIGMA_DASH_EDITOR (widget);
  gint            index;

  if (bevent->button == 1 && bevent->type == GDK_BUTTON_PRESS)
    {
      gtk_grab_add (widget);

      index = dash_x_to_index (editor, bevent->x);

      editor->edit_mode = ! editor->segments [index];
      editor->edit_button_x0 = bevent->x;

      editor->segments [index] = editor->edit_mode;

      gtk_widget_queue_draw (widget);
    }

  return TRUE;
}

static gboolean
ligma_dash_editor_button_release (GtkWidget      *widget,
                                 GdkEventButton *bevent)
{
  LigmaDashEditor *editor = LIGMA_DASH_EDITOR (widget);

  if (bevent->button == 1)
    {
      gtk_grab_remove (widget);

      update_options_from_segments (editor);
    }

  return TRUE;
}

static gboolean
ligma_dash_editor_motion_notify (GtkWidget      *widget,
                                GdkEventMotion *mevent)
{
  LigmaDashEditor *editor = LIGMA_DASH_EDITOR (widget);
  gint            x, index;

  index = dash_x_to_index (editor, mevent->x);
  editor->segments [index] = editor->edit_mode;

  if (mevent->x > editor->edit_button_x0)
    {
      for (x = editor->edit_button_x0; x < mevent->x; x += editor->block_width)
        {
          index = dash_x_to_index (editor, x);
          editor->segments[index] = editor->edit_mode;
        }
    }

  if (mevent->x < editor->edit_button_x0)
    {
      for (x = editor->edit_button_x0; x > mevent->x; x -= editor->block_width)
        {
          index = dash_x_to_index (editor, x);
          editor->segments[index] = editor->edit_mode;
        }
    }

  gtk_widget_queue_draw (widget);

  return TRUE;
}

GtkWidget *
ligma_dash_editor_new (LigmaStrokeOptions *stroke_options)
{
  g_return_val_if_fail (LIGMA_IS_STROKE_OPTIONS (stroke_options), NULL);

  return g_object_new (LIGMA_TYPE_DASH_EDITOR,
                       "stroke-options", stroke_options,
                       NULL);
}

void
ligma_dash_editor_shift_right (LigmaDashEditor *editor)
{
  gboolean swap;
  gint     i;

  g_return_if_fail (LIGMA_IS_DASH_EDITOR (editor));
  g_return_if_fail (editor->n_segments > 0);

  swap = editor->segments[editor->n_segments - 1];
  for (i = editor->n_segments - 1; i > 0; i--)
    editor->segments[i] = editor->segments[i-1];
  editor->segments[0] = swap;

  update_options_from_segments (editor);
}

void
ligma_dash_editor_shift_left (LigmaDashEditor *editor)
{
  gboolean swap;
  gint     i;

  g_return_if_fail (LIGMA_IS_DASH_EDITOR (editor));
  g_return_if_fail (editor->n_segments > 0);

  swap = editor->segments[0];
  for (i = 1; i < editor->n_segments; i++)
    editor->segments[i-1] = editor->segments[i];
  editor->segments[editor->n_segments - 1] = swap;

  update_options_from_segments (editor);
}

static void
update_segments_from_options (LigmaDashEditor *editor)
{
  GArray *dash_info;

  if (editor->stroke_options == NULL || editor->segments == NULL)
    return;

  g_return_if_fail (LIGMA_IS_STROKE_OPTIONS (editor->stroke_options));

  gtk_widget_queue_draw (GTK_WIDGET (editor));

  dash_info = ligma_stroke_options_get_dash_info (editor->stroke_options);

  ligma_dash_pattern_fill_segments (dash_info,
                                   editor->segments, editor->n_segments);
}

static void
update_options_from_segments (LigmaDashEditor *editor)
{
  GArray *pattern = ligma_dash_pattern_new_from_segments (editor->segments,
                                                         editor->n_segments,
                                                         editor->dash_length);

  ligma_stroke_options_take_dash_pattern (editor->stroke_options,
                                         LIGMA_DASH_CUSTOM, pattern);
}

static void
update_blocksize (LigmaDashEditor *editor)
{
  GtkWidget     *widget = GTK_WIDGET (editor);
  GtkAllocation  allocation;

  gtk_widget_get_allocation (widget, &allocation);

  editor->block_height = 6;

  editor->block_width = MAX (ROUND (editor->dash_length /
                                    editor->n_segments * editor->block_height),
                             4);
  editor->block_height = MIN (ROUND (((float) editor->block_width) *
                                     editor->n_segments / editor->dash_length),
                              allocation.height - 4);
}

static gint
dash_x_to_index (LigmaDashEditor *editor,
                 gint            x)
{
  gint index = x - editor->x0;

  while (index < 0)
    index += editor->n_segments * editor->block_width;

  index = (index / editor->block_width) % editor->n_segments;

  return index;
}
