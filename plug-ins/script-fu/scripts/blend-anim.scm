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
; blend-anim.scm   version 1.03   1999/12/21
;
; CHANGE-LOG:
; 1.00 - initial release
; 1.01 - some code cleanup, no real changes
; 1.02 - use ligma-message to output an error message if called
;        with less than three layers
; 1.03 - only call blur plugin when blut-radius >= 1.0
;
; Copyright (C) 1997-1999 Sven Neumann <sven@ligma.org>
;
;
; Blends two or more layers over a background, so that an animation can
; be saved. A minimum of three layers is required.

(define (script-fu-blend-anim img
                              drawable
                              frames
                              max-blur
                              looped)

  (define (multi-raise-layer image layer times)
    (while (> times 0)
       (ligma-image-raise-item image layer)
       (set! times (- times 1))
    )
  )

  (let* (
        (max-blur (max max-blur 0))
        (frames (max frames 0))
        (image (car (ligma-image-duplicate img)))
        (width (car (ligma-image-get-width image)))
        (height (car (ligma-image-get-height image)))
        (layers (ligma-image-get-layers image))
        (num-layers (car layers))
        (layer-array (cadr layers))
        (slots (- num-layers 2))
        (bg-layer (aref layer-array (- num-layers 1)))
        (max-width 0)
        (max-height 0)
        (offset-x 0)
        (offset-y 0)
        )

    (if (> num-layers 2)
        (begin
		  (ligma-image-undo-disable image)

		  (if (= looped TRUE)
			  ; add a copy of the lowest blend layer on top
			  (let* ((copy (car (ligma-layer-copy
						 (aref layer-array (- num-layers 2)) TRUE))))
				(ligma-image-insert-layer image copy 0 0)
				(set! layers (ligma-image-get-layers image))
				(set! num-layers (car layers))
				(set! layer-array (cadr layers))
				(set! slots (- num-layers 2))
				(set! bg-layer (aref layer-array (- num-layers 1)))))

		  ; make all layers invisible and check for sizes
		  (let* ((min-offset-x width)
				 (min-offset-y height)
				 (layer-count slots))
			(ligma-item-set-visible bg-layer FALSE)
			(while (> layer-count -1)
				   (let* ((layer (aref layer-array layer-count))
				  (layer-width (+ (car (ligma-drawable-get-width layer))
						  (* max-blur 2)))
				  (layer-height (+ (car (ligma-drawable-get-height layer))
						   (* max-blur 2)))
				  (layer-offsets (ligma-drawable-get-offsets layer))
				  (layer-offset-x (- (car layer-offsets) max-blur))
				  (layer-offset-y (- (cadr layer-offsets) max-blur)))
				 (ligma-item-set-visible layer FALSE)
				 (set! max-width (max max-width layer-width))
				 (set! max-height (max max-height layer-height))
				 (set! min-offset-x (min min-offset-x layer-offset-x))
				 (set! min-offset-y (min min-offset-y layer-offset-y))
				 (set! layer-count (- layer-count 1))))
			(set! offset-x (- (car (ligma-drawable-get-offsets bg-layer))
					  min-offset-x))
			(set! offset-y (- (cadr (ligma-drawable-get-offsets bg-layer))
					  min-offset-y)))

		  ; create intermediate frames by merging copies of adjacent layers
		  ; with the background layer
		  (let* ((layer-count slots))
			(while (> layer-count 0)
			   (let* ((frame-count frames)
				  (lower-layer (aref layer-array layer-count))
				  (upper-layer (aref layer-array (- layer-count 1))))
				 (while (> frame-count 0)
					(let* ((opacity (* (/ frame-count (+ frames 1)) 100))
				   (blur (/ (* opacity max-blur) 100))
				   (upper-copy (car (ligma-layer-copy upper-layer TRUE)))
				   (lower-copy (car (ligma-layer-copy lower-layer TRUE)))
				   (bg-copy (car (ligma-layer-copy bg-layer TRUE))))
				  (ligma-image-insert-layer image bg-copy 0 0)
				  (ligma-image-insert-layer image lower-copy 0 0)
				  (ligma-image-insert-layer image upper-copy 0 0)
				  (ligma-item-set-visible upper-copy TRUE)
				  (ligma-item-set-visible lower-copy TRUE)
				  (ligma-item-set-visible bg-copy TRUE)
				  (ligma-layer-set-opacity upper-copy (- 100 opacity))
				  (ligma-layer-set-opacity lower-copy opacity)
				  (ligma-layer-set-opacity bg-copy 100)
				  (if (> max-blur 0)
				  (let* ((layer-width (car (ligma-drawable-get-width upper-copy)))
						 (layer-height (car (ligma-drawable-get-height upper-copy))))
					(ligma-layer-set-lock-alpha upper-copy FALSE)
					(ligma-layer-resize upper-copy
							   (+ layer-width (* blur 2))
							   (+ layer-height (* blur 2))
							   blur
							   blur)
					(if (>= blur 1.0)
						(plug-in-gauss-rle RUN-NONINTERACTIVE
							   image
							   upper-copy
							   blur
							   TRUE TRUE))
					(set! blur (- max-blur blur))
					(ligma-layer-set-lock-alpha lower-copy FALSE)
					(set! layer-width (car (ligma-drawable-get-width
								lower-copy)))
					(set! layer-height (car (ligma-drawable-get-height
								 lower-copy)))
					(ligma-layer-resize lower-copy
							   (+ layer-width (* blur 2))
							   (+ layer-height (* blur 2))
							   blur
							   blur)
					(if (>= blur 1.0)
						(plug-in-gauss-rle RUN-NONINTERACTIVE
							   image
							   lower-copy
							   blur
							   TRUE TRUE))))
				  (ligma-layer-resize bg-copy
							 max-width
							 max-height
							 offset-x
							 offset-y)
				  (let* ((merged-layer (car (ligma-image-merge-visible-layers
							   image CLIP-TO-IMAGE))))
					(ligma-item-set-visible merged-layer FALSE))
				  (set! frame-count (- frame-count 1))))
				 (set! layer-count (- layer-count 1)))))

		  ; merge all original blend layers but the lowest one
			  ; with copies of the background layer
		  (let* ((layer-count 0))
			(while (< layer-count slots)
				   (let* ((orig-layer (aref layer-array layer-count))
				  (bg-copy (car (ligma-layer-copy bg-layer TRUE))))
				 (ligma-image-insert-layer image
						   bg-copy
						   -1
						   (* layer-count (+ frames 1)))
				 (multi-raise-layer image
						orig-layer
						(+ (* (- slots layer-count) frames) 1))
				 (ligma-item-set-visible orig-layer TRUE)
				 (ligma-item-set-visible bg-copy TRUE)
				 (ligma-layer-resize bg-copy
						max-width
						max-height
						offset-x
						offset-y)
				 (let* ((merged-layer (car (ligma-image-merge-visible-layers
						  image CLIP-TO-IMAGE))))
			   (ligma-item-set-visible merged-layer FALSE))
			   (set! layer-count (+ layer-count 1)))))

		  ; merge the lowest blend layer with the background layer
		  (let* ((orig-layer (aref layer-array (- num-layers 2))))
			(ligma-item-set-visible bg-layer TRUE)
			(ligma-item-set-visible orig-layer TRUE)
			(ligma-image-merge-visible-layers image CLIP-TO-IMAGE))

		  ; make all layers visible again
		  (let* ((result-layers (ligma-image-get-layers image))
				 (num-result-layers (car result-layers))
				 (result-layer-array (cadr result-layers))
				 (layer-count (- num-result-layers 1)))
			(while (> layer-count -1)
			   (let* ((layer (aref result-layer-array layer-count))
				  (name (string-append _"Frame" " "
						       (number->string
							(- num-result-layers layer-count) 10))))
				 (ligma-item-set-visible layer TRUE)
				 (ligma-item-set-name layer name)
				 (set! layer-count (- layer-count 1))))

			(if (= looped TRUE)
				; remove the topmost layer
				(ligma-image-remove-layer image (aref result-layer-array 0))))

		  (ligma-image-undo-enable image)
		  (ligma-display-new image)
		  (ligma-displays-flush)
		)

      (ligma-message _"Blend Animation needs at least three source layers")
    )
  )
)

(script-fu-register "script-fu-blend-anim"
    _"_Blend..."
    _"Create intermediate layers to blend two or more layers over a background as an animation"
    "Sven Neumann <sven@ligma.org>"
    "Sven Neumann"
    "1999/12/21"
    "RGB* GRAY*"
    SF-IMAGE       "Image"               0
    SF-DRAWABLE    "Drawable"            0
    SF-ADJUSTMENT _"Intermediate frames" '(3 1 1024 1 10 0 1)
    SF-ADJUSTMENT _"Max. blur radius"    '(0 0 1024 1 10 0 1)
    SF-TOGGLE     _"Looped"              TRUE
)

(script-fu-menu-register "script-fu-blend-anim"
                         "<Image>/Filters/Animation/Animators")
