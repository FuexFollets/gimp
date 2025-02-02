/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * ligmaconfig-path.c
 * Copyright (C) 2001  Sven Neumann <sven@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <gio/gio.h>

#include "libligmabase/ligmabase.h"

#include "ligmaconfig-error.h"
#include "ligmaconfig-path.h"

#include "libligma/libligma-intl.h"


/**
 * SECTION: ligmaconfig-path
 * @title: LigmaConfig-path
 * @short_description: File path utilities for libligmaconfig.
 *
 * File path utilities for libligmaconfig.
 **/


/**
 * ligma_config_path_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a LigmaConfigPath string property
 *
 * Since: 2.4
 **/
GType
ligma_config_path_get_type (void)
{
  static GType path_type = 0;

  if (! path_type)
    {
      const GTypeInfo type_info = { 0, };

      path_type = g_type_register_static (G_TYPE_STRING, "LigmaConfigPath",
                                          &type_info, 0);
    }

  return path_type;
}


/*
 * LIGMA_TYPE_PARAM_CONFIG_PATH
 */

#define LIGMA_PARAM_SPEC_CONFIG_PATH(pspec) (G_TYPE_CHECK_INSTANCE_CAST ((pspec), LIGMA_TYPE_PARAM_CONFIG_PATH, LigmaParamSpecConfigPath))

typedef struct _LigmaParamSpecConfigPath LigmaParamSpecConfigPath;

struct _LigmaParamSpecConfigPath
{
  GParamSpecString    parent_instance;

  LigmaConfigPathType  type;
};

static void  ligma_param_config_path_class_init (GParamSpecClass *class);

/**
 * ligma_param_config_path_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a directory path object
 *
 * Since: 2.4
 **/
GType
ligma_param_config_path_get_type (void)
{
  static GType spec_type = 0;

  if (! spec_type)
    {
      const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_config_path_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecConfigPath),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_STRING,
                                          "LigmaParamConfigPath",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
ligma_param_config_path_class_init (GParamSpecClass *class)
{
  class->value_type = LIGMA_TYPE_CONFIG_PATH;
}

/**
 * ligma_param_spec_config_path:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @type:          a #LigmaConfigPathType value.
 * @default_value: Value to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a filename, dir name,
 * or list of file or dir names.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 2.4
 **/
GParamSpec *
ligma_param_spec_config_path (const gchar        *name,
                             const gchar        *nick,
                             const gchar        *blurb,
                             LigmaConfigPathType  type,
                             const gchar        *default_value,
                             GParamFlags         flags)
{
  GParamSpecString *pspec;

  pspec = g_param_spec_internal (LIGMA_TYPE_PARAM_CONFIG_PATH,
                                 name, nick, blurb, flags);

  pspec->default_value = g_strdup (default_value);

  LIGMA_PARAM_SPEC_CONFIG_PATH (pspec)->type = type;

  return G_PARAM_SPEC (pspec);
}

/**
 * ligma_param_spec_config_path_type:
 * @pspec:         A #GParamSpec for a path param
 *
 * Tells whether the path param encodes a filename,
 * dir name, or list of file or dir names.
 *
 * Returns: a #LigmaConfigPathType value
 *
 * Since: 2.4
 **/
LigmaConfigPathType
ligma_param_spec_config_path_type (GParamSpec *pspec)
{
  g_return_val_if_fail (LIGMA_IS_PARAM_SPEC_CONFIG_PATH (pspec), 0);

  return LIGMA_PARAM_SPEC_CONFIG_PATH (pspec)->type;
}


/*
 * LigmaConfig path utilities
 */

static gchar        * ligma_config_path_expand_only   (const gchar  *path,
                                                      GError      **error) G_GNUC_MALLOC;
static inline gchar * ligma_config_path_extract_token (const gchar **str);
static gchar        * ligma_config_path_unexpand_only (const gchar  *path) G_GNUC_MALLOC;


/**
 * ligma_config_build_data_path:
 * @name: directory name (in UTF-8 encoding)
 *
 * Creates a search path as it is used in the ligmarc file.  The path
 * returned by ligma_config_build_data_path() includes a directory
 * below the user's ligma directory and one in the system-wide data
 * directory.
 *
 * Note that you cannot use this path directly with ligma_path_parse().
 * As it is in the ligmarc notation, you first need to expand and
 * recode it using ligma_config_path_expand().
 *
 * Returns: a newly allocated string
 *
 * Since: 2.4
 **/
gchar *
ligma_config_build_data_path (const gchar *name)
{
  return g_strconcat ("${ligma_dir}", G_DIR_SEPARATOR_S, name,
                      G_SEARCHPATH_SEPARATOR_S,
                      "${ligma_data_dir}", G_DIR_SEPARATOR_S, name,
                      NULL);
}

/**
 * ligma_config_build_plug_in_path:
 * @name: directory name (in UTF-8 encoding)
 *
 * Creates a search path as it is used in the ligmarc file.  The path
 * returned by ligma_config_build_plug_in_path() includes a directory
 * below the user's ligma directory and one in the system-wide plug-in
 * directory.
 *
 * Note that you cannot use this path directly with ligma_path_parse().
 * As it is in the ligmarc notation, you first need to expand and
 * recode it using ligma_config_path_expand().
 *
 * Returns: a newly allocated string
 *
 * Since: 2.4
 **/
gchar *
ligma_config_build_plug_in_path (const gchar *name)
{
  return g_strconcat ("${ligma_dir}", G_DIR_SEPARATOR_S, name,
                      G_SEARCHPATH_SEPARATOR_S,
                      "${ligma_plug_in_dir}", G_DIR_SEPARATOR_S, name,
                      NULL);
}

/**
 * ligma_config_build_writable_path:
 * @name: directory name (in UTF-8 encoding)
 *
 * Creates a search path as it is used in the ligmarc file.  The path
 * returned by ligma_config_build_writable_path() is just the writable
 * parts of the search path constructed by ligma_config_build_data_path().
 *
 * Note that you cannot use this path directly with ligma_path_parse().
 * As it is in the ligmarc notation, you first need to expand and
 * recode it using ligma_config_path_expand().
 *
 * Returns: a newly allocated string
 *
 * Since: 2.4
 **/
gchar *
ligma_config_build_writable_path (const gchar *name)
{
  return g_strconcat ("${ligma_dir}", G_DIR_SEPARATOR_S, name, NULL);
}

/**
 * ligma_config_build_system_path:
 * @name: directory name (in UTF-8 encoding)
 *
 * Creates a search path as it is used in the ligmarc file.  The path
 * returned by ligma_config_build_system_path() is just the read-only
 * parts of the search path constructed by ligma_config_build_plug_in_path().
 *
 * Note that you cannot use this path directly with ligma_path_parse().
 * As it is in the ligmarc notation, you first need to expand and
 * recode it using ligma_config_path_expand().
 *
 * Returns: a newly allocated string
 *
 * Since: 2.10.6
 **/
gchar *
ligma_config_build_system_path (const gchar *name)
{
  return g_strconcat ("${ligma_plug_in_dir}", G_DIR_SEPARATOR_S, name, NULL);
}

/**
 * ligma_config_path_expand:
 * @path:   a NUL-terminated string in UTF-8 encoding
 * @recode: whether to convert to the filesystem's encoding
 * @error:  return location for errors
 *
 * Paths as stored in ligmarc and other config files have to be treated
 * special.  The string may contain special identifiers such as for
 * example ${ligma_dir} that have to be substituted before use. Also
 * the user's filesystem may be in a different encoding than UTF-8
 * (which is what is used for the ligmarc). This function does the
 * variable substitution for you and can also attempt to convert to
 * the filesystem encoding.
 *
 * To reverse the expansion, use ligma_config_path_unexpand().
 *
 * Returns: a newly allocated NUL-terminated string
 *
 * Since: 2.4
 **/
gchar *
ligma_config_path_expand (const gchar  *path,
                         gboolean      recode,
                         GError      **error)
{
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (recode)
    {
      gchar *retval;
      gchar *expanded = ligma_config_path_expand_only (path, error);

      if (! expanded)
        return NULL;

      retval = g_filename_from_utf8 (expanded, -1, NULL, NULL, error);

      g_free (expanded);

      return retval;
    }

  return ligma_config_path_expand_only (path, error);
}

/**
 * ligma_config_path_expand_to_files:
 * @path:  a NUL-terminated string in UTF-8 encoding
 * @error: return location for errors
 *
 * Paths as stored in the ligmarc have to be treated special. The
 * string may contain special identifiers such as for example
 * ${ligma_dir} that have to be substituted before use. Also the user's
 * filesystem may be in a different encoding than UTF-8 (which is what
 * is used for the ligmarc).
 *
 * This function runs @path through ligma_config_path_expand() and
 * ligma_path_parse(), then turns the filenames returned by
 * ligma_path_parse() into GFile using g_file_new_for_path().
 *
 * Returns: (element-type GFile) (transfer full):
                 a #GList of newly allocated #GFile objects.
 *
 * Since: 2.10
 **/
GList *
ligma_config_path_expand_to_files (const gchar  *path,
                                  GError      **error)
{
  GList *files;
  GList *list;
  gchar *expanded;

  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  expanded = ligma_config_path_expand (path, TRUE, error);

  if (! expanded)
    return NULL;

  files = ligma_path_parse (expanded, 256, FALSE, NULL);

  g_free (expanded);

  for (list = files; list; list = g_list_next (list))
    {
      gchar *dir = list->data;

      list->data = g_file_new_for_path (dir);
      g_free (dir);
    }

  return files;
}

/**
 * ligma_config_path_unexpand:
 * @path:   a NUL-terminated string
 * @recode: whether @path is in filesystem encoding or UTF-8
 * @error:  return location for errors
 *
 * The inverse operation of ligma_config_path_expand()
 *
 * This function takes a @path and tries to substitute the first
 * elements by well-known special identifiers such as for example
 * ${ligma_dir}. The unexpanded path can then be stored in ligmarc and
 * other config files.
 *
 * If @recode is %TRUE then @path is in local filesystem encoding,
 * if @recode is %FALSE then @path is assumed to be UTF-8.
 *
 * Returns: a newly allocated NUL-terminated UTF-8 string
 *
 * Since: 2.10
 **/
gchar *
ligma_config_path_unexpand (const gchar  *path,
                           gboolean      recode,
                           GError      **error)
{
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  if (recode)
    {
      gchar *retval;
      gchar *utf8 = g_filename_to_utf8 (path, -1, NULL, NULL, error);

      if (! utf8)
        return NULL;

      retval = ligma_config_path_unexpand_only (utf8);

      g_free (utf8);

      return retval;
    }

  return ligma_config_path_unexpand_only (path);
}

/**
 * ligma_file_new_for_config_path:
 * @path:   a NUL-terminated string in UTF-8 encoding
 * @error:  return location for errors
 *
 * Expands @path using ligma_config_path_expand() and returns a #GFile
 * for the expanded path.
 *
 * To reverse the expansion, use ligma_file_get_config_path().
 *
 * Returns: (nullable) (transfer full): a newly allocated #GFile,
 *          or %NULL if the expansion failed.
 *
 * Since: 2.10
 **/
GFile *
ligma_file_new_for_config_path (const gchar  *path,
                               GError      **error)
{
  GFile *file = NULL;
  gchar *expanded;

  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  expanded = ligma_config_path_expand (path, TRUE, error);

  if (expanded)
    {
      file = g_file_new_for_path (expanded);
      g_free (expanded);
    }

  return file;
}

/**
 * ligma_file_get_config_path:
 * @file:   a #GFile
 * @error:  return location for errors
 *
 * Unexpands @file's path using ligma_config_path_unexpand() and
 * returns the unexpanded path.
 *
 * The inverse operation of ligma_file_new_for_config_path().
 *
 * Returns: a newly allocated NUL-terminated UTF-8 string, or %NULL if
 *               unexpanding failed.
 *
 * Since: 2.10
 **/
gchar *
ligma_file_get_config_path (GFile   *file,
                           GError **error)
{
  gchar *unexpanded = NULL;
  gchar *path;

  g_return_val_if_fail (G_IS_FILE (file), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);

  path = g_file_get_path (file);

  if (path)
    {
      unexpanded = ligma_config_path_unexpand (path, TRUE, error);
      g_free (path);
    }
  else
    {
      g_set_error_literal (error, 0, 0,
                           _("File has no path representation"));
    }

  return unexpanded;
}


/*  private functions  */

#define SUBSTS_ALLOC 4

static gchar *
ligma_config_path_expand_only (const gchar  *path,
                              GError      **error)
{
  const gchar *home;
  const gchar *p;
  const gchar *s;
  gchar       *n;
  gchar       *token;
  gchar       *filename = NULL;
  gchar       *expanded = NULL;
  gchar      **substs   = NULL;
  guint        n_substs = 0;
  gint         length   = 0;
  guint        i;

  home = g_get_home_dir ();
  if (home)
    home = ligma_filename_to_utf8 (home);

  p = path;

  while (*p)
    {
      if (*p == '~' && home)
        {
          length += strlen (home);
          p += 1;
        }
      else if ((token = ligma_config_path_extract_token (&p)) != NULL)
        {
          for (i = 0; i < n_substs; i++)
            if (strcmp (substs[2*i], token) == 0)
              break;

          if (i < n_substs)
            {
              s = substs[2*i+1];
            }
          else
            {
              s = NULL;

              if (strcmp (token, "ligma_dir") == 0)
                s = ligma_directory ();
              else if (strcmp (token, "ligma_data_dir") == 0)
                s = ligma_data_directory ();
              else if (strcmp (token, "ligma_plug_in_dir") == 0 ||
                       strcmp (token, "ligma_plugin_dir") == 0)
                s = ligma_plug_in_directory ();
              else if (strcmp (token, "ligma_sysconf_dir") == 0)
                s = ligma_sysconf_directory ();
              else if (strcmp (token, "ligma_installation_dir") == 0)
                s = ligma_installation_directory ();
              else if (strcmp (token, "ligma_cache_dir") == 0)
                s = ligma_cache_directory ();
              else if (strcmp (token, "ligma_temp_dir") == 0)
                s = ligma_temp_directory ();

              if (!s)
                s = g_getenv (token);

#ifdef G_OS_WIN32
              /* The default user ligmarc on Windows references
               * ${TEMP}, but not all Windows installations have that
               * environment variable, even if it should be kinda
               * standard. So special-case it.
               */
              if (!s && strcmp (token, "TEMP") == 0)
                s = g_get_tmp_dir ();
#endif  /* G_OS_WIN32 */
            }

          if (!s)
            {
              g_set_error (error, LIGMA_CONFIG_ERROR, LIGMA_CONFIG_ERROR_PARSE,
                           _("Cannot expand ${%s}"), token);
              g_free (token);
              goto cleanup;
            }

          if (n_substs % SUBSTS_ALLOC == 0)
            substs = g_renew (gchar *, substs, 2 * (n_substs + SUBSTS_ALLOC));

          substs[2*n_substs]     = token;
          substs[2*n_substs + 1] = (gchar *) ligma_filename_to_utf8 (s);

          length += strlen (substs[2*n_substs + 1]);

          n_substs++;
        }
      else
        {
          length += g_utf8_skip[(const guchar) *p];
          p = g_utf8_next_char (p);
        }
    }

  if (n_substs == 0)
    return g_strdup (path);

  expanded = g_new (gchar, length + 1);

  p = path;
  n = expanded;

  while (*p)
    {
      if (*p == '~' && home)
        {
          *n = '\0';
          strcat (n, home);
          n += strlen (home);
          p += 1;
        }
      else if ((token = ligma_config_path_extract_token (&p)) != NULL)
        {
          for (i = 0; i < n_substs; i++)
            {
              if (strcmp (substs[2*i], token) == 0)
                {
                  s = substs[2*i+1];

                  *n = '\0';
                  strcat (n, s);
                  n += strlen (s);

                  break;
                }
            }

          g_free (token);
        }
      else
        {
          *n++ = *p++;
        }
    }

  *n = '\0';

 cleanup:
  for (i = 0; i < n_substs; i++)
    g_free (substs[2*i]);

  g_free (substs);
  g_free (filename);

  return expanded;
}

static inline gchar *
ligma_config_path_extract_token (const gchar **str)
{
  const gchar *p;
  gchar       *token;

  if (strncmp (*str, "${", 2))
    return NULL;

  p = *str + 2;

  while (*p && (*p != '}'))
    p = g_utf8_next_char (p);

  if (! *p)
    return NULL;

  token = g_strndup (*str + 2, g_utf8_pointer_to_offset (*str + 2, p));

  *str = p + 1; /* after the closing bracket */

  return token;
}

static gchar *
ligma_config_path_unexpand_only (const gchar *path)
{
  const struct
  {
    const gchar *id;
    const gchar *prefix;
  }
  identifiers[] =
  {
    { "${ligma_plug_in_dir}",      ligma_plug_in_directory () },
    { "${ligma_data_dir}",         ligma_data_directory () },
    { "${ligma_sysconf_dir}",      ligma_sysconf_directory () },
    { "${ligma_installation_dir}", ligma_installation_directory () },
    { "${ligma_cache_dir}",        ligma_cache_directory () },
    { "${ligma_temp_dir}",         ligma_temp_directory () },
    { "${ligma_dir}",              ligma_directory () }
  };

  GList *files;
  GList *list;
  gchar *unexpanded;

  files = ligma_path_parse (path, 256, FALSE, NULL);

  for (list = files; list; list = g_list_next (list))
    {
      gchar *dir = list->data;
      gint   i;

      for (i = 0; i < G_N_ELEMENTS (identifiers); i++)
        {
          if (g_str_has_prefix (dir, identifiers[i].prefix))
            {
              gchar *tmp = g_strconcat (identifiers[i].id,
                                        dir + strlen (identifiers[i].prefix),
                                        NULL);

              g_free (dir);
              list->data = tmp;

              break;
            }
        }
    }

  unexpanded = ligma_path_to_str (files);

  ligma_path_free (files);

  return unexpanded;
}
