/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaparasiteio.h
 * Copyright (C) 1999 Tor Lillqvist <tml@iki.fi>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __LIGMA_PARASITE_IO_H__
#define __LIGMA_PARASITE_IO_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


/*  Data structures for various standard parasites used by plug-ins and
 *  the LIGMA core, and functions to build and parse their string
 *  representations.
 */

/*
 *  Pixmap brush pipes.
 */

#define LIGMA_PIXPIPE_MAXDIM 4

typedef struct _LigmaPixPipeParams LigmaPixPipeParams;

/**
 * LigmaPixPipeParams:
 * @step:       Step
 * @ncells:     Number of cells
 * @dim:        Dimension
 * @cols:       Columns
 * @rows:       Rows
 * @cellwidth:  Cell width
 * @cellheight: Cell height
 * @placement:  Placement
 * @rank:       Rank
 * @selection:  Selection
 *
 * PLease somebody help documenting this.
 **/
struct _LigmaPixPipeParams
{
  gint   step;
  gint   ncells;
  gint   dim;
  gint   cols;
  gint   rows;
  gint   cellwidth;
  gint   cellheight;
  gchar *placement;
  gint   rank[LIGMA_PIXPIPE_MAXDIM];
  gchar *selection[LIGMA_PIXPIPE_MAXDIM];
};

/* Initialize with dummy values */
void    ligma_pixpipe_params_init  (LigmaPixPipeParams *params);

/* Parse a string into a LigmaPixPipeParams */
void    ligma_pixpipe_params_parse (const gchar       *parameters,
                                   LigmaPixPipeParams *params);

/* Build a string representation of LigmaPixPipeParams */
gchar * ligma_pixpipe_params_build (LigmaPixPipeParams *params) G_GNUC_MALLOC;

/* Free the internal values. It does not free the struct itself. */
void    ligma_pixpipe_params_free  (LigmaPixPipeParams *params);

G_END_DECLS

#endif /* __LIGMA_PARASITE_IO_H__ */
