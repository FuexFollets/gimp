/**************************************************************/
/* Dialog creation and updaters, callbacks and event-handlers */
/**************************************************************/

#include "config.h"

#include <libligma/ligma.h>
#include <libligma/ligmaui.h>

#include "arcball.h"
#include "map-object-ui.h"
#include "map-object-icons.h"
#include "map-object-image.h"
#include "map-object-apply.h"
#include "map-object-preview.h"
#include "map-object-main.h"

#include "libligma/stdplugins-intl.h"


GtkWidget     *previewarea = NULL;

static GtkWidget   *appwin            = NULL;
static GtkNotebook *options_note_book = NULL;

static GtkWidget *pointlightwid;
static GtkWidget *dirlightwid;

static GtkAdjustment *xadj, *yadj, *zadj;

static GtkWidget *box_page      = NULL;
static GtkWidget *cylinder_page = NULL;

static guint left_button_pressed = FALSE;
static guint light_hit           = FALSE;


static void create_main_notebook       (GtkWidget     *container);

static gint preview_events             (GtkWidget     *area,
                                        GdkEvent      *event);

static void update_light_pos_entries   (void);

static void double_adjustment_update   (GtkAdjustment *adjustment,
                                        gpointer       data);

static void toggle_update              (GtkWidget     *widget,
                                        gpointer       data);

static void lightmenu_callback         (GtkWidget     *widget,
                                        gpointer       data);

static void preview_callback           (GtkWidget     *widget,
                                        gpointer       data);

static gint box_constrain              (LigmaImage     *image,
                                        LigmaItem      *item,
                                        gpointer       data);
static gint cylinder_constrain         (LigmaImage     *image,
                                        LigmaItem      *item,
                                        gpointer       data);

static GtkWidget * create_options_page     (void);
static GtkWidget * create_light_page       (void);
static GtkWidget * create_material_page    (void);
static GtkWidget * create_orientation_page (void);
static GtkWidget * create_box_page         (void);
static GtkWidget * create_cylinder_page    (void);


/******************************************************/
/* Update angle & position (redraw grid if necessary) */
/******************************************************/

static void
scale_entry_update_double (LigmaLabelSpin *entry,
                           gdouble       *value)
{
  *value = ligma_label_spin_get_value (entry);

  if (mapvals.livepreview)
    compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

static void
double_adjustment_update (GtkAdjustment *adjustment,
                          gpointer       data)
{
  ligma_double_adjustment_update (adjustment, data);

  if (mapvals.livepreview)
    compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

static void
update_light_pos_entries (void)
{
  g_signal_handlers_block_by_func (xadj,
                                   double_adjustment_update,
                                   &mapvals.lightsource.position.x);
  gtk_adjustment_set_value (xadj,
                            mapvals.lightsource.position.x);
  g_signal_handlers_unblock_by_func (xadj,
                                     double_adjustment_update,
                                     &mapvals.lightsource.position.x);

  g_signal_handlers_block_by_func (yadj,
                                   double_adjustment_update,
                                   &mapvals.lightsource.position.y);
  gtk_adjustment_set_value (yadj,
                            mapvals.lightsource.position.y);
  g_signal_handlers_unblock_by_func (yadj,
                                     double_adjustment_update,
                                     &mapvals.lightsource.position.y);

  g_signal_handlers_block_by_func (zadj,
                                   double_adjustment_update,
                                   &mapvals.lightsource.position.z);
  gtk_adjustment_set_value (zadj,
                            mapvals.lightsource.position.z);
  g_signal_handlers_unblock_by_func (zadj,
                                     double_adjustment_update,
                                     &mapvals.lightsource.position.z);
}

/**********************/
/* Std. toggle update */
/**********************/

static void
toggle_update (GtkWidget *widget,
               gpointer   data)
{
  ligma_toggle_button_update (widget, data);

  compute_preview_image ();
  gtk_widget_queue_draw (previewarea);
}

/*****************************************/
/* Main window light type menu callback. */
/*****************************************/

static void
lightmenu_callback (GtkWidget *widget,
                    gpointer   data)
{
  int active;

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget),
                                 &active);

  mapvals.lightsource.type = active;

  if (mapvals.lightsource.type == POINT_LIGHT)
    {
      gtk_widget_hide (dirlightwid);
      gtk_widget_show (pointlightwid);
    }
  else if (mapvals.lightsource.type == DIRECTIONAL_LIGHT)
    {
      gtk_widget_hide (pointlightwid);
      gtk_widget_show (dirlightwid);
    }
  else
    {
      gtk_widget_hide (pointlightwid);
      gtk_widget_hide (dirlightwid);
    }

  if (mapvals.livepreview)
    {
      compute_preview_image ();
      gtk_widget_queue_draw (previewarea);
    }
}

/***************************************/
/* Main window map type menu callback. */
/***************************************/

static void
mapmenu_callback (GtkWidget *widget,
                  gpointer   data)
{
  int active;

  ligma_int_combo_box_get_active (LIGMA_INT_COMBO_BOX (widget),
                                 &active);

  mapvals.maptype = active;

  if (mapvals.livepreview)
    {
      compute_preview_image ();
      gtk_widget_queue_draw (previewarea);
    }

  if (mapvals.maptype == MAP_BOX)
    {
      if (cylinder_page && gtk_widget_get_parent (GTK_WIDGET (cylinder_page)) ==
          GTK_WIDGET (options_note_book))
        {
          gtk_container_remove (GTK_CONTAINER (options_note_book), cylinder_page);
        }

      if (!box_page)
        {
          box_page = create_box_page ();
          g_object_ref (box_page);
        }
      gtk_notebook_append_page (options_note_book,
                                box_page,
                                gtk_label_new_with_mnemonic (_("_Box")));
    }
  else if (mapvals.maptype == MAP_CYLINDER)
    {
      if (box_page && gtk_widget_get_parent (GTK_WIDGET (box_page)) ==
          GTK_WIDGET (options_note_book))
        {
          gtk_container_remove (GTK_CONTAINER (options_note_book), box_page);
        }

      if (!cylinder_page)
        {
          cylinder_page = create_cylinder_page ();
          g_object_ref (cylinder_page);
        }
      gtk_notebook_append_page (options_note_book,
                                cylinder_page,
                                gtk_label_new_with_mnemonic (_("C_ylinder")));
    }
  else
    {
      if (box_page && gtk_widget_get_parent (GTK_WIDGET (box_page)) ==
          GTK_WIDGET (options_note_book))
        {
          gtk_container_remove (GTK_CONTAINER (options_note_book), box_page);
        }

      if (cylinder_page && gtk_widget_get_parent (GTK_WIDGET (cylinder_page)) ==
          GTK_WIDGET (options_note_book))
        {
          gtk_container_remove (GTK_CONTAINER (options_note_book), cylinder_page);
        }
    }
}

/******************************************/
/* Main window "Preview!" button callback */
/******************************************/

static void
preview_callback (GtkWidget *widget,
                  gpointer   data)
{
  compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

static void
zoomed_callback (LigmaZoomModel *model)
{
  mapvals.zoom = ligma_zoom_model_get_factor (model);

  compute_preview_image ();

  gtk_widget_queue_draw (previewarea);
}

/**********************************************/
/* Main window "Apply" button callback.       */
/* Render to LIGMA image, close down and exit. */
/**********************************************/

static gint
box_constrain (LigmaImage *image,
               LigmaItem  *item,
               gpointer   data)
{
  return (ligma_drawable_is_rgb (LIGMA_DRAWABLE (item)) &&
          ! ligma_drawable_is_indexed (LIGMA_DRAWABLE (item)));
}

static gint
cylinder_constrain (LigmaImage *image,
                    LigmaItem  *item,
                    gpointer   data)
{
  return (ligma_drawable_is_rgb (LIGMA_DRAWABLE (item)) &&
          ! ligma_drawable_is_indexed (LIGMA_DRAWABLE (item)));
}

/******************************/
/* Preview area event handler */
/******************************/

static gint
preview_events (GtkWidget *area,
                GdkEvent  *event)
{
  HVect __attribute__((unused))pos;
/*  HMatrix RotMat;
  gdouble a,b,c; */

  switch (event->type)
    {
      case GDK_ENTER_NOTIFY:
        break;

      case GDK_LEAVE_NOTIFY:
        break;

      case GDK_BUTTON_PRESS:
        light_hit = check_light_hit (event->button.x, event->button.y);
        if (light_hit == FALSE)
          {
            pos.x = -(2.0 * (gdouble) event->button.x /
                      (gdouble) PREVIEW_WIDTH - 1.0);
            pos.y = (2.0 * (gdouble) event->button.y /
                     (gdouble) PREVIEW_HEIGHT - 1.0);
            /*ArcBall_Mouse(pos);
            ArcBall_BeginDrag(); */
          }
        left_button_pressed = TRUE;
        break;

      case GDK_BUTTON_RELEASE:
        if (light_hit == TRUE)
          {
            compute_preview_image ();

            gtk_widget_queue_draw (previewarea);
          }
        else
          {
            pos.x = -(2.0 * (gdouble) event->button.x /
                      (gdouble) PREVIEW_WIDTH - 1.0);
            pos.y = (2.0 * (gdouble) event->button.y /
                     (gdouble) PREVIEW_HEIGHT - 1.0);
            /*ArcBall_Mouse(pos);
            ArcBall_EndDrag(); */
          }
        left_button_pressed = FALSE;
        break;

      case GDK_MOTION_NOTIFY:
        if (left_button_pressed == TRUE)
          {
            if (light_hit == TRUE)
              {
                gint live = mapvals.livepreview;

                mapvals.livepreview = FALSE;
                update_light (event->motion.x, event->motion.y);
                update_light_pos_entries ();
                mapvals.livepreview = live;

                gtk_widget_queue_draw (previewarea);
              }
            else
              {
                pos.x = -(2.0 * (gdouble) event->motion.x /
                          (gdouble) PREVIEW_WIDTH - 1.0);
                pos.y = (2.0 * (gdouble) event->motion.y /
                         (gdouble) PREVIEW_HEIGHT - 1.0);
/*                ArcBall_Mouse(pos);
                ArcBall_Update();
                ArcBall_Values(&a,&b,&c);
                Alpha+=RadToDeg(-a);
                Beta+RadToDeg(-b);
                Gamma+=RadToDeg(-c);
                if (Alpha>180) Alpha-=360;
                if (Alpha<-180) Alpha+=360;
                if (Beta>180) Beta-=360;
                if (Beta<-180) Beta+=360;
                if (Gamma>180) Gamma-=360;
                if (Gamma<-180) Gamma+=360;
                      UpdateAngleSliders(); */
              }
          }
        break;

      default:
        break;
    }

  return FALSE;
}


static GtkWidget *
spin_button_new (GtkAdjustment **adjustment,  /* return value */
                 gdouble         value,
                 gdouble         lower,
                 gdouble         upper,
                 gdouble         step_increment,
                 gdouble         page_increment,
                 gdouble         page_size,
                 gdouble         climb_rate,
                 guint           digits)
{
  GtkWidget *spinbutton;

  *adjustment = gtk_adjustment_new (value, lower, upper,
                                    step_increment, page_increment, 0);

  spinbutton = ligma_spin_button_new (*adjustment,
                                     climb_rate, digits);

  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);

  return spinbutton;
}

/*******************************/
/* Create general options page */
/*******************************/

static GtkWidget *
create_options_page (void)
{
  GtkWidget     *page;
  GtkWidget     *frame;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *label;
  GtkWidget     *combo;
  GtkWidget     *toggle;
  GtkWidget     *grid;
  GtkWidget     *spinbutton;
  GtkWidget     *scale;
  GtkAdjustment *adj;

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  /* General options */

  frame = ligma_frame_new (_("General Options"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  label = gtk_label_new (_("Map to:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  combo = ligma_int_combo_box_new (_("Plane"),    MAP_PLANE,
                                  _("Sphere"),   MAP_SPHERE,
                                  _("Box"),      MAP_BOX,
                                  _("Cylinder"), MAP_CYLINDER,
                                  NULL);
  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo), mapvals.maptype);
  gtk_box_pack_start (GTK_BOX (hbox), combo, TRUE, TRUE, 0);
  gtk_widget_show (combo);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (mapmenu_callback),
                    &mapvals.maptype);

  ligma_help_set_help_data (combo, _("Type of object to map to"), NULL);

  toggle = gtk_check_button_new_with_label (_("Transparent background"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mapvals.transparent_background);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.transparent_background);

  ligma_help_set_help_data (toggle,
                           _("Make image transparent outside object"), NULL);

  toggle = gtk_check_button_new_with_label (_("Tile source image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mapvals.tiled);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.tiled);

  ligma_help_set_help_data (toggle,
                           _("Tile source image: useful for infinite planes"),
                           NULL);

  toggle = gtk_check_button_new_with_label (_("Create new image"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mapvals.create_new_image);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &mapvals.create_new_image);

  ligma_help_set_help_data (toggle,
                           _("Create a new image when applying filter"), NULL);

  toggle = gtk_check_button_new_with_label (_("Create new layer"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mapvals.create_new_layer);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &mapvals.create_new_layer);

  ligma_help_set_help_data (toggle,
                           _("Create a new layer when applying filter"), NULL);

  /* Antialiasing options */

  frame = ligma_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  toggle = gtk_check_button_new_with_mnemonic (_("Enable _antialiasing"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
                                mapvals.antialiasing);
  gtk_frame_set_label_widget (GTK_FRAME (frame), toggle);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (ligma_toggle_button_update),
                    &mapvals.antialiasing);

  ligma_help_set_help_data (toggle,
                           _("Enable/disable jagged edges removal "
                             "(antialiasing)"), NULL);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  g_object_bind_property (toggle, "active",
                          grid,   "sensitive",
                          G_BINDING_SYNC_CREATE);

  scale = ligma_scale_entry_new (_("_Depth:"), mapvals.maxdepth, 1.0, 5.0, 1);
  ligma_help_set_help_data (scale, _("Antialiasing quality. Higher is better, "
                                    "but slower"), NULL);
  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.maxdepth);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 0, 3, 1);
  gtk_widget_show (scale);

  spinbutton = spin_button_new (&adj, mapvals.pixelthreshold,
                                0.001, 1000, 0.1, 1, 0, 0, 3);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("_Threshold:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.pixelthreshold);

  ligma_help_set_help_data (spinbutton,
                           _("Stop when pixel differences are smaller than "
                             "this value"), NULL);

  gtk_widget_show (page);

  return page;
}

/******************************/
/* Create light settings page */
/******************************/

static GtkWidget *
create_light_page (void)
{
  GtkWidget     *page;
  GtkWidget     *frame;
  GtkWidget     *grid;
  GtkWidget     *combo;
  GtkWidget     *colorbutton;
  GtkWidget     *spinbutton;
  GtkAdjustment *adj;

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = ligma_frame_new (_("Light Settings"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  combo = ligma_int_combo_box_new (_("Point light"),       POINT_LIGHT,
                                  _("Directional light"), DIRECTIONAL_LIGHT,
                                  _("No light"),          NO_LIGHT,
                                  NULL);
  ligma_int_combo_box_set_active (LIGMA_INT_COMBO_BOX (combo),
                                 mapvals.lightsource.type);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("Lightsource type:"), 0.0, 0.5,
                            combo, 1);

  g_signal_connect (combo, "changed",
                    G_CALLBACK (lightmenu_callback),
                    &mapvals.lightsource.type);

  ligma_help_set_help_data (combo, _("Type of light source to apply"), NULL);

  colorbutton = ligma_color_button_new (_("Select lightsource color"),
                                       64, 16,
                                       &mapvals.lightsource.color,
                                       LIGMA_COLOR_AREA_FLAT);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Lightsource color:"), 0.0, 0.5,
                            colorbutton, 1);

  g_signal_connect (colorbutton, "color-changed",
                    G_CALLBACK (ligma_color_button_get_color),
                    &mapvals.lightsource.color);

  ligma_help_set_help_data (colorbutton,
                           _("Set light source color"), NULL);

  pointlightwid = ligma_frame_new (_("Position"));
  gtk_box_pack_start (GTK_BOX (page), pointlightwid, FALSE, FALSE, 0);

  if (mapvals.lightsource.type == POINT_LIGHT)
    gtk_widget_show (pointlightwid);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (pointlightwid), grid);
  gtk_widget_show (grid);

  spinbutton = spin_button_new (&xadj, mapvals.lightsource.position.x,
                                -G_MAXFLOAT, G_MAXFLOAT,
                                0.1, 1.0, 0.0, 0.0, 2);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("X:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (xadj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.lightsource.position.x);

  ligma_help_set_help_data (spinbutton,
                           _("Light source X position in XYZ space"), NULL);

  spinbutton = spin_button_new (&yadj, mapvals.lightsource.position.y,
                                -G_MAXFLOAT, G_MAXFLOAT,
                                0.1, 1.0, 0.0, 0.0, 2);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Y:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (yadj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.lightsource.position.y);

  ligma_help_set_help_data (spinbutton,
                           _("Light source Y position in XYZ space"), NULL);

  spinbutton = spin_button_new (&zadj, mapvals.lightsource.position.z,
                                -G_MAXFLOAT, G_MAXFLOAT,
                                0.1, 1.0, 0.0, 0.0, 2);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("Z:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (zadj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.lightsource.position.z);

  ligma_help_set_help_data (spinbutton,
                           _("Light source Z position in XYZ space"), NULL);


  dirlightwid = ligma_frame_new (_("Direction Vector"));
  gtk_box_pack_start (GTK_BOX (page), dirlightwid, FALSE, FALSE, 0);

  if (mapvals.lightsource.type == DIRECTIONAL_LIGHT)
    gtk_widget_show (dirlightwid);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (dirlightwid), grid);
  gtk_widget_show (grid);

  spinbutton = spin_button_new (&adj, mapvals.lightsource.direction.x,
                                -1.0, 1.0, 0.01, 0.1, 0.0, 0.0, 2);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                            _("X:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.lightsource.direction.x);

  ligma_help_set_help_data (spinbutton,
                           _("Light source X direction in XYZ space"), NULL);

  spinbutton = spin_button_new (&adj, mapvals.lightsource.direction.y,
                                -1.0, 1.0, 0.01, 0.1, 0.0, 0.0, 2);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                            _("Y:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.lightsource.direction.y);

  ligma_help_set_help_data (spinbutton,
                           _("Light source Y direction in XYZ space"), NULL);

  spinbutton = spin_button_new (&adj, mapvals.lightsource.direction.z,
                                -1.0, 1.0, 0.01, 0.1, 0.0, 0.0, 2);
  ligma_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                            _("Z:"), 0.0, 0.5,
                            spinbutton, 1);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.lightsource.direction.z);

  ligma_help_set_help_data (spinbutton,
                           _("Light source Z direction in XYZ space"), NULL);

  gtk_widget_show (page);

  return page;
}

/*********************************/
/* Create material settings page */
/*********************************/

static GtkWidget *
create_material_page (void)
{
  GtkSizeGroup  *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GtkWidget     *page;
  GtkWidget     *frame;
  GtkWidget     *grid;
  GtkWidget     *label;
  GtkWidget     *hbox;
  GtkWidget     *spinbutton;
  GtkWidget     *image;
  GtkAdjustment *adj;

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = ligma_frame_new (_("Intensity Levels"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign (hbox, GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (hbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* Ambient intensity */

  image = gtk_image_new_from_icon_name (MAPOBJECT_INTENSITY_AMBIENT_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                    _("Ambient:"), 0.0, 0.5,
                                    image, 1);
  gtk_size_group_add_widget (group, label);

  spinbutton = spin_button_new (&adj, mapvals.material.ambient_int,
                                0, G_MAXFLOAT, 0.1, 1.0, 0.0, 0.0, 2);
  gtk_grid_attach (GTK_GRID (grid), spinbutton, 2, 0, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.material.ambient_int);

  ligma_help_set_help_data (spinbutton,
                           _("Amount of original color to show where no "
                             "direct light falls"), NULL);

  image = gtk_image_new_from_icon_name (MAPOBJECT_INTENSITY_AMBIENT_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_grid_attach (GTK_GRID (grid), image, 3, 0, 1, 1);
  gtk_widget_show (image);

  /* Diffuse intensity */

  image = gtk_image_new_from_icon_name (MAPOBJECT_INTENSITY_DIFFUSE_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                    _("Diffuse:"), 0.0, 0.5,
                                    image, 1);
  gtk_size_group_add_widget (group, label);

  spinbutton = spin_button_new (&adj, mapvals.material.diffuse_int,
                                0, G_MAXFLOAT, 0.1, 1.0, 0.0, 0.0, 2);
  gtk_grid_attach (GTK_GRID (grid), spinbutton, 2, 1, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.material.diffuse_int);

  ligma_help_set_help_data (spinbutton,
                           _("Intensity of original color when lit by a light "
                             "source"), NULL);

  image = gtk_image_new_from_icon_name (MAPOBJECT_INTENSITY_DIFFUSE_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_grid_attach (GTK_GRID (grid), image, 3, 1, 1, 1);
  gtk_widget_show (image);

  frame = ligma_frame_new (_("Reflectivity"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_set_halign (hbox, GTK_ALIGN_START);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (hbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (hbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  /* Diffuse reflection */

  image = gtk_image_new_from_icon_name (MAPOBJECT_REFLECTIVITY_DIFFUSE_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 0,
                                    _("Diffuse:"), 0.0, 0.5,
                                    image, 1);
  gtk_size_group_add_widget (group, label);

  spinbutton = spin_button_new (&adj, mapvals.material.diffuse_ref,
                                0, G_MAXFLOAT, 0.1, 1.0, 0.0, 0.0, 2);
  gtk_grid_attach (GTK_GRID (grid), spinbutton, 2, 0, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.material.diffuse_ref);

  ligma_help_set_help_data (spinbutton,
                           _("Higher values makes the object reflect more "
                             "light (appear lighter)"), NULL);

  image = gtk_image_new_from_icon_name (MAPOBJECT_REFLECTIVITY_DIFFUSE_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_grid_attach (GTK_GRID (grid), image, 3, 0, 1, 1);
  gtk_widget_show (image);

  /* Specular reflection */

  image = gtk_image_new_from_icon_name (MAPOBJECT_REFLECTIVITY_SPECULAR_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 1,
                                    _("Specular:"), 0.0, 0.5,
                                    image, 1);
  gtk_size_group_add_widget (group, label);

  spinbutton = spin_button_new (&adj, mapvals.material.specular_ref,
                                0, G_MAXFLOAT, 0.1, 1.0, 0.0, 0.0, 2);
  gtk_grid_attach (GTK_GRID (grid), spinbutton, 2, 1, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.material.specular_ref);

  ligma_help_set_help_data (spinbutton,
                           _("Controls how intense the highlights will be"),
                           NULL);

  image = gtk_image_new_from_icon_name (MAPOBJECT_REFLECTIVITY_SPECULAR_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_grid_attach (GTK_GRID (grid), image, 3, 1, 1, 1);
  gtk_widget_show (image);

  /* Highlight */

  image = gtk_image_new_from_icon_name (MAPOBJECT_REFLECTIVITY_HIGHLIGHT_LOW,
                                        GTK_ICON_SIZE_BUTTON);
  label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, 2,
                                    _("Highlight:"), 0.0, 0.5,
                                    image, 1);
  gtk_size_group_add_widget (group, label);

  spinbutton = spin_button_new (&adj, mapvals.material.highlight,
                                0, G_MAXFLOAT, 0.1, 1.0, 0.0, 0.0, 2);
  gtk_grid_attach (GTK_GRID (grid), spinbutton, 2, 2, 1, 1);
  gtk_widget_show (spinbutton);

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (double_adjustment_update),
                    &mapvals.material.highlight);

  ligma_help_set_help_data (spinbutton,
                           _("Higher values makes the highlights more focused"),
                           NULL);

  image = gtk_image_new_from_icon_name (MAPOBJECT_REFLECTIVITY_HIGHLIGHT_HIGH,
                                        GTK_ICON_SIZE_BUTTON);
  gtk_grid_attach (GTK_GRID (grid), image, 3, 2, 1, 1);
  gtk_widget_show (image);

  gtk_widget_show (page);

  g_object_unref (group);

  return page;
}

/****************************************/
/* Create orientation and position page */
/****************************************/

static GtkWidget *
create_orientation_page (void)
{
  GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GtkWidget    *page;
  GtkWidget    *frame;
  GtkWidget    *grid;
  GtkWidget    *scale;
  GtkWidget    *spinbutton;

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = ligma_frame_new (_("Position"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  scale = ligma_scale_entry_new (_("X:"), mapvals.position.x, -1.0, 2.0, 5);
  ligma_help_set_help_data (scale, _("Object X position in XYZ space"), NULL);
  spinbutton = ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale));
  gtk_size_group_add_widget (group, spinbutton);
  gtk_spin_button_configure (GTK_SPIN_BUTTON (spinbutton), NULL, 0.01, 5);

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.position.x);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 0, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Y:"), mapvals.position.y, -1.0, 2.0, 5);
  ligma_help_set_help_data (scale, _("Object Y position in XYZ space"), NULL);
  spinbutton = ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale));
  gtk_size_group_add_widget (group, spinbutton);
  gtk_spin_button_configure (GTK_SPIN_BUTTON (spinbutton), NULL, 0.01, 5);

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.position.y);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 1, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Z:"), mapvals.position.z, -1.0, 2.0, 5);
  ligma_help_set_help_data (scale, _("Object Z position in XYZ space"), NULL);
  spinbutton = ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale));
  gtk_size_group_add_widget (group, spinbutton);
  gtk_spin_button_configure (GTK_SPIN_BUTTON (spinbutton), NULL, 0.01, 5);

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.position.z);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 2, 3, 1);
  gtk_widget_show (scale);

  frame = ligma_frame_new (_("Rotation"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  scale = ligma_scale_entry_new (_("X:"), mapvals.alpha, -180.0, 180.0, 1);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 1.0, 15.0);
  ligma_help_set_help_data (scale, _("Rotation angle about X axis"), NULL);
  gtk_size_group_add_widget (group, ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale)));

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.alpha);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 0, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Y:"), mapvals.beta, -180.0, 180.0, 1);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 1.0, 15.0);
  ligma_help_set_help_data (scale, _("Rotation angle about Y axis"), NULL);
  gtk_size_group_add_widget (group, ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale)));

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.beta);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 1, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Z:"), mapvals.gamma, -180.0, 180.0, 1);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 1.0, 15.0);
  ligma_help_set_help_data (scale, _("Rotation angle about Z axis"), NULL);
  gtk_size_group_add_widget (group, ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale)));

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.gamma);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 2, 3, 1);
  gtk_widget_show (scale);

  gtk_widget_show (page);

  g_object_unref (group);

  return page;
}

static GtkWidget *
create_box_page (void)
{
  GtkWidget *page;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *grid;
  GtkWidget *scale;
  GtkWidget *spinbutton;
  gint       i;

  static gchar *labels[] =
  {
    N_("Front:"), N_("Back:"),
    N_("Top:"),   N_("Bottom:"),
    N_("Left:"),  N_("Right:")
  };

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = ligma_frame_new (_("Map Images to Box Faces"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_add (GTK_CONTAINER (frame), vbox);
  gtk_widget_show (vbox);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID(grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID(grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 5);
  gtk_widget_show (grid);

  for (i = 0; i < 6; i++)
    {
      GtkWidget *combo;

      combo = ligma_drawable_combo_box_new (box_constrain, NULL, NULL);
      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  mapvals.boxmap_id[i],
                                  G_CALLBACK (ligma_int_combo_box_get_active),
                                  &mapvals.boxmap_id[i], NULL);

      ligma_grid_attach_aligned (GTK_GRID (grid), 0, i,
                                gettext (labels[i]), 0.0, 0.5,
                                combo, 1);
    }

  /* Scale scales */

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID(grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID(grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  scale = ligma_scale_entry_new (_("Scale X:"), mapvals.scale.x, 0.0, 5.0, 2);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.01, 0.1);
  ligma_help_set_help_data (scale, _("X scale (size)"), NULL);
  spinbutton = ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale));
  gtk_spin_button_configure (GTK_SPIN_BUTTON (spinbutton), NULL, 0.1, 2);

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.scale.x);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 0, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Y:"), mapvals.scale.y, 0.0, 5.0, 2);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.01, 0.1);
  ligma_help_set_help_data (scale, _("Y scale (size)"), NULL);
  spinbutton = ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale));
  gtk_spin_button_configure (GTK_SPIN_BUTTON (spinbutton), NULL, 0.1, 2);

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.scale.y);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 1, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("Z:"), mapvals.scale.z, 0.0, 5.0, 2);
  ligma_label_spin_set_increments (LIGMA_LABEL_SPIN (scale), 0.01, 0.1);
  ligma_help_set_help_data (scale, _("Z scale (size)"), NULL);
  spinbutton = ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale));
  gtk_spin_button_configure (GTK_SPIN_BUTTON (spinbutton), NULL, 0.1, 2);

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.scale.z);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 2, 3, 1);
  gtk_widget_show (scale);

  gtk_widget_show (page);

  return page;
}

static GtkWidget *
create_cylinder_page (void)
{
  GtkSizeGroup *group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  GtkWidget    *page;
  GtkWidget    *frame;
  GtkWidget    *grid;
  GtkWidget    *scale;
  GtkWidget    *spinbutton;
  gint          i;

  static const gchar *labels[] = { N_("_Top:"), N_("_Bottom:") };

  page = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (page), 12);

  frame = ligma_frame_new (_("Images for the Cap Faces"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  /* Option menus */

  for (i = 0; i < 2; i++)
    {
      GtkWidget *combo;
      GtkWidget *label;

      combo = ligma_drawable_combo_box_new (cylinder_constrain, NULL, NULL);
      ligma_int_combo_box_connect (LIGMA_INT_COMBO_BOX (combo),
                                  mapvals.cylindermap_id[i],
                                  G_CALLBACK (ligma_int_combo_box_get_active),
                                  &mapvals.cylindermap_id[i], NULL);

      label = ligma_grid_attach_aligned (GTK_GRID (grid), 0, i,
                                        gettext (labels[i]), 0.0, 0.5,
                                        combo, 1);
      gtk_size_group_add_widget (group, label);
    }

  frame = ligma_frame_new (_("Size"));
  gtk_box_pack_start (GTK_BOX (page), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_container_add (GTK_CONTAINER (frame), grid);
  gtk_widget_show (grid);

  scale = ligma_scale_entry_new (_("R_adius:"), mapvals.cylinder_radius, 0.0, 2.0, 2);
  ligma_help_set_help_data (scale, _("Cylinder radius"), NULL);
  gtk_size_group_add_widget (group, ligma_labeled_get_label (LIGMA_LABELED (scale)));
  spinbutton = ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale));
  gtk_spin_button_configure (GTK_SPIN_BUTTON (spinbutton), NULL, 0.1, 2);

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.cylinder_radius);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 0, 3, 1);
  gtk_widget_show (scale);

  scale = ligma_scale_entry_new (_("L_ength:"), mapvals.cylinder_length, 0.0, 2.0, 2);
  ligma_help_set_help_data (scale, _("Cylinder length"), NULL);
  gtk_size_group_add_widget (group, ligma_labeled_get_label (LIGMA_LABELED (scale)));
  spinbutton = ligma_label_spin_get_spin_button (LIGMA_LABEL_SPIN (scale));
  gtk_spin_button_configure (GTK_SPIN_BUTTON (spinbutton), NULL, 0.1, 2);

  g_signal_connect (scale, "value-changed",
                    G_CALLBACK (scale_entry_update_double),
                    &mapvals.cylinder_length);
  gtk_grid_attach (GTK_GRID (grid), scale, 0, 1, 3, 1);
  gtk_widget_show (scale);

  gtk_widget_show (page);

  g_object_unref (group);

  return page;
}

/****************************/
/* Create notbook and pages */
/****************************/

static void
create_main_notebook (GtkWidget *container)
{
  GtkWidget *page;

  options_note_book = GTK_NOTEBOOK (gtk_notebook_new ());
  gtk_container_add (GTK_CONTAINER (container),
                     GTK_WIDGET (options_note_book));

  page = create_options_page ();
  gtk_notebook_append_page (options_note_book, page,
                            gtk_label_new_with_mnemonic (_("O_ptions")));

  page = create_light_page ();
  gtk_notebook_append_page (options_note_book, page,
                            gtk_label_new_with_mnemonic (_("_Light")));

  page = create_material_page ();
  gtk_notebook_append_page (options_note_book, page,
                            gtk_label_new_with_mnemonic (_("_Material")));

  page = create_orientation_page ();
  gtk_notebook_append_page (options_note_book, page,
                            gtk_label_new_with_mnemonic (_("O_rientation")));

  if (mapvals.maptype == MAP_BOX)
    {
      box_page = create_box_page ();
      g_object_ref (box_page);
      gtk_notebook_append_page (options_note_book, box_page,
                                gtk_label_new_with_mnemonic (_("_Box")));
    }
  else if (mapvals.maptype == MAP_CYLINDER)
    {
      cylinder_page = create_cylinder_page ();
      g_object_ref (cylinder_page);
      gtk_notebook_append_page (options_note_book, cylinder_page,
                                gtk_label_new_with_mnemonic (_("C_ylinder")));
    }

  gtk_widget_show (GTK_WIDGET (options_note_book));
}

/********************************/
/* Create and show main dialog. */
/********************************/

gboolean
main_dialog (LigmaDrawable *drawable)
{
  GtkWidget     *main_hbox;
  GtkWidget     *vbox;
  GtkWidget     *hbox;
  GtkWidget     *frame;
  GtkWidget     *button;
  GtkWidget     *toggle;
  LigmaZoomModel *model;
  gboolean       run = FALSE;

  ligma_ui_init (PLUG_IN_BINARY);

  appwin = ligma_dialog_new (_("Map to Object"), PLUG_IN_ROLE,
                            NULL, 0,
                            ligma_standard_help_func, PLUG_IN_PROC,

                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_OK"),     GTK_RESPONSE_OK,

                            NULL);

  ligma_dialog_set_alternative_button_order (GTK_DIALOG (appwin),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  ligma_window_set_transient (GTK_WINDOW (appwin));

  main_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (main_hbox), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (appwin))),
                      main_hbox, FALSE, FALSE, 0);
  gtk_widget_show (main_hbox);

  /* Create the Preview */

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (main_hbox), vbox, FALSE, FALSE, 0);
  gtk_widget_show (vbox);

  /* Add preview widget and various buttons to the first part */

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_realize (appwin);
  previewarea = gtk_drawing_area_new ();
  gtk_widget_set_size_request (previewarea, PREVIEW_WIDTH, PREVIEW_HEIGHT);
  gtk_widget_set_events (previewarea, (GDK_EXPOSURE_MASK |
                                       GDK_BUTTON1_MOTION_MASK |
                                       GDK_BUTTON_PRESS_MASK |
                                       GDK_BUTTON_RELEASE_MASK));
  gtk_container_add (GTK_CONTAINER (frame), previewarea);
  gtk_widget_show (previewarea);

  g_signal_connect (previewarea, "event",
                    G_CALLBACK (preview_events),
                    previewarea);

  g_signal_connect (previewarea, "draw",
                    G_CALLBACK (preview_draw),
                    previewarea);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_mnemonic (_("_Preview!"));
  g_object_set (gtk_bin_get_child (GTK_BIN (button)),
                "margin-start", 2,
                "margin-end",   2,
                NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (preview_callback),
                    NULL);

  ligma_help_set_help_data (button, _("Recompute preview image"), NULL);

  model = ligma_zoom_model_new ();
  ligma_zoom_model_set_range (model, 0.25, 1.0);
  ligma_zoom_model_zoom (model, LIGMA_ZOOM_TO, mapvals.zoom);

  button = ligma_zoom_button_new (model, LIGMA_ZOOM_IN, GTK_ICON_SIZE_MENU);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = ligma_zoom_button_new (model, LIGMA_ZOOM_OUT, GTK_ICON_SIZE_MENU);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  g_signal_connect (model, "zoomed",
                    G_CALLBACK (zoomed_callback),
                    NULL);

  toggle = gtk_check_button_new_with_mnemonic (_("Show _wireframe"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mapvals.showgrid);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.showgrid);

  toggle = gtk_check_button_new_with_mnemonic (_("Update preview _live"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle), mapvals.livepreview);
  gtk_box_pack_start (GTK_BOX (vbox), toggle, FALSE, FALSE, 0);
  gtk_widget_show (toggle);

  g_signal_connect (toggle, "toggled",
                    G_CALLBACK (toggle_update),
                    &mapvals.livepreview);

  create_main_notebook (main_hbox);

  gtk_widget_show (appwin);

  {
    GdkCursor *cursor;

    cursor = gdk_cursor_new_for_display (gtk_widget_get_display (previewarea),
                                         GDK_HAND2);
    gdk_window_set_cursor (gtk_widget_get_window (previewarea), cursor);
    g_object_unref (cursor);
  }

  image_setup (drawable, TRUE);

  compute_preview_image ();

  if (ligma_dialog_run (LIGMA_DIALOG (appwin)) == GTK_RESPONSE_OK)
    run = TRUE;

  gtk_widget_destroy (appwin);
  if (preview_rgb_data)
    g_free (preview_rgb_data);
  if (preview_surface)
    cairo_surface_destroy (preview_surface);
  if (box_page)
    g_object_unref (box_page);
  if (cylinder_page)
    g_object_unref (cylinder_page);

  return run;
}
