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

#ifndef __MENUS_H__
#define __MENUS_H__


extern LigmaMenuFactory *global_menu_factory;


void       menus_init    (Ligma               *ligma,
                          LigmaActionFactory  *action_factory);
void       menus_exit    (Ligma               *ligma);

void       menus_restore (Ligma               *ligma);
void       menus_save    (Ligma               *ligma,
                          gboolean            always_save);

gboolean   menus_clear   (Ligma               *ligma,
                          GError            **error);
void       menus_remove  (Ligma               *ligma);


#endif /* __MENUS_H__ */
