/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmapluginmanager-query.h
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

#ifndef __LIGMA_PLUG_IN_MANAGER_QUERY_H__
#define __LIGMA_PLUG_IN_MANAGER_QUERY_H__


gint   ligma_plug_in_manager_query (LigmaPlugInManager   *manager,
                                   const gchar         *search_str,
                                   gchar             ***procedure_strs,
                                   gchar             ***accel_strs,
                                   gchar             ***prog_strs,
                                   gint32             **time_ints);


#endif /* __LIGMA_PLUG_IN_MANAGER_QUERY_H__ */
