; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
;
;
; drop-shadow.scm   version 1.05   2011/4/21
;
; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - fixed the problem with a remaining copy of the selection
; 1.02 - some code cleanup, no real changes
; 1.03 - can't call ligma-drawable-edit-fill until layer is added to image!
; 1.04
; 1.05 - replaced deprecated function calls with new ones for 2.8
;
; Copyright (C) 1997-1999 Sven Neumann <sven@ligma.org>
;
;
; Adds a drop-shadow of the current selection or alpha-channel.
;
; This script is derived from my script add-shadow, which has become
; obsolete now. Thanks to Andrew Donkin (ard@cs.waikato.ac.nz) for his
; idea to add alpha-support to add-shadow.


(define (script-fu-drop-shadow image
                               drawable
                               shadow-transl-x
                               shadow-transl-y
                               shadow-blur
                               shadow-color
                               shadow-opacity
                               allow-resize)
  (let* (
        (shadow-blur (max shadow-blur 0))
        (shadow-opacity (min shadow-opacity 100))
        (shadow-opacity (max shadow-opacity 0))
        (type (car (ligma-drawable-type-with-alpha drawable)))
        (image-width (car (ligma-image-get-width image)))
        (image-height (car (ligma-image-get-height image)))
        (from-selection 0)
        (active-selection 0)
        (shadow-layer 0)
        )

  (ligma-context-push)
  (ligma-context-set-defaults)

  (ligma-image-set-selected-layers image 1 (make-vector 1 drawable))

  (ligma-image-undo-group-start image)

  (ligma-layer-add-alpha drawable)
  (if (= (car (ligma-selection-is-empty image)) TRUE)
      (begin
        (ligma-image-select-item image CHANNEL-OP-REPLACE drawable)
        (set! from-selection FALSE))
      (begin
        (set! from-selection TRUE)
        (set! active-selection (car (ligma-selection-save image)))))

  (let* ((selection-bounds (ligma-selection-bounds image))
         (select-offset-x (cadr selection-bounds))
         (select-offset-y (caddr selection-bounds))
         (select-width (- (cadr (cddr selection-bounds)) select-offset-x))
         (select-height (- (caddr (cddr selection-bounds)) select-offset-y))

         (shadow-width (+ select-width (* 2 shadow-blur)))
         (shadow-height (+ select-height (* 2 shadow-blur)))

         (shadow-offset-x (- select-offset-x shadow-blur))
         (shadow-offset-y (- select-offset-y shadow-blur)))

    (if (= allow-resize TRUE)
        (let* ((new-image-width image-width)
               (new-image-height image-height)
               (image-offset-x 0)
               (image-offset-y 0))

          (if (< (+ shadow-offset-x shadow-transl-x) 0)
              (begin
                (set! image-offset-x (- 0 (+ shadow-offset-x
                                             shadow-transl-x)))
                (set! shadow-offset-x (- 0 shadow-transl-x))
                (set! new-image-width (+ new-image-width image-offset-x))))

          (if (< (+ shadow-offset-y shadow-transl-y) 0)
              (begin
                (set! image-offset-y (- 0 (+ shadow-offset-y
                                             shadow-transl-y)))
                (set! shadow-offset-y (- 0 shadow-transl-y))
                (set! new-image-height (+ new-image-height image-offset-y))))

          (if (> (+ (+ shadow-width shadow-offset-x) shadow-transl-x)
                 new-image-width)
              (set! new-image-width
                    (+ (+ shadow-width shadow-offset-x) shadow-transl-x)))

          (if (> (+ (+ shadow-height shadow-offset-y) shadow-transl-y)
                 new-image-height)
              (set! new-image-height
                    (+ (+ shadow-height shadow-offset-y) shadow-transl-y)))

          (ligma-image-resize image
                             new-image-width
                             new-image-height
                             image-offset-x
                             image-offset-y)
        )
    )

    (set! shadow-layer (car (ligma-layer-new image
                                            shadow-width
                                            shadow-height
                                            type
                                            "Drop Shadow"
                                            shadow-opacity
                                            LAYER-MODE-NORMAL)))
    (ligma-image-set-selected-layers image 1 (make-vector 1 drawable))
    (ligma-image-insert-layer image shadow-layer 0 -1)
    (ligma-layer-set-offsets shadow-layer
                            shadow-offset-x
                            shadow-offset-y))

  (ligma-drawable-fill shadow-layer FILL-TRANSPARENT)
  (ligma-context-set-background shadow-color)
  (ligma-drawable-edit-fill shadow-layer FILL-BACKGROUND)
  (ligma-selection-none image)
  (ligma-layer-set-lock-alpha shadow-layer FALSE)
  (if (>= shadow-blur 1.0) (plug-in-gauss-rle RUN-NONINTERACTIVE
                                              image
                                              shadow-layer
                                              shadow-blur
                                              TRUE
                                              TRUE))
  (ligma-item-transform-translate shadow-layer shadow-transl-x shadow-transl-y)

  (if (= from-selection TRUE)
      (begin
        (ligma-image-select-item image CHANNEL-OP-REPLACE active-selection)
        (ligma-drawable-edit-clear shadow-layer)
        (ligma-image-remove-channel image active-selection)))

  (if (and
       (= (car (ligma-layer-is-floating-sel drawable)) 0)
       (= from-selection FALSE))
      (ligma-image-raise-item image drawable))

  (ligma-image-set-selected-layers image 1 (make-vector 1 drawable))
  (ligma-image-undo-group-end image)
  (ligma-displays-flush)

  (ligma-context-pop)
  )
)

(script-fu-register "script-fu-drop-shadow"
  _"_Drop Shadow (legacy)..."
  _"Add a drop shadow to the selected region (or alpha)"
  "Sven Neumann <sven@ligma.org>"
  "Sven Neumann"
  "1999/12/21"
  "RGB* GRAY*"
  SF-IMAGE      "Image"           0
  SF-DRAWABLE   "Drawable"        0
  SF-ADJUSTMENT _"Offset X"       '(4 -4096 4096 1 10 0 1)
  SF-ADJUSTMENT _"Offset Y"       '(4 -4096 4096 1 10 0 1)
  SF-ADJUSTMENT _"Blur radius"    '(15 0 1024 1 10 0 1)
  SF-COLOR      _"Color"          "black"
  SF-ADJUSTMENT _"Opacity"        '(60 0 100 1 10 0 0)
  SF-TOGGLE     _"Allow resizing" TRUE
)

(script-fu-menu-register "script-fu-drop-shadow"
                         "<Image>/Filters/Light and Shadow/Shadow")
