/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Config file serialization and deserialization interface
 * Copyright (C) 2001-2002  Sven Neumann <sven@ligma.org>
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

#include <glib.h>

#include "ligmaconfig-error.h"


/**
 * SECTION: ligmaconfig-error
 * @title: LigmaConfig-error
 * @short_description: Error utils for libligmaconfig.
 *
 * Error utils for libligmaconfig.
 **/


/**
 * ligma_config_error_quark:
 *
 * This function is never called directly. Use LIGMA_CONFIG_ERROR() instead.
 *
 * Returns: the #GQuark that defines the LigmaConfig error domain.
 *
 * Since: 2.4
 **/
GQuark
ligma_config_error_quark (void)
{
  return g_quark_from_static_string ("ligma-config-error-quark");
}
