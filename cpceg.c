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

static cairo_surface_t *surface = NULL;
static gboolean quit = FALSE;

int any_load (char *s, int q); // exported


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

  if (e)
    printf ("Error\n");
}


void
gtk_quit (GtkWidget *object, gpointer data)
{
  quit = TRUE;
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

  gtk_builder_connect_signals (builder, NULL);
  g_object_unref(builder);

  gtk_widget_set_size_request (drawing_area, NATIVE_RES_X, NATIVE_RES_Y);

  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_data(css, "* {background-color:black;}", -1, NULL);
  GtkStyleContext *context = gtk_widget_get_style_context (blackbox);
  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref (css);

  gtk_widget_show_all (mainwindow);

  while (gtk_events_pending ())
    gtk_main_iteration ();
}


int
gtk_loop (void)
{
  while (gtk_events_pending ())
    gtk_main_iteration ();

  if (quit)
    return 1;

  return 0;
}
