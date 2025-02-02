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

#ifndef __LIGMA_DISPLAY_SHELL_CALLBACKS_H__
#define __LIGMA_DISPLAY_SHELL_CALLBACKS_H__


void       ligma_display_shell_canvas_realize          (GtkWidget        *widget,
                                                       LigmaDisplayShell *shell);
void       ligma_display_shell_canvas_size_allocate    (GtkWidget        *widget,
                                                       GtkAllocation    *alloc,
                                                       LigmaDisplayShell *shell);
gboolean   ligma_display_shell_canvas_draw             (GtkWidget        *widget,
                                                       cairo_t          *cr,
                                                       LigmaDisplayShell *shell);

gboolean   ligma_display_shell_origin_button_press     (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       LigmaDisplayShell *shell);

gboolean   ligma_display_shell_quick_mask_button_press (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       LigmaDisplayShell *shell);
void       ligma_display_shell_quick_mask_toggled      (GtkWidget        *widget,
                                                       LigmaDisplayShell *shell);

gboolean   ligma_display_shell_navigation_button_press (GtkWidget        *widget,
                                                       GdkEventButton   *bevent,
                                                       LigmaDisplayShell *shell);


#endif /* __LIGMA_DISPLAY_SHELL_CALLBACKS_H__ */
