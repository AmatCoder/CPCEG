/*
 * video.c
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


#include <gtk/gtk.h>

static gboolean isrecording = FALSE;

// exported
void session_user(int k);
void set_session_shift(int mode);


void
onscreen_status (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8901);
}


void
pixel_filtering (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8904);
}


void
x_masking (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8903);
}


void
y_masking (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8902);
}


void
palette_changed (GtkCheckMenuItem* self, gpointer data)
{
  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(self));

  switch (label[0])
  {
    case 'M':
      session_user (0x8B01);
      break;
    case 'D':
      session_user (0x8B02);
      break;
    case 'B':
      session_user (0x8B04);
      break;
    case 'G':
      session_user (0x8B05);
      break;
    default:
      session_user (0x8B03);
  }
}


void
scanlines_changed (GtkCheckMenuItem* self, gpointer data)
{
  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(self));

  switch (label[0])
  {
    case 'H':
      session_user (0x0B02);
      break;
    case 'S':
      session_user (0x0B03);//?
      break;
    case 'D':
      session_user (0x0B04);//?
      break;
    default:
      session_user (0x0B01);
  }
}


void
scanlines_blend (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x0B08);//?
}


void
frameskip_changed (GtkMenuItem* self, gpointer data)
{
  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(self));

  switch (label[0])
  {
    case 'R':
      session_user (0x9100);
      break;
    case 'L':
      session_user (0x9200);
      break;
    case 'F':
      session_user (0x9300);
      break;
    default:
      session_user (0x9400);
  }
}


void
save_bmp (GtkMenuItem* self, gpointer data)
{
  session_user (0x8C00); // TODO: save to a path specified by the user
}


void
record_video_file (GtkCheckMenuItem* self, gpointer data)
{
  isrecording = !isrecording;
  set_session_shift (1);
  session_user (0x8C00); // TODO: save to a path specified by the user and convert to AVI
  set_session_shift (0);

  gtk_widget_set_sensitive (GTK_WIDGET(gtk_builder_get_object(data, "film1")), !isrecording);
  gtk_widget_set_sensitive (GTK_WIDGET(gtk_builder_get_object(data, "film2")), !isrecording);
}


void
high_resolution (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8C01);
}


void
high_framerate (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8C02);
}

