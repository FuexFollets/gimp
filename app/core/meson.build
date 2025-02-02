stamp_core_enums = custom_target('stamp-core-enums.h',
  input : [
    files(
      'core-enums.h'
    ),
  ],
  output: [ 'stamp-core-enums.h', ],
  command: [
    mkenums_wrap, perl,
    meson.project_source_root(), meson.current_source_dir(),
    meson.current_build_dir(),
    'core-',
    '#include <gio/gio.h>\n'              +
    '#include "libligmabase/ligmabase.h"\n',
    '#include "ligma-intl.h"'
  ],
  build_by_default: true
)

appcoremarshal = gnome.genmarshal('ligmamarshal',
  prefix: 'ligma_marshal',
  sources: 'ligmamarshal.list',
  install_header: false,
)

libappcore_sources = [
  'ligma-atomic.c',
  'ligma-batch.c',
  'ligma-cairo.c',
  'ligma-contexts.c',
  'ligma-data-factories.c',
  'ligma-edit.c',
  'ligma-filter-history.c',
  'ligma-gradients.c',
  'ligma-gui.c',
  'ligma-internal-data.c',
  'ligma-memsize.c',
  'ligma-modules.c',
  'ligma-palettes.c',
  'ligma-parallel.cc',
  'ligma-parasites.c',
  'ligma-spawn.c',
  'ligma-tags.c',
  'ligma-templates.c',
  'ligma-transform-resize.c',
  'ligma-transform-3d-utils.c',
  'ligma-transform-utils.c',
  'ligma-units.c',
  'ligma-user-install.c',
  'ligma-utils.c',
  'ligma.c',
  'ligmaasync.c',
  'ligmaasyncset.c',
  'ligmaauxitem.c',
  'ligmaauxitemundo.c',
  'ligmabacktrace-linux.c',
  'ligmabacktrace-none.c',
  'ligmabacktrace-windows.c',
  'ligmabezierdesc.c',
  'ligmaboundary.c',
  'ligmabrush-boundary.c',
  'ligmabrush-load.c',
  'ligmabrush-mipmap.cc',
  'ligmabrush-save.c',
  'ligmabrush-transform.cc',
  'ligmabrush.c',
  'ligmabrushcache.c',
  'ligmabrushclipboard.c',
  'ligmabrushgenerated-load.c',
  'ligmabrushgenerated-save.c',
  'ligmabrushgenerated.c',
  'ligmabrushpipe-load.c',
  'ligmabrushpipe-save.c',
  'ligmabrushpipe.c',
  'ligmabuffer.c',
  'ligmacancelable.c',
  'ligmachannel-combine.c',
  'ligmachannel-select.c',
  'ligmachannel.c',
  'ligmachannelpropundo.c',
  'ligmachannelundo.c',
  'ligmachunkiterator.c',
  'ligmacontainer-filter.c',
  'ligmacontainer.c',
  'ligmacontext.c',
  'ligmacoords-interpolate.c',
  'ligmacoords.c',
  'ligmacurve-load.c',
  'ligmacurve-map.c',
  'ligmacurve-save.c',
  'ligmacurve.c',
  'ligmadashpattern.c',
  'ligmadata.c',
  'ligmadatafactory.c',
  'ligmadataloaderfactory.c',
  'ligmadisplay.c',
  'ligmadocumentlist.c',
  'ligmadrawable-bucket-fill.c',
  'ligmadrawable-combine.c',
  'ligmadrawable-edit.c',
  'ligmadrawable-equalize.c',
  'ligmadrawable-fill.c',
  'ligmadrawable-filters.c',
  'ligmadrawable-floating-selection.c',
  'ligmadrawable-foreground-extract.c',
  'ligmadrawable-gradient.c',
  'ligmadrawable-histogram.c',
  'ligmadrawable-levels.c',
  'ligmadrawable-offset.c',
  'ligmadrawable-operation.c',
  'ligmadrawable-preview.c',
  'ligmadrawable-shadow.c',
  'ligmadrawable-stroke.c',
  'ligmadrawable-transform.c',
  'ligmadrawable.c',
  'ligmadrawablefilter.c',
  'ligmadrawablemodundo.c',
  'ligmadrawablepropundo.c',
  'ligmadrawablestack.c',
  'ligmadrawableundo.c',
  'ligmadynamics-load.c',
  'ligmadynamics-save.c',
  'ligmadynamics.c',
  'ligmadynamicsoutput.c',
  'ligmaerror.c',
  'ligmaextension-error.c',
  'ligmaextension.c',
  'ligmaextensionmanager.c',
  'ligmafilloptions.c',
  'ligmafilter.c',
  'ligmafilteredcontainer.c',
  'ligmafilterstack.c',
  'ligmafloatingselectionundo.c',
  'ligmagradient-load.c',
  'ligmagradient-save.c',
  'ligmagradient.c',
  'ligmagrid.c',
  'ligmagrouplayer.c',
  'ligmagrouplayerundo.c',
  'ligmaguide.c',
  'ligmaguideundo.c',
  'ligmahistogram.c',
  'ligmaidtable.c',
  'ligmaimage-arrange.c',
  'ligmaimage-color-profile.c',
  'ligmaimage-colormap.c',
  'ligmaimage-convert-indexed.c',
  'ligmaimage-convert-precision.c',
  'ligmaimage-convert-type.c',
  'ligmaimage-crop.c',
  'ligmaimage-duplicate.c',
  'ligmaimage-flip.c',
  'ligmaimage-grid.c',
  'ligmaimage-guides.c',
  'ligmaimage-item-list.c',
  'ligmaimage-merge.c',
  'ligmaimage-metadata.c',
  'ligmaimage-new.c',
  'ligmaimage-pick-color.c',
  'ligmaimage-pick-item.c',
  'ligmaimage-preview.c',
  'ligmaimage-quick-mask.c',
  'ligmaimage-resize.c',
  'ligmaimage-rotate.c',
  'ligmaimage-sample-points.c',
  'ligmaimage-scale.c',
  'ligmaimage-snap.c',
  'ligmaimage-symmetry.c',
  'ligmaimage-transform.c',
  'ligmaimage-undo-push.c',
  'ligmaimage-undo.c',
  'ligmaimage.c',
  'ligmaimagefile.c',
  'ligmaimageproxy.c',
  'ligmaimageundo.c',
  'ligmaitem-exclusive.c',
  'ligmaitem-preview.c',
  'ligmaitem.c',
  'ligmaitemlist.c',
  'ligmaitempropundo.c',
  'ligmaitemstack.c',
  'ligmaitemtree.c',
  'ligmaitemundo.c',
  'ligmalayer-floating-selection.c',
  'ligmalayer-new.c',
  'ligmalayer.c',
  'ligmalayermask.c',
  'ligmalayermaskpropundo.c',
  'ligmalayermaskundo.c',
  'ligmalayerpropundo.c',
  'ligmalayerstack.c',
  'ligmalayerundo.c',
  'ligmalineart.c',
  'ligmalist.c',
  'ligmamaskundo.c',
  'ligmamybrush-load.c',
  'ligmamybrush.c',
  'ligmaobject.c',
  'ligmaobjectqueue.c',
  'ligmapaintinfo.c',
  'ligmapalette-import.c',
  'ligmapalette-load.c',
  'ligmapalette-save.c',
  'ligmapalette.c',
  'ligmapalettemru.c',
  'ligmaparamspecs-desc.c',
  'ligmaparamspecs.c',
  'ligmaparasitelist.c',
  'ligmapattern-load.c',
  'ligmapattern-save.c',
  'ligmapattern.c',
  'ligmapatternclipboard.c',
  'ligmapdbprogress.c',
  'ligmapickable-auto-shrink.c',
  'ligmapickable-contiguous-region.cc',
  'ligmapickable.c',
  'ligmaprogress.c',
  'ligmaprojectable.c',
  'ligmaprojection.c',
  'ligmasamplepoint.c',
  'ligmasamplepointundo.c',
  'ligmascanconvert.c',
  'ligmaselection.c',
  'ligmasettings.c',
  'ligmastrokeoptions.c',
  'ligmasubprogress.c',
  'ligmasymmetry-mandala.c',
  'ligmasymmetry-mirror.c',
  'ligmasymmetry-tiling.c',
  'ligmasymmetry.c',
  'ligmatag.c',
  'ligmatagcache.c',
  'ligmatagged.c',
  'ligmataggedcontainer.c',
  'ligmatempbuf.c',
  'ligmatemplate.c',
  'ligmatilehandlerprojectable.c',
  'ligmatoolgroup.c',
  'ligmatoolinfo.c',
  'ligmatoolitem.c',
  'ligmatooloptions.c',
  'ligmatoolpreset-load.c',
  'ligmatoolpreset-save.c',
  'ligmatoolpreset.c',
  'ligmatreehandler.c',
  'ligmatreeproxy.c',
  'ligmatriviallycancelablewaitable.c',
  'ligmauncancelablewaitable.c',
  'ligmaundo.c',
  'ligmaundostack.c',
  'ligmaunit.c',
  'ligmaviewable.c',
  'ligmawaitable.c',

  'core-enums.c',
  stamp_core_enums,

  appcoremarshal,
  cursors_sources,
  pickers_sources,
  icons_core_sources,
]

libappcore = static_library('appcore',
  libappcore_sources,
  include_directories: [ rootInclude, rootAppInclude, ],
  c_args: '-DG_LOG_DOMAIN="Ligma-Core"',
  dependencies: [
    cairo,
    gegl,
    gdk_pixbuf,
    libmypaint,
    gexiv2,
    appstream_glib,
    math,
    dl,
    libunwind,
  ],
)
