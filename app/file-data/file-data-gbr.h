/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#ifndef __FILE_DATA_GBR_H__
#define __FILE_DATA_GBR_H__


LigmaValueArray * file_gbr_load_invoker      (LigmaProcedure         *procedure,
                                             Ligma                  *ligma,
                                             LigmaContext           *context,
                                             LigmaProgress          *progress,
                                             const LigmaValueArray  *args,
                                             GError               **error);

LigmaValueArray * file_gbr_save_invoker      (LigmaProcedure         *procedure,
                                             Ligma                  *ligma,
                                             LigmaContext           *context,
                                             LigmaProgress          *progress,
                                             const LigmaValueArray  *args,
                                             GError               **error);

LigmaLayer      * file_gbr_brush_to_layer    (LigmaImage             *image,
                                             LigmaBrush             *brush);
LigmaBrush      * file_gbr_drawable_to_brush (LigmaDrawable          *drawable,
                                             const GeglRectangle   *rect,
                                             const gchar           *name,
                                             gdouble                spacing);


#endif /* __FILE_DATA_GBR_H__ */
