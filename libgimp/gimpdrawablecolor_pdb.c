/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmadrawablecolor_pdb.c
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

/* NOTE: This file is auto-generated by pdbgen.pl */

#include "config.h"

#include "stamp-pdbgen.h"

#include "ligma.h"


/**
 * SECTION: ligmadrawablecolor
 * @title: ligmadrawablecolor
 * @short_description: Functions for manipulating a drawable's color.
 *
 * Functions for manipulating a drawable's color, including curves and
 * histograms.
 **/


/**
 * ligma_drawable_brightness_contrast:
 * @drawable: The drawable.
 * @brightness: Brightness adjustment.
 * @contrast: Contrast adjustment.
 *
 * Modify brightness/contrast in the specified drawable.
 *
 * This procedures allows the brightness and contrast of the specified
 * drawable to be modified. Both 'brightness' and 'contrast' parameters
 * are defined between -1.0 and 1.0.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_brightness_contrast (LigmaDrawable *drawable,
                                   gdouble       brightness,
                                   gdouble       contrast)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          G_TYPE_DOUBLE, brightness,
                                          G_TYPE_DOUBLE, contrast,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-brightness-contrast",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_color_balance:
 * @drawable: The drawable.
 * @transfer_mode: Transfer mode.
 * @preserve_lum: Preserve luminosity values at each pixel.
 * @cyan_red: Cyan-Red color balance.
 * @magenta_green: Magenta-Green color balance.
 * @yellow_blue: Yellow-Blue color balance.
 *
 * Modify the color balance of the specified drawable.
 *
 * Modify the color balance of the specified drawable. There are three
 * axis which can be modified: cyan-red, magenta-green, and
 * yellow-blue. Negative values increase the amount of the former,
 * positive values increase the amount of the latter. Color balance can
 * be controlled with the 'transfer_mode' setting, which allows
 * shadows, mid-tones, and highlights in an image to be affected
 * differently. The 'preserve-lum' parameter, if TRUE, ensures that the
 * luminosity of each pixel remains fixed.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_color_balance (LigmaDrawable     *drawable,
                             LigmaTransferMode  transfer_mode,
                             gboolean          preserve_lum,
                             gdouble           cyan_red,
                             gdouble           magenta_green,
                             gdouble           yellow_blue)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          LIGMA_TYPE_TRANSFER_MODE, transfer_mode,
                                          G_TYPE_BOOLEAN, preserve_lum,
                                          G_TYPE_DOUBLE, cyan_red,
                                          G_TYPE_DOUBLE, magenta_green,
                                          G_TYPE_DOUBLE, yellow_blue,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-color-balance",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_colorize_hsl:
 * @drawable: The drawable.
 * @hue: Hue in degrees.
 * @saturation: Saturation in percent.
 * @lightness: Lightness in percent.
 *
 * Render the drawable as a grayscale image seen through a colored
 * glass.
 *
 * Desaturates the drawable, then tints it with the specified color.
 * This tool is only valid on RGB color images. It will not operate on
 * grayscale drawables.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_colorize_hsl (LigmaDrawable *drawable,
                            gdouble       hue,
                            gdouble       saturation,
                            gdouble       lightness)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          G_TYPE_DOUBLE, hue,
                                          G_TYPE_DOUBLE, saturation,
                                          G_TYPE_DOUBLE, lightness,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-colorize-hsl",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_curves_explicit:
 * @drawable: The drawable.
 * @channel: The channel to modify.
 * @num_values: The number of values in the new curve.
 * @values: (array length=num_values) (element-type gdouble): The explicit curve.
 *
 * Modifies the intensity curve(s) for specified drawable.
 *
 * Modifies the intensity mapping for one channel in the specified
 * drawable. The channel can be either an intensity component, or the
 * value. The 'values' parameter is an array of doubles which
 * explicitly defines how each pixel value in the drawable will be
 * modified. Use the ligma_drawable_curves_spline() function to modify
 * intensity levels with Catmull Rom splines.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_curves_explicit (LigmaDrawable         *drawable,
                               LigmaHistogramChannel  channel,
                               gint                  num_values,
                               const gdouble        *values)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          LIGMA_TYPE_HISTOGRAM_CHANNEL, channel,
                                          G_TYPE_INT, num_values,
                                          LIGMA_TYPE_FLOAT_ARRAY, NULL,
                                          G_TYPE_NONE);
  ligma_value_set_float_array (ligma_value_array_index (args, 3), values, num_values);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-curves-explicit",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_curves_spline:
 * @drawable: The drawable.
 * @channel: The channel to modify.
 * @num_points: The number of values in the control point array.
 * @points: (array length=num_points) (element-type gdouble): The spline control points: { cp1.x, cp1.y, cp2.x, cp2.y, ... }.
 *
 * Modifies the intensity curve(s) for specified drawable.
 *
 * Modifies the intensity mapping for one channel in the specified
 * drawable. The channel can be either an intensity component, or the
 * value. The 'points' parameter is an array of doubles which define a
 * set of control points which describe a Catmull Rom spline which
 * yields the final intensity curve. Use the
 * ligma_drawable_curves_explicit() function to explicitly modify
 * intensity levels.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_curves_spline (LigmaDrawable         *drawable,
                             LigmaHistogramChannel  channel,
                             gint                  num_points,
                             const gdouble        *points)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          LIGMA_TYPE_HISTOGRAM_CHANNEL, channel,
                                          G_TYPE_INT, num_points,
                                          LIGMA_TYPE_FLOAT_ARRAY, NULL,
                                          G_TYPE_NONE);
  ligma_value_set_float_array (ligma_value_array_index (args, 3), points, num_points);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-curves-spline",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_desaturate:
 * @drawable: The drawable.
 * @desaturate_mode: The formula to use to desaturate.
 *
 * Desaturate the contents of the specified drawable, with the
 * specified formula.
 *
 * This procedure desaturates the contents of the specified drawable,
 * with the specified formula. This procedure only works on drawables
 * of type RGB color.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_desaturate (LigmaDrawable       *drawable,
                          LigmaDesaturateMode  desaturate_mode)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          LIGMA_TYPE_DESATURATE_MODE, desaturate_mode,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-desaturate",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_equalize:
 * @drawable: The drawable.
 * @mask_only: Equalization option.
 *
 * Equalize the contents of the specified drawable.
 *
 * This procedure equalizes the contents of the specified drawable.
 * Each intensity channel is equalized independently. The equalized
 * intensity is given as inten' = (255 - inten). The 'mask_only' option
 * specifies whether to adjust only the area of the image within the
 * selection bounds, or the entire image based on the histogram of the
 * selected area. If there is no selection, the entire image is
 * adjusted based on the histogram for the entire image.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_equalize (LigmaDrawable *drawable,
                        gboolean      mask_only)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          G_TYPE_BOOLEAN, mask_only,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-equalize",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_histogram:
 * @drawable: The drawable.
 * @channel: The channel to query.
 * @start_range: Start of the intensity measurement range.
 * @end_range: End of the intensity measurement range.
 * @mean: (out): Mean intensity value.
 * @std_dev: (out): Standard deviation of intensity values.
 * @median: (out): Median intensity value.
 * @pixels: (out): Alpha-weighted pixel count for entire image.
 * @count: (out): Alpha-weighted pixel count for range.
 * @percentile: (out): Percentile that range falls under.
 *
 * Returns information on the intensity histogram for the specified
 * drawable.
 *
 * This tool makes it possible to gather information about the
 * intensity histogram of a drawable. A channel to examine is first
 * specified. This can be either value, red, green, or blue, depending
 * on whether the drawable is of type color or grayscale. Second, a
 * range of intensities are specified. The ligma_drawable_histogram()
 * function returns statistics based on the pixels in the drawable that
 * fall under this range of values. Mean, standard deviation, median,
 * number of pixels, and percentile are all returned. Additionally, the
 * total count of pixels in the image is returned. Counts of pixels are
 * weighted by any associated alpha values and by the current selection
 * mask. That is, pixels that lie outside an active selection mask will
 * not be counted. Similarly, pixels with transparent alpha values will
 * not be counted. The returned mean, std_dev and median are in the
 * range (0..255) for 8-bit images or if the plug-in is not
 * precision-aware, and in the range (0.0..1.0) otherwise.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_histogram (LigmaDrawable         *drawable,
                         LigmaHistogramChannel  channel,
                         gdouble               start_range,
                         gdouble               end_range,
                         gdouble              *mean,
                         gdouble              *std_dev,
                         gdouble              *median,
                         gdouble              *pixels,
                         gdouble              *count,
                         gdouble              *percentile)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          LIGMA_TYPE_HISTOGRAM_CHANNEL, channel,
                                          G_TYPE_DOUBLE, start_range,
                                          G_TYPE_DOUBLE, end_range,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-histogram",
                                              args);
  ligma_value_array_unref (args);

  *mean = 0.0;
  *std_dev = 0.0;
  *median = 0.0;
  *pixels = 0.0;
  *count = 0.0;
  *percentile = 0.0;

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  if (success)
    {
      *mean = LIGMA_VALUES_GET_DOUBLE (return_vals, 1);
      *std_dev = LIGMA_VALUES_GET_DOUBLE (return_vals, 2);
      *median = LIGMA_VALUES_GET_DOUBLE (return_vals, 3);
      *pixels = LIGMA_VALUES_GET_DOUBLE (return_vals, 4);
      *count = LIGMA_VALUES_GET_DOUBLE (return_vals, 5);
      *percentile = LIGMA_VALUES_GET_DOUBLE (return_vals, 6);
    }

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_hue_saturation:
 * @drawable: The drawable.
 * @hue_range: Range of affected hues.
 * @hue_offset: Hue offset in degrees.
 * @lightness: Lightness modification.
 * @saturation: Saturation modification.
 * @overlap: Overlap other hue channels.
 *
 * Modify hue, lightness, and saturation in the specified drawable.
 *
 * This procedure allows the hue, lightness, and saturation in the
 * specified drawable to be modified. The 'hue-range' parameter
 * provides the capability to limit range of affected hues. The
 * 'overlap' parameter provides blending into neighboring hue channels
 * when rendering.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_hue_saturation (LigmaDrawable *drawable,
                              LigmaHueRange  hue_range,
                              gdouble       hue_offset,
                              gdouble       lightness,
                              gdouble       saturation,
                              gdouble       overlap)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          LIGMA_TYPE_HUE_RANGE, hue_range,
                                          G_TYPE_DOUBLE, hue_offset,
                                          G_TYPE_DOUBLE, lightness,
                                          G_TYPE_DOUBLE, saturation,
                                          G_TYPE_DOUBLE, overlap,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-hue-saturation",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_invert:
 * @drawable: The drawable.
 * @linear: Whether to invert in linear space.
 *
 * Invert the contents of the specified drawable.
 *
 * This procedure inverts the contents of the specified drawable. Each
 * intensity channel is inverted independently. The inverted intensity
 * is given as inten' = (255 - inten). If 'linear' is TRUE, the
 * drawable is inverted in linear space.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_invert (LigmaDrawable *drawable,
                      gboolean      linear)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          G_TYPE_BOOLEAN, linear,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-invert",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_levels:
 * @drawable: The drawable.
 * @channel: The channel to modify.
 * @low_input: Intensity of lowest input.
 * @high_input: Intensity of highest input.
 * @clamp_input: Clamp input values before applying output levels.
 * @gamma: Gamma adjustment factor.
 * @low_output: Intensity of lowest output.
 * @high_output: Intensity of highest output.
 * @clamp_output: Clamp final output values.
 *
 * Modifies intensity levels in the specified drawable.
 *
 * This tool allows intensity levels in the specified drawable to be
 * remapped according to a set of parameters. The low/high input levels
 * specify an initial mapping from the source intensities. The gamma
 * value determines how intensities between the low and high input
 * intensities are interpolated. A gamma value of 1.0 results in a
 * linear interpolation. Higher gamma values result in more high-level
 * intensities. Lower gamma values result in more low-level
 * intensities. The low/high output levels constrain the final
 * intensity mapping--that is, no final intensity will be lower than
 * the low output level and no final intensity will be higher than the
 * high output level. This tool is only valid on RGB color and
 * grayscale images.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_levels (LigmaDrawable         *drawable,
                      LigmaHistogramChannel  channel,
                      gdouble               low_input,
                      gdouble               high_input,
                      gboolean              clamp_input,
                      gdouble               gamma,
                      gdouble               low_output,
                      gdouble               high_output,
                      gboolean              clamp_output)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          LIGMA_TYPE_HISTOGRAM_CHANNEL, channel,
                                          G_TYPE_DOUBLE, low_input,
                                          G_TYPE_DOUBLE, high_input,
                                          G_TYPE_BOOLEAN, clamp_input,
                                          G_TYPE_DOUBLE, gamma,
                                          G_TYPE_DOUBLE, low_output,
                                          G_TYPE_DOUBLE, high_output,
                                          G_TYPE_BOOLEAN, clamp_output,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-levels",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_levels_stretch:
 * @drawable: The drawable.
 *
 * Automatically modifies intensity levels in the specified drawable.
 *
 * This procedure allows intensity levels in the specified drawable to
 * be remapped according to a set of guessed parameters. It is
 * equivalent to clicking the \"Auto\" button in the Levels tool.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_levels_stretch (LigmaDrawable *drawable)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-levels-stretch",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_posterize:
 * @drawable: The drawable.
 * @levels: Levels of posterization.
 *
 * Posterize the specified drawable.
 *
 * This procedures reduces the number of shades allows in each
 * intensity channel to the specified 'levels' parameter.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_posterize (LigmaDrawable *drawable,
                         gint          levels)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          G_TYPE_INT, levels,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-posterize",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}

/**
 * ligma_drawable_threshold:
 * @drawable: The drawable.
 * @channel: The channel to base the threshold on.
 * @low_threshold: The low threshold value.
 * @high_threshold: The high threshold value.
 *
 * Threshold the specified drawable.
 *
 * This procedures generates a threshold map of the specified drawable.
 * All pixels between the values of 'low_threshold' and
 * 'high_threshold', on the scale of 'channel' are replaced with white,
 * and all other pixels with black.
 *
 * Returns: TRUE on success.
 *
 * Since: 2.10
 **/
gboolean
ligma_drawable_threshold (LigmaDrawable         *drawable,
                         LigmaHistogramChannel  channel,
                         gdouble               low_threshold,
                         gdouble               high_threshold)
{
  LigmaValueArray *args;
  LigmaValueArray *return_vals;
  gboolean success = TRUE;

  args = ligma_value_array_new_from_types (NULL,
                                          LIGMA_TYPE_DRAWABLE, drawable,
                                          LIGMA_TYPE_HISTOGRAM_CHANNEL, channel,
                                          G_TYPE_DOUBLE, low_threshold,
                                          G_TYPE_DOUBLE, high_threshold,
                                          G_TYPE_NONE);

  return_vals = ligma_pdb_run_procedure_array (ligma_get_pdb (),
                                              "ligma-drawable-threshold",
                                              args);
  ligma_value_array_unref (args);

  success = LIGMA_VALUES_GET_ENUM (return_vals, 0) == LIGMA_PDB_SUCCESS;

  ligma_value_array_unref (return_vals);

  return success;
}
