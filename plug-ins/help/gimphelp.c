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

/*  This code is written so that it can also be compiled standalone.
 *  It shouldn't depend on libligma.
 */

#include "config.h"

#include <string.h>

#include "libligma/ligma.h"

#include "ligmahelp.h"

#ifdef DISABLE_NLS
#define _(String)  (String)
#else
#include "libligma/stdplugins-intl.h"
#endif


/*  private variables  */

static GHashTable  *domain_hash = NULL;


/*  public functions  */

gboolean
ligma_help_init (const gchar **domain_names,
                const gchar **domain_uris)
{
  gint i;
  guint num_domain_names, num_domain_uris;

  num_domain_names = domain_names? g_strv_length ((gchar **) domain_names) : 0;
  num_domain_uris = domain_uris? g_strv_length ((gchar **) domain_uris) : 0;
  if (num_domain_names != num_domain_uris)
    {
      g_printerr ("help: number of names doesn't match number of URIs.\n");

      return FALSE;
    }

  for (i = 0; i < num_domain_names; i++)
    ligma_help_register_domain (domain_names[i], domain_uris[i]);

  return TRUE;
}

void
ligma_help_exit (void)
{
  if (domain_hash)
    {
      g_hash_table_destroy (domain_hash);
      domain_hash = NULL;
    }
}

void
ligma_help_register_domain (const gchar *domain_name,
                           const gchar *domain_uri)
{
  g_return_if_fail (domain_name != NULL);
  g_return_if_fail (domain_uri != NULL);

#ifdef LIGMA_HELP_DEBUG
  g_printerr ("help: registering help domain \"%s\" with base uri \"%s\"\n",
              domain_name, domain_uri);
#endif

  if (! domain_hash)
    domain_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free,
                                         (GDestroyNotify) ligma_help_domain_free);

  g_hash_table_insert (domain_hash,
                       g_strdup (domain_name),
                       ligma_help_domain_new (domain_name, domain_uri));
}

LigmaHelpDomain *
ligma_help_lookup_domain (const gchar *domain_name)
{
  g_return_val_if_fail (domain_name, NULL);

  if (domain_hash)
    return g_hash_table_lookup (domain_hash, domain_name);

  return NULL;
}

GList *
ligma_help_parse_locales (const gchar *help_locales)
{
  GList       *locales = NULL;
  GList       *list;
  const gchar *s;
  const gchar *p;

  g_return_val_if_fail (help_locales != NULL, NULL);

  /*  split the string at colons, building a list  */
  s = help_locales;
  for (p = strchr (s, ':'); p; p = strchr (s, ':'))
    {
      gchar *new = g_strndup (s, p - s);
      locales = g_list_append (locales, new);
      s = p + 1;
    }

  if (*s)
    locales = g_list_append (locales, g_strdup (s));

  /*  if the list doesn't contain the default locale yet, append it  */
  for (list = locales; list; list = list->next)
    if (strcmp ((const gchar *) list->data, LIGMA_HELP_DEFAULT_LOCALE) == 0)
      break;

  if (! list)
    locales = g_list_append (locales, g_strdup (LIGMA_HELP_DEFAULT_LOCALE));

#ifdef LIGMA_HELP_DEBUG
  g_printerr ("help: locales: ");
  for (list = locales; list; list = list->next)
    g_printerr ("%s ", (const gchar *) list->data);
  g_printerr ("\n");
#endif

  return locales;
}
