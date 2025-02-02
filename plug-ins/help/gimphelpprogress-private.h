/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * The LIGMA Help plug-in
 * Copyright (C) 1999-2008 Sven Neumann <sven@ligma.org>
 *                         Michael Natterer <mitch@ligma.org>
 *                         Henrik Brix Andersen <brix@ligma.org>
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

#ifndef __LIGMA_HELP_PROGRESS_PRIVATE_H__
#define __LIGMA_HELP_PROGRESS_PRIVATE_H__


/*  internal API  */

void  _ligma_help_progress_start  (LigmaHelpProgress   *progress,
                                  GCancellable       *cancellable,
                                  const gchar        *format,
                                  ...) G_GNUC_PRINTF (3, 4)       G_GNUC_INTERNAL;
void  _ligma_help_progress_update (LigmaHelpProgress   *progress,
                                  gdouble             percentage) G_GNUC_INTERNAL;
void  _ligma_help_progress_pulse  (LigmaHelpProgress   *progress)   G_GNUC_INTERNAL;
void  _ligma_help_progress_finish (LigmaHelpProgress   *progress)   G_GNUC_INTERNAL;


#endif /* ! __LIGMA_HELP_PROGRESS_PRIVATE_H__ */
