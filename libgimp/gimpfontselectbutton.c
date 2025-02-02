/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmafontselectbutton.c
 * Copyright (C) 2003  Sven Neumann  <sven@ligma.org>
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
#include "ligmafontselectbutton.h"
#include "ligmauimarshal.h"

#include "libligma-intl.h"


/**
 * SECTION: ligmafontselectbutton
 * @title: LigmaFontSelectButton
 * @short_description: A button which pops up a font selection dialog.
 *
 * A button which pops up a font selection dialog.
 **/


enum
{
  FONT_SET,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TITLE,
  PROP_FONT_NAME,
  N_PROPS
};


struct _LigmaFontSelectButtonPrivate
{
  gchar       *title;

  gchar       *font_name;      /* local copy */

  GtkWidget   *inside;
  GtkWidget   *label;
};


/*  local function prototypes  */

static void   ligma_font_select_button_finalize     (GObject      *object);

static void   ligma_font_select_button_set_property (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void   ligma_font_select_button_get_property (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);

static void   ligma_font_select_button_clicked  (LigmaFontSelectButton *button);

static void   ligma_font_select_button_callback (const gchar *font_name,
                                                gboolean     dialog_closing,
                                                gpointer     user_data);

static void   ligma_font_select_drag_data_received (LigmaFontSelectButton *button,
                                                   GdkDragContext       *context,
                                                   gint                  x,
                                                   gint                  y,
                                                   GtkSelectionData     *selection,
                                                   guint                 info,
                                                   guint                 time);

static GtkWidget * ligma_font_select_button_create_inside (LigmaFontSelectButton *button);


static const GtkTargetEntry target = { "application/x-ligma-font-name", 0 };

static guint font_button_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *font_button_props[N_PROPS] = { NULL, };


G_DEFINE_TYPE_WITH_PRIVATE (LigmaFontSelectButton, ligma_font_select_button,
                            LIGMA_TYPE_SELECT_BUTTON)


static void
ligma_font_select_button_class_init (LigmaFontSelectButtonClass *klass)
{
  GObjectClass          *object_class        = G_OBJECT_CLASS (klass);
  LigmaSelectButtonClass *select_button_class = LIGMA_SELECT_BUTTON_CLASS (klass);

  object_class->finalize     = ligma_font_select_button_finalize;
  object_class->set_property = ligma_font_select_button_set_property;
  object_class->get_property = ligma_font_select_button_get_property;

  select_button_class->select_destroy = ligma_font_select_destroy;

  klass->font_set = NULL;

  /**
   * LigmaFontSelectButton:title:
   *
   * The title to be used for the font selection popup dialog.
   *
   * Since: 2.4
   */
  font_button_props[PROP_TITLE] = g_param_spec_string ("title",
                                                       "Title",
                                                       "The title to be used for the font selection popup dialog",
                                                       _("Font Selection"),
                                                       LIGMA_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY);

  /**
   * LigmaFontSelectButton:font-name:
   *
   * The name of the currently selected font.
   *
   * Since: 2.4
   */
  font_button_props[PROP_FONT_NAME] = g_param_spec_string ("font-name",
                                                           "Font name",
                                                           "The name of the currently selected font",
                                                           "Sans-serif",
                                                           LIGMA_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, font_button_props);

  /**
   * LigmaFontSelectButton::font-set:
   * @widget: the object which received the signal.
   * @font_name: the name of the currently selected font.
   * @dialog_closing: whether the dialog was closed or not.
   *
   * The ::font-set signal is emitted when the user selects a font.
   *
   * Since: 2.4
   */
  font_button_signals[FONT_SET] =
    g_signal_new ("font-set",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaFontSelectButtonClass, font_set),
                  NULL, NULL,
                  _ligmaui_marshal_VOID__STRING_BOOLEAN,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  G_TYPE_BOOLEAN);
}

static void
ligma_font_select_button_init (LigmaFontSelectButton *button)
{
  button->priv = ligma_font_select_button_get_instance_private (button);

  button->priv->inside = ligma_font_select_button_create_inside (button);
  gtk_container_add (GTK_CONTAINER (button), button->priv->inside);
}

/**
 * ligma_font_select_button_new:
 * @title: (nullable): Title of the dialog to use or %NULL to use the default title.
 * @font_name: (nullable): Initial font name.
 *
 * Creates a new #GtkWidget that completely controls the selection of
 * a font.  This widget is suitable for placement in a table in a
 * plug-in dialog.
 *
 * Returns: A #GtkWidget that you can use in your UI.
 *
 * Since: 2.4
 */
GtkWidget *
ligma_font_select_button_new (const gchar *title,
                             const gchar *font_name)
{
  GtkWidget *button;

  if (title)
    button = g_object_new (LIGMA_TYPE_FONT_SELECT_BUTTON,
                           "title",     title,
                           "font-name", font_name,
                           NULL);
  else
    button = g_object_new (LIGMA_TYPE_FONT_SELECT_BUTTON,
                           "font-name", font_name,
                           NULL);

  return button;
}

/**
 * ligma_font_select_button_get_font:
 * @button: A #LigmaFontSelectButton
 *
 * Retrieves the name of currently selected font.
 *
 * Returns: an internal copy of the font name which must not be freed.
 *
 * Since: 2.4
 */
const gchar *
ligma_font_select_button_get_font (LigmaFontSelectButton *button)
{
  g_return_val_if_fail (LIGMA_IS_FONT_SELECT_BUTTON (button), NULL);

  return button->priv->font_name;
}

/**
 * ligma_font_select_button_set_font:
 * @button: A #LigmaFontSelectButton
 * @font_name: (nullable): Font name to set; %NULL means no change.
 *
 * Sets the current font for the font select button.
 *
 * Since: 2.4
 */
void
ligma_font_select_button_set_font (LigmaFontSelectButton *button,
                                  const gchar          *font_name)
{
  LigmaSelectButton *select_button;

  g_return_if_fail (LIGMA_IS_FONT_SELECT_BUTTON (button));

  select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      ligma_fonts_set_popup (select_button->temp_callback, font_name);
    }
  else
    {
      gchar *name;

      if (font_name && *font_name)
        name = g_strdup (font_name);
      else
        name = ligma_context_get_font ();

      ligma_font_select_button_callback (name, FALSE, button);

      g_free (name);
    }
}


/*  private functions  */

static void
ligma_font_select_button_finalize (GObject *object)
{
  LigmaFontSelectButton *button = LIGMA_FONT_SELECT_BUTTON (object);

  g_clear_pointer (&button->priv->font_name, g_free);
  g_clear_pointer (&button->priv->title,     g_free);

  G_OBJECT_CLASS (ligma_font_select_button_parent_class)->finalize (object);
}

static void
ligma_font_select_button_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  LigmaFontSelectButton *button = LIGMA_FONT_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      button->priv->title = g_value_dup_string (value);
      break;

    case PROP_FONT_NAME:
      ligma_font_select_button_set_font (button,
                                        g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_font_select_button_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  LigmaFontSelectButton *button = LIGMA_FONT_SELECT_BUTTON (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, button->priv->title);
      break;

    case PROP_FONT_NAME:
      g_value_set_string (value, button->priv->font_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_font_select_button_callback (const gchar *font_name,
                                  gboolean     dialog_closing,
                                  gpointer     user_data)
{
  LigmaFontSelectButton        *button        = LIGMA_FONT_SELECT_BUTTON (user_data);
  LigmaFontSelectButtonPrivate *priv          = button->priv;
  LigmaSelectButton            *select_button = LIGMA_SELECT_BUTTON (button);

  g_free (priv->font_name);
  priv->font_name = g_strdup (font_name);

  gtk_label_set_text (GTK_LABEL (priv->label), font_name);

  if (dialog_closing)
    select_button->temp_callback = NULL;

  g_signal_emit (button, font_button_signals[FONT_SET], 0,
                 font_name, dialog_closing);
  g_object_notify_by_pspec (G_OBJECT (button), font_button_props[PROP_FONT_NAME]);
}

static void
ligma_font_select_button_clicked (LigmaFontSelectButton *button)
{
  LigmaSelectButton *select_button = LIGMA_SELECT_BUTTON (button);

  if (select_button->temp_callback)
    {
      /*  calling ligma_fonts_set_popup() raises the dialog  */
      ligma_fonts_set_popup (select_button->temp_callback,
                            button->priv->font_name);
    }
  else
    {
      select_button->temp_callback =
        ligma_font_select_new (button->priv->title,
                              button->priv->font_name,
                              ligma_font_select_button_callback,
                              button, NULL);
    }
}

static void
ligma_font_select_drag_data_received (LigmaFontSelectButton *button,
                                     GdkDragContext       *context,
                                     gint                  x,
                                     gint                  y,
                                     GtkSelectionData     *selection,
                                     guint                 info,
                                     guint                 time)
{
  gint   length = gtk_selection_data_get_length (selection);
  gchar *str;

  if (gtk_selection_data_get_format (selection) != 8 || length < 1)
    {
      g_warning ("%s: received invalid font data", G_STRFUNC);
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

          ligma_font_select_button_set_font (button, name);
        }
    }

  g_free (str);
}

static GtkWidget *
ligma_font_select_button_create_inside (LigmaFontSelectButton *font_button)
{
  LigmaFontSelectButtonPrivate *priv = font_button->priv;
  GtkWidget                   *button;
  GtkWidget                   *hbox;
  GtkWidget                   *image;

  button = gtk_button_new ();

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add (GTK_CONTAINER (button), hbox);

  image = gtk_image_new_from_icon_name (LIGMA_ICON_FONT,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

  priv->label = gtk_label_new (priv->font_name);
  gtk_box_pack_start (GTK_BOX (hbox), priv->label, TRUE, TRUE, 4);

  gtk_widget_show_all (button);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (ligma_font_select_button_clicked),
                            font_button);

  gtk_drag_dest_set (GTK_WIDGET (button),
                     GTK_DEST_DEFAULT_HIGHLIGHT |
                     GTK_DEST_DEFAULT_MOTION |
                     GTK_DEST_DEFAULT_DROP,
                     &target, 1,
                     GDK_ACTION_COPY);

  g_signal_connect_swapped (button, "drag-data-received",
                            G_CALLBACK (ligma_font_select_drag_data_received),
                            font_button);

  return button;
}
