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

#ifndef __SCREENSHOT_X11_H__
#define __SCREENSHOT_X11_H__


#ifdef GDK_WINDOWING_X11

gboolean               screenshot_x11_available        (void);

ScreenshotCapabilities screenshot_x11_get_capabilities (void);

LigmaPDBStatusType      screenshot_x11_shoot            (ScreenshotValues  *shootvals,
                                                        GdkMonitor        *monitor,
                                                        LigmaImage        **image,
                                                        GError           **error);

#endif /* GDK_WINDOWING_X11 */


#endif /* __SCREENSHOT_X11_H__ */
