; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Circuit board effect
; Copyright (c) 1997 Adrian Likins
;
; Generates what looks a little like the back of an old circuit board.
; Looks even better when gradient-mapp'ed with a suitable gradient.
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


(define (script-fu-circuit image
                           drawable
                           mask-size
                           seed
                           remove-bg
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
          (set! from-selection FALSE))
        (begin
          (set! from-selection TRUE)
          (set! active-selection (car (ligma-selection-save image)))))

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
                                                  _"Effect layer"
                                                  100
                                                  LAYER-MODE-NORMAL)))

          (ligma-image-insert-layer image effect-layer 0 -1)
          (ligma-layer-set-offsets effect-layer select-offset-x select-offset-y)
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
          (ligma-image-set-selected-layers image 1 (vector effect-layer)))
          (set! effect-layer drawable)
    )
    (set! active-layer effect-layer)

    (if (= remove-bg TRUE)
        (ligma-context-set-foreground '(0 0 0))
        (ligma-context-set-foreground '(14 14 14))
    )

    (ligma-image-select-item image CHANNEL-OP-REPLACE active-selection)
    (plug-in-maze RUN-NONINTERACTIVE image active-layer 5 5 TRUE 0 seed 57 1)
    (plug-in-oilify RUN-NONINTERACTIVE image active-layer mask-size 0)
    (plug-in-edge RUN-NONINTERACTIVE image active-layer 2 1 0)
    (if (= type RGBA-IMAGE)
      (ligma-drawable-desaturate active-layer DESATURATE-LIGHTNESS))

    (if (and
         (= remove-bg TRUE)
         (= separate-layer TRUE))
        (begin
          (ligma-image-select-color image CHANNEL-OP-REPLACE active-layer '(0 0 0))
          (ligma-drawable-edit-clear active-layer)))

    (if (= keep-selection FALSE)
        (ligma-selection-none image))

    (ligma-image-remove-channel image active-selection)
    (ligma-image-set-selected-layers image 1 (vector drawable))

    (ligma-image-undo-group-end image)

    (ligma-displays-flush)

    (ligma-context-pop)
  )
)

(script-fu-register "script-fu-circuit"
  _"_Circuit..."
  _"Fill the selected region (or alpha) with traces like those on a circuit board"
  "Adrian Likins <adrian@ligma.org>"
  "Adrian Likins"
  "10/17/97"
  "RGB* GRAY*"
  SF-IMAGE      "Image"             0
  SF-DRAWABLE   "Drawable"          0
  SF-ADJUSTMENT _"Oilify mask size" '(17 3 50 1 10 0 1)
  SF-ADJUSTMENT _"Circuit seed"     '(3 1 3000000 1 10 0 1)
  SF-TOGGLE     _"No background (only for separate layer)" FALSE
  SF-TOGGLE     _"Keep selection"   TRUE
  SF-TOGGLE     _"Separate layer"   TRUE
)

(script-fu-menu-register "script-fu-circuit"
                         "<Image>/Filters/Render")
