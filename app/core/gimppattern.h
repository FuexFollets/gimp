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

#ifndef __LIGMA_PATTERN_H__
#define __LIGMA_PATTERN_H__


#include "ligmadata.h"


#define LIGMA_TYPE_PATTERN            (ligma_pattern_get_type ())
#define LIGMA_PATTERN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PATTERN, LigmaPattern))
#define LIGMA_PATTERN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PATTERN, LigmaPatternClass))
#define LIGMA_IS_PATTERN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PATTERN))
#define LIGMA_IS_PATTERN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PATTERN))
#define LIGMA_PATTERN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PATTERN, LigmaPatternClass))


typedef struct _LigmaPatternClass LigmaPatternClass;

struct _LigmaPattern
{
  LigmaData     parent_instance;

  LigmaTempBuf *mask;
};

struct _LigmaPatternClass
{
  LigmaDataClass  parent_class;
};


GType         ligma_pattern_get_type      (void) G_GNUC_CONST;

LigmaData    * ligma_pattern_new           (LigmaContext *context,
                                          const gchar *name);
LigmaData    * ligma_pattern_get_standard  (LigmaContext *context);

LigmaTempBuf * ligma_pattern_get_mask      (LigmaPattern *pattern);
GeglBuffer  * ligma_pattern_create_buffer (LigmaPattern *pattern);


#endif /* __LIGMA_PATTERN_H__ */
