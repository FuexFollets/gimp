; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Selection-to-brush
; Copyright (c) 1997 Adrian Likins
; aklikins@eos.ncsu.edu
;
; Takes the current selection, saves it as a brush, and makes it the
; active brush..
;
;       Parts of this script from Sven Neuman's Drop-Shadow and
;       Seth Burgess's mkbrush scripts.
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


(define (script-fu-selection-to-brush image
                                      drawable
                                      name
                                      filename
                                      spacing)
  (let* (
        (type (car (ligma-drawable-type-with-alpha drawable)))
        (selection-bounds (ligma-selection-bounds image))
        (select-offset-x  (cadr selection-bounds))
        (select-offset-y  (caddr selection-bounds))
        (selection-width  (- (cadr (cddr selection-bounds))  select-offset-x))
        (selection-height (- (caddr (cddr selection-bounds)) select-offset-y))
        (from-selection 0)
        (active-selection 0)
        (brush-draw-type 0)
        (brush-image-type 0)
        (brush-image 0)
        (brush-draw 0)
        (filename2 0)
        )

    (ligma-context-push)
    (ligma-context-set-defaults)

    (ligma-image-undo-disable image)

    (if (= (car (ligma-selection-is-empty image)) TRUE)
        (begin
          (ligma-image-select-item image CHANNEL-OP-REPLACE drawable)
          (set! from-selection FALSE)
        )
        (begin
          (set! from-selection TRUE)
          (set! active-selection (car (ligma-selection-save image)))
        )
    )

    (ligma-edit-copy 1 (make-vector 1 drawable))

    (set! brush-draw-type
          (if (= type GRAYA-IMAGE)
              GRAY-IMAGE
              RGBA-IMAGE))

    (set! brush-image-type
          (if (= type GRAYA-IMAGE)
              GRAY
              RGB))

    (set! brush-image (car (ligma-image-new selection-width
                                           selection-height
                                           brush-image-type)))

    (set! brush-draw
          (car (ligma-layer-new brush-image
                               selection-width
                               selection-height
                               brush-draw-type
                               "Brush"
                               100
                               LAYER-MODE-NORMAL)))

    (ligma-image-insert-layer brush-image brush-draw 0 0)

    (ligma-selection-none brush-image)

    (if (= type GRAYA-IMAGE)
        (begin
          (ligma-context-set-background '(255 255 255))
          (ligma-drawable-fill brush-draw FILL-BACKGROUND))
        (ligma-drawable-fill brush-draw FILL-TRANSPARENT)
    )

    (let* (
           (pasted (ligma-edit-paste brush-draw FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
      (ligma-floating-sel-anchor floating-sel)
    )

    (set! filename2 (string-append ligma-directory
                                   "/brushes/"
                                   filename
                                   (number->string image)
                                   ".gbr"))

    (file-gbr-save 1 brush-image 1 (make-vector 1 brush-draw) filename2 spacing name)

    (if (= from-selection TRUE)
        (begin
          (ligma-image-select-item image CHANNEL-OP-REPLACE active-selection)
          (ligma-image-remove-channel image active-selection)
        )
    )

    (ligma-image-undo-enable image)
    (ligma-image-set-selected-layers image 1 (make-vector 1 drawable))
    (ligma-image-delete brush-image)
    (ligma-displays-flush)

    (ligma-context-pop)

    (ligma-brushes-refresh)
    (ligma-context-set-brush name)
  )
)

(script-fu-register "script-fu-selection-to-brush"
  _"To _Brush..."
  _"Convert a selection to a brush"
  "Adrian Likins <adrian@ligma.org>"
  "Adrian Likins"
  "10/07/97"
  "RGB* GRAY*"
  SF-IMAGE       "Image"       0
  SF-DRAWABLE    "Drawable"    0
  SF-STRING     _"_Brush name"  "My Brush"
  SF-STRING     _"_File name"   "mybrush"
  SF-ADJUSTMENT _"_Spacing"     '(25 0 1000 1 1 1 0)
)
