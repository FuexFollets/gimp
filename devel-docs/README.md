---
title: Developers documentation
---

This manual holds information that you will find useful if you
develop a LIGMA plug-in or want to contribute to the LIGMA core.

People only interested into plug-ins can probably read just the
[Plug-in development](#plug-in-development) section. If you wish to
contribute to all parts of LIGMA, the whole documentation is of interest.

[TOC]

## Plug-in development
### Concepts
#### Basics

Plug-ins in LIGMA are executables which LIGMA can call upon certain
conditions. Since they are separate executables, it means that they are
run as their own process, making the plug-in infrastructure very robust.
No plug-in should ever crash LIGMA, even with the worst bugs. If such
thing happens, you can consider this a core bug.

On the other hand, a plug-in can mess your opened files, so a badly
developed plug-in could still leave your opened images in an undesirable
state. If this happens, you'd be advised to close and reopen the file
(provided you saved recently).

Another downside of plug-ins is that LIGMA currently doesn't have any
sandboxing ability. Since we explained that plug-ins are run by LIGMA as
independant processes, it also means they have the same rights as your
LIGMA process. Therefore be careful that you trust the source of your
plug-ins. You should never run shady plug-ins from untrusted sources.

LIGMA comes itself with a lot of plug-ins. Actually nearly all file
format support is implemented as a plug-in (XCF support being the
exception: the only format implemented as core code). This makes it a
very good base to study plug-in development.

#### Procedural DataBase (PDB)

Obviously since plug-ins are separate processes, they need a way to
communicate with LIGMA. This is the Procedural Database role, also known
as **PDB**.

The PDB is our protocol allowing plug-ins to request or send information
from or to the main LIGMA process.

Not only this, but every plug-in has the ability to register one or
several procedures itself, which means that any plug-in can call
features brought by other plug-ins through the PDB.

#### libligma and libligmaui

The LIGMA project provides plug-in developers with the `libligma` library.
This is the main library which any plug-in needs. All the core PDB
procedures have a wrapper in `libligma` so you actually nearly never need
to call PDB procedures explicitly (exception being when you call
procedures registered by other plug-ins; these won't have a wrapper).

The `libligmaui` library is an optional one which provides various
graphical interface utility functions, based on the LIGMA toolkit
(`GTK`). Of course, it means that linking to this library is not
mandatory (unlike `libligma`). Some cases where you would not do this
are: because you don't need any graphical interface (e.g. a plug-in
doing something directly without dialog, or even a plug-in meant to be
run on non-GUI servers); because you want to use pure GTK directly
without going through `libligmaui` facility; because you want to make
your GUI with another toolkit…

The whole C reference documentation for both these libraries can be
generated in the main LIGMA build with the `--enable-gi-docgen` autotools
option or the `-Dgi-docgen=enabled` meson option (you need to have the
`gi-docgen` tools installed).

TODO: add online links when it is up for the new APIs.

### Programming Languages

While C is our main language, and the one `libligma` and `libligmaui` are
provided in, these 2 libraries are also introspected thanks to the
[GObject-Introspection](https://gi.readthedocs.io/en/latest/) (**GI**)
project. It means you can in fact create plug-ins with absolutely any
[language with a GI binding](https://wiki.gnome.org/Projects/GObjectIntrospection/Users)
though of course it may not always be as easy as the theory goes.

The LIGMA project explicitly tests the following languages and even
provides a test plug-in as a case study:

* [C](https://gitlab.gnome.org/GNOME/ligma/-/blob/master/extensions/goat-exercises/goat-exercise-c.c) (not a binding)
* [Python 3](https://gitlab.gnome.org/GNOME/ligma/-/blob/master/extensions/goat-exercises/goat-exercise-py3.py)
  (binding)
* [Lua](https://gitlab.gnome.org/GNOME/ligma/-/blob/master/extensions/goat-exercises/goat-exercise-lua.lua)
  (binding)
* [Vala](https://gitlab.gnome.org/GNOME/ligma/-/blob/master/extensions/goat-exercises/goat-exercise-vala.vala)
  (binding)
* [Javascript](https://gitlab.gnome.org/GNOME/ligma/-/blob/master/extensions/goat-exercises/goat-exercise-gjs.js)
  (binding, not supported on Windows for the time being)

One of the big advantage of these automatic bindings is that they are
full-featured since they don't require manual tweaking. Therefore any
function in the C library should have an equivalent in any of the
bindings.

TODO: binding reference documentation.

**Note**: several GObject-Introspection's Scheme bindings exist though
we haven't tested them. Nevertheless, LIGMA also provides historically
the "script-fu" interface, based on an integrated Scheme implementation.
It is different from the other bindings (even from any GI Scheme
binding) and doesn't use `libligma`. Please see the [Script-fu
development](#script-fu-development) section.

### Tutorials

TODO: at least in C and in one of the officially supported binding
(ideally even in all of them).

### Porting from LIGMA 2 plug-ins

### Debugging

LIGMA provides an infrastructure to help debugging plug-ins.

You are invited to read the [dedicated
documentation](debug-plug-ins.txt).

## Script-fu development

`Script-fu` is its own thing as it is a way to run Scheme script with
LIGMA. It is itself implemented as an always-running plug-in with its own
Scheme mini-interpreter and therefore `Script-fu` scripts do not use
`libligma` or `libligmaui`. They interface with the PDB through the
`Script-fu` plug-in.

### Tutorials

### Porting from LIGMA 2 scripts

## GEGL operation development

## Custom data

This section list all types of data usable to enhance LIGMA
functionalities. If you are interested to contribute default data to
LIGMA, be aware that we are looking for a very good base set, not an
unfinite number of data for all possible usage (even the less common
ones).

Furthermore we only accept data on Libre licenses:

* [Free Art License](https://artlibre.org/licence/lal/en/)
* [CC0](https://creativecommons.org/publicdomain/zero/1.0/)
* [CC BY](https://creativecommons.org/licenses/by/4.0/)
* [CC BY-SA](https://creativecommons.org/licenses/by-sa/4.0/)

Of course you are free to share data usable by LIGMA on any license you
want on your own. Providing them as third-party LIGMA
[extensions](#ligma-extensions-gex) is probably the best idea.

### Brushes

LIGMA currently supports the following brush formats:

* LIGMA Brush (GBR): format to store pixmap brushes
* LIGMA Brush Pipe (GIH): format to store a series of pixmap brushes
* LIGMA Generated Brush (VBR): format of "generated" brushes
* LIGMA Brush Pixmap (GPB): *OBSOLETE* format to store pixel brushes
* MyPaint brushes v1 (MYB)
* Photoshop ABR Brush
* Paint Shop Pro JBR Brush

We do fully support the LIGMA formats obviously, as well as MyPaint
brushes, since we use the official `libmypaint` library. We are not sure
how well we support other third-party formats, especially if they had
recent versions.

If you are interested in brushes from a developer perspective, you are
welcome to read specifications of LIGMA formats:
[GBR](specifications/gbr.txt), [GIH](specifications/gih.txt),
[VBR](specifications/vbr.txt) or the obsolete [GPB](specifications/gpb.txt).

If you want to contribute brushes to the official LIGMA, be aware we
would only accept brushes in non-obsolete LIGMA formats. All these
formats can be generated by LIGMA itself from images.

If you want to contribute MyPaint brushes, we recommend to propose them
to the [MyPaint-brushes](https://github.com/mypaint/mypaint-brushes/)
data project, which is also used by LIGMA for its default MyPaint brush
set.

Otherwise, you are welcome to provide brush set in any format as
third-party [extensions](#ligma-extensions-gex).

### Dynamics

LIGMA supports the LIGMA Paint Dynamics format which can be generated from
within LIGMA.

### Patterns

LIGMA supports the LIGMA Pattern format (PAT, whose
[specification](specifications/pat.txt) is available for developers).

This format can be exported by LIGMA itself.

Alternatively LIGMA supports patterns from `GdkPixbuf` (TODO: get more
information?).

### Palettes

LIGMA supports the LIGMA Palette format which can be generated from within
LIGMA.

### Gradients

LIGMA supports the LIGMA Gradient format (GGR, whose
[specification](specifications/ggr.txt) is available for developers)
which can be generated from within LIGMA.

Alternatively LIGMA supports the SVG Gradient format.

### Themes

GTK3 uses CSS themes. Don't be fooled though. It's not real CSS in that
it doesn't have all the features of real web CSS, and since it's for
desktop applications, some things are necessarily different. What it
means is mostly that it "looks similar" enough that people used to web
styling should not be too disorientated.

You can start by looking at the [official
documentation](https://docs.gtk.org/gtk3/migrating-themes.html) for
theme migration (from GTK+2 to 3), which gives a good overview, though
it's far from being perfect unfortunately.

Another good idea would be to look at existing well maintained GTK3
themes to get inspiration and see how things work.

Finally you can look at our existing themes, like the [System
theme](https://gitlab.gnome.org/GNOME/ligma/-/blob/master/themes/System/ligma.css).
Note though that this `System` theme is pretty bare, and that's its goal
(try to theme as few as possible over whatever is the current real
system theme).

TODO: for any theme maker reading this, what we want for LIGMA 3.0 are at
least the following additional themes:

- a full custom theme using neutral grayscale colors with a dark and
  light variant;
- a mid-gray neutral theme.

As a last trick for theme makers, we recommend to work with the
GtkInspector tool, which allows you to test CSS rules live in the `CSS`
tab. You can run the `GtkInspector` by going to the `File > Debug` menu
and selecting `Start GtkInspector` menu item.

It also allows you to find the name of a widget to use in your CSS
rules. To do so:

* Start the `GtkInspector`;
* go on the "Objects" tab;
* click the "target" 🞋 icon on the headerbar's top-left, then pick in
  LIGMA interface the widget you are interested to style;
* the widget name will be displayed on the top of the information area
  of the dialog.
* Feel free to browse the various sections to see the class hierachy,
  CSS nodes and so on.
* The second top-left button (just next to the target icon) allows you
  to switch between the details of the selected widget and the widget
  hierarchy (container widgets containing other widgets), which is also
  very useful information.

Additionally you can quickly switch between the light and dark variant
of a same theme by going to "Visual" tab and switching the "Dark
Variant" button ON or OFF.

### Icon themes

Icon sets (a.k.a. "icon themes") have been separated from themes since
LIGMA 2.10 so you can have any icon theme with any theme.

We currently only support 2 such icon themes — Symbolic and Color — and
we keep around the Legacy icons.

We don't want too many alternative designs as official icon themes
(people are welcome to publish their favorite designs as third-party
icons) though we would welcome special-purpose icon themes (e.g. high
contrast).

We also welcome design updates as a whole (anyone willing to work on
this should discuss with us and propose something) and obviously fixes
on existing icons or adding missing icons while keeping consistent
styling.

See the dedicated [icons documentation](icons.md) for more technical
information.

### Tool presets


## LIGMA extensions (*.gex*)

## Continuous Integration

For most of its continuous integration (macOS excepted), LIGMA project
uses Gitlab CI. We recommend looking the file
[.gitlab-ci.yml](/.gitlab-ci.yml) which is the startup script.

The main URL for our CI system is
[build.ligma.org](https://build.ligma.org) which redirects to Gitlab
pipelines page.

Note that it is important to keep working CI jobs for a healthy code
source. Therefore when you push some code which breaks the CI (you
should receive a notification email when you do so), you are expected to
look at the failed jobs' logs, try and understand the issue(s) and fix
them (or ask for help). Don't just shrug this because it works locally
(the point of the CI is to build in more conditions than developers
usually do locally).

Of course, sometimes CI failures are out of our control, for instance
when downloaded dependencies have issues, or because of runner issues.
You should still check that these were reported and that
packagers/maintainers of these parts are aware and working on a fix.

### Automatic pipelines

At each commit pushed to the repository, several pipelines are currently
running, such as:

- Debian testing autotools and meson builds (autotools is still the
  official build system while meson is experimental).
- Windows builds (cross or natively compiled).

Additionally, we test build with alternative tools or options (e.g. with
`Clang` instead of `gcc` compiler) or jobs which may take much longer,
such as package creation as scheduled pipelines (once every few days).

The above listing is not necessarily exhaustive nor is it meant to be.
Only the [.gitlab-ci.yml](/.gitlab-ci.yml) script is meant to be
authoritative. The top comment in this file should stay as exhaustive
as possible.

### Manual pipelines

It is possible to trigger pipelines manually, for instance with specific
jobs, if you have the "*Developer*" Gitlab role:

1. go to the [Pipelines](https://gitlab.gnome.org/GNOME/ligma/-/pipelines)
   page.
2. Hit the "*Run pipeline*" button.
3. Choose the branch or tag you wish to build.
4. Add relevant variables. A list of variables named `LIGMA_CI_*` are
   available (just set them to any value) and will trigger specific job
   lists. These variables are listed in the top comment of
   [.gitlab-ci.yml](/.gitlab-ci.yml).

### Merge request pipelines

Special pipelines happen for merge request code. For instance, these
also include a (non-perfect) code style check.

Additionally you can trigger Windows installer or flatpack standalone
packages to be generated with the MR code as explained in
[gitlab-mr.md](gitlab-mr.md).

### Release pipeline

Special pipelines happen when pushing git `tags`. These should be tested
before a release to avoid unexpected release-time issues, as explained
in [release-howto.txt](release-howto.txt).

### Exception: macOS

As an exception, macOS is currently built with the `Circle-CI` service.
The whole CI scripts and documentation can be found in the dedicated
[ligma-macos-build](https://gitlab.gnome.org/Infrastructure/ligma-macos-build)
repository.

Eventually we want to move this pipeline to Gitlab as well.

## Core development

When writing code, any core developer is expected to follow:

- LIGMA's [coding style](https://developer.ligma.org/core/coding_style/);
- the [directory structure](#directory-structure-of-ligma-source-tree)
- our [header file inclusion policy](includes.txt)

[LIGMA's developer wiki](https://wiki.ligma.org/index.php/Main_Page) can
also contain various valuable resources.

Finally the [debugging-tips](debugging-tips.md) file contain many very
useful tricks to help you debugging in various common cases.

### Newcomers

If this is your first time contributing to LIGMA, you might be interested
by build instructions. The previously mentioned wiki in particular has a
[Hacking:Building](https://wiki.ligma.org/wiki/Hacking:Building) page
with various per-platform subpages. The [HACKING](HACKING.md) docs will
also be of interest.

You might also like to read these [instructions on submitting
patches](https://ligma.org/bugs/howtos/submit-patch.html).

If you are unsure what to work on, this [list of bugs for
newcomers](https://gitlab.gnome.org/GNOME/ligma/-/issues?scope=all&state=opened&label_name[]=4.%20Newcomers)
might be a good start. It doesn't necessarily contain only bugs for
beginner developers. Some of them might be for experienced developers
who just don't know yet enough the codebase.

Nevertheless we often recommend to rather work on topics which you
appreciate, or even better: fixes for bugs you encounter or features you
want. These are the most self-rewarding contributions which will really
make you feel like developing on LIGMA means developing for yourself.

### Core Contributors

### Maintainers

LIGMA maintainers have a few more responsibilities, in particular
regarding releases and coordination.

Some of these duties include:

- setting the version of LIGMA as well as the API version. This is
  explained in [libtool-instructions.txt](libtool-instructions.txt).
- Making a release by followng accurately the process described in
  [release-howto.txt](release-howto.txt).
- Managing dependencies: except for core projects (such as `babl` and
  `GEGL`), we should stay as conservative as possible for the stable
  branch (otherwise distributions might end up getting stuck providing
  very old LIGMA versions). On development builds, we should verify any
  mandatory dependency is at the very least available in Debian testing
  and MSYS2; we may be a bit more adventurous for optional dependencies
  yet stay reasonable (a feature is not so useful if nobody can build
  it). In any case, any dependency bump must be carefully weighed within
  reason, especially when getting close to make the development branch
  into the new stable branch. See also [os-support.txt](os-support.txt).
- Maintain [milestones](gitlab-milestones.txt).
- Maintain [NEWS](/NEWS) file. Any developer is actually encouraged to
  update it when they do noteworthy changes, but this is the
  maintainers' role to do the finale checks and make sure we don't miss
  anything. The purpose of this rule is to make it as easy as possible
  to make a LIGMA release as looking in this file to write release notes
  is much easier than reviewing hundreds of commits.

#### AppStream metadata

One of the requirement of a good release is to have a proper `<release>`
tag in the [AppStream metadata
file](desktop/org.ligma.LIGMA.appdata.xml.in.in). This metadata is used by
various installers (e.g. GNOME Software, KDE Discover), software
websites (e.g. Flathub). Having good release info in particular will
help people know what happened on the last release, and also it will
have LIGMA feature among the "recently updated" software list, when the
installer/website has such a section.

Moreover we use this data within LIGMA itself where we feature recent
changes in the Welcome dialog after an update.

What you should take care of are the following points:

* For the general rules on AppStream format, please refer to its
  [specifications](https://www.freedesktop.org/software/appstream/docs/).
* Native language text are translated if a tag name starts with `_`.
  Therefore do not use `<p>` but `<_p>` in the source. Same for `<_li>`
  instead of `<li>`. These will be transformed by our build system.
* It also means you should push the `<release>` text early to leave time
  to translators.
* Since we use this data in LIGMA itself, we stick to a specific
  contents in a `<release>` tag. In particular, all `<release>` tags
  must start with one or several `<_p>` paragraphs, followed by a `<ul>`
  list.
* Make sure the `date` and `version` attributes are appropriate. When
  the release date is still unknown, setting "TODO" is a good practice
  as our CI will `grep TODO` on even micro versions and fail on them.
* We have a custom feature in LIGMA: adding `demo` attributes to `<_li>`
  points of the release will generate a feature tour (basically blinking
  several pieces of LIGMA in order).
  The format is as follows:
    - demo steps are comma-separated;
    - each step are in the form `dockable:widget=value`. You could write
      only `dockable` (which would blink the dockable), or
      `dockable:widget` (which would only blink the specific widget).
      The full form would not only blink the widget but also change its
      value (only boolean and integer types are supported for now).
    - dockable names can be found in `app/dialogs/dialogs.c`. Since they
      all start with `ligma-`, writing the suffix or not is equivalent.
    - the widget IDs will default to the associated property. If the
      widget is not a propwidget, or you wish to create a specific ID,
      `ligma_widget_set_identifier()` must have been set explicitly to
      this widget.
    - as a special case, tool buttons (in `toolbox:` dockable) IDs are
      the action names, so you can just search in `Edit > Keyboard
      Shortcuts` menu. These are usually of the form `tools-*` so the
      short form without `tools-` is also accepted.
    - spaces in this `demo` attribute are ignored which allows to
      pretty-write the demo rules for better reading.

### Directory structure of LIGMA source tree

LIGMA source tree can be divided into the main application, libraries, plug-ins,
data files and some stuff that don't fit into these categories. Here are the
top-level directories:

| Folder          | Description |
| ---             | ---         |
| app/            | Source code of the main LIGMA application             |
| app-tools/      | Source code of distributed tools                     |
| build/          | Scripts for creating binary packages                 |
| cursors/        | Bitmaps used to construct cursors                    |
| data/           | Data files: brushes, gradients, patterns, images…    |
| desktop/        | Desktop integration files                            |
| devel-docs/     | Developers documentation                             |
| docs/           | Users documentation                                  |
| etc/            | Configuration files installed with LIGMA              |
| extensions/     | Source code of extensions                            |
| icons/          | Official icon themes                                 |
| libligma/        | Library for plug-ins (core does not link against)    |
| libligmabase/    | Basic functions shared by core and plug-ins          |
| libligmacolor/   | Color-related functions shared by core and plug-ins  |
| libligmaconfig/  | Config functions shared by core and plug-ins         |
| libligmamath/    | Mathematic operations useful for core and plug-ins   |
| libligmamodule/  | Abstracts dynamic loading of modules (used to implement loadable color selectors and display filters) |
| libligmathumb/   | Thumbnail functions shared by core and plug-ins      |
| libligmawidgets/ | User interface elements (widgets) and utility functions shared by core and plug-ins                   |
| m4macros/       | Scripts for autotools configuration                  |
| menus/          | XML/XSL files used to generate menus                 |
| modules/        | Color selectors and display filters loadable at run-time |
| pdb/            | Scripts for PDB source code generation               |
| plug-ins/       | Source code for plug-ins distributed with LIGMA       |
| po/             | Translations of strings used in the core application |
| po-libligma/     | Translations of strings used in libligma              |
| po-plug-ins/    | Translations of strings used in C plug-ins           |
| po-python/      | Translations of strings used in Python plug-ins      |
| po-script-fu/   | Translations of strings used in Script-Fu scripts    |
| po-tags/        | Translations of strings used in tags                 |
| po-tips/        | Translations of strings used in tips                 |
| po-windows-installer/ | Translations of strings used in the Windows installer |
| themes/         | Official themes                                      |
| tools/          | Source code for non-distributed LIGMA-related tools   |
| .gitlab/        | Gitlab-related templates or scripts                  |

The source code of the main LIGMA application is found in the `app/` directory:

| Folder          | Description |
| ---             | ---         |
| app/actions/    | Code of actions (`LigmaAction*` defined in `app/widgets/`) (depends: GTK)         |
| app/config/     | Config files handling: LigmaConfig interface and LigmaRc object (depends: GObject) |
| app/core/       | Core of LIGMA **core** (depends: GObject)                                         |
| app/dialogs/    | Dialog widgets (depends: GTK)                                                    |
| app/display/    | Handles displays (e.g. image windows) (depends: GTK)                             |
| app/file/       | File handling routines in **core** (depends: GIO)                                |
| app/file-data/  | LIGMA file formats (gbr, gex, gih, pat) support (depends: GIO)                    |
| app/gegl/       | Wrapper code for babl and GEGL API (depends: babl, GEGL)                         |
| app/gui/        | Code that puts the user interface together (depends: GTK)                        |
| app/menus/      | Code for menus (depends: GTK)                                                    |
| app/operations/ | Custom GEGL operations (depends: GEGL)                                           |
| app/paint/      | Paint core that provides different ways to paint strokes (depends: GEGL)         |
| app/pdb/        | Core side of the Procedural Database, exposes internal functionality             |
| app/plug-in/    | Plug-in handling in **core**                                                     |
| app/propgui/    | Property widgets generated from config properties (depends: GTK)                 |
| app/tests/      | Core unit testing framework                                                      |
| app/text/       | Text handling in **core**                                                        |
| app/tools/      | User interface part of the tools. Actual tool functionality is in core           |
| app/vectors/    | Vectors framework in **core**                                                    |
| app/widgets/    | Collection of widgets used in the application GUI                                |
| app/xcf/        | XCF file handling in **core**                                                    |

You should also check out [ligma-module-dependencies.svg](ligma-module-dependencies.svg).
**TODO**: this SVG file is interesting yet very outdated. It should not
be considered as some kind dependency rule and should be updated.

### Advanced concepts

#### XCF

The `XCF` format is the core image format of LIGMA, which mirrors
features made available in LIGMA. More than an image format, you may
consider it as a work or project format, as it is not made for finale
presentation of an artwork but for the work-in-progress processus.

Developers are welcome to read the [specifications of XCF](specifications/xcf.txt).

#### Locks

Items in an image can be locked in various ways to prevent different
types of edits.

This is further explained in [the specifications of locks](https://developer.ligma.org/core/specifications/locks/).

#### UI Framework

LIGMA has an evolved GUI framework, with a toolbox, dockables, menus…

This [document describing how the LIGMA UI framework functions and how it
is implemented](ui-framework.txt) might be of interest.

#### Contexts

LIGMA uses a lot a concept of "contexts". We recommend reading more about
[how LigmaContexts are used in LIGMA](contexts.txt).

#### Undo

LIGMA undo system can be challenging at times. This [quick overview of
the undo system](undo.txt) can be of interest as a first introduction.

#### Parasites

LIGMA has a concept of "parasite" data which basically correspond to
persistent or semi-persistent data which can be attached to images or
items (layers, channels, paths) within an image. These parasites are
saved in the XCF format.

Parasites can also be attached globally to the LIGMA session.

Parasite contents is format-free and you can use any parasite name,
nevertheless LIGMA itself uses parasite so you should read the
[descriptions of known parasites](parasites.txt).

#### Metadata

LIGMA supports Exif, IPTC and XMP metadata as well as various image
format-specific metadata. The topic is quite huge and complex, if not
overwhelming.

This [old document](https://developer.ligma.org/core/specifications/exif_handling/)
might be of interest (or maybe not, it has not been recently reviewed and might
be widely outdated; in any case, it is not a complete document at all as we
definitely do a lot more nowadays). **TODO**: review this document and delete or
update it depending of whether it still makes sense.

#### Tagging

Various data in LIGMA can be tagged across sessions.

This document on [how resource tagging in LIGMA works](tagging.txt) may
be of interest.
