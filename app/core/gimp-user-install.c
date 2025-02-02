/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligma-user-install.c
 * Copyright (C) 2000-2008 Michael Natterer and Sven Neumann
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

/* This file contains functions to help migrate the settings from a
 * previous LIGMA version to be used with the current (newer) version.
 */

#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef PLATFORM_OSX
#include <AppKit/AppKit.h>
#endif

#include <gio/gio.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#include <libligmabase/ligmawin32-io.h>
#endif

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "config/ligmaconfig-file.h"
#include "config/ligmarc.h"

#include "ligma-templates.h"
#include "ligma-tags.h"
#include "ligma-user-install.h"

#include "ligma-intl.h"


struct _LigmaUserInstall
{
  GObject                 *ligma;

  gboolean                verbose;

  gchar                  *old_dir;
  gint                    old_major;
  gint                    old_minor;

  gint                    scale_factor;

  const gchar            *migrate;

  LigmaUserInstallLogFunc  log;
  gpointer                log_data;
};

typedef enum
{
  USER_INSTALL_MKDIR, /* Create the directory        */
  USER_INSTALL_COPY   /* Copy from sysconf directory */
} LigmaUserInstallAction;

static const struct
{
  const gchar           *name;
  LigmaUserInstallAction  action;
}
ligma_user_install_items[] =
{
  { "menurc",          USER_INSTALL_COPY  },
  { "brushes",         USER_INSTALL_MKDIR },
  { "dynamics",        USER_INSTALL_MKDIR },
  { "fonts",           USER_INSTALL_MKDIR },
  { "gradients",       USER_INSTALL_MKDIR },
  { "palettes",        USER_INSTALL_MKDIR },
  { "patterns",        USER_INSTALL_MKDIR },
  { "tool-presets",    USER_INSTALL_MKDIR },
  { "plug-ins",        USER_INSTALL_MKDIR },
  { "modules",         USER_INSTALL_MKDIR },
  { "interpreters",    USER_INSTALL_MKDIR },
  { "environ",         USER_INSTALL_MKDIR },
  { "scripts",         USER_INSTALL_MKDIR },
  { "templates",       USER_INSTALL_MKDIR },
  { "themes",          USER_INSTALL_MKDIR },
  { "icons",           USER_INSTALL_MKDIR },
  { "tmp",             USER_INSTALL_MKDIR },
  { "curves",          USER_INSTALL_MKDIR },
  { "levels",          USER_INSTALL_MKDIR },
  { "filters",         USER_INSTALL_MKDIR },
  { "fractalexplorer", USER_INSTALL_MKDIR },
  { "gfig",            USER_INSTALL_MKDIR },
  { "gflare",          USER_INSTALL_MKDIR },
  { "ligmaressionist",  USER_INSTALL_MKDIR }
};


static gboolean  user_install_detect_old         (LigmaUserInstall    *install,
                                                  const gchar        *ligma_dir);
static gchar *   user_install_old_style_ligmadir  (void);

static void      user_install_log                (LigmaUserInstall    *install,
                                                  const gchar        *format,
                                                  ...) G_GNUC_PRINTF (2, 3);
static void      user_install_log_newline        (LigmaUserInstall    *install);
static void      user_install_log_error          (LigmaUserInstall    *install,
                                                  GError            **error);

static gboolean  user_install_mkdir              (LigmaUserInstall    *install,
                                                  const gchar        *dirname);
static gboolean  user_install_mkdir_with_parents (LigmaUserInstall    *install,
                                                  const gchar        *dirname);
static gboolean  user_install_file_copy          (LigmaUserInstall    *install,
                                                  const gchar        *source,
                                                  const gchar        *dest,
                                                  const gchar        *old_options_regexp,
                                                  GRegexEvalCallback  update_callback);
static gboolean  user_install_dir_copy           (LigmaUserInstall    *install,
                                                  gint                level,
                                                  const gchar        *source,
                                                  const gchar        *base,
                                                  const gchar        *update_pattern,
                                                  GRegexEvalCallback  update_callback);

static gboolean  user_install_create_files       (LigmaUserInstall    *install);
static gboolean  user_install_migrate_files      (LigmaUserInstall    *install);


/*  public functions  */

LigmaUserInstall *
ligma_user_install_new (GObject  *ligma,
                       gboolean  verbose)
{
  LigmaUserInstall *install = g_slice_new0 (LigmaUserInstall);

  install->ligma    = ligma;
  install->verbose = verbose;

  user_install_detect_old (install, ligma_directory ());

#ifdef PLATFORM_OSX
  /* The config path on OSX has for a very short time frame (2.8.2 only)
     been "~/Library/LIGMA". It changed to "~/Library/Application Support"
     in 2.8.4 and was in the home folder (as was other UNIX) before. */

  if (! install->old_dir)
    {
      gchar             *dir;
      NSAutoreleasePool *pool;
      NSArray           *path;
      NSString          *library_dir;

      pool = [[NSAutoreleasePool alloc] init];

      path = NSSearchPathForDirectoriesInDomains (NSLibraryDirectory,
                                                  NSUserDomainMask, YES);
      library_dir = [path objectAtIndex:0];

      dir = g_build_filename ([library_dir UTF8String],
                              LIGMADIR, LIGMA_USER_VERSION, NULL);

      [pool drain];

      user_install_detect_old (install, dir);
      g_free (dir);
    }

#endif

  if (! install->old_dir)
    {
      /* if the default XDG-style config directory was not found, try
       * the "old-style" path in the home folder.
       */
      gchar *dir = user_install_old_style_ligmadir ();
      user_install_detect_old (install, dir);
      g_free (dir);
    }

  return install;
}

gboolean
ligma_user_install_run (LigmaUserInstall *install,
                       gint             scale_factor)
{
  gchar *dirname;

  g_return_val_if_fail (install != NULL, FALSE);

  install->scale_factor = scale_factor;
  dirname = g_filename_display_name (ligma_directory ());

  if (install->migrate)
    user_install_log (install,
                      _("It seems you have used LIGMA %s before.  "
                        "LIGMA will now migrate your user settings to '%s'."),
                      install->migrate, dirname);
  else
    user_install_log (install,
                      _("It appears that you are using LIGMA for the "
                        "first time.  LIGMA will now create a folder "
                        "named '%s' and copy some files to it."),
                      dirname);

  g_free (dirname);

  user_install_log_newline (install);

  if (! user_install_mkdir_with_parents (install, ligma_directory ()))
    return FALSE;

  if (install->migrate)
    if (! user_install_migrate_files (install))
      return FALSE;

  return user_install_create_files (install);
}

void
ligma_user_install_free (LigmaUserInstall *install)
{
  g_return_if_fail (install != NULL);

  g_free (install->old_dir);

  g_slice_free (LigmaUserInstall, install);
}

void
ligma_user_install_set_log_handler (LigmaUserInstall        *install,
                                   LigmaUserInstallLogFunc  log,
                                   gpointer                user_data)
{
  g_return_if_fail (install != NULL);

  install->log      = log;
  install->log_data = user_data;
}


/*  Local functions  */

static gboolean
user_install_detect_old (LigmaUserInstall *install,
                         const gchar     *ligma_dir)
{
  gchar    *dir     = g_strdup (ligma_dir);
  gchar    *version;
  gboolean  migrate = FALSE;

  version = strstr (dir, LIGMA_APP_VERSION);
  g_snprintf (version, 5, "%d.XY", 2);

  if (version)
    {
      gint i;

      for (i = (LIGMA_MINOR_VERSION & ~1); i >= 0; i -= 2)
        {
          /*  we assume that LIGMA_APP_VERSION is in the form '2.x'  */
          g_snprintf (version + 2, 3, "%d", i);

          migrate = g_file_test (dir, G_FILE_TEST_IS_DIR);

          if (migrate)
            {
#ifdef LIGMA_UNSTABLE
              g_printerr ("ligma-user-install: migrating from %s\n", dir);
#endif
              install->old_major = 2;
              install->old_minor = i;

              break;
            }
        }
    }

  if (migrate)
    {
      install->old_dir = dir;
      install->migrate = (const gchar *) version;
    }
  else
    {
      g_free (dir);
    }

  return migrate;
}

static gchar *
user_install_old_style_ligmadir (void)
{
  const gchar *home_dir = g_get_home_dir ();
  gchar       *ligma_dir = NULL;

  if (home_dir)
    {
      ligma_dir = g_build_filename (home_dir, ".ligma-" LIGMA_APP_VERSION, NULL);
    }
  else
    {
      gchar *user_name = g_strdup (g_get_user_name ());
      gchar *subdir_name;

#ifdef G_OS_WIN32
      gchar *p = user_name;

      while (*p)
        {
          /* Replace funny characters in the user name with an
           * underscore. The code below also replaces some
           * characters that in fact are legal in file names, but
           * who cares, as long as the definitely illegal ones are
           * caught.
           */
          if (!g_ascii_isalnum (*p) && !strchr ("-.,@=", *p))
            *p = '_';
          p++;
        }
#endif

#ifndef G_OS_WIN32
      g_message ("warning: no home directory.");
#endif
      subdir_name = g_strconcat (".ligma-" LIGMA_APP_VERSION ".", user_name, NULL);
      ligma_dir = g_build_filename (ligma_data_directory (),
                                   subdir_name,
                                   NULL);
      g_free (user_name);
      g_free (subdir_name);
    }

  return ligma_dir;
}

static void
user_install_log (LigmaUserInstall *install,
                  const gchar     *format,
                  ...)
{
  va_list args;

  va_start (args, format);

  if (format)
    {
      gchar *message = g_strdup_vprintf (format, args);

      if (install->verbose)
        g_print ("%s\n", message);

      if (install->log)
        install->log (message, FALSE, install->log_data);

      g_free (message);
    }

  va_end (args);
}

static void
user_install_log_newline (LigmaUserInstall *install)
{
  if (install->verbose)
    g_print ("\n");

  if (install->log)
    install->log (NULL, FALSE, install->log_data);
}

static void
user_install_log_error (LigmaUserInstall  *install,
                        GError          **error)
{
  if (error && *error)
    {
      const gchar *message = ((*error)->message ?
                              (*error)->message : "(unknown error)");

      if (install->log)
        install->log (message, TRUE, install->log_data);
      else
        g_print ("error: %s\n", message);

      g_clear_error (error);
    }
}

static gboolean
user_install_file_copy (LigmaUserInstall    *install,
                        const gchar        *source,
                        const gchar        *dest,
                        const gchar        *old_options_regexp,
                        GRegexEvalCallback  update_callback)
{
  GError   *error = NULL;
  gboolean  success;

  user_install_log (install, _("Copying file '%s' from '%s'..."),
                    ligma_filename_to_utf8 (dest),
                    ligma_filename_to_utf8 (source));

  success = ligma_config_file_copy (source, dest, old_options_regexp, update_callback, install, &error);

  user_install_log_error (install, &error);

  return success;
}

static gboolean
user_install_mkdir (LigmaUserInstall *install,
                    const gchar     *dirname)
{
  user_install_log (install, _("Creating folder '%s'..."),
                    ligma_filename_to_utf8 (dirname));

  if (g_mkdir (dirname,
               S_IRUSR | S_IWUSR | S_IXUSR |
               S_IRGRP | S_IXGRP |
               S_IROTH | S_IXOTH) == -1)
    {
      GError *error = NULL;

      g_set_error (&error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Cannot create folder '%s': %s"),
                   ligma_filename_to_utf8 (dirname), g_strerror (errno));

      user_install_log_error (install, &error);

      return FALSE;
    }

  return TRUE;
}

static gboolean
user_install_mkdir_with_parents (LigmaUserInstall *install,
                                 const gchar     *dirname)
{
  user_install_log (install, _("Creating folder '%s'..."),
                    ligma_filename_to_utf8 (dirname));

  if (g_mkdir_with_parents (dirname,
                            S_IRUSR | S_IWUSR | S_IXUSR |
                            S_IRGRP | S_IXGRP |
                            S_IROTH | S_IXOTH) == -1)
    {
      GError *error = NULL;

      g_set_error (&error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Cannot create folder '%s': %s"),
                   ligma_filename_to_utf8 (dirname), g_strerror (errno));

      user_install_log_error (install, &error);

      return FALSE;
    }

  return TRUE;
}

/* The regexp pattern of all options changed from menurc of LIGMA 2.8.
 * Add any pattern that we want to recognize for replacement in the menurc of
 * the next release
 */
#define MENURC_OVER20_UPDATE_PATTERN \
  "\"<Actions>/buffers/buffers-paste-as-new\""  "|" \
  "\"<Actions>/edit/edit-paste-as-new\""        "|" \
  "\"<Actions>/file/file-export\""              "|" \
  "\"<Actions>/file/file-export-to\""           "|" \
  "\"<Actions>/layers/layers-text-tool\""       "|" \
  "\"<Actions>/plug-in/plug-in-gauss\""         "|" \
  "\"<Actions>/tools/tools-value-[1-4]-.*\""    "|" \
  "\"<Actions>/vectors/vectors-path-tool\""     "|" \
  "\"<Actions>/tools/tools-blend\""             "|" \
  "\"<Actions>/view/view-rotate-reset\""

/**
 * callback to use for updating a menurc from LIGMA over 2.0.
 * data is unused (always NULL).
 * The updated value will be matched line by line.
 */
static gboolean
user_update_menurc_over20 (const GMatchInfo *matched_value,
                           GString          *new_value,
                           gpointer          data)
{
  LigmaUserInstall *install = (LigmaUserInstall *) data;
  gchar           *match   = g_match_info_fetch (matched_value, 0);

  /* "*-paste-as-new" renamed to "*-paste-as-new-image"
   */
  if (g_strcmp0 (match, "\"<Actions>/buffers/buffers-paste-as-new\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/buffers/buffers-paste-as-new-image\"");
    }
  else if (g_strcmp0 (match, "\"<Actions>/edit/edit-paste-as-new\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/edit/edit-paste-as-new-image\"");
    }
  /* file-export-* changes to follow file-save-* patterns.  Actions
   * available since LIGMA 2.8, changed for 2.10 in commit 4b14ed2.
   */
  else if (g_strcmp0 (match, "\"<Actions>/file/file-export\"") == 0)
    {
      if (install->old_major == 2 && install->old_minor <= 8)
        g_string_append (new_value, "\"<Actions>/file/file-export-as\"");
      else
        /* Don't change from a 2.10 config. */
        g_string_append (new_value, match);
    }
  else if (g_strcmp0 (match, "\"<Actions>/file/file-export-to\"") == 0)
    {
      if (install->old_major == 2 && install->old_minor <= 8)
        g_string_append (new_value, "\"<Actions>/file/file-export\"");
      else
        /* Don't change from a 2.10 config. */
        g_string_append (new_value, match);
    }
  else if (g_strcmp0 (match, "\"<Actions>/layers/layers-text-tool\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/layers/layers-edit\"");
    }
  /* plug-in-gauss doesn't exist anymore since commit ff59aebbe88.
   * The expected replacement would be filters-gaussian-blur which is
   * gegl:gaussian-blur operation. See also bug 775931.
   */
  else if (g_strcmp0 (match, "\"<Actions>/plug-in/plug-in-gauss\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/filters/filters-gaussian-blur\"");
    }
  /* Tools settings renamed more user-friendly.  Actions available
   * since LIGMA 2.4, changed for 2.10 in commit 0bdb747.
   */
  else if (g_str_has_prefix (match, "\"<Actions>/tools/tools-value-1-"))
    {
      g_string_append (new_value, "\"<Actions>/tools/tools-opacity-");
      g_string_append (new_value, match + 31);
    }
  else if (g_str_has_prefix (match, "\"<Actions>/tools/tools-value-2-"))
    {
      g_string_append (new_value, "\"<Actions>/tools/tools-size-");
      g_string_append (new_value, match + 31);
    }
  else if (g_str_has_prefix (match, "\"<Actions>/tools/tools-value-3-"))
    {
      g_string_append (new_value, "\"<Actions>/tools/tools-aspect-");
      g_string_append (new_value, match + 31);
    }
  else if (g_str_has_prefix (match, "\"<Actions>/tools/tools-value-4-"))
    {
      g_string_append (new_value, "\"<Actions>/tools/tools-angle-");
      g_string_append (new_value, match + 31);
    }
  else if (g_strcmp0 (match, "\"<Actions>/vectors/vectors-path-tool\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/vectors/vectors-edit\"");
    }
  else if (g_strcmp0 (match, "\"<Actions>/tools/tools-blend\"") == 0)
    {
      g_string_append (new_value, "\"<Actions>/tools/tools-gradient\"");
    }
  else if (g_strcmp0 (match, "\"<Actions>/view/view-rotate-reset\"") == 0)
    {
      /* view-rotate-reset became view-reset and new view-rotate-reset
       * and view-flip-reset actions were created.
       * See commit 15fb4a7be0.
       */
      if (install->old_major == 2)
        g_string_append (new_value, "\"<Actions>/view/view-reset\"");
      else
        /* In advance for a migration from a 3.0 config to higher ones. */
        g_string_append (new_value, match);
    }
  else
    {
      /* Should not happen. Just in case we match something unexpected by
       * mistake.
       */
      g_message ("(WARNING) %s: invalid match \"%s\"", G_STRFUNC, match);
      g_string_append (new_value, match);
    }

  g_free (match);
  return FALSE;
}

#define CONTROLLERRC_UPDATE_PATTERN \
  "\\(map \"(scroll|cursor)-[^\"]*\\bcontrol\\b[^\"]*\""

static gboolean
user_update_controllerrc (const GMatchInfo *matched_value,
                          GString          *new_value,
                          gpointer          data)
{
  gchar  *original;
  gchar  *replacement;
  GRegex *regexp = NULL;

  /* No need of a complicated pattern here.
   * CONTROLLERRC_UPDATE_PATTERN took care of it first.
   */
  regexp   = g_regex_new ("\\bcontrol\\b", 0, 0, NULL);
  original = g_match_info_fetch (matched_value, 0);

  replacement = g_regex_replace (regexp, original, -1, 0,
                                 "primary", 0, NULL);
  g_string_append (new_value, replacement);

  g_free (original);
  g_free (replacement);
  g_regex_unref (regexp);

  return FALSE;
}

#define SESSIONRC_UPDATE_PATTERN \
  "\\(position [0-9]* [0-9]*\\)"        "|"  \
  "\\(size [0-9]* [0-9]*\\)"            "|" \
  "\\(left-docks-width \"?[0-9]*\"?\\)" "|" \
  "\\(right-docks-width \"?[0-9]*\"?\\)"

static gboolean
user_update_sessionrc (const GMatchInfo *matched_value,
                       GString          *new_value,
                       gpointer          data)
{
  LigmaUserInstall *install = (LigmaUserInstall *) data;
  gchar           *original;

  original = g_match_info_fetch (matched_value, 0);

  if (install->scale_factor != 1 && install->scale_factor > 0)
    {
      /* GTK < 3.0 didn't have scale factor support. It means that any
       * size and position back then would be in real pixel size. Now
       * with GTK3 and over, we need to think of position and size in
       * virtual/application pixels.
       * In particular it means that if we were to just copy the numbers
       * from GTK2 to GTK3 on a display with scale factor of 2, every
       * width, height and position would be 2 times too big (a full
       * screen window would end up off-screen).
       */
      GRegex     *regexp;
      GMatchInfo *match_info;
      gchar      *match;

      /* First copy the pattern title. */
      regexp = g_regex_new ("\\([a-z-]* ", 0, 0, NULL);
      g_regex_match (regexp, original, 0, &match_info);
      match = g_match_info_fetch (match_info, 0);
      g_string_append (new_value, match);

      g_match_info_free (match_info);
      g_regex_unref (regexp);
      g_free (match);

      /* Now copy the numbers. */
      regexp = g_regex_new ("[0-9]+|\"", 0, 0, NULL);
      g_regex_match (regexp, original, 0, &match_info);

      while (g_match_info_matches (match_info))
        {
          match = g_match_info_fetch (match_info, 0);
          if (g_strcmp0 (match, "\"") == 0)
            {
              g_string_append (new_value, match);
            }
          else
            {
              gint num;

              num = g_ascii_strtoll (match, NULL, 10);
              num /= install->scale_factor;

              g_string_append_printf (new_value, " %d", num);
            }

          g_free (match);
          g_match_info_next (match_info, NULL);
        }

      g_match_info_free (match_info);
      g_regex_unref (regexp);

      g_string_append (new_value, ")");
    }
  else
    {
      /* Just copy as-is. */
      g_string_append (new_value, original);
    }

  g_free (original);

  return FALSE;
}

#define LIGMARC_UPDATE_PATTERN \
  "\\(theme [^)]*\\)"    "|" \
  "\\(.*-path [^)]*\\)"

static gboolean
user_update_ligmarc (const GMatchInfo *matched_value,
                    GString          *new_value,
                    gpointer          data)
{
  /* Do not migrate paths and themes from LIGMA < 2.10. */
  return FALSE;
}

#define LIGMARESSIONIST_UPDATE_PATTERN \
  "selectedbrush=Brushes/paintbrush.pgm"

static gboolean
user_update_ligmaressionist (const GMatchInfo *matched_value,
                            GString          *new_value,
                            gpointer          data)
{
  gchar *match = g_match_info_fetch (matched_value, 0);

  /* See bug 791934: both brushes are identical. */
  if (g_strcmp0 (match, "selectedbrush=Brushes/paintbrush.pgm") == 0)
    {
      g_string_append (new_value, "selectedbrush=Brushes/paintbrush01.pgm");
    }
  else
    {
      g_message ("(WARNING) %s: invalid match \"%s\"", G_STRFUNC, match);
      g_string_append (new_value, match);
    }

  g_free (match);
  return FALSE;
}

#define TOOL_PRESETS_UPDATE_PATTERN \
  "LigmaImageMapOptions"       "|" \
  "LigmaBlendOptions"          "|" \
  "ligma-blend-tool"           "|" \
  "ligma-tool-blend"           "|" \
  "dynamics \"Dynamics Off\"" "|" \
  "\\(dynamics-expanded yes\\)"

static gboolean
user_update_tool_presets (const GMatchInfo *matched_value,
                          GString          *new_value,
                          gpointer          data)
{
  gchar *match = g_match_info_fetch (matched_value, 0);

  if (g_strcmp0 (match, "LigmaImageMapOptions") == 0)
    {
      g_string_append (new_value, "LigmaFilterOptions");
    }
  else if (g_strcmp0 (match, "LigmaBlendOptions") == 0)
    {
      g_string_append (new_value, "LigmaGradientOptions");
    }
  else if (g_strcmp0 (match, "ligma-blend-tool") == 0)
    {
      g_string_append (new_value, "ligma-gradient-tool");
    }
  else if (g_strcmp0 (match, "ligma-tool-blend") == 0)
    {
      g_string_append (new_value, "ligma-tool-gradient");
    }
  else if (g_strcmp0 (match, "dynamics \"Dynamics Off\"") == 0)
    {
      g_string_append (new_value, "dynamics-enabled no");
    }
  else if (g_strcmp0 (match, "(dynamics-expanded yes)") == 0)
    {
      /* This option just doesn't exist anymore. */
    }
  else
    {
      g_message ("(WARNING) %s: invalid match \"%s\"", G_STRFUNC, match);
      g_string_append (new_value, match);
    }

  g_free (match);
  return FALSE;
}

/* Actually not only for contextrc, but all other files where
 * ligma-blend-tool may appear. Apparently that is also "devicerc", as
 * well as "toolrc" (but this one is skipped anyway).
 */
#define CONTEXTRC_UPDATE_PATTERN \
  "ligma-blend-tool"           "|" \
  "dynamics \"Dynamics Off\"" "|" \
  "\\(dynamics-expanded yes\\)"

static gboolean
user_update_contextrc_over20 (const GMatchInfo *matched_value,
                              GString          *new_value,
                              gpointer          data)
{
  gchar *match = g_match_info_fetch (matched_value, 0);

  if (g_strcmp0 (match, "ligma-blend-tool") == 0)
    {
      g_string_append (new_value, "ligma-gradient-tool");
    }
  else if (g_strcmp0 (match, "dynamics \"Dynamics Off\"") == 0)
    {
      g_string_append (new_value, "dynamics-enabled no");
    }
  else if (g_strcmp0 (match, "(dynamics-expanded yes)") == 0)
    {
      /* This option just doesn't exist anymore. */
    }
  else
    {
      g_message ("(WARNING) %s: invalid match \"%s\"", G_STRFUNC, match);
      g_string_append (new_value, match);
    }

  g_free (match);
  return FALSE;
}

static gboolean
user_install_dir_copy (LigmaUserInstall    *install,
                       gint                level,
                       const gchar        *source,
                       const gchar        *base,
                       const gchar        *update_pattern,
                       GRegexEvalCallback  update_callback)
{
  GDir        *source_dir = NULL;
  GDir        *dest_dir   = NULL;
  gchar        dest[1024];
  const gchar *basename;
  gchar       *dirname = NULL;
  gchar       *name;
  GError      *error   = NULL;
  gboolean     success = FALSE;

  if (level >= 5)
    {
      /* Config migration is recursive, but we can't go on forever,
       * since we may fall into recursive symlinks in particular (which
       * is a security risk to fill a disk, and would also block LIGMA
       * forever at migration stage).
       * Let's just break the recursivity at 5 levels, which is just an
       * arbitrary value (but I don't think there should be any data
       * deeper than this).
       */
      goto error;
    }

  name = g_path_get_basename (source);
  dirname = g_build_filename (base, name, NULL);
  g_free (name);

  success = user_install_mkdir (install, dirname);
  if (! success)
    goto error;

  success = (dest_dir = g_dir_open (dirname, 0, &error)) != NULL;
  if (! success)
    goto error;

  success = (source_dir = g_dir_open (source, 0, &error)) != NULL;
  if (! success)
    goto error;

  while ((basename = g_dir_read_name (source_dir)) != NULL)
    {
      name = g_build_filename (source, basename, NULL);

      if (g_file_test (name, G_FILE_TEST_IS_REGULAR))
        {
          g_snprintf (dest, sizeof (dest), "%s%c%s",
                      dirname, G_DIR_SEPARATOR, basename);

          success = user_install_file_copy (install, name, dest,
                                            update_pattern,
                                            update_callback);
          if (! success)
            {
              g_free (name);
              goto error;
            }
        }
      else
        {
          user_install_dir_copy (install, level + 1, name, dirname,
                                 update_pattern, update_callback);
        }

      g_free (name);
    }

 error:
  user_install_log_error (install, &error);

  if (source_dir)
    g_dir_close (source_dir);

  if (dest_dir)
    g_dir_close (dest_dir);

  if (dirname)
    g_free (dirname);

  return success;
}

static gboolean
user_install_create_files (LigmaUserInstall *install)
{
  gchar dest[1024];
  gchar source[1024];
  gint  i;

  for (i = 0; i < G_N_ELEMENTS (ligma_user_install_items); i++)
    {
      g_snprintf (dest, sizeof (dest), "%s%c%s",
                  ligma_directory (),
                  G_DIR_SEPARATOR,
                  ligma_user_install_items[i].name);

      if (g_file_test (dest, G_FILE_TEST_EXISTS))
        continue;

      switch (ligma_user_install_items[i].action)
        {
        case USER_INSTALL_MKDIR:
          if (! user_install_mkdir (install, dest))
            return FALSE;
          break;

        case USER_INSTALL_COPY:
          g_snprintf (source, sizeof (source), "%s%c%s",
                      ligma_sysconf_directory (), G_DIR_SEPARATOR,
                      ligma_user_install_items[i].name);

          if (! user_install_file_copy (install, source, dest, NULL, NULL))
            return FALSE;
          break;
        }
    }

  g_snprintf (dest, sizeof (dest), "%s%c%s",
              ligma_directory (), G_DIR_SEPARATOR, "tags.xml");

  if (! g_file_test (dest, G_FILE_TEST_IS_REGULAR))
    {
      /* if there was no tags.xml, install it with default tag set.
       */
      if (! ligma_tags_user_install ())
        {
          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
user_install_migrate_files (LigmaUserInstall *install)
{
  GDir        *dir;
  const gchar *basename;
  gchar        dest[1024];
  LigmaRc      *ligmarc;
  GError      *error = NULL;

  dir = g_dir_open (install->old_dir, 0, &error);

  if (! dir)
    {
      user_install_log_error (install, &error);
      return FALSE;
    }

  while ((basename = g_dir_read_name (dir)) != NULL)
    {
      gchar *source = g_build_filename (install->old_dir, basename, NULL);

      if (g_file_test (source, G_FILE_TEST_IS_REGULAR))
        {
          const gchar        *update_pattern = NULL;
          GRegexEvalCallback  update_callback = NULL;

          /*  skip these files for all old versions  */
          if (strcmp (basename, "documents") == 0      ||
              g_str_has_prefix (basename, "ligmaswap.") ||
              strcmp (basename, "pluginrc") == 0       ||
              strcmp (basename, "themerc") == 0        ||
              strcmp (basename, "toolrc") == 0         ||
              strcmp (basename, "gtkrc") == 0)
            {
              goto next_file;
            }
          else if (install->old_major < 3 &&
                   strcmp (basename, "sessionrc") == 0)
            {
              /* We need to update size and positions because of scale
               * factor support.
               */
              update_pattern  = SESSIONRC_UPDATE_PATTERN;
              update_callback = user_update_sessionrc;
            }
          else if (strcmp (basename, "menurc") == 0)
            {
              switch (install->old_minor)
                {
                case 0:
                  /*  skip menurc for ligma 2.0 as the format has changed  */
                  goto next_file;
                  break;
                default:
                  update_pattern  = MENURC_OVER20_UPDATE_PATTERN;
                  update_callback = user_update_menurc_over20;
                  break;
                }
            }
          else if (strcmp (basename, "controllerrc") == 0)
            {
              update_pattern  = CONTROLLERRC_UPDATE_PATTERN;
              update_callback = user_update_controllerrc;
            }
          else if (strcmp (basename, "ligmarc") == 0)
            {
              update_pattern  = LIGMARC_UPDATE_PATTERN;
              update_callback = user_update_ligmarc;
            }
          else if (strcmp (basename, "contextrc") == 0      ||
                   strcmp (basename, "devicerc") == 0)
            {
              update_pattern  = CONTEXTRC_UPDATE_PATTERN;
              update_callback = user_update_contextrc_over20;
            }

          g_snprintf (dest, sizeof (dest), "%s%c%s",
                      ligma_directory (), G_DIR_SEPARATOR, basename);

          user_install_file_copy (install, source, dest,
                                  update_pattern, update_callback);
        }
      else if (g_file_test (source, G_FILE_TEST_IS_DIR))
        {
          const gchar        *update_pattern = NULL;
          GRegexEvalCallback  update_callback = NULL;

          /*  skip these directories for all old versions  */
          if (strcmp (basename, "tmp") == 0          ||
              strcmp (basename, "tool-options") == 0 ||
              strcmp (basename, "themes") == 0)
            {
              goto next_file;
            }
          else if (install->old_major < 3 &&
                   (strcmp (basename, "plug-ins") == 0 ||
                    strcmp (basename, "scripts") == 0))
            {
              /* Major API update. */
              goto next_file;
            }

          if (strcmp (basename, "ligmaressionist") == 0)
            {
              update_pattern  = LIGMARESSIONIST_UPDATE_PATTERN;
              update_callback = user_update_ligmaressionist;
            }
          else if (strcmp (basename, "tool-presets") == 0)
            {
              update_pattern  = TOOL_PRESETS_UPDATE_PATTERN;
              update_callback = user_update_tool_presets;
            }
          user_install_dir_copy (install, 0, source, ligma_directory (),
                                 update_pattern, update_callback);
        }

    next_file:
      g_free (source);
    }

  /*  create the tmp directory that was explicitly not copied  */

  g_snprintf (dest, sizeof (dest), "%s%c%s",
              ligma_directory (), G_DIR_SEPARATOR, "tmp");

  user_install_mkdir (install, dest);
  g_dir_close (dir);

  ligma_templates_migrate (install->old_dir);

  ligmarc = ligma_rc_new (install->ligma, NULL, NULL, FALSE);
  ligma_rc_migrate (ligmarc);
  ligma_rc_save (ligmarc);
  g_object_unref (ligmarc);

  return TRUE;
}
