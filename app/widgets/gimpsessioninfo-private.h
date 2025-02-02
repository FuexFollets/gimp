/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasessioninfo-private.h
 * Copyright (C) 2001-2008 Michael Natterer <mitch@ligma.org>
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

#ifndef __LIGMA_SESSION_INFO_PRIVATE_H__
#define __LIGMA_SESSION_INFO_PRIVATE_H__


struct _LigmaSessionInfoPrivate
{
  /*  the dialog factory entry for object we have session info for
   *  note that pure "dock" entries don't have any factory entry
   */
  LigmaDialogFactoryEntry *factory_entry;

  gint                    x;
  gint                    y;
  gint                    width;
  gint                    height;
  gboolean                right_align;
  gboolean                bottom_align;
  GdkMonitor             *monitor;

  /*  only valid while restoring and saving the session  */
  gboolean                open;

  /*  dialog specific list of LigmaSessionInfoAux  */
  GList                  *aux_info;

  GtkWidget              *widget;

  /*  list of LigmaSessionInfoDock  */
  GList                  *docks;
};


#endif /* __LIGMA_SESSION_INFO_PRIVATE_H__ */
