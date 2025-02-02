/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatagentry.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "widgets-types.h"

#include "core/ligmacontainer.h"
#include "core/ligmacontext.h"
#include "core/ligmatag.h"
#include "core/ligmatagged.h"
#include "core/ligmataggedcontainer.h"
#include "core/ligmaviewable.h"

#include "ligmacombotagentry.h"
#include "ligmatagentry.h"
#include "ligmatagpopup.h"

#include "ligma-intl.h"


#define MENU_SCROLL_STEP1            8
#define MENU_SCROLL_STEP2           15
#define MENU_SCROLL_FAST_ZONE        8
#define MENU_SCROLL_TIMEOUT1        50
#define MENU_SCROLL_TIMEOUT2        20

#define LIGMA_TAG_POPUP_MARGIN        5
#define LIGMA_TAG_POPUP_PADDING       2
#define LIGMA_TAG_POPUP_LINE_SPACING  2

enum
{
  PROP_0,
  PROP_OWNER
};

struct _PopupTagData
{
  LigmaTag       *tag;
  GdkRectangle   bounds;
  GtkStateFlags  state_flags;
};


static void     ligma_tag_popup_constructed             (GObject        *object);
static void     ligma_tag_popup_dispose                 (GObject        *object);
static void     ligma_tag_popup_set_property            (GObject        *object,
                                                        guint           property_id,
                                                        const GValue   *value,
                                                        GParamSpec     *pspec);
static void     ligma_tag_popup_get_property            (GObject        *object,
                                                        guint           property_id,
                                                        GValue         *value,
                                                        GParamSpec     *pspec);

static void     ligma_tag_popup_get_border              (GtkWidget      *widget,
                                                        GtkBorder      *border);
static gboolean ligma_tag_popup_border_draw             (GtkWidget      *widget,
                                                        cairo_t        *cr,
                                                        LigmaTagPopup   *popup);
static void     ligma_tag_popup_list_style_updated      (GtkWidget      *widget,
                                                        LigmaTagPopup   *popup);
static gboolean ligma_tag_popup_list_draw               (GtkWidget      *widget,
                                                        cairo_t        *cr,
                                                        LigmaTagPopup   *popup);
static gboolean ligma_tag_popup_border_event            (GtkWidget      *widget,
                                                        GdkEvent       *event);
static gboolean ligma_tag_popup_list_event              (GtkWidget      *widget,
                                                        GdkEvent       *event,
                                                        LigmaTagPopup   *popup);
static gboolean ligma_tag_popup_is_in_tag               (PopupTagData   *tag_data,
                                                        gint            x,
                                                        gint            y);
static void     ligma_tag_popup_queue_draw_tag          (LigmaTagPopup   *widget,
                                                        PopupTagData   *tag_data);
static void     ligma_tag_popup_toggle_tag              (LigmaTagPopup   *popup,
                                                        PopupTagData   *tag_data);
static void     ligma_tag_popup_check_can_toggle        (LigmaTagged     *tagged,
                                                        LigmaTagPopup   *popup);
static gint     ligma_tag_popup_layout_tags             (LigmaTagPopup   *popup,
                                                        gint            width);
static gboolean ligma_tag_popup_scroll_timeout          (gpointer        data);
static void     ligma_tag_popup_remove_scroll_timeout   (LigmaTagPopup   *popup);
static gboolean ligma_tag_popup_scroll_timeout_initial  (gpointer        data);
static void     ligma_tag_popup_start_scrolling         (LigmaTagPopup   *popup);
static void     ligma_tag_popup_stop_scrolling          (LigmaTagPopup   *popup);
static void     ligma_tag_popup_scroll_by               (LigmaTagPopup   *popup,
                                                        gint            step,
                                                        const GdkEvent *event);
static void     ligma_tag_popup_handle_scrolling        (LigmaTagPopup   *popup,
                                                        gint            x,
                                                        gint            y,
                                                        gboolean        enter,
                                                        gboolean        motion);

static gboolean ligma_tag_popup_button_scroll           (LigmaTagPopup   *popup,
                                                        GdkEventButton *event);

static void     get_arrows_visible_area                (LigmaTagPopup   *combo_entry,
                                                        GdkRectangle   *border,
                                                        GdkRectangle   *upper,
                                                        GdkRectangle   *lower,
                                                        gint           *arrow_space);
static void     get_arrows_sensitive_area              (LigmaTagPopup   *popup,
                                                        GdkRectangle   *upper,
                                                        GdkRectangle   *lower);


G_DEFINE_TYPE (LigmaTagPopup, ligma_tag_popup, GTK_TYPE_WINDOW);

#define parent_class ligma_tag_popup_parent_class


static void
ligma_tag_popup_class_init (LigmaTagPopupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = ligma_tag_popup_constructed;
  object_class->dispose      = ligma_tag_popup_dispose;
  object_class->set_property = ligma_tag_popup_set_property;
  object_class->get_property = ligma_tag_popup_get_property;

  g_object_class_install_property (object_class, PROP_OWNER,
                                   g_param_spec_object ("owner", NULL, NULL,
                                                        LIGMA_TYPE_COMBO_TAG_ENTRY,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
ligma_tag_popup_init (LigmaTagPopup *popup)
{
  GtkWidget *widget = GTK_WIDGET (popup);

  popup->upper_arrow_state = GTK_STATE_NORMAL;
  popup->lower_arrow_state = GTK_STATE_NORMAL;

  gtk_widget_add_events (widget,
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_KEY_RELEASE_MASK    |
                         GDK_SCROLL_MASK         |
                         GDK_SMOOTH_SCROLL_MASK);

  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (popup)),
                               GTK_STYLE_CLASS_MENU);

  popup->frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (popup->frame), GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (popup), popup->frame);
  gtk_widget_show (popup->frame);

  popup->border_area = gtk_event_box_new ();
  gtk_container_add (GTK_CONTAINER (popup->frame), popup->border_area);
  gtk_widget_show (popup->border_area);

  popup->tag_area = gtk_drawing_area_new ();
  gtk_widget_set_halign (popup->border_area, GTK_ALIGN_FILL);
  gtk_widget_set_valign (popup->border_area, GTK_ALIGN_FILL);
  gtk_widget_add_events (popup->tag_area,
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_POINTER_MOTION_MASK);
  gtk_container_add (GTK_CONTAINER (popup->border_area), popup->tag_area);
  gtk_widget_show (popup->tag_area);

  g_signal_connect (popup->border_area, "draw",
                    G_CALLBACK (ligma_tag_popup_border_draw),
                    popup);
  g_signal_connect (popup, "event",
                    G_CALLBACK (ligma_tag_popup_border_event),
                    NULL);
  g_signal_connect (popup->tag_area, "style-updated",
                    G_CALLBACK (ligma_tag_popup_list_style_updated),
                    popup);
  g_signal_connect (popup->tag_area, "draw",
                    G_CALLBACK (ligma_tag_popup_list_draw),
                    popup);
  g_signal_connect (popup->tag_area, "event",
                    G_CALLBACK (ligma_tag_popup_list_event),
                    popup);
}

static void
ligma_tag_popup_constructed (GObject *object)
{
  LigmaTagPopup        *popup = LIGMA_TAG_POPUP (object);
  LigmaTaggedContainer *container;
  GtkWidget           *entry;
  GtkAllocation        entry_allocation;
  GtkBorder            popup_border;
  gint                 x;
  gint                 y;
  gint                 width;
  gint                 height;
  gint                 popup_height;
  GHashTable          *tag_hash;
  GList               *tag_list;
  GList               *tag_iterator;
  gint                 i;
  gint                 max_height;
  GdkRectangle         workarea;
  gchar              **current_tags;
  gint                 current_count;
  GdkRectangle         popup_rects[2]; /* variants of popup placement */
  GdkRectangle         popup_rect; /* best popup rect in screen coordinates */

  G_OBJECT_CLASS (parent_class)->constructed (object);

  entry = GTK_WIDGET (popup->combo_entry);

  gtk_window_set_screen (GTK_WINDOW (popup), gtk_widget_get_screen (entry));

  gtk_widget_get_allocation (entry, &entry_allocation);

  gtk_widget_style_get (GTK_WIDGET (popup),
                        "scroll-arrow-vlength", &popup->scroll_arrow_height,
                        NULL);

  current_tags  = ligma_tag_entry_parse_tags (LIGMA_TAG_ENTRY (popup->combo_entry));
  current_count = g_strv_length (current_tags);

  container = LIGMA_TAG_ENTRY (popup->combo_entry)->container;

  tag_hash = container->tag_ref_counts;
  tag_list = g_hash_table_get_keys (tag_hash);
  tag_list = g_list_sort (tag_list, ligma_tag_compare_func);

  popup->tag_count = g_list_length (tag_list);
  popup->tag_data  = g_new0 (PopupTagData, popup->tag_count);

  for (i = 0, tag_iterator = tag_list;
       i < popup->tag_count;
       i++, tag_iterator = g_list_next (tag_iterator))
    {
      PopupTagData *tag_data = &popup->tag_data[i];
      gint          j;

      tag_data->tag         = tag_iterator->data;
      tag_data->state_flags = 0;

      g_object_ref (tag_data->tag);

      for (j = 0; j < current_count; j++)
        {
          if (! ligma_tag_compare_with_string (tag_data->tag, current_tags[j]))
            {
              tag_data->state_flags = GTK_STATE_FLAG_SELECTED;
              break;
            }
        }
    }

  g_list_free (tag_list);
  g_strfreev (current_tags);

  if (LIGMA_TAG_ENTRY (popup->combo_entry)->mode == LIGMA_TAG_ENTRY_MODE_QUERY)
    {
      for (i = 0; i < popup->tag_count; i++)
        {
          if (! (popup->tag_data[i].state_flags & GTK_STATE_FLAG_SELECTED))
            {
              popup->tag_data[i].state_flags = GTK_STATE_FLAG_INSENSITIVE;
            }
        }

      ligma_container_foreach (LIGMA_CONTAINER (container),
                              (GFunc) ligma_tag_popup_check_can_toggle,
                              popup);
    }

  ligma_tag_popup_get_border (GTK_WIDGET (popup), &popup_border);

  width  = (entry_allocation.width -
            popup_border.left - popup_border.right);
  height = (ligma_tag_popup_layout_tags (popup, width) +
            popup_border.top + popup_border.bottom);

  gdk_window_get_origin (gtk_widget_get_window (entry), &x, &y);

  if (! gtk_widget_get_has_window (entry))
    {
      x += entry_allocation.x;
      y += entry_allocation.y;
    }

  max_height = entry_allocation.height * 10;

  gdk_monitor_get_workarea (ligma_widget_get_monitor (entry), &workarea);

  popup_height = MIN (height, max_height);

  popup_rects[0].x      = x;
  popup_rects[0].y      = 0;
  popup_rects[0].width  = entry_allocation.width;
  popup_rects[0].height = y + entry_allocation.height;

  popup_rects[1].x      = x;
  popup_rects[1].y      = y;
  popup_rects[1].width  = popup_rects[0].width;
  popup_rects[1].height = workarea.height - popup_rects[0].height;

  if (popup_rects[0].height >= popup_height)
    {
      popup_rect = popup_rects[0];
      popup_rect.y += popup_rects[0].height - popup_height;
      popup_rect.height = popup_height;
    }
  else if (popup_rects[1].height >= popup_height)
    {
      popup_rect = popup_rects[1];
      popup_rect.height = popup_height;
    }
  else
    {
      if (popup_rects[0].height >= popup_rects[1].height)
        {
          popup_rect = popup_rects[0];
          popup_rect.y += popup->scroll_arrow_height + popup_border.top;
        }
      else
        {
          popup_rect = popup_rects[1];
          popup_rect.y -= popup->scroll_arrow_height + popup_border.bottom;
        }

      popup_height = popup_rect.height;
    }

  if (popup_height < height)
    {
      popup->arrows_visible    = TRUE;
      popup->upper_arrow_state = GTK_STATE_INSENSITIVE;

      gtk_widget_set_margin_top    (popup->tag_area,
                                    popup->scroll_arrow_height + 2);
      gtk_widget_set_margin_bottom (popup->tag_area,
                                    popup->scroll_arrow_height + 2);

      popup_height -= 2 * popup->scroll_arrow_height + 4;

      popup->scroll_height = height - popup_height;
      popup->scroll_y      = 0;
      popup->scroll_step   = 0;
    }

  gtk_widget_set_size_request (popup->tag_area, width, popup_height);

  gtk_window_move (GTK_WINDOW (popup), popup_rect.x, popup_rect.y);
  gtk_window_resize (GTK_WINDOW (popup), popup_rect.width, popup_rect.height);
}

static void
ligma_tag_popup_dispose (GObject *object)
{
  LigmaTagPopup *popup = LIGMA_TAG_POPUP (object);

  ligma_tag_popup_remove_scroll_timeout (popup);

  g_clear_object (&popup->combo_entry);
  g_clear_object (&popup->layout);

  if (popup->tag_data)
    {
      gint i;

      for (i = 0; i < popup->tag_count; i++)
        {
          g_object_unref (popup->tag_data[i].tag);
        }

      g_clear_pointer (&popup->tag_data, g_free);
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_tag_popup_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  LigmaTagPopup *popup = LIGMA_TAG_POPUP (object);

  switch (property_id)
    {
    case PROP_OWNER:
      popup->combo_entry = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_tag_popup_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  LigmaTagPopup *popup = LIGMA_TAG_POPUP (object);

  switch (property_id)
    {
    case PROP_OWNER:
      g_value_set_object (value, popup->combo_entry);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

/**
 * ligma_tag_popup_new:
 * @combo_entry: #LigmaComboTagEntry which is owner of the popup window.
 *
 * Tag popup widget is only useful for for #LigmaComboTagEntry and
 * should not be used elsewhere.
 *
 * Returns: a newly created #LigmaTagPopup widget.
 **/
GtkWidget *
ligma_tag_popup_new (LigmaComboTagEntry *combo_entry)
{
  GtkWidget *toplevel;

  g_return_val_if_fail (LIGMA_IS_COMBO_TAG_ENTRY (combo_entry), NULL);

  toplevel = gtk_widget_get_toplevel (GTK_WIDGET (combo_entry));

  return g_object_new (LIGMA_TYPE_TAG_POPUP,
                       "type",          GTK_WINDOW_POPUP,
                       "owner",         combo_entry,
                       "transient-for", GTK_IS_WINDOW (toplevel) ? toplevel : NULL,
                       NULL);
}

/**
 * ligma_tag_popup_show:
 * @tag_popup:  an instance of #LigmaTagPopup
 *
 * Show tag popup widget. If mouse grab cannot be obtained for widget,
 * it is destroyed.
 **/
void
ligma_tag_popup_show (LigmaTagPopup *popup,
                     GdkEvent     *event)
{
  GtkWidget *widget;

  g_return_if_fail (LIGMA_IS_TAG_POPUP (popup));

  widget = GTK_WIDGET (popup);

  gtk_widget_show (widget);

  gtk_grab_add (widget);
  gtk_widget_grab_focus (widget);

  if (gdk_seat_grab (gdk_event_get_seat (event),
                     gtk_widget_get_window (widget),
                     GDK_SEAT_CAPABILITY_ALL,
                     TRUE, NULL, event, NULL, NULL) != GDK_GRAB_SUCCESS)
    {
      /* pointer grab must be attained otherwise user would have
       * problems closing the popup window.
       */
      gtk_grab_remove (widget);
      gtk_widget_destroy (widget);
    }
}

static gint
ligma_tag_popup_layout_tags (LigmaTagPopup *popup,
                            gint          width)
{
  PangoFontMetrics *font_metrics;
  PangoContext     *context;
  gint              x;
  gint              y;
  gint              height = 0;
  gint              i;
  gint              line_height;
  gint              space_width;

  x = LIGMA_TAG_POPUP_MARGIN;
  y = LIGMA_TAG_POPUP_MARGIN;

  context = gtk_widget_get_pango_context (popup->tag_area);

  if (! popup->layout)
    popup->layout = pango_layout_new (context);

  font_metrics = pango_context_get_metrics (context,
                                            pango_context_get_font_description (context),
                                            NULL);

  line_height = PANGO_PIXELS ((pango_font_metrics_get_ascent (font_metrics) +
                               pango_font_metrics_get_descent (font_metrics)));
  space_width = PANGO_PIXELS (pango_font_metrics_get_approximate_char_width (font_metrics));

  pango_font_metrics_unref (font_metrics);

  for (i = 0; i < popup->tag_count; i++)
    {
      PopupTagData   *tag_data = &popup->tag_data[i];
      PangoRectangle  ink;
      PangoRectangle  logical;

      pango_layout_set_text (popup->layout,
                             ligma_tag_get_name (tag_data->tag), -1);
      pango_layout_get_pixel_extents (popup->layout, &ink, &logical);

      tag_data->bounds.width  = MAX (ink.width,  logical.width);
      tag_data->bounds.height = MAX (ink.height, logical.height);

      tag_data->bounds.width  += 2 * LIGMA_TAG_POPUP_PADDING;
      tag_data->bounds.height += 2 * LIGMA_TAG_POPUP_PADDING;

      if (x + space_width + tag_data->bounds.width +
          LIGMA_TAG_POPUP_MARGIN - 1 > width)
        {
          x = LIGMA_TAG_POPUP_MARGIN;
          y += line_height + 2 * LIGMA_TAG_POPUP_PADDING + LIGMA_TAG_POPUP_LINE_SPACING;
        }

      tag_data->bounds.x = x;
      tag_data->bounds.y = y;

      x += tag_data->bounds.width + space_width;
    }

  if (gtk_widget_get_direction (GTK_WIDGET (popup)) == GTK_TEXT_DIR_RTL)
    {
      for (i = 0; i < popup->tag_count; i++)
        {
          PopupTagData *tag_data = &popup->tag_data[i];

          tag_data->bounds.x = (width -
                                tag_data->bounds.x -
                                tag_data->bounds.width);
        }
    }

  height = y + line_height + LIGMA_TAG_POPUP_MARGIN;

  return height;
}

static void
ligma_tag_popup_get_border (GtkWidget *widget,
                           GtkBorder *border)
{
  GtkStyleContext *context;
  GtkStateFlags    state;
  GtkBorder        padding, border_width;

  context = gtk_widget_get_style_context (widget);
  state = gtk_widget_get_state_flags (widget);

  gtk_style_context_get_padding (context, state, &padding);
  gtk_style_context_get_border (context, state, &border_width);

  border->left   = border_width.left + padding.left;
  border->right  = border_width.right + padding.right;
  border->top    = border_width.top + padding.top;
  border->bottom = border_width.bottom + padding.bottom;
}

static void
ligma_tag_popup_list_style_updated (GtkWidget    *widget,
                                   LigmaTagPopup *popup)
{
  g_clear_object (&popup->layout);
}

static gboolean
ligma_tag_popup_border_draw (GtkWidget    *widget,
                            cairo_t      *cr,
                            LigmaTagPopup *popup)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GdkRectangle     border;
  GdkRectangle     upper;
  GdkRectangle     lower;
  GtkBorder        popup_border;
  gint             arrow_space;
  gint             arrow_size;

  if (! gtk_cairo_should_draw_window (cr, gtk_widget_get_window (widget)))
    return FALSE;

  get_arrows_visible_area (popup, &border, &upper, &lower, &arrow_space);
  ligma_tag_popup_get_border (widget, &popup_border);

  arrow_size = 0.7 * arrow_space;

  gtk_render_background (style, cr, 0, 0,
                         gtk_widget_get_allocated_width (widget),
                         gtk_widget_get_allocated_height (widget));
  gtk_render_frame (style, cr, 0, 0,
                    gtk_widget_get_allocated_width (widget),
                    gtk_widget_get_allocated_height (widget));

  if (popup->arrows_visible)
    {
      gtk_style_context_save (style);

      /*  upper arrow  */

      gtk_style_context_set_state (style, popup->upper_arrow_state);

      gtk_render_background (style, cr,
                             upper.x, upper.y,
                             upper.width, upper.height);
      gtk_render_frame (style, cr,
                        upper.x, upper.y,
                        upper.width, upper.height);

      gtk_render_arrow (style, cr, 0,
                        upper.x + (upper.width - arrow_size) / 2,
                        upper.y + popup_border.top + (arrow_space - arrow_size) / 2,
                        arrow_size);

      /*  lower arrow  */

      gtk_style_context_set_state (style, popup->lower_arrow_state);

      gtk_render_background (style, cr,
                             lower.x, lower.y,
                             lower.width, lower.height);
      gtk_render_frame (style, cr,
                        lower.x, lower.y,
                        lower.width, lower.height);

      gtk_render_arrow (style, cr, G_PI,
                        lower.x + (lower.width - arrow_size) / 2,
                        lower.y + popup_border.top + (arrow_space - arrow_size) / 2,
                        arrow_size);

      gtk_style_context_restore (style);
    }

  return FALSE;
}

static gboolean
ligma_tag_popup_border_event (GtkWidget *widget,
                             GdkEvent  *event)
{
  LigmaTagPopup *popup = LIGMA_TAG_POPUP (widget);

  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *button_event = (GdkEventButton *) event;
      GtkAllocation   allocation;
      gint            x;
      gint            y;

      if (button_event->window == gtk_widget_get_window (widget) &&
          ligma_tag_popup_button_scroll (popup, button_event))
        {
          return TRUE;
        }

      gtk_widget_get_allocation (widget, &allocation);

      gdk_window_get_device_position (gtk_widget_get_window (widget),
                                      gdk_event_get_device (event),
                                      &x, &y, NULL);

      if (button_event->window != gtk_widget_get_window (popup->tag_area) &&
          (x < allocation.y                    ||
           y < allocation.x                    ||
           x > allocation.x + allocation.width ||
           y > allocation.y + allocation.height))
        {
          /* user has clicked outside the popup area,
           * which means it should be hidden.
           */
          gtk_grab_remove (widget);
          gdk_seat_ungrab (gdk_event_get_seat (event));
          gtk_widget_destroy (widget);
        }
    }
  else if (event->type == GDK_MOTION_NOTIFY)
    {
      GdkEventMotion *motion_event = (GdkEventMotion *) event;
      gint            x, y;

      gdk_window_get_device_position (gtk_widget_get_window (widget),
                                      gdk_event_get_device (event),
                                      &x, &y, NULL);

      ligma_tag_popup_handle_scrolling (popup, x, y,
                                       motion_event->window ==
                                       gtk_widget_get_window (widget),
                                       TRUE);
    }
  else if (event->type == GDK_BUTTON_RELEASE)
    {
      GdkEventButton *button_event = (GdkEventButton *) event;

      popup->single_select_disabled = TRUE;

      if (button_event->window == gtk_widget_get_window (widget) &&
          ligma_tag_popup_button_scroll (popup, button_event))
        {
          return TRUE;
        }
    }
  else if (event->type == GDK_GRAB_BROKEN)
    {
      gtk_grab_remove (widget);
      gdk_seat_ungrab (gdk_event_get_seat (event));
      gtk_widget_destroy (widget);
    }
  else if (event->type == GDK_KEY_PRESS)
    {
      gtk_grab_remove (widget);
      gdk_seat_ungrab (gdk_event_get_seat (event));
      gtk_widget_destroy (widget);
    }
  else if (event->type == GDK_SCROLL)
    {
      ligma_tag_popup_scroll_by (popup, 0, event);
      return TRUE;
    }

  return FALSE;
}

static gboolean
ligma_tag_popup_list_draw (GtkWidget    *widget,
                          cairo_t      *cr,
                          LigmaTagPopup *popup)
{
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  gint            i;

  if (! popup->layout)
    {
      GtkBorder     popup_border;
      GtkAllocation entry_allocation;
      gint          width;

      ligma_tag_popup_get_border (GTK_WIDGET (popup), &popup_border);

      gtk_widget_get_allocation (GTK_WIDGET (popup->combo_entry),
                                 &entry_allocation);

      width = (entry_allocation.width -
               popup_border.left - popup_border.right);

      ligma_tag_popup_layout_tags (popup, width);
    }

  for (i = 0; i < popup->tag_count; i++)
    {
      PopupTagData *tag_data = &popup->tag_data[i];

      gtk_style_context_save (style);

      pango_layout_set_text (popup->layout,
                             ligma_tag_get_name (tag_data->tag), -1);

      if (tag_data->state_flags & GTK_STATE_FLAG_SELECTED)
        {
          gtk_style_context_add_class (style, GTK_STYLE_CLASS_VIEW);
          gtk_style_context_set_state (style, GTK_STATE_FLAG_SELECTED);
        }
      else if (tag_data->state_flags & GTK_STATE_FLAG_INSENSITIVE)
        {
          gtk_style_context_add_class (style, GTK_STYLE_CLASS_VIEW);
          gtk_style_context_set_state (style, GTK_STATE_FLAG_INSENSITIVE);
        }

      if (tag_data == popup->prelight &&
          ! (tag_data->state_flags & GTK_STATE_FLAG_INSENSITIVE))
        {
          PangoAttribute  *attribute;
          PangoAttrList   *attributes;

          attributes = pango_attr_list_new ();

          attribute = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
          pango_attr_list_insert (attributes, attribute);

          pango_layout_set_attributes (popup->layout, attributes);
          pango_attr_list_unref (attributes);
        }
      else
        {
          pango_layout_set_attributes (popup->layout, NULL);
        }

      if (tag_data->state_flags & (GTK_STATE_FLAG_SELECTED |
                                   GTK_STATE_FLAG_INSENSITIVE))
        {
          gtk_render_background (style, cr,
                                 tag_data->bounds.x,
                                 tag_data->bounds.y - popup->scroll_y,
                                 tag_data->bounds.width,
                                 tag_data->bounds.height);
        }

      gtk_render_layout (style, cr,
                         (tag_data->bounds.x +
                          LIGMA_TAG_POPUP_PADDING),
                         (tag_data->bounds.y -
                          popup->scroll_y +
                          LIGMA_TAG_POPUP_PADDING),
                         popup->layout);

      if (tag_data == popup->prelight                            &&
          ! (tag_data->state_flags & GTK_STATE_FLAG_INSENSITIVE) &&
          ! popup->single_select_disabled)
        {
          gtk_render_focus (style, cr,
                            tag_data->bounds.x,
                            tag_data->bounds.y - popup->scroll_y,
                            tag_data->bounds.width,
                            tag_data->bounds.height);
        }

      gtk_style_context_restore (style);
    }

  return FALSE;
}

static gboolean
ligma_tag_popup_list_event (GtkWidget    *widget,
                           GdkEvent     *event,
                           LigmaTagPopup *popup)
{
  if (event->type == GDK_BUTTON_PRESS)
    {
      GdkEventButton *button_event = (GdkEventButton *) event;
      gint            x;
      gint            y;
      gint            i;

      popup->single_select_disabled = TRUE;

      x = button_event->x;
      y = button_event->y + popup->scroll_y;

      for (i = 0; i < popup->tag_count; i++)
        {
          PopupTagData *tag_data = &popup->tag_data[i];

          if (ligma_tag_popup_is_in_tag (tag_data, x, y))
            {
              ligma_tag_popup_toggle_tag (popup, tag_data);
              gtk_widget_queue_draw (widget);
              break;
            }
        }
    }
  else if (event->type == GDK_MOTION_NOTIFY)
    {
      GdkEventMotion *motion_event = (GdkEventMotion *) event;
      PopupTagData   *prelight     = NULL;
      gint            x;
      gint            y;
      gint            i;

      x = motion_event->x;
      y = motion_event->y + popup->scroll_y;

      for (i = 0; i < popup->tag_count; i++)
        {
          PopupTagData *tag_data = &popup->tag_data[i];

          if (ligma_tag_popup_is_in_tag (tag_data, x, y))
            {
              prelight = tag_data;
              break;
            }
        }

      if (prelight != popup->prelight)
        {
          if (popup->prelight)
            ligma_tag_popup_queue_draw_tag (popup, popup->prelight);

          popup->prelight = prelight;

          if (popup->prelight)
            ligma_tag_popup_queue_draw_tag (popup, popup->prelight);
        }
    }
  else if (event->type == GDK_BUTTON_RELEASE &&
           ! popup->single_select_disabled)
    {
      GdkEventButton *button_event = (GdkEventButton *) event;
      gint            x;
      gint            y;
      gint            i;

      popup->single_select_disabled = TRUE;

      x = button_event->x;
      y = button_event->y + popup->scroll_y;

      for (i = 0; i < popup->tag_count; i++)
        {
          PopupTagData *tag_data = &popup->tag_data[i];

          if (ligma_tag_popup_is_in_tag (tag_data, x, y))
            {
              ligma_tag_popup_toggle_tag (popup, tag_data);
              gtk_widget_destroy (GTK_WIDGET (popup));
              break;
            }
        }
    }

  return FALSE;
}

static gboolean
ligma_tag_popup_is_in_tag (PopupTagData *tag_data,
                          gint          x,
                          gint          y)
{
  if (x >= tag_data->bounds.x                          &&
      y >= tag_data->bounds.y                          &&
      x <  tag_data->bounds.x + tag_data->bounds.width &&
      y <  tag_data->bounds.y + tag_data->bounds.height)
    {
      return TRUE;
    }

  return FALSE;
}

static void
ligma_tag_popup_queue_draw_tag (LigmaTagPopup *popup,
                               PopupTagData *tag_data)
{
  gtk_widget_queue_draw_area (popup->tag_area,
                              tag_data->bounds.x,
                              tag_data->bounds.y - popup->scroll_y,
                              tag_data->bounds.width,
                              tag_data->bounds.height);
}

static void
ligma_tag_popup_toggle_tag (LigmaTagPopup *popup,
                           PopupTagData *tag_data)
{
  gchar    **current_tags;
  GString   *tag_str;
  gint       length;
  gint       i;
  gboolean   tag_toggled_off = FALSE;

  if (tag_data->state_flags & GTK_STATE_FLAG_SELECTED)
    {
      tag_data->state_flags = 0;
    }
  else if (! (tag_data->state_flags & GTK_STATE_FLAG_INSENSITIVE))
    {
      tag_data->state_flags = GTK_STATE_FLAG_SELECTED;
    }
  else
    {
      return;
    }

  current_tags = ligma_tag_entry_parse_tags (LIGMA_TAG_ENTRY (popup->combo_entry));
  tag_str = g_string_new ("");
  length = g_strv_length (current_tags);
  for (i = 0; i < length; i++)
    {
      if (! ligma_tag_compare_with_string (tag_data->tag, current_tags[i]))
        {
          tag_toggled_off = TRUE;
        }
      else
        {
          if (tag_str->len)
            {
              g_string_append (tag_str, ligma_tag_entry_get_separator ());
              g_string_append_c (tag_str, ' ');
            }

          g_string_append (tag_str, current_tags[i]);
        }
    }

  if (! tag_toggled_off)
    {
      /* this tag was not selected yet, so it needs to be toggled on */

      if (tag_str->len)
        {
          g_string_append (tag_str, ligma_tag_entry_get_separator ());
          g_string_append_c (tag_str, ' ');
        }

      g_string_append (tag_str, ligma_tag_get_name (tag_data->tag));
    }

  ligma_tag_entry_set_tag_string (LIGMA_TAG_ENTRY (popup->combo_entry),
                                 tag_str->str);

  g_string_free (tag_str, TRUE);
  g_strfreev (current_tags);

  if (LIGMA_TAG_ENTRY (popup->combo_entry)->mode == LIGMA_TAG_ENTRY_MODE_QUERY)
    {
      LigmaTaggedContainer *container;

      container = LIGMA_TAG_ENTRY (popup->combo_entry)->container;

      for (i = 0; i < popup->tag_count; i++)
        {
          if (! (popup->tag_data[i].state_flags & GTK_STATE_FLAG_SELECTED))
            {
              popup->tag_data[i].state_flags |= GTK_STATE_FLAG_INSENSITIVE;
            }
        }

      ligma_container_foreach (LIGMA_CONTAINER (container),
                              (GFunc) ligma_tag_popup_check_can_toggle,
                              popup);
    }
}

static int
ligma_tag_popup_data_compare (const void *a,
                             const void *b)
{
  return ligma_tag_compare_func (((PopupTagData *) a)->tag,
                                ((PopupTagData *) b)->tag);
}

static void
ligma_tag_popup_check_can_toggle (LigmaTagged   *tagged,
                                 LigmaTagPopup *popup)
{
  GList *iterator;

  for (iterator = ligma_tagged_get_tags (tagged);
       iterator;
       iterator = g_list_next (iterator))
    {
      PopupTagData  search_key;
      PopupTagData *search_result;

      search_key.tag = iterator->data;

      search_result =
        (PopupTagData *) bsearch (&search_key,
                                  popup->tag_data, popup->tag_count,
                                  sizeof (PopupTagData),
                                  ligma_tag_popup_data_compare);

      if (search_result)
        {
          if (search_result->state_flags & GTK_STATE_FLAG_INSENSITIVE)
            {
              search_result->state_flags &= ~GTK_STATE_FLAG_INSENSITIVE;
            }
        }
    }
}

static gboolean
ligma_tag_popup_scroll_timeout (gpointer data)
{
  LigmaTagPopup *popup = data;
  gboolean      touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  ligma_tag_popup_scroll_by (popup, popup->scroll_step, NULL);

  return TRUE;
}

static void
ligma_tag_popup_remove_scroll_timeout (LigmaTagPopup *popup)
{
  if (popup->scroll_timeout_id)
    {
      g_source_remove (popup->scroll_timeout_id);
      popup->scroll_timeout_id = 0;
    }
}

static gboolean
ligma_tag_popup_scroll_timeout_initial (gpointer data)
{
  LigmaTagPopup *popup = data;
  guint         timeout;
  gboolean      touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-timeout-repeat",   &timeout,
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  ligma_tag_popup_scroll_by (popup, popup->scroll_step, NULL);

  ligma_tag_popup_remove_scroll_timeout (popup);

  popup->scroll_timeout_id =
    gdk_threads_add_timeout (timeout,
                             ligma_tag_popup_scroll_timeout,
                             popup);

  return FALSE;
}

static void
ligma_tag_popup_start_scrolling (LigmaTagPopup *popup)
{
  guint    timeout;
  gboolean touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-timeout-repeat",   &timeout,
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  ligma_tag_popup_scroll_by (popup, popup->scroll_step, NULL);

  popup->scroll_timeout_id =
    gdk_threads_add_timeout (timeout,
                             ligma_tag_popup_scroll_timeout_initial,
                             popup);
}

static void
ligma_tag_popup_stop_scrolling (LigmaTagPopup *popup)
{
  gboolean touchscreen_mode;

  ligma_tag_popup_remove_scroll_timeout (popup);

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  if (! touchscreen_mode)
    {
      popup->upper_arrow_prelight = FALSE;
      popup->lower_arrow_prelight = FALSE;
    }
}

static void
ligma_tag_popup_scroll_by (LigmaTagPopup   *popup,
                          gint            step,
                          const GdkEvent *event)
{
  GtkStateType arrow_state;
  gint         new_scroll_y = popup->scroll_y;

  /* If event is set, we override the step value. */
  if (event)
    {
      GdkEventScroll *scroll_event = (GdkEventScroll *) event;
      gdouble         delta_x;
      gdouble         delta_y;

      switch (scroll_event->direction)
        {
        case GDK_SCROLL_RIGHT:
        case GDK_SCROLL_DOWN:
          if (popup->smooth_scrolling)
            /* In some case, we get both a SMOOTH event and step events
             * (right, left, up, down). If we process them all, we get
             * some fluid scrolling with regular bumps, which feels
             * buggy. So when smooth scrolling is in progress, we just
             * skip step events until we receive the stop scroll event.
             */
            return;

          step = MENU_SCROLL_STEP2;
          break;

        case GDK_SCROLL_LEFT:
        case GDK_SCROLL_UP:
          if (popup->smooth_scrolling)
            return;

          step = - MENU_SCROLL_STEP2;
          break;

        case GDK_SCROLL_SMOOTH:
          if (gdk_event_get_scroll_deltas (event, &delta_x, &delta_y))
            {
              popup->smooth_scrolling = TRUE;
              step = 0;

              if (delta_x < 0.0 || delta_y < 0.0)
                step = (gint) (MIN (delta_x, delta_y) * MENU_SCROLL_STEP2);
              else if (delta_x > 0.0 || delta_y > 0.0)
                step = (gint) (MAX (delta_x, delta_y) * MENU_SCROLL_STEP2);
            }
          break;
        }

      if (gdk_event_is_scroll_stop_event (event))
        popup->smooth_scrolling = FALSE;
    }

  if (step == 0)
    return;

  new_scroll_y += step;
  arrow_state = popup->upper_arrow_state;

  if (new_scroll_y < 0)
    {
      new_scroll_y = 0;

      if (arrow_state != GTK_STATE_INSENSITIVE)
        ligma_tag_popup_stop_scrolling (popup);

      arrow_state = GTK_STATE_INSENSITIVE;
    }
  else
    {
      arrow_state = (popup->upper_arrow_prelight ?
                     GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);
    }

  if (arrow_state != popup->upper_arrow_state)
    {
      popup->upper_arrow_state = arrow_state;
      gtk_widget_queue_draw (GTK_WIDGET (popup));
    }

  arrow_state = popup->lower_arrow_state;

  if (new_scroll_y >= popup->scroll_height)
    {
      new_scroll_y = popup->scroll_height - 1;

      if (arrow_state != GTK_STATE_INSENSITIVE)
        ligma_tag_popup_stop_scrolling (popup);

      arrow_state = GTK_STATE_INSENSITIVE;
    }
  else
    {
      arrow_state = (popup->lower_arrow_prelight ?
                     GTK_STATE_PRELIGHT : GTK_STATE_NORMAL);
    }

  if (arrow_state != popup->lower_arrow_state)
    {
      popup->lower_arrow_state = arrow_state;
      gtk_widget_queue_draw (GTK_WIDGET (popup));
    }

  if (new_scroll_y != popup->scroll_y)
    {
      popup->scroll_y = new_scroll_y;

      gdk_window_scroll (gtk_widget_get_window (popup->tag_area), 0, -step);
    }
}

static void
ligma_tag_popup_handle_scrolling (LigmaTagPopup *popup,
                                 gint          x,
                                 gint          y,
                                 gboolean      enter,
                                 gboolean      motion)
{
  GdkRectangle rect;
  gboolean     in_arrow;
  gboolean     scroll_fast = FALSE;
  gboolean     touchscreen_mode;

  g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                "gtk-touchscreen-mode", &touchscreen_mode,
                NULL);

  /*  upper arrow handling  */

  get_arrows_sensitive_area (popup, &rect, NULL);

  in_arrow = FALSE;
  if (popup->arrows_visible    &&
      x >= rect.x              &&
      x <  rect.x + rect.width &&
      y >= rect.y              &&
      y <  rect.y + rect.height)
    {
      in_arrow = TRUE;
    }

  if (touchscreen_mode)
    popup->upper_arrow_prelight = in_arrow;

  if (popup->upper_arrow_state != GTK_STATE_INSENSITIVE)
    {
      gboolean arrow_pressed = FALSE;

      if (popup->arrows_visible)
        {
          if (touchscreen_mode)
            {
              if (enter && popup->upper_arrow_prelight)
                {
                  if (popup->scroll_timeout_id == 0)
                    {
                      ligma_tag_popup_remove_scroll_timeout (popup);
                      popup->scroll_step = -MENU_SCROLL_STEP2; /* always fast */

                      if (! motion)
                        {
                          /* Only do stuff on click. */
                          ligma_tag_popup_start_scrolling (popup);
                          arrow_pressed = TRUE;
                        }
                    }
                  else
                    {
                      arrow_pressed = TRUE;
                    }
                }
              else if (! enter)
                {
                  ligma_tag_popup_stop_scrolling (popup);
                }
            }
          else /* !touchscreen_mode */
            {
              scroll_fast = (y < rect.y + MENU_SCROLL_FAST_ZONE);

              if (enter && in_arrow &&
                  (! popup->upper_arrow_prelight ||
                   popup->scroll_fast != scroll_fast))
                {
                  popup->upper_arrow_prelight = TRUE;
                  popup->scroll_fast          = scroll_fast;

                  ligma_tag_popup_remove_scroll_timeout (popup);
                  popup->scroll_step = (scroll_fast ?
                                        -MENU_SCROLL_STEP2 : -MENU_SCROLL_STEP1);

                  popup->scroll_timeout_id =
                    gdk_threads_add_timeout (scroll_fast ?
                                             MENU_SCROLL_TIMEOUT2 :
                                             MENU_SCROLL_TIMEOUT1,
                                             ligma_tag_popup_scroll_timeout,
                                             popup);
                }
              else if (! enter && ! in_arrow && popup->upper_arrow_prelight)
                {
                  ligma_tag_popup_stop_scrolling (popup);
                }
            }
        }

      /*  ligma_tag_popup_start_scrolling() might have hit the top of the
       *  tag_popup, so check if the button isn't insensitive before
       *  changing it to something else.
       */
      if (popup->upper_arrow_state != GTK_STATE_INSENSITIVE)
        {
          GtkStateType arrow_state = GTK_STATE_NORMAL;

          if (arrow_pressed)
            arrow_state = GTK_STATE_ACTIVE;
          else if (popup->upper_arrow_prelight)
            arrow_state = GTK_STATE_PRELIGHT;

          if (arrow_state != popup->upper_arrow_state)
            {
              popup->upper_arrow_state = arrow_state;

              gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET (popup)),
                                          &rect, FALSE);
            }
        }
    }

  /*  lower arrow handling  */

  get_arrows_sensitive_area (popup, NULL, &rect);

  in_arrow = FALSE;
  if (popup->arrows_visible    &&
      x >= rect.x              &&
      x <  rect.x + rect.width &&
      y >= rect.y              &&
      y <  rect.y + rect.height)
    {
      in_arrow = TRUE;
    }

  if (touchscreen_mode)
    popup->lower_arrow_prelight = in_arrow;

  if (popup->lower_arrow_state != GTK_STATE_INSENSITIVE)
    {
      gboolean arrow_pressed = FALSE;

      if (popup->arrows_visible)
        {
          if (touchscreen_mode)
            {
              if (enter && popup->lower_arrow_prelight)
                {
                  if (popup->scroll_timeout_id == 0)
                    {
                      ligma_tag_popup_remove_scroll_timeout (popup);
                      popup->scroll_step = MENU_SCROLL_STEP2; /* always fast */

                      if (! motion)
                        {
                          /* Only do stuff on click. */
                          ligma_tag_popup_start_scrolling (popup);
                          arrow_pressed = TRUE;
                        }
                    }
                  else
                    {
                      arrow_pressed = TRUE;
                    }
                }
              else if (! enter)
                {
                  ligma_tag_popup_stop_scrolling (popup);
                }
            }
          else /* !touchscreen_mode */
            {
              scroll_fast = (y > rect.y + rect.height - MENU_SCROLL_FAST_ZONE);

              if (enter && in_arrow &&
                  (! popup->lower_arrow_prelight ||
                   popup->scroll_fast != scroll_fast))
                {
                  popup->lower_arrow_prelight = TRUE;
                  popup->scroll_fast          = scroll_fast;

                  ligma_tag_popup_remove_scroll_timeout (popup);
                  popup->scroll_step = (scroll_fast ?
                                        MENU_SCROLL_STEP2 : MENU_SCROLL_STEP1);

                  popup->scroll_timeout_id =
                    gdk_threads_add_timeout (scroll_fast ?
                                             MENU_SCROLL_TIMEOUT2 :
                                             MENU_SCROLL_TIMEOUT1,
                                             ligma_tag_popup_scroll_timeout,
                                             popup);
                }
              else if (! enter && ! in_arrow && popup->lower_arrow_prelight)
                {
                  ligma_tag_popup_stop_scrolling (popup);
                }
            }
        }

      /*  ligma_tag_popup_start_scrolling() might have hit the bottom of the
       *  popup, so check if the button isn't insensitive before
       *  changing it to something else.
       */
      if (popup->lower_arrow_state != GTK_STATE_INSENSITIVE)
        {
          GtkStateType arrow_state = GTK_STATE_NORMAL;

          if (arrow_pressed)
            arrow_state = GTK_STATE_ACTIVE;
          else if (popup->lower_arrow_prelight)
            arrow_state = GTK_STATE_PRELIGHT;

          if (arrow_state != popup->lower_arrow_state)
            {
              popup->lower_arrow_state = arrow_state;

              gdk_window_invalidate_rect (gtk_widget_get_window (GTK_WIDGET (popup)),
                                          &rect, FALSE);
            }
        }
    }
}

static gboolean
ligma_tag_popup_button_scroll (LigmaTagPopup   *popup,
                              GdkEventButton *event)
{
  if (popup->upper_arrow_prelight || popup->lower_arrow_prelight)
    {
      gboolean touchscreen_mode;

      g_object_get (gtk_widget_get_settings (GTK_WIDGET (popup)),
                    "gtk-touchscreen-mode", &touchscreen_mode,
                    NULL);

      if (touchscreen_mode)
        ligma_tag_popup_handle_scrolling (popup,
                                         event->x_root,
                                         event->y_root,
                                         event->type == GDK_BUTTON_PRESS,
                                         FALSE);

      return TRUE;
    }

  return FALSE;
}

static void
get_arrows_visible_area (LigmaTagPopup *popup,
                         GdkRectangle *border,
                         GdkRectangle *upper,
                         GdkRectangle *lower,
                         gint         *arrow_space)
{
  gint padding_top    = gtk_widget_get_margin_top    (popup->tag_area);
  gint padding_bottom = gtk_widget_get_margin_bottom (popup->tag_area);
  gint padding_left   = gtk_widget_get_margin_start  (popup->tag_area);
  gint padding_right  = gtk_widget_get_margin_end    (popup->tag_area);

  gtk_widget_get_allocation (popup->border_area, border);

  upper->x      = border->x + padding_left;
  upper->y      = border->y;
  upper->width  = border->width - padding_left - padding_right;
  upper->height = padding_top;

  lower->x      = border->x + padding_left;
  lower->y      = border->y + border->height - padding_bottom;
  lower->width  = border->width - padding_left - padding_right;
  lower->height = padding_bottom;

  *arrow_space = popup->scroll_arrow_height;
}

static void
get_arrows_sensitive_area (LigmaTagPopup *popup,
                           GdkRectangle *upper,
                           GdkRectangle *lower)
{
  GdkRectangle tmp_border;
  GdkRectangle tmp_upper;
  GdkRectangle tmp_lower;
  gint         tmp_arrow_space;

  get_arrows_visible_area (popup,
                           &tmp_border, &tmp_upper, &tmp_lower, &tmp_arrow_space);

  if (upper)
    *upper = tmp_upper;

  if (lower)
    *lower = tmp_lower;
}
