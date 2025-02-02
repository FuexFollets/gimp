/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmazoommodel.c
 * Copyright (C) 2005  David Odin <dindinx@ligma.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include "ligmawidgetstypes.h"

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"

#include "ligmahelpui.h"
#include "ligmawidgetsmarshal.h"
#include "ligmazoommodel.h"


/**
 * SECTION: ligmazoommodel
 * @title: LigmaZoomModel
 * @short_description: A model for zoom values.
 *
 * A model for zoom values.
 **/


#define ZOOM_MIN  (1.0 / 256.0)
#define ZOOM_MAX  (256.0)

enum
{
  ZOOMED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_VALUE,
  PROP_MINIMUM,
  PROP_MAXIMUM,
  PROP_FRACTION,
  PROP_PERCENTAGE,
  N_PROPS
};


struct _LigmaZoomModelPrivate
{
  gdouble  value;
  gdouble  minimum;
  gdouble  maximum;
};

#define GET_PRIVATE(obj) (((LigmaZoomModel *) (obj))->priv)


static void  ligma_zoom_model_set_property (GObject      *object,
                                           guint         property_id,
                                           const GValue *value,
                                           GParamSpec   *pspec);
static void  ligma_zoom_model_get_property (GObject      *object,
                                           guint         property_id,
                                           GValue       *value,
                                           GParamSpec   *pspec);


static guint zoom_model_signals[LAST_SIGNAL] = { 0, };
static GParamSpec *object_props[N_PROPS] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (LigmaZoomModel, ligma_zoom_model, G_TYPE_OBJECT)

#define parent_class ligma_zoom_model_parent_class


static void
ligma_zoom_model_class_init (LigmaZoomModelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  /**
   * LigmaZoomModel::zoomed:
   * @model: the object that received the signal
   * @old_factor: the zoom factor before it changes
   * @new_factor: the zoom factor after it has changed.
   *
   * Emitted when the zoom factor of the zoom model changes.
   */
  zoom_model_signals[ZOOMED] =
      g_signal_new ("zoomed",
                    G_TYPE_FROM_CLASS (klass),
                    G_SIGNAL_RUN_LAST,
                    G_STRUCT_OFFSET (LigmaZoomModelClass,
                                     zoomed),
                    NULL, NULL,
                    _ligma_widgets_marshal_VOID__DOUBLE_DOUBLE,
                    G_TYPE_NONE, 2,
                    G_TYPE_DOUBLE, G_TYPE_DOUBLE);

  object_class->set_property = ligma_zoom_model_set_property;
  object_class->get_property = ligma_zoom_model_get_property;

  /**
   * LigmaZoomModel:value:
   *
   * The zoom factor.
   */
  object_props[PROP_VALUE] = g_param_spec_double ("value",
                                                  "Value",
                                                  "Zoom factor",
                                                  ZOOM_MIN, ZOOM_MAX,
                                                  1.0,
                                                  LIGMA_PARAM_READWRITE);
  /**
   * LigmaZoomModel:minimum:
   *
   * The minimum zoom factor.
   */
  object_props[PROP_MINIMUM] = g_param_spec_double ("minimum",
                                                    "Minimum",
                                                    "Lower limit for the zoom factor",
                                                    ZOOM_MIN, ZOOM_MAX,
                                                    ZOOM_MIN,
                                                    LIGMA_PARAM_READWRITE);
  /**
   * LigmaZoomModel:maximum:
   *
   * The maximum zoom factor.
   */
  object_props[PROP_MAXIMUM] = g_param_spec_double ("maximum",
                                                    "Maximum",
                                                    "Upper limit for the zoom factor",
                                                    ZOOM_MIN, ZOOM_MAX,
                                                    ZOOM_MAX,
                                                    LIGMA_PARAM_READWRITE);

  /**
   * LigmaZoomModel:fraction:
   *
   * The zoom factor expressed as a fraction.
   */
  object_props[PROP_FRACTION] = g_param_spec_string ("fraction",
                                                     "Fraction",
                                                     "The zoom factor expressed as a fraction",
                                                     "1:1",
                                                     LIGMA_PARAM_READABLE);
  /**
   * LigmaZoomModel:percentage:
   *
   * The zoom factor expressed as percentage.
   */
  object_props[PROP_PERCENTAGE] = g_param_spec_string ("percentage",
                                                       "Percentage",
                                                       "The zoom factor expressed as a percentage",
                                                       "100%",
                                                       LIGMA_PARAM_READABLE);

  g_object_class_install_properties (object_class, N_PROPS, object_props);
}

static void
ligma_zoom_model_init (LigmaZoomModel *model)
{
  LigmaZoomModelPrivate *priv;

  model->priv = ligma_zoom_model_get_instance_private (model);

  priv = GET_PRIVATE (model);

  priv->value   = 1.0;
  priv->minimum = ZOOM_MIN;
  priv->maximum = ZOOM_MAX;
}

static void
ligma_zoom_model_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  LigmaZoomModelPrivate *priv  = GET_PRIVATE (object);
  gdouble               previous_value;

  previous_value = priv->value;
  g_object_freeze_notify (object);

  switch (property_id)
    {
    case PROP_VALUE:
      priv->value = g_value_get_double (value);

      g_object_notify_by_pspec (object, object_props[PROP_VALUE]);
      g_object_notify_by_pspec (object, object_props[PROP_FRACTION]);
      g_object_notify_by_pspec (object, object_props[PROP_PERCENTAGE]);
      break;

    case PROP_MINIMUM:
      priv->minimum = MIN (g_value_get_double (value), priv->maximum);
      break;

    case PROP_MAXIMUM:
      priv->maximum = MAX (g_value_get_double (value), priv->minimum);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }

  if (priv->value > priv->maximum || priv->value < priv->minimum)
    {
      priv->value = CLAMP (priv->value, priv->minimum, priv->maximum);

      g_object_notify_by_pspec (object, object_props[PROP_VALUE]);
      g_object_notify_by_pspec (object, object_props[PROP_FRACTION]);
      g_object_notify_by_pspec (object, object_props[PROP_PERCENTAGE]);
    }

  g_object_thaw_notify (object);

  if (priv->value != previous_value)
    {
      g_signal_emit (object, zoom_model_signals[ZOOMED],
                     0, previous_value, priv->value);
    }
}

static void
ligma_zoom_model_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  LigmaZoomModelPrivate *priv = GET_PRIVATE (object);
  gchar                *tmp;

  switch (property_id)
    {
    case PROP_VALUE:
      g_value_set_double (value, priv->value);
      break;

    case PROP_MINIMUM:
      g_value_set_double (value, priv->minimum);
      break;

    case PROP_MAXIMUM:
      g_value_set_double (value, priv->maximum);
      break;

    case PROP_FRACTION:
      {
        gint  numerator;
        gint  denominator;

        ligma_zoom_model_get_fraction (LIGMA_ZOOM_MODEL (object),
                                      &numerator, &denominator);

        tmp = g_strdup_printf ("%d:%d", numerator, denominator);
        g_value_set_string (value, tmp);
        g_free (tmp);
      }
      break;

    case PROP_PERCENTAGE:
      tmp = g_strdup_printf (priv->value >= 0.15 ? "%.0f%%" : "%.2f%%",
                             priv->value * 100.0);
      g_value_set_string (value, tmp);
      g_free (tmp);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_zoom_model_zoom_in (LigmaZoomModel *model)
{
  LigmaZoomModelPrivate *priv = GET_PRIVATE (model);

  if (priv->value < priv->maximum)
    ligma_zoom_model_zoom (model, LIGMA_ZOOM_IN, 0.0);
}

static void
ligma_zoom_model_zoom_out (LigmaZoomModel *model)
{
  LigmaZoomModelPrivate *priv = GET_PRIVATE (model);

  if (priv->value > priv->minimum)
    ligma_zoom_model_zoom (model, LIGMA_ZOOM_OUT, 0.0);
}

/**
 * ligma_zoom_model_new:
 *
 * Creates a new #LigmaZoomModel.
 *
 * Returns: a new #LigmaZoomModel.
 *
 * Since LIGMA 2.4
 **/
LigmaZoomModel *
ligma_zoom_model_new (void)
{
  return g_object_new (LIGMA_TYPE_ZOOM_MODEL, NULL);
}


/**
 * ligma_zoom_model_set_range:
 * @model: a #LigmaZoomModel
 * @min: new lower limit for zoom factor
 * @max: new upper limit for zoom factor
 *
 * Sets the allowed range of the @model.
 *
 * Since LIGMA 2.4
 **/
void
ligma_zoom_model_set_range (LigmaZoomModel *model,
                           gdouble        min,
                           gdouble        max)
{
  g_return_if_fail (LIGMA_IS_ZOOM_MODEL (model));
  g_return_if_fail (min < max);
  g_return_if_fail (min >= ZOOM_MIN);
  g_return_if_fail (max <= ZOOM_MAX);

  g_object_set (model,
                "minimum", min,
                "maximum", max,
                NULL);
}

/**
 * ligma_zoom_model_zoom:
 * @model:     a #LigmaZoomModel
 * @zoom_type: the #LigmaZoomType
 * @scale:     ignored unless @zoom_type == %LIGMA_ZOOM_TO
 *
 * Since LIGMA 2.4
 **/
void
ligma_zoom_model_zoom (LigmaZoomModel *model,
                      LigmaZoomType   zoom_type,
                      gdouble        scale)
{
  gdouble delta = 0.0;

  g_return_if_fail (LIGMA_IS_ZOOM_MODEL (model));

  if (zoom_type == LIGMA_ZOOM_SMOOTH)
    delta = scale;

  if (zoom_type != LIGMA_ZOOM_TO)
    scale = ligma_zoom_model_get_factor (model);

  g_object_set (model,
                "value", ligma_zoom_model_zoom_step (zoom_type, scale, delta),
                NULL);
}

/**
 * ligma_zoom_model_get_factor:
 * @model: a #LigmaZoomModel
 *
 * Retrieves the current zoom factor of @model.
 *
 * Returns: the current scale factor
 *
 * Since LIGMA 2.4
 **/
gdouble
ligma_zoom_model_get_factor (LigmaZoomModel *model)
{
  g_return_val_if_fail (LIGMA_IS_ZOOM_MODEL (model), 1.0);

  return GET_PRIVATE (model)->value;
}


/**
 * ligma_zoom_model_get_fraction
 * @model:       a #LigmaZoomModel
 * @numerator: (out): return location for numerator
 * @denominator: (out): return location for denominator
 *
 * Retrieves the current zoom factor of @model as a fraction.
 *
 * Since LIGMA 2.4
 **/
void
ligma_zoom_model_get_fraction (LigmaZoomModel *model,
                              gint          *numerator,
                              gint          *denominator)
{
  gint     p0, p1, p2;
  gint     q0, q1, q2;
  gdouble  zoom_factor;
  gdouble  remainder, next_cf;
  gboolean swapped = FALSE;

  g_return_if_fail (LIGMA_IS_ZOOM_MODEL (model));
  g_return_if_fail (numerator != NULL && denominator != NULL);

  zoom_factor = ligma_zoom_model_get_factor (model);

  /* make sure that zooming behaves symmetrically */
  if (zoom_factor < 1.0)
    {
      zoom_factor = 1.0 / zoom_factor;
      swapped = TRUE;
    }

  /* calculate the continued fraction for the desired zoom factor */

  p0 = 1;
  q0 = 0;
  p1 = floor (zoom_factor);
  q1 = 1;

  remainder = zoom_factor - p1;

  while (fabs (remainder) >= 0.0001 &&
         fabs (((gdouble) p1 / q1) - zoom_factor) > 0.0001)
    {
      remainder = 1.0 / remainder;

      next_cf = floor (remainder);

      p2 = next_cf * p1 + p0;
      q2 = next_cf * q1 + q0;

      /* Numerator and Denominator are limited by 256 */
      /* also absurd ratios like 170:171 are excluded */
      if (p2 > 256 || q2 > 256 || (p2 > 1 && q2 > 1 && p2 * q2 > 200))
        break;

      /* remember the last two fractions */
      p0 = p1;
      p1 = p2;
      q0 = q1;
      q1 = q2;

      remainder = remainder - next_cf;
    }

  zoom_factor = (gdouble) p1 / q1;

  /* hard upper and lower bounds for zoom ratio */

  if (zoom_factor > 256.0)
    {
      p1 = 256;
      q1 = 1;
    }
  else if (zoom_factor < 1.0 / 256.0)
    {
      p1 = 1;
      q1 = 256;
    }

  if (swapped)
    {
      *numerator = q1;
      *denominator = p1;
    }
  else
    {
      *numerator = p1;
      *denominator = q1;
    }
}

static GtkWidget *
zoom_button_new (const gchar *icon_name,
                 GtkIconSize  icon_size)
{
  GtkWidget *button;
  GtkWidget *image;

  image = gtk_image_new_from_icon_name (icon_name,
                                        icon_size > 0 ?
                                        icon_size : GTK_ICON_SIZE_BUTTON);

  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  return button;
}

static void
zoom_in_button_callback (LigmaZoomModel *model,
                         gdouble        old,
                         gdouble        new,
                         GtkWidget     *button)
{
  LigmaZoomModelPrivate *priv = GET_PRIVATE (model);

  gtk_widget_set_sensitive (button, priv->value != priv->maximum);
}

static void
zoom_out_button_callback (LigmaZoomModel *model,
                          gdouble        old,
                          gdouble        new,
                          GtkWidget     *button)
{
  LigmaZoomModelPrivate *priv = GET_PRIVATE (model);

  gtk_widget_set_sensitive (button, priv->value != priv->minimum);
}

/**
 * ligma_zoom_button_new:
 * @model:     a #LigmaZoomModel
 * @zoom_type:
 * @icon_size: use 0 for a button with text labels
 *
 * Returns: (transfer full): a newly created GtkButton
 *
 * Since LIGMA 2.4
 **/
GtkWidget *
ligma_zoom_button_new (LigmaZoomModel *model,
                      LigmaZoomType   zoom_type,
                      GtkIconSize    icon_size)
{
  GtkWidget *button = NULL;

  g_return_val_if_fail (LIGMA_IS_ZOOM_MODEL (model), NULL);

  switch (zoom_type)
    {
    case LIGMA_ZOOM_IN:
      button = zoom_button_new ("zoom-in", icon_size);
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (ligma_zoom_model_zoom_in),
                                model);
      g_signal_connect_object (model, "zoomed",
                               G_CALLBACK (zoom_in_button_callback),
                               button, 0);
      break;

    case LIGMA_ZOOM_OUT:
      button = zoom_button_new ("zoom-out", icon_size);
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (ligma_zoom_model_zoom_out),
                                model);
      g_signal_connect_object (model, "zoomed",
                               G_CALLBACK (zoom_out_button_callback),
                               button, 0);
      break;

    default:
      g_warning ("sorry, no button for this zoom type (%d)", zoom_type);
      break;
    }

  if (button)
    {
      gdouble zoom = ligma_zoom_model_get_factor (model);

      /*  set initial button sensitivity  */
      g_signal_emit (model, zoom_model_signals[ZOOMED], 0, zoom, zoom);

      if (icon_size > 0)
        {
          const gchar *desc;

          if (ligma_enum_get_value (LIGMA_TYPE_ZOOM_TYPE, zoom_type,
                                   NULL, NULL, &desc, NULL))
            {
              ligma_help_set_help_data (button, desc, NULL);
            }
        }
    }

  return button;
}

/**
 * ligma_zoom_model_zoom_step:
 * @zoom_type: the zoom type
 * @scale:     ignored unless @zoom_type == %LIGMA_ZOOM_TO
 * @delta:     the delta from a smooth zoom event
 *
 * Utility function to calculate a new scale factor.
 *
 * Returns: the new scale factor
 *
 * Since LIGMA 2.4
 **/
gdouble
ligma_zoom_model_zoom_step (LigmaZoomType zoom_type,
                           gdouble      scale,
                           gdouble      delta)
{
  gint    i, n_presets;
  gdouble new_scale = 1.0;

  /* This table is constructed to have fractions, that approximate
   * sqrt(2)^k. This gives a smooth feeling regardless of the starting
   * zoom level.
   *
   * Zooming in/out always jumps to a zoom step from the list below.
   * However, we try to guarantee a certain size of the step, to
   * avoid silly jumps from 101% to 100%.
   *
   * The factor 1.1 is chosen a bit arbitrary, but feels better
   * than the geometric median of the zoom steps (2^(1/4)).
   */

#define ZOOM_MIN_STEP 1.1

  const gdouble presets[] = {
    1.0 / 256, 1.0 / 180, 1.0 / 128, 1.0 / 90,
    1.0 / 64,  1.0 / 45,  1.0 / 32,  1.0 / 23,
    1.0 / 16,  1.0 / 11,  1.0 / 8,   2.0 / 11,
    1.0 / 4,   1.0 / 3,   1.0 / 2,   2.0 / 3,
      1.0,
               3.0 / 2,      2.0,      3.0,
      4.0,    11.0 / 2,      8.0,     11.0,
      16.0,     23.0,       32.0,     45.0,
      64.0,     90.0,      128.0,    180.0,
      256.0,
  };

  n_presets = G_N_ELEMENTS (presets);

  switch (zoom_type)
    {
    case LIGMA_ZOOM_IN:
      scale *= ZOOM_MIN_STEP;

      new_scale = presets[n_presets - 1];
      for (i = n_presets - 1; i >= 0 && presets[i] > scale; i--)
        new_scale = presets[i];

      break;

    case LIGMA_ZOOM_OUT:
      scale /= ZOOM_MIN_STEP;

      new_scale = presets[0];
      for (i = 0; i < n_presets && presets[i] < scale; i++)
        new_scale = presets[i];

      break;

    case LIGMA_ZOOM_IN_MORE:
      scale = ligma_zoom_model_zoom_step (LIGMA_ZOOM_IN, scale, 0.0);
      scale = ligma_zoom_model_zoom_step (LIGMA_ZOOM_IN, scale, 0.0);
      scale = ligma_zoom_model_zoom_step (LIGMA_ZOOM_IN, scale, 0.0);
      new_scale = scale;
      break;

    case LIGMA_ZOOM_OUT_MORE:
      scale = ligma_zoom_model_zoom_step (LIGMA_ZOOM_OUT, scale, 0.0);
      scale = ligma_zoom_model_zoom_step (LIGMA_ZOOM_OUT, scale, 0.0);
      scale = ligma_zoom_model_zoom_step (LIGMA_ZOOM_OUT, scale, 0.0);
      new_scale = scale;
      break;

    case LIGMA_ZOOM_IN_MAX:
      new_scale = ZOOM_MAX;
      break;

    case LIGMA_ZOOM_OUT_MAX:
      new_scale = ZOOM_MIN;
      break;

    case LIGMA_ZOOM_TO:
      new_scale = scale;
      break;

    case LIGMA_ZOOM_SMOOTH:
      if (delta > 0.0)
        new_scale = scale * (1.0 + 0.1 * delta);
      else if (delta < 0.0)
        new_scale = scale / (1.0 + 0.1 * -delta);
      else
        new_scale = scale;
      break;

    case LIGMA_ZOOM_PINCH:
      if (delta > 0.0)
        new_scale = scale * (1.0 + delta);
      else if (delta < 0.0)
        new_scale = scale / (1.0 + -delta);
      else
        new_scale = scale;
      break;
    }

  return CLAMP (new_scale, ZOOM_MIN, ZOOM_MAX);

#undef ZOOM_MIN_STEP
}
