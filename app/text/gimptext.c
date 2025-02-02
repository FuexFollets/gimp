/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaText
 * Copyright (C) 2002-2003  Sven Neumann <sven@ligma.org>
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

#include <string.h>

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <pango/pango.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "text-types.h"

#include "core/ligma.h"
#include "core/ligma-memsize.h"
#include "core/ligma-utils.h"
#include "core/ligmacontainer.h"
#include "core/ligmadashpattern.h"
#include "core/ligmadatafactory.h"
#include "core/ligmastrokeoptions.h"
#include "core/ligmapattern.h"

#include "ligmatext.h"


enum
{
  PROP_0,
  PROP_LIGMA,
  PROP_TEXT,
  PROP_MARKUP,
  PROP_FONT,
  PROP_FONT_SIZE,
  PROP_UNIT,
  PROP_ANTIALIAS,
  PROP_HINT_STYLE,
  PROP_KERNING,
  PROP_LANGUAGE,
  PROP_BASE_DIR,
  PROP_COLOR,
  PROP_OUTLINE,
  PROP_JUSTIFICATION,
  PROP_INDENTATION,
  PROP_LINE_SPACING,
  PROP_LETTER_SPACING,
  PROP_BOX_MODE,
  PROP_BOX_WIDTH,
  PROP_BOX_HEIGHT,
  PROP_BOX_UNIT,
  PROP_TRANSFORMATION,
  PROP_OFFSET_X,
  PROP_OFFSET_Y,
  PROP_BORDER,

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
  /* for backward compatibility */
  PROP_HINTING
};

enum
{
  CHANGED,
  LAST_SIGNAL
};

static void     ligma_text_config_iface_init           (LigmaConfigInterface *iface);
static gboolean ligma_text_serialize_property          (LigmaConfig       *config,
                                                       guint             property_id,
                                                       const GValue     *value,
                                                       GParamSpec       *pspec,
                                                       LigmaConfigWriter *writer);
static gboolean ligma_text_deserialize_property        (LigmaConfig       *config,
                                                       guint             property_id,
                                                       GValue           *value,
                                                       GParamSpec       *pspec,
                                                       GScanner         *scanner,
                                                       GTokenType       *expected);

static void     ligma_text_finalize                    (GObject      *object);
static void     ligma_text_get_property                (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static void     ligma_text_set_property                (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void     ligma_text_dispatch_properties_changed (GObject      *object,
                                                       guint         n_pspecs,
                                                       GParamSpec  **pspecs);
static gint64   ligma_text_get_memsize                 (LigmaObject   *object,
                                                       gint64       *gui_size);


G_DEFINE_TYPE_WITH_CODE (LigmaText, ligma_text, LIGMA_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_text_config_iface_init))

#define parent_class ligma_text_parent_class

static guint text_signals[LAST_SIGNAL] = { 0 };


static void
ligma_text_class_init (LigmaTextClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaRGB          black;
  LigmaRGB          gray;
  LigmaMatrix2      identity;
  gchar           *language;
  GParamSpec      *array_spec;

  text_signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaTextClass, changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->finalize                    = ligma_text_finalize;
  object_class->get_property                = ligma_text_get_property;
  object_class->set_property                = ligma_text_set_property;
  object_class->dispatch_properties_changed = ligma_text_dispatch_properties_changed;

  ligma_object_class->get_memsize            = ligma_text_get_memsize;

  ligma_rgba_set (&black, 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&gray, 0.75, 0.75, 0.75, LIGMA_OPACITY_OPAQUE);
  ligma_matrix2_identity (&identity);

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_TEXT,
                           "text",
                           NULL, NULL,
                           NULL,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_MARKUP,
                           "markup",
                           NULL, NULL,
                           NULL,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_FONT,
                           "font",
                           NULL, NULL,
                           "Sans-serif",
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_FONT_SIZE,
                           "font-size",
                           NULL, NULL,
                           0.0, 8192.0, 24.0,
                           LIGMA_PARAM_STATIC_STRINGS);

  /*  We use the name "font-size-unit" for backward compatibility.
   *  The unit is also used for other sizes in the text object.
   */
  LIGMA_CONFIG_PROP_UNIT (object_class, PROP_UNIT,
                         "font-size-unit",
                         NULL, NULL,
                         TRUE, FALSE, LIGMA_UNIT_PIXEL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_ANTIALIAS,
                            "antialias",
                            NULL, NULL,
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_HINT_STYLE,
                         "hint-style",
                         NULL, NULL,
                         LIGMA_TYPE_TEXT_HINT_STYLE,
                         LIGMA_TEXT_HINT_STYLE_MEDIUM,
                         LIGMA_PARAM_STATIC_STRINGS |
                         LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_KERNING,
                            "kerning",
                            NULL, NULL,
                            FALSE,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_CONFIG_PARAM_DEFAULTS);

  language = ligma_get_default_language (NULL);

  LIGMA_CONFIG_PROP_STRING (object_class, PROP_LANGUAGE,
                           "language",
                           NULL, NULL,
                           language,
                           LIGMA_PARAM_STATIC_STRINGS);

  g_free (language);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_BASE_DIR,
                         "base-direction",
                         NULL, NULL,
                         LIGMA_TYPE_TEXT_DIRECTION,
                         LIGMA_TEXT_DIRECTION_LTR,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_RGB (object_class, PROP_COLOR,
                        "color",
                        NULL, NULL,
                        FALSE, &black,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE,
                         "outline",
                         NULL, NULL,
                         LIGMA_TYPE_TEXT_OUTLINE,
                         LIGMA_TEXT_OUTLINE_NONE,
                         LIGMA_PARAM_STATIC_STRINGS |
                         LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_JUSTIFICATION,
                         "justify",
                         NULL, NULL,
                         LIGMA_TYPE_TEXT_JUSTIFICATION,
                         LIGMA_TEXT_JUSTIFY_LEFT,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_INDENTATION,
                           "indent",
                           NULL, NULL,
                           -8192.0, 8192.0, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_LINE_SPACING,
                           "line-spacing",
                           NULL, NULL,
                           -8192.0, 8192.0, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_LETTER_SPACING,
                           "letter-spacing",
                           NULL, NULL,
                           -8192.0, 8192.0, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_ENUM (object_class, PROP_BOX_MODE,
                         "box-mode",
                         NULL, NULL,
                         LIGMA_TYPE_TEXT_BOX_MODE,
                         LIGMA_TEXT_BOX_DYNAMIC,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BOX_WIDTH,
                           "box-width",
                           NULL, NULL,
                           0.0, LIGMA_MAX_IMAGE_SIZE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_BOX_HEIGHT,
                           "box-height",
                           NULL, NULL,
                           0.0, LIGMA_MAX_IMAGE_SIZE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_UNIT (object_class, PROP_BOX_UNIT,
                         "box-unit",
                         NULL, NULL,
                         TRUE, FALSE, LIGMA_UNIT_PIXEL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_MATRIX2 (object_class, PROP_TRANSFORMATION,
                            "transformation",
                            NULL, NULL,
                            &identity,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OFFSET_X,
                           "offset-x",
                           NULL, NULL,
                           -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OFFSET_Y,
                           "offset-y",
                           NULL, NULL,
                           -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                           LIGMA_PARAM_STATIC_STRINGS |
                           LIGMA_CONFIG_PARAM_DEFAULTS);

  /*  border does only exist to implement the old text API  */
  g_object_class_install_property (object_class, PROP_BORDER,
                                   g_param_spec_int ("border", NULL, NULL,
                                                     0, LIGMA_MAX_IMAGE_SIZE, 0,
                                                     G_PARAM_CONSTRUCT |
                                                     LIGMA_PARAM_WRITABLE));

   LIGMA_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_STYLE,
                          "outline-style", NULL, NULL,
                          LIGMA_TYPE_FILL_STYLE,
                          LIGMA_FILL_STYLE_SOLID,
                          LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_OUTLINE_PATTERN,
                            "outline-pattern", NULL, NULL,
                            LIGMA_TYPE_PATTERN,
                            LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_RGB (object_class, PROP_OUTLINE_FOREGROUND,
                         "outline-foreground", NULL, NULL,
                         FALSE, &gray,
                         LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_WIDTH,
                            "outline-width", NULL, NULL,
                            0.0, 8192.0, 4.0,
                            LIGMA_PARAM_STATIC_STRINGS |
                            LIGMA_CONFIG_PARAM_DEFAULTS);
   LIGMA_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_CAP_STYLE,
                          "outline-cap-style", NULL, NULL,
                          LIGMA_TYPE_CAP_STYLE, LIGMA_CAP_BUTT,
                          LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_ENUM (object_class, PROP_OUTLINE_JOIN_STYLE,
                          "outline-join-style", NULL, NULL,
                          LIGMA_TYPE_JOIN_STYLE, LIGMA_JOIN_MITER,
                          LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_MITER_LIMIT,
                            "outline-miter-limit",
                            NULL, NULL,
                            0.0, 100.0, 10.0,
                            LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_OUTLINE_ANTIALIAS,
                             "outline-antialias", NULL, NULL,
                             TRUE,
                             LIGMA_PARAM_STATIC_STRINGS);
   LIGMA_CONFIG_PROP_DOUBLE (object_class, PROP_OUTLINE_DASH_OFFSET,
                            "outline-dash-offset", NULL, NULL,
                            0.0, 2000.0, 0.0,
                            LIGMA_PARAM_STATIC_STRINGS);

   array_spec = g_param_spec_double ("outline-dash-length", NULL, NULL,
                                     0.0, 2000.0, 1.0, LIGMA_PARAM_READWRITE);
   g_object_class_install_property (object_class, PROP_OUTLINE_DASH_INFO,
                                    ligma_param_spec_value_array ("outline-dash-info",
                                                                 NULL, NULL,
                                                                 array_spec,
                                                                 LIGMA_PARAM_STATIC_STRINGS |
                                                                 LIGMA_CONFIG_PARAM_FLAGS));

  /*  the old hinting options have been replaced by 'hint-style'  */
  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_HINTING,
                            "hinting",
                            NULL, NULL,
                            TRUE,
                            LIGMA_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, PROP_LIGMA,
                                   g_param_spec_object ("ligma", NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

}

static void
ligma_text_init (LigmaText *text)
{
}

static void
ligma_text_config_iface_init (LigmaConfigInterface *iface)
{
  iface->serialize_property   = ligma_text_serialize_property;
  iface->deserialize_property = ligma_text_deserialize_property;
}

static void
ligma_text_finalize (GObject *object)
{
  LigmaText *text = LIGMA_TEXT (object);

  g_clear_pointer (&text->text,     g_free);
  g_clear_pointer (&text->markup,   g_free);
  g_clear_pointer (&text->font,     g_free);
  g_clear_pointer (&text->language, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_text_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
  LigmaText *text = LIGMA_TEXT (object);

  switch (property_id)
    {
    case PROP_TEXT:
      g_value_set_string (value, text->text);
      break;
    case PROP_MARKUP:
      g_value_set_string (value, text->markup);
      break;
    case PROP_FONT:
      g_value_set_string (value, text->font);
      break;
    case PROP_FONT_SIZE:
      g_value_set_double (value, text->font_size);
      break;
    case PROP_UNIT:
      g_value_set_int (value, text->unit);
      break;
    case PROP_ANTIALIAS:
      g_value_set_boolean (value, text->antialias);
      break;
    case PROP_HINT_STYLE:
      g_value_set_enum (value, text->hint_style);
      break;
    case PROP_KERNING:
      g_value_set_boolean (value, text->kerning);
      break;
    case PROP_BASE_DIR:
      g_value_set_enum (value, text->base_dir);
      break;
    case PROP_LANGUAGE:
      g_value_set_string (value, text->language);
      break;
    case PROP_COLOR:
      g_value_set_boxed (value, &text->color);
      break;
    case PROP_OUTLINE:
      g_value_set_enum (value, text->outline);
      break;
    case PROP_JUSTIFICATION:
      g_value_set_enum (value, text->justify);
      break;
    case PROP_INDENTATION:
      g_value_set_double (value, text->indent);
      break;
    case PROP_LINE_SPACING:
      g_value_set_double (value, text->line_spacing);
      break;
    case PROP_LETTER_SPACING:
      g_value_set_double (value, text->letter_spacing);
      break;
    case PROP_BOX_MODE:
      g_value_set_enum (value, text->box_mode);
      break;
    case PROP_BOX_WIDTH:
      g_value_set_double (value, text->box_width);
      break;
    case PROP_BOX_HEIGHT:
      g_value_set_double (value, text->box_height);
      break;
    case PROP_BOX_UNIT:
      g_value_set_int (value, text->box_unit);
      break;
    case PROP_TRANSFORMATION:
      g_value_set_boxed (value, &text->transformation);
      break;
    case PROP_OFFSET_X:
      g_value_set_double (value, text->offset_x);
      break;
    case PROP_OFFSET_Y:
      g_value_set_double (value, text->offset_y);
      break;
    case PROP_OUTLINE_STYLE:
      g_value_set_enum (value, text->outline_style);
      break;
    case PROP_OUTLINE_FOREGROUND:
      g_value_set_boxed (value, &text->outline_foreground);
      break;
    case PROP_OUTLINE_PATTERN:
      g_value_set_object (value, text->outline_pattern);
      break;
    case PROP_OUTLINE_WIDTH:
      g_value_set_double (value, text->outline_width);
      break;
    case PROP_OUTLINE_CAP_STYLE:
      g_value_set_enum (value, text->outline_cap_style);
      break;
    case PROP_OUTLINE_JOIN_STYLE:
      g_value_set_enum (value, text->outline_join_style);
      break;
    case PROP_OUTLINE_MITER_LIMIT:
      g_value_set_double (value, text->outline_miter_limit);
      break;
    case PROP_OUTLINE_ANTIALIAS:
      g_value_set_boolean (value, text->outline_antialias);
      break;
    case PROP_OUTLINE_DASH_OFFSET:
      g_value_set_double (value, text->outline_dash_offset);
      break;
    case PROP_OUTLINE_DASH_INFO:
      {
        LigmaValueArray *value_array;

        value_array = ligma_dash_pattern_to_value_array (text->outline_dash_info);
        g_value_take_boxed (value, value_array);
      }
      break;
    case PROP_HINTING:
      g_value_set_boolean (value,
                           text->hint_style != LIGMA_TEXT_HINT_STYLE_NONE);
      break;
    case PROP_LIGMA:
      g_value_set_object (value, text->ligma);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_text_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  LigmaText    *text = LIGMA_TEXT (object);
  LigmaRGB     *color;
  LigmaMatrix2 *matrix;

  switch (property_id)
    {
    case PROP_TEXT:
      g_free (text->text);
      text->text = g_value_dup_string (value);
      if (text->text && text->markup)
        {
          g_clear_pointer (&text->markup, g_free);
          g_object_notify (object, "markup");
        }
      break;
    case PROP_MARKUP:
      g_free (text->markup);
      text->markup = g_value_dup_string (value);
      if (text->markup && text->text)
        {
          g_clear_pointer (&text->text, g_free);
          g_object_notify (object, "text");
        }
      break;
    case PROP_FONT:
      {
        const gchar *font = g_value_get_string (value);

        g_free (text->font);

        if (font)
          {
            gsize len = strlen (font);

            if (g_str_has_suffix (font, " Not-Rotated"))
              len -= strlen ( " Not-Rotated");

            text->font = g_strndup (font, len);
          }
        else
          {
            text->font = NULL;
          }
      }
      break;
    case PROP_FONT_SIZE:
      text->font_size = g_value_get_double (value);
      break;
    case PROP_UNIT:
      text->unit = g_value_get_int (value);
      break;
    case PROP_ANTIALIAS:
      text->antialias = g_value_get_boolean (value);
      break;
    case PROP_HINT_STYLE:
      text->hint_style = g_value_get_enum (value);
      break;
    case PROP_KERNING:
      text->kerning = g_value_get_boolean (value);
      break;
    case PROP_LANGUAGE:
      g_free (text->language);
      text->language = g_value_dup_string (value);
      break;
    case PROP_BASE_DIR:
      text->base_dir = g_value_get_enum (value);
      break;
    case PROP_COLOR:
      color = g_value_get_boxed (value);
      text->color = *color;
      break;
    case PROP_OUTLINE:
      text->outline = g_value_get_enum (value);
      break;
    case PROP_JUSTIFICATION:
      text->justify = g_value_get_enum (value);
      break;
    case PROP_INDENTATION:
      text->indent = g_value_get_double (value);
      break;
    case PROP_LINE_SPACING:
      text->line_spacing = g_value_get_double (value);
      break;
    case PROP_LETTER_SPACING:
      text->letter_spacing = g_value_get_double (value);
      break;
    case PROP_BOX_MODE:
      text->box_mode = g_value_get_enum (value);
      break;
    case PROP_BOX_WIDTH:
      text->box_width = g_value_get_double (value);
      break;
    case PROP_BOX_HEIGHT:
      text->box_height = g_value_get_double (value);
      break;
    case PROP_BOX_UNIT:
      text->box_unit = g_value_get_int (value);
      break;
    case PROP_TRANSFORMATION:
      matrix = g_value_get_boxed (value);
      text->transformation = *matrix;
      break;
    case PROP_OFFSET_X:
      text->offset_x = g_value_get_double (value);
      break;
    case PROP_OFFSET_Y:
      text->offset_y = g_value_get_double (value);
      break;
    case PROP_OUTLINE_STYLE:
      text->outline_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_FOREGROUND:
      color                    = g_value_get_boxed (value);
      text->outline_foreground = *color;
      break;
    case PROP_OUTLINE_PATTERN:
      {
        LigmaPattern *pattern = g_value_get_object (value);

        if (text->outline_pattern != pattern)
          {
            if (text->outline_pattern)
              g_object_unref (text->outline_pattern);

            text->outline_pattern = pattern ? g_object_ref (pattern) : pattern;
          }
        break;
      }
    case PROP_OUTLINE_WIDTH:
      text->outline_width = g_value_get_double (value);
      break;
    case PROP_OUTLINE_CAP_STYLE:
      text->outline_cap_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_JOIN_STYLE:
      text->outline_join_style = g_value_get_enum (value);
      break;
    case PROP_OUTLINE_MITER_LIMIT:
      text->outline_miter_limit = g_value_get_double (value);
      break;
    case PROP_OUTLINE_ANTIALIAS:
      text->outline_antialias = g_value_get_boolean (value);
      break;
    case PROP_OUTLINE_DASH_OFFSET:
      text->outline_dash_offset = g_value_get_double (value);
      break;
    case PROP_OUTLINE_DASH_INFO:
      {
        LigmaValueArray *value_array = g_value_get_boxed (value);
        text->outline_dash_info = ligma_dash_pattern_from_value_array (value_array);
      }
      break;
    case PROP_BORDER:
      text->border = g_value_get_int (value);
      break;
    case PROP_HINTING:
      /* interpret "hinting" only if "hint-style" has its default
       * value, so we don't overwrite a serialized new hint-style with
       * a compat "hinting" that is only there for old LIGMA versions
       */
      if (text->hint_style == LIGMA_TEXT_HINT_STYLE_MEDIUM)
        text->hint_style = (g_value_get_boolean (value) ?
                            LIGMA_TEXT_HINT_STYLE_MEDIUM :
                            LIGMA_TEXT_HINT_STYLE_NONE);
      break;
    case PROP_LIGMA:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_text_dispatch_properties_changed (GObject     *object,
                                       guint        n_pspecs,
                                       GParamSpec **pspecs)
{
  G_OBJECT_CLASS (parent_class)->dispatch_properties_changed (object,
                                                              n_pspecs, pspecs);

  g_signal_emit (object, text_signals[CHANGED], 0);
}

static gint64
ligma_text_get_memsize (LigmaObject *object,
                       gint64     *gui_size)
{
  LigmaText *text    = LIGMA_TEXT (object);
  gint64    memsize = 0;

  memsize += ligma_string_get_memsize (text->text);
  memsize += ligma_string_get_memsize (text->markup);
  memsize += ligma_string_get_memsize (text->font);
  memsize += ligma_string_get_memsize (text->language);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

void
ligma_text_get_transformation (LigmaText    *text,
                              LigmaMatrix3 *matrix)
{
  g_return_if_fail (LIGMA_IS_TEXT (text));
  g_return_if_fail (matrix != NULL);

  matrix->coeff[0][0] = text->transformation.coeff[0][0];
  matrix->coeff[0][1] = text->transformation.coeff[0][1];
  matrix->coeff[0][2] = text->offset_x;

  matrix->coeff[1][0] = text->transformation.coeff[1][0];
  matrix->coeff[1][1] = text->transformation.coeff[1][1];
  matrix->coeff[1][2] = text->offset_y;

  matrix->coeff[2][0] = 0.0;
  matrix->coeff[2][1] = 0.0;
  matrix->coeff[2][2] = 1.0;
}

static gboolean
ligma_text_serialize_property (LigmaConfig       *config,
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
ligma_text_deserialize_property (LigmaConfig *object,
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
          LigmaText      *text = LIGMA_TEXT (object);
          LigmaContainer *container;
          LigmaObject    *deserialize_obj;

          if (! object_name)
            object_name = g_strdup ("");

          container = ligma_data_factory_get_container (text->ligma->pattern_factory);

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
  else if (property_id == PROP_OUTLINE_DASH_INFO)
    {
      if (ligma_scanner_parse_identifier (scanner, "NULL"))
        {
          g_value_take_boxed (value, NULL);
          return TRUE;
        }
    }

  return FALSE;
}
