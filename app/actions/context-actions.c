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

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "core/ligma.h"
#include "core/ligmabrushgenerated.h"
#include "core/ligmacontext.h"
#include "core/ligmalist.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmahelp-ids.h"

#include "actions.h"
#include "context-actions.h"
#include "context-commands.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static const LigmaActionEntry context_actions[] =
{
  { "context-menu",            NULL,                     NC_("context-action",
                                                             "_Context")    },
  { "context-colors-menu",     LIGMA_ICON_COLORS_DEFAULT, NC_("context-action",
                                                             "_Colors")     },
  { "context-opacity-menu",    LIGMA_ICON_TRANSPARENCY,   NC_("context-action",
                                                             "_Opacity")    },
  { "context-paint-mode-menu", LIGMA_ICON_TOOL_PENCIL,    NC_("context-action",
                                                             "Paint _Mode") },
  { "context-tool-menu",       LIGMA_ICON_DIALOG_TOOLS,   NC_("context-action",
                                                             "_Tool")       },
  { "context-brush-menu",      LIGMA_ICON_BRUSH,          NC_("context-action",
                                                             "_Brush")      },
  { "context-pattern-menu",    LIGMA_ICON_PATTERN,        NC_("context-action",
                                                             "_Pattern")    },
  { "context-palette-menu",    LIGMA_ICON_PALETTE,        NC_("context-action",
                                                             "_Palette")    },
  { "context-gradient-menu",   LIGMA_ICON_GRADIENT,       NC_("context-action",
                                                             "_Gradient")   },
  { "context-font-menu",       LIGMA_ICON_FONT,           NC_("context-action",
                                                             "_Font")       },

  { "context-brush-shape-menu",    NULL,                 NC_("context-action",
                                                             "_Shape")      },
  { "context-brush-radius-menu",   NULL,                 NC_("context-action",
                                                             "_Radius")     },
  { "context-brush-spikes-menu",   NULL,                 NC_("context-action",
                                                             "S_pikes")     },
  { "context-brush-hardness-menu", NULL,                 NC_("context-action",
                                                             "_Hardness")   },
  { "context-brush-aspect-menu",   NULL,                 NC_("context-action",
                                                             "_Aspect Ratio")},
  { "context-brush-angle-menu",    NULL,                 NC_("context-action",
                                                             "A_ngle")      },

  { "context-colors-default", LIGMA_ICON_COLORS_DEFAULT,
    NC_("context-action", "_Default Colors"), "D",
    NC_("context-action",
        "Set foreground color to black, background color to white"),
    context_colors_default_cmd_callback,
    LIGMA_HELP_TOOLBOX_DEFAULT_COLORS },

  { "context-colors-swap", LIGMA_ICON_COLORS_SWAP,
    NC_("context-action", "S_wap Colors"), "X",
    NC_("context-action", "Exchange foreground and background colors"),
    context_colors_swap_cmd_callback,
    LIGMA_HELP_TOOLBOX_SWAP_COLORS }
};

static LigmaEnumActionEntry context_palette_foreground_actions[] =
{
  { "context-palette-foreground-set", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Set Color From Palette"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-palette-foreground-first", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Use First Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-foreground-last", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Last Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-foreground-previous", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Previous Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-foreground-next", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Next Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-palette-foreground-previous-skip", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Back Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-palette-foreground-next-skip", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Forward Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static LigmaEnumActionEntry context_palette_background_actions[] =
{
  { "context-palette-background-set", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Set Color From Palette"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-palette-background-first", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Use First Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-background-last", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Use Last Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-background-previous", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Use Previous Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-background-next", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Use Next Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-palette-background-previous-skip", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Skip Back Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-palette-background-next-skip", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Skip Forward Palette Color"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static LigmaEnumActionEntry context_colormap_foreground_actions[] =
{
  { "context-colormap-foreground-set", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Foreground: Set Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-colormap-foreground-first", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use First Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-colormap-foreground-last", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use Last Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-colormap-foreground-previous", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use Previous Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-foreground-next", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Foreground: Use Next Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-colormap-foreground-previous-skip", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Foreground: Skip Back Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-foreground-next-skip", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Foreground: Skip Forward Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static LigmaEnumActionEntry context_colormap_background_actions[] =
{
  { "context-colormap-background-set", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Background: Set Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-colormap-background-first", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Background: Use First Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-colormap-background-last", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Background: Use Last Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-colormap-background-previous", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Background: Use Previous Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-background-next", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Background: Use Next Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-colormap-background-previous-skip", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Background: Skip Back Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-colormap-background-next-skip", LIGMA_ICON_COLORMAP,
    NC_("context-action", "Background: Skip Forward Color From Colormap"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static LigmaEnumActionEntry context_swatch_foreground_actions[] =
{
  { "context-swatch-foreground-set", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Set Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-swatch-foreground-first", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Use First Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-swatch-foreground-last", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Last Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-swatch-foreground-previous", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Previous Color From Swatch"), "9", NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-foreground-next", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Use Next Color From Swatch"), "0", NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-swatch-foreground-previous-skip", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Back Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-foreground-next-skip", LIGMA_ICON_PALETTE,
    NC_("context-action", "Foreground: Skip Forward Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static LigmaEnumActionEntry context_swatch_background_actions[] =
{
  { "context-swatch-background-set", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Set Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, FALSE,
    NULL },
  { "context-swatch-background-first", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Use First Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-swatch-background-last", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Use Last Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-swatch-background-previous", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Use Previous Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-background-next", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Use Next Color From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-swatch-background-previous-skip", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Skip Color Back From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-swatch-background-next-skip", LIGMA_ICON_PALETTE,
    NC_("context-action", "Background: Skip Color Forward From Swatch"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_foreground_red_actions[] =
{
  { "context-foreground-red-set", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-red-minimum", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-red-maximum", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-red-decrease", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-red-increase", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-red-decrease-skip", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-red-increase-skip", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Foreground Red: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_foreground_green_actions[] =
{
  { "context-foreground-green-set", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-green-minimum", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-green-maximum", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-green-decrease", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-green-increase", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-green-decrease-skip", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-green-increase-skip", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Foreground Green: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_foreground_blue_actions[] =
{
  { "context-foreground-blue-set", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-blue-minimum", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-blue-maximum", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-blue-decrease", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-blue-increase", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-blue-decrease-skip", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-blue-increase-skip", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Foreground Blue: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_background_red_actions[] =
{
  { "context-background-red-set", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-red-minimum", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-red-maximum", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-red-decrease", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-red-increase", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-red-decrease-skip", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-red-increase-skip", LIGMA_ICON_CHANNEL_RED,
    NC_("context-action", "Background Red: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_background_green_actions[] =
{
  { "context-background-green-set", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-green-minimum", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-green-maximum", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-green-decrease", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-green-increase", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-green-decrease-skip", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-green-increase-skip", LIGMA_ICON_CHANNEL_GREEN,
    NC_("context-action", "Background Green: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_background_blue_actions[] =
{
  { "context-background-blue-set", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-blue-minimum", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-blue-maximum", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-blue-decrease", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-blue-increase", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-blue-decrease-skip", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-blue-increase-skip", LIGMA_ICON_CHANNEL_BLUE,
    NC_("context-action", "Background Blue: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_foreground_hue_actions[] =
{
  { "context-foreground-hue-set", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-hue-minimum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-hue-maximum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-hue-decrease", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-hue-increase", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-hue-decrease-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-hue-increase-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Hue: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_foreground_saturation_actions[] =
{
  { "context-foreground-saturation-set", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-saturation-minimum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-saturation-maximum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-saturation-decrease", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-saturation-increase", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-saturation-decrease-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-saturation-increase-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Saturation: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_foreground_value_actions[] =
{
  { "context-foreground-value-set", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-foreground-value-minimum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-foreground-value-maximum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-foreground-value-decrease", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-value-increase", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-foreground-value-decrease-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-foreground-value-increase-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Foreground Value: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_background_hue_actions[] =
{
  { "context-background-hue-set", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-hue-minimum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-hue-maximum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-hue-decrease", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-hue-increase", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-hue-decrease-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-hue-increase-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Hue: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_background_saturation_actions[] =
{
  { "context-background-saturation-set", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-saturation-minimum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-saturation-maximum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-saturation-decrease", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-saturation-increase", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-saturation-decrease-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-saturation-increase-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Saturation: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_background_value_actions[] =
{
  { "context-background-value-set", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-background-value-minimum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-background-value-maximum", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-background-value-decrease", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Decrease by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-background-value-increase", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Increase by 1%"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-background-value-decrease-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Decrease by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-background-value-increase-skip", LIGMA_ICON_TOOL_HUE_SATURATION,
    NC_("context-action", "Background Value: Increase by 10%"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_opacity_actions[] =
{
  { "context-opacity-set", LIGMA_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Set Transparency"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-opacity-transparent", LIGMA_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make Completely Transparent"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-opacity-opaque", LIGMA_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make Completely Opaque"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-opacity-decrease", LIGMA_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 1% More Transparent"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-opacity-increase", LIGMA_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 1% More Opaque"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-opacity-decrease-skip", LIGMA_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 10% More Transparent"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-opacity-increase-skip", LIGMA_ICON_TRANSPARENCY,
    NC_("context-action", "Tool Opacity: Make 10% More Opaque"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_paint_mode_actions[] =
{
  { "context-paint-mode-first", LIGMA_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select First"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-paint-mode-last", LIGMA_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select Last"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-paint-mode-previous", LIGMA_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select Previous"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-paint-mode-next", LIGMA_ICON_TOOL_PENCIL,
    NC_("context-action", "Tool Paint Mode: Select Next"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_tool_select_actions[] =
{
  { "context-tool-select-set", LIGMA_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Choose by Index"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-tool-select-first", LIGMA_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to First"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-tool-select-last", LIGMA_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to Last"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-tool-select-previous", LIGMA_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to Previous"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-tool-select-next", LIGMA_ICON_DIALOG_TOOLS,
    NC_("context-action", "Tool Selection: Switch to Next"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_brush_select_actions[] =
{
  { "context-brush-select-set", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Select by Index"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-select-first", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to First"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-select-last", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to Last"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-select-previous", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to Previous"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-select-next", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Selection: Switch to Next"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_pattern_select_actions[] =
{
  { "context-pattern-select-set", LIGMA_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Select by Index"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-pattern-select-first", LIGMA_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to First"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-pattern-select-last", LIGMA_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to Last"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-pattern-select-previous", LIGMA_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to Previous"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-pattern-select-next", LIGMA_ICON_PATTERN,
    NC_("context-action", "Pattern Selection: Switch to Next"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_palette_select_actions[] =
{
  { "context-palette-select-set", LIGMA_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Select by Index"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-palette-select-first", LIGMA_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to First"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-palette-select-last", LIGMA_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to Last"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-palette-select-previous", LIGMA_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to Previous"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-palette-select-next", LIGMA_ICON_PALETTE,
    NC_("context-action", "Palette Selection: Switch to Next"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_gradient_select_actions[] =
{
  { "context-gradient-select-set", LIGMA_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Select by Index"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-gradient-select-first", LIGMA_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to First"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-gradient-select-last", LIGMA_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to Last"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-gradient-select-previous", LIGMA_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to Previous"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-gradient-select-next", LIGMA_ICON_GRADIENT,
    NC_("context-action", "Gradient Selection: Switch to Next"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_font_select_actions[] =
{
  { "context-font-select-set", LIGMA_ICON_FONT,
    NC_("context-action", "Font Selection: Select by Index"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-font-select-first", LIGMA_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to First"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-font-select-last", LIGMA_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to Last"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-font-select-previous", LIGMA_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to Previous"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-font-select-next", LIGMA_ICON_FONT,
    NC_("context-action", "Font Selection: Switch to Next"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_brush_spacing_actions[] =
{
  { "context-brush-spacing-set", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-spacing-minimum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-spacing-maximum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-spacing-decrease", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spacing-increase", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-spacing-decrease-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Decrease by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spacing-increase-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spacing (Editor): Increase by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_brush_shape_actions[] =
{
  { "context-brush-shape-circle", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Shape (Editor): Use Circular"), NULL, NULL,
    LIGMA_BRUSH_GENERATED_CIRCLE, FALSE,
    NULL },
  { "context-brush-shape-square", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Shape (Editor): Use Square"), NULL, NULL,
    LIGMA_BRUSH_GENERATED_SQUARE, FALSE,
    NULL },
  { "context-brush-shape-diamond", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Shape (Editor): Use Diamond"), NULL, NULL,
    LIGMA_BRUSH_GENERATED_DIAMOND, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_brush_radius_actions[] =
{
  { "context-brush-radius-set", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-radius-minimum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-radius-maximum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-radius-decrease-less", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease by 0.1"), NULL, NULL,
    LIGMA_ACTION_SELECT_SMALL_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-less", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase by 0.1"), NULL, NULL,
    LIGMA_ACTION_SELECT_SMALL_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase by 10"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL },
  { "context-brush-radius-decrease-percent", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Decrease Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-radius-increase-percent", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Radius (Editor): Increase Relative"), NULL, NULL,
    LIGMA_ACTION_SELECT_PERCENT_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_brush_spikes_actions[] =
{
  { "context-brush-spikes-set", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-spikes-minimum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-spikes-maximum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-spikes-decrease", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spikes-increase", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-spikes-decrease-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Decrease by 4"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-spikes-increase-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Spikes (Editor): Increase by 4"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_brush_hardness_actions[] =
{
  { "context-brush-hardness-set", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-hardness-minimum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-hardness-maximum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-hardness-decrease", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Decrease by 0.01"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-hardness-increase", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Increase by 0.01"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-hardness-decrease-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Decrease by 0.1"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-hardness-increase-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Hardness (Editor): Increase by 0.1"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_brush_aspect_actions[] =
{
  { "context-brush-aspect-set", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-aspect-minimum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Set to Minimum"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-aspect-maximum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Set to Maximum"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-aspect-decrease", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Decrease by 0.1"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-aspect-increase", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Increase by 0.1"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-aspect-decrease-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Decrease by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-aspect-increase-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Aspect Ratio (Editor): Increase by 1"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaEnumActionEntry context_brush_angle_actions[] =
{
  { "context-brush-angle-set", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Set"), NULL, NULL,
    LIGMA_ACTION_SELECT_SET, TRUE,
    NULL },
  { "context-brush-angle-minimum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Make Horizontal"), NULL, NULL,
    LIGMA_ACTION_SELECT_FIRST, FALSE,
    NULL },
  { "context-brush-angle-maximum", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Make Vertical"), NULL, NULL,
    LIGMA_ACTION_SELECT_LAST, FALSE,
    NULL },
  { "context-brush-angle-decrease", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Right by 1°"), NULL, NULL,
    LIGMA_ACTION_SELECT_PREVIOUS, FALSE,
    NULL },
  { "context-brush-angle-increase", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Left by 1°"), NULL, NULL,
    LIGMA_ACTION_SELECT_NEXT, FALSE,
    NULL },
  { "context-brush-angle-decrease-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Right by 15°"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_PREVIOUS, FALSE,
    NULL },
  { "context-brush-angle-increase-skip", LIGMA_ICON_BRUSH,
    NC_("context-action", "Brush Angle (Editor): Rotate Left by 15°"), NULL, NULL,
    LIGMA_ACTION_SELECT_SKIP_NEXT, FALSE,
    NULL }
};

static const LigmaToggleActionEntry context_toggle_actions[] =
{
  { "context-dynamics-toggle", NULL,
    NC_("context-action", "_Enable/Disable Dynamics"), NULL,
    NC_("context-action", "Apply or ignore the dynamics when painting"),
    context_toggle_dynamics_cmd_callback,
    FALSE,
    NULL },
};


void
context_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "context-action",
                                 context_actions,
                                 G_N_ELEMENTS (context_actions));

  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_palette_foreground_actions,
                                      G_N_ELEMENTS (context_palette_foreground_actions),
                                      context_palette_foreground_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_palette_background_actions,
                                      G_N_ELEMENTS (context_palette_background_actions),
                                      context_palette_background_cmd_callback);

  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_colormap_foreground_actions,
                                      G_N_ELEMENTS (context_colormap_foreground_actions),
                                      context_colormap_foreground_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_colormap_background_actions,
                                      G_N_ELEMENTS (context_colormap_background_actions),
                                      context_colormap_background_cmd_callback);

  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_swatch_foreground_actions,
                                      G_N_ELEMENTS (context_swatch_foreground_actions),
                                      context_swatch_foreground_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_swatch_background_actions,
                                      G_N_ELEMENTS (context_swatch_background_actions),
                                      context_swatch_background_cmd_callback);


  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_red_actions,
                                      G_N_ELEMENTS (context_foreground_red_actions),
                                      context_foreground_red_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_green_actions,
                                      G_N_ELEMENTS (context_foreground_green_actions),
                                      context_foreground_green_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_blue_actions,
                                      G_N_ELEMENTS (context_foreground_blue_actions),
                                      context_foreground_blue_cmd_callback);

  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_hue_actions,
                                      G_N_ELEMENTS (context_foreground_hue_actions),
                                      context_foreground_hue_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_saturation_actions,
                                      G_N_ELEMENTS (context_foreground_saturation_actions),
                                      context_foreground_saturation_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_foreground_value_actions,
                                      G_N_ELEMENTS (context_foreground_value_actions),
                                      context_foreground_value_cmd_callback);

  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_background_red_actions,
                                      G_N_ELEMENTS (context_background_red_actions),
                                      context_background_red_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_background_green_actions,
                                      G_N_ELEMENTS (context_background_green_actions),
                                      context_background_green_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_background_blue_actions,
                                      G_N_ELEMENTS (context_background_blue_actions),
                                      context_background_blue_cmd_callback);

  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_background_hue_actions,
                                      G_N_ELEMENTS (context_background_hue_actions),
                                      context_background_hue_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_background_saturation_actions,
                                      G_N_ELEMENTS (context_background_saturation_actions),
                                      context_background_saturation_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_background_value_actions,
                                      G_N_ELEMENTS (context_background_value_actions),
                                      context_background_value_cmd_callback);

  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_opacity_actions,
                                      G_N_ELEMENTS (context_opacity_actions),
                                      context_opacity_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_paint_mode_actions,
                                      G_N_ELEMENTS (context_paint_mode_actions),
                                      context_paint_mode_cmd_callback);

  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_tool_select_actions,
                                      G_N_ELEMENTS (context_tool_select_actions),
                                      context_tool_select_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_brush_select_actions,
                                      G_N_ELEMENTS (context_brush_select_actions),
                                      context_brush_select_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_pattern_select_actions,
                                      G_N_ELEMENTS (context_pattern_select_actions),
                                      context_pattern_select_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_palette_select_actions,
                                      G_N_ELEMENTS (context_palette_select_actions),
                                      context_palette_select_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_gradient_select_actions,
                                      G_N_ELEMENTS (context_gradient_select_actions),
                                      context_gradient_select_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_font_select_actions,
                                      G_N_ELEMENTS (context_font_select_actions),
                                      context_font_select_cmd_callback);

  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_brush_spacing_actions,
                                      G_N_ELEMENTS (context_brush_spacing_actions),
                                      context_brush_spacing_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_brush_shape_actions,
                                      G_N_ELEMENTS (context_brush_shape_actions),
                                      context_brush_shape_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_brush_radius_actions,
                                      G_N_ELEMENTS (context_brush_radius_actions),
                                      context_brush_radius_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_brush_spikes_actions,
                                      G_N_ELEMENTS (context_brush_spikes_actions),
                                      context_brush_spikes_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_brush_hardness_actions,
                                      G_N_ELEMENTS (context_brush_hardness_actions),
                                      context_brush_hardness_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_brush_aspect_actions,
                                      G_N_ELEMENTS (context_brush_aspect_actions),
                                      context_brush_aspect_cmd_callback);
  ligma_action_group_add_enum_actions (group, "context-action",
                                      context_brush_angle_actions,
                                      G_N_ELEMENTS (context_brush_angle_actions),
                                      context_brush_angle_cmd_callback);

  ligma_action_group_add_toggle_actions (group, "context-action",
                                        context_toggle_actions,
                                        G_N_ELEMENTS (context_toggle_actions));
}

void
context_actions_update (LigmaActionGroup *group,
                        gpointer         data)
{
#if 0
  LigmaContext *context   = action_data_get_context (data);
  gboolean     generated = FALSE;
  gdouble      radius    = 0.0;
  gint         spikes    = 0;
  gdouble      hardness  = 0.0;
  gdouble      aspect    = 0.0;
  gdouble      angle     = 0.0;

  if (context)
    {
      LigmaBrush *brush = ligma_context_get_brush (context);

      if (LIGMA_IS_BRUSH_GENERATED (brush))
        {
          LigmaBrushGenerated *gen = LIGMA_BRUSH_GENERATED (brush);

          generated = TRUE;

          radius   = ligma_brush_generated_get_radius       (gen);
          spikes   = ligma_brush_generated_get_spikes       (gen);
          hardness = ligma_brush_generated_get_hardness     (gen);
          aspect   = ligma_brush_generated_get_aspect_ratio (gen);
          angle    = ligma_brush_generated_get_angle        (gen);
        }
    }

#define SET_SENSITIVE(action,condition) \
        ligma_action_group_set_action_sensitive (group, "context-" action, (condition) != 0)

  SET_SENSITIVE ("brush-radius-minimum",       generated && radius > 1.0);
  SET_SENSITIVE ("brush-radius-decrease",      generated && radius > 1.0);
  SET_SENSITIVE ("brush-radius-decrease-skip", generated && radius > 1.0);

  SET_SENSITIVE ("brush-radius-maximum",       generated && radius < 4000.0);
  SET_SENSITIVE ("brush-radius-increase",      generated && radius < 4000.0);
  SET_SENSITIVE ("brush-radius-increase-skip", generated && radius < 4000.0);

  SET_SENSITIVE ("brush-angle-minimum",       generated);
  SET_SENSITIVE ("brush-angle-decrease",      generated);
  SET_SENSITIVE ("brush-angle-decrease-skip", generated);

  SET_SENSITIVE ("brush-angle-maximum",       generated);
  SET_SENSITIVE ("brush-angle-increase",      generated);
  SET_SENSITIVE ("brush-angle-increase-skip", generated);
#undef SET_SENSITIVE

#endif
}
