/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaimageconvert_pdb.h
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

#ifndef __LIGMA_IMAGE_CONVERT_PDB_H__
#define __LIGMA_IMAGE_CONVERT_PDB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


gboolean ligma_image_convert_rgb               (LigmaImage              *image);
gboolean ligma_image_convert_grayscale         (LigmaImage              *image);
gboolean ligma_image_convert_indexed           (LigmaImage              *image,
                                               LigmaConvertDitherType   dither_type,
                                               LigmaConvertPaletteType  palette_type,
                                               gint                    num_cols,
                                               gboolean                alpha_dither,
                                               gboolean                remove_unused,
                                               const gchar            *palette);
gboolean ligma_image_convert_set_dither_matrix (gint                    width,
                                               gint                    height,
                                               gint                    matrix_length,
                                               const guint8           *matrix);
gboolean ligma_image_convert_precision         (LigmaImage              *image,
                                               LigmaPrecision           precision);


G_END_DECLS

#endif /* __LIGMA_IMAGE_CONVERT_PDB_H__ */
