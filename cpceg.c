/*
 * cpceg.c
 *
 * Copyright 2022 AmatCoder
 *
 * This file is part of CPCEG.
 *
 * CPCEG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * CPCEG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CPCEG; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

#include "cpceg.h"
#include "resources.h"

#define NATIVE_RES_X 384
#define NATIVE_RES_Y 268
#define TARGET_RES_X (NATIVE_RES_X * 2)
#define TARGET_RES_Y (NATIVE_RES_Y * 2)

static GtkBuilder* builder;
static GtkWidget *mainwindow;
static cairo_surface_t *surface = NULL;
static GtkWidget *sbar1;
static GtkWidget *sbar3;
static gchar* conf_path = NULL;
static gchar* info_t = NULL;
static gchar* perf_t = NULL;
static gboolean quit = FALSE;
static gboolean disconnected = FALSE;
static gboolean plus_enabled = FALSE;
static gboolean vjoy_enabled = FALSE;
static gint flip_joy = 0;

static unsigned char *kbd;

// exported
int bios_load(char *s);
void session_clean(void);
void session_user(int k);

void set_motion(int x, int y);
void set_button(int z);


static const unsigned char kbd_map_gdk[]=
{
  //TODO: 0x16 (and mirror), 0x09 (copy) and 0x10,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0x42, 0x40, 0x41, 0x39, 0x38, 0x31, 0x30,
  0x29, 0x28, 0x21, 0x20, 0x19, 0x18, 0x4F, 0x44,
  0x43, 0x3B, 0x3A, 0x32, 0x33, 0x2B, 0x2A, 0x23,
  0x22, 0x1B, 0x1A, 0x11, 0x12, 0x17, 0x45, 0x3C,
  0x3D, 0x35, 0x34, 0x2C, 0x2D, 0x25, 0x24, 0x1D,
  0x1C, 0, 0x15, 0x13, 0x47, 0x3F, 0x3E, 0x37,
  0x36, 0x2E, 0x26, 0x27, 0x1F, 0x1E, 0x15, 0,

  0, 0x2F, 0x46, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0x0A,
  0x0B, 0x03, 0, 0x14, 0x0C, 0x04, 0, 0x0D,
  0x0E, 0x05, 0x0F, 0x07, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0x06, 0x17, 0, 0, 0, 0, 0, 0x00,
  0, 0x08, 0x01, 0, 0x02, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0
};

static unsigned char kbd_j[]=
{ 0x6F, 0x48,
  0x74, 0x49,
  0x71, 0x4A,
  0x72, 0x4B,
  0x34, 0x4C,
  0x35, 0x4D,
  0x36, 0x4C,
  0x37, 0x4D
};


static int
key_to_joy (int k)
{
  if (vjoy_enabled)
  {
    int i;
    for (i = 0; i < 12; i += 2)
      if (kbd_j[i]==k)
        return kbd_j[i+1];
  }

  return kbd_map_gdk[k];
}


gboolean
key_event (GtkWidget *widget,
           GdkEvent  *event,
           gpointer   data)
{
  if (event->key.hardware_keycode > 127)
    return TRUE;

  guint k = key_to_joy (event->key.hardware_keycode);

  if (event->key.type == GDK_KEY_PRESS)
    kbd[k/8]|=1<<(k%8);
  else
    kbd[k/8]&=~(1<<(k%8));

  //printf("%s: %i - %x\n", (event->key.type == 8) ? "Pressed" : "Released", event->key.hardware_keycode, k);

  return FALSE;
}


void
gtk_set_kbd (unsigned char* kbd_bit)
{
  kbd = kbd_bit;
}


void
virtual_joystick (GtkCheckMenuItem* self, gpointer data)
{
  //session_user (0x0400);
  vjoy_enabled = gtk_check_menu_item_get_active (self);
}


void
flip_joystick (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x0401);
  session_clean();

  flip_joy = gtk_check_menu_item_get_active (self) ? 1 : 0;

	kbd_j[9] = kbd_j[13] = (0x4C + flip_joy);
	kbd_j[11] = kbd_j[15] = (0x4D - flip_joy);
}


gboolean
draw_cb (GtkWidget *widget,
         cairo_t   *cr,
         gpointer   data)
{
  if (surface != NULL)
  {
    guint width = gtk_widget_get_allocated_width (widget);
    guint height = gtk_widget_get_allocated_height (widget);

    double sx = (double)width / TARGET_RES_X;
    double sy = (double)height / TARGET_RES_Y;

    cairo_scale (cr, sx, sy);
    cairo_set_source_surface (cr, surface, 0, 0);
    cairo_paint (cr);

    gtk_widget_queue_draw (widget);
  }

  return FALSE;
}


void
gtk_update_cairo_surface (unsigned char* frame,
                          int offset_x,
                          int offset_y,
                          int stride)
{
  if (surface == NULL)
  {
    surface = cairo_image_surface_create_for_data (frame,
                                                   CAIRO_FORMAT_ARGB32,
                                                   (offset_x + TARGET_RES_X),
                                                   (offset_y + TARGET_RES_Y),
                                                   stride);

    cairo_surface_set_device_offset (surface, offset_x, offset_y);
    gtk_widget_queue_draw (mainwindow);
  }
  else
  {
    cairo_surface_flush (surface);
    unsigned char *data = cairo_image_surface_get_data (surface);
    data = frame;
    cairo_surface_mark_dirty (surface);
  }
}


gboolean
mouse_moved (GtkWidget* widget, GdkEventMotion* event)
{
  GtkAllocation allocation;
  gtk_widget_get_allocation (widget, &allocation);

  //printf("%i - %i - %i - %i\n", (int)allocation.width/TARGET_RES_X, (int)allocation.height/TARGET_RES_Y, (int)event->x, (int)event->y);
  //printf("%f - %f\n", (event->x/((double)allocation.width/(TARGET_RES_X))), (event->y/((double)allocation.height/(TARGET_RES_Y))));

  set_motion( (int)(event->x/((double)allocation.width/(TARGET_RES_X))), (int)(event->y/((double)allocation.height/(TARGET_RES_Y))) );

  return TRUE;
}


gboolean
mouse_button_event (GtkWidget* widget, GdkEventButton* event)
{
  set_button (event->type == GDK_BUTTON_PRESS);
  return TRUE;
}


void
show_about (GtkWidget *object, gpointer window)
{
  session_user (0x0F00);

  gtk_about_dialog_set_logo ((GtkAboutDialog*) window, NULL);
  gtk_dialog_run (GTK_DIALOG (window));
  gtk_widget_hide (window);

  session_user (0x8F00);
}


static void
rc_set_misc (const gchar* setting, const gchar* value)
{
  if (g_strcmp0 (setting, "misc") == 0)
  {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "misc1")), ((*value&1) == 1));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "misc2")), !!(*value&2) == 0);
  }
  else if (g_strcmp0 (setting, "fdcw") == 0)
  {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "fdcw1")), !!(*value&2) == 0);
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "fdcw2")), ((*value&1) == 1));
  }
  else if (g_strcmp0 (setting, "casette") == 0)
  {
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "casette1")), (!!(*value&2) == 1));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "casette2")), (!!(*value&4) == 1));
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(gtk_builder_get_object(builder, "casette3")), ((*value&1) == 1));
  }
}


static void
rc_set_check (const gchar* setting)
{
  GObject* obj = gtk_builder_get_object (builder, setting);

  if (GTK_IS_CHECK_MENU_ITEM (obj))
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(obj), TRUE);
}


static void
get_rc_settings (void)
{
  char* keys[] = {"snap", NULL};

  gchar* value = NULL;
  gchar* rc = g_strconcat (conf_path, G_DIR_SEPARATOR_S, ".cpcecrc", NULL);
  if (g_file_test (rc, G_FILE_TEST_EXISTS))
  {
    gchar *contents;
    gsize length;
    if (g_file_get_contents ( rc, &contents, &length, NULL))
    {
      gchar** lines = g_strsplit (contents, "\n", -1);

      int i = 0;
      while (lines[i] != NULL)
      {
        rc_set_check (lines[i]);
        gchar** values = g_strsplit (lines[i], " ", -1);
        if (values[1] != NULL)
        {
          rc_set_misc (values[0], values[1]);

          int j = 0;
          while (keys[j] != NULL)
          {
            if (g_strcmp0 (keys[j], values[0]) == 0)
            {
              //printf("%s - *%s*\n",keys[j], values[1]);
              GObject* obj = gtk_builder_get_object(builder, keys[j]);

              if (GTK_IS_MENU_ITEM (obj))
              {
                gchar* basename = g_path_get_basename (values[1]);
                gtk_menu_item_set_label (GTK_MENU_ITEM(obj), basename);
                g_free (basename);
              }
            }
            j++;
          }
        }
        i++;
        g_strfreev (values);
      }
      g_strfreev (lines);
    }
    g_free (contents);
  }
  g_free (rc);
}


void
model_changed (GtkCheckMenuItem* self, gpointer user_data)
{
  if (disconnected)
    return;

  const gchar *label = gtk_menu_item_get_label((GtkMenuItem*) self);

  gchar* rom = NULL;
  if (g_strcmp0(label, "464") == 0)
    rom = "cpc464.rom";
  else if (g_strcmp0(label, "664") == 0)
    rom = "cpc664.rom";
  else if (g_strcmp0(label, "6128") == 0)
    rom = "cpc6128.rom";
  else if (g_strcmp0(label, "Plus") == 0)
    rom = "cpcplus.rom";

  if (rom != NULL)
  {
    gchar* rom_path = g_strconcat (conf_path, rom, NULL);
    bios_load (rom_path);
    session_user (0x0500);
    session_clean();
  }
}


void
clock_changed (GtkWidget *object, gpointer data)
{
  if (disconnected)
    return;

  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(object));

  switch (label[1])
  {
    case '1':
      session_user (0x0601);
      break;
    case '2':
      session_user (0x0602);
      break;
    case '3':
      session_user (0x0603);
      break;
    default:
      session_user (0x0604);
  }

  session_clean();
}


void
ram_changed (GtkWidget *object, gpointer data)
{
  if (disconnected)
    return;

  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(object));
  switch (label[2])
  {
    case 'k':
      session_user (0x8511);
      break;
    case '8':
      session_user (0x8512);
      break;
    case '2':
      session_user (0x8513);
      break;
    case '0':
      session_user (0x8514);
      break;
    default:
      session_user (0x8515);
  }

  session_clean();
}


static void
crtc_sensitive (void)
{
  GObject* obj = gtk_builder_get_object(builder, "crtc 3");
  gtk_widget_set_sensitive (GTK_WIDGET (obj), plus_enabled);

  obj = gtk_builder_get_object(builder, "crtc 0");
  gtk_widget_set_sensitive (GTK_WIDGET (obj), !plus_enabled);

  obj = gtk_builder_get_object(builder, "crtc 1");
  gtk_widget_set_sensitive (GTK_WIDGET (obj), !plus_enabled);

  obj = gtk_builder_get_object(builder, "crtc 2");
  gtk_widget_set_sensitive (GTK_WIDGET (obj), !plus_enabled);

  obj = gtk_builder_get_object(builder, "crtc 4");
  gtk_widget_set_sensitive (GTK_WIDGET (obj), !plus_enabled);
}


void
crtc_changed (GtkWidget *object, gpointer data)
{
  if (disconnected)
    return;

  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(object));

  switch (label[0])
  {
    case '0':
      session_user (0x8501);
      break;
    case '1':
      session_user (0x8502);
      break;
    case '2':
      session_user (0x8503);
      break;
    case '3':
      //session_user (0x8504);
      break;
    default:
      session_user (0x8505);
  }

  session_clean();
}


static void
set_menu_check (const gchar* id)
{
  GObject *menu =  gtk_builder_get_object(builder, id);

  if (menu)
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(menu), TRUE);
}


void gtk_set_info (const char* info, const char* perf)
{
  if (g_strcmp0 (perf, perf_t) != 0)
  {
    g_free (perf_t);
    perf_t = g_strdup (perf);
    gtk_label_set_text ((GtkLabel*)sbar3, perf_t);
  }

  if ((info != NULL) && (g_strcmp0 (info, info_t) != 0))
  {
    disconnected = TRUE;

    g_free (info_t);
    info_t = g_strdup (info);
    gtk_label_set_text ((GtkLabel*)sbar1, info_t);

    gchar** fields = g_strsplit_set (info_t,": ", 5);

    gchar* type;
    char b = fields[0][3];
    switch (b)
    {
      case '4' :
        type = "type 0";
        break;
      case '6' :
        if (fields[0][4] == '6')
          type = "type 1";
        else
          type = "type 2";
        break;
      default :
        type = "type 3";
     }

    set_menu_check (type);

    gchar* manufacturer;
    char c = fields[3][4];
    switch (c)
    {
      case '0' :
        manufacturer = "Hitachi";
        break;
      case '1' :
        manufacturer = "UMC";
        break;
      case '2' :
        manufacturer = "Motorola";
        break;
      case '3' :
        manufacturer = "Amstrad ASIC";
        break;
      default :
        manufacturer = "Amstrad";
     }

    gchar* crtc = g_strdup_printf ("crtc %c", c);
    set_menu_check (crtc);
    g_free (crtc);

    gchar* bank;
    char d = fields[2][2];
    switch (d)
    {
      case 'K' :
        bank = "bank 0";
        break;
      case '8' :
        bank = "bank 1";
        break;
      case '2' :
        bank = "bank 2";
        break;
      case '0' :
        bank = "bank 3";
        break;
      default :
        bank = "bank 4";
     }

    set_menu_check (bank);

    gchar* tooltip = g_strdup_printf ("Model: %s\n\
Actually used RAM space: %s\n\
RAM configuration: %s\n\
CRTC type: %c - %s\n\
Clock speed: %s", fields[0], fields[1], fields[2], c, manufacturer, fields[4]);

    plus_enabled = (fields[0][3] == 'P');
    crtc_sensitive();

    g_strfreev (fields);

    gtk_widget_set_tooltip_text (sbar1, tooltip);
    g_free (tooltip);

    disconnected = FALSE;
  }
}

void
gtk_quit (GtkWidget *object, gpointer data)
{
  quit = TRUE;
}


int
gtk_loop (void)
{
  while (gtk_events_pending())
    gtk_main_iteration();

  if (quit)
    return 1;

  return 0;
}


void
gtk_create_window_new (void)
{
  GtkWidget *blackbox;
  GtkWidget *drawing_area;
  GtkTargetEntry targetentries[] = {"text/uri-list", 0, 0};

  gtk_init (NULL, NULL);

  builder = gtk_builder_new();
  gtk_builder_add_from_file (builder, "./share/glade/cpceg.ui", NULL);

  mainwindow = GTK_WIDGET (gtk_builder_get_object(builder, "main_window"));
  blackbox = GTK_WIDGET (gtk_builder_get_object(builder, "black_box"));
  drawing_area = GTK_WIDGET (gtk_builder_get_object(builder, "drawing"));
  sbar1 = GTK_WIDGET (gtk_builder_get_object(builder, "sbar_1"));
  sbar3 = GTK_WIDGET (gtk_builder_get_object(builder, "sbar_3"));

  gtk_widget_set_size_request (drawing_area, NATIVE_RES_X, NATIVE_RES_Y);

  gtk_window_set_default_icon (gdk_pixbuf_new_from_resource ("/com/github/AmatCoder/CPCEG/cpcec.png", NULL));

  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_data(css, "* {background-color:black;}", -1, NULL);
  GtkStyleContext *context = gtk_widget_get_style_context (blackbox);
  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref (css);

  gtk_drag_dest_set (mainwindow, GTK_DEST_DEFAULT_DROP, targetentries, 1, GDK_ACTION_LINK);

  gtk_widget_show_all (mainwindow);

  gtk_loop();
}


void
gtk_session_init (char* session_path)
{
  conf_path = g_strconcat (g_get_user_config_dir(), G_DIR_SEPARATOR_S, "cpceg", G_DIR_SEPARATOR_S, NULL);

  if (!g_file_test (conf_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
  {
    if (g_mkdir_with_parents (conf_path, 0700) == -1)
    {
      printf("Error!\n");
      return;
    }
  }

  gchar* models[] = {"cpc464.rom", "cpc664.rom", "cpc6128.rom", "cpcplus.rom", "cpcados.rom", NULL};
  int i = 0;
  while (models[i] != NULL)
  {
    gchar* rom = g_strconcat (conf_path, models[i], NULL);
    if (!g_file_test (rom, G_FILE_TEST_EXISTS))
    {
      GResource* r = resources_get_resource();
      gchar* romres = g_strconcat ("/com/github/AmatCoder/CPCEG/", models[i], NULL);
      GInputStream* is = g_resource_open_stream (r, romres, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
      gsize size;
      g_resource_get_info (r, romres, G_RESOURCE_LOOKUP_FLAGS_NONE, &size, NULL, NULL);
      void* buffer = g_malloc (size);
      g_input_stream_read (is, buffer, size, NULL, NULL);
      g_file_set_contents (rom, (const gchar*) buffer, size, NULL);

      g_free (buffer);
      g_object_unref (is);
      g_free (romres);
    }
    i++;
    g_free (rom);
  }

  g_strlcpy (session_path, conf_path, PATH_MAX);

  gtk_create_window_new();
  get_rc_settings();
  gtk_builder_connect_signals (builder, builder);
  //g_object_unref(builder);
}
