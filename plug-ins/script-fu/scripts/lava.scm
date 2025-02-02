; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Lava effect
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; based on a idea by Sven Riedel <lynx@heim8.tu-clausthal.de>
; tweaked a bit by Sven Neumann <neumanns@uni-duesseldorf.de>
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.


(define (script-fu-lava image
                        drawable
                        seed
                        tile_size
                        mask_size
                        gradient
                        keep-selection
                        separate-layer
                        current-grad)
  (let* (
        (type (car (ligma-drawable-type-with-alpha drawable)))
        (image-width (car (ligma-image-get-width image)))
        (image-height (car (ligma-image-get-height image)))
        (active-selection 0)
        (selection-bounds 0)
        (select-offset-x 0)
        (select-offset-y 0)
        (select-width 0)
        (select-height 0)
        (lava-layer 0)
        (active-layer 0)
        (selected-layers (ligma-image-get-selected-layers image))
        (num-selected-layers (car selected-layers))
        (selected-layers-array (cadr selected-layers))
        )

    (if (= num-selected-layers 1)
        (begin
            (ligma-context-push)
            (ligma-context-set-defaults)
            (ligma-image-undo-group-start image)

            (if (= (car (ligma-drawable-has-alpha drawable)) FALSE)
                (ligma-layer-add-alpha drawable)
            )

            (if (= (car (ligma-selection-is-empty image)) TRUE)
                (ligma-image-select-item image CHANNEL-OP-REPLACE drawable)
            )

            (set! active-selection (car (ligma-selection-save image)))
            (ligma-image-set-selected-layers image 1 (make-vector 1 drawable))

            (set! selection-bounds (ligma-selection-bounds image))
            (set! select-offset-x (cadr selection-bounds))
            (set! select-offset-y (caddr selection-bounds))
            (set! select-width (- (cadr (cddr selection-bounds)) select-offset-x))
            (set! select-height (- (caddr (cddr selection-bounds)) select-offset-y))

            (if (= separate-layer TRUE)
                (begin
                  (set! lava-layer (car (ligma-layer-new image
                                                        select-width
                                                        select-height
                                                        type
                                                        "Lava Layer"
                                                        100
                                                        LAYER-MODE-NORMAL-LEGACY)))

                  (ligma-image-insert-layer image lava-layer 0 -1)
                  (ligma-layer-set-offsets lava-layer select-offset-x select-offset-y)
                  (ligma-selection-none image)
                  (ligma-drawable-edit-clear lava-layer)

                  (ligma-image-select-item image CHANNEL-OP-REPLACE drawable)
                  (ligma-image-set-selected-layers image 1 (make-vector 1 lava-layer))
                )
            )

            (set! selected-layers (ligma-image-get-selected-layers image))
            (set! num-selected-layers (car selected-layers))
            (set! selected-layers-array (cadr selected-layers))
            (set! active-layer (aref selected-layers-array (- num-selected-layers 1)))

            (if (= current-grad FALSE)
                (ligma-context-set-gradient gradient)
            )

            (plug-in-solid-noise RUN-NONINTERACTIVE image active-layer FALSE TRUE seed 2 2 2)
            (plug-in-cubism RUN-NONINTERACTIVE image active-layer tile_size 2.5 0)
            (plug-in-oilify RUN-NONINTERACTIVE image active-layer mask_size 0)
            (plug-in-edge RUN-NONINTERACTIVE image active-layer 2 0 0)
            (plug-in-gauss-rle RUN-NONINTERACTIVE image active-layer 2 TRUE TRUE)
            (plug-in-gradmap RUN-NONINTERACTIVE image num-selected-layers selected-layers-array)

            (if (= keep-selection FALSE)
                (ligma-selection-none image)
            )

            (ligma-image-set-selected-layers image 1 (make-vector 1 drawable))
            (ligma-image-remove-channel image active-selection)

            (ligma-image-undo-group-end image)
            (ligma-context-pop)

            (ligma-displays-flush)
        )
    ; else
        (ligma-message _"Lava works with exactly one selected layer")
    )
  )
)

(script-fu-register "script-fu-lava"
  _"_Lava..."
  _"Fill the current selection with lava"
  "Adrian Likins <adrian@ligma.org>"
  "Adrian Likins"
  "10/12/97"
  "RGB* GRAY*"
  SF-IMAGE       "Image"          0
  SF-DRAWABLE    "Drawable"       0
  SF-ADJUSTMENT _"Seed"           '(10 1 30000 1 10 0 1)
  SF-ADJUSTMENT _"Size"           '(10 0 100 1 10 0 1)
  SF-ADJUSTMENT _"Roughness"      '(7 3 50 1 10 0 0)
  SF-GRADIENT   _"Gradient"       "German flag smooth"
  SF-TOGGLE     _"Keep selection" TRUE
  SF-TOGGLE     _"Separate layer" TRUE
  SF-TOGGLE     _"Use current gradient" FALSE
)

(script-fu-menu-register "script-fu-lava"
                         "<Image>/Filters/Render")
