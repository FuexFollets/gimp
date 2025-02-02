/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmacontext.c
 * Copyright (C) 1999-2010 Michael Natterer
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

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"
#include "libligmacolor/ligmacolor.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "ligma.h"
#include "ligma-memsize.h"
#include "ligmabrush.h"
#include "ligmabuffer.h"
#include "ligmacontainer.h"
#include "ligmacontext.h"
#include "ligmadatafactory.h"
#include "ligmadisplay.h"
#include "ligmadynamics.h"
#include "ligmaimagefile.h"
#include "ligmagradient.h"
#include "ligmaimage.h"
#include "ligmalineart.h"
#include "ligmamybrush.h"
#include "ligmapaintinfo.h"
#include "ligmapalette.h"
#include "ligmapattern.h"
#include "ligmatemplate.h"
#include "ligmatoolinfo.h"
#include "ligmatoolpreset.h"

#include "text/ligmafont.h"

#include "ligma-intl.h"


#define RGBA_EPSILON 1e-10

typedef void (* LigmaContextCopyPropFunc) (LigmaContext *src,
                                          LigmaContext *dest);

#define context_find_defined(context, prop)                              \
  while (!(((context)->defined_props) & (1 << (prop))) && (context)->parent) \
    (context) = (context)->parent

#define COPY_NAME(src, dest, member)            \
  g_free (dest->member);                        \
  dest->member = g_strdup (src->member)


/*  local function prototypes  */

static void    ligma_context_config_iface_init (LigmaConfigInterface   *iface);

static void       ligma_context_constructed    (GObject               *object);
static void       ligma_context_dispose        (GObject               *object);
static void       ligma_context_finalize       (GObject               *object);
static void       ligma_context_set_property   (GObject               *object,
                                               guint                  property_id,
                                               const GValue          *value,
                                               GParamSpec            *pspec);
static void       ligma_context_get_property   (GObject               *object,
                                               guint                  property_id,
                                               GValue                *value,
                                               GParamSpec            *pspec);
static gint64     ligma_context_get_memsize    (LigmaObject            *object,
                                               gint64                *gui_size);

static gboolean   ligma_context_serialize            (LigmaConfig       *config,
                                                     LigmaConfigWriter *writer,
                                                     gpointer          data);
static gboolean   ligma_context_deserialize          (LigmaConfig       *config,
                                                     GScanner         *scanner,
                                                     gint              nest_level,
                                                     gpointer          data);
static gboolean   ligma_context_serialize_property   (LigmaConfig       *config,
                                                     guint             property_id,
                                                     const GValue     *value,
                                                     GParamSpec       *pspec,
                                                     LigmaConfigWriter *writer);
static gboolean   ligma_context_deserialize_property (LigmaConfig       *config,
                                                     guint             property_id,
                                                     GValue           *value,
                                                     GParamSpec       *pspec,
                                                     GScanner         *scanner,
                                                     GTokenType       *expected);
static LigmaConfig * ligma_context_duplicate          (LigmaConfig       *config);
static gboolean     ligma_context_copy               (LigmaConfig       *src,
                                                     LigmaConfig       *dest,
                                                     GParamFlags       flags);

/*  image  */
static void ligma_context_image_removed       (LigmaContainer    *container,
                                              LigmaImage        *image,
                                              LigmaContext      *context);
static void ligma_context_real_set_image      (LigmaContext      *context,
                                              LigmaImage        *image);

/*  display  */
static void ligma_context_display_removed     (LigmaContainer    *container,
                                              LigmaDisplay      *display,
                                              LigmaContext      *context);
static void ligma_context_real_set_display    (LigmaContext      *context,
                                              LigmaDisplay      *display);

/*  tool  */
static void ligma_context_tool_dirty          (LigmaToolInfo     *tool_info,
                                              LigmaContext      *context);
static void ligma_context_tool_removed        (LigmaContainer    *container,
                                              LigmaToolInfo     *tool_info,
                                              LigmaContext      *context);
static void ligma_context_tool_list_thaw      (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_tool       (LigmaContext      *context,
                                              LigmaToolInfo     *tool_info);

/*  paint info  */
static void ligma_context_paint_info_dirty    (LigmaPaintInfo    *paint_info,
                                              LigmaContext      *context);
static void ligma_context_paint_info_removed  (LigmaContainer    *container,
                                              LigmaPaintInfo    *paint_info,
                                              LigmaContext      *context);
static void ligma_context_paint_info_list_thaw(LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_paint_info (LigmaContext      *context,
                                              LigmaPaintInfo    *paint_info);

/*  foreground  */
static void ligma_context_real_set_foreground (LigmaContext      *context,
                                              const LigmaRGB    *color);

/*  background  */
static void ligma_context_real_set_background (LigmaContext      *context,
                                              const LigmaRGB    *color);

/*  opacity  */
static void ligma_context_real_set_opacity    (LigmaContext      *context,
                                              gdouble           opacity);

/*  paint mode  */
static void ligma_context_real_set_paint_mode (LigmaContext      *context,
                                              LigmaLayerMode     paint_mode);

/*  brush  */
static void ligma_context_brush_dirty         (LigmaBrush        *brush,
                                              LigmaContext      *context);
static void ligma_context_brush_removed       (LigmaContainer    *brush_list,
                                              LigmaBrush        *brush,
                                              LigmaContext      *context);
static void ligma_context_brush_list_thaw     (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_brush      (LigmaContext      *context,
                                              LigmaBrush        *brush);

/*  dynamics  */

static void ligma_context_dynamics_dirty      (LigmaDynamics     *dynamics,
                                              LigmaContext      *context);
static void ligma_context_dynamics_removed    (LigmaContainer    *container,
                                              LigmaDynamics     *dynamics,
                                              LigmaContext      *context);
static void ligma_context_dynamics_list_thaw  (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_dynamics   (LigmaContext      *context,
                                              LigmaDynamics     *dynamics);

/*  mybrush  */
static void ligma_context_mybrush_dirty       (LigmaMybrush      *brush,
                                              LigmaContext      *context);
static void ligma_context_mybrush_removed     (LigmaContainer    *brush_list,
                                              LigmaMybrush      *brush,
                                              LigmaContext      *context);
static void ligma_context_mybrush_list_thaw   (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_mybrush    (LigmaContext      *context,
                                              LigmaMybrush      *brush);

/*  pattern  */
static void ligma_context_pattern_dirty       (LigmaPattern      *pattern,
                                              LigmaContext      *context);
static void ligma_context_pattern_removed     (LigmaContainer    *container,
                                              LigmaPattern      *pattern,
                                              LigmaContext      *context);
static void ligma_context_pattern_list_thaw   (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_pattern    (LigmaContext      *context,
                                              LigmaPattern      *pattern);

/*  gradient  */
static void ligma_context_gradient_dirty      (LigmaGradient     *gradient,
                                              LigmaContext      *context);
static void ligma_context_gradient_removed    (LigmaContainer    *container,
                                              LigmaGradient     *gradient,
                                              LigmaContext      *context);
static void ligma_context_gradient_list_thaw  (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_gradient   (LigmaContext      *context,
                                              LigmaGradient     *gradient);

/*  palette  */
static void ligma_context_palette_dirty       (LigmaPalette      *palette,
                                              LigmaContext      *context);
static void ligma_context_palette_removed     (LigmaContainer    *container,
                                              LigmaPalette      *palette,
                                              LigmaContext      *context);
static void ligma_context_palette_list_thaw   (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_palette    (LigmaContext      *context,
                                              LigmaPalette      *palette);

/*  font  */
static void ligma_context_font_dirty          (LigmaFont         *font,
                                              LigmaContext      *context);
static void ligma_context_font_removed        (LigmaContainer    *container,
                                              LigmaFont         *font,
                                              LigmaContext      *context);
static void ligma_context_font_list_thaw      (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_font       (LigmaContext      *context,
                                              LigmaFont         *font);

/*  tool preset  */
static void ligma_context_tool_preset_dirty     (LigmaToolPreset   *tool_preset,
                                                LigmaContext      *context);
static void ligma_context_tool_preset_removed   (LigmaContainer    *container,
                                                LigmaToolPreset   *tool_preset,
                                                LigmaContext      *context);
static void ligma_context_tool_preset_list_thaw (LigmaContainer    *container,
                                                LigmaContext      *context);
static void ligma_context_real_set_tool_preset  (LigmaContext      *context,
                                                LigmaToolPreset   *tool_preset);

/*  buffer  */
static void ligma_context_buffer_dirty        (LigmaBuffer       *buffer,
                                              LigmaContext      *context);
static void ligma_context_buffer_removed      (LigmaContainer    *container,
                                              LigmaBuffer       *buffer,
                                              LigmaContext      *context);
static void ligma_context_buffer_list_thaw    (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_buffer     (LigmaContext      *context,
                                              LigmaBuffer       *buffer);

/*  imagefile  */
static void ligma_context_imagefile_dirty     (LigmaImagefile    *imagefile,
                                              LigmaContext      *context);
static void ligma_context_imagefile_removed   (LigmaContainer    *container,
                                              LigmaImagefile    *imagefile,
                                              LigmaContext      *context);
static void ligma_context_imagefile_list_thaw (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_imagefile  (LigmaContext      *context,
                                              LigmaImagefile    *imagefile);

/*  template  */
static void ligma_context_template_dirty      (LigmaTemplate     *template,
                                              LigmaContext      *context);
static void ligma_context_template_removed    (LigmaContainer    *container,
                                              LigmaTemplate     *template,
                                              LigmaContext      *context);
static void ligma_context_template_list_thaw  (LigmaContainer    *container,
                                              LigmaContext      *context);
static void ligma_context_real_set_template   (LigmaContext      *context,
                                              LigmaTemplate     *template);


/*  line art  */
static gboolean ligma_context_free_line_art   (LigmaContext      *context);


/*  utilities  */
static gpointer ligma_context_find_object     (LigmaContext      *context,
                                              LigmaContainer    *container,
                                              const gchar      *object_name,
                                              gpointer          standard_object);


/*  properties & signals  */

enum
{
  LIGMA_CONTEXT_PROP_0,
  LIGMA_CONTEXT_PROP_LIGMA

  /*  remaining values are in core-enums.h  (LigmaContextPropType)  */
};

enum
{
  DUMMY_0,
  DUMMY_1,
  IMAGE_CHANGED,
  DISPLAY_CHANGED,
  TOOL_CHANGED,
  PAINT_INFO_CHANGED,
  FOREGROUND_CHANGED,
  BACKGROUND_CHANGED,
  OPACITY_CHANGED,
  PAINT_MODE_CHANGED,
  BRUSH_CHANGED,
  DYNAMICS_CHANGED,
  MYBRUSH_CHANGED,
  PATTERN_CHANGED,
  GRADIENT_CHANGED,
  PALETTE_CHANGED,
  FONT_CHANGED,
  TOOL_PRESET_CHANGED,
  BUFFER_CHANGED,
  IMAGEFILE_CHANGED,
  TEMPLATE_CHANGED,
  PROP_NAME_CHANGED,
  LAST_SIGNAL
};

static const gchar * const ligma_context_prop_names[] =
{
  NULL, /* PROP_0 */
  "ligma",
  "image",
  "display",
  "tool",
  "paint-info",
  "foreground",
  "background",
  "opacity",
  "paint-mode",
  "brush",
  "dynamics",
  "mybrush",
  "pattern",
  "gradient",
  "palette",
  "font",
  "tool-preset",
  "buffer",
  "imagefile",
  "template"
};

static GType ligma_context_prop_types[] =
{
  G_TYPE_NONE, /* PROP_0    */
  G_TYPE_NONE, /* PROP_LIGMA */
  0,
  G_TYPE_NONE,
  0,
  0,
  G_TYPE_NONE,
  G_TYPE_NONE,
  G_TYPE_NONE,
  G_TYPE_NONE,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0
};


G_DEFINE_TYPE_WITH_CODE (LigmaContext, ligma_context, LIGMA_TYPE_VIEWABLE,
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG,
                                                ligma_context_config_iface_init))

#define parent_class ligma_context_parent_class

static LigmaConfigInterface *parent_config_iface = NULL;

static guint ligma_context_signals[LAST_SIGNAL] = { 0 };


static void
ligma_context_class_init (LigmaContextClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  LigmaObjectClass *ligma_object_class = LIGMA_OBJECT_CLASS (klass);
  LigmaRGB          black;
  LigmaRGB          white;

  ligma_rgba_set (&black, 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&white, 1.0, 1.0, 1.0, LIGMA_OPACITY_OPAQUE);

  ligma_context_signals[IMAGE_CHANGED] =
    g_signal_new ("image-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, image_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_IMAGE);

  ligma_context_signals[DISPLAY_CHANGED] =
    g_signal_new ("display-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, display_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DISPLAY);

  ligma_context_signals[TOOL_CHANGED] =
    g_signal_new ("tool-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, tool_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_TOOL_INFO);

  ligma_context_signals[PAINT_INFO_CHANGED] =
    g_signal_new ("paint-info-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, paint_info_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_PAINT_INFO);

  ligma_context_signals[FOREGROUND_CHANGED] =
    g_signal_new ("foreground-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, foreground_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_RGB | G_SIGNAL_TYPE_STATIC_SCOPE);

  ligma_context_signals[BACKGROUND_CHANGED] =
    g_signal_new ("background-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, background_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_RGB | G_SIGNAL_TYPE_STATIC_SCOPE);

  ligma_context_signals[OPACITY_CHANGED] =
    g_signal_new ("opacity-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, opacity_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_DOUBLE);

  ligma_context_signals[PAINT_MODE_CHANGED] =
    g_signal_new ("paint-mode-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, paint_mode_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_LAYER_MODE);

  ligma_context_signals[BRUSH_CHANGED] =
    g_signal_new ("brush-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, brush_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_BRUSH);

  ligma_context_signals[DYNAMICS_CHANGED] =
    g_signal_new ("dynamics-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, dynamics_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_DYNAMICS);

  ligma_context_signals[MYBRUSH_CHANGED] =
    g_signal_new ("mybrush-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, mybrush_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_MYBRUSH);

  ligma_context_signals[PATTERN_CHANGED] =
    g_signal_new ("pattern-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, pattern_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_PATTERN);

  ligma_context_signals[GRADIENT_CHANGED] =
    g_signal_new ("gradient-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, gradient_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_GRADIENT);

  ligma_context_signals[PALETTE_CHANGED] =
    g_signal_new ("palette-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, palette_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_PALETTE);

  ligma_context_signals[FONT_CHANGED] =
    g_signal_new ("font-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, font_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_FONT);

  ligma_context_signals[TOOL_PRESET_CHANGED] =
    g_signal_new ("tool-preset-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, tool_preset_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_TOOL_PRESET);

  ligma_context_signals[BUFFER_CHANGED] =
    g_signal_new ("buffer-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, buffer_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_BUFFER);

  ligma_context_signals[IMAGEFILE_CHANGED] =
    g_signal_new ("imagefile-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, imagefile_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_IMAGEFILE);

  ligma_context_signals[TEMPLATE_CHANGED] =
    g_signal_new ("template-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, template_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  LIGMA_TYPE_TEMPLATE);

  ligma_context_signals[PROP_NAME_CHANGED] =
    g_signal_new ("prop-name-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (LigmaContextClass, prop_name_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);

  object_class->constructed      = ligma_context_constructed;
  object_class->set_property     = ligma_context_set_property;
  object_class->get_property     = ligma_context_get_property;
  object_class->dispose          = ligma_context_dispose;
  object_class->finalize         = ligma_context_finalize;

  ligma_object_class->get_memsize = ligma_context_get_memsize;

  klass->image_changed           = NULL;
  klass->display_changed         = NULL;
  klass->tool_changed            = NULL;
  klass->paint_info_changed      = NULL;
  klass->foreground_changed      = NULL;
  klass->background_changed      = NULL;
  klass->opacity_changed         = NULL;
  klass->paint_mode_changed      = NULL;
  klass->brush_changed           = NULL;
  klass->dynamics_changed        = NULL;
  klass->mybrush_changed         = NULL;
  klass->pattern_changed         = NULL;
  klass->gradient_changed        = NULL;
  klass->palette_changed         = NULL;
  klass->font_changed            = NULL;
  klass->tool_preset_changed     = NULL;
  klass->buffer_changed          = NULL;
  klass->imagefile_changed       = NULL;
  klass->template_changed        = NULL;
  klass->prop_name_changed       = NULL;

  ligma_context_prop_types[LIGMA_CONTEXT_PROP_IMAGE]       = LIGMA_TYPE_IMAGE;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_TOOL]        = LIGMA_TYPE_TOOL_INFO;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_PAINT_INFO]  = LIGMA_TYPE_PAINT_INFO;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_BRUSH]       = LIGMA_TYPE_BRUSH;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_DYNAMICS]    = LIGMA_TYPE_DYNAMICS;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_MYBRUSH]     = LIGMA_TYPE_MYBRUSH;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_PATTERN]     = LIGMA_TYPE_PATTERN;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_GRADIENT]    = LIGMA_TYPE_GRADIENT;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_PALETTE]     = LIGMA_TYPE_PALETTE;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_FONT]        = LIGMA_TYPE_FONT;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_TOOL_PRESET] = LIGMA_TYPE_TOOL_PRESET;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_BUFFER]      = LIGMA_TYPE_BUFFER;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_IMAGEFILE]   = LIGMA_TYPE_IMAGEFILE;
  ligma_context_prop_types[LIGMA_CONTEXT_PROP_TEMPLATE]    = LIGMA_TYPE_TEMPLATE;

  g_object_class_install_property (object_class, LIGMA_CONTEXT_PROP_LIGMA,
                                   g_param_spec_object ("ligma",
                                                        NULL, NULL,
                                                        LIGMA_TYPE_LIGMA,
                                                        LIGMA_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (object_class, LIGMA_CONTEXT_PROP_IMAGE,
                                   g_param_spec_object (ligma_context_prop_names[LIGMA_CONTEXT_PROP_IMAGE],
                                                        NULL, NULL,
                                                        LIGMA_TYPE_IMAGE,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, LIGMA_CONTEXT_PROP_DISPLAY,
                                   g_param_spec_object (ligma_context_prop_names[LIGMA_CONTEXT_PROP_DISPLAY],
                                                        NULL, NULL,
                                                        LIGMA_TYPE_DISPLAY,
                                                        LIGMA_PARAM_READWRITE));

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_TOOL,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_TOOL],
                           NULL, NULL,
                           LIGMA_TYPE_TOOL_INFO,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_PAINT_INFO,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_PAINT_INFO],
                           NULL, NULL,
                           LIGMA_TYPE_PAINT_INFO,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_RGB (object_class, LIGMA_CONTEXT_PROP_FOREGROUND,
                        ligma_context_prop_names[LIGMA_CONTEXT_PROP_FOREGROUND],
                        _("Foreground"),
                        _("Foreground color"),
                         FALSE, &black,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_RGB (object_class, LIGMA_CONTEXT_PROP_BACKGROUND,
                        ligma_context_prop_names[LIGMA_CONTEXT_PROP_BACKGROUND],
                        _("Background"),
                        _("Background color"),
                        FALSE, &white,
                        LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_DOUBLE (object_class, LIGMA_CONTEXT_PROP_OPACITY,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_OPACITY],
                           _("Opacity"),
                           _("Opacity"),
                           LIGMA_OPACITY_TRANSPARENT,
                           LIGMA_OPACITY_OPAQUE,
                           LIGMA_OPACITY_OPAQUE,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_ENUM (object_class, LIGMA_CONTEXT_PROP_PAINT_MODE,
                         ligma_context_prop_names[LIGMA_CONTEXT_PROP_PAINT_MODE],
                         _("Paint Mode"),
                         _("Paint Mode"),
                         LIGMA_TYPE_LAYER_MODE,
                         LIGMA_LAYER_MODE_NORMAL,
                         LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_BRUSH,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_BRUSH],
                           _("Brush"),
                           _("Brush"),
                           LIGMA_TYPE_BRUSH,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_DYNAMICS,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_DYNAMICS],
                           _("Dynamics"),
                           _("Paint dynamics"),
                           LIGMA_TYPE_DYNAMICS,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_MYBRUSH,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_MYBRUSH],
                           _("MyPaint Brush"),
                           _("MyPaint Brush"),
                           LIGMA_TYPE_MYBRUSH,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_PATTERN,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_PATTERN],
                           _("Pattern"),
                           _("Pattern"),
                           LIGMA_TYPE_PATTERN,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_GRADIENT,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_GRADIENT],
                           _("Gradient"),
                           _("Gradient"),
                           LIGMA_TYPE_GRADIENT,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_PALETTE,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_PALETTE],
                           _("Palette"),
                           _("Palette"),
                           LIGMA_TYPE_PALETTE,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_FONT,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_FONT],
                           _("Font"),
                           _("Font"),
                           LIGMA_TYPE_FONT,
                           LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, LIGMA_CONTEXT_PROP_TOOL_PRESET,
                           ligma_context_prop_names[LIGMA_CONTEXT_PROP_TOOL_PRESET],
                           _("Tool Preset"),
                           _("Tool Preset"),
                           LIGMA_TYPE_TOOL_PRESET,
                           LIGMA_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class, LIGMA_CONTEXT_PROP_BUFFER,
                                   g_param_spec_object (ligma_context_prop_names[LIGMA_CONTEXT_PROP_BUFFER],
                                                        NULL, NULL,
                                                        LIGMA_TYPE_BUFFER,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, LIGMA_CONTEXT_PROP_IMAGEFILE,
                                   g_param_spec_object (ligma_context_prop_names[LIGMA_CONTEXT_PROP_IMAGEFILE],
                                                        NULL, NULL,
                                                        LIGMA_TYPE_IMAGEFILE,
                                                        LIGMA_PARAM_READWRITE));

  g_object_class_install_property (object_class, LIGMA_CONTEXT_PROP_TEMPLATE,
                                   g_param_spec_object (ligma_context_prop_names[LIGMA_CONTEXT_PROP_TEMPLATE],
                                                        NULL, NULL,
                                                        LIGMA_TYPE_TEMPLATE,
                                                        LIGMA_PARAM_READWRITE));
}

static void
ligma_context_init (LigmaContext *context)
{
  context->ligma            = NULL;

  context->parent          = NULL;

  context->defined_props   = LIGMA_CONTEXT_PROP_MASK_ALL;
  context->serialize_props = LIGMA_CONTEXT_PROP_MASK_ALL;

  context->image           = NULL;
  context->display         = NULL;

  context->tool_info       = NULL;
  context->tool_name       = NULL;

  context->paint_info      = NULL;
  context->paint_name      = NULL;

  context->brush           = NULL;
  context->brush_name      = NULL;

  context->dynamics        = NULL;
  context->dynamics_name   = NULL;

  context->mybrush         = NULL;
  context->mybrush_name    = NULL;

  context->pattern         = NULL;
  context->pattern_name    = NULL;

  context->gradient        = NULL;
  context->gradient_name   = NULL;

  context->palette         = NULL;
  context->palette_name    = NULL;

  context->font            = NULL;
  context->font_name       = NULL;

  context->tool_preset      = NULL;
  context->tool_preset_name = NULL;

  context->buffer          = NULL;
  context->buffer_name     = NULL;

  context->imagefile       = NULL;
  context->imagefile_name  = NULL;

  context->template        = NULL;
  context->template_name   = NULL;

  context->line_art            = NULL;
  context->line_art_timeout_id = 0;
}

static void
ligma_context_config_iface_init (LigmaConfigInterface *iface)
{
  parent_config_iface = g_type_interface_peek_parent (iface);

  if (! parent_config_iface)
    parent_config_iface = g_type_default_interface_peek (LIGMA_TYPE_CONFIG);

  iface->serialize            = ligma_context_serialize;
  iface->deserialize          = ligma_context_deserialize;
  iface->serialize_property   = ligma_context_serialize_property;
  iface->deserialize_property = ligma_context_deserialize_property;
  iface->duplicate            = ligma_context_duplicate;
  iface->copy                 = ligma_context_copy;
}

static void
ligma_context_constructed (GObject *object)
{
  Ligma          *ligma;
  LigmaContainer *container;

  G_OBJECT_CLASS (parent_class)->constructed (object);

  ligma = LIGMA_CONTEXT (object)->ligma;

  ligma_assert (LIGMA_IS_LIGMA (ligma));

  ligma->context_list = g_list_prepend (ligma->context_list, object);

  g_signal_connect_object (ligma->images, "remove",
                           G_CALLBACK (ligma_context_image_removed),
                           object, 0);
  g_signal_connect_object (ligma->displays, "remove",
                           G_CALLBACK (ligma_context_display_removed),
                           object, 0);

  g_signal_connect_object (ligma->tool_info_list, "remove",
                           G_CALLBACK (ligma_context_tool_removed),
                           object, 0);
  g_signal_connect_object (ligma->tool_info_list, "thaw",
                           G_CALLBACK (ligma_context_tool_list_thaw),
                           object, 0);

  g_signal_connect_object (ligma->paint_info_list, "remove",
                           G_CALLBACK (ligma_context_paint_info_removed),
                           object, 0);
  g_signal_connect_object (ligma->paint_info_list, "thaw",
                           G_CALLBACK (ligma_context_paint_info_list_thaw),
                           object, 0);

  container = ligma_data_factory_get_container (ligma->brush_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_context_brush_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (ligma_context_brush_list_thaw),
                           object, 0);

  container = ligma_data_factory_get_container (ligma->dynamics_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_context_dynamics_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (ligma_context_dynamics_list_thaw),
                           object, 0);

  container = ligma_data_factory_get_container (ligma->mybrush_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_context_mybrush_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (ligma_context_mybrush_list_thaw),
                           object, 0);

  container = ligma_data_factory_get_container (ligma->pattern_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_context_pattern_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (ligma_context_pattern_list_thaw),
                           object, 0);

  container = ligma_data_factory_get_container (ligma->gradient_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_context_gradient_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (ligma_context_gradient_list_thaw),
                           object, 0);

  container = ligma_data_factory_get_container (ligma->palette_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_context_palette_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (ligma_context_palette_list_thaw),
                           object, 0);

  container = ligma_data_factory_get_container (ligma->font_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_context_font_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (ligma_context_font_list_thaw),
                           object, 0);

  container = ligma_data_factory_get_container (ligma->tool_preset_factory);
  g_signal_connect_object (container, "remove",
                           G_CALLBACK (ligma_context_tool_preset_removed),
                           object, 0);
  g_signal_connect_object (container, "thaw",
                           G_CALLBACK (ligma_context_tool_preset_list_thaw),
                           object, 0);

  g_signal_connect_object (ligma->named_buffers, "remove",
                           G_CALLBACK (ligma_context_buffer_removed),
                           object, 0);
  g_signal_connect_object (ligma->named_buffers, "thaw",
                           G_CALLBACK (ligma_context_buffer_list_thaw),
                           object, 0);

  g_signal_connect_object (ligma->documents, "remove",
                           G_CALLBACK (ligma_context_imagefile_removed),
                           object, 0);
  g_signal_connect_object (ligma->documents, "thaw",
                           G_CALLBACK (ligma_context_imagefile_list_thaw),
                           object, 0);

  g_signal_connect_object (ligma->templates, "remove",
                           G_CALLBACK (ligma_context_template_removed),
                           object, 0);
  g_signal_connect_object (ligma->templates, "thaw",
                           G_CALLBACK (ligma_context_template_list_thaw),
                           object, 0);

  ligma_context_set_paint_info (LIGMA_CONTEXT (object),
                               ligma_paint_info_get_standard (ligma));
}

static void
ligma_context_dispose (GObject *object)
{
  LigmaContext *context = LIGMA_CONTEXT (object);

  ligma_context_set_parent (context, NULL);

  if (context->ligma)
    {
      context->ligma->context_list = g_list_remove (context->ligma->context_list,
                                                   context);
      context->ligma = NULL;
    }

  g_clear_object (&context->tool_info);
  g_clear_object (&context->paint_info);
  g_clear_object (&context->brush);
  g_clear_object (&context->dynamics);
  g_clear_object (&context->mybrush);
  g_clear_object (&context->pattern);
  g_clear_object (&context->gradient);
  g_clear_object (&context->palette);
  g_clear_object (&context->font);
  g_clear_object (&context->tool_preset);
  g_clear_object (&context->buffer);
  g_clear_object (&context->imagefile);
  g_clear_object (&context->template);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
ligma_context_finalize (GObject *object)
{
  LigmaContext *context = LIGMA_CONTEXT (object);

  context->parent  = NULL;
  context->image   = NULL;
  context->display = NULL;

  g_clear_pointer (&context->tool_name,        g_free);
  g_clear_pointer (&context->paint_name,       g_free);
  g_clear_pointer (&context->brush_name,       g_free);
  g_clear_pointer (&context->dynamics_name,    g_free);
  g_clear_pointer (&context->mybrush_name,     g_free);
  g_clear_pointer (&context->pattern_name,     g_free);
  g_clear_pointer (&context->gradient_name,    g_free);
  g_clear_pointer (&context->palette_name,     g_free);
  g_clear_pointer (&context->font_name,        g_free);
  g_clear_pointer (&context->tool_preset_name, g_free);
  g_clear_pointer (&context->buffer_name,      g_free);
  g_clear_pointer (&context->imagefile_name,   g_free);
  g_clear_pointer (&context->template_name,    g_free);

  g_clear_object (&context->line_art);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_context_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  LigmaContext *context = LIGMA_CONTEXT (object);

  switch (property_id)
    {
    case LIGMA_CONTEXT_PROP_LIGMA:
      context->ligma = g_value_get_object (value);
      break;
    case LIGMA_CONTEXT_PROP_IMAGE:
      ligma_context_set_image (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_DISPLAY:
      ligma_context_set_display (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_TOOL:
      ligma_context_set_tool (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_PAINT_INFO:
      ligma_context_set_paint_info (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_FOREGROUND:
      ligma_context_set_foreground (context, g_value_get_boxed (value));
      break;
    case LIGMA_CONTEXT_PROP_BACKGROUND:
      ligma_context_set_background (context, g_value_get_boxed (value));
      break;
    case LIGMA_CONTEXT_PROP_OPACITY:
      ligma_context_set_opacity (context, g_value_get_double (value));
      break;
    case LIGMA_CONTEXT_PROP_PAINT_MODE:
      ligma_context_set_paint_mode (context, g_value_get_enum (value));
      break;
    case LIGMA_CONTEXT_PROP_BRUSH:
      ligma_context_set_brush (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_DYNAMICS:
      ligma_context_set_dynamics (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_MYBRUSH:
      ligma_context_set_mybrush (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_PATTERN:
      ligma_context_set_pattern (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_GRADIENT:
      ligma_context_set_gradient (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_PALETTE:
      ligma_context_set_palette (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_FONT:
      ligma_context_set_font (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_TOOL_PRESET:
      ligma_context_set_tool_preset (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_BUFFER:
      ligma_context_set_buffer (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_IMAGEFILE:
      ligma_context_set_imagefile (context, g_value_get_object (value));
      break;
    case LIGMA_CONTEXT_PROP_TEMPLATE:
      ligma_context_set_template (context, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_context_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  LigmaContext *context = LIGMA_CONTEXT (object);

  switch (property_id)
    {
    case LIGMA_CONTEXT_PROP_LIGMA:
      g_value_set_object (value, context->ligma);
      break;
    case LIGMA_CONTEXT_PROP_IMAGE:
      g_value_set_object (value, ligma_context_get_image (context));
      break;
    case LIGMA_CONTEXT_PROP_DISPLAY:
      g_value_set_object (value, ligma_context_get_display (context));
      break;
    case LIGMA_CONTEXT_PROP_TOOL:
      g_value_set_object (value, ligma_context_get_tool (context));
      break;
    case LIGMA_CONTEXT_PROP_PAINT_INFO:
      g_value_set_object (value, ligma_context_get_paint_info (context));
      break;
    case LIGMA_CONTEXT_PROP_FOREGROUND:
      {
        LigmaRGB color;

        ligma_context_get_foreground (context, &color);
        g_value_set_boxed (value, &color);
      }
      break;
    case LIGMA_CONTEXT_PROP_BACKGROUND:
      {
        LigmaRGB color;

        ligma_context_get_background (context, &color);
        g_value_set_boxed (value, &color);
      }
      break;
    case LIGMA_CONTEXT_PROP_OPACITY:
      g_value_set_double (value, ligma_context_get_opacity (context));
      break;
    case LIGMA_CONTEXT_PROP_PAINT_MODE:
      g_value_set_enum (value, ligma_context_get_paint_mode (context));
      break;
    case LIGMA_CONTEXT_PROP_BRUSH:
      g_value_set_object (value, ligma_context_get_brush (context));
      break;
    case LIGMA_CONTEXT_PROP_DYNAMICS:
      g_value_set_object (value, ligma_context_get_dynamics (context));
      break;
    case LIGMA_CONTEXT_PROP_MYBRUSH:
      g_value_set_object (value, ligma_context_get_mybrush (context));
      break;
    case LIGMA_CONTEXT_PROP_PATTERN:
      g_value_set_object (value, ligma_context_get_pattern (context));
      break;
    case LIGMA_CONTEXT_PROP_GRADIENT:
      g_value_set_object (value, ligma_context_get_gradient (context));
      break;
    case LIGMA_CONTEXT_PROP_PALETTE:
      g_value_set_object (value, ligma_context_get_palette (context));
      break;
    case LIGMA_CONTEXT_PROP_FONT:
      g_value_set_object (value, ligma_context_get_font (context));
      break;
    case LIGMA_CONTEXT_PROP_TOOL_PRESET:
      g_value_set_object (value, ligma_context_get_tool_preset (context));
      break;
    case LIGMA_CONTEXT_PROP_BUFFER:
      g_value_set_object (value, ligma_context_get_buffer (context));
      break;
    case LIGMA_CONTEXT_PROP_IMAGEFILE:
      g_value_set_object (value, ligma_context_get_imagefile (context));
      break;
    case LIGMA_CONTEXT_PROP_TEMPLATE:
      g_value_set_object (value, ligma_context_get_template (context));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
ligma_context_get_memsize (LigmaObject *object,
                          gint64     *gui_size)
{
  LigmaContext *context = LIGMA_CONTEXT (object);
  gint64       memsize = 0;

  memsize += ligma_string_get_memsize (context->tool_name);
  memsize += ligma_string_get_memsize (context->paint_name);
  memsize += ligma_string_get_memsize (context->brush_name);
  memsize += ligma_string_get_memsize (context->dynamics_name);
  memsize += ligma_string_get_memsize (context->mybrush_name);
  memsize += ligma_string_get_memsize (context->pattern_name);
  memsize += ligma_string_get_memsize (context->palette_name);
  memsize += ligma_string_get_memsize (context->font_name);
  memsize += ligma_string_get_memsize (context->tool_preset_name);
  memsize += ligma_string_get_memsize (context->buffer_name);
  memsize += ligma_string_get_memsize (context->imagefile_name);
  memsize += ligma_string_get_memsize (context->template_name);

  return memsize + LIGMA_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
ligma_context_serialize (LigmaConfig       *config,
                        LigmaConfigWriter *writer,
                        gpointer          data)
{
  return ligma_config_serialize_changed_properties (config, writer);
}

static gboolean
ligma_context_deserialize (LigmaConfig *config,
                          GScanner   *scanner,
                          gint        nest_level,
                          gpointer    data)
{
  LigmaContext   *context        = LIGMA_CONTEXT (config);
  LigmaLayerMode  old_paint_mode = context->paint_mode;
  gboolean       success;

  success = ligma_config_deserialize_properties (config, scanner, nest_level);

  if (context->paint_mode != old_paint_mode)
    {
      if (context->paint_mode == LIGMA_LAYER_MODE_OVERLAY_LEGACY)
        g_object_set (context,
                      "paint-mode", LIGMA_LAYER_MODE_SOFTLIGHT_LEGACY,
                      NULL);
    }

  return success;
}

static gboolean
ligma_context_serialize_property (LigmaConfig       *config,
                                 guint             property_id,
                                 const GValue     *value,
                                 GParamSpec       *pspec,
                                 LigmaConfigWriter *writer)
{
  LigmaContext *context = LIGMA_CONTEXT (config);
  LigmaObject  *serialize_obj;

  /*  serialize nothing if the property is not in serialize_props  */
  if (! ((1 << property_id) & context->serialize_props))
    return TRUE;

  switch (property_id)
    {
    case LIGMA_CONTEXT_PROP_TOOL:
    case LIGMA_CONTEXT_PROP_PAINT_INFO:
    case LIGMA_CONTEXT_PROP_BRUSH:
    case LIGMA_CONTEXT_PROP_DYNAMICS:
    case LIGMA_CONTEXT_PROP_MYBRUSH:
    case LIGMA_CONTEXT_PROP_PATTERN:
    case LIGMA_CONTEXT_PROP_GRADIENT:
    case LIGMA_CONTEXT_PROP_PALETTE:
    case LIGMA_CONTEXT_PROP_FONT:
    case LIGMA_CONTEXT_PROP_TOOL_PRESET:
      serialize_obj = g_value_get_object (value);
      break;

    default:
      return FALSE;
    }

  ligma_config_writer_open (writer, pspec->name);

  if (serialize_obj)
    ligma_config_writer_string (writer, ligma_object_get_name (serialize_obj));
  else
    ligma_config_writer_print (writer, "NULL", 4);

  ligma_config_writer_close (writer);

  return TRUE;
}

static gboolean
ligma_context_deserialize_property (LigmaConfig *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec,
                                   GScanner   *scanner,
                                   GTokenType *expected)
{
  LigmaContext   *context = LIGMA_CONTEXT (object);
  LigmaContainer *container;
  gpointer       standard;
  gchar        **name_loc;
  gchar         *object_name;

  switch (property_id)
    {
    case LIGMA_CONTEXT_PROP_TOOL:
      container = context->ligma->tool_info_list;
      standard  = ligma_tool_info_get_standard (context->ligma);
      name_loc  = &context->tool_name;
      break;

    case LIGMA_CONTEXT_PROP_PAINT_INFO:
      container = context->ligma->paint_info_list;
      standard  = ligma_paint_info_get_standard (context->ligma);
      name_loc  = &context->paint_name;
      break;

    case LIGMA_CONTEXT_PROP_BRUSH:
      container = ligma_data_factory_get_container (context->ligma->brush_factory);
      standard  = ligma_brush_get_standard (context);
      name_loc  = &context->brush_name;
      break;

    case LIGMA_CONTEXT_PROP_DYNAMICS:
      container = ligma_data_factory_get_container (context->ligma->dynamics_factory);
      standard  = ligma_dynamics_get_standard (context);
      name_loc  = &context->dynamics_name;
      break;

    case LIGMA_CONTEXT_PROP_MYBRUSH:
      container = ligma_data_factory_get_container (context->ligma->mybrush_factory);
      standard  = ligma_mybrush_get_standard (context);
      name_loc  = &context->mybrush_name;
      break;

    case LIGMA_CONTEXT_PROP_PATTERN:
      container = ligma_data_factory_get_container (context->ligma->pattern_factory);
      standard  = ligma_pattern_get_standard (context);
      name_loc  = &context->pattern_name;
      break;

    case LIGMA_CONTEXT_PROP_GRADIENT:
      container = ligma_data_factory_get_container (context->ligma->gradient_factory);
      standard  = ligma_gradient_get_standard (context);
      name_loc  = &context->gradient_name;
      break;

    case LIGMA_CONTEXT_PROP_PALETTE:
      container = ligma_data_factory_get_container (context->ligma->palette_factory);
      standard  = ligma_palette_get_standard (context);
      name_loc  = &context->palette_name;
      break;

    case LIGMA_CONTEXT_PROP_FONT:
      container = ligma_data_factory_get_container (context->ligma->font_factory);
      standard  = ligma_font_get_standard ();
      name_loc  = &context->font_name;
      break;

    case LIGMA_CONTEXT_PROP_TOOL_PRESET:
      container = ligma_data_factory_get_container (context->ligma->tool_preset_factory);
      standard  = NULL;
      name_loc  = &context->tool_preset_name;
      break;

    default:
      return FALSE;
    }

  if (ligma_scanner_parse_identifier (scanner, "NULL"))
    {
      g_value_set_object (value, NULL);
    }
  else if (ligma_scanner_parse_string (scanner, &object_name))
    {
      LigmaObject *deserialize_obj;

      if (! object_name)
        object_name = g_strdup ("");

      deserialize_obj = ligma_container_get_child_by_name (container,
                                                          object_name);

      if (! deserialize_obj)
        {
          g_value_set_object (value, standard);

          g_free (*name_loc);
          *name_loc = g_strdup (object_name);
        }
      else
        {
          g_value_set_object (value, deserialize_obj);
        }

      g_free (object_name);
    }
  else
    {
      *expected = G_TOKEN_STRING;
    }

  return TRUE;
}

static LigmaConfig *
ligma_context_duplicate (LigmaConfig *config)
{
  LigmaContext *context = LIGMA_CONTEXT (config);
  LigmaContext *new;

  new = LIGMA_CONTEXT (parent_config_iface->duplicate (config));

  COPY_NAME (context, new, tool_name);
  COPY_NAME (context, new, paint_name);
  COPY_NAME (context, new, brush_name);
  COPY_NAME (context, new, dynamics_name);
  COPY_NAME (context, new, mybrush_name);
  COPY_NAME (context, new, pattern_name);
  COPY_NAME (context, new, gradient_name);
  COPY_NAME (context, new, palette_name);
  COPY_NAME (context, new, font_name);
  COPY_NAME (context, new, tool_preset_name);
  COPY_NAME (context, new, buffer_name);
  COPY_NAME (context, new, imagefile_name);
  COPY_NAME (context, new, template_name);

  return LIGMA_CONFIG (new);
}

static gboolean
ligma_context_copy (LigmaConfig  *src,
                   LigmaConfig  *dest,
                   GParamFlags  flags)
{
  LigmaContext *src_context  = LIGMA_CONTEXT (src);
  LigmaContext *dest_context = LIGMA_CONTEXT (dest);
  gboolean     success      = parent_config_iface->copy (src, dest, flags);

  COPY_NAME (src_context, dest_context, tool_name);
  COPY_NAME (src_context, dest_context, paint_name);
  COPY_NAME (src_context, dest_context, brush_name);
  COPY_NAME (src_context, dest_context, dynamics_name);
  COPY_NAME (src_context, dest_context, mybrush_name);
  COPY_NAME (src_context, dest_context, pattern_name);
  COPY_NAME (src_context, dest_context, gradient_name);
  COPY_NAME (src_context, dest_context, palette_name);
  COPY_NAME (src_context, dest_context, font_name);
  COPY_NAME (src_context, dest_context, tool_preset_name);
  COPY_NAME (src_context, dest_context, buffer_name);
  COPY_NAME (src_context, dest_context, imagefile_name);
  COPY_NAME (src_context, dest_context, template_name);

  return success;
}


/*****************************************************************************/
/*  public functions  ********************************************************/

LigmaContext *
ligma_context_new (Ligma        *ligma,
                  const gchar *name,
                  LigmaContext *template)
{
  LigmaContext *context;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (template == NULL || LIGMA_IS_CONTEXT (template), NULL);

  context = g_object_new (LIGMA_TYPE_CONTEXT,
                          "name", name,
                          "ligma", ligma,
                          NULL);

  if (template)
    {
      context->defined_props = template->defined_props;

      ligma_context_copy_properties (template, context,
                                    LIGMA_CONTEXT_PROP_MASK_ALL);
    }

  return context;
}

LigmaContext *
ligma_context_get_parent (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->parent;
}

static void
ligma_context_parent_notify (LigmaContext *parent,
                            GParamSpec  *pspec,
                            LigmaContext *context)
{
  if (pspec->owner_type == LIGMA_TYPE_CONTEXT)
    {
      LigmaContextPropType prop = pspec->param_id;

      /*  copy from parent if the changed property is undefined;
       *  ignore properties that are not context properties, for
       *  example notifications on the context's "ligma" property
       */
      if ((prop >= LIGMA_CONTEXT_PROP_FIRST) &&
          (prop <= LIGMA_CONTEXT_PROP_LAST)  &&
          ! ((1 << prop) & context->defined_props))
        {
          ligma_context_copy_property (parent, context, prop);
        }
    }
}

void
ligma_context_set_parent (LigmaContext *context,
                         LigmaContext *parent)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (parent == NULL || LIGMA_IS_CONTEXT (parent));
  g_return_if_fail (parent == NULL || parent->parent != context);
  g_return_if_fail (context != parent);

  if (context->parent == parent)
    return;

  if (context->parent)
    {
      g_signal_handlers_disconnect_by_func (context->parent,
                                            ligma_context_parent_notify,
                                            context);

      g_object_remove_weak_pointer (G_OBJECT (context->parent),
                                    (gpointer) &context->parent);
    }

  context->parent = parent;

  if (parent)
    {
      g_object_add_weak_pointer (G_OBJECT (context->parent),
                                 (gpointer) &context->parent);

      /*  copy all undefined properties from the new parent  */
      ligma_context_copy_properties (parent, context,
                                    ~context->defined_props &
                                    LIGMA_CONTEXT_PROP_MASK_ALL);

      g_signal_connect_object (parent, "notify",
                               G_CALLBACK (ligma_context_parent_notify),
                               context,
                               0);
    }
}


/*  define / undefinine context properties  */

void
ligma_context_define_property (LigmaContext         *context,
                              LigmaContextPropType  prop,
                              gboolean             defined)
{
  LigmaContextPropMask mask;

  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail ((prop >= LIGMA_CONTEXT_PROP_FIRST) &&
                    (prop <= LIGMA_CONTEXT_PROP_LAST));

  mask = (1 << prop);

  if (defined)
    {
      if (! (context->defined_props & mask))
        {
          context->defined_props |= mask;
        }
    }
  else
    {
      if (context->defined_props & mask)
        {
          context->defined_props &= ~mask;

          if (context->parent)
            ligma_context_copy_property (context->parent, context, prop);
        }
    }
}

gboolean
ligma_context_property_defined (LigmaContext         *context,
                               LigmaContextPropType  prop)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), FALSE);

  return (context->defined_props & (1 << prop)) ? TRUE : FALSE;
}

void
ligma_context_define_properties (LigmaContext         *context,
                                LigmaContextPropMask  prop_mask,
                                gboolean             defined)
{
  LigmaContextPropType prop;

  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  for (prop = LIGMA_CONTEXT_PROP_FIRST; prop <= LIGMA_CONTEXT_PROP_LAST; prop++)
    if ((1 << prop) & prop_mask)
      ligma_context_define_property (context, prop, defined);
}


/*  specify which context properties will be serialized  */

void
ligma_context_set_serialize_properties (LigmaContext         *context,
                                       LigmaContextPropMask  props_mask)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  context->serialize_props = props_mask;
}

LigmaContextPropMask
ligma_context_get_serialize_properties (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), 0);

  return context->serialize_props;
}


/*  copying context properties  */

void
ligma_context_copy_property (LigmaContext         *src,
                            LigmaContext         *dest,
                            LigmaContextPropType  prop)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (src));
  g_return_if_fail (LIGMA_IS_CONTEXT (dest));
  g_return_if_fail ((prop >= LIGMA_CONTEXT_PROP_FIRST) &&
                    (prop <= LIGMA_CONTEXT_PROP_LAST));

  switch (prop)
    {
    case LIGMA_CONTEXT_PROP_IMAGE:
      ligma_context_real_set_image (dest, src->image);
      break;

    case LIGMA_CONTEXT_PROP_DISPLAY:
      ligma_context_real_set_display (dest, src->display);
      break;

    case LIGMA_CONTEXT_PROP_TOOL:
      ligma_context_real_set_tool (dest, src->tool_info);
      COPY_NAME (src, dest, tool_name);
      break;

    case LIGMA_CONTEXT_PROP_PAINT_INFO:
      ligma_context_real_set_paint_info (dest, src->paint_info);
      COPY_NAME (src, dest, paint_name);
      break;

    case LIGMA_CONTEXT_PROP_FOREGROUND:
      ligma_context_real_set_foreground (dest, &src->foreground);
      break;

    case LIGMA_CONTEXT_PROP_BACKGROUND:
      ligma_context_real_set_background (dest, &src->background);
      break;

    case LIGMA_CONTEXT_PROP_OPACITY:
      ligma_context_real_set_opacity (dest, src->opacity);
      break;

    case LIGMA_CONTEXT_PROP_PAINT_MODE:
      ligma_context_real_set_paint_mode (dest, src->paint_mode);
      break;

    case LIGMA_CONTEXT_PROP_BRUSH:
      ligma_context_real_set_brush (dest, src->brush);
      COPY_NAME (src, dest, brush_name);
      break;

    case LIGMA_CONTEXT_PROP_DYNAMICS:
      ligma_context_real_set_dynamics (dest, src->dynamics);
      COPY_NAME (src, dest, dynamics_name);
      break;

    case LIGMA_CONTEXT_PROP_MYBRUSH:
      ligma_context_real_set_mybrush (dest, src->mybrush);
      COPY_NAME (src, dest, mybrush_name);
      break;

    case LIGMA_CONTEXT_PROP_PATTERN:
      ligma_context_real_set_pattern (dest, src->pattern);
      COPY_NAME (src, dest, pattern_name);
      break;

    case LIGMA_CONTEXT_PROP_GRADIENT:
      ligma_context_real_set_gradient (dest, src->gradient);
      COPY_NAME (src, dest, gradient_name);
      break;

    case LIGMA_CONTEXT_PROP_PALETTE:
      ligma_context_real_set_palette (dest, src->palette);
      COPY_NAME (src, dest, palette_name);
      break;

    case LIGMA_CONTEXT_PROP_FONT:
      ligma_context_real_set_font (dest, src->font);
      COPY_NAME (src, dest, font_name);
      break;

    case LIGMA_CONTEXT_PROP_TOOL_PRESET:
      ligma_context_real_set_tool_preset (dest, src->tool_preset);
      COPY_NAME (src, dest, tool_preset_name);
      break;

    case LIGMA_CONTEXT_PROP_BUFFER:
      ligma_context_real_set_buffer (dest, src->buffer);
      COPY_NAME (src, dest, buffer_name);
      break;

    case LIGMA_CONTEXT_PROP_IMAGEFILE:
      ligma_context_real_set_imagefile (dest, src->imagefile);
      COPY_NAME (src, dest, imagefile_name);
      break;

    case LIGMA_CONTEXT_PROP_TEMPLATE:
      ligma_context_real_set_template (dest, src->template);
      COPY_NAME (src, dest, template_name);
      break;

    default:
      break;
    }
}

void
ligma_context_copy_properties (LigmaContext         *src,
                              LigmaContext         *dest,
                              LigmaContextPropMask  prop_mask)
{
  LigmaContextPropType prop;

  g_return_if_fail (LIGMA_IS_CONTEXT (src));
  g_return_if_fail (LIGMA_IS_CONTEXT (dest));

  for (prop = LIGMA_CONTEXT_PROP_FIRST; prop <= LIGMA_CONTEXT_PROP_LAST; prop++)
    if ((1 << prop) & prop_mask)
      ligma_context_copy_property (src, dest, prop);
}


/*  attribute access functions  */

/*****************************************************************************/
/*  manipulate by GType  *****************************************************/

LigmaContextPropType
ligma_context_type_to_property (GType type)
{
  LigmaContextPropType prop;

  for (prop = LIGMA_CONTEXT_PROP_FIRST; prop <= LIGMA_CONTEXT_PROP_LAST; prop++)
    {
      if (g_type_is_a (type, ligma_context_prop_types[prop]))
        return prop;
    }

  return -1;
}

const gchar *
ligma_context_type_to_prop_name (GType type)
{
  LigmaContextPropType prop;

  for (prop = LIGMA_CONTEXT_PROP_FIRST; prop <= LIGMA_CONTEXT_PROP_LAST; prop++)
    {
      if (g_type_is_a (type, ligma_context_prop_types[prop]))
        return ligma_context_prop_names[prop];
    }

  return NULL;
}

const gchar *
ligma_context_type_to_signal_name (GType type)
{
  LigmaContextPropType prop;

  for (prop = LIGMA_CONTEXT_PROP_FIRST; prop <= LIGMA_CONTEXT_PROP_LAST; prop++)
    {
      if (g_type_is_a (type, ligma_context_prop_types[prop]))
        return g_signal_name (ligma_context_signals[prop]);
    }

  return NULL;
}

LigmaObject *
ligma_context_get_by_type (LigmaContext *context,
                          GType        type)
{
  LigmaContextPropType  prop;
  LigmaObject          *object = NULL;

  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  prop = ligma_context_type_to_property (type);

  g_return_val_if_fail (prop != -1, NULL);

  g_object_get (context,
                ligma_context_prop_names[prop], &object,
                NULL);

  /*  g_object_get() refs the object, this function however is a getter,
   *  which usually doesn't ref it's return value
   */
  if (object)
    g_object_unref (object);

  return object;
}

void
ligma_context_set_by_type (LigmaContext *context,
                          GType        type,
                          LigmaObject  *object)
{
  LigmaContextPropType  prop;
  GParamSpec          *pspec;
  GValue               value = G_VALUE_INIT;

  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (object == NULL || G_IS_OBJECT (object));

  prop = ligma_context_type_to_property (type);

  g_return_if_fail (prop != -1);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (context),
                                        ligma_context_prop_names[prop]);

  g_return_if_fail (pspec != NULL);

  g_value_init (&value, pspec->value_type);
  g_value_set_object (&value, object);

  /*  we use ligma_context_set_property() (which in turn only calls
   *  ligma_context_set_foo() functions) instead of the much more obvious
   *  g_object_set(); this avoids g_object_freeze_notify()/thaw_notify()
   *  around the g_object_set() and makes LigmaContext callbacks being
   *  called in a much more predictable order. See bug #731279.
   */
  ligma_context_set_property (G_OBJECT (context),
                             pspec->param_id,
                             (const GValue *) &value,
                             pspec);

  g_value_unset (&value);
}

void
ligma_context_changed_by_type (LigmaContext *context,
                              GType        type)
{
  LigmaContextPropType  prop;
  LigmaObject          *object;

  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  prop = ligma_context_type_to_property (type);

  g_return_if_fail (prop != -1);

  object = ligma_context_get_by_type (context, type);

  g_signal_emit (context,
                 ligma_context_signals[prop], 0,
                 object);
}


/*****************************************************************************/
/*  image  *******************************************************************/

LigmaImage *
ligma_context_get_image (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->image;
}

void
ligma_context_set_image (LigmaContext *context,
                        LigmaImage   *image)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (image == NULL || LIGMA_IS_IMAGE (image));

  context_find_defined (context, LIGMA_CONTEXT_PROP_IMAGE);

  ligma_context_real_set_image (context, image);
}

void
ligma_context_image_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[IMAGE_CHANGED], 0,
                 context->image);
}

static void
ligma_context_image_removed (LigmaContainer *container,
                            LigmaImage     *image,
                            LigmaContext   *context)
{
  if (context->image == image)
    ligma_context_real_set_image (context, NULL);
}

static void
ligma_context_real_set_image (LigmaContext *context,
                             LigmaImage   *image)
{
  if (context->image == image)
    return;

  context->image = image;

  g_object_notify (G_OBJECT (context), "image");
  ligma_context_image_changed (context);
}


/*****************************************************************************/
/*  display  *****************************************************************/

LigmaDisplay *
ligma_context_get_display (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->display;
}

void
ligma_context_set_display (LigmaContext *context,
                          LigmaDisplay *display)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (display == NULL || LIGMA_IS_DISPLAY (display));

  context_find_defined (context, LIGMA_CONTEXT_PROP_DISPLAY);

  ligma_context_real_set_display (context, display);
}

void
ligma_context_display_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[DISPLAY_CHANGED], 0,
                 context->display);
}

static void
ligma_context_display_removed (LigmaContainer *container,
                              LigmaDisplay   *display,
                              LigmaContext   *context)
{
  if (context->display == display)
    ligma_context_real_set_display (context, NULL);
}

static void
ligma_context_real_set_display (LigmaContext *context,
                               LigmaDisplay *display)
{
  LigmaDisplay *old_display;

  if (context->display == display)
    {
      /*  make sure that setting a display *always* sets the image
       *  to that display's image, even if the display already
       *  matches
       */
      if (display)
        {
          LigmaImage *image;

          g_object_get (display, "image", &image, NULL);

          ligma_context_real_set_image (context, image);

          if (image)
            g_object_unref (image);
        }

      return;
    }

  old_display = context->display;

  context->display = display;

  if (context->display)
    {
      LigmaImage *image;

      g_object_get (display, "image", &image, NULL);

      ligma_context_real_set_image (context, image);

      if (image)
        g_object_unref (image);
    }
  else if (old_display)
    {
      ligma_context_real_set_image (context, NULL);
    }

  g_object_notify (G_OBJECT (context), "display");
  ligma_context_display_changed (context);
}


/*****************************************************************************/
/*  tool  ********************************************************************/

LigmaToolInfo *
ligma_context_get_tool (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->tool_info;
}

void
ligma_context_set_tool (LigmaContext  *context,
                       LigmaToolInfo *tool_info)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (tool_info == NULL || LIGMA_IS_TOOL_INFO (tool_info));

  context_find_defined (context, LIGMA_CONTEXT_PROP_TOOL);

  ligma_context_real_set_tool (context, tool_info);
}

void
ligma_context_tool_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[TOOL_CHANGED], 0,
                 context->tool_info);
}

static void
ligma_context_tool_dirty (LigmaToolInfo *tool_info,
                         LigmaContext  *context)
{
  g_free (context->tool_name);
  context->tool_name = g_strdup (ligma_object_get_name (tool_info));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_TOOL);
}

static void
ligma_context_tool_list_thaw (LigmaContainer *container,
                             LigmaContext   *context)
{
  LigmaToolInfo *tool_info;

  if (! context->tool_name)
    context->tool_name = g_strdup ("ligma-paintbrush-tool");

  tool_info = ligma_context_find_object (context, container,
                                        context->tool_name,
                                        ligma_tool_info_get_standard (context->ligma));

  ligma_context_real_set_tool (context, tool_info);
}

static void
ligma_context_tool_removed (LigmaContainer *container,
                           LigmaToolInfo  *tool_info,
                           LigmaContext   *context)
{
  if (tool_info == context->tool_info)
    {
      g_signal_handlers_disconnect_by_func (context->tool_info,
                                            ligma_context_tool_dirty,
                                            context);
      g_clear_object (&context->tool_info);

      if (! ligma_container_frozen (container))
        ligma_context_tool_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_tool (LigmaContext  *context,
                            LigmaToolInfo *tool_info)
{
  if (context->tool_info == tool_info)
    return;

  if (context->tool_name &&
      tool_info != ligma_tool_info_get_standard (context->ligma))
    {
      g_clear_pointer (&context->tool_name, g_free);
    }

  if (context->tool_info)
    g_signal_handlers_disconnect_by_func (context->tool_info,
                                          ligma_context_tool_dirty,
                                          context);

  g_set_object (&context->tool_info, tool_info);

  if (tool_info)
    {
      g_signal_connect_object (tool_info, "name-changed",
                               G_CALLBACK (ligma_context_tool_dirty),
                               context,
                               0);

      if (tool_info != ligma_tool_info_get_standard (context->ligma))
        context->tool_name = g_strdup (ligma_object_get_name (tool_info));

      if (tool_info->paint_info)
        ligma_context_real_set_paint_info (context, tool_info->paint_info);
    }

  g_object_notify (G_OBJECT (context), "tool");
  ligma_context_tool_changed (context);
}


/*****************************************************************************/
/*  paint info  **************************************************************/

LigmaPaintInfo *
ligma_context_get_paint_info (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->paint_info;
}

void
ligma_context_set_paint_info (LigmaContext   *context,
                             LigmaPaintInfo *paint_info)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (paint_info == NULL || LIGMA_IS_PAINT_INFO (paint_info));

  context_find_defined (context, LIGMA_CONTEXT_PROP_PAINT_INFO);

  ligma_context_real_set_paint_info (context, paint_info);
}

void
ligma_context_paint_info_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[PAINT_INFO_CHANGED], 0,
                 context->paint_info);
}

static void
ligma_context_paint_info_dirty (LigmaPaintInfo *paint_info,
                               LigmaContext   *context)
{
  g_free (context->paint_name);
  context->paint_name = g_strdup (ligma_object_get_name (paint_info));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_PAINT_INFO);
}

/*  the global paint info list is there again after refresh  */
static void
ligma_context_paint_info_list_thaw (LigmaContainer *container,
                                   LigmaContext   *context)
{
  LigmaPaintInfo *paint_info;

  if (! context->paint_name)
    context->paint_name = g_strdup ("ligma-paintbrush");

  paint_info = ligma_context_find_object (context, container,
                                         context->paint_name,
                                         ligma_paint_info_get_standard (context->ligma));

  ligma_context_real_set_paint_info (context, paint_info);
}

static void
ligma_context_paint_info_removed (LigmaContainer *container,
                                 LigmaPaintInfo *paint_info,
                                 LigmaContext   *context)
{
  if (paint_info == context->paint_info)
    {
      g_signal_handlers_disconnect_by_func (context->paint_info,
                                            ligma_context_paint_info_dirty,
                                            context);
      g_clear_object (&context->paint_info);

      if (! ligma_container_frozen (container))
        ligma_context_paint_info_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_paint_info (LigmaContext   *context,
                                  LigmaPaintInfo *paint_info)
{
  if (context->paint_info == paint_info)
    return;

  if (context->paint_name &&
      paint_info != ligma_paint_info_get_standard (context->ligma))
    {
      g_clear_pointer (&context->paint_name, g_free);
    }

  if (context->paint_info)
    g_signal_handlers_disconnect_by_func (context->paint_info,
                                          ligma_context_paint_info_dirty,
                                          context);

  g_set_object (&context->paint_info, paint_info);

  if (paint_info)
    {
      g_signal_connect_object (paint_info, "name-changed",
                               G_CALLBACK (ligma_context_paint_info_dirty),
                               context,
                               0);

      if (paint_info != ligma_paint_info_get_standard (context->ligma))
        context->paint_name = g_strdup (ligma_object_get_name (paint_info));
    }

  g_object_notify (G_OBJECT (context), "paint-info");
  ligma_context_paint_info_changed (context);
}


/*****************************************************************************/
/*  foreground color  ********************************************************/

void
ligma_context_get_foreground (LigmaContext *context,
                             LigmaRGB     *color)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (color != NULL);

  *color = context->foreground;
}

void
ligma_context_set_foreground (LigmaContext   *context,
                             const LigmaRGB *color)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (color != NULL);

  context_find_defined (context, LIGMA_CONTEXT_PROP_FOREGROUND);

  ligma_context_real_set_foreground (context, color);
}

void
ligma_context_foreground_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[FOREGROUND_CHANGED], 0,
                 &context->foreground);
}

static void
ligma_context_real_set_foreground (LigmaContext   *context,
                                  const LigmaRGB *color)
{
  if (ligma_rgba_distance (&context->foreground, color) < RGBA_EPSILON)
    return;

  context->foreground = *color;
  ligma_rgb_set_alpha (&context->foreground, LIGMA_OPACITY_OPAQUE);

  g_object_notify (G_OBJECT (context), "foreground");
  ligma_context_foreground_changed (context);
}


/*****************************************************************************/
/*  background color  ********************************************************/

void
ligma_context_get_background (LigmaContext *context,
                             LigmaRGB     *color)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_return_if_fail (color != NULL);

  *color = context->background;
}

void
ligma_context_set_background (LigmaContext   *context,
                             const LigmaRGB *color)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (color != NULL);

  context_find_defined (context, LIGMA_CONTEXT_PROP_BACKGROUND);

  ligma_context_real_set_background (context, color);
}

void
ligma_context_background_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[BACKGROUND_CHANGED], 0,
                 &context->background);
}

static void
ligma_context_real_set_background (LigmaContext   *context,
                                  const LigmaRGB *color)
{
  if (ligma_rgba_distance (&context->background, color) < RGBA_EPSILON)
    return;

  context->background = *color;
  ligma_rgb_set_alpha (&context->background, LIGMA_OPACITY_OPAQUE);

  g_object_notify (G_OBJECT (context), "background");
  ligma_context_background_changed (context);
}


/*****************************************************************************/
/*  color utility functions  *************************************************/

void
ligma_context_set_default_colors (LigmaContext *context)
{
  LigmaContext *bg_context;
  LigmaRGB      fg;
  LigmaRGB      bg;

  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  bg_context = context;

  context_find_defined (context, LIGMA_CONTEXT_PROP_FOREGROUND);
  context_find_defined (bg_context, LIGMA_CONTEXT_PROP_BACKGROUND);

  ligma_rgba_set (&fg, 0.0, 0.0, 0.0, LIGMA_OPACITY_OPAQUE);
  ligma_rgba_set (&bg, 1.0, 1.0, 1.0, LIGMA_OPACITY_OPAQUE);

  ligma_context_real_set_foreground (context, &fg);
  ligma_context_real_set_background (bg_context, &bg);
}

void
ligma_context_swap_colors (LigmaContext *context)
{
  LigmaContext *bg_context;
  LigmaRGB      fg;
  LigmaRGB      bg;

  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  bg_context = context;

  context_find_defined (context, LIGMA_CONTEXT_PROP_FOREGROUND);
  context_find_defined (bg_context, LIGMA_CONTEXT_PROP_BACKGROUND);

  ligma_context_get_foreground (context, &fg);
  ligma_context_get_background (bg_context, &bg);

  ligma_context_real_set_foreground (context, &bg);
  ligma_context_real_set_background (bg_context, &fg);
}


/*****************************************************************************/
/*  opacity  *****************************************************************/

gdouble
ligma_context_get_opacity (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), LIGMA_OPACITY_OPAQUE);

  return context->opacity;
}

void
ligma_context_set_opacity (LigmaContext *context,
                          gdouble      opacity)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  context_find_defined (context, LIGMA_CONTEXT_PROP_OPACITY);

  ligma_context_real_set_opacity (context, opacity);
}

void
ligma_context_opacity_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[OPACITY_CHANGED], 0,
                 context->opacity);
}

static void
ligma_context_real_set_opacity (LigmaContext *context,
                               gdouble      opacity)
{
  if (context->opacity == opacity)
    return;

  context->opacity = opacity;

  g_object_notify (G_OBJECT (context), "opacity");
  ligma_context_opacity_changed (context);
}


/*****************************************************************************/
/*  paint mode  **************************************************************/

LigmaLayerMode
ligma_context_get_paint_mode (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), LIGMA_LAYER_MODE_NORMAL);

  return context->paint_mode;
}

void
ligma_context_set_paint_mode (LigmaContext   *context,
                             LigmaLayerMode  paint_mode)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  context_find_defined (context, LIGMA_CONTEXT_PROP_PAINT_MODE);

  ligma_context_real_set_paint_mode (context, paint_mode);
}

void
ligma_context_paint_mode_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[PAINT_MODE_CHANGED], 0,
                 context->paint_mode);
}

static void
ligma_context_real_set_paint_mode (LigmaContext   *context,
                                  LigmaLayerMode  paint_mode)
{
  if (context->paint_mode == paint_mode)
    return;

  context->paint_mode = paint_mode;

  g_object_notify (G_OBJECT (context), "paint-mode");
  ligma_context_paint_mode_changed (context);
}


/*****************************************************************************/
/*  brush  *******************************************************************/

LigmaBrush *
ligma_context_get_brush (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->brush;
}

void
ligma_context_set_brush (LigmaContext *context,
                        LigmaBrush   *brush)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (brush == NULL || LIGMA_IS_BRUSH (brush));

  context_find_defined (context, LIGMA_CONTEXT_PROP_BRUSH);

  ligma_context_real_set_brush (context, brush);
}

void
ligma_context_brush_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[BRUSH_CHANGED], 0,
                 context->brush);
}

static void
ligma_context_brush_dirty (LigmaBrush   *brush,
                          LigmaContext *context)
{
  g_free (context->brush_name);
  context->brush_name = g_strdup (ligma_object_get_name (brush));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_BRUSH);
}

static void
ligma_context_brush_list_thaw (LigmaContainer *container,
                              LigmaContext   *context)
{
  LigmaBrush *brush;

  if (! context->brush_name)
    context->brush_name = g_strdup (context->ligma->config->default_brush);

  brush = ligma_context_find_object (context, container,
                                    context->brush_name,
                                    ligma_brush_get_standard (context));

  ligma_context_real_set_brush (context, brush);
}

/*  the active brush disappeared  */
static void
ligma_context_brush_removed (LigmaContainer *container,
                            LigmaBrush     *brush,
                            LigmaContext   *context)
{
  if (brush == context->brush)
    {
      g_signal_handlers_disconnect_by_func (context->brush,
                                            ligma_context_brush_dirty,
                                            context);
      g_clear_object (&context->brush);

      if (! ligma_container_frozen (container))
        ligma_context_brush_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_brush (LigmaContext *context,
                             LigmaBrush   *brush)
{
  if (context->brush == brush)
    return;

  if (context->brush_name &&
      brush != LIGMA_BRUSH (ligma_brush_get_standard (context)))
    {
      g_clear_pointer (&context->brush_name, g_free);
    }

  if (context->brush)
    g_signal_handlers_disconnect_by_func (context->brush,
                                          ligma_context_brush_dirty,
                                          context);

  g_set_object (&context->brush, brush);

  if (brush)
    {
      g_signal_connect_object (brush, "name-changed",
                               G_CALLBACK (ligma_context_brush_dirty),
                               context,
                               0);

      if (brush != LIGMA_BRUSH (ligma_brush_get_standard (context)))
        context->brush_name = g_strdup (ligma_object_get_name (brush));
    }

  g_object_notify (G_OBJECT (context), "brush");
  ligma_context_brush_changed (context);
}


/*****************************************************************************/
/*  dynamics *****************************************************************/

LigmaDynamics *
ligma_context_get_dynamics (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->dynamics;
}

void
ligma_context_set_dynamics (LigmaContext  *context,
                           LigmaDynamics *dynamics)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (dynamics == NULL || LIGMA_IS_DYNAMICS (dynamics));

  context_find_defined (context, LIGMA_CONTEXT_PROP_DYNAMICS);

  ligma_context_real_set_dynamics (context, dynamics);
}

void
ligma_context_dynamics_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[DYNAMICS_CHANGED], 0,
                 context->dynamics);
}

static void
ligma_context_dynamics_dirty (LigmaDynamics *dynamics,
                             LigmaContext  *context)
{
  g_free (context->dynamics_name);
  context->dynamics_name = g_strdup (ligma_object_get_name (dynamics));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_DYNAMICS);
}

static void
ligma_context_dynamics_removed (LigmaContainer *container,
                               LigmaDynamics  *dynamics,
                               LigmaContext   *context)
{
  if (dynamics == context->dynamics)
    {
      g_signal_handlers_disconnect_by_func (context->dynamics,
                                            ligma_context_dynamics_dirty,
                                            context);
      g_clear_object (&context->dynamics);

      if (! ligma_container_frozen (container))
        ligma_context_dynamics_list_thaw (container, context);
    }
}

static void
ligma_context_dynamics_list_thaw (LigmaContainer *container,
                                 LigmaContext   *context)
{
  LigmaDynamics *dynamics;

  if (! context->dynamics_name)
    context->dynamics_name = g_strdup (context->ligma->config->default_dynamics);

  dynamics = ligma_context_find_object (context, container,
                                       context->dynamics_name,
                                       ligma_dynamics_get_standard (context));

  ligma_context_real_set_dynamics (context, dynamics);
}

static void
ligma_context_real_set_dynamics (LigmaContext  *context,
                                LigmaDynamics *dynamics)
{
  if (context->dynamics == dynamics)
    return;

  if (context->dynamics_name &&
      dynamics != LIGMA_DYNAMICS (ligma_dynamics_get_standard (context)))
    {
      g_clear_pointer (&context->dynamics_name, g_free);
    }

  if (context->dynamics)
    g_signal_handlers_disconnect_by_func (context->dynamics,
                                          ligma_context_dynamics_dirty,
                                          context);

  g_set_object (&context->dynamics, dynamics);

  if (dynamics)
    {
      g_signal_connect_object (dynamics, "name-changed",
                               G_CALLBACK (ligma_context_dynamics_dirty),
                               context,
                               0);

      if (dynamics != LIGMA_DYNAMICS (ligma_dynamics_get_standard (context)))
        context->dynamics_name = g_strdup (ligma_object_get_name (dynamics));
    }

  g_object_notify (G_OBJECT (context), "dynamics");
  ligma_context_dynamics_changed (context);
}


/*****************************************************************************/
/*  mybrush  *****************************************************************/

LigmaMybrush *
ligma_context_get_mybrush (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->mybrush;
}

void
ligma_context_set_mybrush (LigmaContext *context,
                          LigmaMybrush *brush)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (brush == NULL || LIGMA_IS_MYBRUSH (brush));

  context_find_defined (context, LIGMA_CONTEXT_PROP_MYBRUSH);

  ligma_context_real_set_mybrush (context, brush);
}

void
ligma_context_mybrush_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[MYBRUSH_CHANGED], 0,
                 context->mybrush);
}

static void
ligma_context_mybrush_dirty (LigmaMybrush *brush,
                            LigmaContext *context)
{
  g_free (context->mybrush_name);
  context->mybrush_name = g_strdup (ligma_object_get_name (brush));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_MYBRUSH);
}

static void
ligma_context_mybrush_list_thaw (LigmaContainer *container,
                                LigmaContext   *context)
{
  LigmaMybrush *brush;

  if (! context->mybrush_name)
    context->mybrush_name = g_strdup (context->ligma->config->default_mypaint_brush);

  brush = ligma_context_find_object (context, container,
                                    context->mybrush_name,
                                    ligma_mybrush_get_standard (context));

  ligma_context_real_set_mybrush (context, brush);
}

static void
ligma_context_mybrush_removed (LigmaContainer *container,
                              LigmaMybrush   *brush,
                              LigmaContext   *context)
{
  if (brush == context->mybrush)
    {
      g_signal_handlers_disconnect_by_func (context->mybrush,
                                            ligma_context_mybrush_dirty,
                                            context);
      g_clear_object (&context->mybrush);

      if (! ligma_container_frozen (container))
        ligma_context_mybrush_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_mybrush (LigmaContext *context,
                               LigmaMybrush *brush)
{
  if (context->mybrush == brush)
    return;

  if (context->mybrush_name &&
      brush != LIGMA_MYBRUSH (ligma_mybrush_get_standard (context)))
    {
      g_clear_pointer (&context->mybrush_name, g_free);
    }

  if (context->mybrush)
    g_signal_handlers_disconnect_by_func (context->mybrush,
                                          ligma_context_mybrush_dirty,
                                          context);

  g_set_object (&context->mybrush, brush);

  if (brush)
    {
      g_signal_connect_object (brush, "name-changed",
                               G_CALLBACK (ligma_context_mybrush_dirty),
                               context,
                               0);

      if (brush != LIGMA_MYBRUSH (ligma_mybrush_get_standard (context)))
        context->mybrush_name = g_strdup (ligma_object_get_name (brush));
    }

  g_object_notify (G_OBJECT (context), "mybrush");
  ligma_context_mybrush_changed (context);
}


/*****************************************************************************/
/*  pattern  *****************************************************************/

LigmaPattern *
ligma_context_get_pattern (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->pattern;
}

void
ligma_context_set_pattern (LigmaContext *context,
                          LigmaPattern *pattern)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (pattern == NULL || LIGMA_IS_PATTERN (pattern));

  context_find_defined (context, LIGMA_CONTEXT_PROP_PATTERN);

  ligma_context_real_set_pattern (context, pattern);
}

void
ligma_context_pattern_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[PATTERN_CHANGED], 0,
                 context->pattern);
}

static void
ligma_context_pattern_dirty (LigmaPattern *pattern,
                            LigmaContext *context)
{
  g_free (context->pattern_name);
  context->pattern_name = g_strdup (ligma_object_get_name (pattern));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_PATTERN);
}

static void
ligma_context_pattern_list_thaw (LigmaContainer *container,
                                LigmaContext   *context)
{
  LigmaPattern *pattern;

  if (! context->pattern_name)
    context->pattern_name = g_strdup (context->ligma->config->default_pattern);

  pattern = ligma_context_find_object (context, container,
                                      context->pattern_name,
                                      ligma_pattern_get_standard (context));

  ligma_context_real_set_pattern (context, pattern);
}

static void
ligma_context_pattern_removed (LigmaContainer *container,
                              LigmaPattern   *pattern,
                              LigmaContext   *context)
{
  if (pattern == context->pattern)
    {
      g_signal_handlers_disconnect_by_func (context->pattern,
                                            ligma_context_pattern_dirty,
                                            context);
      g_clear_object (&context->pattern);

      if (! ligma_container_frozen (container))
        ligma_context_pattern_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_pattern (LigmaContext *context,
                               LigmaPattern *pattern)
{
  if (context->pattern == pattern)
    return;

  if (context->pattern_name &&
      pattern != LIGMA_PATTERN (ligma_pattern_get_standard (context)))
    {
      g_clear_pointer (&context->pattern_name, g_free);
    }

  if (context->pattern)
    g_signal_handlers_disconnect_by_func (context->pattern,
                                          ligma_context_pattern_dirty,
                                          context);

  g_set_object (&context->pattern, pattern);

  if (pattern)
    {
      g_signal_connect_object (pattern, "name-changed",
                               G_CALLBACK (ligma_context_pattern_dirty),
                               context,
                               0);

      if (pattern != LIGMA_PATTERN (ligma_pattern_get_standard (context)))
        context->pattern_name = g_strdup (ligma_object_get_name (pattern));
    }

  g_object_notify (G_OBJECT (context), "pattern");
  ligma_context_pattern_changed (context);
}


/*****************************************************************************/
/*  gradient  ****************************************************************/

LigmaGradient *
ligma_context_get_gradient (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->gradient;
}

void
ligma_context_set_gradient (LigmaContext  *context,
                           LigmaGradient *gradient)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (gradient == NULL || LIGMA_IS_GRADIENT (gradient));

  context_find_defined (context, LIGMA_CONTEXT_PROP_GRADIENT);

  ligma_context_real_set_gradient (context, gradient);
}

void
ligma_context_gradient_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[GRADIENT_CHANGED], 0,
                 context->gradient);
}

static void
ligma_context_gradient_dirty (LigmaGradient *gradient,
                             LigmaContext  *context)
{
  g_free (context->gradient_name);
  context->gradient_name = g_strdup (ligma_object_get_name (gradient));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_GRADIENT);
}

static void
ligma_context_gradient_list_thaw (LigmaContainer *container,
                                 LigmaContext   *context)
{
  LigmaGradient *gradient;

  if (! context->gradient_name)
    context->gradient_name = g_strdup (context->ligma->config->default_gradient);

  gradient = ligma_context_find_object (context, container,
                                       context->gradient_name,
                                       ligma_gradient_get_standard (context));

  ligma_context_real_set_gradient (context, gradient);
}

static void
ligma_context_gradient_removed (LigmaContainer *container,
                               LigmaGradient  *gradient,
                               LigmaContext   *context)
{
  if (gradient == context->gradient)
    {
      g_signal_handlers_disconnect_by_func (context->gradient,
                                            ligma_context_gradient_dirty,
                                            context);
      g_clear_object (&context->gradient);

      if (! ligma_container_frozen (container))
        ligma_context_gradient_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_gradient (LigmaContext  *context,
                                LigmaGradient *gradient)
{
  if (context->gradient == gradient)
    return;

  if (context->gradient_name &&
      gradient != LIGMA_GRADIENT (ligma_gradient_get_standard (context)))
    {
      g_clear_pointer (&context->gradient_name, g_free);
    }

  if (context->gradient)
    g_signal_handlers_disconnect_by_func (context->gradient,
                                          ligma_context_gradient_dirty,
                                          context);

  g_set_object (&context->gradient, gradient);

  if (gradient)
    {
      g_signal_connect_object (gradient, "name-changed",
                               G_CALLBACK (ligma_context_gradient_dirty),
                               context,
                               0);

      if (gradient != LIGMA_GRADIENT (ligma_gradient_get_standard (context)))
        context->gradient_name = g_strdup (ligma_object_get_name (gradient));
    }

  g_object_notify (G_OBJECT (context), "gradient");
  ligma_context_gradient_changed (context);
}


/*****************************************************************************/
/*  palette  *****************************************************************/

LigmaPalette *
ligma_context_get_palette (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->palette;
}

void
ligma_context_set_palette (LigmaContext *context,
                          LigmaPalette *palette)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (palette == NULL || LIGMA_IS_PALETTE (palette));

  context_find_defined (context, LIGMA_CONTEXT_PROP_PALETTE);

  ligma_context_real_set_palette (context, palette);
}

void
ligma_context_palette_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[PALETTE_CHANGED], 0,
                 context->palette);
}

static void
ligma_context_palette_dirty (LigmaPalette *palette,
                            LigmaContext *context)
{
  g_free (context->palette_name);
  context->palette_name = g_strdup (ligma_object_get_name (palette));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_PALETTE);
}

static void
ligma_context_palette_list_thaw (LigmaContainer *container,
                                LigmaContext   *context)
{
  LigmaPalette *palette;

  if (! context->palette_name)
    context->palette_name = g_strdup (context->ligma->config->default_palette);

  palette = ligma_context_find_object (context, container,
                                      context->palette_name,
                                      ligma_palette_get_standard (context));

  ligma_context_real_set_palette (context, palette);
}

static void
ligma_context_palette_removed (LigmaContainer *container,
                              LigmaPalette   *palette,
                              LigmaContext   *context)
{
  if (palette == context->palette)
    {
      g_signal_handlers_disconnect_by_func (context->palette,
                                            ligma_context_palette_dirty,
                                            context);
      g_clear_object (&context->palette);

      if (! ligma_container_frozen (container))
        ligma_context_palette_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_palette (LigmaContext *context,
                               LigmaPalette *palette)
{
  if (context->palette == palette)
    return;

  if (context->palette_name &&
      palette != LIGMA_PALETTE (ligma_palette_get_standard (context)))
    {
      g_clear_pointer (&context->palette_name, g_free);
    }

  if (context->palette)
    g_signal_handlers_disconnect_by_func (context->palette,
                                          ligma_context_palette_dirty,
                                          context);

  g_set_object (&context->palette, palette);

  if (palette)
    {
      g_signal_connect_object (palette, "name-changed",
                               G_CALLBACK (ligma_context_palette_dirty),
                               context,
                               0);

      if (palette != LIGMA_PALETTE (ligma_palette_get_standard (context)))
        context->palette_name = g_strdup (ligma_object_get_name (palette));
    }

  g_object_notify (G_OBJECT (context), "palette");
  ligma_context_palette_changed (context);
}


/*****************************************************************************/
/*  font     *****************************************************************/

LigmaFont *
ligma_context_get_font (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->font;
}

void
ligma_context_set_font (LigmaContext *context,
                       LigmaFont    *font)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (font == NULL || LIGMA_IS_FONT (font));

  context_find_defined (context, LIGMA_CONTEXT_PROP_FONT);

  ligma_context_real_set_font (context, font);
}

const gchar *
ligma_context_get_font_name (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->font_name;
}

void
ligma_context_set_font_name (LigmaContext *context,
                            const gchar *name)
{
  LigmaContainer *container;
  LigmaObject    *font;

  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  container = ligma_data_factory_get_container (context->ligma->font_factory);
  font      = ligma_container_get_child_by_name (container, name);

  if (font)
    {
      ligma_context_set_font (context, LIGMA_FONT (font));
    }
  else
    {
      /* No font with this name exists, use the standard font, but
       * keep the intended name around
       */
      ligma_context_set_font (context, LIGMA_FONT (ligma_font_get_standard ()));

      g_free (context->font_name);
      context->font_name = g_strdup (name);
    }
}

void
ligma_context_font_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[FONT_CHANGED], 0,
                 context->font);
}

static void
ligma_context_font_dirty (LigmaFont    *font,
                         LigmaContext *context)
{
  g_free (context->font_name);
  context->font_name = g_strdup (ligma_object_get_name (font));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_FONT);
}

static void
ligma_context_font_list_thaw (LigmaContainer *container,
                             LigmaContext   *context)
{
  LigmaFont *font;

  if (! context->font_name)
    context->font_name = g_strdup (context->ligma->config->default_font);

  font = ligma_context_find_object (context, container,
                                   context->font_name,
                                   ligma_font_get_standard ());

  ligma_context_real_set_font (context, font);
}

static void
ligma_context_font_removed (LigmaContainer *container,
                           LigmaFont      *font,
                           LigmaContext   *context)
{
  if (font == context->font)
    {
      g_signal_handlers_disconnect_by_func (context->font,
                                            ligma_context_font_dirty,
                                            context);
      g_clear_object (&context->font);

      if (! ligma_container_frozen (container))
        ligma_context_font_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_font (LigmaContext *context,
                            LigmaFont    *font)
{
  if (context->font == font)
    return;

  if (context->font_name &&
      font != LIGMA_FONT (ligma_font_get_standard ()))
    {
      g_clear_pointer (&context->font_name, g_free);
    }

  if (context->font)
    g_signal_handlers_disconnect_by_func (context->font,
                                          ligma_context_font_dirty,
                                          context);

  g_set_object (&context->font, font);

  if (font)
    {
      g_signal_connect_object (font, "name-changed",
                               G_CALLBACK (ligma_context_font_dirty),
                               context,
                               0);

      if (font != LIGMA_FONT (ligma_font_get_standard ()))
        context->font_name = g_strdup (ligma_object_get_name (font));
    }

  g_object_notify (G_OBJECT (context), "font");
  ligma_context_font_changed (context);
}


/********************************************************************************/
/*  tool preset *****************************************************************/

LigmaToolPreset *
ligma_context_get_tool_preset (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->tool_preset;
}

void
ligma_context_set_tool_preset (LigmaContext    *context,
                              LigmaToolPreset *tool_preset)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (tool_preset == NULL || LIGMA_IS_TOOL_PRESET (tool_preset));

  context_find_defined (context, LIGMA_CONTEXT_PROP_TOOL_PRESET);

  ligma_context_real_set_tool_preset (context, tool_preset);
}

void
ligma_context_tool_preset_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[TOOL_PRESET_CHANGED], 0,
                 context->tool_preset);
}

static void
ligma_context_tool_preset_dirty (LigmaToolPreset *tool_preset,
                                LigmaContext    *context)
{
  g_free (context->tool_preset_name);
  context->tool_preset_name = g_strdup (ligma_object_get_name (tool_preset));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_TOOL_PRESET);
}

static void
ligma_context_tool_preset_removed (LigmaContainer  *container,
                                  LigmaToolPreset *tool_preset,
                                  LigmaContext    *context)
{
  if (tool_preset == context->tool_preset)
    {
      g_signal_handlers_disconnect_by_func (context->tool_preset,
                                            ligma_context_tool_preset_dirty,
                                            context);
      g_clear_object (&context->tool_preset);

      if (! ligma_container_frozen (container))
        ligma_context_tool_preset_list_thaw (container, context);
    }
}

static void
ligma_context_tool_preset_list_thaw (LigmaContainer *container,
                                    LigmaContext   *context)
{
  LigmaToolPreset *tool_preset;

  tool_preset = ligma_context_find_object (context, container,
                                          context->tool_preset_name,
                                          NULL);

  ligma_context_real_set_tool_preset (context, tool_preset);
}

static void
ligma_context_real_set_tool_preset (LigmaContext    *context,
                                   LigmaToolPreset *tool_preset)
{
  if (context->tool_preset == tool_preset)
    return;

  if (context->tool_preset_name)
    {
      g_clear_pointer (&context->tool_preset_name, g_free);
    }

  if (context->tool_preset)
    g_signal_handlers_disconnect_by_func (context->tool_preset,
                                          ligma_context_tool_preset_dirty,
                                          context);

  g_set_object (&context->tool_preset, tool_preset);

  if (tool_preset)
    {
      g_signal_connect_object (tool_preset, "name-changed",
                               G_CALLBACK (ligma_context_tool_preset_dirty),
                               context,
                               0);

      context->tool_preset_name = g_strdup (ligma_object_get_name (tool_preset));
    }

  g_object_notify (G_OBJECT (context), "tool-preset");
  ligma_context_tool_preset_changed (context);
}


/*****************************************************************************/
/*  buffer  ******************************************************************/

LigmaBuffer *
ligma_context_get_buffer (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->buffer;
}

void
ligma_context_set_buffer (LigmaContext *context,
                         LigmaBuffer *buffer)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (buffer == NULL || LIGMA_IS_BUFFER (buffer));

  context_find_defined (context, LIGMA_CONTEXT_PROP_BUFFER);

  ligma_context_real_set_buffer (context, buffer);
}

void
ligma_context_buffer_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[BUFFER_CHANGED], 0,
                 context->buffer);
}

static void
ligma_context_buffer_dirty (LigmaBuffer  *buffer,
                           LigmaContext *context)
{
  g_free (context->buffer_name);
  context->buffer_name = g_strdup (ligma_object_get_name (buffer));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_BUFFER);
}

static void
ligma_context_buffer_list_thaw (LigmaContainer *container,
                               LigmaContext   *context)
{
  LigmaBuffer *buffer;

  buffer = ligma_context_find_object (context, container,
                                     context->buffer_name,
                                     NULL);

  if (buffer)
    {
      ligma_context_real_set_buffer (context, buffer);
    }
  else
    {
      g_object_notify (G_OBJECT (context), "buffer");
      ligma_context_buffer_changed (context);
    }
}

static void
ligma_context_buffer_removed (LigmaContainer *container,
                             LigmaBuffer    *buffer,
                             LigmaContext   *context)
{
  if (buffer == context->buffer)
    {
      g_signal_handlers_disconnect_by_func (context->buffer,
                                            ligma_context_buffer_dirty,
                                            context);
      g_clear_object (&context->buffer);

      if (! ligma_container_frozen (container))
        ligma_context_buffer_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_buffer (LigmaContext *context,
                              LigmaBuffer  *buffer)
{
  if (context->buffer == buffer)
    return;

  if (context->buffer_name)
    {
      g_clear_pointer (&context->buffer_name, g_free);
    }

  if (context->buffer)
    g_signal_handlers_disconnect_by_func (context->buffer,
                                          ligma_context_buffer_dirty,
                                          context);

  g_set_object (&context->buffer, buffer);

  if (buffer)
    {
      g_signal_connect_object (buffer, "name-changed",
                               G_CALLBACK (ligma_context_buffer_dirty),
                               context,
                               0);

      context->buffer_name = g_strdup (ligma_object_get_name (buffer));
    }

  g_object_notify (G_OBJECT (context), "buffer");
  ligma_context_buffer_changed (context);
}


/*****************************************************************************/
/*  imagefile  ***************************************************************/

LigmaImagefile *
ligma_context_get_imagefile (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->imagefile;
}

void
ligma_context_set_imagefile (LigmaContext   *context,
                            LigmaImagefile *imagefile)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (imagefile == NULL || LIGMA_IS_IMAGEFILE (imagefile));

  context_find_defined (context, LIGMA_CONTEXT_PROP_IMAGEFILE);

  ligma_context_real_set_imagefile (context, imagefile);
}

void
ligma_context_imagefile_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[IMAGEFILE_CHANGED], 0,
                 context->imagefile);
}

static void
ligma_context_imagefile_dirty (LigmaImagefile *imagefile,
                              LigmaContext   *context)
{
  g_free (context->imagefile_name);
  context->imagefile_name = g_strdup (ligma_object_get_name (imagefile));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_IMAGEFILE);
}

static void
ligma_context_imagefile_list_thaw (LigmaContainer *container,
                                  LigmaContext   *context)
{
  LigmaImagefile *imagefile;

  imagefile = ligma_context_find_object (context, container,
                                        context->imagefile_name,
                                        NULL);

  if (imagefile)
    {
      ligma_context_real_set_imagefile (context, imagefile);
    }
  else
    {
      g_object_notify (G_OBJECT (context), "imagefile");
      ligma_context_imagefile_changed (context);
    }
}

static void
ligma_context_imagefile_removed (LigmaContainer *container,
                                LigmaImagefile *imagefile,
                                LigmaContext   *context)
{
  if (imagefile == context->imagefile)
    {
      g_signal_handlers_disconnect_by_func (context->imagefile,
                                            ligma_context_imagefile_dirty,
                                            context);
      g_clear_object (&context->imagefile);

      if (! ligma_container_frozen (container))
        ligma_context_imagefile_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_imagefile (LigmaContext   *context,
                                 LigmaImagefile *imagefile)
{
  if (context->imagefile == imagefile)
    return;

  if (context->imagefile_name)
    {
      g_clear_pointer (&context->imagefile_name, g_free);
    }

  if (context->imagefile)
    g_signal_handlers_disconnect_by_func (context->imagefile,
                                          ligma_context_imagefile_dirty,
                                          context);

  g_set_object (&context->imagefile, imagefile);

  if (imagefile)
    {
      g_signal_connect_object (imagefile, "name-changed",
                               G_CALLBACK (ligma_context_imagefile_dirty),
                               context,
                               0);

      context->imagefile_name = g_strdup (ligma_object_get_name (imagefile));
    }

  g_object_notify (G_OBJECT (context), "imagefile");
  ligma_context_imagefile_changed (context);
}


/*****************************************************************************/
/*  template  ***************************************************************/

LigmaTemplate *
ligma_context_get_template (LigmaContext *context)
{
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  return context->template;
}

void
ligma_context_set_template (LigmaContext  *context,
                           LigmaTemplate *template)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (template == NULL || LIGMA_IS_TEMPLATE (template));

  context_find_defined (context, LIGMA_CONTEXT_PROP_TEMPLATE);

  ligma_context_real_set_template (context, template);
}

void
ligma_context_template_changed (LigmaContext *context)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));

  g_signal_emit (context,
                 ligma_context_signals[TEMPLATE_CHANGED], 0,
                 context->template);
}

static void
ligma_context_template_dirty (LigmaTemplate *template,
                             LigmaContext  *context)
{
  g_free (context->template_name);
  context->template_name = g_strdup (ligma_object_get_name (template));

  g_signal_emit (context, ligma_context_signals[PROP_NAME_CHANGED], 0,
                 LIGMA_CONTEXT_PROP_TEMPLATE);
}

static void
ligma_context_template_list_thaw (LigmaContainer *container,
                                 LigmaContext   *context)
{
  LigmaTemplate *template;

  template = ligma_context_find_object (context, container,
                                       context->template_name,
                                       NULL);

  if (template)
    {
      ligma_context_real_set_template (context, template);
    }
  else
    {
      g_object_notify (G_OBJECT (context), "template");
      ligma_context_template_changed (context);
    }
}

static void
ligma_context_template_removed (LigmaContainer *container,
                               LigmaTemplate  *template,
                               LigmaContext   *context)
{
  if (template == context->template)
    {
      g_signal_handlers_disconnect_by_func (context->template,
                                            ligma_context_template_dirty,
                                            context);
      g_clear_object (&context->template);

      if (! ligma_container_frozen (container))
        ligma_context_template_list_thaw (container, context);
    }
}

static void
ligma_context_real_set_template (LigmaContext  *context,
                                LigmaTemplate *template)
{
  if (context->template == template)
    return;

  if (context->template_name)
    {
      g_clear_pointer (&context->template_name, g_free);
    }

  if (context->template)
    g_signal_handlers_disconnect_by_func (context->template,
                                          ligma_context_template_dirty,
                                          context);

  g_set_object (&context->template, template);

  if (template)
    {
      g_signal_connect_object (template, "name-changed",
                               G_CALLBACK (ligma_context_template_dirty),
                               context,
                               0);

      context->template_name = g_strdup (ligma_object_get_name (template));
    }

  g_object_notify (G_OBJECT (context), "template");
  ligma_context_template_changed (context);
}


/*****************************************************************************/
/*  Line Art  ****************************************************************/

LigmaLineArt *
ligma_context_take_line_art (LigmaContext *context)
{
  LigmaLineArt *line_art;

  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  if (context->line_art)
    {
      g_source_remove (context->line_art_timeout_id);
      context->line_art_timeout_id = 0;

      line_art = context->line_art;
      context->line_art = NULL;
    }
  else
    {
      line_art = ligma_line_art_new ();
    }

  return line_art;
}

/*
 * ligma_context_store_line_art:
 * @context:
 * @line_art:
 *
 * The @context takes ownership of @line_art until the next time it is
 * requested with ligma_context_take_line_art() or until 3 minutes have
 * passed.
 * This function allows to temporarily store the computed line art data
 * in case it is needed very soon again, so that not to free and
 * recompute all the time the data when quickly switching tools.
 */
void
ligma_context_store_line_art (LigmaContext *context,
                             LigmaLineArt *line_art)
{
  g_return_if_fail (LIGMA_IS_CONTEXT (context));
  g_return_if_fail (LIGMA_IS_LINE_ART (line_art));

  if (context->line_art)
    {
      g_source_remove (context->line_art_timeout_id);
      context->line_art_timeout_id = 0;
    }

  context->line_art            = line_art;
  context->line_art_timeout_id = g_timeout_add (180000,
                                                (GSourceFunc) ligma_context_free_line_art,
                                                context);
}

static gboolean
ligma_context_free_line_art (LigmaContext *context)
{
  g_clear_object (&context->line_art);

  context->line_art_timeout_id = 0;

  return G_SOURCE_REMOVE;
}


/*****************************************************************************/
/*  utility functions  *******************************************************/

static gpointer
ligma_context_find_object (LigmaContext   *context,
                          LigmaContainer *container,
                          const gchar   *object_name,
                          gpointer       standard_object)
{
  LigmaObject *object = NULL;

  if (object_name)
    object = ligma_container_get_child_by_name (container, object_name);

  if (! object && ! ligma_container_is_empty (container))
    object = ligma_container_get_child_by_index (container, 0);

  if (! object)
    object = standard_object;

  return object;
}
