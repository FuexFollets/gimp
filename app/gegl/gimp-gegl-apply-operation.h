/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-apply-operation.h
 * Copyright (C) 2012 Øyvind Kolås <pippin@ligma.org>
 *                    Sven Neumann <sven@ligma.org>
 *                    Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_GEGL_APPLY_OPERATION_H__
#define __LIGMA_GEGL_APPLY_OPERATION_H__


/*  generic functions, also used by the specific ones below  */

void       ligma_gegl_apply_operation        (GeglBuffer          *src_buffer,
                                             LigmaProgress        *progress,
                                             const gchar         *undo_desc,
                                             GeglNode            *operation,
                                             GeglBuffer          *dest_buffer,
                                             const GeglRectangle *dest_rect,
                                             gboolean             crop_input);

gboolean   ligma_gegl_apply_cached_operation (GeglBuffer          *src_buffer,
                                             LigmaProgress        *progress,
                                             const gchar         *undo_desc,
                                             GeglNode            *operation,
                                             gboolean             connect_src_buffer,
                                             GeglBuffer          *dest_buffer,
                                             const GeglRectangle *dest_rect,
                                             gboolean             crop_input,
                                             GeglBuffer          *cache,
                                             const GeglRectangle *valid_rects,
                                             gint                 n_valid_rects,
                                             gboolean             cancelable);


/*  apply specific operations  */

void   ligma_gegl_apply_dither          (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        gint                    levels,
                                        gint                    dither_type);

void   ligma_gegl_apply_flatten         (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        const LigmaRGB          *background,
                                        const Babl             *space,
                                        LigmaLayerColorSpace     composite_space);

void   ligma_gegl_apply_feather         (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        const GeglRectangle    *dest_rect,
                                        gdouble                 radius_x,
                                        gdouble                 radius_y,
                                        gboolean                edge_lock);

void   ligma_gegl_apply_border          (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        const GeglRectangle    *dest_rect,
                                        gint                    radius_x,
                                        gint                    radius_y,
                                        LigmaChannelBorderStyle  style,
                                        gboolean                edge_lock);

void   ligma_gegl_apply_grow            (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        const GeglRectangle    *dest_rect,
                                        gint                    radius_x,
                                        gint                    radius_y);

void   ligma_gegl_apply_shrink          (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        const GeglRectangle    *dest_rect,
                                        gint                    radius_x,
                                        gint                    radius_y,
                                        gboolean                edge_lock);

void   ligma_gegl_apply_flood           (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        const GeglRectangle    *dest_rect);

/* UGLY: private enum of gegl:gaussian-blur */
typedef enum
{
  GAUSSIAN_BLUR_ABYSS_NONE,
  GAUSSIAN_BLUR_ABYSS_CLAMP
} GaussianBlurAbyssPolicy;

void   ligma_gegl_apply_gaussian_blur   (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        const GeglRectangle    *dest_rect,
                                        gdouble                 std_dev_x,
                                        gdouble                 std_dev_y,
                                        GaussianBlurAbyssPolicy abyss_policy);

void   ligma_gegl_apply_invert_gamma    (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer);

void   ligma_gegl_apply_invert_linear   (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer);

void   ligma_gegl_apply_opacity         (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        GeglBuffer             *mask,
                                        gint                    mask_offset_x,
                                        gint                    mask_offset_y,
                                        gdouble                 opacity);

void   ligma_gegl_apply_scale           (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        LigmaInterpolationType   interpolation_type,
                                        gdouble                 x,
                                        gdouble                 y);

void   ligma_gegl_apply_set_alpha       (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        gdouble                 value);

void   ligma_gegl_apply_threshold       (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        gdouble                 value);

void   ligma_gegl_apply_transform       (GeglBuffer             *src_buffer,
                                        LigmaProgress           *progress,
                                        const gchar            *undo_desc,
                                        GeglBuffer             *dest_buffer,
                                        LigmaInterpolationType   interpolation_type,
                                        LigmaMatrix3            *transform);


#endif /* __LIGMA_GEGL_APPLY_OPERATION_H__ */
