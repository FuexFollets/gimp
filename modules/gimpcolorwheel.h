/* HSV color selector for GTK+
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Simon Budig <Simon.Budig@unix-ag.org> (original code)
 *          Federico Mena-Quintero <federico@ligma.org> (cleanup for GTK+)
 *          Jonathan Blandford <jrb@redhat.com> (cleanup for GTK+)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __LIGMA_COLOR_WHEEL_H__
#define __LIGMA_COLOR_WHEEL_H__

G_BEGIN_DECLS

#define LIGMA_TYPE_COLOR_WHEEL            (ligma_color_wheel_get_type ())
#define LIGMA_COLOR_WHEEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_COLOR_WHEEL, LigmaColorWheel))
#define LIGMA_COLOR_WHEEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_COLOR_WHEEL, LigmaColorWheelClass))
#define LIGMA_IS_COLOR_WHEEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_COLOR_WHEEL))
#define LIGMA_IS_COLOR_WHEEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_COLOR_WHEEL))
#define LIGMA_COLOR_WHEEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_COLOR_WHEEL, LigmaColorWheelClass))


typedef struct _LigmaColorWheel      LigmaColorWheel;
typedef struct _LigmaColorWheelClass LigmaColorWheelClass;

struct _LigmaColorWheel
{
  GtkWidget parent_instance;
};

struct _LigmaColorWheelClass
{
  GtkWidgetClass parent_class;

  /* Notification signals */
  void (* changed) (LigmaColorWheel   *wheel);

  /* Keybindings */
  void (* move)    (LigmaColorWheel   *wheel,
                    GtkDirectionType  type);

  /* Padding for future expansion */
  void (*_ligma_reserved1) (void);
  void (*_ligma_reserved2) (void);
  void (*_ligma_reserved3) (void);
  void (*_ligma_reserved4) (void);
};


void        color_wheel_register_type          (GTypeModule     *module);

GType       ligma_color_wheel_get_type          (void) G_GNUC_CONST;
GtkWidget * ligma_color_wheel_new               (void);

void        ligma_color_wheel_set_color         (LigmaColorWheel  *wheel,
                                                double           h,
                                                double           s,
                                                double           v);
void        ligma_color_wheel_get_color         (LigmaColorWheel  *wheel,
                                                gdouble         *h,
                                                gdouble         *s,
                                                gdouble         *v);

void        ligma_color_wheel_set_ring_fraction (LigmaColorWheel  *wheel,
                                                gdouble          fraction);
gdouble     ligma_color_wheel_get_ring_fraction (LigmaColorWheel  *wheel);

void        ligma_color_wheel_set_color_config  (LigmaColorWheel  *wheel,
                                                LigmaColorConfig *config);

gboolean    ligma_color_wheel_is_adjusting      (LigmaColorWheel  *wheel);

G_END_DECLS

#endif /* __LIGMA_COLOR_WHEEL_H__ */
