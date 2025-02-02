/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmapropgui-motion-blur-linear.c
 * Copyright (C) 2019  Michael Natterer <mitch@ligma.org>
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

#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "propgui-types.h"

#include "core/ligmacontext.h"

#include "ligmapropgui-generic.h"
#include "ligmapropgui-motion-blur-linear.h"

#include "ligma-intl.h"


static void
line_callback (GObject       *config,
               GeglRectangle *area,
               gdouble        x1,
               gdouble        y1,
               gdouble        x2,
               gdouble        y2)
{
  gdouble length;
  gdouble angle;

  g_object_set_data_full (G_OBJECT (config), "area",
                          g_memdup2 (area, sizeof (GeglRectangle)),
                          (GDestroyNotify) g_free);

  length = sqrt (SQR (x2 - x1) + SQR (y2 - y1));
  angle  = atan2 (y2 - y1, x2 - x1);

  angle = angle / G_PI * 180.0;

  g_object_set (config,
                "length", length,
                "angle",  angle,
                NULL);
}

static void
config_notify (GObject          *config,
               const GParamSpec *pspec,
               gpointer          set_data)
{
  LigmaControllerLineCallback  set_func;
  GeglRectangle              *area;
  gdouble                     length;
  gdouble                     angle;
  gdouble                     x1, y1, x2, y2;

  set_func = g_object_get_data (G_OBJECT (config), "set-func");
  area     = g_object_get_data (G_OBJECT (config), "area");

  g_object_get (config,
                "length", &length,
                "angle",  &angle,
                NULL);

  angle = angle / 180.0 * G_PI;

  x1 = area->x + area->width  / 2.0;
  y1 = area->x + area->height / 2.0;
  x2 = x1 + cos (angle) * length;
  y2 = y1 + sin (angle) * length;

  set_func (set_data, area, x1, y1, x2, y2);
}

GtkWidget *
_ligma_prop_gui_new_motion_blur_linear (GObject                  *config,
                                       GParamSpec              **param_specs,
                                       guint                     n_param_specs,
                                       GeglRectangle            *area,
                                       LigmaContext              *context,
                                       LigmaCreatePickerFunc      create_picker_func,
                                       LigmaCreateControllerFunc  create_controller_func,
                                       gpointer                  creator)
{
  GtkWidget *vbox;

  g_return_val_if_fail (G_IS_OBJECT (config), NULL);
  g_return_val_if_fail (param_specs != NULL, NULL);
  g_return_val_if_fail (n_param_specs > 0, NULL);
  g_return_val_if_fail (LIGMA_IS_CONTEXT (context), NULL);

  vbox = _ligma_prop_gui_new_generic (config,
                                     param_specs, n_param_specs,
                                     area, context,
                                     create_picker_func,
                                     create_controller_func,
                                     creator);


  if (create_controller_func)
    {
      GCallback set_func;
      gpointer  set_data;

      set_func = create_controller_func (creator,
                                         LIGMA_CONTROLLER_TYPE_LINE,
                                         _("Linear Motion Blur: "),
                                         (GCallback) line_callback,
                                         config,
                                         &set_data);

      g_object_set_data (G_OBJECT (config), "set-func", set_func);

      g_object_set_data_full (G_OBJECT (config), "area",
                              g_memdup2 (area, sizeof (GeglRectangle)),
                              (GDestroyNotify) g_free);

      config_notify (config, NULL, set_data);

      g_signal_connect (config, "notify",
                        G_CALLBACK (config_notify),
                        set_data);
    }

  return vbox;
}
