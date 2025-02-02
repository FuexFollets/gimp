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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "menus-types.h"

#include "config/ligmaconfig-file.h"
#include "config/ligmaguiconfig.h"

#include "core/ligma.h"

#include "widgets/ligmaactionfactory.h"
#include "widgets/ligmadashboard.h"
#include "widgets/ligmamenufactory.h"

#include "dockable-menu.h"
#include "image-menu.h"
#include "menus.h"
#include "plug-in-menus.h"
#include "tool-options-menu.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   menus_can_change_accels (LigmaGuiConfig   *config);
static void   menus_remove_accels     (gpointer         data,
                                       const gchar     *accel_path,
                                       guint            accel_key,
                                       GdkModifierType  accel_mods,
                                       gboolean         changed);


/*  global variables  */

LigmaMenuFactory * global_menu_factory = NULL;


/*  private variables  */

static gboolean   menurc_deleted      = FALSE;


/*  public functions  */

void
menus_init (Ligma              *ligma,
            LigmaActionFactory *action_factory)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (LIGMA_IS_ACTION_FACTORY (action_factory));
  g_return_if_fail (global_menu_factory == NULL);

  /* We need to make sure the property is installed before using it */
  g_type_class_ref (GTK_TYPE_MENU);

  menus_can_change_accels (LIGMA_GUI_CONFIG (ligma->config));

  g_signal_connect (ligma->config, "notify::can-change-accels",
                    G_CALLBACK (menus_can_change_accels), NULL);

  global_menu_factory = ligma_menu_factory_new (ligma, action_factory);

  ligma_menu_factory_manager_register (global_menu_factory, "<Image>",
                                      "file",
                                      "context",
                                      "debug",
                                      "help",
                                      "edit",
                                      "select",
                                      "view",
                                      "image",
                                      "drawable",
                                      "layers",
                                      "channels",
                                      "vectors",
                                      "tools",
                                      "dialogs",
                                      "windows",
                                      "plug-in",
                                      "filters",
                                      "quick-mask",
                                      NULL,
                                      "/image-menubar",
                                      "image-menu.xml", image_menu_setup,
                                      "/dummy-menubar",
                                      "image-menu.xml", image_menu_setup,
                                      "/quick-mask-popup",
                                      "quick-mask-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Toolbox>",
                                      "file",
                                      "context",
                                      "help",
                                      "edit",
                                      "select",
                                      "view",
                                      "image",
                                      "drawable",
                                      "layers",
                                      "channels",
                                      "vectors",
                                      "tools",
                                      "windows",
                                      "dialogs",
                                      "plug-in",
                                      "filters",
                                      "quick-mask",
                                      NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Dock>",
                                      "file",
                                      "context",
                                      "edit",
                                      "select",
                                      "view",
                                      "image",
                                      "drawable",
                                      "layers",
                                      "channels",
                                      "vectors",
                                      "tools",
                                      "windows",
                                      "dialogs",
                                      "plug-in",
                                      "quick-mask",
                                      "dock",
                                      NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Layers>",
                                      "layers",
                                      "plug-in",
                                      "filters",
                                      NULL,
                                      "/layers-popup",
                                      "layers-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Channels>",
                                      "channels",
                                      "plug-in",
                                      "filters",
                                      NULL,
                                      "/channels-popup",
                                      "channels-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Vectors>",
                                      "vectors",
                                      "plug-in",
                                      NULL,
                                      "/vectors-popup",
                                      "vectors-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<VectorToolPath>",
                                      "vector-toolpath",
                                      NULL,
                                      "/vector-toolpath-popup",
                                      "vector-toolpath-menu.xml",
                                      NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Colormap>",
                                      "colormap",
                                      "plug-in",
                                      NULL,
                                      "/colormap-popup",
                                      "colormap-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Dockable>",
                                      "dockable",
                                      "dock",
                                      NULL,
                                      "/dockable-popup",
                                      "dockable-menu.xml", dockable_menu_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Brushes>",
                                      "brushes",
                                      "plug-in",
                                      NULL,
                                      "/brushes-popup",
                                      "brushes-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Dynamics>",
                                      "dynamics",
                                      "plug-in",
                                      NULL,
                                      "/dynamics-popup",
                                      "dynamics-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<MyPaintBrushes>",
                                      "mypaint-brushes",
                                      "plug-in",
                                      NULL,
                                      "/mypaint-brushes-popup",
                                      "mypaint-brushes-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Patterns>",
                                      "patterns",
                                      "plug-in",
                                      NULL,
                                      "/patterns-popup",
                                      "patterns-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Gradients>",
                                      "gradients",
                                      "plug-in",
                                      NULL,
                                      "/gradients-popup",
                                      "gradients-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Palettes>",
                                      "palettes",
                                      "plug-in",
                                      NULL,
                                      "/palettes-popup",
                                      "palettes-menu.xml", plug_in_menus_setup,
                                      NULL);


  ligma_menu_factory_manager_register (global_menu_factory, "<ToolPresets>",
                                      "tool-presets",
                                      "plug-in",
                                      NULL,
                                      "/tool-presets-popup",
                                      "tool-presets-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Fonts>",
                                      "fonts",
                                      "plug-in",
                                      NULL,
                                      "/fonts-popup",
                                      "fonts-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Buffers>",
                                      "buffers",
                                      "plug-in",
                                      NULL,
                                      "/buffers-popup",
                                      "buffers-menu.xml", plug_in_menus_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Documents>",
                                      "documents",
                                      NULL,
                                      "/documents-popup",
                                      "documents-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Templates>",
                                      "templates",
                                      NULL,
                                      "/templates-popup",
                                      "templates-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Images>",
                                      "images",
                                      NULL,
                                      "/images-popup",
                                      "images-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<BrushEditor>",
                                      "brush-editor",
                                      NULL,
                                      "/brush-editor-popup",
                                      "brush-editor-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<DynamicsEditor>",
                                      "dynamics-editor",
                                      NULL,
                                      "/dynamics-editor-popup",
                                      "dynamics-editor-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<GradientEditor>",
                                      "gradient-editor",
                                      NULL,
                                      "/gradient-editor-popup",
                                      "gradient-editor-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<PaletteEditor>",
                                      "palette-editor",
                                      NULL,
                                      "/palette-editor-popup",
                                      "palette-editor-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<ToolPresetEditor>",
                                      "tool-preset-editor",
                                      NULL,
                                      "/tool-preset-editor-popup",
                                      "tool-preset-editor-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Selection>",
                                      "select",
                                      "vectors",
                                      NULL,
                                      "/selection-popup",
                                      "selection-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<NavigationEditor>",
                                      "view",
                                      NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Undo>",
                                      "edit",
                                      NULL,
                                      "/undo-popup",
                                      "undo-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<ErrorConsole>",
                                      "error-console",
                                      NULL,
                                      "/error-console-popup",
                                      "error-console-menu.xml", NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<ToolOptions>",
                                      "tool-options",
                                      NULL,
                                      "/tool-options-popup",
                                      "tool-options-menu.xml",
                                      tool_options_menu_setup,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<TextEditor>",
                                      "text-editor",
                                      NULL,
                                      "/text-editor-toolbar",
                                      "text-editor-toolbar.xml",
                                      NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<TextTool>",
                                      "text-tool",
                                      NULL,
                                      "/text-tool-popup",
                                      "text-tool-menu.xml",
                                      NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<CursorInfo>",
                                      "cursor-info",
                                      NULL,
                                      "/cursor-info-popup",
                                      "cursor-info-menu.xml",
                                      NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<SamplePoints>",
                                      "sample-points",
                                      NULL,
                                      "/sample-points-popup",
                                      "sample-points-menu.xml",
                                      NULL,
                                      NULL);

  ligma_menu_factory_manager_register (global_menu_factory, "<Dashboard>",
                                      "dashboard",
                                      NULL,
                                      "/dashboard-popup",
                                      "dashboard-menu.xml", ligma_dashboard_menu_setup,
                                       NULL);
}

void
menus_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (global_menu_factory != NULL);

  g_object_unref (global_menu_factory);
  global_menu_factory = NULL;

  g_signal_handlers_disconnect_by_func (ligma->config,
                                        menus_can_change_accels,
                                        NULL);
}

void
menus_restore (Ligma *ligma)
{
  GFile *file;
  gchar *filename;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  file = ligma_directory_file ("menurc", NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  filename = g_file_get_path (file);
  gtk_accel_map_load (filename);
  g_free (filename);

  g_object_unref (file);
}

void
menus_save (Ligma     *ligma,
            gboolean  always_save)
{
  GFile *file;
  gchar *filename;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (menurc_deleted && ! always_save)
    return;

  file = ligma_directory_file ("menurc", NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  filename = g_file_get_path (file);
  gtk_accel_map_save (filename);
  g_free (filename);

  g_object_unref (file);

  menurc_deleted = FALSE;
}

gboolean
menus_clear (Ligma    *ligma,
             GError **error)
{
  GFile    *file;
  GFile    *source;
  gboolean  success  = TRUE;
  GError   *my_error = NULL;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file   = ligma_directory_file ("menurc", NULL);
  source = ligma_sysconf_directory_file ("menurc", NULL);

  if (g_file_copy (source, file, G_FILE_COPY_OVERWRITE,
                   NULL, NULL, NULL, NULL))
    {
      menurc_deleted = TRUE;
    }
  else if (! g_file_delete (file, NULL, &my_error) &&
           my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      g_set_error (error, my_error->domain, my_error->code,
                   _("Deleting \"%s\" failed: %s"),
                   ligma_file_get_utf8_name (file), my_error->message);
      success = FALSE;
    }
  else
    {
      menurc_deleted = TRUE;
    }

  g_clear_error (&my_error);
  g_object_unref (source);
  g_object_unref (file);

  return success;
}

void
menus_remove (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  gtk_accel_map_foreach (ligma, menus_remove_accels);
}


/*  private functions  */

static void
menus_can_change_accels (LigmaGuiConfig *config)
{
  g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                "gtk-can-change-accels", config->can_change_accels,
                NULL);
}

static void
menus_remove_accels (gpointer        data,
                     const gchar    *accel_path,
                     guint           accel_key,
                     GdkModifierType accel_mods,
                     gboolean        changed)
{
  gtk_accel_map_change_entry (accel_path, 0, 0, TRUE);
}
