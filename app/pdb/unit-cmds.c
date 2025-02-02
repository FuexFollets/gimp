/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-2003 Spencer Kimball and Peter Mattis
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

/* NOTE: This file is auto-generated by pdbgen.pl. */

#include "config.h"

#include "stamp-pdbgen.h"

#include <gegl.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libligmabase/ligmabase.h"

#include "libligmabase/ligmabase.h"

#include "pdb-types.h"

#include "core/ligmaparamspecs.h"
#include "core/ligmaunit.h"

#include "ligmapdb.h"
#include "ligmaprocedure.h"
#include "internal-procs.h"


static LigmaValueArray *
unit_get_number_of_units_invoker (LigmaProcedure         *procedure,
                                  Ligma                  *ligma,
                                  LigmaContext           *context,
                                  LigmaProgress          *progress,
                                  const LigmaValueArray  *args,
                                  GError               **error)
{
  LigmaValueArray *return_vals;
  gint num_units = 0;

  num_units = _ligma_unit_get_number_of_units (ligma);

  return_vals = ligma_procedure_get_return_values (procedure, TRUE, NULL);
  g_value_set_int (ligma_value_array_index (return_vals, 1), num_units);

  return return_vals;
}

static LigmaValueArray *
unit_get_number_of_built_in_units_invoker (LigmaProcedure         *procedure,
                                           Ligma                  *ligma,
                                           LigmaContext           *context,
                                           LigmaProgress          *progress,
                                           const LigmaValueArray  *args,
                                           GError               **error)
{
  LigmaValueArray *return_vals;
  gint num_units = 0;

  num_units = _ligma_unit_get_number_of_built_in_units (ligma);

  return_vals = ligma_procedure_get_return_values (procedure, TRUE, NULL);
  g_value_set_int (ligma_value_array_index (return_vals, 1), num_units);

  return return_vals;
}

static LigmaValueArray *
unit_new_invoker (LigmaProcedure         *procedure,
                  Ligma                  *ligma,
                  LigmaContext           *context,
                  LigmaProgress          *progress,
                  const LigmaValueArray  *args,
                  GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  const gchar *identifier;
  gdouble factor;
  gint digits;
  const gchar *symbol;
  const gchar *abbreviation;
  const gchar *singular;
  const gchar *plural;
  LigmaUnit unit_id = LIGMA_UNIT_PIXEL;

  identifier = g_value_get_string (ligma_value_array_index (args, 0));
  factor = g_value_get_double (ligma_value_array_index (args, 1));
  digits = g_value_get_int (ligma_value_array_index (args, 2));
  symbol = g_value_get_string (ligma_value_array_index (args, 3));
  abbreviation = g_value_get_string (ligma_value_array_index (args, 4));
  singular = g_value_get_string (ligma_value_array_index (args, 5));
  plural = g_value_get_string (ligma_value_array_index (args, 6));

  if (success)
    {
      unit_id = _ligma_unit_new (ligma, identifier, factor, digits,
                                symbol, abbreviation, singular, plural);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_int (ligma_value_array_index (return_vals, 1), unit_id);

  return return_vals;
}

static LigmaValueArray *
unit_get_deletion_flag_invoker (LigmaProcedure         *procedure,
                                Ligma                  *ligma,
                                LigmaContext           *context,
                                LigmaProgress          *progress,
                                const LigmaValueArray  *args,
                                GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaUnit unit_id;
  gboolean deletion_flag = FALSE;

  unit_id = g_value_get_int (ligma_value_array_index (args, 0));

  if (success)
    {
      deletion_flag = _ligma_unit_get_deletion_flag (ligma, unit_id);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_boolean (ligma_value_array_index (return_vals, 1), deletion_flag);

  return return_vals;
}

static LigmaValueArray *
unit_set_deletion_flag_invoker (LigmaProcedure         *procedure,
                                Ligma                  *ligma,
                                LigmaContext           *context,
                                LigmaProgress          *progress,
                                const LigmaValueArray  *args,
                                GError               **error)
{
  gboolean success = TRUE;
  LigmaUnit unit_id;
  gboolean deletion_flag;

  unit_id = g_value_get_int (ligma_value_array_index (args, 0));
  deletion_flag = g_value_get_boolean (ligma_value_array_index (args, 1));

  if (success)
    {
      _ligma_unit_set_deletion_flag (ligma, unit_id, deletion_flag);
    }

  return ligma_procedure_get_return_values (procedure, success,
                                           error ? *error : NULL);
}

static LigmaValueArray *
unit_get_identifier_invoker (LigmaProcedure         *procedure,
                             Ligma                  *ligma,
                             LigmaContext           *context,
                             LigmaProgress          *progress,
                             const LigmaValueArray  *args,
                             GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaUnit unit_id;
  gchar *identifier = NULL;

  unit_id = g_value_get_int (ligma_value_array_index (args, 0));

  if (success)
    {
      identifier = g_strdup (_ligma_unit_get_identifier (ligma, unit_id));
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_take_string (ligma_value_array_index (return_vals, 1), identifier);

  return return_vals;
}

static LigmaValueArray *
unit_get_factor_invoker (LigmaProcedure         *procedure,
                         Ligma                  *ligma,
                         LigmaContext           *context,
                         LigmaProgress          *progress,
                         const LigmaValueArray  *args,
                         GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaUnit unit_id;
  gdouble factor = 0.0;

  unit_id = g_value_get_int (ligma_value_array_index (args, 0));

  if (success)
    {
      factor = _ligma_unit_get_factor (ligma, unit_id);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_double (ligma_value_array_index (return_vals, 1), factor);

  return return_vals;
}

static LigmaValueArray *
unit_get_digits_invoker (LigmaProcedure         *procedure,
                         Ligma                  *ligma,
                         LigmaContext           *context,
                         LigmaProgress          *progress,
                         const LigmaValueArray  *args,
                         GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaUnit unit_id;
  gint digits = 0;

  unit_id = g_value_get_int (ligma_value_array_index (args, 0));

  if (success)
    {
      digits = _ligma_unit_get_digits (ligma, unit_id);
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_set_int (ligma_value_array_index (return_vals, 1), digits);

  return return_vals;
}

static LigmaValueArray *
unit_get_symbol_invoker (LigmaProcedure         *procedure,
                         Ligma                  *ligma,
                         LigmaContext           *context,
                         LigmaProgress          *progress,
                         const LigmaValueArray  *args,
                         GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaUnit unit_id;
  gchar *symbol = NULL;

  unit_id = g_value_get_int (ligma_value_array_index (args, 0));

  if (success)
    {
      symbol = g_strdup (_ligma_unit_get_symbol (ligma, unit_id));
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_take_string (ligma_value_array_index (return_vals, 1), symbol);

  return return_vals;
}

static LigmaValueArray *
unit_get_abbreviation_invoker (LigmaProcedure         *procedure,
                               Ligma                  *ligma,
                               LigmaContext           *context,
                               LigmaProgress          *progress,
                               const LigmaValueArray  *args,
                               GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaUnit unit_id;
  gchar *abbreviation = NULL;

  unit_id = g_value_get_int (ligma_value_array_index (args, 0));

  if (success)
    {
      abbreviation = g_strdup (_ligma_unit_get_abbreviation (ligma, unit_id));
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_take_string (ligma_value_array_index (return_vals, 1), abbreviation);

  return return_vals;
}

static LigmaValueArray *
unit_get_singular_invoker (LigmaProcedure         *procedure,
                           Ligma                  *ligma,
                           LigmaContext           *context,
                           LigmaProgress          *progress,
                           const LigmaValueArray  *args,
                           GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaUnit unit_id;
  gchar *singular = NULL;

  unit_id = g_value_get_int (ligma_value_array_index (args, 0));

  if (success)
    {
      singular = g_strdup (_ligma_unit_get_singular (ligma, unit_id));
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_take_string (ligma_value_array_index (return_vals, 1), singular);

  return return_vals;
}

static LigmaValueArray *
unit_get_plural_invoker (LigmaProcedure         *procedure,
                         Ligma                  *ligma,
                         LigmaContext           *context,
                         LigmaProgress          *progress,
                         const LigmaValueArray  *args,
                         GError               **error)
{
  gboolean success = TRUE;
  LigmaValueArray *return_vals;
  LigmaUnit unit_id;
  gchar *plural = NULL;

  unit_id = g_value_get_int (ligma_value_array_index (args, 0));

  if (success)
    {
      plural = g_strdup (_ligma_unit_get_plural (ligma, unit_id));
    }

  return_vals = ligma_procedure_get_return_values (procedure, success,
                                                  error ? *error : NULL);

  if (success)
    g_value_take_string (ligma_value_array_index (return_vals, 1), plural);

  return return_vals;
}

void
register_unit_procs (LigmaPDB *pdb)
{
  LigmaProcedure *procedure;

  /*
   * ligma-unit-get-number-of-units
   */
  procedure = ligma_procedure_new (unit_get_number_of_units_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-number-of-units");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the number of units.",
                                  "This procedure returns the number of defined units.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_int ("num-units",
                                                     "num units",
                                                     "The number of units",
                                                     G_MININT32, G_MAXINT32, 0,
                                                     LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-get-number-of-built-in-units
   */
  procedure = ligma_procedure_new (unit_get_number_of_built_in_units_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-number-of-built-in-units");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the number of built-in units.",
                                  "This procedure returns the number of defined units built-in to LIGMA.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_int ("num-units",
                                                     "num units",
                                                     "The number of built-in units",
                                                     G_MININT32, G_MAXINT32, 0,
                                                     LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-new
   */
  procedure = ligma_procedure_new (unit_new_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-new");
  ligma_procedure_set_static_help (procedure,
                                  "Creates a new unit and returns it's integer ID.",
                                  "This procedure creates a new unit and returns it's integer ID. Note that the new unit will have it's deletion flag set to TRUE, so you will have to set it to FALSE with 'ligma-unit-set-deletion-flag' to make it persistent.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("identifier",
                                                       "identifier",
                                                       "The new unit's identifier",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_double ("factor",
                                                    "factor",
                                                    "The new unit's factor",
                                                    -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                                    LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_int ("digits",
                                                 "digits",
                                                 "The new unit's digits",
                                                 G_MININT32, G_MAXINT32, 0,
                                                 LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("symbol",
                                                       "symbol",
                                                       "The new unit's symbol",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("abbreviation",
                                                       "abbreviation",
                                                       "The new unit's abbreviation",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("singular",
                                                       "singular",
                                                       "The new unit's singular form",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_string ("plural",
                                                       "plural",
                                                       "The new unit's plural form",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_unit ("unit-id",
                                                         "unit id",
                                                         "The new unit's ID",
                                                         TRUE,
                                                         FALSE,
                                                         LIGMA_UNIT_PIXEL,
                                                         LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-get-deletion-flag
   */
  procedure = ligma_procedure_new (unit_get_deletion_flag_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-deletion-flag");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the deletion flag of the unit.",
                                  "This procedure returns the deletion flag of the unit. If this value is TRUE the unit's definition will not be saved in the user's unitrc file on ligma exit.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_unit ("unit-id",
                                                     "unit id",
                                                     "The unit's integer ID",
                                                     TRUE,
                                                     FALSE,
                                                     LIGMA_UNIT_PIXEL,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_boolean ("deletion-flag",
                                                         "deletion flag",
                                                         "The unit's deletion flag",
                                                         FALSE,
                                                         LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-set-deletion-flag
   */
  procedure = ligma_procedure_new (unit_set_deletion_flag_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-set-deletion-flag");
  ligma_procedure_set_static_help (procedure,
                                  "Sets the deletion flag of a unit.",
                                  "This procedure sets the unit's deletion flag. If the deletion flag of a unit is TRUE on ligma exit, this unit's definition will not be saved in the user's unitrc.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_unit ("unit-id",
                                                     "unit id",
                                                     "The unit's integer ID",
                                                     TRUE,
                                                     FALSE,
                                                     LIGMA_UNIT_PIXEL,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_argument (procedure,
                               g_param_spec_boolean ("deletion-flag",
                                                     "deletion flag",
                                                     "The new deletion flag of the unit",
                                                     FALSE,
                                                     LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-get-identifier
   */
  procedure = ligma_procedure_new (unit_get_identifier_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-identifier");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the textual identifier of the unit.",
                                  "This procedure returns the textual identifier of the unit. For built-in units it will be the english singular form of the unit's name. For user-defined units this should equal to the singular form.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_unit ("unit-id",
                                                     "unit id",
                                                     "The unit's integer ID",
                                                     TRUE,
                                                     FALSE,
                                                     LIGMA_UNIT_PIXEL,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_string ("identifier",
                                                           "identifier",
                                                           "The unit's textual identifier",
                                                           FALSE, FALSE, FALSE,
                                                           NULL,
                                                           LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-get-factor
   */
  procedure = ligma_procedure_new (unit_get_factor_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-factor");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the factor of the unit.",
                                  "This procedure returns the unit's factor which indicates how many units make up an inch. Note that asking for the factor of \"pixels\" will produce an error.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_unit ("unit-id",
                                                     "unit id",
                                                     "The unit's integer ID",
                                                     TRUE,
                                                     FALSE,
                                                     LIGMA_UNIT_PIXEL,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_double ("factor",
                                                        "factor",
                                                        "The unit's factor",
                                                        -G_MAXDOUBLE, G_MAXDOUBLE, 0,
                                                        LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-get-digits
   */
  procedure = ligma_procedure_new (unit_get_digits_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-digits");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the number of digits of the unit.",
                                  "This procedure returns the number of digits you should provide in input or output functions to get approximately the same accuracy as with two digits and inches. Note that asking for the digits of \"pixels\" will produce an error.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_unit ("unit-id",
                                                     "unit id",
                                                     "The unit's integer ID",
                                                     TRUE,
                                                     FALSE,
                                                     LIGMA_UNIT_PIXEL,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   g_param_spec_int ("digits",
                                                     "digits",
                                                     "The unit's number of digits",
                                                     G_MININT32, G_MAXINT32, 0,
                                                     LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-get-symbol
   */
  procedure = ligma_procedure_new (unit_get_symbol_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-symbol");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the symbol of the unit.",
                                  "This procedure returns the symbol of the unit (\"''\" for inches).",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_unit ("unit-id",
                                                     "unit id",
                                                     "The unit's integer ID",
                                                     TRUE,
                                                     FALSE,
                                                     LIGMA_UNIT_PIXEL,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_string ("symbol",
                                                           "symbol",
                                                           "The unit's symbol",
                                                           FALSE, FALSE, FALSE,
                                                           NULL,
                                                           LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-get-abbreviation
   */
  procedure = ligma_procedure_new (unit_get_abbreviation_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-abbreviation");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the abbreviation of the unit.",
                                  "This procedure returns the abbreviation of the unit (\"in\" for inches).",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_unit ("unit-id",
                                                     "unit id",
                                                     "The unit's integer ID",
                                                     TRUE,
                                                     FALSE,
                                                     LIGMA_UNIT_PIXEL,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_string ("abbreviation",
                                                           "abbreviation",
                                                           "The unit's abbreviation",
                                                           FALSE, FALSE, FALSE,
                                                           NULL,
                                                           LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-get-singular
   */
  procedure = ligma_procedure_new (unit_get_singular_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-singular");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the singular form of the unit.",
                                  "This procedure returns the singular form of the unit.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_unit ("unit-id",
                                                     "unit id",
                                                     "The unit's integer ID",
                                                     TRUE,
                                                     FALSE,
                                                     LIGMA_UNIT_PIXEL,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_string ("singular",
                                                           "singular",
                                                           "The unit's singular form",
                                                           FALSE, FALSE, FALSE,
                                                           NULL,
                                                           LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);

  /*
   * ligma-unit-get-plural
   */
  procedure = ligma_procedure_new (unit_get_plural_invoker);
  ligma_object_set_static_name (LIGMA_OBJECT (procedure),
                               "ligma-unit-get-plural");
  ligma_procedure_set_static_help (procedure,
                                  "Returns the plural form of the unit.",
                                  "This procedure returns the plural form of the unit.",
                                  NULL);
  ligma_procedure_set_static_attribution (procedure,
                                         "Michael Natterer <mitch@ligma.org>",
                                         "Michael Natterer",
                                         "1999");
  ligma_procedure_add_argument (procedure,
                               ligma_param_spec_unit ("unit-id",
                                                     "unit id",
                                                     "The unit's integer ID",
                                                     TRUE,
                                                     FALSE,
                                                     LIGMA_UNIT_PIXEL,
                                                     LIGMA_PARAM_READWRITE));
  ligma_procedure_add_return_value (procedure,
                                   ligma_param_spec_string ("plural",
                                                           "plural",
                                                           "The unit's plural form",
                                                           FALSE, FALSE, FALSE,
                                                           NULL,
                                                           LIGMA_PARAM_READWRITE));
  ligma_pdb_register_procedure (pdb, procedure);
  g_object_unref (procedure);
}
