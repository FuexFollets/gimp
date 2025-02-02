/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaoperationpassthrough.c
 * Copyright (C) 2008 Michael Natterer <mitch@ligma.org>
 *               2017 Ell
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

#include <gegl-plugin.h>

#include "../operations-types.h"

#include "ligmaoperationpassthrough.h"


G_DEFINE_TYPE (LigmaOperationPassThrough, ligma_operation_pass_through,
               LIGMA_TYPE_OPERATION_REPLACE)


static void
ligma_operation_pass_through_class_init (LigmaOperationPassThroughClass *klass)
{
  GeglOperationClass          *operation_class  = GEGL_OPERATION_CLASS (klass);
  LigmaOperationLayerModeClass *layer_mode_class = LIGMA_OPERATION_LAYER_MODE_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "ligma:pass-through",
                                 "description", "LIGMA pass through mode operation",
                                 NULL);

  /* don't use REPLACE mode's specialized get_affected_region(); PASS_THROUGH
   * behaves like an ordinary layer mode here.
   */
  layer_mode_class->get_affected_region = NULL;
}

static void
ligma_operation_pass_through_init (LigmaOperationPassThrough *self)
{
}
