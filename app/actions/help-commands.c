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

#include "core/ligmaprogress.h"

#include "widgets/ligmahelp.h"

#include "actions.h"
#include "help-commands.h"


void
help_help_cmd_callback (LigmaAction *action,
                        GVariant   *value,
                        gpointer    data)
{
  Ligma        *ligma;
  LigmaDisplay *display;
  return_if_no_ligma (ligma, data);
  return_if_no_display (display, data);

  ligma_help_show (ligma, LIGMA_PROGRESS (display), NULL, NULL);
}

void
help_context_help_cmd_callback (LigmaAction *action,
                                GVariant   *value,
                                gpointer    data)
{
  GtkWidget *widget;
  return_if_no_widget (widget, data);

  ligma_context_help (widget);
}
