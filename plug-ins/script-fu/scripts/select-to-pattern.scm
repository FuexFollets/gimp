; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Based on select-to-brush by
;         Copyright (c) 1997 Adrian Likins aklikins@eos.ncsu.edu
; Author Cameron Gregory, http://www.flamingtext.com/
;
; Takes the current selection, saves it as a pattern and makes it the active
; pattern
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


(define (script-fu-selection-to-pattern image drawable desc filename)

  (let* (
        (selection-width 0)
        (selection-height 0)
        (selection-bounds 0)
        (select-offset-x 0)
        (select-offset-y 0)
        (pattern-draw-type 0)
        (pattern-image-type 0)
        (pattern-image 0)
        (pattern-draw 0)
        (filename2 0)
        )

  (if (= (car (ligma-selection-is-empty image)) TRUE)
      (begin
        (set! selection-width (car (ligma-drawable-get-width drawable)))
        (set! selection-height (car (ligma-drawable-get-height drawable)))
      )
      (begin
        (set! selection-bounds (ligma-drawable-mask-bounds drawable))
        (set! select-offset-x (cadr selection-bounds))
        (set! select-offset-y (caddr selection-bounds))
        (set! selection-width (- (cadr (cddr selection-bounds)) select-offset-x))
        (set! selection-height (- (caddr (cddr selection-bounds)) select-offset-y))
      )
  )

  (if (= (car (ligma-drawable-has-alpha drawable)) TRUE)
      (set! pattern-draw-type RGBA-IMAGE)
      (set! pattern-draw-type RGB-IMAGE)
  )

  (set! pattern-image-type RGB)

  (set! pattern-image (car (ligma-image-new selection-width selection-height
                                           pattern-image-type)))

  (set! pattern-draw
        (car (ligma-layer-new pattern-image selection-width selection-height
                             pattern-draw-type "Pattern" 100 LAYER-MODE-NORMAL)))

  (ligma-drawable-fill pattern-draw FILL-TRANSPARENT)

  (ligma-image-insert-layer pattern-image pattern-draw 0 0)

  (ligma-edit-copy 1 (vector drawable))

  (let* (
           (pasted (ligma-edit-paste pattern-draw FALSE))
           (num-pasted (car pasted))
           (floating-sel (aref (cadr pasted) (- num-pasted 1)))
          )
    (ligma-floating-sel-anchor floating-sel)
  )

  (set! filename2 (string-append ligma-directory
                                 "/patterns/"
                                 filename
                                 (number->string image)
                                 ".pat"))

  (file-pat-save 1 pattern-image 1 (vector pattern-draw) filename2 desc)
  (ligma-patterns-refresh)
  (ligma-context-set-pattern desc)

  (ligma-image-delete pattern-image)
  (ligma-displays-flush)
  )
)

(script-fu-register "script-fu-selection-to-pattern"
  _"To _Pattern..."
  _"Convert a selection to a pattern"
  "Cameron Gregory <cameron@bloke.com>"
  "Cameron Gregory"
  "09/02/2003"
  "RGB* GRAY*"
  SF-IMAGE     "Image"        0
  SF-DRAWABLE  "Drawable"     0
  SF-STRING   _"_Pattern name" "My Pattern"
  SF-STRING   _"_File name"    "mypattern"
)
