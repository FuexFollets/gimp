/*
 * This is a plug-in for LIGMA.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2003 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#include "config.h"

#include <gtk/gtk.h>

#include "imap_commands.h"

#include "libligma/stdplugins-intl.h"

static void object_up_command_destruct(Command_t *parent);
static CmdExecuteValue_t object_up_command_execute(Command_t *parent);
static void object_up_command_undo(Command_t *parent);

static CommandClass_t object_up_command_class = {
   object_up_command_destruct,
   object_up_command_execute,
   object_up_command_undo,
   NULL                         /* object_up_command_redo */
};

typedef struct {
   Command_t parent;
   ObjectList_t *list;
   Object_t *obj;
} ObjectUpCommand_t;

Command_t*
object_up_command_new(ObjectList_t *list, Object_t *obj)
{
   ObjectUpCommand_t *command = g_new(ObjectUpCommand_t, 1);
   command->list = list;
   command->obj = object_ref(obj);
   return command_init(&command->parent, _("Move Up"),
                       &object_up_command_class);
}

static void
object_up_command_destruct(Command_t *parent)
{
   ObjectUpCommand_t *command = (ObjectUpCommand_t*) parent;
   object_unref(command->obj);
}

static CmdExecuteValue_t
object_up_command_execute(Command_t *parent)
{
   ObjectUpCommand_t *command = (ObjectUpCommand_t*) parent;
   object_list_move_up(command->list, command->obj);
   return CMD_APPEND;
}

static void
object_up_command_undo(Command_t *parent)
{
   ObjectUpCommand_t *command = (ObjectUpCommand_t*) parent;
   object_list_move_down(command->list, command->obj);
}
