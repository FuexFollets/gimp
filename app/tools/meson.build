stamp_tools_enums = custom_target('stamp-tools-enums.h',
  input : [
    files(
      'tools-enums.h'
    ),
  ],
  output: [ 'stamp-tools-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'tools-',
    '#include <gio/gio.h>\n'              +
    '#include "libligmabase/ligmabase.h"\n' +
    '#include "core/core-enums.h"\n',
    '#include "ligma-intl.h"'
  ],
  build_by_default: true
)

libapptools_sources = [
  'ligma-tools.c',
  'ligma-tool-options-manager.c',
  'ligmaairbrushtool.c',
  'ligmaalignoptions.c',
  'ligmaaligntool.c',
  'ligmabrightnesscontrasttool.c',
  'ligmabrushtool.c',
  'ligmabucketfilloptions.c',
  'ligmabucketfilltool.c',
  'ligmabycolorselecttool.c',
  'ligmacageoptions.c',
  'ligmacagetool.c',
  'ligmacloneoptions-gui.c',
  'ligmaclonetool.c',
  'ligmacoloroptions.c',
  'ligmacolorpickeroptions.c',
  'ligmacolorpickertool.c',
  'ligmacolortool.c',
  'ligmaconvolvetool.c',
  'ligmacropoptions.c',
  'ligmacroptool.c',
  'ligmacurvestool.c',
  'ligmadodgeburntool.c',
  'ligmadrawtool.c',
  'ligmaeditselectiontool.c',
  'ligmaellipseselecttool.c',
  'ligmaerasertool.c',
  'ligmafilteroptions.c',
  'ligmafiltertool-settings.c',
  'ligmafiltertool-widgets.c',
  'ligmafiltertool.c',
  'ligmaflipoptions.c',
  'ligmafliptool.c',
  'ligmaforegroundselectoptions.c',
  'ligmaforegroundselecttool.c',
  'ligmaforegroundselecttoolundo.c',
  'ligmafreeselecttool.c',
  'ligmafuzzyselecttool.c',
  'ligmagegltool.c',
  'ligmagenerictransformtool.c',
  'ligmagradientoptions.c',
  'ligmagradienttool-editor.c',
  'ligmagradienttool.c',
  'ligmaguidetool.c',
  'ligmahandletransformoptions.c',
  'ligmahandletransformtool.c',
  'ligmahealtool.c',
  'ligmahistogramoptions.c',
  'ligmainkoptions-gui.c',
  'ligmainktool.c',
  'ligmaiscissorsoptions.c',
  'ligmaiscissorstool.c',
  'ligmalevelstool.c',
  'ligmamagnifyoptions.c',
  'ligmamagnifytool.c',
  'ligmameasureoptions.c',
  'ligmameasuretool.c',
  'ligmamoveoptions.c',
  'ligmamovetool.c',
  'ligmamybrushoptions-gui.c',
  'ligmamybrushtool.c',
  'ligmanpointdeformationoptions.c',
  'ligmanpointdeformationtool.c',
  'ligmaoffsettool.c',
  'ligmaoperationtool.c',
  'ligmapaintbrushtool.c',
  'ligmapaintoptions-gui.c',
  'ligmapaintselectoptions.c',
  'ligmapaintselecttool.c',
  'ligmapainttool-paint.c',
  'ligmapainttool.c',
  'ligmapenciltool.c',
  'ligmaperspectiveclonetool.c',
  'ligmaperspectivetool.c',
  'ligmapolygonselecttool.c',
  'ligmarectangleoptions.c',
  'ligmarectangleselectoptions.c',
  'ligmarectangleselecttool.c',
  'ligmaregionselectoptions.c',
  'ligmaregionselecttool.c',
  'ligmarotatetool.c',
  'ligmasamplepointtool.c',
  'ligmascaletool.c',
  'ligmaseamlesscloneoptions.c',
  'ligmaseamlessclonetool.c',
  'ligmaselectionoptions.c',
  'ligmaselectiontool.c',
  'ligmasheartool.c',
  'ligmasmudgetool.c',
  'ligmasourcetool.c',
  'ligmatextoptions.c',
  'ligmatexttool-editor.c',
  'ligmatexttool.c',
  'ligmathresholdtool.c',
  'ligmatilehandleriscissors.c',
  'ligmatool-progress.c',
  'ligmatool.c',
  'ligmatoolcontrol.c',
  'ligmatooloptions-gui.c',
  'ligmatools-utils.c',
  'ligmatransform3doptions.c',
  'ligmatransform3dtool.c',
  'ligmatransformgridoptions.c',
  'ligmatransformgridtool.c',
  'ligmatransformgridtoolundo.c',
  'ligmatransformoptions.c',
  'ligmatransformtool.c',
  'ligmaunifiedtransformtool.c',
  'ligmavectoroptions.c',
  'ligmavectortool.c',
  'ligmawarpoptions.c',
  'ligmawarptool.c',
  'tool_manager.c',
  'tools-enums.c',
  stamp_tools_enums,

  appcoremarshal[1],
]

libapptools = static_library('apptools',
  libapptools_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Tools"',
  dependencies: [
    gegl, gtk3,
  ],
)
