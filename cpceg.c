/*
 * cpceg.c
 *
 * Copyright 2021 AmatCoder
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

#define NATIVE_RES_X 384
#define NATIVE_RES_Y 268
#define TARGET_RES_X (NATIVE_RES_X * 2)
#define TARGET_RES_Y (NATIVE_RES_Y * 2)

static unsigned char *kbd;

static cairo_surface_t *surface = NULL;
static gboolean quit = FALSE;
static GtkWidget *sbar1;
static GtkWidget *sbar3;
static gchar* last_snap = NULL;

// exported
int any_load (char *s, int q);
int snap_load(char *s);
int snap_save(char *s);


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


gboolean
key_event (GtkWidget *widget,
           GdkEvent  *event,
           gpointer   data)
{
  if (event->key.hardware_keycode > 127)
    return TRUE;

  guint k = kbd_map_gdk[event->key.hardware_keycode];

  if (event->key.type == GDK_KEY_PRESS)
    kbd[k/8]|=1<<(k%8);
  else
    kbd[k/8]&=~(1<<(k%8));

  //printf("%s: %i - %x\n", (event->key.type == 8) ? "Pressed" : "Released", event->key.hardware_keycode, k);

  return FALSE;
}


void
gtk_set_kbd (unsigned char* kdb_bit)
{
  kbd = kdb_bit;
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
  }
  else
  {
    cairo_surface_flush (surface);
    unsigned char *data = cairo_image_surface_get_data (surface);
    data = frame;
    cairo_surface_mark_dirty (surface);
  }
}


void
snap_last (GtkWidget *object, gpointer parent)
{
  const gchar* mode = gtk_menu_item_get_label ((GtkMenuItem*) object);

  if (last_snap != NULL)
  {
    if (mode[0] == 'S')
      snap_save (last_snap);
    else
      snap_load (last_snap);
  }
}


void
snap_file (GtkWidget *object, gpointer parent)
{
  GtkWidget *dialog;
  gchar* title;
  gchar* button;
  GtkFileChooserAction action;

  const gchar* mode = gtk_menu_item_get_label ((GtkMenuItem*) object);
  if (mode[0] == 'S')
  {
    action = GTK_FILE_CHOOSER_ACTION_SAVE;
    title = "Save...";
    button = "_Save";
  }
  else
  {
    action = GTK_FILE_CHOOSER_ACTION_OPEN;
    title = "Open...";
    button = "_Open";
  }

  dialog = gtk_file_chooser_dialog_new (title,
                                        (GtkWindow*) parent,
                                        action,
                                        button,
                                        GTK_RESPONSE_ACCEPT,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        NULL);

  gtk_file_chooser_set_do_overwrite_confirmation ((GtkFileChooser*)dialog, TRUE);

  if (gtk_dialog_run ((GtkDialog*) dialog) == ((gint) GTK_RESPONSE_ACCEPT))
  {
    gchar* filename = gtk_file_chooser_get_filename ((GtkFileChooser*) dialog);

    if (filename != NULL)
    {
      int e = 0;
      if (mode[0] == 'S')
        e = snap_save (filename);
      else
        e = snap_load (filename);

      if (e == 0)
      {
        g_free (last_snap);
        last_snap = g_strdup (filename);
      }
      else printf ("Error\n");

      g_free (filename);
    }
  }

  gtk_widget_destroy (dialog);
}



void
load_any_file (GtkWidget *object, gpointer parent)
{
  gchar* filename = NULL;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;


  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new ("Select...",
                                        (GtkWindow*) parent,
                                        action,
                                        "_Open",
                                        GTK_RESPONSE_ACCEPT,
                                        "_Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        NULL);


  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_filter_set_name (filter,"All files");

  gtk_file_chooser_add_filter ((GtkFileChooser*)dialog, filter);

  if (gtk_dialog_run ((GtkDialog*) dialog) == ((gint) GTK_RESPONSE_ACCEPT))
    filename = gtk_file_chooser_get_filename ((GtkFileChooser*) dialog);

  gtk_widget_destroy (dialog);

  int e = 0;
  if (filename)
    e = any_load (filename, 1);

  if (e != 0)
    printf ("Error\n");
}


void
gtk_quit (GtkWidget *object, gpointer data)
{
  quit = TRUE;
}


void
show_about (GtkWidget *object, gpointer window)
{
  gtk_dialog_run (GTK_DIALOG (window));
  gtk_widget_hide (window);
}


void gtk_set_info3 (const char* perf)
{
  gtk_label_set_text ((GtkLabel*)sbar3, perf);
}


void gtk_set_info1 (const char* info)
{
  gtk_label_set_text ((GtkLabel*)sbar1, info);

  gchar** fields = g_strsplit_set (info,": ", 4);

  gchar* manufacturer;
  char c = fields[2][4];
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

  gchar* tooltip = g_strdup_printf ("Actually used RAM space: %sK\n\
RAM configuration: %s\n\
CRTC type: %c - %s\n\
Clock speed: %s", fields[0], fields[1], c, manufacturer, fields[3]);

  g_strfreev (fields);

  gtk_widget_set_tooltip_text (sbar1, tooltip);

  g_free (tooltip);
}


void
gtk_create_window_new (void)
{
  gtk_init (NULL, NULL);

  GtkWidget *mainwindow;
  GtkWidget *blackbox;
  GtkWidget *drawing_area;

  GtkBuilder* builder = gtk_builder_new();
  gtk_builder_add_from_file (builder, "./glade/cpceg.ui", NULL);

  mainwindow = GTK_WIDGET (gtk_builder_get_object(builder, "main_window"));
  blackbox = GTK_WIDGET (gtk_builder_get_object(builder, "black_box"));
  drawing_area = GTK_WIDGET (gtk_builder_get_object(builder, "drawing"));
  sbar1 = GTK_WIDGET (gtk_builder_get_object(builder, "sbar_1"));
  sbar3 = GTK_WIDGET (gtk_builder_get_object(builder, "sbar_3"));

  gtk_builder_connect_signals (builder, NULL);
  g_object_unref(builder);

  gtk_widget_set_size_request (drawing_area, NATIVE_RES_X, NATIVE_RES_Y);

  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_data(css, "* {background-color:black;}", -1, NULL);
  GtkStyleContext *context = gtk_widget_get_style_context (blackbox);
  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref (css);

  gtk_widget_show_all (mainwindow);

  while (gtk_events_pending())
  {
    gtk_main_iteration();
    gtk_widget_queue_draw (drawing_area);
  }
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
