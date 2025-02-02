/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * LigmaRc serialization routines
 * Copyright (C) 2001-2005  Sven Neumann <sven@ligma.org>
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

#ifndef __LIGMA_RC_SERIALIZE_H__
#define __LIGMA_RC_SERIALIZE_H__


gboolean  ligma_rc_serialize (LigmaConfig       *config,
                             LigmaConfigWriter *writer,
                             gpointer          data);


#endif /* __LIGMA_RC_SERIALIZE_H__ */
