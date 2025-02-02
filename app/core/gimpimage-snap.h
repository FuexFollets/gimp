/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattisbvf
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

#ifndef __LIGMA_IMAGE_SNAP_H__
#define __LIGMA_IMAGE_SNAP_H__


gboolean    ligma_image_snap_x         (LigmaImage *image,
                                       gdouble    x,
                                       gdouble   *tx,
                                       gdouble    epsilon_x,
                                       gboolean   snap_to_guides,
                                       gboolean   snap_to_grid,
                                       gboolean   snap_to_canvas);
gboolean    ligma_image_snap_y         (LigmaImage *image,
                                       gdouble    y,
                                       gdouble   *ty,
                                       gdouble    epsilon_y,
                                       gboolean   snap_to_guides,
                                       gboolean   snap_to_grid,
                                       gboolean   snap_to_canvas);
gboolean    ligma_image_snap_point     (LigmaImage *image,
                                       gdouble    x,
                                       gdouble    y,
                                       gdouble   *tx,
                                       gdouble   *ty,
                                       gdouble    epsilon_x,
                                       gdouble    epsilon_y,
                                       gboolean   snap_to_guides,
                                       gboolean   snap_to_grid,
                                       gboolean   snap_to_canvas,
                                       gboolean   snap_to_vectors,
                                       gboolean   show_all);
gboolean    ligma_image_snap_rectangle (LigmaImage *image,
                                       gdouble    x1,
                                       gdouble    y1,
                                       gdouble    x2,
                                       gdouble    y2,
                                       gdouble   *tx1,
                                       gdouble   *ty1,
                                       gdouble    epsilon_x,
                                       gdouble    epsilon_y,
                                       gboolean   snap_to_guides,
                                       gboolean   snap_to_grid,
                                       gboolean   snap_to_canvas,
                                       gboolean   snap_to_vectors);


#endif /* __LIGMA_IMAGE_SNAP_H__ */
