/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmasessioninfo-dock.c
 * Copyright (C) 2001-2007 Michael Natterer <mitch@ligma.org>
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

#include <string.h>

#include <gtk/gtk.h>

#include "libligmaconfig/ligmaconfig.h"

#include "widgets-types.h"

#include "ligmadialogfactory.h"
#include "ligmadock.h"
#include "ligmadockbook.h"
#include "ligmadockcontainer.h"
#include "ligmadockwindow.h"
#include "ligmasessioninfo.h"
#include "ligmasessioninfo-aux.h"
#include "ligmasessioninfo-book.h"
#include "ligmasessioninfo-dock.h"
#include "ligmasessioninfo-private.h"
#include "ligmatoolbox.h"


enum
{
  SESSION_INFO_SIDE,
  SESSION_INFO_POSITION,
  SESSION_INFO_BOOK
};


static LigmaAlignmentType ligma_session_info_dock_get_side (LigmaDock *dock);


static LigmaAlignmentType
ligma_session_info_dock_get_side (LigmaDock *dock)
{
  LigmaAlignmentType result   = -1;
  GtkWidget        *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (dock));

  if (LIGMA_IS_DOCK_CONTAINER (toplevel))
    {
      LigmaDockContainer *container = LIGMA_DOCK_CONTAINER (toplevel);

      result = ligma_dock_container_get_dock_side (container, dock);
    }

  return result;
}


/*  public functions  */

LigmaSessionInfoDock *
ligma_session_info_dock_new (const gchar *dock_type)
{
  LigmaSessionInfoDock *dock_info = NULL;

  dock_info = g_slice_new0 (LigmaSessionInfoDock);
  dock_info->dock_type = g_strdup (dock_type);
  dock_info->side      = -1;

  return dock_info;
}

void
ligma_session_info_dock_free (LigmaSessionInfoDock *dock_info)
{
  g_return_if_fail (dock_info != NULL);

  g_clear_pointer (&dock_info->dock_type, g_free);

  if (dock_info->books)
    {
      g_list_free_full (dock_info->books,
                        (GDestroyNotify) ligma_session_info_book_free);
      dock_info->books = NULL;
    }

  g_slice_free (LigmaSessionInfoDock, dock_info);
}

void
ligma_session_info_dock_serialize (LigmaConfigWriter    *writer,
                                  LigmaSessionInfoDock *dock_info)
{
  GList *list;

  g_return_if_fail (writer != NULL);
  g_return_if_fail (dock_info != NULL);

  ligma_config_writer_open (writer, dock_info->dock_type);

  if (dock_info->side != -1)
    {
      const char *side_text =
        dock_info->side == LIGMA_ALIGN_LEFT ? "left" : "right";

      ligma_config_writer_open (writer, "side");
      ligma_config_writer_print (writer, side_text, strlen (side_text));
      ligma_config_writer_close (writer);
    }

  if (dock_info->position != 0)
    {
      gint position;

      position = ligma_session_info_apply_position_accuracy (dock_info->position);

      ligma_config_writer_open (writer, "position");
      ligma_config_writer_printf (writer, "%d", position);
      ligma_config_writer_close (writer);
    }

  for (list = dock_info->books; list; list = g_list_next (list))
    ligma_session_info_book_serialize (writer, list->data);

  ligma_config_writer_close (writer);
}

GTokenType
ligma_session_info_dock_deserialize (GScanner             *scanner,
                                    gint                  scope,
                                    LigmaSessionInfoDock **dock_info,
                                    const gchar          *dock_type)
{
  GTokenType token;

  g_return_val_if_fail (scanner != NULL, G_TOKEN_LEFT_PAREN);
  g_return_val_if_fail (dock_info != NULL, G_TOKEN_LEFT_PAREN);

  g_scanner_scope_add_symbol (scanner, scope, "side",
                              GINT_TO_POINTER (SESSION_INFO_SIDE));
  g_scanner_scope_add_symbol (scanner, scope, "position",
                              GINT_TO_POINTER (SESSION_INFO_POSITION));
  g_scanner_scope_add_symbol (scanner, scope, "book",
                              GINT_TO_POINTER (SESSION_INFO_BOOK));

  *dock_info = ligma_session_info_dock_new (dock_type);

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
              LigmaSessionInfoBook *book;

            case SESSION_INFO_SIDE:
              token = G_TOKEN_IDENTIFIER;
              if (g_scanner_peek_next_token (scanner) != token)
                break;

              g_scanner_get_next_token (scanner);

              if (strcmp ("left", scanner->value.v_identifier) == 0)
                (*dock_info)->side = LIGMA_ALIGN_LEFT;
              else
                (*dock_info)->side = LIGMA_ALIGN_RIGHT;
              break;

            case SESSION_INFO_POSITION:
              token = G_TOKEN_INT;
              if (! ligma_scanner_parse_int (scanner, &((*dock_info)->position)))
                (*dock_info)->position = 0;
              break;

            case SESSION_INFO_BOOK:
              g_scanner_set_scope (scanner, scope + 1);
              token = ligma_session_info_book_deserialize (scanner, scope + 1,
                                                          &book);

              if (token == G_TOKEN_LEFT_PAREN)
                {
                  (*dock_info)->books = g_list_append ((*dock_info)->books, book);
                  g_scanner_set_scope (scanner, scope);
                }
              else
                return token;

              break;

            default:
              return token;
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  g_scanner_scope_remove_symbol (scanner, scope, "book");
  g_scanner_scope_remove_symbol (scanner, scope, "position");
  g_scanner_scope_remove_symbol (scanner, scope, "side");

  return token;
}

LigmaSessionInfoDock *
ligma_session_info_dock_from_widget (LigmaDock *dock)
{
  LigmaSessionInfoDock *dock_info;
  GList               *list;
  GtkWidget           *parent;

  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  dock_info = ligma_session_info_dock_new (LIGMA_IS_TOOLBOX (dock) ?
                                          "ligma-toolbox" :
                                          "ligma-dock");

  for (list = ligma_dock_get_dockbooks (dock); list; list = g_list_next (list))
    {
      LigmaSessionInfoBook *book;

      book = ligma_session_info_book_from_widget (list->data);

      dock_info->books = g_list_prepend (dock_info->books, book);
    }

  dock_info->books = g_list_reverse (dock_info->books);
  dock_info->side  = ligma_session_info_dock_get_side (dock);

  parent = gtk_widget_get_parent (GTK_WIDGET (dock));

  if (GTK_IS_PANED (parent))
    {
      GtkPaned *paned = GTK_PANED (parent);

      if (GTK_WIDGET (dock) == gtk_paned_get_child2 (paned))
        dock_info->position = gtk_paned_get_position (paned);
    }

  return dock_info;
}

LigmaDock *
ligma_session_info_dock_restore (LigmaSessionInfoDock *dock_info,
                                LigmaDialogFactory   *factory,
                                GdkMonitor          *monitor,
                                LigmaDockContainer   *dock_container)
{
  gint           n_books = 0;
  GtkWidget     *dock;
  GList         *iter;
  LigmaUIManager *ui_manager;

  g_return_val_if_fail (LIGMA_IS_DIALOG_FACTORY (factory), NULL);
  g_return_val_if_fail (GDK_IS_MONITOR (monitor), NULL);

  ui_manager = ligma_dock_container_get_ui_manager (dock_container);
  dock       = ligma_dialog_factory_dialog_new (factory, monitor,
                                               ui_manager,
                                               NULL,
                                               dock_info->dock_type,
                                               -1 /*view_size*/,
                                               FALSE /*present*/);

  g_return_val_if_fail (LIGMA_IS_DOCK (dock), NULL);

  /* Add the dock to the dock window immediately so the stuff in the
   * dock has access to e.g. a dialog factory
   */
  ligma_dock_container_add_dock (dock_container,
                                LIGMA_DOCK (dock),
                                dock_info);

  /* Note that if it is a toolbox, we will get here even though we
   * don't have any books
   */
  for (iter = dock_info->books;
       iter;
       iter = g_list_next (iter))
    {
      LigmaSessionInfoBook *book_info = iter->data;
      GtkWidget           *dockbook;

      dockbook = GTK_WIDGET (ligma_session_info_book_restore (book_info,
                                                             LIGMA_DOCK (dock)));

      if (dockbook)
        {
          GtkWidget *parent = gtk_widget_get_parent (dockbook);

          n_books++;

          if (GTK_IS_PANED (parent))
            {
              GtkPaned *paned = GTK_PANED (parent);

              if (dockbook == gtk_paned_get_child2 (paned))
                gtk_paned_set_position (paned, book_info->position);
            }
        }
    }

  /* Now remove empty dockbooks from the list, check the comment in
   * ligma_session_info_book_restore() which explains why the dock
   * can contain empty dockbooks at all
   */
  if (dock_info->books)
    {
      GList *books;

      books = g_list_copy (ligma_dock_get_dockbooks (LIGMA_DOCK (dock)));

      while (books)
        {
          GtkContainer *dockbook = books->data;
          GList        *children = gtk_container_get_children (dockbook);

          if (children)
            {
              g_list_free (children);
            }
          else
            {
              g_object_ref (dockbook);
              ligma_dock_remove_book (LIGMA_DOCK (dock), LIGMA_DOCKBOOK (dockbook));
              gtk_widget_destroy (GTK_WIDGET (dockbook));
              g_object_unref (dockbook);

              n_books--;
            }

          books = g_list_remove (books, dockbook);
        }
    }

  /*  if we removed all books again, the dock was destroyed, so bail out  */
  if (dock_info->books && n_books == 0)
    {
      return NULL;
    }

  gtk_widget_show (dock);

  return LIGMA_DOCK (dock);
}
