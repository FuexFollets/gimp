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

#ifndef __COLORMAP_COMMANDS_H__
#define __COLORMAP_COMMANDS_H__


void   colormap_edit_color_cmd_callback   (LigmaAction *action,
                                           GVariant   *value,
                                           gpointer    data);
void   colormap_add_color_cmd_callback    (LigmaAction *action,
                                           GVariant   *value,
                                           gpointer    data);
void   colormap_to_selection_cmd_callback (LigmaAction *action,
                                           GVariant   *value,
                                           gpointer    data);


#endif /* __COLORMAP_COMMANDS_H__ */
