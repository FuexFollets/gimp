#!/usr/bin/env python3
#   Copyright (C) 1997  James Henstridge <james@daa.com.au>
#
#   This program is free software: you can redistribute it and/or modify
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
from gi.repository import GObject
from gi.repository import GLib
from gi.repository import Gio
import time
import sys

def N_(message): return message
def _(message): return GLib.dgettext(None, message)


def foggify(procedure, run_mode, image, n_drawables, drawables, args, data):
    config = procedure.create_config()
    config.begin_run(image, run_mode, args)

    if run_mode == Ligma.RunMode.INTERACTIVE:
        LigmaUi.init('python-fu-foggify')
        dialog = LigmaUi.ProcedureDialog(procedure=procedure, config=config)
        dialog.fill(None)
        if not dialog.run():
            dialog.destroy()
            config.end_run(Ligma.PDBStatusType.CANCEL)
            return procedure.new_return_values(Ligma.PDBStatusType.CANCEL, GLib.Error())
        else:
            dialog.destroy()

    color      = config.get_property('color')
    name       = config.get_property('name')
    turbulence = config.get_property('turbulence')
    opacity    = config.get_property('opacity')

    Ligma.context_push()
    image.undo_group_start()

    if image.get_base_type() is Ligma.ImageBaseType.RGB:
        type = Ligma.ImageType.RGBA_IMAGE
    else:
        type = Ligma.ImageType.GRAYA_IMAGE
    for drawable in drawables:
        fog = Ligma.Layer.new(image, name,
                             drawable.get_width(), drawable.get_height(),
                             type, opacity,
                             Ligma.LayerMode.NORMAL)
        fog.fill(Ligma.FillType.TRANSPARENT)
        image.insert_layer(fog, drawable.get_parent(),
                           image.get_item_position(drawable))

        Ligma.context_set_background(color)
        fog.edit_fill(Ligma.FillType.BACKGROUND)

        # create a layer mask for the new layer
        mask = fog.create_mask(0)
        fog.add_mask(mask)

        # add some clouds to the layer
        Ligma.get_pdb().run_procedure('plug-in-plasma', [
            GObject.Value(Ligma.RunMode, Ligma.RunMode.NONINTERACTIVE),
            GObject.Value(Ligma.Image, image),
            GObject.Value(Ligma.Drawable, mask),
            GObject.Value(GObject.TYPE_INT, int(time.time())),
            GObject.Value(GObject.TYPE_DOUBLE, turbulence),
        ])

        # apply the clouds to the layer
        fog.remove_mask(Ligma.MaskApplyMode.APPLY)
        fog.set_visible(True)

    Ligma.displays_flush()

    image.undo_group_end()
    Ligma.context_pop()

    config.end_run(Ligma.PDBStatusType.SUCCESS)

    return procedure.new_return_values(Ligma.PDBStatusType.SUCCESS, GLib.Error())

_color = Ligma.RGB()
_color.set(240.0, 0, 0)

class Foggify (Ligma.PlugIn):
    ## Parameters ##
    __gproperties__ = {
        "name": (str,
                 _("Layer _name"),
                 _("Layer name"),
                 _("Clouds"),
                 GObject.ParamFlags.READWRITE),
        "turbulence": (float,
                       _("_Turbulence"),
                       _("Turbulence"),
                       0.0, 7.0, 1.0,
                       GObject.ParamFlags.READWRITE),
        "opacity": (float,
                    _("O_pacity"),
                    _("Opacity"),
                    0.0, 100.0, 100.0,
                    GObject.ParamFlags.READWRITE),
    }
    # I use a different syntax for this property because I think it is
    # supposed to allow setting a default, except it doesn't seem to
    # work. I still leave it this way for now until we figure this out
    # as it should be the better syntax.
    color = GObject.Property(type =Ligma.RGB, default=_color,
                             nick =_("_Fog color"),
                             blurb=_("Fog color"))

    ## LigmaPlugIn virtual methods ##
    def do_set_i18n(self, procname):
        return True, 'ligma30-python', None

    def do_query_procedures(self):
        return [ 'python-fu-foggify' ]

    def do_create_procedure(self, name):
        procedure = Ligma.ImageProcedure.new(self, name,
                                            Ligma.PDBProcType.PLUGIN,
                                            foggify, None)
        procedure.set_image_types("RGB*, GRAY*");
        procedure.set_sensitivity_mask (Ligma.ProcedureSensitivityMask.DRAWABLE |
                                        Ligma.ProcedureSensitivityMask.DRAWABLES)
        procedure.set_documentation (_("Add a layer of fog"),
                                     _("Adds a layer of fog to the image."),
                                     name)
        procedure.set_menu_label(_("_Fog..."))
        procedure.set_attribution("James Henstridge",
                                  "James Henstridge",
                                  "1999,2007")
        procedure.add_menu_path ("<Image>/Filters/Decor")

        procedure.add_argument_from_property(self, "name")
        procedure.add_argument_from_property(self, "color")
        procedure.add_argument_from_property(self, "turbulence")
        procedure.add_argument_from_property(self, "opacity")
        return procedure

Ligma.main(Foggify.__gtype__, sys.argv)
