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

#ifndef __XCF_H__
#define __XCF_H__


void        xcf_init        (Ligma           *ligma);
void        xcf_exit        (Ligma           *ligma);

LigmaImage * xcf_load_stream (Ligma           *ligma,
                             GInputStream   *input,
                             GFile          *input_file,
                             LigmaProgress   *progress,
                             GError        **error);

gboolean    xcf_save_stream (Ligma           *ligma,
                             LigmaImage      *image,
                             GOutputStream  *output,
                             GFile          *output_file,
                             LigmaProgress   *progress,
                             GError        **error);

#endif /* __XCF_H__ */
