/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaimagesamplepoints_pdb.h
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

/* NOTE: This file is auto-generated by pdbgen.pl */

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_IMAGE_SAMPLE_POINTS_PDB_H__
#define __LIGMA_IMAGE_SAMPLE_POINTS_PDB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


guint    ligma_image_add_sample_point          (LigmaImage *image,
                                               gint       position_x,
                                               gint       position_y);
gboolean ligma_image_delete_sample_point       (LigmaImage *image,
                                               guint      sample_point);
guint    ligma_image_find_next_sample_point    (LigmaImage *image,
                                               guint      sample_point);
gint     ligma_image_get_sample_point_position (LigmaImage *image,
                                               guint      sample_point,
                                               gint      *position_y);


G_END_DECLS

#endif /* __LIGMA_IMAGE_SAMPLE_POINTS_PDB_H__ */
