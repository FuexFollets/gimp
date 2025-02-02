; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Selection to Image
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; Takes the Current selection and saves it as a separate image.
;
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


(define (script-fu-selection-to-image image drawable)
  (let* (
        (draw-type (car (ligma-drawable-type-with-alpha drawable)))
        (image-type (car (ligma-image-get-base-type image)))
        (selection-bounds (ligma-selection-bounds image))
        (select-offset-x (cadr selection-bounds))
        (select-offset-y (caddr selection-bounds))
        (selection-width (- (cadr (cddr selection-bounds)) select-offset-x))
        (selection-height (- (caddr (cddr selection-bounds)) select-offset-y))
        (active-selection 0)
        (from-selection 0)
        (new-image 0)
        (new-draw 0)
        )

    (ligma-context-push)
    (ligma-context-set-defaults)

    (ligma-image-undo-disable image)

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

    (ligma-edit-copy 1 (vector drawable))

    (set! new-image (car (ligma-image-new selection-width
                                         selection-height image-type)))
    (set! new-draw (car (ligma-layer-new new-image
                                        selection-width selection-height
                                        draw-type "Selection" 100 LAYER-MODE-NORMAL)))
    (ligma-image-insert-layer new-image new-draw 0 0)
    (ligma-drawable-fill new-draw FILL-BACKGROUND)

    (let* (
           (pasted (ligma-edit-paste new-draw FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
      (ligma-floating-sel-anchor floating-sel)
    )

    (ligma-image-undo-enable image)
    (ligma-image-set-selected-layers image 1 (vector drawable))
    (ligma-display-new new-image)
    (ligma-displays-flush)

    (ligma-context-pop)
  )
)

(script-fu-register "script-fu-selection-to-image"
  _"To _Image"
  _"Convert a selection to an image"
  "Adrian Likins <adrian@ligma.org>"
  "Adrian Likins"
  "10/07/97"
  "RGB* GRAY*"
  SF-IMAGE "Image"       0
  SF-DRAWABLE "Drawable" 0
)
