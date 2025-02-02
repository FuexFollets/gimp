/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "paint/ligmapaintoptions.h"


#include "ligmacurve.h"
#include "ligmacurve-map.h"

#include "ligmadynamicsoutput.h"

#include "ligma-intl.h"


#define DEFAULT_USE_PRESSURE  FALSE
#define DEFAULT_USE_VELOCITY  FALSE
#define DEFAULT_USE_DIRECTION FALSE
#define DEFAULT_USE_TILT      FALSE
#define DEFAULT_USE_WHEEL     FALSE
#define DEFAULT_USE_RANDOM    FALSE
#define DEFAULT_USE_FADE      FALSE


enum
{
  PROP_0,

  PROP_TYPE,
  PROP_USE_PRESSURE,
  PROP_USE_VELOCITY,
  PROP_USE_DIRECTION,
  PROP_USE_TILT,
  PROP_USE_WHEEL,
  PROP_USE_RANDOM,
  PROP_USE_FADE,
  PROP_PRESSURE_CURVE,
  PROP_VELOCITY_CURVE,
  PROP_DIRECTION_CURVE,
  PROP_TILT_CURVE,
  PROP_WHEEL_CURVE,
  PROP_RANDOM_CURVE,
  PROP_FADE_CURVE
};


typedef struct _LigmaDynamicsOutputPrivate LigmaDynamicsOutputPrivate;

struct _LigmaDynamicsOutputPrivate
{
  LigmaDynamicsOutputType  type;

  gboolean                use_pressure;
  gboolean                use_velocity;
  gboolean                use_direction;
  gboolean                use_tilt;
  gboolean                use_wheel;
  gboolean                use_random;
  gboolean                use_fade;

  LigmaCurve              *pressure_curve;
  LigmaCurve              *velocity_curve;
  LigmaCurve              *direction_curve;
  LigmaCurve              *tilt_curve;
  LigmaCurve              *wheel_curve;
  LigmaCurve              *random_curve;
  LigmaCurve              *fade_curve;
};

#define GET_PRIVATE(output) \
        ((LigmaDynamicsOutputPrivate *) ligma_dynamics_output_get_instance_private ((LigmaDynamicsOutput *) (output)))


static void   ligma_dynamics_output_finalize     (GObject           *object);
static void   ligma_dynamics_output_set_property (GObject           *object,
                                                 guint              property_id,
                                                 const GValue      *value,
                                                 GParamSpec        *pspec);
static void   ligma_dynamics_output_get_property (GObject           *object,
                                                 guint              property_id,
                                                 GValue            *value,
                                                 GParamSpec        *pspec);
static void   ligma_dynamics_output_copy_curve   (LigmaCurve          *src,
                                                 LigmaCurve          *dest);

static LigmaCurve *
              ligma_dynamics_output_create_curve (LigmaDynamicsOutput *output,
                                                 const gchar        *name);
static void   ligma_dynamics_output_curve_dirty  (LigmaCurve          *curve,
                                                 LigmaDynamicsOutput *output);


G_DEFINE_TYPE_WITH_CODE (LigmaDynamicsOutput, ligma_dynamics_output,
                         LIGMA_TYPE_OBJECT,
                         G_ADD_PRIVATE (LigmaDynamicsOutput)
                         G_IMPLEMENT_INTERFACE (LIGMA_TYPE_CONFIG, NULL))

#define parent_class ligma_dynamics_output_parent_class


static void
ligma_dynamics_output_class_init (LigmaDynamicsOutputClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize     = ligma_dynamics_output_finalize;
  object_class->set_property = ligma_dynamics_output_set_property;
  object_class->get_property = ligma_dynamics_output_get_property;

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_enum ("type", NULL,
                                                      _("Output type"),
                                                      LIGMA_TYPE_DYNAMICS_OUTPUT_TYPE,
                                                      LIGMA_DYNAMICS_OUTPUT_OPACITY,
                                                      LIGMA_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT));

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_PRESSURE,
                            "use-pressure",
                            NULL, NULL,
                            DEFAULT_USE_PRESSURE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_VELOCITY,
                            "use-velocity",
                            NULL, NULL,
                            DEFAULT_USE_VELOCITY,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_DIRECTION,
                            "use-direction",
                            NULL, NULL,
                            DEFAULT_USE_DIRECTION,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_TILT,
                            "use-tilt",
                            NULL, NULL,
                            DEFAULT_USE_TILT,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_WHEEL,
                            "use-wheel",
                            NULL, NULL,
                            DEFAULT_USE_TILT,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_RANDOM,
                            "use-random",
                            NULL, NULL,
                            DEFAULT_USE_RANDOM,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_BOOLEAN (object_class, PROP_USE_FADE,
                            "use-fade",
                            NULL, NULL,
                            DEFAULT_USE_FADE,
                            LIGMA_PARAM_STATIC_STRINGS);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_PRESSURE_CURVE,
                           "pressure-curve",
                            NULL, NULL,
                           LIGMA_TYPE_CURVE,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_VELOCITY_CURVE,
                           "velocity-curve",
                            NULL, NULL,
                           LIGMA_TYPE_CURVE,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_DIRECTION_CURVE,
                           "direction-curve",
                            NULL, NULL,
                           LIGMA_TYPE_CURVE,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_TILT_CURVE,
                           "tilt-curve",
                            NULL, NULL,
                           LIGMA_TYPE_CURVE,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_WHEEL_CURVE,
                           "wheel-curve",
                            NULL, NULL,
                           LIGMA_TYPE_CURVE,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_RANDOM_CURVE,
                           "random-curve",
                            NULL, NULL,
                           LIGMA_TYPE_CURVE,
                           LIGMA_CONFIG_PARAM_AGGREGATE);

  LIGMA_CONFIG_PROP_OBJECT (object_class, PROP_FADE_CURVE,
                           "fade-curve",
                            NULL, NULL,
                           LIGMA_TYPE_CURVE,
                           LIGMA_CONFIG_PARAM_AGGREGATE);
}

static void
ligma_dynamics_output_init (LigmaDynamicsOutput *output)
{
  LigmaDynamicsOutputPrivate *private = GET_PRIVATE (output);

  private->pressure_curve  = ligma_dynamics_output_create_curve (output,
                                                                "pressure-curve");
  private->velocity_curve  = ligma_dynamics_output_create_curve (output,
                                                                "velocity-curve");
  private->direction_curve = ligma_dynamics_output_create_curve (output,
                                                                "direction-curve");
  private->tilt_curve      = ligma_dynamics_output_create_curve (output,
                                                                "tilt-curve");
  private->wheel_curve     = ligma_dynamics_output_create_curve (output,
                                                                "wheel-curve");
  private->random_curve    = ligma_dynamics_output_create_curve (output,
                                                                "random-curve");
  private->fade_curve      = ligma_dynamics_output_create_curve (output,
                                                                "fade-curve");
}

static void
ligma_dynamics_output_finalize (GObject *object)
{
  LigmaDynamicsOutputPrivate *private = GET_PRIVATE (object);

  g_clear_object (&private->pressure_curve);
  g_clear_object (&private->velocity_curve);
  g_clear_object (&private->direction_curve);
  g_clear_object (&private->tilt_curve);
  g_clear_object (&private->wheel_curve);
  g_clear_object (&private->random_curve);
  g_clear_object (&private->fade_curve);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_dynamics_output_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  LigmaDynamicsOutputPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TYPE:
      private->type = g_value_get_enum (value);
      break;

    case PROP_USE_PRESSURE:
      private->use_pressure = g_value_get_boolean (value);
      break;

    case PROP_USE_VELOCITY:
      private->use_velocity = g_value_get_boolean (value);
      break;

    case PROP_USE_DIRECTION:
      private->use_direction = g_value_get_boolean (value);
      break;

    case PROP_USE_TILT:
      private->use_tilt = g_value_get_boolean (value);
      break;

    case PROP_USE_WHEEL:
      private->use_wheel = g_value_get_boolean (value);
      break;

    case PROP_USE_RANDOM:
      private->use_random = g_value_get_boolean (value);
      break;

    case PROP_USE_FADE:
      private->use_fade = g_value_get_boolean (value);
      break;

    case PROP_PRESSURE_CURVE:
      ligma_dynamics_output_copy_curve (g_value_get_object (value),
                                       private->pressure_curve);
      break;

    case PROP_VELOCITY_CURVE:
      ligma_dynamics_output_copy_curve (g_value_get_object (value),
                                       private->velocity_curve);
      break;

    case PROP_DIRECTION_CURVE:
      ligma_dynamics_output_copy_curve (g_value_get_object (value),
                                       private->direction_curve);
      break;

    case PROP_TILT_CURVE:
      ligma_dynamics_output_copy_curve (g_value_get_object (value),
                                       private->tilt_curve);
      break;

    case PROP_WHEEL_CURVE:
      ligma_dynamics_output_copy_curve (g_value_get_object (value),
                                       private->wheel_curve);
      break;

    case PROP_RANDOM_CURVE:
      ligma_dynamics_output_copy_curve (g_value_get_object (value),
                                       private->random_curve);
      break;

    case PROP_FADE_CURVE:
      ligma_dynamics_output_copy_curve (g_value_get_object (value),
                                       private->fade_curve);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
ligma_dynamics_output_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  LigmaDynamicsOutputPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_TYPE:
      g_value_set_enum (value, private->type);
      break;

    case PROP_USE_PRESSURE:
      g_value_set_boolean (value, private->use_pressure);
      break;

    case PROP_USE_VELOCITY:
      g_value_set_boolean (value, private->use_velocity);
      break;

    case PROP_USE_DIRECTION:
      g_value_set_boolean (value, private->use_direction);
      break;

    case PROP_USE_TILT:
      g_value_set_boolean (value, private->use_tilt);
      break;

    case PROP_USE_WHEEL:
      g_value_set_boolean (value, private->use_wheel);
      break;

    case PROP_USE_RANDOM:
      g_value_set_boolean (value, private->use_random);
      break;

    case PROP_USE_FADE:
      g_value_set_boolean (value, private->use_fade);
      break;

    case PROP_PRESSURE_CURVE:
      g_value_set_object (value, private->pressure_curve);
      break;

    case PROP_VELOCITY_CURVE:
      g_value_set_object (value, private->velocity_curve);
      break;

    case PROP_DIRECTION_CURVE:
      g_value_set_object (value, private->direction_curve);
      break;

    case PROP_TILT_CURVE:
      g_value_set_object (value, private->tilt_curve);
      break;

    case PROP_WHEEL_CURVE:
      g_value_set_object (value, private->wheel_curve);
      break;

    case PROP_RANDOM_CURVE:
      g_value_set_object (value, private->random_curve);
      break;

    case PROP_FADE_CURVE:
      g_value_set_object (value, private->fade_curve);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

LigmaDynamicsOutput *
ligma_dynamics_output_new (const gchar            *name,
                          LigmaDynamicsOutputType  type)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (LIGMA_TYPE_DYNAMICS_OUTPUT,
                       "name", name,
                       "type", type,
                       NULL);
}

gboolean
ligma_dynamics_output_is_enabled (LigmaDynamicsOutput *output)
{
  LigmaDynamicsOutputPrivate *private = GET_PRIVATE (output);

  return (private->use_pressure  ||
          private->use_velocity  ||
          private->use_direction ||
          private->use_tilt      ||
          private->use_wheel     ||
          private->use_random    ||
          private->use_fade);
}

gdouble
ligma_dynamics_output_get_linear_value (LigmaDynamicsOutput *output,
                                       const LigmaCoords   *coords,
                                       LigmaPaintOptions   *options,
                                       gdouble             fade_point)
{
  LigmaDynamicsOutputPrivate *private = GET_PRIVATE (output);
  gdouble                    total   = 0.0;
  gdouble                    result  = 1.0;
  gint                       factors = 0;

  if (private->use_pressure)
    {
      total += ligma_curve_map_value (private->pressure_curve,
                                     coords->pressure);
      factors++;
    }

  if (private->use_velocity)
    {
      total += ligma_curve_map_value (private->velocity_curve,
                                    (1.0 - coords->velocity));
      factors++;
    }

  if (private->use_direction)
    {
      total += ligma_curve_map_value (private->direction_curve,
                                     fmod (coords->direction + 0.5, 1));
      factors++;
    }

  if (private->use_tilt)
    {
      total += ligma_curve_map_value (private->tilt_curve,
                                     (1.0 - sqrt (SQR (coords->xtilt) +
                                      SQR (coords->ytilt))));
      factors++;
    }

  if (private->use_wheel)
    {
      gdouble wheel;

      wheel = coords->wheel;

      total += ligma_curve_map_value (private->wheel_curve, wheel);
      factors++;
    }

  if (private->use_random)
    {
      total += ligma_curve_map_value (private->random_curve,
                                     g_random_double_range (0.0, 1.0));
      factors++;
    }

  if (private->use_fade)
    {
      total += ligma_curve_map_value (private->fade_curve, fade_point);

      factors++;
    }

  if (factors > 0)
    result = total / factors;

#if 0
  g_printerr ("Dynamics queried(linear). Result: %f, factors: %d, total: %f\n",
              result, factors, total);
#endif

  return result;
}

gdouble
ligma_dynamics_output_get_angular_value (LigmaDynamicsOutput *output,
                                        const LigmaCoords   *coords,
                                        LigmaPaintOptions   *options,
                                        gdouble             fade_point)
{
  LigmaDynamicsOutputPrivate *private = GET_PRIVATE (output);
  gdouble                    total   = 0.0;
  gdouble                    result  = 0.0; /* angles are additive, so we return zero for no change. */
  gint                       factors = 0;

  if (private->use_pressure)
    {
      total += ligma_curve_map_value (private->pressure_curve,
                                     coords->pressure);
      factors++;
    }

  if (private->use_velocity)
    {
      total += ligma_curve_map_value (private->velocity_curve,
                                    (1.0 - coords->velocity));
      factors++;
    }

  if (private->use_direction)
    {
      gdouble angle = ligma_curve_map_value (private->direction_curve,
                                            coords->direction);

      if (options->brush_lock_to_view)
        {
          if (coords->reflect)
            angle = 0.5 - angle;

          angle -= coords->angle;
          angle  = fmod (fmod (angle, 1.0) + 1.0, 1.0);
        }

      total += angle;
      factors++;
    }

  /* For tilt to make sense, it needs to be converted to an angle, not
   * just a vector
   */
  if (private->use_tilt)
    {
      gdouble tilt_x = coords->xtilt;
      gdouble tilt_y = coords->ytilt;
      gdouble tilt   = 0.0;

      if (tilt_x == 0.0)
        {
          if (tilt_y > 0.0)
            tilt = 0.25;
          else if (tilt_y < 0.0)
            tilt = 0.75;
          else
            tilt = 0.0;
        }
      else
        {
          tilt = atan ((- 1.0 * tilt_y) /
                                tilt_x) / (2 * G_PI);

          if (tilt_x > 0.0)
            tilt = tilt + 0.5;
        }

      tilt = tilt + 0.5; /* correct the angle, its wrong by 180 degrees */

      while (tilt > 1.0)
        tilt -= 1.0;

      while (tilt < 0.0)
        tilt += 1.0;

      total += ligma_curve_map_value (private->tilt_curve, tilt);
      factors++;
    }

  if (private->use_wheel)
    {
      gdouble angle = 1.0 - fmod(0.5 + coords->wheel, 1);

      total += ligma_curve_map_value (private->wheel_curve, angle);
      factors++;
    }

  if (private->use_random)
    {
      total += ligma_curve_map_value (private->random_curve,
                                     g_random_double_range (0.0, 1.0));
      factors++;
    }

  if (private->use_fade)
    {
      total += ligma_curve_map_value (private->fade_curve, fade_point);

      factors++;
    }

  if (factors > 0)
    result = total / factors;

#if 0
  g_printerr ("Dynamics queried(angle). Result: %f, factors: %d, total: %f\n",
              result, factors, total);
#endif

   return result;
}

gdouble
ligma_dynamics_output_get_aspect_value (LigmaDynamicsOutput *output,
                                       const LigmaCoords   *coords,
                                       LigmaPaintOptions   *options,
                                       gdouble             fade_point)
{
  LigmaDynamicsOutputPrivate *private = GET_PRIVATE (output);
  gdouble                    total   = 0.0;
  gint                       factors = 0;
  gdouble                    sign    = 1.0;
  gdouble                    result  = 1.0;

  if (private->use_pressure)
    {
      total += ligma_curve_map_value (private->pressure_curve,
                                     coords->pressure);
      factors++;
    }

  if (private->use_velocity)
    {
      total += ligma_curve_map_value (private->velocity_curve,
                                     coords->velocity);
      factors++;
    }

  if (private->use_direction)
    {
      gdouble direction = ligma_curve_map_value (private->direction_curve,
                                                coords->direction);

      if (((direction > 0.875) && (direction <= 1.0)) ||
          ((direction > 0.0) && (direction < 0.125))  ||
          ((direction > 0.375) && (direction < 0.625)))
        sign = -1.0;

      total += 1.0;
      factors++;
    }

  if (private->use_tilt)
    {
      gdouble tilt_value =  MAX (fabs (coords->xtilt), fabs (coords->ytilt));

      tilt_value = ligma_curve_map_value (private->tilt_curve,
                                         tilt_value);

      total += tilt_value;

      factors++;
    }

  if (private->use_wheel)
    {
      gdouble wheel = ligma_curve_map_value (private->wheel_curve,
                                            coords->wheel);

      if (((wheel > 0.875) && (wheel <= 1.0)) ||
          ((wheel > 0.0) && (wheel < 0.125))  ||
          ((wheel > 0.375) && (wheel < 0.625)))
        sign = -1.0;

      total += 1.0;
      factors++;

    }

  if (private->use_random)
    {
      gdouble random = ligma_curve_map_value (private->random_curve,
                                             g_random_double_range (0.0, 1.0));

      total += random;
      factors++;
    }

  if (private->use_fade)
    {
      total += ligma_curve_map_value (private->fade_curve, fade_point);

      factors++;
    }

  if (factors > 0)
    result = total / factors;


#if 0
  g_printerr ("Dynamics queried(aspect). Result: %f, factors: %d, total: %f sign: %f\n",
              result, factors, total, sign);
#endif
  result = CLAMP (result * sign, -1.0, 1.0);

  return result;
}

static void
ligma_dynamics_output_copy_curve (LigmaCurve *src,
                                 LigmaCurve *dest)
{
  if (src && dest)
    {
      ligma_config_copy (LIGMA_CONFIG (src),
                        LIGMA_CONFIG (dest),
                        LIGMA_CONFIG_PARAM_SERIALIZE);
    }
}

static LigmaCurve *
ligma_dynamics_output_create_curve (LigmaDynamicsOutput *output,
                                   const gchar        *name)
{
  LigmaCurve *curve = LIGMA_CURVE (ligma_curve_new (name));

  g_signal_connect_object (curve, "dirty",
                           G_CALLBACK (ligma_dynamics_output_curve_dirty),
                           output, 0);

  return curve;
}

static void
ligma_dynamics_output_curve_dirty (LigmaCurve          *curve,
                                  LigmaDynamicsOutput *output)
{
  g_object_notify (G_OBJECT (output), ligma_object_get_name (curve));
}
