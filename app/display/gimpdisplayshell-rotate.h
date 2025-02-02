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

#ifndef __LIGMA_DISPLAY_SHELL_ROTATE_H__
#define __LIGMA_DISPLAY_SHELL_ROTATE_H__


void   ligma_display_shell_flip                    (LigmaDisplayShell *shell,
                                                   gboolean          flip_horizontally,
                                                   gboolean          flip_vertically);

void   ligma_display_shell_rotate                  (LigmaDisplayShell *shell,
                                                   gdouble           delta);
void   ligma_display_shell_rotate_to               (LigmaDisplayShell *shell,
                                                   gdouble           value);
void   ligma_display_shell_rotate_drag             (LigmaDisplayShell *shell,
                                                   gdouble           last_x,
                                                   gdouble           last_y,
                                                   gdouble           cur_x,
                                                   gdouble           cur_y,
                                                   gboolean          constrain);

void   ligma_display_shell_rotate_update_transform (LigmaDisplayShell *shell);


#endif  /*  __LIGMA_DISPLAY_SHELL_ROTATE_H__  */
