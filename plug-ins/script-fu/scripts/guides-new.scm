;; -*-scheme-*-

;; Alan Horkan 2004.  Public Domain.
;; so long as remove this block of comments from your script
;; feel free to use it for whatever you like.

(define (script-fu-guide-new image
                             drawable
                             direction
                             position)
  (let* (
        (width (car (ligma-image-get-width image)))
        (height (car (ligma-image-get-height image)))
        )

    (if (= direction 0)
        ;; check position is inside the image boundaries
        (if (<= position height) (ligma-image-add-hguide image position))
        (if (<= position width) (ligma-image-add-vguide image position))
    )

    (ligma-displays-flush)
  )
)

(script-fu-register "script-fu-guide-new"
  _"New _Guide..."
  _"Add a guide at the orientation and position specified (in pixels)"
  "Alan Horkan"
  "Alan Horkan, 2004.  Public Domain."
  "2004-04-02"
  "*"
  SF-IMAGE      "Image"      0
  SF-DRAWABLE   "Drawable"   0
  SF-OPTION     _"_Direction" '(_"Horizontal" _"Vertical")
  SF-ADJUSTMENT _"_Position"  (list 0 0 MAX-IMAGE-SIZE 1 10 0 1)
)

(script-fu-menu-register "script-fu-guide-new"
                         "<Image>/Image/Guides")
