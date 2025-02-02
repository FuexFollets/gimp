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

#ifndef __LIGMA_HELP_DOMAIN_H__
#define __LIGMA_HELP_DOMAIN_H__


struct _LigmaHelpDomain
{
  gchar      *help_domain;
  gchar      *help_uri;
  GHashTable *help_locales;
};


LigmaHelpDomain * ligma_help_domain_new           (const gchar       *domain_name,
                                                 const gchar       *domain_uri);
void             ligma_help_domain_free          (LigmaHelpDomain    *domain);

LigmaHelpLocale * ligma_help_domain_lookup_locale (LigmaHelpDomain    *domain,
                                                 const gchar       *locale_id,
                                                 LigmaHelpProgress  *progress);
gchar          * ligma_help_domain_map           (LigmaHelpDomain    *domain,
                                                 GList             *help_locales,
                                                 const gchar       *help_id,
                                                 LigmaHelpProgress  *progress,
                                                 LigmaHelpLocale   **locale,
                                                 gboolean          *fatal_error);
void             ligma_help_domain_exit          (void);


#endif /* ! __LIGMA_HELP_DOMAIN_H__ */
