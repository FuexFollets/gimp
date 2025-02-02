/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-gegl-loops.h
 * Copyright (C) 2012 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_GEGL_LOOPS_H__
#define __LIGMA_GEGL_LOOPS_H__


void   ligma_gegl_buffer_copy           (GeglBuffer               *src_buffer,
                                        const GeglRectangle      *src_rect,
                                        GeglAbyssPolicy           abyss_policy,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect);

void   ligma_gegl_clear                 (GeglBuffer               *buffer,
                                        const GeglRectangle      *rect);

/*  this is a pretty stupid port of concolve_region() that only works
 *  on a linear source buffer
 */
void   ligma_gegl_convolve              (GeglBuffer               *src_buffer,
                                        const GeglRectangle      *src_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        const gfloat             *kernel,
                                        gint                      kernel_size,
                                        gdouble                   divisor,
                                        LigmaConvolutionType       mode,
                                        gboolean                  alpha_weighting);

void   ligma_gegl_dodgeburn             (GeglBuffer               *src_buffer,
                                        const GeglRectangle      *src_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   exposure,
                                        LigmaDodgeBurnType         type,
                                        LigmaTransferMode          mode);

void   ligma_gegl_smudge_with_paint     (GeglBuffer               *accum_buffer,
                                        const GeglRectangle      *accum_rect,
                                        GeglBuffer               *canvas_buffer,
                                        const GeglRectangle      *canvas_rect,
                                        const LigmaRGB            *brush_color,
                                        GeglBuffer               *paint_buffer,
                                        gboolean                  no_erasing,
                                        gdouble                   flow,
                                        gdouble                   rate);

void   ligma_gegl_apply_mask            (GeglBuffer               *mask_buffer,
                                        const GeglRectangle      *mask_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   opacity);

void   ligma_gegl_combine_mask          (GeglBuffer               *mask_buffer,
                                        const GeglRectangle      *mask_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   opacity);

void   ligma_gegl_combine_mask_weird    (GeglBuffer               *mask_buffer,
                                        const GeglRectangle      *mask_rect,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        gdouble                   opacity,
                                        gboolean                  stipple);

void   ligma_gegl_index_to_mask         (GeglBuffer               *indexed_buffer,
                                        const GeglRectangle      *indexed_rect,
                                        const Babl               *indexed_format,
                                        GeglBuffer               *mask_buffer,
                                        const GeglRectangle      *mask_rect,
                                        gint                      index);

void   ligma_gegl_convert_color_profile (GeglBuffer               *src_buffer,
                                        const GeglRectangle      *src_rect,
                                        LigmaColorProfile         *src_profile,
                                        GeglBuffer               *dest_buffer,
                                        const GeglRectangle      *dest_rect,
                                        LigmaColorProfile         *dest_profile,
                                        LigmaColorRenderingIntent  intent,
                                        gboolean                  bpc,
                                        LigmaProgress             *progress);

void   ligma_gegl_average_color         (GeglBuffer               *buffer,
                                        const GeglRectangle      *rect,
                                        gboolean                  clip_to_buffer,
                                        GeglAbyssPolicy           abyss_policy,
                                        const Babl               *format,
                                        gpointer                  color);


#endif /* __LIGMA_GEGL_LOOPS_H__ */
