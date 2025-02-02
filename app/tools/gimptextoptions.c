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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"
#include "libligmawidgets/ligmawidgets.h"
#include "libligmawidgets/ligmawidgets-private.h"

#include "tools-types.h"

#include "config/ligmaconfig-utils.h"

#include "core/ligma.h"
#include "core/ligmacontainer.h"
#include "core/ligmadashpattern.h"
#include "core/ligmadatafactory.h"
#include "core/ligmapattern.h"
#include "core/ligmastrokeoptions.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmaviewable.h"

#include "text/ligmatext.h"

#include "widgets/ligmacolorpanel.h"
#include "widgets/ligmamenufactory.h"
#include "widgets/ligmapropwidgets.h"
#include "widgets/ligmastrokeeditor.h"
#include "widgets/ligmatextbuffer.h"
#include "widgets/ligmatexteditor.h"
#include "widgets/ligmaviewablebox.h"
#include "widgets/ligmawidgets-utils.h"

#include "ligmatextoptions.h"
#include "ligmatooloptions-gui.h"

#include "ligma-intl.h"


enum
{
  PROP_0,
  PROP_FONT_SIZE,
  PROP_UNIT,
  PROP_ANTIALIAS,
  PROP_HINT_STYLE,
  PROP_LANGUAGE,
  PROP_BASE_DIR,
  PROP_JUSTIFICATION,
  PROP_INDENTATION,
  PROP_LINE_SPACING,
  PROP_LETTER_SPACING,
  PROP_BOX_MODE,

  PROP_OUTLINE,
  PROP_OUTLINE_STYLE,       /* fill-options */
  PROP_OUTLINE_FOREGROUND,  /* context */
  PROP_OUTLINE_PATTERN,     /* context */
  PROP_OUTLINE_WIDTH,       /* stroke-options */
  PROP_OUTLINE_UNIT,
  PROP_OUTLINE_CAP_STYLE,
  PROP_OUTLINE_JOIN_STYLE,
  PROP_OUTLINE_MITER_LIMIT,
  PROP_OUTLINE_ANTIALIAS,   /* fill-options */
  PROP_OUTLINE_DASH_OFFSET,
  PROP_OUTLINE_DASH_INFO,
  PROP_USE_EDITOR,

  PROP_FONT_VIEW_TYPE,
  PROP_FONT_VIEW_SIZE
};


static void
             ligma_text_options_config_iface_init    (LigmaConfigInterface *config_iface);
static gboolean
             ligma_text_options_serialize_property   (LigmaConfig          *config,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec,
                                                     LigmaConfigWriter    *writer);
static gboolean
             ligma_text_options_deserialize_property (LigmaConfig          *config,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec,
                                                     GScanner            *scanner,
                                                     GTokenType          *expected);

static void  ligma_text_options_finalize             (GObject             *object);
static void  ligma_text_options_set_property         (GObject             *object,
                                                     guint                property_id,
                                                     const GValue        *value,
                                                     GParamSpec          *pspec);
static void  ligma_text_options_get_property         (GObject             *object,
                                                     guint                property_id,
                                                     GValue              *value,
                                                     GParamSpec          *pspec);

static void  ligma_text_options_reset                (LigmaConfig          *config);

static void  ligma_text_options_notify_font          (LigmaContext         *context,
                                                     GParamSpec          *pspec,
                                                     LigmaText            *text);
static void  ligma_text_options_notify_text_font     (LigmaText            *text,
                                                     GParamSpec          *pspec,
                                                     LigmaContext         *context);
static void  ligma_text_options_notify_color         (LigmaContext         *context,
                                                     GParamSpec          *pspec,
                                                     LigmaText            *text);
static void  ligma_text_options_notify_text_color    (LigmaText            *text,
                                                     GParamSpec          *pspec,
                                                     LigmaContext         *context);
static void  ligma_text_options_outline_changed      (GtkWidget           *combo,
                                                     GtkWidget           *vbox);



G_DEFINE_TYPE_WITH_CODE (LigmaTextOptions, ligma_text_options,
                         LIGMA_TYPE_TOOL_OPTIONS,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_text_options_config_iface_init))

#define parent_class ligma_text_options_parent_class

static LigmaConfigInterface *parent_config_iface = NULL;


static void
ligma_text_options_class_init (LigmaTextOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  LigmaRGB       gray;
  GParamSpec   *array_spec;

  ligma_rgba_set (&gray, 0.75, 0.75, 0.75, LIGMA_OPACITY_OPAQUE);
  object_class->finalize     = ligma_text_options_finalize;
  object_class->set_property = ligma_text_options_set_property;
  object_class->get_property = ligma_text_options_get_property;

  LIGMA_CONFIG_PROP_UNIT (object_class, PROP_UNIT,
                         "font-size-unit",
                         _("Unit"),
                         _("Font size unit"),
                         TRUE, FALSE, LIGMA_UNIT_PIXEL,
                         LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_FONT_SIZE,
                           "font-size",
                           _("Font size"),
                           _("Font size"),
                           0.0, 8192.0, 62.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            _("Antialiasing"),
                            NULL,
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_HINT_STYLE,
                         "hint-style",
                         _("Hinting"),
                         _("Hinting alters the font outline to "
                           "produce a crisp bitmap at small "
                           "sizes"),
                         LIGMA_TYPE_TEXT_HINT_STYLE,
                         LIGMA_TEXT_HINT_STYLE_MEDIUM,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_LANGUAGE,
                           "language",
                           _("Language"),
                           _("The text language may have an effect "
                             "on the way the text is rendered."),
                           (const gchar *) gtk_get_default_language (),
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_BASE_DIR,
                         "base-direction",
                         NULL, NULL,
                         LIGMA_TYPE_TEXT_DIRECTION,
                         LIGMA_TEXT_DIRECTION_LTR,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_JUSTIFICATION,
                         "justify",
                         _("Justify"),
                         _("Text alignment"),
                         LIGMA_TYPE_TEXT_JUSTIFICATION,
                         LIGMA_TEXT_JUSTIFY_LEFT,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_INDENTATION,
                           "indent",
                           _("Indentation"),
                           _("Indentation of the first line"),
                           -8192.0, 8192.0, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_LINE_SPACING,
                           "line-spacing",
                           _("Line spacing"),
                           _("Adjust line spacing"),
                           -8192.0, 8192.0, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_LETTER_SPACING,
                           "letter-spacing",
                           _("Letter spacing"),
                           _("Adjust letter spacing"),
                           -8192.0, 8192.0, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_BOX_MODE,
                         "box-mode",
                         _("Box"),
                         _("Whether text flows into rectangular shape or "
                           "moves into a new line when you press Enter"),
                         LIGMA_TYPE_TEXT_BOX_MODE,
                         LIGMA_TEXT_BOX_DYNAMIC,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_EDITOR,
                            "use-editor",
                            _("Use editor"),
                            _("Use an external editor window for text entry"),
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_FONT_VIEW_TYPE,
                         "font-view-type",
                         NULL, NULL,
                         LIGMA_TYPE_VIEW_TYPE,
                         LIGMA_VIEW_TYPE_LIST,
                         LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_INT (object_class, PROP_FONT_VIEW_SIZE,
                        "font-view-size",
                        NULL, NULL,
                        LIGMA_VIEW_SIZE_TINY,
                        LIGMA_VIEWABLE_MAX_BUTTON_SIZE,
                        LIGMA_VIEW_SIZE_SMALL,
                        LIGMA_PARAM_STATIC_STRINGS);
  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE,
                         "outline",
                         NULL, NULL,
                         LIGMA_TYPE_TEXT_OUTLINE,
                         LIGMA_TEXT_OUTLINE_NONE,
                         LIGMA_PARAM_STATIC_STRINGS |
                         LIGMA_CONFIG_PARAM_DEFAULTS);
   LIGMA_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_STYLE,
                          "outline-style",
                          NULL, NULL,
                          LIGMA_TYPE_FILL_STYLE,
                          LIGMA_FILL_STYLE_SOLID,
                          LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_RGB (object_class, PROP_OUTLINE_FOREGROUND,
                         "outline-foreground",
                         NULL, NULL,
                         FALSE, &gray,
                         LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_OUTLINE_PATTERN,
                            "outline-pattern",
                            NULL, NULL,
                            LIGMA_TYPE_PATTERN,
                            LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_WIDTH,
                            "outline-width",
                            _("Outline width"),
                            _("Adjust outline width"),
                            0, 8192.0, 2.0,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_CONFIG_PARAM_DEFAULTS);
   LIGMA_CONFIG_PROP_UNIT (object_class, PROP_OUTLINE_UNIT,
                          "outline-unit",
                          _("Unit"),
                          _("Outline width unit"),
                          TRUE, FALSE, LIGMA_UNIT_PIXEL,
                          LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_CAP_STYLE,
                          "outline-cap-style",
                          NULL, NULL,
                          LIGMA_TYPE_CAP_STYLE, LIGMA_CAP_BUTT,
                          LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_JOIN_STYLE,
                          "outline-join-style",
                          NULL, NULL,
                          LIGMA_TYPE_JOIN_STYLE, LIGMA_JOIN_MITER,
                          LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_MITER_LIMIT,
                            "outline-miter-limit",
                            _("Outline miter limit"),
                            _("Convert a mitered join to a bevelled "
                              "join if the miter would extend to a "
                              "distance of more than miter-limit * "
                              "line-width from the actual join point."),
                            0.0, 100.0, 10.0,
                            LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_OUTLINE_ANTIALIAS,
                             "outline-antialias",
                             NULL, NULL,
                             TRUE,
                             LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_DASH_OFFSET,
                            "outline-dash-offset",
                            NULL, NULL,
                            0.0, 2000.0, 0.0,
                            LIGMA_PARAM_STATIC_STRINGS);

   array_spec = g_param_spec_double ("outline-dash-length",
                                     NULL, NULL,
                                     0.0, 2000.0, 1.0,
                                     LIGMA_PARAM_READWRITE);
   g_object_class_install_property (object_class, PROP_OUTLINE_DASH_INFO,
                                    ligma_param_spec_value_array ("outline-dash-info",
                                                                 NULL, NULL,
                                                                 array_spec,
                                                                 LIGMA_PARAM_STATIC_STRINGS |
                                                                 LIGMA_CONFIG_PARAM_FLAGS));
}

static void
ligma_text_options_config_iface_init (LigmaConfigInterface *config_iface)
{
  parent_config_iface = g_type_interface_peek_parent (config_iface);

  config_iface->reset = ligma_text_options_reset;

  config_iface->serialize_property   = ligma_text_options_serialize_property;
  config_iface->deserialize_property = ligma_text_options_deserialize_property;
}

static void
ligma_text_options_init (LigmaTextOptions *options)
{
  options->size_entry = NULL;
}

static void
ligma_text_options_finalize (GObject *object)
{
  LigmaTextOptions *options = LIGMA_TEXT_OPTIONS (object);

  g_clear_pointer (&options->language, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_text_options_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  LigmaTextOptions *options = LIGMA_TEXT_OPTIONS (object);

  switch (property_id)
    {
    case PROP_FONT_SIZE:
      g_value_set_double (value, options->font_size);
      break;
    case PROP_UNIT:
      g_value_set_int (value, options->unit);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, options->antialias);
      break;
    case PROP_HINT_STYLE:
      g_value_set_enum (value, options->hint_style);
      break;
    case PROP_LANGUAGE:
      g_value_set_string (value, options->language);
      break;
    case PROP_BASE_DIR:
      g_value_set_enum (value, options->base_dir);
      break;
    case PROP_JUSTIFICATION:
      g_value_set_enum (value, options->justify);
      break;
    case PROP_INDENTATION:
      g_value_set_double (value, options->indent);
      break;
    case PROP_LINE_SPACING:
      g_value_set_double (value, options->line_spacing);
      break;
    case PROP_LETTER_SPACING:
      g_value_set_double (value, options->letter_spacing);
      break;
    case PROP_BOX_MODE:
      g_value_set_enum (value, options->box_mode);
      break;

    case PROP_OUTLINE:
      g_value_set_enum (value, options->outline);
      break;
    case PROP_OUTLINE_STYLE:
      g_value_set_enum (value, options->outline_style);
      break;
    case PROP_OUTLINE_FOREGROUND:
      g_value_set_boxed (value, &options->outline_foreground);
      break;
    case PROP_OUTLINE_PATTERN:
      g_value_set_object (value, options->outline_pattern);
      break;
    case PROP_OUTLINE_WIDTH:
      g_value_set_double (value, options->outline_width);
      break;
    case PROP_OUTLINE_UNIT:
      g_value_set_int (value, options->outline_unit);
      break;
    case PROP_OUTLINE_CAP_STYLE:
      g_value_set_enum (value, options->outline_cap_style);
      break;
    case PROP_OUTLINE_JOIN_STYLE:
      g_value_set_enum (value, options->outline_join_style);
      break;
    case PROP_OUTLINE_MITER_LIMIT:
      g_value_set_double (value, options->outline_miter_limit);
      break;
    case PROP_OUTLINE_ANTIALIAS:
      g_value_set_boolean (value, options->outline_antialias);
      break;
    case PROP_OUTLINE_DASH_OFFSET:
      g_value_set_double (value, options->outline_dash_offset);
      break;
    case PROP_OUTLINE_DASH_INFO:
      {
        LigmaValueArray *value_array;

        value_array = ligma_dash_pattern_to_value_array (options->outline_dash_info);
        g_value_take_boxed (value, value_array);
      }
      break;

    case PROP_USE_EDITOR:
      g_value_set_boolean (value, options->use_editor);
      break;

    case PROP_FONT_VIEW_TYPE:
      g_value_set_enum (value, options->font_view_type);
      break;
    case PROP_FONT_VIEW_SIZE:
      g_value_set_int (value, options->font_view_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_text_options_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  LigmaTextOptions *options = LIGMA_TEXT_OPTIONS (object);
  LigmaRGB         *color;

  switch (property_id)
    {
    case PROP_FONT_SIZE:
      options->font_size = g_value_get_double (value);
      break;
    case PROP_UNIT:
      options->unit = g_value_get_int (value);
      break;
    case PROP_ANTIALIAS:
      options->antialias = g_value_get_boolean (value);
      break;
    case PROP_HINT_STYLE:
      options->hint_style = g_value_get_enum (value);
      break;
    case PROP_BASE_DIR:
      options->base_dir = g_value_get_enum (value);
      break;
    case PROP_LANGUAGE:
      g_free (options->language);
      options->language = g_value_dup_string (value);
      break;
    case PROP_JUSTIFICATION:
      options->justify = g_value_get_enum (value);
      break;
    case PROP_INDENTATION:
      options->indent = g_value_get_double (value);
      break;
    case PROP_LINE_SPACING:
      options->line_spacing = g_value_get_double (value);
      break;
    case PROP_LETTER_SPACING:
      options->letter_spacing = g_value_get_double (value);
      break;
    case PROP_BOX_MODE:
      options->box_mode = g_value_get_enum (value);
      break;

    case PROP_OUTLINE:
      options->outline = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_STYLE:
      options->outline_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_FOREGROUND:
      color                       = g_value_get_boxed (value);
      options->outline_foreground = *color;
      break;
    case PROP_OUTLINE_PATTERN:
      {
        LigmaPattern *pattern = g_value_get_object (value);

        if (options->outline_pattern != pattern)
          {
            if (options->outline_pattern)
              g_object_unref (options->outline_pattern);

            options->outline_pattern = pattern ? g_object_ref (pattern) : pattern;
          }
        break;
      }
    case PROP_OUTLINE_WIDTH:
      options->outline_width = g_value_get_double (value);
      break;
    case PROP_OUTLINE_UNIT:
      options->outline_unit = g_value_get_int (value);
      break;
    case PROP_OUTLINE_CAP_STYLE:
      options->outline_cap_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_JOIN_STYLE:
      options->outline_join_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_MITER_LIMIT:
      options->outline_miter_limit = g_value_get_double (value);
      break;
    case PROP_OUTLINE_ANTIALIAS:
      options->outline_antialias = g_value_get_boolean (value);
      break;
    case PROP_OUTLINE_DASH_OFFSET:
      options->outline_dash_offset = g_value_get_double (value);
      break;
    case PROP_OUTLINE_DASH_INFO:
      {
        LigmaValueArray *value_array = g_value_get_boxed (value);
        options->outline_dash_info = ligma_dash_pattern_from_value_array (value_array);
      }
      break;

    case PROP_USE_EDITOR:
      options->use_editor = g_value_get_boolean (value);
      break;

    case PROP_FONT_VIEW_TYPE:
      options->font_view_type = g_value_get_enum (value);
      break;
    case PROP_FONT_VIEW_SIZE:
      options->font_view_size = g_value_get_int (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_text_options_reset (LigmaConfig *config)
{
  GObject *object = G_OBJECT (config);

  /*  implement reset() ourselves because the default impl would
   *  reset *all* properties, including all rectangle properties
   *  of the text box
   */

  /* context */
  ligma_config_reset_property (object, "font");
  ligma_config_reset_property (object, "foreground");

  /* text options */
  ligma_config_reset_property (object, "font-size");
  ligma_config_reset_property (object, "font-size-unit");
  ligma_config_reset_property (object, "antialias");
  ligma_config_reset_property (object, "hint-style");
  ligma_config_reset_property (object, "language");
  ligma_config_reset_property (object, "base-direction");
  ligma_config_reset_property (object, "justify");
  ligma_config_reset_property (object, "indent");
  ligma_config_reset_property (object, "line-spacing");
  ligma_config_reset_property (object, "letter-spacing");
  ligma_config_reset_property (object, "box-mode");

  ligma_config_reset_property (object, "outline");
  ligma_config_reset_property (object, "outline-style");
  ligma_config_reset_property (object, "outline-foreground");
  ligma_config_reset_property (object, "outline-pattern");
  ligma_config_reset_property (object, "outline-width");
  ligma_config_reset_property (object, "outline-unit");
  ligma_config_reset_property (object, "outline-cap-style");
  ligma_config_reset_property (object, "outline-join-style");
  ligma_config_reset_property (object, "outline-miter-limit");
  ligma_config_reset_property (object, "outline-antialias");
  ligma_config_reset_property (object, "outline-dash-offset");
  ligma_config_reset_property (object, "outline-dash-info");

  ligma_config_reset_property (object, "use-editor");
}

static void
ligma_text_options_notify_font (LigmaContext *context,
                               GParamSpec  *pspec,
                               LigmaText    *text)
{
  g_signal_handlers_block_by_func (text,
                                   ligma_text_options_notify_text_font,
                                   context);

  g_object_set (text, "font", ligma_context_get_font_name (context), NULL);

  g_signal_handlers_unblock_by_func (text,
                                     ligma_text_options_notify_text_font,
                                     context);
}

static void
ligma_text_options_notify_text_font (LigmaText    *text,
                                    GParamSpec  *pspec,
                                    LigmaContext *context)
{
  g_signal_handlers_block_by_func (context,
                                   ligma_text_options_notify_font, text);

  ligma_context_set_font_name (context, text->font);

  g_signal_handlers_unblock_by_func (context,
                                     ligma_text_options_notify_font, text);
}

static void
ligma_text_options_notify_color (LigmaContext *context,
                                GParamSpec  *pspec,
                                LigmaText    *text)
{
  LigmaRGB  color;

  ligma_context_get_foreground (context, &color);

  g_signal_handlers_block_by_func (text,
                                   ligma_text_options_notify_text_color,
                                   context);

  g_object_set (text, "color", &color, NULL);

  g_signal_handlers_unblock_by_func (text,
                                     ligma_text_options_notify_text_color,
                                     context);
}

static void
ligma_text_options_notify_text_color (LigmaText    *text,
                                     GParamSpec  *pspec,
                                     LigmaContext *context)
{
  g_signal_handlers_block_by_func (context,
                                   ligma_text_options_notify_color, text);

  ligma_context_set_foreground (context, &text->color);

  g_signal_handlers_unblock_by_func (context,
                                     ligma_text_options_notify_color, text);
}

/*  This function could live in ligmatexttool.c also.
 *  But it takes some bloat out of that file...
 */
void
ligma_text_options_connect_text (LigmaTextOptions *options,
                                LigmaText        *text)
{
  LigmaContext *context;
  LigmaRGB      color;

  g_return_if_fail (LIGMA_IS_TEXT_OPTIONS (options));
  g_return_if_fail (LIGMA_IS_TEXT (text));

  context = LIGMA_CONTEXT (options);

  ligma_context_get_foreground (context, &color);

  ligma_config_sync (G_OBJECT (options), G_OBJECT (text), 0);

  g_object_set (text,
                "color", &color,
                "font",  ligma_context_get_font_name (context),
                NULL);

  ligma_config_connect (G_OBJECT (options), G_OBJECT (text), NULL);

  g_signal_connect_object (options, "notify::font",
                           G_CALLBACK (ligma_text_options_notify_font),
                           text, 0);
  g_signal_connect_object (text, "notify::font",
                           G_CALLBACK (ligma_text_options_notify_text_font),
                           options, 0);

  g_signal_connect_object (options, "notify::foreground",
                           G_CALLBACK (ligma_text_options_notify_color),
                           text, 0);
  g_signal_connect_object (text, "notify::color",
                           G_CALLBACK (ligma_text_options_notify_text_color),
                           options, 0);
}

GtkWidget *
ligma_text_options_gui (LigmaToolOptions *tool_options)
{
  GObject           *config    = G_OBJECT (tool_options);
  LigmaTextOptions   *options   = LIGMA_TEXT_OPTIONS (tool_options);
  GtkWidget         *main_vbox = ligma_tool_options_gui (tool_options);
  LigmaAsyncSet      *async_set;
  GtkWidget         *options_vbox;
  GtkWidget         *grid;
  GtkWidget         *vbox;
  GtkWidget         *hbox;
  GtkWidget         *button;
  GtkWidget         *entry;
  GtkWidget         *box;
  GtkWidget         *spinbutton;
  GtkWidget         *combo;
  GtkWidget         *editor;
  LigmaStrokeOptions *stroke_options;
  GtkWidget         *outline_frame;
  GtkSizeGroup      *size_group;
  gint               row = 0;

  async_set =
    ligma_data_factory_get_async_set (tool_options->tool_info->ligma->font_factory);

  box = ligma_busy_box_new (_("Loading fonts (this may take a while...)"));
  gtk_container_set_border_width (GTK_CONTAINER (box), 8);
  gtk_box_pack_start (GTK_BOX (main_vbox), box, FALSE, FALSE, 0);

  g_object_bind_property (async_set, "empty",
                          box,       "visible",
                          G_BINDING_SYNC_CREATE |
                          G_BINDING_INVERT_BOOLEAN);

  options_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL,
                              gtk_box_get_spacing (GTK_BOX (main_vbox)));
  gtk_box_pack_start (GTK_BOX (main_vbox), options_vbox, FALSE, FALSE, 0);
  gtk_widget_show (options_vbox);

  g_object_bind_property (async_set,    "empty",
                          options_vbox, "sensitive",
                          G_BINDING_SYNC_CREATE);

  hbox = ligma_prop_font_box_new (NULL, LIGMA_CONTEXT (tool_options),
                                 _("Font"), 2,
                                 "font-view-type", "font-view-size");
  gtk_box_pack_start (GTK_BOX (options_vbox), hbox, FALSE, FALSE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (options_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  entry = ligma_prop_size_entry_new (config,
                                    "font-size", FALSE, "font-size-unit", "%p",
                                    LIGMA_SIZE_ENTRY_UPDATE_SIZE, 72.0);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Size:"), 0.0, 0.5,
                            entry, 2);

  options->size_entry = entry;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (options_vbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  button = ligma_prop_check_button_new (config, "use-editor", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  button = ligma_prop_check_button_new (config, "antialias", NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (options_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  row = 0;

  size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  button = ligma_prop_enum_combo_box_new (config, "hint-style", -1, -1);
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Hinting:"), 0.0, 0.5,
                            button, 1);
  gtk_size_group_add_widget (size_group, button);

  button = ligma_prop_color_button_new (config, "foreground", _("Text Color"),
                                       40, 24, LIGMA_COLOR_AREA_FLAT);
  ligma_color_button_set_update (LIGMA_COLOR_BUTTON (button), TRUE);
  ligma_color_panel_set_context (LIGMA_COLOR_PANEL (button),
                                LIGMA_CONTEXT (options));
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Color:"), 0.0, 0.5,
                            button, 1);
  gtk_size_group_add_widget (size_group, button);

  button = ligma_prop_enum_combo_box_new (config, "outline", -1, -1);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                             _("Style:"), 0.0, 0.5,
                             button, 1);
  gtk_size_group_add_widget (size_group, button);

  outline_frame = ligma_frame_new (_("Outline Options"));
  ligma_widget_set_identifier (outline_frame, "text-outline-settings");
  gtk_box_pack_start (GTK_BOX (options_vbox), outline_frame, FALSE, FALSE, 0);
  gtk_widget_show (outline_frame);

  g_signal_connect (button, "changed",
                    G_CALLBACK (ligma_text_options_outline_changed),
                    outline_frame);
  ligma_text_options_outline_changed (button, outline_frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_box_pack_start (GTK_BOX (options_vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  row = 0;

  box = ligma_prop_enum_icon_box_new (config, "justify", "format-justify", 0, 0);
  gtk_widget_set_halign (box, GTK_ALIGN_START);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Justify:"), 0.0, 0.5,
                            box, 2);
  gtk_size_group_add_widget (size_group, box);
  g_object_unref (size_group);

  spinbutton = ligma_prop_spin_button_new (config, "indent", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gtk_widget_set_halign (spinbutton, GTK_ALIGN_START);
  ligma_grid_attach_icon (GTK_GRID (grid), row++,
                         LIGMA_ICON_FORMAT_INDENT_MORE,
                         spinbutton, 1);

  spinbutton = ligma_prop_spin_button_new (config, "line-spacing", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gtk_widget_set_halign (spinbutton, GTK_ALIGN_START);
  ligma_grid_attach_icon (GTK_GRID (grid), row++,
                         LIGMA_ICON_FORMAT_TEXT_SPACING_LINE,
                         spinbutton, 1);

  spinbutton = ligma_prop_spin_button_new (config,
                                          "letter-spacing", 1.0, 10.0, 1);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 5);
  gtk_widget_set_halign (spinbutton, GTK_ALIGN_START);
  ligma_grid_attach_icon (GTK_GRID (grid), row++,
                         LIGMA_ICON_FORMAT_TEXT_SPACING_LETTER,
                         spinbutton, 1);

  combo = ligma_prop_enum_combo_box_new (config, "box-mode", 0, 0);
  gtk_widget_set_halign (combo, GTK_ALIGN_START);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, row++,
                            _("Box:"), 0.0, 0.5,
                            combo, 1);
  stroke_options = ligma_stroke_options_new (LIGMA_CONTEXT (options)->ligma,
                                            NULL, FALSE);
#define BIND(a)                                    \
  g_object_bind_property (options, "outline-" #a,  \
                          stroke_options, #a,                           \
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE)
  BIND (style);
  BIND (foreground);
  BIND (pattern);
  BIND (width);
  BIND (unit);
  BIND (cap-style);
  BIND (join-style);
  BIND (miter-limit);
  BIND (antialias);
  BIND (dash-offset);
  BIND (dash-info);

  editor = ligma_stroke_editor_new (stroke_options, 72.0, TRUE);
  gtk_container_add (GTK_CONTAINER (outline_frame), editor);
  gtk_widget_show (editor);

  g_object_unref (stroke_options);

  /*  Only add the language entry if the iso-codes package is available.  */

#ifdef HAVE_ISO_CODES
  {
    GtkWidget *label;

    vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start (GTK_BOX (options_vbox), vbox, FALSE, FALSE, 0);
    gtk_widget_show (vbox);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Language:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    entry = ligma_prop_language_entry_new (config, "language");
    gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);
  }
#endif

  return main_vbox;
}

static void
ligma_text_options_editor_dir_changed (LigmaTextEditor  *editor,
                                      LigmaTextOptions *options)
{
  g_object_set (options,
                "base-direction", editor->base_dir,
                NULL);
}

static void
ligma_text_options_editor_notify_dir (LigmaTextOptions *options,
                                     GParamSpec      *pspec,
                                     LigmaTextEditor  *editor)
{
  LigmaTextDirection  dir;

  g_object_get (options,
                "base-direction", &dir,
                NULL);

  ligma_text_editor_set_direction (editor, dir);
}

static void
ligma_text_options_editor_notify_font (LigmaTextOptions *options,
                                      GParamSpec      *pspec,
                                      LigmaTextEditor  *editor)
{
  const gchar *font_name;

  font_name = ligma_context_get_font_name (LIGMA_CONTEXT (options));

  ligma_text_editor_set_font_name (editor, font_name);
}

static void
ligma_text_options_outline_changed (GtkWidget *combo,
                                   GtkWidget *vbox)
{
  LigmaTextOutline active;

  active = (LigmaTextOutline) gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

  if (active == LIGMA_TEXT_OUTLINE_NONE)
    gtk_widget_hide (vbox);
  else
    gtk_widget_show (vbox);
}

GtkWidget *
ligma_text_options_editor_new (GtkWindow       *parent,
                              Ligma            *ligma,
                              LigmaTextOptions *options,
                              LigmaMenuFactory *menu_factory,
                              const gchar     *title,
                              LigmaText        *text,
                              LigmaTextBuffer  *text_buffer,
                              gdouble          xres,
                              gdouble          yres)
{
  GtkWidget   *editor;
  const gchar *font_name;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT_OPTIONS (options), NULL);
  g_return_val_if_fail (LIGMA_IS_MENU_FACTORY (menu_factory), NULL);
  g_return_val_if_fail (title != NULL, NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT (text), NULL);
  g_return_val_if_fail (LIGMA_IS_TEXT_BUFFER (text_buffer), NULL);

  editor = ligma_text_editor_new (title, parent, ligma, menu_factory,
                                 text, text_buffer, xres, yres);

  font_name = ligma_context_get_font_name (LIGMA_CONTEXT (options));

  ligma_text_editor_set_direction (LIGMA_TEXT_EDITOR (editor),
                                  options->base_dir);
  ligma_text_editor_set_font_name (LIGMA_TEXT_EDITOR (editor),
                                  font_name);

  g_signal_connect_object (editor, "dir-changed",
                           G_CALLBACK (ligma_text_options_editor_dir_changed),
                           options, 0);
  g_signal_connect_object (options, "notify::base-direction",
                           G_CALLBACK (ligma_text_options_editor_notify_dir),
                           editor, 0);
  g_signal_connect_object (options, "notify::font",
                           G_CALLBACK (ligma_text_options_editor_notify_font),
                           editor, 0);

  return editor;
}

static gboolean
ligma_text_options_serialize_property (LigmaConfig       *config,
                                      guint             property_id,
                                      const GValue     *value,
                                      GParamSpec       *pspec,
                                      LigmaConfigWriter *writer)
{
  if (property_id == PROP_OUTLINE_PATTERN)
    {
      LigmaObject *serialize_obj = g_value_get_object (value);

      ligma_config_writer_open (writer, pspec->name);

      if (serialize_obj)
        ligma_config_writer_string (writer, ligma_object_get_name (serialize_obj));
      else
        ligma_config_writer_print (writer, "NULL", 4);

      ligma_config_writer_close (writer);

      return TRUE;
    }

  return FALSE;
}

static gboolean
ligma_text_options_deserialize_property (LigmaConfig *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec,
                                        GScanner   *scanner,
                                        GTokenType *expected)
{
  if (property_id == PROP_OUTLINE_PATTERN)
    {
      gchar *object_name;

      if (ligma_scanner_parse_identifier (scanner, "NULL"))
        {
          g_value_set_object (value, NULL);
        }
      else if (ligma_scanner_parse_string (scanner, &object_name))
        {
          LigmaContext   *context = LIGMA_CONTEXT (object);
          LigmaContainer *container;
          LigmaObject    *deserialize_obj;

          if (! object_name)
            object_name = g_strdup ("");

          container = ligma_data_factory_get_container (context->ligma->pattern_factory);

          deserialize_obj = ligma_container_get_child_by_name (container,
                                                              object_name);

          g_value_set_object (value, deserialize_obj);

          g_free (object_name);
        }
      else
        {
          *expected = G_TOKEN_STRING;
        }

      return TRUE;
    }

  return FALSE;
}
