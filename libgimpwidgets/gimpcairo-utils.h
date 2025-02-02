/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmacairo-utils.h
 * Copyright (C) 2007 Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_CAIRO_UTILS_H__
#define __LIGMA_CAIRO_UTILS_H__


gboolean          ligma_cairo_set_focus_line_pattern     (cairo_t       *cr,
                                                         GtkWidget     *widget);

cairo_surface_t * ligma_cairo_surface_create_from_pixbuf (GdkPixbuf     *pixbuf);


#endif /* __LIGMA_CAIRO_UTILS_H__ */
