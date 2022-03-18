/*
 * file.c
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


#include "dialogs.h"


// exported
int any_load (char *s, int q);
int snap_load(char *s);
int snap_save(char *s);
int disc_open(char *s,int drive,int canwrite);
int disc_create(char *s);
void disc_flip_sides(int drive);
void disc_close(int drive);
int tape_open(char *s);
int tape_create(char *s);
void session_clean(void);
void session_user(int k);


static gboolean
match_header (const gchar* content, const char* header)
{
  guint i;
  for (i = 0; i < 8; i++)
  {
    if (header[i] != content[i])
      return FALSE;
  }

  return TRUE;
}


static void
set_menu_files_labels (const gchar* filename, gpointer builder)
{
  const char snap_header[8] = { 0x4D, 0x56, 0x20, 0x2D, 0x20, 0x53, 0x4E, 0x41 };
  const char dsk_header[8] = { 0x45, 0x58, 0x54, 0x45, 0x4E, 0x44, 0x45, 0x44 };
  const char mvc_header[8] = { 0x4D, 0x56, 0x20, 0x2D, 0x20, 0x43, 0x50, 0x43 };
  const char tzx_header[8] = { 0x5A, 0x58, 0x54, 0x61, 0x70, 0x65, 0x21, 0x1A };
  const char csw_header[8] = { 0x43, 0x6F, 0x6D, 0x70, 0x72, 0x65, 0x73, 0x73 }; // TODO: wav & cpr files

  gchar *contents;
  gsize length;
  if (g_file_get_contents (filename, &contents, &length, NULL))
  {
    if (length > 8)
    {
      GtkMenuItem* menu = NULL;
      if (match_header (contents, snap_header))
        menu = ((GtkMenuItem*) gtk_builder_get_object (builder, "snap"));
      else if ((match_header (contents, dsk_header)) || (match_header (contents, mvc_header)))
        menu = ((GtkMenuItem*) gtk_builder_get_object (builder, "disc"));  //only on A?
      else if ( (match_header (contents, tzx_header)) || (match_header (contents, csw_header)))
        menu = ((GtkMenuItem*) gtk_builder_get_object (builder, "tape_menu"));

      if (menu != NULL)
      {
        gtk_menu_item_set_label ((GtkMenuItem*) gtk_builder_get_object (builder, "tape_menu"), "Empty");
        gtk_menu_item_set_label ((GtkMenuItem*) gtk_builder_get_object (builder, "disc"), "Empty");
        gtk_menu_item_set_label ((GtkMenuItem*) gtk_builder_get_object (builder, "discb"), "Empty");

        gchar* basename = g_path_get_basename (filename);
        gtk_menu_item_set_label (menu, basename);
        g_free (basename);
      }
    }
  }
  g_free (contents);
}


static void
load_any_file (gchar* filename, gpointer data)
{
  int e = 0;
  if (filename != NULL)
    e = any_load (filename, 1);

  if (e != 0)
    show_error ("File not compatible!");
  else
  {
    set_menu_files_labels (filename, data);
    session_clean();
  }
}


void
open_any_file (GtkWidget *object, gpointer data)
{
  gchar* patterns[] = {"*.sna", "*.dsk", "*.cpr", "*.rom", "*.cdt", "*.tzx", "*.csw", "*.wav", NULL};
  gchar* filename = dialog_load_file ("Select any file...",
                                       patterns,
                                      "CPC files (*.sna, *.dsk, *.cpr, *.rom, *.cdt, *.tzx, *.csw, *.wav)");

  if (filename != NULL)
  {
    load_any_file (filename, data);
    g_free (filename);
  }
}


static int
get_disc_drive (gpointer item)
{
  const gchar *label = gtk_widget_get_name ((GtkWidget*) item);

  return (label[0] == 'A') ? 0 : 1;
}


void
disc_insert (GtkWidget *object, gpointer data)
{
  gchar* patterns[] = {"*.dsk", NULL};
  gchar* filename = dialog_load_file ("Select a disc...", patterns, "Disc files (*.dsk)");

  if (filename != NULL)
  {
    int d = get_disc_drive (data);
    if (disc_open (filename, d, 0))
      show_error ("Cannot open disc!");
    else
    {
      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((GtkMenuItem*) data, basename);
      g_free (basename);
    }
    g_free (filename);
  }
}


void
disc_create0 (GtkWidget *object, gpointer data)
{
  gchar* filename = dialog_save_file ("New disc...", "Untitled.dsk");

  if (filename != NULL)
  {
    if (disc_create (filename))
      show_error ("Cannot save disc!");
    else
    {
      int d = get_disc_drive (data);
      disc_open (filename, d, 1);

      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((GtkMenuItem*) data, basename);
      g_free (basename);
    }
    g_free (filename);
  }
}


void
disc_flip0 (GtkCheckMenuItem* self, gpointer data)
{
  int d = get_disc_drive (data);
  disc_flip_sides (d);
}


void
disc_remove (GtkWidget *object, gpointer data)
{
  int d = get_disc_drive (data);
  disc_close (d);

  gtk_menu_item_set_label ((GtkMenuItem*) data, "Empty");
}


void
tape_remove (GtkWidget *object, gpointer data)
{
  session_user (0x0800);
  gtk_menu_item_set_label ((GtkMenuItem*) gtk_builder_get_object (data, "tape_menu"), "Empty");
}


void
tape_record0 (GtkWidget *object, gpointer data)
{
  gchar* filename = dialog_save_file ("New tape...", "Untitled.csw");

  if (filename != NULL)
  {
    if (tape_create (filename))
      show_error ("Cannot save tape!");
    else
    {
      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((GtkMenuItem*) gtk_builder_get_object (data, "tape_menu"), basename);
      g_free (basename);
    }
    g_free (filename);
  }
}


void
tape_insert (GtkWidget *object, gpointer data)
{
  gchar* patterns[] = {"*.cdt", "*.tzx", "*.csw", "*.wav", NULL};
  gchar* filename = dialog_load_file ("Select a tape...", patterns, "Tape files (*.cdt, *.tzx, *.csw, *.wav)");

  if (filename != NULL)
  {
    if (tape_open (filename))
      show_error ("Cannot open tape!");
    else
    {
      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((GtkMenuItem*) gtk_builder_get_object (data, "tape_menu"), basename);
      g_free (basename);
    }
    g_free (filename);
  }
}


void
snap_last (GtkWidget *object, gpointer data)
{
  const gchar* mode = gtk_menu_item_get_label ((GtkMenuItem*) object);

  (mode[0] == 'S') ? session_user (0x0200) : session_user (0x0300);
}


void
snap_file (GtkWidget *object, gpointer data)
{
  gchar* filename;
  const gchar* mode = gtk_menu_item_get_label ((GtkMenuItem*) object);

  if (mode[0] == 'S')
  {
    filename = dialog_save_file ("Save snapshot...", "Untitled.sna");
  }
  else
  {
    gchar* patterns[] = {"*.sna", NULL};
    filename = dialog_load_file ("Load snapshot", patterns, "Snapshot files (*.sna)");
  }

  if (filename != NULL)
  {
    int e = 0;
    if (mode[0] == 'S')
      e = snap_save (filename);
    else
      e = snap_load (filename);

    if (e != 0)
      show_error ("Cannot load/save snap file!");
    else
    {
      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((GtkMenuItem*) gtk_builder_get_object (data, "snap"), basename);
      g_free (basename);
    }

    g_free (filename);
  }
}


void
drag_data (GtkWidget *self,
           GdkDragContext *context,
           gint x,
           gint y,
           GtkSelectionData *seldata,
           guint info,
           guint time,
           gpointer data)
{
  gchar **uris = g_uri_list_extract_uris (gtk_selection_data_get_data (seldata));

  if (uris == NULL)
  {
    gtk_drag_finish (context, FALSE, FALSE, time);
    return;
  }

  gchar* filename = g_filename_from_uri (uris[0], NULL, NULL);

  if (filename != NULL)
    load_any_file (filename, data);
  else
  {
    g_strfreev (uris);
    gtk_drag_finish (context, FALSE, FALSE, time);
    return;
  }

  g_free (filename);
  g_strfreev (uris);

  gtk_drag_finish (context, TRUE, FALSE, time);
}


void
select_firmware (GtkWidget *object, gpointer data)
{
  gchar* patterns[] = {"*.cpr", "*.rom", NULL};
  gchar* filename = dialog_load_file ("Load firmware", patterns, "CPC roms (*.cpr, *.rom)");

  if (filename != NULL)
  {
    load_any_file (filename, data);
    g_free (filename);
  }
}

