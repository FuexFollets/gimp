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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef __GLIBC__
#include <malloc.h>
#endif

#include <locale.h>

#include <gio/gio.h>

#ifdef G_OS_WIN32
#include <io.h> /* get_osfhandle */

#endif /* G_OS_WIN32 */

#if defined(ENABLE_RELOCATABLE_RESOURCES) && defined(__APPLE__)
#include <libgen.h> /* dirname */
#include <sys/stat.h>
#endif /* __APPLE__ */

#ifndef LIGMA_CONSOLE_COMPILATION
#include <gtk/gtk.h>
#else
#include <gdk-pixbuf/gdk-pixbuf.h>
#endif

#include <babl/babl.h>

#include "libligmabase/ligmabase.h"

#include "pdb/pdb-types.h"

#include "config/ligmaearlyrc.h"
#include "config/ligmaconfig-dump.h"

#include "core/ligma.h"
#include "core/ligmabacktrace.h"

#include "pdb/ligmapdb.h"
#include "pdb/ligmaprocedure.h"
#include "pdb/internal-procs.h"

#include "about.h"
#include "app.h"
#include "language.h"
#include "sanity.h"
#include "signals.h"
#include "unique.h"

#ifdef G_OS_WIN32
/* To get PROCESS_DEP_* defined we need _WIN32_WINNT at 0x0601. We still
 * use the API optionally only if present, though.
 */
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <conio.h>
#endif

#include "ligma-log.h"
#include "ligma-intl.h"
#include "ligma-version.h"


static gboolean  ligma_option_fatal_warnings   (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);
static gboolean  ligma_option_stack_trace_mode (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);
static gboolean  ligma_option_pdb_compat_mode  (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);
static gboolean  ligma_option_dump_ligmarc      (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);
static gboolean  ligma_option_dump_pdb_procedures_deprecated
                                              (const gchar  *option_name,
                                               const gchar  *value,
                                               gpointer      data,
                                               GError      **error);

static void      ligma_show_version_and_exit   (void) G_GNUC_NORETURN;
static void      ligma_show_license_and_exit   (void) G_GNUC_NORETURN;

static void      ligma_init_i18n               (void);
static void      ligma_init_malloc             (void);

#if defined (G_OS_WIN32) && !defined (LIGMA_CONSOLE_COMPILATION)
static void      ligma_open_console_window     (void);
#else
#define ligma_open_console_window() /* as nothing */
#endif

static const gchar        *system_ligmarc     = NULL;
static const gchar        *user_ligmarc       = NULL;
static const gchar        *session_name      = NULL;
static const gchar        *batch_interpreter = NULL;
static const gchar       **batch_commands    = NULL;
static const gchar       **filenames         = NULL;
static gboolean            quit              = FALSE;
static gboolean            as_new            = FALSE;
static gboolean            no_interface      = FALSE;
static gboolean            no_data           = FALSE;
static gboolean            no_fonts          = FALSE;
static gboolean            no_splash         = FALSE;
static gboolean            be_verbose        = FALSE;
static gboolean            new_instance      = FALSE;
#if defined (USE_SYSV_SHM) || defined (USE_POSIX_SHM) || defined (G_OS_WIN32)
static gboolean            use_shm           = TRUE;
#else
static gboolean            use_shm           = FALSE;
#endif
static gboolean            use_cpu_accel     = TRUE;
static gboolean            console_messages  = FALSE;
static gboolean            use_debug_handler = FALSE;

#ifdef LIGMA_UNSTABLE
static gboolean            show_playground   = TRUE;
static gboolean            show_debug_menu   = TRUE;
static LigmaStackTraceMode  stack_trace_mode  = LIGMA_STACK_TRACE_QUERY;
static LigmaPDBCompatMode   pdb_compat_mode   = LIGMA_PDB_COMPAT_WARN;
#else
static gboolean            show_playground   = FALSE;
static gboolean            show_debug_menu   = FALSE;
static LigmaStackTraceMode  stack_trace_mode  = LIGMA_STACK_TRACE_NEVER;
static LigmaPDBCompatMode   pdb_compat_mode   = LIGMA_PDB_COMPAT_ON;
#endif


static const GOptionEntry main_entries[] =
{
  { "version", 'v', G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, (GOptionArgFunc) ligma_show_version_and_exit,
    N_("Show version information and exit"), NULL
  },
  {
    "license", 0, G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, (GOptionArgFunc) ligma_show_license_and_exit,
    N_("Show license information and exit"), NULL
  },
  {
    "verbose", 0, 0,
    G_OPTION_ARG_NONE, &be_verbose,
    N_("Be more verbose"), NULL
  },
  {
    "new-instance", 'n', 0,
    G_OPTION_ARG_NONE, &new_instance,
    N_("Start a new LIGMA instance"), NULL
  },
  {
    "as-new", 'a', 0,
    G_OPTION_ARG_NONE, &as_new,
    N_("Open images as new"), NULL
  },
  {
    "no-interface", 'i', 0,
    G_OPTION_ARG_NONE, &no_interface,
    N_("Run without a user interface"), NULL
  },
  {
    "no-data", 'd', 0,
    G_OPTION_ARG_NONE, &no_data,
    N_("Do not load brushes, gradients, patterns, ..."), NULL
  },
  {
    "no-fonts", 'f', 0,
    G_OPTION_ARG_NONE, &no_fonts,
    N_("Do not load any fonts"), NULL
  },
  {
    "no-splash", 's', 0,
    G_OPTION_ARG_NONE, &no_splash,
    N_("Do not show a splash screen"), NULL
  },
  {
    "no-shm", 0, G_OPTION_FLAG_REVERSE,
    G_OPTION_ARG_NONE, &use_shm,
    N_("Do not use shared memory between LIGMA and plug-ins"), NULL
  },
  {
    "no-cpu-accel", 0, G_OPTION_FLAG_REVERSE,
    G_OPTION_ARG_NONE, &use_cpu_accel,
    N_("Do not use special CPU acceleration functions"), NULL
  },
  {
    "session", 0, 0,
    G_OPTION_ARG_FILENAME, &session_name,
    N_("Use an alternate sessionrc file"), "<name>"
  },
  {
    "ligmarc", 'g', 0,
    G_OPTION_ARG_FILENAME, &user_ligmarc,
    N_("Use an alternate user ligmarc file"), "<filename>"
  },
  {
    "system-ligmarc", 0, 0,
    G_OPTION_ARG_FILENAME, &system_ligmarc,
    N_("Use an alternate system ligmarc file"), "<filename>"
  },
  {
    "batch", 'b', 0,
    G_OPTION_ARG_STRING_ARRAY, &batch_commands,
    N_("Batch command to run (can be used multiple times)"), "<command>"
  },
  {
    "batch-interpreter", 0, 0,
    G_OPTION_ARG_STRING, &batch_interpreter,
    N_("The procedure to process batch commands with"), "<proc>"
  },
  {
    "quit", 0, 0,
    G_OPTION_ARG_NONE, &quit,
    N_("Quit immediately after performing requested actions"), NULL
  },
  {
    "console-messages", 'c', 0,
    G_OPTION_ARG_NONE, &console_messages,
    N_("Send messages to console instead of using a dialog"), NULL
  },
  {
    "pdb-compat-mode", 0, 0,
    G_OPTION_ARG_CALLBACK, ligma_option_pdb_compat_mode,
    /*  don't translate the mode names (off|on|warn)  */
    N_("PDB compatibility mode (off|on|warn)"), "<mode>"
  },
  {
    "stack-trace-mode", 0, 0,
    G_OPTION_ARG_CALLBACK, ligma_option_stack_trace_mode,
    /*  don't translate the mode names (never|query|always)  */
    N_("Debug in case of a crash (never|query|always)"), "<mode>"
  },
  {
    "debug-handlers", 0, 0,
    G_OPTION_ARG_NONE, &use_debug_handler,
    N_("Enable non-fatal debugging signal handlers"), NULL
  },
  {
    "g-fatal-warnings", 0, G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, ligma_option_fatal_warnings,
    N_("Make all warnings fatal"), NULL
  },
  {
    "dump-ligmarc", 0, G_OPTION_FLAG_NO_ARG,
    G_OPTION_ARG_CALLBACK, ligma_option_dump_ligmarc,
    N_("Output a ligmarc file with default settings"), NULL
  },
  {
    "dump-ligmarc-system", 0, G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_CALLBACK, ligma_option_dump_ligmarc,
    NULL, NULL
  },
  {
    "dump-ligmarc-manpage", 0, G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_CALLBACK, ligma_option_dump_ligmarc,
    NULL, NULL
  },
  {
    "dump-pdb-procedures-deprecated", 0,
    G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_CALLBACK, ligma_option_dump_pdb_procedures_deprecated,
    N_("Output a sorted list of deprecated procedures in the PDB"), NULL
  },
  {
    "show-playground", 0, 0,
    G_OPTION_ARG_NONE, &show_playground,
    N_("Show a preferences page with experimental features"), NULL
  },
  {
    "show-debug-menu", 0, G_OPTION_FLAG_HIDDEN,
    G_OPTION_ARG_NONE, &show_debug_menu,
    N_("Show an image submenu with debug actions"), NULL
  },
  {
    G_OPTION_REMAINING, 0, 0,
    G_OPTION_ARG_FILENAME_ARRAY, &filenames,
    NULL, NULL
  },
  { NULL }
};

#if defined(ENABLE_RELOCATABLE_RESOURCES) && defined(__APPLE__)
static void
ligma_macos_setenv (const char * progname)
{
  /* helper to set environment variables for LIGMA to be relocatable.
   * Due to the latest changes it is not recommended to set it in the shell
   * wrapper anymore.
   */
  gchar  *resolved_path;
  /* on some OSX installations open file limit is 256 and LIGMA needs more */
  struct  rlimit limit;

  limit.rlim_cur = 10000;
  limit.rlim_max = 10000;
  setrlimit (RLIMIT_NOFILE, &limit);
  resolved_path = g_canonicalize_filename (progname, NULL);
  if (resolved_path && ! g_getenv ("LIGMA_NO_WRAPPER"))
    {
      /* set path to the app folder to make sure that our python is called
       * instead of system one
       */
      static gboolean            show_playground   = TRUE;

      gchar   *path;
      gchar   *tmp;
      gchar   *app_dir;
      gchar   *res_dir;
      size_t   path_len;
      struct   stat sb;
      gboolean need_pythonhome = TRUE;

      app_dir = g_path_get_dirname (resolved_path);
      tmp = g_strdup_printf ("%s/../Resources", app_dir);
      res_dir = g_canonicalize_filename (tmp, NULL);
      g_free (tmp);
      if (res_dir && !stat (res_dir, &sb) && S_ISDIR (sb.st_mode))
        {
          g_print ("LIGMA is started as MacOS application\n");
        }
      else
        {
          tmp = g_strdup_printf ("%s/../share", app_dir);
          res_dir = g_canonicalize_filename (tmp, NULL);
          g_free (tmp);
          if (res_dir && !stat (res_dir, &sb) && S_ISDIR (sb.st_mode))
            {
              g_free (res_dir);

              g_print ("LIGMA is started in the build directory\n");

              tmp = g_strdup_printf ("%s/..", app_dir); /* running in build dir */
              res_dir = g_canonicalize_filename (tmp, NULL);
              g_free (tmp);
            }
          else
            {
              g_free (res_dir);
              return;
            }
        }

      /* Detect we were built in homebrew for MacOS */
      tmp = g_strdup_printf ("%s/Frameworks/Python.framework", res_dir);
      if (tmp && !stat (tmp, &sb) && S_ISDIR (sb.st_mode))
        {
          g_print ("LIGMA was built with homebrew\n");
          need_pythonhome = FALSE;
        }
      g_free (tmp);
      /* Detect we were built in MacPorts for MacOS */
      tmp = g_strdup_printf ("%s/Library/Frameworks/Python.framework", res_dir);
      if (tmp && !stat (tmp, &sb) && S_ISDIR (sb.st_mode))
        {
          g_print ("LIGMA was built with MacPorts\n");
          need_pythonhome = FALSE;
        }
      g_free (tmp);

      path_len = strlen (g_getenv ("PATH") ? g_getenv ("PATH") : "") + strlen (app_dir) + 2;
      path = g_try_malloc (path_len);
      if (path == NULL)
        {
          g_warning ("Failed to allocate memory");
          app_exit (EXIT_FAILURE);
        }
      if (g_getenv ("PATH"))
        g_snprintf (path, path_len, "%s:%s", app_dir, g_getenv ("PATH"));
      else
        g_snprintf (path, path_len, "%s", app_dir);
      g_free (app_dir);
      g_setenv ("PATH", path, TRUE);
      g_free (path);
      tmp = g_strdup_printf ("%s/lib/gtk-3.0/3.0.0", res_dir);
      g_setenv ("GTK_PATH", tmp, TRUE);
      g_free (tmp);
      tmp = g_strdup_printf ("%s/lib/gegl-0.4", res_dir);
      g_setenv ("GEGL_PATH", tmp, TRUE);
      g_free (tmp);
      tmp = g_strdup_printf ("%s/lib/babl-0.1", res_dir);
      g_setenv ("BABL_PATH", tmp, TRUE);
      g_free (tmp);
      tmp = g_strdup_printf ("%s/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache", res_dir);
      g_setenv ("GDK_PIXBUF_MODULE_FILE", tmp, TRUE);
      g_free (tmp);
      tmp = g_strdup_printf ("%s/etc/fonts", res_dir);
      g_setenv ("FONTCONFIG_PATH", tmp, TRUE);
      g_free (tmp);
      if (need_pythonhome)
        {
          tmp = g_strdup_printf ("%s", res_dir);
          g_setenv ("PYTHONHOME", tmp, TRUE);
          g_free (tmp);
        }
      tmp = g_strdup_printf ("%s/lib/python3.9", res_dir);
      g_setenv ("PYTHONPATH", tmp, TRUE);
      g_free (tmp);
      tmp = g_strdup_printf ("%s/lib/gio/modules", res_dir);
      g_setenv ("GIO_MODULE_DIR", tmp, TRUE);
      g_free (tmp);
      tmp = g_strdup_printf ("%s/share/libwmf/fonts", res_dir);
      g_setenv ("WMF_FONTDIR", tmp, TRUE);
      g_free (tmp);
      if (g_getenv ("DYLD_LIBRARY_PATH"))
        tmp = g_strdup_printf ("%s/lib:%s", res_dir, g_getenv ("DYLD_LIBRARY_PATH"));
      else
        tmp = g_strdup_printf ("%s/lib", res_dir);
      g_setenv ("DYLD_LIBRARY_PATH", tmp, TRUE);
      g_free (tmp);
      if (g_getenv ("XDG_DATA_DIRS"))
        tmp = g_strdup_printf ("%s/share:%s", res_dir, g_getenv ("XDG_DATA_DIRS"));
      else
        tmp = g_strdup_printf ("%s/share", res_dir);
      g_setenv ("XDG_DATA_DIRS", tmp, TRUE);
      g_free (tmp);
      tmp = g_strdup_printf ("%s/lib/girepository-1.0", res_dir);
      g_setenv ("GI_TYPELIB_PATH", tmp, TRUE);
      g_free (tmp);
      if (g_getenv ("HOME") != NULL)
        {
          tmp = g_strdup_printf ("%s/Library/Application Support/LIGMA/3.00/cache",
                                 g_getenv ("HOME"));
          g_setenv ("XDG_CACHE_HOME", tmp, TRUE);
          g_free (tmp);
        }
      g_free (res_dir);
    }
  g_free (resolved_path);
}
#endif

/* ligma_early_configuration () is executed as soon as we can read
 * the "ligmarc" files, but before any library initialization takes
 * place
 */
static void
ligma_early_configuration (void)
{
  GFile       *system_ligmarc_file = NULL;
  GFile       *user_ligmarc_file   = NULL;
  LigmaEarlyRc *earlyrc;
  gchar       *language;

  if (system_ligmarc)
    system_ligmarc_file = g_file_new_for_commandline_arg (system_ligmarc);

  if (user_ligmarc)
    user_ligmarc_file = g_file_new_for_commandline_arg (user_ligmarc);

  /* LigmaEarlyRc is reponsible for reading "ligmarc" files for the
   * sole purpose of getting some configuration data that is needed
   * in the early initialization phase
   */
  earlyrc = ligma_early_rc_new (system_ligmarc_file,
                               user_ligmarc_file,
                               be_verbose);

  /* Language needs to be determined first, before any LigmaContext is
   * instantiated (which happens when the Ligma object is created)
   * because its properties need to be properly localized in the
   * settings language (if different from system language). Otherwise we
   * end up with pieces of GUI always using the system language (cf. bug
   * 787457)
   */
  language = ligma_early_rc_get_language (earlyrc);

  /*  change the locale if a language if specified  */
  language_init (language);
  if (language)
    g_free (language);

#if defined (G_OS_WIN32) && !defined (LIGMA_CONSOLE_COMPILATION)

#if GTK_MAJOR_VERSION > 3
#warning For GTK4 and above use the proper backend-specific API instead of the GDK_WIN32_TABLET_INPUT_API environment variable
#endif

  /* Set a GdkWin32-specific environment variable to specify
   * the desired pen / touch input API to use on Windows
  */
  if (gtk_get_major_version () == 3 &&
      (gtk_get_minor_version () > 24 ||
       (gtk_get_minor_version () == 24 &&
        gtk_get_micro_version () >= 30)))
    {
      LigmaWin32PointerInputAPI api = ligma_early_rc_get_win32_pointer_input_api (earlyrc);

      switch (api)
        {
        case LIGMA_WIN32_POINTER_INPUT_API_WINTAB:
          g_setenv ("GDK_WIN32_TABLET_INPUT_API", "wintab", TRUE);
          break;
        case LIGMA_WIN32_POINTER_INPUT_API_WINDOWS_INK:
          g_setenv ("GDK_WIN32_TABLET_INPUT_API", "winpointer", TRUE);
          break;
        }
    }

#endif

  g_object_unref (earlyrc);

  g_clear_object (&system_ligmarc_file);
  g_clear_object (&user_ligmarc_file);
}

static gboolean
ligma_options_group_parse_hook (GOptionContext   *context,
                               GOptionGroup     *group,
                               gpointer          data,
                               GError          **error)
{
  /*  early initialization from data stored in "ligmarc" files  */
  ligma_early_configuration ();

  return TRUE;
}

int
main (int    argc,
      char **argv)
{
  GOptionContext *context;
  GError         *error = NULL;
  const gchar    *abort_message;
  gchar          *basename;
  GFile          *system_ligmarc_file = NULL;
  GFile          *user_ligmarc_file   = NULL;
  GOptionGroup   *ligma_group         = NULL;
  gchar          *backtrace_file     = NULL;
  gint            retval;
  gint            i;

#ifdef ENABLE_WIN32_DEBUG_CONSOLE
  ligma_open_console_window ();
#endif
#if defined(ENABLE_RELOCATABLE_RESOURCES) && defined(__APPLE__)
  /* remove MacOS session identifier from the command line args */
  gint newargc = 0;
  for (gint i = 0; i < argc; i++)
    {
      if (!g_str_has_prefix (argv[i], "-psn_"))
        {
          argv[newargc] = argv[i];
          newargc++;
        }
    }
  if (argc > newargc)
    {
      argv[newargc] = NULL; /* glib expects NULL terminated array */
      argc = newargc;
    }

  ligma_macos_setenv (argv[0]);
#endif

#if defined (__GNUC__) && defined (_WIN64)
  /* mingw-w64, at least the unstable build from late July 2008,
   * starts subsystem:windows programs in main(), but passes them
   * bogus argc and argv. __argc and __argv are OK, though, so just
   * use them.
   */
  argc = __argc;
  argv = __argv;
#endif

  /* Initialize LigmaBacktrace early on.  In particular, we want the
   * Windows backend to catch the SET_THREAD_NAME exceptions of newly
   * created threads.
   */
  ligma_backtrace_init ();

  /* Start signal handlers early. */
  ligma_init_signal_handlers (&backtrace_file);

#ifdef G_OS_WIN32
  /* Reduce risks */
  {
    typedef BOOL (WINAPI *t_SetDllDirectoryA) (LPCSTR lpPathName);
    t_SetDllDirectoryA p_SetDllDirectoryA;

    p_SetDllDirectoryA =
      (t_SetDllDirectoryA) GetProcAddress (GetModuleHandle ("kernel32.dll"),
                                           "SetDllDirectoryA");
    if (p_SetDllDirectoryA)
      (*p_SetDllDirectoryA) ("");
  }

  /* On Windows, set DLL search path to $INSTALLDIR/bin so that .exe
     plug-ins in the plug-ins directory can find libligma and file
     library DLLs without needing to set external PATH. */
  {
    const gchar *install_dir;
    gchar       *bin_dir;
    LPWSTR       w_bin_dir;
    int          n;

    w_bin_dir = NULL;
    install_dir = ligma_installation_directory ();
    bin_dir = g_build_filename (install_dir, "bin", NULL);

    n = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS,
                             bin_dir, -1, NULL, 0);
    if (n == 0)
      goto out;

    w_bin_dir = g_malloc_n (n + 1, sizeof (wchar_t));
    n = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS,
                             bin_dir, -1,
                             w_bin_dir, (n + 1) * sizeof (wchar_t));
    if (n == 0)
      goto out;

    SetDllDirectoryW (w_bin_dir);

  out:
    if (w_bin_dir)
      g_free (w_bin_dir);
    g_free (bin_dir);
  }

#ifndef _WIN64
  {
    typedef BOOL (WINAPI *t_SetProcessDEPPolicy) (DWORD dwFlags);
    t_SetProcessDEPPolicy p_SetProcessDEPPolicy;

    p_SetProcessDEPPolicy =
      (t_SetProcessDEPPolicy) GetProcAddress (GetModuleHandle ("kernel32.dll"),
                                              "SetProcessDEPPolicy");
    if (p_SetProcessDEPPolicy)
      (*p_SetProcessDEPPolicy) (PROCESS_DEP_ENABLE|PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);
  }
#endif

  /* Group all our windows together on the taskbar */
  {
    typedef HRESULT (WINAPI *t_SetCurrentProcessExplicitAppUserModelID) (PCWSTR lpPathName);
    t_SetCurrentProcessExplicitAppUserModelID p_SetCurrentProcessExplicitAppUserModelID;

    p_SetCurrentProcessExplicitAppUserModelID =
      (t_SetCurrentProcessExplicitAppUserModelID) GetProcAddress (GetModuleHandle ("shell32.dll"),
                                                                  "SetCurrentProcessExplicitAppUserModelID");
    if (p_SetCurrentProcessExplicitAppUserModelID)
      (*p_SetCurrentProcessExplicitAppUserModelID) (L"ligma.LigmaApplication");
  }
#endif

  ligma_init_malloc ();

  ligma_env_init (FALSE);

  ligma_log_init ();

  ligma_init_i18n ();

  g_set_application_name (LIGMA_NAME);

#ifdef G_OS_WIN32
  argv = g_win32_get_command_line ();
#else
  argv = g_strdupv (argv);
#endif

  basename = g_path_get_basename (argv[0]);
  g_set_prgname (basename);
  g_free (basename);

  /* Check argv[] for "--verbose" first */
  for (i = 1; i < argc; i++)
    {
      const gchar *arg = argv[i];

      if (arg[0] != '-')
        continue;

      if ((strcmp (arg, "--verbose") == 0) || (strcmp (arg, "-v") == 0))
        {
          be_verbose = TRUE;
        }
    }

  /* Check argv[] for "--no-interface" before trying to initialize gtk+. */
  for (i = 1; i < argc; i++)
    {
      const gchar *arg = argv[i];

      if (arg[0] != '-')
        continue;

      if ((strcmp (arg, "--no-interface") == 0) || (strcmp (arg, "-i") == 0))
        {
          no_interface = TRUE;
        }
      else if ((strcmp (arg, "--version") == 0) || (strcmp (arg, "-v") == 0))
        {
          ligma_show_version_and_exit ();
        }
#if defined (G_OS_WIN32) && !defined (LIGMA_CONSOLE_COMPILATION)
      else if ((strcmp (arg, "--help") == 0) ||
               (strcmp (arg, "-?") == 0) ||
               (strncmp (arg, "--help-", 7) == 0))
        {
          ligma_open_console_window ();
        }
#endif
    }

#ifdef LIGMA_CONSOLE_COMPILATION
  no_interface = TRUE;
#endif

  context = g_option_context_new (_("[FILE|URI...]"));
  g_option_context_set_summary (context, LIGMA_NAME);

  g_option_context_add_main_entries (context, main_entries, GETTEXT_PACKAGE);

  /* The LIGMA option group is just an empty option group, created for the sole
   * purpose of running a post-parse hook before any other of dependant libraries
   * are run. This makes it possible to apply options from configuration data
   * obtained from "ligmarc" files, before other libraries have a chance to run
   * some of their intialization code.
   */
  ligma_group = g_option_group_new ("ligma", "", "", NULL, NULL);
  g_option_group_set_parse_hooks (ligma_group, NULL, ligma_options_group_parse_hook);
  g_option_context_add_group (context, ligma_group);

  app_libs_init (context, no_interface);

  if (! g_option_context_parse_strv (context, &argv, &error))
    {
      if (error)
        {
          ligma_open_console_window ();
          g_print ("%s\n", error->message);
          g_error_free (error);
        }
      else
        {
          g_print ("%s\n",
                   _("LIGMA could not initialize the graphical user interface.\n"
                     "Make sure a proper setup for your display environment "
                     "exists."));
        }

      app_exit (EXIT_FAILURE);
    }

  if (no_interface || be_verbose || console_messages || batch_commands != NULL)
    ligma_open_console_window ();

  if (no_interface)
    new_instance = TRUE;

#ifndef LIGMA_CONSOLE_COMPILATION
  if (! new_instance && ligma_unique_open (filenames, as_new))
    {
      if (be_verbose)
        g_print ("%s\n",
                 _("Another LIGMA instance is already running."));

      if (batch_commands)
        ligma_unique_batch_run (batch_interpreter, batch_commands);

      gdk_notify_startup_complete ();

      return EXIT_SUCCESS;
    }
#endif

  abort_message = sanity_check_early ();
  if (abort_message)
    app_abort (no_interface, abort_message);

  if (system_ligmarc)
    system_ligmarc_file = g_file_new_for_commandline_arg (system_ligmarc);

  if (user_ligmarc)
    user_ligmarc_file = g_file_new_for_commandline_arg (user_ligmarc);

  retval = app_run (argv[0],
                    filenames,
                    system_ligmarc_file,
                    user_ligmarc_file,
                    session_name,
                    batch_interpreter,
                    batch_commands,
                    quit,
                    as_new,
                    no_interface,
                    no_data,
                    no_fonts,
                    no_splash,
                    be_verbose,
                    use_shm,
                    use_cpu_accel,
                    console_messages,
                    use_debug_handler,
                    show_playground,
                    show_debug_menu,
                    stack_trace_mode,
                    pdb_compat_mode,
                    backtrace_file);

  g_free (backtrace_file);

  g_clear_object (&system_ligmarc_file);
  g_clear_object (&user_ligmarc_file);

  g_strfreev (argv);

  g_option_context_free (context);

  return retval;
}


#ifdef G_OS_WIN32

/* Provide WinMain in case we build LIGMA as a subsystem:windows
 * application. Well, we do. When built with mingw, though, user code
 * execution still starts in main() in that case. So WinMain() gets
 * used on MSVC builds only.
 */

#ifdef __GNUC__
#  ifndef _stdcall
#    define _stdcall  __attribute__((stdcall))
#  endif
#endif

int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
         struct HINSTANCE__ *hPrevInstance,
         char               *lpszCmdLine,
         int                 nCmdShow)
{
  return main (__argc, __argv);
}

#ifndef LIGMA_CONSOLE_COMPILATION

static void
wait_console_window (void)
{
  FILE *console = fopen ("CONOUT$", "w");

  SetConsoleTitleW (g_utf8_to_utf16 (_("LIGMA output. Type any character to close this window."), -1, NULL, NULL, NULL));
  fprintf (console, _("(Type any character to close this window)\n"));
  fflush (console);
  _getch ();
}

static void
ligma_open_console_window (void)
{
  if (((HANDLE) _get_osfhandle (fileno (stdout)) == INVALID_HANDLE_VALUE ||
       (HANDLE) _get_osfhandle (fileno (stderr)) == INVALID_HANDLE_VALUE) && AllocConsole ())
    {
      if ((HANDLE) _get_osfhandle (fileno (stdout)) == INVALID_HANDLE_VALUE)
        freopen ("CONOUT$", "w", stdout);

      if ((HANDLE) _get_osfhandle (fileno (stderr)) == INVALID_HANDLE_VALUE)
        freopen ("CONOUT$", "w", stderr);

      SetConsoleTitleW (g_utf8_to_utf16 (_("LIGMA output. You can minimize this window, but don't close it."), -1, NULL, NULL, NULL));

      atexit (wait_console_window);
    }
}
#endif

#endif /* G_OS_WIN32 */


static gboolean
ligma_option_fatal_warnings (const gchar  *option_name,
                            const gchar  *value,
                            gpointer      data,
                            GError      **error)
{
  GLogLevelFlags fatal_mask;

  fatal_mask = g_log_set_always_fatal (G_LOG_FATAL_MASK);
  fatal_mask |= G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL;

  g_log_set_always_fatal (fatal_mask);

  return TRUE;
}

static gboolean
ligma_option_stack_trace_mode (const gchar  *option_name,
                              const gchar  *value,
                              gpointer      data,
                              GError      **error)
{
  if (strcmp (value, "never") == 0)
    stack_trace_mode = LIGMA_STACK_TRACE_NEVER;
  else if (strcmp (value, "query") == 0)
    stack_trace_mode = LIGMA_STACK_TRACE_QUERY;
  else if (strcmp (value, "always") == 0)
    stack_trace_mode = LIGMA_STACK_TRACE_ALWAYS;
  else
    return FALSE;

  return TRUE;
}

static gboolean
ligma_option_pdb_compat_mode (const gchar  *option_name,
                             const gchar  *value,
                             gpointer      data,
                             GError      **error)
{
  if (! strcmp (value, "off"))
    pdb_compat_mode = LIGMA_PDB_COMPAT_OFF;
  else if (! strcmp (value, "on"))
    pdb_compat_mode = LIGMA_PDB_COMPAT_ON;
  else if (! strcmp (value, "warn"))
    pdb_compat_mode = LIGMA_PDB_COMPAT_WARN;
  else
    return FALSE;

  return TRUE;
}

static gboolean
ligma_option_dump_ligmarc (const gchar  *option_name,
                         const gchar  *value,
                         gpointer      data,
                         GError      **error)
{
  LigmaConfigDumpFormat format = LIGMA_CONFIG_DUMP_NONE;

  ligma_open_console_window ();

  if (strcmp (option_name, "--dump-ligmarc") == 0)
    format = LIGMA_CONFIG_DUMP_LIGMARC;
  if (strcmp (option_name, "--dump-ligmarc-system") == 0)
    format = LIGMA_CONFIG_DUMP_LIGMARC_SYSTEM;
  else if (strcmp (option_name, "--dump-ligmarc-manpage") == 0)
    format = LIGMA_CONFIG_DUMP_LIGMARC_MANPAGE;

  if (format)
    {
      Ligma     *ligma;
      gboolean  success;

      babl_init ();
      ligma = g_object_new (LIGMA_TYPE_LIGMA, NULL);
      ligma_load_config (ligma, NULL, NULL);

      success = ligma_config_dump (G_OBJECT (ligma), format);

      g_object_unref (ligma);

      app_exit (success ? EXIT_SUCCESS : EXIT_FAILURE);
    }

  return FALSE;
}

static gboolean
ligma_option_dump_pdb_procedures_deprecated (const gchar  *option_name,
                                            const gchar  *value,
                                            gpointer      data,
                                            GError      **error)
{
  Ligma  *ligma;
  GList *deprecated_procs;
  GList *iter;

  babl_init ();
  ligma = g_object_new (LIGMA_TYPE_LIGMA, NULL);
  ligma_load_config (ligma, NULL, NULL);

  /* Make sure to turn on compatibility mode so deprecated procedures
   * are included
   */
  ligma->pdb_compat_mode = LIGMA_PDB_COMPAT_ON;

  /* Initialize the list of procedures */
  internal_procs_init (ligma->pdb);

  /* Get deprecated procedures */
  deprecated_procs = ligma_pdb_get_deprecated_procedures (ligma->pdb);

  for (iter = deprecated_procs; iter; iter = g_list_next (iter))
    {
      LigmaProcedure *procedure = LIGMA_PROCEDURE (iter->data);

      g_print ("%s\n", ligma_object_get_name (procedure));
    }

  g_list_free (deprecated_procs);

  g_object_unref (ligma);

  app_exit (EXIT_SUCCESS);

  return FALSE;
}

static void
ligma_show_version_and_exit (void)
{
  ligma_open_console_window ();
  ligma_version_show (be_verbose);

  app_exit (EXIT_SUCCESS);
}

static void
ligma_show_license_and_exit (void)
{
  ligma_open_console_window ();
  ligma_version_show (be_verbose);

  g_print ("\n");
  g_print (LIGMA_LICENSE);
  g_print ("\n\n");

  app_exit (EXIT_SUCCESS);
}

static void
ligma_init_malloc (void)
{
#ifdef LIGMA_GLIB_MEM_PROFILER
  g_mem_set_vtable (glib_mem_profiler_table);
  g_atexit (g_mem_profile);
#endif

#ifdef __GLIBC__
  /* Tweak memory allocation so that memory allocated in chunks >= 4k
   * (64x64 pixel 1bpp tile) gets returned to the system when free()'d.
   *
   * The default value for M_MMAP_THRESHOLD in glibc-2.3 is 128k.
   * This is said to be an empirically derived value that works well
   * in most systems. Lowering it to 4k is thus probably not the ideal
   * solution.
   *
   * An alternative to tuning this parameter would be to use
   * malloc_trim(), for example after releasing a large tile-manager.
   */
#if 0
  mallopt (M_MMAP_THRESHOLD, TILE_WIDTH * TILE_HEIGHT);
#endif
#endif
}

static void
ligma_init_i18n (void)
{
  /*  We may change the locale later if the user specifies a language
   *  in the ligmarc file. Here we are just initializing the locale
   *  according to the environment variables and set up the paths to
   *  the message catalogs.
   */

  setlocale (LC_ALL, "");

  bindtextdomain (GETTEXT_PACKAGE"-libligma", ligma_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE"-libligma", "UTF-8");
#endif

  bindtextdomain (GETTEXT_PACKAGE, ligma_locale_directory ());
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
#endif

  textdomain (GETTEXT_PACKAGE);
}
