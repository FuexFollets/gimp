/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * LIGMA Plug-in for Windows Icon files.
 * Copyright (C) 2002 Christian Kreibich <christian@whoop.org>.
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


#include <config.h>

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

/* #define ICO_DBG */

#include "ico.h"
#include "ico-dialog.h"
#include "ico-save.h"

#include "libligma/stdplugins-intl.h"

static void   ico_dialog_bpp_changed     (GtkWidget   *combo,
                                          GObject     *hbox);
static void   ico_dialog_toggle_compress (GtkWidget   *checkbox,
                                          GObject     *hbox);
static void   ico_dialog_check_compat    (GtkWidget   *dialog,
                                          IcoSaveInfo *info);
static void   ico_dialog_ani_update_inam (GtkEntry    *entry,
                                          gpointer     data);
static void   ico_dialog_ani_update_iart (GtkEntry    *entry,
                                          gpointer     data);


GtkWidget *
ico_dialog_new (IcoSaveInfo   *info,
                AniFileHeader *ani_header,
                AniSaveInfo   *ani_info)
{
  GtkWidget     *dialog;
  GtkWidget     *main_vbox;
  GtkWidget     *vbox;
  GtkWidget     *frame;
  GtkWidget     *scrolled_window;
  GtkWidget     *viewport;
  GtkWidget     *warning;

  dialog = ligma_export_dialog_new (ani_header ?
                                   _("Windows Animated Cursor") : info->is_cursor ?
                                   _("Windows Cursor") : _("Windows Icon"),
                                   PLUG_IN_BINARY,
                                   "plug-in-winicon");

  /* We store an array that holds each icon's requested bit depth
     with the dialog. It's queried when the dialog is closed so the
     save routine knows what colormaps etc to generate in the saved
     file. We store twice the number necessary because in the second
     set, the color depths that are automatically suggested are stored
     for later comparison.
  */

  g_object_set_data (G_OBJECT (dialog), "save_info", info);
  if (ani_header)
    {
      g_object_set_data (G_OBJECT (dialog), "save_ani_header", ani_header);
      g_object_set_data (G_OBJECT (dialog), "save_ani_info", ani_info);
    }

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 6);
  gtk_box_pack_start (GTK_BOX (ligma_export_dialog_get_content_area (dialog)),
                      main_vbox, TRUE, TRUE, 0);
  gtk_widget_show (main_vbox);

  /*Animated Cursor */
  if (ani_header)
    {
      GtkWidget     *grid;
      GtkAdjustment *adjustment;
      GtkWidget     *spin;
      GtkWidget     *label;
      GtkWidget     *hbox;
      GtkWidget     *entry;

      frame = ligma_frame_new (_("Animated Cursor Settings"));
      gtk_box_pack_start (GTK_BOX (main_vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      grid = gtk_grid_new ();
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_container_add (GTK_CONTAINER (frame), grid);
      gtk_widget_show (grid);

      /* Cursor Name */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("_Cursor Name (Optional)"),
                                0.0, 0.5,
                                hbox, 1);

      entry = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (entry),
                          ani_info->inam ? ani_info->inam : "");
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_widget_show (entry);

      g_signal_connect (GTK_ENTRY (entry), "focus-out-event",
                        G_CALLBACK (ico_dialog_ani_update_inam),
                        NULL);

      /* Author Name */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      ligma_grid_attach_aligned (GTK_GRID (grid), 0, 3,
                                _("_Author Name (Optional)"),
                                0.0, 0.5,
                                hbox, 1);

      entry = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (entry),
                          ani_info->iart ? ani_info->iart : "");
      gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
      gtk_widget_show (entry);

      g_signal_connect (GTK_ENTRY (entry), "focus-out-event",
                        G_CALLBACK (ico_dialog_ani_update_iart),
                        NULL);

      /* Default delay spin */
      hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
      ligma_grid_attach_aligned (GTK_GRID (grid), 0, 5,
                                _("_Delay between frames:"),
                                0.0, 0.5,
                                hbox, 1);

      adjustment = gtk_adjustment_new (ani_header->jif_rate, 1, G_MAXINT,
                                       1, 10, 0);
      spin = ligma_spin_button_new (adjustment, 1, 0);
      gtk_box_pack_start (GTK_BOX (hbox), spin, FALSE, FALSE, 0);
      gtk_widget_show (spin);

      label = gtk_label_new (_(" jiffies (16.66 ms)"));
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      g_signal_connect (adjustment, "value-changed",
                        G_CALLBACK (ligma_int_adjustment_update),
                        &ani_header->jif_rate);
    }

  /* Cursor */
  frame = ligma_frame_new (_("Icon Details"));
  gtk_box_pack_start (GTK_BOX (main_vbox), frame, TRUE, TRUE, 4);
  gtk_widget_show (frame);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add (GTK_CONTAINER (frame), scrolled_window);
  gtk_widget_show (scrolled_window);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled_window), viewport);
  gtk_widget_show (viewport);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  g_object_set_data (G_OBJECT (dialog), "icons_vbox", vbox);
  gtk_container_add (GTK_CONTAINER (viewport), vbox);
  gtk_widget_show (vbox);

  warning = g_object_new (LIGMA_TYPE_HINT_BOX,
                          "icon-name", LIGMA_ICON_DIALOG_WARNING,
                          "hint",
                          _("Large icons and compression are not supported "
                            "by all programs. Older applications may not "
                            "open this file correctly."),
                          NULL);
  gtk_box_pack_end (GTK_BOX (main_vbox), warning, FALSE, FALSE, 12);
  /* don't show the warning here */

  g_object_set_data (G_OBJECT (dialog), "warning", warning);

  return dialog;
}

static GtkWidget *
ico_preview_new (LigmaDrawable *layer)
{
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  gint       width  = ligma_drawable_get_width (layer);
  gint       height = ligma_drawable_get_height (layer);

  pixbuf = ligma_drawable_get_thumbnail (layer,
                                        MIN (width, 128), MIN (height, 128),
                                        LIGMA_PIXBUF_SMALL_CHECKS);
  image = gtk_image_new_from_pixbuf (pixbuf);
  g_object_unref (pixbuf);

  return image;
}

/* This function creates and returns an hbox for an icon,
   which then gets added to the dialog's main vbox. */
static GtkWidget *
ico_create_icon_hbox (GtkWidget    *icon_preview,
                      LigmaDrawable *layer,
                      gint          layer_num,
                      IcoSaveInfo  *info)
{
  static GtkSizeGroup *size = NULL;

  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *combo;
  GtkWidget *checkbox;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  /* To make life easier for the callbacks, we store the
     layer's ID and stacking number with the hbox. */

  g_object_set_data (G_OBJECT (hbox),
                     "icon_layer", layer);
  g_object_set_data (G_OBJECT (hbox),
                     "icon_layer_num", GINT_TO_POINTER (layer_num));

  g_object_set_data (G_OBJECT (hbox), "icon_preview", icon_preview);
  gtk_widget_set_halign (icon_preview, GTK_ALIGN_END);
  gtk_widget_set_valign (icon_preview, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (hbox), icon_preview, FALSE, FALSE, 0);
  gtk_widget_show (icon_preview);

  if (! size)
    size = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  gtk_size_group_add_widget (size, icon_preview);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  combo = ligma_int_combo_box_new (_("1 bpp, 1-bit alpha, 2-slot palette"),   1,
                                  _("4 bpp, 1-bit alpha, 16-slot palette"),  4,
                                  _("8 bpp, 1-bit alpha, 256-slot palette"), 8,
                                  _("24 bpp, 1-bit alpha, no palette"),     24,
                                  _("32 bpp, 8-bit alpha, no palette"),     32,
                                  NULL);
  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo),
                                 info->depths[layer_num]);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (ico_dialog_bpp_changed),
                    hbox);

  g_object_set_data (G_OBJECT (hbox), "icon_menu", combo);

  gtk_box_pack_start (GTK_BOX (vbox), combo, FALSE, FALSE, 0);
  gtk_widget_show (combo);

  checkbox = gtk_check_button_new_with_label (_("Compressed (PNG)"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbox),
                                info->compress[layer_num]);
  g_signal_connect (checkbox, "toggled",
                    G_CALLBACK (ico_dialog_toggle_compress), hbox);
  gtk_box_pack_start (GTK_BOX (vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show (checkbox);

  return hbox;
}

static GtkWidget *
ico_dialog_get_layer_preview (GtkWidget    *dialog,
                              LigmaDrawable *layer)
{
  GtkWidget *preview;
  GtkWidget *icon_hbox;
  gchar      key[ICO_MAXBUF];

  g_snprintf (key, sizeof (key), "layer_%i_hbox",
              ligma_item_get_id (LIGMA_ITEM (layer)));
  icon_hbox = g_object_get_data (G_OBJECT (dialog), key);

  if (!icon_hbox)
    {
      D(("Something's wrong -- couldn't look up hbox by layer ID\n"));
      return NULL;
    }

  preview = g_object_get_data (G_OBJECT (icon_hbox), "icon_preview");

  if (!icon_hbox)
    {
      D(("Something's wrong -- couldn't look up preview from hbox\n"));
      return NULL;
    }

  return preview;
}

static void
ico_dialog_update_icon_preview (GtkWidget    *dialog,
                                LigmaDrawable *layer,
                                gint          bpp)
{
  GtkWidget  *preview = ico_dialog_get_layer_preview (dialog, layer);
  GdkPixbuf  *pixbuf;
  const Babl *format;
  gint        w       = ligma_drawable_get_width (layer);
  gint        h       = ligma_drawable_get_height (layer);

  if (! preview)
    return;

  switch (ligma_drawable_type (layer))
    {
    case LIGMA_RGB_IMAGE:
      format = babl_format ("R'G'B' u8");
      break;

    case LIGMA_RGBA_IMAGE:
      format = babl_format ("R'G'B'A u8");
      break;

    case LIGMA_GRAY_IMAGE:
      format = babl_format ("Y' u8");
      break;

    case LIGMA_GRAYA_IMAGE:
      format = babl_format ("Y'A u8");
      break;

    case LIGMA_INDEXED_IMAGE:
    case LIGMA_INDEXEDA_IMAGE:
      format = ligma_drawable_get_format (layer);
      break;

    default:
      g_return_if_reached ();
    }

  if (bpp <= 8)
    {
      GeglBuffer *buffer;
      GeglBuffer *tmp;
      LigmaImage  *image;
      LigmaImage  *tmp_image;
      LigmaLayer  *tmp_layer;
      guchar     *buf;
      guchar     *cmap;
      gint        num_colors;

      image = ligma_item_get_image (LIGMA_ITEM (layer));

      tmp_image = ligma_image_new (w, h, ligma_image_get_base_type (image));
      ligma_image_undo_disable (tmp_image);

      if (ligma_drawable_is_indexed (layer))
        {
          cmap = ligma_image_get_colormap (image, &num_colors);
          ligma_image_set_colormap (tmp_image, cmap, num_colors);
          g_free (cmap);
        }

      tmp_layer = ligma_layer_new (tmp_image, "temporary", w, h,
                                  ligma_drawable_type (layer),
                                  100,
                                  ligma_image_get_default_new_layer_mode (tmp_image));
      ligma_image_insert_layer (tmp_image, tmp_layer, NULL, 0);

      buffer = ligma_drawable_get_buffer (layer);
      tmp    = ligma_drawable_get_buffer (LIGMA_DRAWABLE (tmp_layer));

      buf = g_malloc (w * h * 4);

      gegl_buffer_get (buffer, GEGL_RECTANGLE (0, 0, w, h), 1.0,
                       format, buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, tmp, NULL);

      g_object_unref (tmp);
      g_object_unref (buffer);

      if (ligma_drawable_is_indexed (layer))
        ligma_image_convert_rgb (tmp_image);

      ligma_image_convert_indexed (tmp_image,
                                  LIGMA_CONVERT_DITHER_FS,
                                  LIGMA_CONVERT_PALETTE_GENERATE,
                                  1 << bpp, TRUE, FALSE, "dummy");

      cmap = ligma_image_get_colormap (tmp_image, &num_colors);

      if (num_colors == (1 << bpp) &&
          ! ico_cmap_contains_black (cmap, num_colors))
        {
          /* Windows icons with color maps need the color black.
           * We need to eliminate one more color to make room for black.
           */
          if (ligma_drawable_is_indexed (layer))
            {
              g_free (cmap);
              cmap = ligma_image_get_colormap (image, &num_colors);
              ligma_image_set_colormap (tmp_image, cmap, num_colors);
            }
          else if (ligma_drawable_is_gray (layer))
            {
              ligma_image_convert_grayscale (tmp_image);
            }
          else
            {
              ligma_image_convert_rgb (tmp_image);
            }

          tmp = ligma_drawable_get_buffer (LIGMA_DRAWABLE (tmp_layer));

          gegl_buffer_set (tmp, GEGL_RECTANGLE (0, 0, w, h), 0,
                           format, buf, GEGL_AUTO_ROWSTRIDE);

          g_object_unref (tmp);

          if (!ligma_drawable_is_rgb (layer))
            ligma_image_convert_rgb (tmp_image);

          ligma_image_convert_indexed (tmp_image,
                                      LIGMA_CONVERT_DITHER_FS,
                                      LIGMA_CONVERT_PALETTE_GENERATE,
                                      (1 << bpp) - 1, TRUE, FALSE, "dummy");
        }

      g_free (cmap);
      g_free (buf);

      pixbuf = ligma_drawable_get_thumbnail (LIGMA_DRAWABLE (tmp_layer),
                                            MIN (w, 128), MIN (h, 128),
                                            LIGMA_PIXBUF_SMALL_CHECKS);

      ligma_image_delete (tmp_image);
    }
  else if (bpp == 24)
    {
      GeglBuffer     *buffer;
      GeglBuffer     *tmp;
      LigmaImage      *image;
      LigmaImage      *tmp_image;
      LigmaLayer      *tmp_layer;
      LigmaValueArray *return_vals;

      image = ligma_item_get_image (LIGMA_ITEM (layer));

      tmp_image = ligma_image_new (w, h, ligma_image_get_base_type (image));
      ligma_image_undo_disable (tmp_image);

      if (ligma_drawable_is_indexed (layer))
        {
          guchar *cmap;
          gint    num_colors;

          cmap = ligma_image_get_colormap (image, &num_colors);
          ligma_image_set_colormap (tmp_image, cmap, num_colors);
          g_free (cmap);
        }

      tmp_layer = ligma_layer_new (tmp_image, "temporary", w, h,
                                  ligma_drawable_type (layer),
                                  100,
                                  ligma_image_get_default_new_layer_mode (tmp_image));
      ligma_image_insert_layer (tmp_image, tmp_layer, NULL, 0);

      buffer = ligma_drawable_get_buffer (layer);
      tmp    = ligma_drawable_get_buffer (LIGMA_DRAWABLE (tmp_layer));

      gegl_buffer_copy (buffer, NULL, GEGL_ABYSS_NONE, tmp, NULL);

      g_object_unref (tmp);
      g_object_unref (buffer);

      if (ligma_drawable_is_indexed (layer))
        ligma_image_convert_rgb (tmp_image);

      return_vals =
        ligma_pdb_run_procedure (ligma_get_pdb (),
                                "plug-in-threshold-alpha",
                                LIGMA_TYPE_RUN_MODE,  LIGMA_RUN_NONINTERACTIVE,
                                LIGMA_TYPE_IMAGE,     tmp_image,
                                LIGMA_TYPE_DRAWABLE,  tmp_layer,
                                G_TYPE_INT,          ICO_ALPHA_THRESHOLD,
                                G_TYPE_NONE);

      ligma_value_array_unref (return_vals);

      pixbuf = ligma_drawable_get_thumbnail (LIGMA_DRAWABLE (tmp_layer),
                                            MIN (w, 128), MIN (h, 128),
                                            LIGMA_PIXBUF_SMALL_CHECKS);

      ligma_image_delete (tmp_image);
    }
  else
    {
      pixbuf = ligma_drawable_get_thumbnail (layer,
                                            MIN (w, 128), MIN (h, 128),
                                            LIGMA_PIXBUF_SMALL_CHECKS);
    }

  gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
  g_object_unref (pixbuf);
}

void
ico_dialog_add_icon (GtkWidget    *dialog,
                     LigmaDrawable *layer,
                     gint          layer_num)
{
  GtkWidget   *vbox;
  GtkWidget   *hbox;
  GtkWidget   *preview;
  gchar        key[ICO_MAXBUF];
  IcoSaveInfo *info;

  vbox = g_object_get_data (G_OBJECT (dialog), "icons_vbox");
  info = g_object_get_data (G_OBJECT (dialog), "save_info");

  preview = ico_preview_new (layer);
  hbox = ico_create_icon_hbox (preview, layer, layer_num, info);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  /* Let's make the hbox accessible through the layer ID */
  g_snprintf (key, sizeof (key), "layer_%i_hbox",
              ligma_item_get_id (LIGMA_ITEM (layer)));
  g_object_set_data (G_OBJECT (dialog), key, hbox);

  ico_dialog_update_icon_preview (dialog, layer, info->depths[layer_num]);

  ico_dialog_check_compat (dialog, info);

  /* Cursor */
  if (info->is_cursor)
    {
      GtkWidget     *grid;
      GtkAdjustment *adj;
      GtkWidget     *spinbutton;

      grid = gtk_grid_new ();
      gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
      gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
      gtk_box_pack_start (GTK_BOX (hbox), grid, FALSE, FALSE, 0);
      gtk_widget_show (grid);

      adj = (GtkAdjustment *)
             gtk_adjustment_new (info->hot_spot_x[layer_num], 0,
                                 G_MAXUINT16, 1, 10, 0);
      spinbutton = ligma_spin_button_new (adj, 1.0, 0);
      gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbutton),
                                 0, G_MAXUINT16);
      ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                _("Hot spot _X:"), 0.0, 0.5,
                                spinbutton, 1);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (ligma_int_adjustment_update),
                        &info->hot_spot_x[layer_num]);

      adj = (GtkAdjustment *)
             gtk_adjustment_new (info->hot_spot_y[layer_num], 0,
                                 G_MAXUINT16, 1, 10, 0);
      spinbutton = ligma_spin_button_new (adj, 1.0, 0);
      gtk_spin_button_set_range (GTK_SPIN_BUTTON (spinbutton),
                                 0, G_MAXUINT16);
      ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                _("Hot spot _Y:"), 0.0, 0.5,
                                spinbutton, 1);
      g_signal_connect (adj, "value-changed",
                        G_CALLBACK (ligma_int_adjustment_update),
                        &info->hot_spot_y[layer_num]);
    }
}

static void
ico_dialog_bpp_changed (GtkWidget *combo,
                        GObject   *hbox)
{
  GtkWidget    *dialog;
  LigmaDrawable *layer;
  gint          layer_num;
  gint          bpp;
  IcoSaveInfo  *info;

  dialog = gtk_widget_get_toplevel (combo);

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (combo), &bpp);

  info = g_object_get_data (G_OBJECT (dialog), "save_info");
  g_assert (info);

  layer     = g_object_get_data (hbox, "icon_layer");
  layer_num = GPOINTER_TO_INT (g_object_get_data (hbox, "icon_layer_num"));

  /* Update vector entry for later when we're actually saving,
     and update the preview right away ... */
  info->depths[layer_num] = bpp;
  ico_dialog_update_icon_preview (dialog, layer, bpp);
}

static void
ico_dialog_toggle_compress (GtkWidget *checkbox,
                            GObject   *hbox)
{
  GtkWidget   *dialog;
  gint         layer_num;
  IcoSaveInfo *info;

  dialog = gtk_widget_get_toplevel (checkbox);

  info = g_object_get_data (G_OBJECT (dialog), "save_info");
  g_assert (info);
  layer_num = GPOINTER_TO_INT (g_object_get_data (hbox, "icon_layer_num"));

  /* Update vector entry for later when we're actually saving */
  info->compress[layer_num] =
    gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (checkbox));

  ico_dialog_check_compat (dialog, info);
}

static void
ico_dialog_check_compat (GtkWidget   *dialog,
                         IcoSaveInfo *info)
{
  GtkWidget *warning;
  GList     *iter;
  gboolean   warn = FALSE;
  gint       i;

  for (iter = info->layers, i = 0; iter; iter = iter->next, i++)
    {
      if (ligma_drawable_get_width (iter->data) > 255  ||
          ligma_drawable_get_height (iter->data) > 255 ||
          info->compress[i])
        {
          warn = TRUE;
          break;
        }
    }

  warning = g_object_get_data (G_OBJECT (dialog), "warning");

  gtk_widget_set_visible (warning, warn);
}

static void
ico_dialog_ani_update_inam (GtkEntry *entry,
                            gpointer  data)
{
  AniSaveInfo *ani_info;
  GtkWidget   *dialog;

  dialog = gtk_widget_get_toplevel (GTK_WIDGET (entry));

  ani_info = g_object_get_data (G_OBJECT (dialog), "save_ani_info");
  ani_info->inam = g_strdup_printf ("%s", gtk_entry_get_text (entry));
}

static void
ico_dialog_ani_update_iart (GtkEntry *entry,
                            gpointer  data)
{
  AniSaveInfo *ani_info;
  GtkWidget   *dialog;

  dialog = gtk_widget_get_toplevel (GTK_WIDGET (entry));

  ani_info = g_object_get_data (G_OBJECT (dialog), "save_ani_info");
  ani_info->iart = g_strdup_printf ("%s", gtk_entry_get_text (entry));
}
