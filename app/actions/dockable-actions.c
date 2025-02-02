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
#include <string.h>

#include <gtk/gtk.h>

#include "libligmawidgets/ligmawidgets.h"

#include "actions-types.h"

#include "widgets/ligmaactiongroup.h"
#include "widgets/ligmacontainerview.h"
#include "widgets/ligmacontainerview-utils.h"
#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadockable.h"
#include "widgets/ligmadockbook.h"
#include "widgets/ligmadocked.h"
#include "widgets/ligmahelp-ids.h"

#include "dialogs-actions.h"
#include "dockable-actions.h"
#include "dockable-commands.h"

#include "ligma-intl.h"


static const LigmaActionEntry dockable_actions[] =
{
  { "dockable-popup", NULL,
    NC_("dockable-action", "Dialogs Menu"), NULL, NULL, NULL,
    LIGMA_HELP_DOCK },

  { "dockable-menu",              "image-missing", ""           },
  { "dockable-add-tab-menu",      NULL, NC_("dockable-action",
                                            "_Add Tab")        },
  { "dockable-preview-size-menu", NULL, NC_("dockable-action",
                                            "_Preview Size")   },
  { "dockable-tab-style-menu",    NULL, NC_("dockable-action",
                                            "_Tab Style")      },

  { "dockable-close-tab", "window-close",
    NC_("dockable-action", "_Close Tab"), "", NULL,
    dockable_close_tab_cmd_callback,
    LIGMA_HELP_DOCK_TAB_CLOSE },

  { "dockable-detach-tab", LIGMA_ICON_DETACH,
    NC_("dockable-action", "_Detach Tab"), "", NULL,
    dockable_detach_tab_cmd_callback,
    LIGMA_HELP_DOCK_TAB_DETACH }
};

#define VIEW_SIZE(action,label,size) \
  { "dockable-preview-size-" action, NULL, \
    (label), NULL, NULL, \
    (size), \
    LIGMA_HELP_DOCK_PREVIEW_SIZE }
#define TAB_STYLE(action,label,style) \
  { "dockable-tab-style-" action, NULL, \
    (label), NULL, NULL, \
    (style), \
    LIGMA_HELP_DOCK_TAB_STYLE }

static const LigmaRadioActionEntry dockable_view_size_actions[] =
{
  VIEW_SIZE ("tiny",
             NC_("preview-size", "_Tiny"),        LIGMA_VIEW_SIZE_TINY),
  VIEW_SIZE ("extra-small",
             NC_("preview-size", "E_xtra Small"), LIGMA_VIEW_SIZE_EXTRA_SMALL),
  VIEW_SIZE ("small",
             NC_("preview-size", "_Small"),       LIGMA_VIEW_SIZE_SMALL),
  VIEW_SIZE ("medium",
             NC_("preview-size", "_Medium"),      LIGMA_VIEW_SIZE_MEDIUM),
  VIEW_SIZE ("large",
             NC_("preview-size", "_Large"),       LIGMA_VIEW_SIZE_LARGE),
  VIEW_SIZE ("extra-large",
             NC_("preview-size", "Ex_tra Large"), LIGMA_VIEW_SIZE_EXTRA_LARGE),
  VIEW_SIZE ("huge",
             NC_("preview-size", "_Huge"),        LIGMA_VIEW_SIZE_HUGE),
  VIEW_SIZE ("enormous",
             NC_("preview-size", "_Enormous"),    LIGMA_VIEW_SIZE_ENORMOUS),
  VIEW_SIZE ("gigantic",
             NC_("preview-size", "_Gigantic"),    LIGMA_VIEW_SIZE_GIGANTIC)
};

static const LigmaRadioActionEntry dockable_tab_style_actions[] =
{
  TAB_STYLE ("icon",
             NC_("tab-style", "_Icon"),           LIGMA_TAB_STYLE_ICON),
  TAB_STYLE ("preview",
             NC_("tab-style", "Current _Status"), LIGMA_TAB_STYLE_PREVIEW),
  TAB_STYLE ("name",
             NC_("tab-style", "_Text"),           LIGMA_TAB_STYLE_NAME),
  TAB_STYLE ("icon-name",
             NC_("tab-style", "I_con & Text"),    LIGMA_TAB_STYLE_ICON_NAME),
  TAB_STYLE ("preview-name",
             NC_("tab-style", "St_atus & Text"),  LIGMA_TAB_STYLE_PREVIEW_NAME)
};

#undef VIEW_SIZE
#undef TAB_STYLE


static const LigmaToggleActionEntry dockable_toggle_actions[] =
{
  { "dockable-lock-tab", NULL,
    NC_("dockable-action", "Loc_k Tab to Dock"), NULL,
    NC_("dockable-action",
        "Protect this tab from being dragged with the mouse pointer"),
    dockable_lock_tab_cmd_callback,
    FALSE,
    LIGMA_HELP_DOCK_TAB_LOCK },

  { "dockable-show-button-bar", NULL,
    NC_("dockable-action", "Show _Button Bar"), NULL, NULL,
    dockable_show_button_bar_cmd_callback,
    TRUE,
    LIGMA_HELP_DOCK_SHOW_BUTTON_BAR }
};

static const LigmaRadioActionEntry dockable_view_type_actions[] =
{
  { "dockable-view-type-list", NULL,
    NC_("dockable-action", "View as _List"), NULL, NULL,
    LIGMA_VIEW_TYPE_LIST,
    LIGMA_HELP_DOCK_VIEW_AS_LIST },

  { "dockable-view-type-grid", NULL,
    NC_("dockable-action", "View as _Grid"), NULL, NULL,
    LIGMA_VIEW_TYPE_GRID,
    LIGMA_HELP_DOCK_VIEW_AS_GRID }
};


void
dockable_actions_setup (LigmaActionGroup *group)
{
  ligma_action_group_add_actions (group, "dockable-action",
                                 dockable_actions,
                                 G_N_ELEMENTS (dockable_actions));

  ligma_action_group_add_toggle_actions (group, "dockable-action",
                                        dockable_toggle_actions,
                                        G_N_ELEMENTS (dockable_toggle_actions));

  ligma_action_group_add_string_actions (group, "dialogs-action",
                                        dialogs_dockable_actions,
                                        n_dialogs_dockable_actions,
                                        dockable_add_tab_cmd_callback);

  ligma_action_group_add_radio_actions (group, "preview-size",
                                       dockable_view_size_actions,
                                       G_N_ELEMENTS (dockable_view_size_actions),
                                       NULL,
                                       LIGMA_VIEW_SIZE_MEDIUM,
                                       dockable_view_size_cmd_callback);

  ligma_action_group_add_radio_actions (group, "tab-style",
                                       dockable_tab_style_actions,
                                       G_N_ELEMENTS (dockable_tab_style_actions),
                                       NULL,
                                       LIGMA_TAB_STYLE_PREVIEW,
                                       dockable_tab_style_cmd_callback);

  ligma_action_group_add_radio_actions (group, "dockable-action",
                                       dockable_view_type_actions,
                                       G_N_ELEMENTS (dockable_view_type_actions),
                                       NULL,
                                       LIGMA_VIEW_TYPE_LIST,
                                       dockable_toggle_view_cmd_callback);
}

void
dockable_actions_update (LigmaActionGroup *group,
                         gpointer         data)
{
  LigmaDockable           *dockable;
  LigmaDockbook           *dockbook;
  LigmaDocked             *docked;
  LigmaDock               *dock;
  LigmaDialogFactoryEntry *entry;
  LigmaContainerView      *view;
  LigmaViewType            view_type           = -1;
  gboolean                list_view_available = FALSE;
  gboolean                grid_view_available = FALSE;
  gboolean                locked              = FALSE;
  LigmaViewSize            view_size           = -1;
  LigmaTabStyle            tab_style           = -1;
  gint                    n_pages             = 0;
  gint                    n_books             = 0;
  LigmaDockedInterface     *docked_iface       = NULL;

  if (LIGMA_IS_DOCKBOOK (data))
    {
      gint page_num;

      dockbook = LIGMA_DOCKBOOK (data);

      page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (dockbook));

      dockable = (LigmaDockable *)
        gtk_notebook_get_nth_page (GTK_NOTEBOOK (dockbook), page_num);
    }
  else if (LIGMA_IS_DOCKABLE (data))
    {
      dockable = LIGMA_DOCKABLE (data);
      dockbook = ligma_dockable_get_dockbook (dockable);
    }
  else
    {
      return;
    }

  docked = LIGMA_DOCKED (gtk_bin_get_child (GTK_BIN (dockable)));
  dock   = ligma_dockbook_get_dock (dockbook);


  ligma_dialog_factory_from_widget (GTK_WIDGET (dockable), &entry);

  if (entry)
    {
      gchar *identifier;
      gchar *substring = NULL;

      identifier = g_strdup (entry->identifier);

      if ((substring = strstr (identifier, "grid")))
        view_type = LIGMA_VIEW_TYPE_GRID;
      else if ((substring = strstr (identifier, "list")))
        view_type = LIGMA_VIEW_TYPE_LIST;

      if (substring)
        {
          memcpy (substring, "list", 4);
          if (ligma_dialog_factory_find_entry (ligma_dock_get_dialog_factory (dock),
                                              identifier))
            list_view_available = TRUE;

          memcpy (substring, "grid", 4);
          if (ligma_dialog_factory_find_entry (ligma_dock_get_dialog_factory (dock),
                                              identifier))
            grid_view_available = TRUE;
        }

      g_free (identifier);
    }

  view = ligma_container_view_get_by_dockable (dockable);

  if (view)
    view_size = ligma_container_view_get_view_size (view, NULL);

  tab_style = ligma_dockable_get_tab_style (dockable);

  n_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (dockbook));
  n_books = g_list_length (ligma_dock_get_dockbooks (dock));

#define SET_ACTIVE(action,active) \
        ligma_action_group_set_action_active (group, action, (active) != 0)
#define SET_VISIBLE(action,active) \
        ligma_action_group_set_action_visible (group, action, (active) != 0)
#define SET_SENSITIVE(action,sensitive) \
        ligma_action_group_set_action_sensitive (group, action, (sensitive) != 0, NULL)


  locked = ligma_dockable_get_locked (dockable);

  SET_SENSITIVE ("dockable-detach-tab", (! locked &&
                                         (n_pages > 1 || n_books > 1)));

  SET_ACTIVE ("dockable-lock-tab", locked);

  SET_VISIBLE ("dockable-preview-size-menu", view_size != -1);

  if (view_size != -1)
    {
      if (view_size >= LIGMA_VIEW_SIZE_GIGANTIC)
        {
          SET_ACTIVE ("dockable-preview-size-gigantic", TRUE);
        }
      else if (view_size >= LIGMA_VIEW_SIZE_ENORMOUS)
        {
          SET_ACTIVE ("dockable-preview-size-enormous", TRUE);
        }
      else if (view_size >= LIGMA_VIEW_SIZE_HUGE)
        {
          SET_ACTIVE ("dockable-preview-size-huge", TRUE);
        }
      else if (view_size >= LIGMA_VIEW_SIZE_EXTRA_LARGE)
        {
          SET_ACTIVE ("dockable-preview-size-extra-large", TRUE);
        }
      else if (view_size >= LIGMA_VIEW_SIZE_LARGE)
        {
          SET_ACTIVE ("dockable-preview-size-large", TRUE);
        }
      else if (view_size >= LIGMA_VIEW_SIZE_MEDIUM)
        {
          SET_ACTIVE ("dockable-preview-size-medium", TRUE);
        }
      else if (view_size >= LIGMA_VIEW_SIZE_SMALL)
        {
          SET_ACTIVE ("dockable-preview-size-small", TRUE);
        }
      else if (view_size >= LIGMA_VIEW_SIZE_EXTRA_SMALL)
        {
          SET_ACTIVE ("dockable-preview-size-extra-small", TRUE);
        }
      else if (view_size >= LIGMA_VIEW_SIZE_TINY)
        {
          SET_ACTIVE ("dockable-preview-size-tiny", TRUE);
        }
    }

  if (tab_style == LIGMA_TAB_STYLE_ICON)
    SET_ACTIVE ("dockable-tab-style-icon", TRUE);
  else if (tab_style == LIGMA_TAB_STYLE_PREVIEW)
    SET_ACTIVE ("dockable-tab-style-preview", TRUE);
  else if (tab_style == LIGMA_TAB_STYLE_NAME)
    SET_ACTIVE ("dockable-tab-style-name", TRUE);
  else if (tab_style == LIGMA_TAB_STYLE_ICON_NAME)
    SET_ACTIVE ("dockable-tab-style-icon-name", TRUE);
  else if (tab_style == LIGMA_TAB_STYLE_PREVIEW_NAME)
    SET_ACTIVE ("dockable-tab-style-preview-name", TRUE);

  docked_iface = LIGMA_DOCKED_GET_IFACE (docked);
  SET_SENSITIVE ("dockable-tab-style-preview",
                 docked_iface->get_preview);
  SET_SENSITIVE ("dockable-tab-style-preview-name",
                 docked_iface->get_preview);

  SET_VISIBLE ("dockable-view-type-grid", view_type != -1);
  SET_VISIBLE ("dockable-view-type-list", view_type != -1);

  if (view_type != -1)
    {
      if (view_type == LIGMA_VIEW_TYPE_LIST)
        SET_ACTIVE ("dockable-view-type-list", TRUE);
      else
        SET_ACTIVE ("dockable-view-type-grid", TRUE);

      SET_SENSITIVE ("dockable-view-type-grid", grid_view_available);
      SET_SENSITIVE ("dockable-view-type-list", list_view_available);
    }

  SET_VISIBLE ("dockable-show-button-bar", ligma_docked_has_button_bar (docked));
  SET_ACTIVE ("dockable-show-button-bar",
              ligma_docked_get_show_button_bar (docked));

#undef SET_ACTIVE
#undef SET_VISIBLE
#undef SET_SENSITIVE
}
