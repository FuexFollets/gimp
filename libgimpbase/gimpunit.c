/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * ligmaunit.c
 * Copyright (C) 2003 Michael Natterer <mitch@ligma.org>
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

#include <math.h>
#include <string.h>

#include <glib-object.h>

#include "ligmabasetypes.h"

#include "ligmabase-private.h"
#include "ligmaunit.h"


/**
 * SECTION: ligmaunit
 * @title: ligmaunit
 * @short_description: Provides a collection of predefined units and
 *                     functions for creating user-defined units.
 * @see_also: #LigmaUnitMenu, #LigmaSizeEntry.
 *
 * Provides a collection of predefined units and functions for
 * creating user-defined units.
 **/


static void   unit_to_string (const GValue *src_value,
                              GValue       *dest_value);
static void   string_to_unit (const GValue *src_value,
                              GValue       *dest_value);

GType
ligma_unit_get_type (void)
{
  static GType unit_type = 0;

  if (! unit_type)
    {
      const GTypeInfo type_info = { 0, };

      unit_type = g_type_register_static (G_TYPE_INT, "LigmaUnit",
                                          &type_info, 0);

      g_value_register_transform_func (unit_type, G_TYPE_STRING,
                                       unit_to_string);
      g_value_register_transform_func (G_TYPE_STRING, unit_type,
                                       string_to_unit);
    }

  return unit_type;
}

static void
unit_to_string (const GValue *src_value,
                GValue       *dest_value)
{
  LigmaUnit unit = (LigmaUnit) g_value_get_int (src_value);

  g_value_set_string (dest_value, ligma_unit_get_identifier (unit));
}

static void
string_to_unit (const GValue *src_value,
                GValue       *dest_value)
{
  const gchar *str;
  gint         num_units;
  gint         i;

  str = g_value_get_string (src_value);

  if (!str || !*str)
    goto error;

  num_units = ligma_unit_get_number_of_units ();

  for (i = LIGMA_UNIT_PIXEL; i < num_units; i++)
    if (strcmp (str, ligma_unit_get_identifier (i)) == 0)
      break;

  if (i == num_units)
    {
      if (strcmp (str, ligma_unit_get_identifier (LIGMA_UNIT_PERCENT)) == 0)
        i = LIGMA_UNIT_PERCENT;
      else
        goto error;
    }

  g_value_set_int (dest_value, i);
  return;

 error:
  g_warning ("Can't convert string '%s' to LigmaUnit.", str);
}


/**
 * ligma_unit_get_number_of_units:
 *
 * Returns the number of units which are known to the #LigmaUnit system.
 *
 * Returns: The number of defined units.
 **/
gint
ligma_unit_get_number_of_units (void)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_number_of_units != NULL,
                        LIGMA_UNIT_END);

  return _ligma_unit_vtable.unit_get_number_of_units ();
}

/**
 * ligma_unit_get_number_of_built_in_units:
 *
 * Returns the number of #LigmaUnit's which are hardcoded in the unit system
 * (UNIT_INCH, UNIT_MM, UNIT_POINT, UNIT_PICA and the two "pseudo unit"
 *  UNIT_PIXEL).
 *
 * Returns: The number of built-in units.
 **/
gint
ligma_unit_get_number_of_built_in_units (void)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_number_of_built_in_units
                        != NULL, LIGMA_UNIT_END);

  return _ligma_unit_vtable.unit_get_number_of_built_in_units ();
}

/**
 * ligma_unit_new:
 * @identifier: The unit's identifier string.
 * @factor: The unit's factor (how many units are in one inch).
 * @digits: The unit's suggested number of digits (see ligma_unit_get_digits()).
 * @symbol: The symbol of the unit (e.g. "''" for inch).
 * @abbreviation: The abbreviation of the unit.
 * @singular: The singular form of the unit.
 * @plural: The plural form of the unit.
 *
 * Returns the integer ID of the new #LigmaUnit.
 *
 * Note that a new unit is always created with its deletion flag
 * set to %TRUE. You will have to set it to %FALSE with
 * ligma_unit_set_deletion_flag() to make the unit definition persistent.
 *
 * Returns: The ID of the new unit.
 **/
LigmaUnit
ligma_unit_new (gchar   *identifier,
               gdouble  factor,
               gint     digits,
               gchar   *symbol,
               gchar   *abbreviation,
               gchar   *singular,
               gchar   *plural)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_new != NULL, LIGMA_UNIT_INCH);

  return _ligma_unit_vtable.unit_new (identifier, factor, digits,
                                     symbol, abbreviation, singular, plural);
}

/**
 * ligma_unit_get_deletion_flag:
 * @unit: The unit you want to know the @deletion_flag of.
 *
 * Returns: The unit's @deletion_flag.
 **/
gboolean
ligma_unit_get_deletion_flag (LigmaUnit unit)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_deletion_flag != NULL, FALSE);

  return _ligma_unit_vtable.unit_get_deletion_flag (unit);
}

/**
 * ligma_unit_set_deletion_flag:
 * @unit: The unit you want to set the @deletion_flag for.
 * @deletion_flag: The new deletion_flag.
 *
 * Sets a #LigmaUnit's @deletion_flag. If the @deletion_flag of a unit is
 * %TRUE when LIGMA exits, this unit will not be saved in the users's
 * "unitrc" file.
 *
 * Trying to change the @deletion_flag of a built-in unit will be silently
 * ignored.
 **/
void
ligma_unit_set_deletion_flag (LigmaUnit unit,
                             gboolean deletion_flag)
{
  g_return_if_fail (_ligma_unit_vtable.unit_set_deletion_flag != NULL);

  _ligma_unit_vtable.unit_set_deletion_flag (unit, deletion_flag);
}

/**
 * ligma_unit_get_factor:
 * @unit: The unit you want to know the factor of.
 *
 * A #LigmaUnit's @factor is defined to be:
 *
 * distance_in_units == (@factor * distance_in_inches)
 *
 * Returns 0 for @unit == LIGMA_UNIT_PIXEL.
 *
 * Returns: The unit's factor.
 **/
gdouble
ligma_unit_get_factor (LigmaUnit unit)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_factor != NULL, 1.0);

  if (unit == LIGMA_UNIT_PIXEL)
    return 0.0;

  return _ligma_unit_vtable.unit_get_factor (unit);
}

/**
 * ligma_unit_get_digits:
 * @unit: The unit you want to know the digits.
 *
 * Returns the number of digits set for @unit.
 * Built-in units' accuracy is approximately the same as an inch with
 * two digits. User-defined units can suggest a different accuracy.
 *
 * Note: the value is as-set by defaults or by the user and does not
 * necessary provide enough precision on high-resolution images.
 * When the information is needed for a specific image, the use of
 * ligma_unit_get_scaled_digits() may be more appropriate.
 *
 * Returns 0 for @unit == LIGMA_UNIT_PIXEL.
 *
 * Returns: The suggested number of digits.
 **/
gint
ligma_unit_get_digits (LigmaUnit unit)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_digits != NULL, 2);

  return _ligma_unit_vtable.unit_get_digits (unit);
}

/**
 * ligma_unit_get_scaled_digits:
 * @unit: The unit you want to know the digits.
 * @resolution: the resolution in PPI.
 *
 * Returns the number of digits a @unit field should provide to get
 * enough accuracy so that every pixel position shows a different
 * value from neighboring pixels.
 *
 * Note: when needing digit accuracy to display a diagonal distance,
 * the @resolution may not correspond to the image's horizontal or
 * vertical resolution, but instead to the result of:
 * `distance_in_pixel / distance_in_inch`.
 *
 * Returns: The suggested number of digits.
 **/
gint
ligma_unit_get_scaled_digits (LigmaUnit unit,
                             gdouble  resolution)
{
  gint digits;

  g_return_val_if_fail (_ligma_unit_vtable.unit_get_digits != NULL, 2);

  digits = ceil (log10 (1.0 /
                        ligma_pixels_to_units (1.0, unit, resolution)));

  return MAX (digits, ligma_unit_get_digits (unit));
}

/**
 * ligma_unit_get_identifier:
 * @unit: The unit you want to know the identifier of.
 *
 * This is an untranslated string and must not be changed or freed.
 *
 * Returns: The unit's identifier.
 **/
const gchar *
ligma_unit_get_identifier (LigmaUnit unit)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_identifier != NULL, NULL);

  return _ligma_unit_vtable.unit_get_identifier (unit);
}

/**
 * ligma_unit_get_symbol:
 * @unit: The unit you want to know the symbol of.
 *
 * This is e.g. "''" for UNIT_INCH.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's symbol.
 **/
const gchar *
ligma_unit_get_symbol (LigmaUnit unit)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_symbol != NULL, NULL);

  return _ligma_unit_vtable.unit_get_symbol (unit);
}

/**
 * ligma_unit_get_abbreviation:
 * @unit: The unit you want to know the abbreviation of.
 *
 * For built-in units, this function returns the translated abbreviation
 * of the unit.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's abbreviation.
 **/
const gchar *
ligma_unit_get_abbreviation (LigmaUnit unit)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_abbreviation != NULL, NULL);

  return _ligma_unit_vtable.unit_get_abbreviation (unit);
}

/**
 * ligma_unit_get_singular:
 * @unit: The unit you want to know the singular form of.
 *
 * For built-in units, this function returns the translated singular form
 * of the unit's name.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's singular form.
 **/
const gchar *
ligma_unit_get_singular (LigmaUnit unit)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_singular != NULL, NULL);

  return _ligma_unit_vtable.unit_get_singular (unit);
}

/**
 * ligma_unit_get_plural:
 * @unit: The unit you want to know the plural form of.
 *
 * For built-in units, this function returns the translated plural form
 * of the unit's name.
 *
 * NOTE: This string must not be changed or freed.
 *
 * Returns: The unit's plural form.
 **/
const gchar *
ligma_unit_get_plural (LigmaUnit unit)
{
  g_return_val_if_fail (_ligma_unit_vtable.unit_get_plural != NULL, NULL);

  return _ligma_unit_vtable.unit_get_plural (unit);
}

static gint print (gchar       *buf,
                   gint         len,
                   gint         start,
                   const gchar *fmt,
                   ...) G_GNUC_PRINTF (4, 5);

static gint
print (gchar       *buf,
       gint         len,
       gint         start,
       const gchar *fmt,
       ...)
{
  va_list args;
  gint printed;

  va_start (args, fmt);

  printed = g_vsnprintf (buf + start, len - start, fmt, args);
  if (printed < 0)
    printed = len - start;

  va_end (args);

  return printed;
}

/**
 * ligma_unit_format_string:
 * @format: A printf-like format string which is used to create the unit
 *          string.
 * @unit:   A unit.
 *
 * The @format string supports the following percent expansions:
 *
 * <informaltable pgwide="1" frame="none" role="enum">
 *   <tgroup cols="2"><colspec colwidth="1*"/><colspec colwidth="8*"/>
 *     <tbody>
 *       <row>
 *         <entry>% f</entry>
 *         <entry>Factor (how many units make up an inch)</entry>
 *        </row>
 *       <row>
 *         <entry>% y</entry>
 *         <entry>Symbol (e.g. "''" for LIGMA_UNIT_INCH)</entry>
 *       </row>
 *       <row>
 *         <entry>% a</entry>
 *         <entry>Abbreviation</entry>
 *       </row>
 *       <row>
 *         <entry>% s</entry>
 *         <entry>Singular</entry>
 *       </row>
 *       <row>
 *         <entry>% p</entry>
 *         <entry>Plural</entry>
 *       </row>
 *       <row>
 *         <entry>%%</entry>
 *         <entry>Literal percent</entry>
 *       </row>
 *     </tbody>
 *   </tgroup>
 * </informaltable>
 *
 * Returns: A newly allocated string with above percent expressions
 *          replaced with the resp. strings for @unit.
 *
 * Since: 2.8
 **/
gchar *
ligma_unit_format_string (const gchar *format,
                         LigmaUnit     unit)
{
  gchar buffer[1024];
  gint  i = 0;

  g_return_val_if_fail (format != NULL, NULL);
  g_return_val_if_fail (unit == LIGMA_UNIT_PERCENT ||
                        (unit >= LIGMA_UNIT_PIXEL &&
                         unit < ligma_unit_get_number_of_units ()), NULL);

  while (i < (sizeof (buffer) - 1) && *format)
    {
      switch (*format)
        {
        case '%':
          format++;
          switch (*format)
            {
            case 0:
              g_warning ("%s: unit-menu-format string ended within %%-sequence",
                         G_STRFUNC);
              break;

            case '%':
              buffer[i++] = '%';
              break;

            case 'f': /* factor (how many units make up an inch) */
              i += print (buffer, sizeof (buffer), i, "%f",
                          ligma_unit_get_factor (unit));
              break;

            case 'y': /* symbol ("''" for inch) */
              i += print (buffer, sizeof (buffer), i, "%s",
                          ligma_unit_get_symbol (unit));
              break;

            case 'a': /* abbreviation */
              i += print (buffer, sizeof (buffer), i, "%s",
                          ligma_unit_get_abbreviation (unit));
              break;

            case 's': /* singular */
              i += print (buffer, sizeof (buffer), i, "%s",
                          ligma_unit_get_singular (unit));
              break;

            case 'p': /* plural */
              i += print (buffer, sizeof (buffer), i, "%s",
                          ligma_unit_get_plural (unit));
              break;

            default:
              g_warning ("%s: unit-menu-format contains unknown format "
                         "sequence '%%%c'", G_STRFUNC, *format);
              break;
            }
          break;

        default:
          buffer[i++] = *format;
          break;
        }

      format++;
    }

  buffer[MIN (i, sizeof (buffer) - 1)] = 0;

  return g_strdup (buffer);
}

/*
 * LIGMA_TYPE_PARAM_UNIT
 */

static void      ligma_param_unit_class_init     (GParamSpecClass *class);
static gboolean  ligma_param_unit_value_validate (GParamSpec      *pspec,
                                                 GValue          *value);

/**
 * ligma_param_unit_get_type:
 *
 * Reveals the object type
 *
 * Returns: the #GType for a unit param object
 *
 * Since: 2.4
 **/
GType
ligma_param_unit_get_type (void)
{
  static GType spec_type = 0;

  if (! spec_type)
    {
      const GTypeInfo type_info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) ligma_param_unit_class_init,
        NULL, NULL,
        sizeof (LigmaParamSpecUnit),
        0, NULL, NULL
      };

      spec_type = g_type_register_static (G_TYPE_PARAM_INT,
                                          "LigmaParamUnit",
                                          &type_info, 0);
    }

  return spec_type;
}

static void
ligma_param_unit_class_init (GParamSpecClass *class)
{
  class->value_type     = LIGMA_TYPE_UNIT;
  class->value_validate = ligma_param_unit_value_validate;
}

static gboolean
ligma_param_unit_value_validate (GParamSpec *pspec,
                                GValue     *value)
{
  GParamSpecInt     *ispec = G_PARAM_SPEC_INT (pspec);
  LigmaParamSpecUnit *uspec = LIGMA_PARAM_SPEC_UNIT (pspec);
  gint               oval  = value->data[0].v_int;

  if (!(uspec->allow_percent && value->data[0].v_int == LIGMA_UNIT_PERCENT))
    {
      value->data[0].v_int = CLAMP (value->data[0].v_int,
                                    ispec->minimum,
                                    ligma_unit_get_number_of_units () - 1);
    }

  return value->data[0].v_int != oval;
}

/**
 * ligma_param_spec_unit:
 * @name:          Canonical name of the param
 * @nick:          Nickname of the param
 * @blurb:         Brief description of param.
 * @allow_pixels:  Whether "pixels" is an allowed unit.
 * @allow_percent: Whether "percent" is an allowed unit.
 * @default_value: Unit to use if none is assigned.
 * @flags:         a combination of #GParamFlags
 *
 * Creates a param spec to hold a units param.
 * See g_param_spec_internal() for more information.
 *
 * Returns: (transfer full): a newly allocated #GParamSpec instance
 *
 * Since: 2.4
 **/
GParamSpec *
ligma_param_spec_unit (const gchar *name,
                      const gchar *nick,
                      const gchar *blurb,
                      gboolean     allow_pixels,
                      gboolean     allow_percent,
                      LigmaUnit     default_value,
                      GParamFlags  flags)
{
  LigmaParamSpecUnit *pspec;
  GParamSpecInt     *ispec;

  pspec = g_param_spec_internal (LIGMA_TYPE_PARAM_UNIT,
                                 name, nick, blurb, flags);

  ispec = G_PARAM_SPEC_INT (pspec);

  ispec->default_value = default_value;
  ispec->minimum       = allow_pixels ? LIGMA_UNIT_PIXEL : LIGMA_UNIT_INCH;
  ispec->maximum       = LIGMA_UNIT_PERCENT - 1;

  pspec->allow_percent = allow_percent;

  return G_PARAM_SPEC (pspec);
}

/**
 * ligma_pixels_to_units:
 * @pixels:     value in pixels
 * @unit:       unit to convert to
 * @resolution: resolution in DPI
 *
 * Converts a @value specified in pixels to @unit.
 *
 * Returns: @pixels converted to units.
 *
 * Since: 2.8
 **/
gdouble
ligma_pixels_to_units (gdouble  pixels,
                      LigmaUnit unit,
                      gdouble  resolution)
{
  if (unit == LIGMA_UNIT_PIXEL)
    return pixels;

  return pixels * ligma_unit_get_factor (unit) / resolution;
}

/**
 * ligma_units_to_pixels:
 * @value:      value in units
 * @unit:       unit of @value
 * @resolution: resloution in DPI
 *
 * Converts a @value specified in @unit to pixels.
 *
 * Returns: @value converted to pixels.
 *
 * Since: 2.8
 **/
gdouble
ligma_units_to_pixels (gdouble  value,
                      LigmaUnit unit,
                      gdouble  resolution)
{
  if (unit == LIGMA_UNIT_PIXEL)
    return value;

  return value * resolution / ligma_unit_get_factor (unit);
}

/**
 * ligma_units_to_points:
 * @value:      value in units
 * @unit:       unit of @value
 * @resolution: resloution in DPI
 *
 * Converts a @value specified in @unit to points.
 *
 * Returns: @value converted to points.
 *
 * Since: 2.8
 **/
gdouble
ligma_units_to_points (gdouble  value,
                      LigmaUnit unit,
                      gdouble  resolution)
{
  if (unit == LIGMA_UNIT_POINT)
    return value;

  if (unit == LIGMA_UNIT_PIXEL)
    return (value * ligma_unit_get_factor (LIGMA_UNIT_POINT) / resolution);

  return (value *
          ligma_unit_get_factor (LIGMA_UNIT_POINT) / ligma_unit_get_factor (unit));
}

/**
 * ligma_unit_is_metric:
 * @unit: The unit
 *
 * Checks if the given @unit is metric. A simplistic test is used
 * that looks at the unit's factor and checks if it is 2.54 multiplied
 * by some common powers of 10. Currently it checks for mm, cm, dm, m.
 *
 * See also: ligma_unit_get_factor()
 *
 * Returns: %TRUE if the @unit is metric.
 *
 * Since: 2.10
 **/
gboolean
ligma_unit_is_metric (LigmaUnit unit)
{
  gdouble factor;

  if (unit == LIGMA_UNIT_MM)
    return TRUE;

  factor = ligma_unit_get_factor (unit);

  if (factor == 0.0)
    return FALSE;

  return ((ABS (factor -  0.0254) < 1e-7) || /* m  */
          (ABS (factor -  0.254)  < 1e-6) || /* dm */
          (ABS (factor -  2.54)   < 1e-5) || /* cm */
          (ABS (factor - 25.4)    < 1e-4));  /* mm */
}
