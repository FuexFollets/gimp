/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmawidgets-error.h
 * Copyright (C) 2008  Martin Nordholts <martinn@svn.gnome.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__LIGMA_WIDGETS_H_INSIDE__) && !defined (LIGMA_WIDGETS_COMPILATION)
#error "Only <libligmawidgets/ligmawidgets.h> can be included directly."
#endif

#ifndef __LIGMA_WIDGETS_ERROR_H__
#define __LIGMA_WIDGETS_ERROR_H__

G_BEGIN_DECLS


/**
 * LigmaWidgetsError:
 * @LIGMA_WIDGETS_PARSE_ERROR: A parse error has occurred
 *
 * Types of errors returned by libligmawidgets functions
 **/
typedef enum
{
  LIGMA_WIDGETS_PARSE_ERROR
} LigmaWidgetsError;


/**
 * LIGMA_WIDGETS_ERROR:
 *
 * The LIGMA widgets error domain.
 *
 * Since: 2.8
 */
#define LIGMA_WIDGETS_ERROR (ligma_widgets_error_quark ())

GQuark  ligma_widgets_error_quark (void) G_GNUC_CONST;


G_END_DECLS

#endif  /* __LIGMA_WIDGETS_ERROR_H__ */
