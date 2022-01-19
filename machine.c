/*
 * machine.c
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
#include "dialogs.h"

static gboolean both_audios = FALSE;

// exported
int dandanator_load(char *s);
void dandanator_remove(void);
void session_user(int k);


void
reset_emulation (GtkWidget *object, gpointer data)
{
  session_user (0x0500);
}


void
pause_machine (GtkCheckMenuItem* self, gpointer data)
{
  if (gtk_check_menu_item_get_active (self))
  {
    session_user (0x8F00);
    gtk_set_info (NULL, "PAUSED");
  }
  else
    session_user (0x0F00);
}

void
vhold_toggled (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x8508);
}


void
machine_audio_changed (GtkCheckMenuItem* self, gpointer data)
{
  if (both_audios)
  {
    both_audios = FALSE;
    return;
  }

  const gchar *label = gtk_menu_item_get_label((GtkMenuItem*) self);
  session_user (label[0] == 'P' ? 0x8518 : 0x8519);

  if ( (gtk_check_menu_item_get_active (self)) && (gtk_check_menu_item_get_active ((GtkCheckMenuItem*) data)) )
  {
    both_audios = TRUE;
    gtk_check_menu_item_set_active ((GtkCheckMenuItem*) data, FALSE);
  }
}


void
dandanator_insert0 (GtkWidget *object, gpointer data)
{
  gchar* patterns[] = {"*.rom", "*.mld", NULL};
  gchar* filename = dialog_load_file ("Insert Dandanator card", patterns, "Dandanator cards (*.rom, *.mld)");

  if (filename != NULL)
  {
    if (dandanator_load (filename))
      show_error ("Cannot open Dandanator card!");
    else
      session_user (0x0500);

    g_free (filename);
  }
}


void
dandanator_remove0 (GtkWidget *object, gpointer data)
{
  dandanator_remove();
  session_user (0x0500);
}


void
dandanator_writeable (GtkCheckMenuItem* self, gpointer data)
{
  session_user (0x0510);
}
