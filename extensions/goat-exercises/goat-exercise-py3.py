#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#    This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <https://www.gnu.org/licenses/>.

import gi
gi.require_version('Ligma', '3.0')
from gi.repository import Ligma
gi.require_version('LigmaUi', '3.0')
from gi.repository import LigmaUi
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio

import os
import sys

def N_(message): return message
def _(message): return GLib.dgettext(None, message)

class Goat (Ligma.PlugIn):
    ## LigmaPlugIn virtual methods ##
    def do_query_procedures(self):
        return [ "plug-in-goat-exercise-python" ]

    def do_create_procedure(self, name):
        procedure = Ligma.ImageProcedure.new(self, name,
                                       Ligma.PDBProcType.PLUGIN,
                                       self.run, None)

        procedure.set_image_types("*")
        procedure.set_sensitivity_mask (Ligma.ProcedureSensitivityMask.DRAWABLE)

        procedure.set_menu_label(N_("Exercise a goat and a python"))
        procedure.set_icon_name(LigmaUi.ICON_GEGL)
        procedure.add_menu_path('<Image>/Filters/Development/Goat exercises/')

        procedure.set_documentation(N_("Exercise a goat in the Python 3 language"),
                                    N_("Takes a goat for a walk in Python 3"),
                                    name)
        procedure.set_attribution("Jehan", "Jehan", "2019")

        return procedure

    def run(self, procedure, run_mode, image, n_drawables, drawables, args, run_data):
        if n_drawables != 1:
            msg = _("Procedure '{}' only works with one drawable.").format(procedure.get_name())
            error = GLib.Error.new_literal(Ligma.PlugIn.error_quark(), msg, 0)
            return procedure.new_return_values(Ligma.PDBStatusType.CALLING_ERROR, error)
        else:
            drawable = drawables[0]

        if run_mode == Ligma.RunMode.INTERACTIVE:
            gi.require_version('Gtk', '3.0')
            from gi.repository import Gtk
            gi.require_version('Gdk', '3.0')
            from gi.repository import Gdk

            LigmaUi.init("goat-exercise-py3.py")

            dialog = LigmaUi.Dialog(use_header_bar=True,
                                   title=_("Exercise a goat (Python 3)"),
                                   role="goat-exercise-Python3")

            dialog.add_button(_("_Cancel"), Gtk.ResponseType.CANCEL)
            dialog.add_button(_("_Source"), Gtk.ResponseType.APPLY)
            dialog.add_button(_("_OK"), Gtk.ResponseType.OK)

            geometry = Gdk.Geometry()
            geometry.min_aspect = 0.5
            geometry.max_aspect = 1.0
            dialog.set_geometry_hints(None, geometry, Gdk.WindowHints.ASPECT)

            box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
            dialog.get_content_area().add(box)
            box.show()

            # XXX We use printf-style string for sharing the localized
            # string. You may just use recommended Python format() or
            # any style you like in your plug-ins.
            head_text=_("This plug-in is an exercise in '%s' to "
                        "demo plug-in creation.\nCheck out the last "
                        "version of the source code online by clicking "
                        "the \"Source\" button.") % ("Python 3")
            label = Gtk.Label(label=head_text)
            box.pack_start(label, False, False, 1)
            label.show()

            contents = None
            # Get the file contents Python-style instead of using
            # GLib.file_get_contents() which returns bytes result, and
            # when converting to string, get newlines as text contents.
            # Rather than wasting time to figure this out, use Python
            # core API!
            with open(os.path.realpath(__file__), 'r') as f:
                contents = f.read()

            if contents is not None:
                scrolled = Gtk.ScrolledWindow()
                scrolled.set_vexpand (True)
                box.pack_start(scrolled, True, True, 1)
                scrolled.show()

                view = Gtk.TextView()
                view.set_wrap_mode(Gtk.WrapMode.WORD)
                view.set_editable(False)
                buffer = view.get_buffer()
                buffer.set_text(contents, -1)
                scrolled.add(view)
                view.show()

            while (True):
                response = dialog.run()
                if response == Gtk.ResponseType.OK:
                    dialog.destroy()
                    break
                elif response == Gtk.ResponseType.APPLY:
                    url = "https://gitlab.gnome.org/GNOME/ligma/-/blob/master/extensions/goat-exercises/goat-exercise-py3.py"
                    Gio.app_info_launch_default_for_uri(url, None)
                    continue
                else:
                    dialog.destroy()
                    return procedure.new_return_values(Ligma.PDBStatusType.CANCEL,
                                                       GLib.Error())

        intersect, x, y, width, height = drawable.mask_intersect()
        if intersect:
            Gegl.init(None)

            buffer = drawable.get_buffer()
            shadow_buffer = drawable.get_shadow_buffer()

            graph = Gegl.Node()
            input = graph.create_child("gegl:buffer-source")
            input.set_property("buffer", buffer)
            invert = graph.create_child("gegl:invert")
            output = graph.create_child("gegl:write-buffer")
            output.set_property("buffer", shadow_buffer)
            input.link(invert)
            invert.link(output)
            output.process()

            # This is extremely important in bindings, since we don't
            # unref buffers. If we don't explicitly flush a buffer, we
            # may left hanging forever. This step is usually done
            # during an unref().
            shadow_buffer.flush()

            drawable.merge_shadow(True)
            drawable.update(x, y, width, height)
            Ligma.displays_flush()

        return procedure.new_return_values(Ligma.PDBStatusType.SUCCESS, GLib.Error())

Ligma.main(Goat.__gtype__, sys.argv)
