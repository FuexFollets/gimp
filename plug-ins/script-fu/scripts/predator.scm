; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Predator effect
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.ed
;
;  The idea here is too make the image/selection look sort of like
;  the view the predator had in the movies. ie, kind of a thermogram
;  type of thing. Works best on colorful rgb images.
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


(define (script-fu-predator image
                            drawable
                            edge-amount
                            pixelize
                            pixel-size
                            keep-selection
                            separate-layer)
  (let* (
        (type (car (ligma-drawable-type-with-alpha drawable)))
        (image-width (car (ligma-image-get-width image)))
        (image-height (car (ligma-image-get-height image)))
        (active-selection 0)
        (from-selection 0)
        (selection-bounds 0)
        (select-offset-x 0)
        (select-offset-y 0)
        (select-width 0)
        (select-height 0)
        (effect-layer 0)
        (active-layer 0)
        )

    (ligma-context-push)
    (ligma-context-set-defaults)
    (ligma-image-undo-group-start image)
    (ligma-layer-add-alpha drawable)

    (if (= (car (ligma-selection-is-empty image)) TRUE)
        (begin
          (ligma-image-select-item image CHANNEL-OP-REPLACE drawable)
          (set! active-selection (car (ligma-selection-save image)))
          (set! from-selection FALSE)
        )
        (begin
          (set! from-selection TRUE)
          (set! active-selection (car (ligma-selection-save image)))
        )
    )

    (set! selection-bounds (ligma-selection-bounds image))
    (set! select-offset-x (cadr selection-bounds))
    (set! select-offset-y (caddr selection-bounds))
    (set! select-width (- (cadr (cddr selection-bounds)) select-offset-x))
    (set! select-height (- (caddr (cddr selection-bounds)) select-offset-y))

    (if (= separate-layer TRUE)
        (begin
          (set! effect-layer (car (ligma-layer-new image
                                                  select-width
                                                  select-height
                                                  type
                                                  "glow layer"
                                                  100
                                                  LAYER-MODE-NORMAL))
          )

          (ligma-layer-set-offsets effect-layer select-offset-x select-offset-y)
          (ligma-image-insert-layer image effect-layer 0 -1)
          (ligma-selection-none image)
          (ligma-drawable-edit-clear effect-layer)

          (ligma-image-select-item image CHANNEL-OP-REPLACE active-selection)
          (ligma-edit-copy 1 (vector drawable))

          (let* (
                 (pasted (ligma-edit-paste effect-layer FALSE))
                 (num-pasted (car pasted))
                 (floating-sel (aref (cadr pasted) (- num-pasted 1)))
                 )
            (ligma-floating-sel-anchor floating-sel)
          )

          (ligma-image-set-selected-layers image 1 (vector effect-layer))
        )
        (set! effect-layer drawable)
    )
    (set! active-layer effect-layer)

    ; all the fun stuff goes here
    (if (= pixelize TRUE)
        (plug-in-pixelize RUN-NONINTERACTIVE image active-layer pixel-size)
    )
    (plug-in-max-rgb RUN-NONINTERACTIVE image active-layer 0)
    (plug-in-edge RUN-NONINTERACTIVE image active-layer edge-amount 1 0)

    ; clean up the selection copy
    (ligma-image-select-item image CHANNEL-OP-REPLACE active-selection)

    (if (= keep-selection FALSE)
        (ligma-selection-none image)
    )

    (ligma-image-set-selected-layers image 1 (vector drawable))
    (ligma-image-remove-channel image active-selection)
    (ligma-image-undo-group-end image)
    (ligma-displays-flush)
    (ligma-context-pop)
  )
)

(script-fu-register "script-fu-predator"
  _"_Predator..."
  _"Add a 'Predator' effect to the selected region (or alpha)"
  "Adrian Likins <adrian@ligma.org>"
  "Adrian Likins"
  "10/12/97"
  "RGB*"
  SF-IMAGE       "Image"          0
  SF-DRAWABLE    "Drawable"       0
  SF-ADJUSTMENT _"Edge amount"    '(2 0 24 1 1 0 0)
  SF-TOGGLE     _"Pixelize"       TRUE
  SF-ADJUSTMENT _"Pixel amount"   '(3 1 16 1 1 0 0)
  SF-TOGGLE     _"Keep selection" TRUE
  SF-TOGGLE     _"Separate layer" TRUE
)

(script-fu-menu-register "script-fu-predator"
                         "<Image>/Filters/Artistic")
