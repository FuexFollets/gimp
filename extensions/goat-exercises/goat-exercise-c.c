/*
 *  Goat exercise plug-in by Øyvind Kolås, pippin@ligma.org
 */

/*
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

#define LIGMA_DISABLE_COMPAR_CRUFT

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "libligma/stdplugins-intl.h"


#define PLUG_IN_BINARY "goat-exercise-c"
#define PLUG_IN_SOURCE PLUG_IN_BINARY ".c"
#define PLUG_IN_PROC   "plug-in-goat-exercise-c"
#define PLUG_IN_ROLE   "goat-exercise-c"

#define GOAT_URI       "https://gitlab.gnome.org/GNOME/ligma/blob/master/extensions/goat-exercises/goat-exercise-c.c"


typedef struct _Goat      Goat;
typedef struct _GoatClass GoatClass;

struct _Goat
{
  LigmaPlugIn      parent_instance;
};

struct _GoatClass
{
  LigmaPlugInClass parent_class;
};


/* Declare local functions.
 */

#define GOAT_TYPE  (goat_get_type ())
#define GOAT (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOAT_TYPE, Goat))

GType                   goat_get_type         (void) G_GNUC_CONST;

static GList          * goat_query_procedures (LigmaPlugIn           *plug_in);
static LigmaProcedure  * goat_create_procedure (LigmaPlugIn           *plug_in,
                                               const gchar          *name);

static LigmaValueArray * goat_run              (LigmaProcedure        *procedure,
                                               LigmaRunMode           run_mode,
                                               LigmaImage            *image,
                                               gint                  n_drawables,
                                               LigmaDrawable        **drawables,
                                               const LigmaValueArray *args,
                                               gpointer              run_data);


G_DEFINE_TYPE (Goat, goat, LIGMA_TYPE_PLUG_IN)

LIGMA_MAIN (GOAT_TYPE)


static void
goat_class_init (GoatClass *klass)
{
  LigmaPlugInClass *plug_in_class = LIGMA_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = goat_query_procedures;
  plug_in_class->create_procedure = goat_create_procedure;
}

static void
goat_init (Goat *goat)
{
}

static GList *
goat_query_procedures (LigmaPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static LigmaProcedure *
goat_create_procedure (LigmaPlugIn  *plug_in,
                       const gchar *name)
{
  LigmaProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = ligma_image_procedure_new (plug_in, name,
                                            LIGMA_PDB_PROC_TYPE_PLUGIN,
                                            goat_run, NULL, NULL);

      ligma_procedure_set_image_types (procedure, "*");
      ligma_procedure_set_sensitivity_mask (procedure,
                                           LIGMA_PROCEDURE_SENSITIVE_DRAWABLE);

      ligma_procedure_set_menu_label (procedure, N_("Exercise in _C minor"));
      ligma_procedure_set_icon_name (procedure, LIGMA_ICON_GEGL);
      ligma_procedure_add_menu_path (procedure,
                                    "<Image>/Filters/Development/Goat exercises/");

      ligma_procedure_set_documentation (procedure,
                                        N_("Exercise a goat in the C language"),
                                        "Takes a goat for a walk",
                                        PLUG_IN_PROC);
      ligma_procedure_set_attribution (procedure,
                                      "Øyvind Kolås <pippin@ligma.org>",
                                      "Øyvind Kolås <pippin@ligma.org>",
                                      "21 march 2012");
    }

  return procedure;
}

static LigmaValueArray *
goat_run (LigmaProcedure        *procedure,
          LigmaRunMode           run_mode,
          LigmaImage            *image,
          gint                  n_drawables,
          LigmaDrawable        **drawables,
          const LigmaValueArray *args,
          gpointer              run_data)
{
  LigmaDrawable      *drawable;
  LigmaPDBStatusType  status = LIGMA_PDB_SUCCESS;
  gint               x, y, width, height;

  if (n_drawables != 1)
    {
      GError *error = NULL;

      g_set_error (&error, LIGMA_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return ligma_procedure_new_return_values (procedure,
                                               LIGMA_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  /* In interactive mode, display a dialog to advertise the exercise. */
  if (run_mode == LIGMA_RUN_INTERACTIVE)
    {
      GtkTextBuffer    *buffer;
      GtkWidget        *dialog;
      GtkWidget        *box;
      GtkWidget        *widget;
      GtkWidget        *scrolled;
      GFile            *file;
      GFileInputStream *input;
      gchar            *head_text;
      gchar            *path;
      GdkGeometry       geometry;
      gchar             source_text[4096];
      gssize            read;
      gint              response;

      ligma_ui_init (PLUG_IN_BINARY);
      dialog = ligma_dialog_new (_("Exercise a goat (C)"), PLUG_IN_ROLE,
                                NULL, GTK_DIALOG_USE_HEADER_BAR,
                                ligma_standard_help_func, PLUG_IN_PROC,

                                _("_Cancel"), GTK_RESPONSE_CANCEL,
                                _("_Source"), GTK_RESPONSE_APPLY,
                                _("_Run"),    GTK_RESPONSE_OK,

                                NULL);
      geometry.min_aspect = 0.5;
      geometry.max_aspect = 1.0;
      gtk_window_set_geometry_hints (GTK_WINDOW (dialog), NULL,
                                     &geometry, GDK_HINT_ASPECT);

      box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
      gtk_container_set_border_width (GTK_CONTAINER (box), 12);
      gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                         box);
      gtk_widget_show (box);

      head_text = g_strdup_printf (_("This plug-in is an exercise in '%s' "
                                     "to demo plug-in creation.\n"
                                     "Check out the last version "
                                     "of the source code online by "
                                     "clicking the \"Source\" button."),
                                   "C");
      widget = gtk_label_new (head_text);
      g_free (head_text);
      gtk_box_pack_start (GTK_BOX (box), widget, FALSE, FALSE, 1);
      gtk_widget_show (widget);

      scrolled = gtk_scrolled_window_new (NULL, NULL);
      gtk_box_pack_start (GTK_BOX (box), scrolled, TRUE, TRUE, 1);
      gtk_widget_set_vexpand (GTK_WIDGET (scrolled), TRUE);
      gtk_widget_show (scrolled);

      path = g_build_filename (ligma_plug_in_directory (), "extensions",
                               "org.ligma.extension.goat-exercises", PLUG_IN_SOURCE,
                               NULL);
      file = g_file_new_for_path (path);
      g_free (path);

      widget = gtk_text_view_new ();
      gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (widget), GTK_WRAP_WORD);
      gtk_text_view_set_editable (GTK_TEXT_VIEW (widget), FALSE);
      buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));

      input = g_file_read (file, NULL, NULL);

      while ((read = g_input_stream_read (G_INPUT_STREAM (input),
                                          source_text, 4096, NULL, NULL)) &&
             read != -1)
        {
          gtk_text_buffer_insert_at_cursor (buffer, source_text, read);
        }

      g_object_unref (file);

      gtk_container_add (GTK_CONTAINER (scrolled), widget);
      gtk_widget_show (widget);

      while ((response = ligma_dialog_run (LIGMA_DIALOG (dialog))))
        {
          if (response == GTK_RESPONSE_OK)
            {
              gtk_widget_destroy (dialog);
              break;
            }
          else if (response == GTK_RESPONSE_APPLY)
            {
              /* Show the code. */
              g_app_info_launch_default_for_uri (GOAT_URI, NULL, NULL);
              continue;
            }
          else /* CANCEL, CLOSE, DELETE_EVENT */
            {
              gtk_widget_destroy (dialog);
              return ligma_procedure_new_return_values (procedure,
                                                       LIGMA_PDB_CANCEL,
                                                       NULL);
            }
        }
    }

  if (ligma_drawable_mask_intersect (drawable, &x, &y, &width, &height))
    {
      GeglBuffer *buffer;
      GeglBuffer *shadow_buffer;

      gegl_init (NULL, NULL);

      buffer        = ligma_drawable_get_buffer (drawable);
      shadow_buffer = ligma_drawable_get_shadow_buffer (drawable);

      gegl_render_op (buffer, shadow_buffer, "gegl:invert", NULL);

      g_object_unref (shadow_buffer); /* flushes the shadow tiles */
      g_object_unref (buffer);

      ligma_drawable_merge_shadow (drawable, TRUE);
      ligma_drawable_update (drawable, x, y, width, height);
      ligma_displays_flush ();

      gegl_exit ();
    }

  return ligma_procedure_new_return_values (procedure, status, NULL);
}
