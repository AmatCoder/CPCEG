/*
 * settings.c
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

// exported
void session_user(int k);
void set_session_shift(int mode);
 

void
speed_changed (GtkWidget *object, gpointer data)
{
  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(object));

  switch (label[1])
  {
    case '1':
      session_user (0x8601);
      break;
    case '2':
      session_user (0x8602);
      break;
    case '3':
      session_user (0x8603);
      break;
    case '4':
      session_user (0x8604);
      break;
    default:
      session_user (0x8600);
  }
}


void
lightgun_changed (GtkCheckMenuItem* self, gpointer data)
{
  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(self));
  gboolean lightgun_enabled = TRUE;

  switch (label[0])
  {
    case 'T':
      session_user (0x8522);
      break;
    case 'G':
      session_user (0x8523);
      break;
    case 'W':
      session_user (0x8524);
      break;
    default:
      session_user (0x8521);
      lightgun_enabled = FALSE;
  }
}


void
strict_sna (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x851F);
}


void
strict_disc (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8590);
}


void
readonly_disc (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8591);
}


void
tape_fs_load (GtkCheckMenuItem* self, gpointer data)
{
  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(self));

  if (label[5] == 'a')
    set_session_shift (1);

  session_user (0x0900);
  set_session_shift (0);
}


void
tape_autorewind (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x0901);
}
