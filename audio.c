/*
 * audio.c
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
void session_clean(void);
void set_session_shift(int mode);


void
sound_playback (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8400);
}


void
audio_acceleration (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8A04); // TODO: needs restart
}


void
stereo_filtering_changed (GtkCheckMenuItem *self, gpointer data)
{
  if (!gtk_check_menu_item_get_active (self))
    return;

  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(self));
  switch (label[0])
  {
    case '0':
      set_session_shift (1);
      session_user (0x8401);
      break;
    case '2':
      set_session_shift (1);
      session_user (0x8404);
      break;
    case '5':
      set_session_shift (1);
      session_user (0x8403);
      break;
    case '1':
      set_session_shift (1);
      session_user (0x8402);
      break;

    case 'N':
      session_user (0x8401);
      break;
    case 'L':
      session_user (0x8402);
      break;
    case 'M':
      session_user (0x8403);
      break;
    case 'H':
     session_user (0x8404);
      break;
    default:
      return;
  }
  session_clean();
  set_session_shift (0);
}


void
record_audio_file (GtkCheckMenuItem* self, gpointer data)
{
  const gchar* label = gtk_menu_item_get_label (GTK_MENU_ITEM(self));

  if (label[7] == 'Y')
    set_session_shift (1);

  session_user (0x0C00); // TODO: save to a path specified by the user
  set_session_shift (0);
}
