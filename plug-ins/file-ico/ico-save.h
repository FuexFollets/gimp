/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * LIGMA Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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

#ifndef __ICO_SAVE_H__
#define __ICO_SAVE_H__


LigmaPDBStatusType ico_save_image          (GFile         *file,
                                           LigmaImage     *image,
                                           gint32         run_mode,
                                           GError       **error);

LigmaPDBStatusType cur_save_image          (GFile         *file,
                                           LigmaImage     *image,
                                           gint32         run_mode,
                                           gint          *n_hot_spot_x,
                                           gint32       **hot_spot_x,
                                           gint          *n_hot_spot_y,
                                           gint32       **hot_spot_y,
                                           GError       **error);

LigmaPDBStatusType ani_save_image          (GFile         *file,
                                           LigmaImage     *image,
                                           gint32         run_mode,
                                           gint          *n_hot_spot_x,
                                           gint32       **hot_spot_x,
                                           gint          *n_hot_spot_y,
                                           gint32       **hot_spot_y,
                                           AniFileHeader *header,
                                           AniSaveInfo   *ani_info,
                                           GError       **error);

gboolean          ico_cmap_contains_black (const guchar  *cmap,
                                           gint           num_colors);


#endif /* __ICO_SAVE_H__ */
