/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmavectors_pdb.h
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

#ifndef __LIGMA_VECTORS_PDB_H__
#define __LIGMA_VECTORS_PDB_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


LigmaVectors*          ligma_vectors_new                       (LigmaImage               *image,
                                                              const gchar             *name);
LigmaVectors*          ligma_vectors_new_from_text_layer       (LigmaImage               *image,
                                                              LigmaLayer               *layer);
LigmaVectors*          ligma_vectors_copy                      (LigmaVectors             *vectors);
gint*                 ligma_vectors_get_strokes               (LigmaVectors             *vectors,
                                                              gint                    *num_strokes);
gdouble               ligma_vectors_stroke_get_length         (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  precision);
gboolean              ligma_vectors_stroke_get_point_at_dist  (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  dist,
                                                              gdouble                  precision,
                                                              gdouble                 *x_point,
                                                              gdouble                 *y_point,
                                                              gdouble                 *slope,
                                                              gboolean                *valid);
gboolean              ligma_vectors_remove_stroke             (LigmaVectors             *vectors,
                                                              gint                     stroke_id);
gboolean              ligma_vectors_stroke_close              (LigmaVectors             *vectors,
                                                              gint                     stroke_id);
gboolean              ligma_vectors_stroke_reverse            (LigmaVectors             *vectors,
                                                              gint                     stroke_id);
gboolean              ligma_vectors_stroke_translate          (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  off_x,
                                                              gdouble                  off_y);
gboolean              ligma_vectors_stroke_scale              (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  scale_x,
                                                              gdouble                  scale_y);
gboolean              ligma_vectors_stroke_rotate             (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  center_x,
                                                              gdouble                  center_y,
                                                              gdouble                  angle);
gboolean              ligma_vectors_stroke_flip               (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              LigmaOrientationType      flip_type,
                                                              gdouble                  axis);
gboolean              ligma_vectors_stroke_flip_free          (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  x1,
                                                              gdouble                  y1,
                                                              gdouble                  x2,
                                                              gdouble                  y2);
LigmaVectorsStrokeType ligma_vectors_stroke_get_points         (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gint                    *num_points,
                                                              gdouble                **controlpoints,
                                                              gboolean                *closed);
gint                  ligma_vectors_stroke_new_from_points    (LigmaVectors             *vectors,
                                                              LigmaVectorsStrokeType    type,
                                                              gint                     num_points,
                                                              const gdouble           *controlpoints,
                                                              gboolean                 closed);
gdouble*              ligma_vectors_stroke_interpolate        (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  precision,
                                                              gint                    *num_coords,
                                                              gboolean                *closed);
gint                  ligma_vectors_bezier_stroke_new_moveto  (LigmaVectors             *vectors,
                                                              gdouble                  x0,
                                                              gdouble                  y0);
gboolean              ligma_vectors_bezier_stroke_lineto      (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  x0,
                                                              gdouble                  y0);
gboolean              ligma_vectors_bezier_stroke_conicto     (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  x0,
                                                              gdouble                  y0,
                                                              gdouble                  x1,
                                                              gdouble                  y1);
gboolean              ligma_vectors_bezier_stroke_cubicto     (LigmaVectors             *vectors,
                                                              gint                     stroke_id,
                                                              gdouble                  x0,
                                                              gdouble                  y0,
                                                              gdouble                  x1,
                                                              gdouble                  y1,
                                                              gdouble                  x2,
                                                              gdouble                  y2);
gint                  ligma_vectors_bezier_stroke_new_ellipse (LigmaVectors             *vectors,
                                                              gdouble                  x0,
                                                              gdouble                  y0,
                                                              gdouble                  radius_x,
                                                              gdouble                  radius_y,
                                                              gdouble                  angle);
gboolean              ligma_vectors_import_from_file          (LigmaImage               *image,
                                                              GFile                   *file,
                                                              gboolean                 merge,
                                                              gboolean                 scale,
                                                              gint                    *num_vectors,
                                                              LigmaVectors           ***vectors);
gboolean              ligma_vectors_import_from_string        (LigmaImage               *image,
                                                              const gchar             *string,
                                                              gint                     length,
                                                              gboolean                 merge,
                                                              gboolean                 scale,
                                                              gint                    *num_vectors,
                                                              LigmaVectors           ***vectors);
gboolean              ligma_vectors_export_to_file            (LigmaImage               *image,
                                                              GFile                   *file,
                                                              LigmaVectors             *vectors);
gchar*                ligma_vectors_export_to_string          (LigmaImage               *image,
                                                              LigmaVectors             *vectors);


G_END_DECLS

#endif /* __LIGMA_VECTORS_PDB_H__ */
