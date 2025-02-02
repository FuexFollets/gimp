/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * busy-dialog.c
 * Copyright (C) 2018 Ell
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

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_PROC   "plug-in-busy-dialog"
#define PLUG_IN_BINARY "busy-dialog"
#define PLUG_IN_ROLE   "ligma-busy-dialog"


typedef struct
{
  GIOChannel *read_channel;
  GIOChannel *write_channel;
} Context;


typedef struct _BusyDialog      BusyDialog;
typedef struct _BusyDialogClass BusyDialogClass;

struct _BusyDialog
{
  LigmaPlugIn parent_instance;
};

struct _BusyDialogClass
{
  LigmaPlugInClass parent_class;
};


#define BUSY_DIALOG_TYPE  (busy_dialog_get_type ())
#define BUSY_DIALOG (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUSY_DIALOG_TYPE, BusyDialog))

GType                     busy_dialog_get_type         (void) G_GNUC_CONST;


static GList             * busy_dialog_query_procedures    (LigmaPlugIn           *plug_in);
static LigmaProcedure     * busy_dialog_create_procedure    (LigmaPlugIn           *plug_in,
                                                            const gchar          *name);
static LigmaValueArray    * busy_dialog_run                 (LigmaProcedure        *procedure,
                                                            const LigmaValueArray *args,
                                                            gpointer              run_data);

static LigmaPDBStatusType   busy_dialog                     (gint              read_fd,
                                                            gint              write_fd,
                                                            const gchar      *message,
                                                            gboolean          cancelable);

static gboolean            busy_dialog_read_channel_notify (GIOChannel       *source,
                                                            GIOCondition      condition,
                                                            Context          *context);

static gboolean            busy_dialog_delete_event        (GtkDialog        *dialog,
                                                            GdkEvent         *event,
                                                            Context          *context);
static void                busy_dialog_response            (GtkDialog        *dialog,
                                                            gint              response_id,
                                                            Context          *context);


G_DEFINE_TYPE (BusyDialog, busy_dialog, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (BUSY_DIALOG_TYPE)
DEFINE_STD_SET_I18N

static void
busy_dialog_class_init (BusyDialogClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = busy_dialog_query_procedures;
  plug_in_class->create_procedure = busy_dialog_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
busy_dialog_init (BusyDialog *busy_dialog)
{
}

static GList *
busy_dialog_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
busy_dialog_create_procedure (LigmaPlugIn  *plug_in,
                              const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_procedure_new (plug_in, name,
                                      LIGMA_PDB_PROC_TYPE_PLUGIN,
                                      busy_dialog_run, NULL, NULL);

      ligma_procedure_set_documentation (procedure,
                                        "Show a dialog while waiting for an "
                                        "operation to finish",
                                        "Used by LIGMA to display a dialog, "
                                        "containing a spinner and a custom "
                                        "message, while waiting for an "
                                        "ongoing operation to finish. "
                                        "Optionally, the dialog may provide "
                                        "a \"Cancel\" button, which can be used "
                                        "to cancel the operation.",
                                        name);
      ligma_procedure_set_attribution (procedure,
                                      "Ell",
                                      "Ell",
                                      "2018");

      LIGMA_PROC_ARG_ENUM (procedure, "run-mode",
                          "Run mode",
                          "The run mode",
                          LIGMA_TYPE_RUN_MODE,
                          LIGMA_RUN_INTERACTIVE,
                          G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "read-fd",
                         "The read file descriptor",
                         "The read file descriptor",
                         G_MININT, G_MAXINT, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_INT (procedure, "write-fd",
                         "The write file descriptor",
                         "The write file descriptor",
                         G_MININT, G_MAXINT, 0,
                         G_PARAM_READWRITE);

      LIGMA_PROC_ARG_STRING (procedure, "message",
                            "The message",
                            "The message",
                            NULL,
                            G_PARAM_READWRITE);

      LIGMA_PROC_ARG_BOOLEAN (procedure, "cancelable",
                             "Whether the dialog is cancelable",
                             "Whether the dialog is cancelable",
                             FALSE,
                             G_PARAM_READWRITE);
    }

  return procedure;
}

static LigmaValueArray *
busy_dialog_run (LigmaProcedure        *procedure,
                 const LigmaValueArray *args,
                 gpointer              run_data)
{
  LigmaValueArray    *return_vals = NULL;
  LigmaPDBStatusType  status = LIGMA_PDB_SUCCESS;
  LigmaRunMode        run_mode;

  run_mode = LIGMA_VALUES_GET_ENUM (args, 0);
  switch (run_mode)
    {
    case LIGMA_RUN_INTERACTIVE:
    case LIGMA_RUN_NONINTERACTIVE:
    case LIGMA_RUN_WITH_LAST_VALS:
      if (ligma_value_array_length (args) != 5)
        {
          status = LIGMA_PDB_CALLING_ERROR;
        }
      else
        {
          status = busy_dialog (LIGMA_VALUES_GET_INT (args, 1),
                                LIGMA_VALUES_GET_INT (args, 2),
                                LIGMA_VALUES_GET_STRING (args, 3),
                                LIGMA_VALUES_GET_BOOLEAN (args, 4));
        }
      break;

    default:
      status = LIGMA_PDB_CALLING_ERROR;
      break;
    }

  return_vals = ligma_procedure_new_return_values (procedure, status, NULL);

  return return_vals;
}

static LigmaPDBStatusType
busy_dialog (gint         read_fd,
             gint         write_fd,
             const gchar *message,
             gboolean     cancelable)
{
  Context    context;
  GtkWidget *window;
  GtkWidget *content_area;
  GtkWidget *vbox;
  GtkWidget *label;
  GtkWidget *box;

#ifdef G_OS_WIN32
  context.read_channel  = g_io_channel_win32_new_fd (read_fd);
  context.write_channel = g_io_channel_win32_new_fd (write_fd);
#else
  context.read_channel  = g_io_channel_unix_new (read_fd);
  context.write_channel = g_io_channel_unix_new (write_fd);
#endif

  g_io_channel_set_close_on_unref (context.read_channel,  TRUE);
  g_io_channel_set_close_on_unref (context.write_channel, TRUE);

  /* triggered when the operation is finished in the main app, and we should
   * quit.
   */
  g_io_add_watch (context.read_channel, G_IO_IN | G_IO_ERR | G_IO_HUP,
                  (GIOFunc) busy_dialog_read_channel_notify,
                  &context);

  /* call gtk_init() before ligma_ui_init(), to avoid DESKTOP_STARTUP_ID from
   * taking effect -- we want the dialog to be prominently displayed above
   * other plug-in windows.
   */
  gtk_init (NULL, NULL);

  ligma_ui_init (PLUG_IN_BINARY);

  /* the main window */
  if (! cancelable)
    {
      window = g_object_new (GTK_TYPE_WINDOW,
                             "title",             _("Please Wait"),
                             "skip-taskbar-hint", TRUE,
                             "deletable",         FALSE,
                             "resizable",         FALSE,
                             "role",              "ligma-busy-dialog",
                             "type-hint",         GDK_WINDOW_TYPE_HINT_DIALOG,
                             "window-position",   GTK_WIN_POS_CENTER,
                             NULL);

      g_signal_connect (window, "delete-event", G_CALLBACK (gtk_true), NULL);

      content_area = window;
    }
  else
    {
      window = g_object_new (GTK_TYPE_DIALOG,
                             "title",             _("Please Wait"),
                             "skip-taskbar-hint", TRUE,
                             "resizable",         FALSE,
                             "role",              "ligma-busy-dialog",
                             "window-position",   GTK_WIN_POS_CENTER,
                             NULL);

      gtk_dialog_add_button (GTK_DIALOG (window),
                             _("_Cancel"), GTK_RESPONSE_CANCEL);

      g_signal_connect (window, "delete-event",
                        G_CALLBACK (busy_dialog_delete_event),
                        &context);

      g_signal_connect (window, "response",
                        G_CALLBACK (busy_dialog_response),
                        &context);

      content_area = gtk_dialog_get_content_area (GTK_DIALOG (window));
    }

  /* the main vbox */
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 16);
  gtk_container_add (GTK_CONTAINER (content_area), vbox);
  gtk_widget_show (vbox);

  /* the title label */
  label = gtk_label_new (_("Please wait for the operation to complete"));
  ligma_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* the busy box */
  box = ligma_busy_box_new (message);
  gtk_container_set_border_width (GTK_CONTAINER (box), 8);
  gtk_box_pack_start (GTK_BOX (vbox), box, TRUE, TRUE, 0);
  gtk_widget_show (box);

  gtk_window_present (GTK_WINDOW (window));

  gtk_main ();

  gtk_widget_destroy (window);

  g_clear_pointer (&context.read_channel,  g_io_channel_unref);
  g_clear_pointer (&context.write_channel, g_io_channel_unref);

  return LIGMA_PDB_SUCCESS;
}

static gboolean
busy_dialog_read_channel_notify (GIOChannel   *source,
                                 GIOCondition  condition,
                                 Context      *context)
{
  gtk_main_quit ();

  return FALSE;
}

static gboolean
busy_dialog_delete_event (GtkDialog *dialog,
                          GdkEvent  *event,
                          Context   *context)
{
  gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);

  return TRUE;
}

static void
busy_dialog_response (GtkDialog *dialog,
                      gint       response_id,
                      Context   *context)
{
  switch (response_id)
    {
    case GTK_RESPONSE_CANCEL:
      {
        GtkWidget *button;

        gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_CANCEL, FALSE);

        button = gtk_dialog_get_widget_for_response (dialog,
                                                     GTK_RESPONSE_CANCEL);
        gtk_button_set_label (GTK_BUTTON (button), _("Canceling..."));

        /* signal the cancellation request to the main app */
        g_clear_pointer (&context->write_channel, g_io_channel_unref);
      }
      break;

    default:
      break;
    }
}
