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

#ifndef __TEXT_ENUMS_H__
#define __TEXT_ENUMS_H__


#define LIGMA_TYPE_TEXT_BOX_MODE (ligma_text_box_mode_get_type ())

GType ligma_text_box_mode_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TEXT_BOX_DYNAMIC, /*< desc="Dynamic" >*/
  LIGMA_TEXT_BOX_FIXED    /*< desc="Fixed"   >*/
} LigmaTextBoxMode;


#define LIGMA_TYPE_TEXT_OUTLINE (ligma_text_outline_get_type ())

GType ligma_text_outline_get_type (void) G_GNUC_CONST;

typedef enum
{
  LIGMA_TEXT_OUTLINE_NONE,        /*< desc="Filled"              >*/
  LIGMA_TEXT_OUTLINE_STROKE_ONLY, /*< desc="Outlined"            >*/
  LIGMA_TEXT_OUTLINE_STROKE_FILL  /*< desc="Outlined and filled" >*/
} LigmaTextOutline;


#endif /* __TEXT_ENUMS_H__ */
