; LIGMA - The GNU Image Manipulation Program
; Copyright (C) 1995 Spencer Kimball and Peter Mattis
;
; Gradient example script --- create an example image of a custom gradient
; Copyright (C) 1997 Federico Mena Quintero
; federico@nuclecu.unam.mx
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

(define (script-fu-gradient-example width
                                    height
                                    gradient-reverse)
  (let* (
        (img (car (ligma-image-new width height RGB)))
        (drawable (car (ligma-layer-new img width height RGB
                                       "Gradient example" 100 LAYER-MODE-NORMAL)))

        ; Calculate colors for checkerboard... just like in the gradient editor

        (fg-color (* 255 (/ 2 3)))
        (bg-color (* 255 (/ 1 3)))
        )

    (ligma-image-undo-disable img)
    (ligma-image-insert-layer img drawable 0 0)

    ; Render background checkerboard

    (ligma-context-push)

    (ligma-context-set-foreground (list fg-color fg-color fg-color))
    (ligma-context-set-background (list bg-color bg-color bg-color))
    (plug-in-checkerboard RUN-NONINTERACTIVE img 1 (vector drawable) 0 8)

    (ligma-context-pop)

    ; Render gradient

    (ligma-context-push)

    (ligma-context-set-gradient-reverse gradient-reverse)
    (ligma-drawable-edit-gradient-fill drawable
				      GRADIENT-LINEAR 0
				      FALSE 0 0
				      TRUE
				      0 0 (- width 1) 0)

    (ligma-context-pop)

    ; Terminate

    (ligma-image-undo-enable img)
    (ligma-display-new img)
  )
)

(script-fu-register "script-fu-gradient-example"
    _"Custom _Gradient..."
    _"Create an image filled with an example of the current gradient"
    "Federico Mena Quintero"
    "Federico Mena Quintero"
    "June 1997"
    ""
    SF-ADJUSTMENT _"Width"            '(400 1 2000 1 10 0 1)
    SF-ADJUSTMENT _"Height"           '(30 1 2000 1 10 0 1)
    SF-TOGGLE     _"Gradient reverse" FALSE
)

(script-fu-menu-register "script-fu-gradient-example"
                         "<Gradients>")
