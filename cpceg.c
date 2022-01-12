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
#include "resources.h"

#define NATIVE_RES_X 384
#define NATIVE_RES_Y 268
#define TARGET_RES_X (NATIVE_RES_X * 2)
#define TARGET_RES_Y (NATIVE_RES_Y * 2)


static GtkWidget *mainwindow;
static cairo_surface_t *surface = NULL;
static gboolean quit = FALSE;
static GtkWidget *sbar1;
static GtkWidget *sbar3;
static GtkWidget *snap_menu;
static GtkWidget *disca_menu;
static GtkWidget *discb_menu;
static GtkWidget *tape_menu;
static gchar* conf_path = NULL;

static unsigned char *kbd;

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
int bios_load(char *s);
void all_reset(void);

void session_user(int k);


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
model_changed (GtkCheckMenuItem* self, gpointer user_data)
{
  if (!gtk_check_menu_item_get_active(self))
    return;

  const gchar *label = gtk_menu_item_get_label((GtkMenuItem*) self);

  gchar* rom = NULL;
  if (g_strcmp0(label, "464") == 0)
    rom = "cpc464.rom";
  else if (g_strcmp0(label, "664") == 0)
    rom = "cpc664.rom";
  else if (g_strcmp0(label, "6128") == 0)
    rom = "cpc6128.rom";
  else if (g_strcmp0(label, "6128plus") == 0)
    rom = "cpcplus.rom";
  else if (g_strcmp0(label, "cpcados") == 0)
    rom = "cpcados.rom";

  if (rom != NULL)
  {
    gchar* rom_path = g_strconcat (conf_path, rom, NULL);
    bios_load (rom_path);
    all_reset();
  }
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
}


static gboolean
check_header (const gchar* content, const char* header)
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
check_menu (const gchar* filename)
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
      if (check_header (contents, snap_header))
        menu = ((GtkMenuItem*) snap_menu);
      else if ((check_header (contents, dsk_header)) || (check_header (contents, mvc_header)))
        menu = ((GtkMenuItem*) disca_menu);  //only on A?
      else if ( (check_header (contents, tzx_header)) || (check_header (contents, csw_header)) )
        menu = ((GtkMenuItem*) tape_menu);

      if (menu != NULL)
      {
        gtk_menu_item_set_label ((GtkMenuItem*) tape_menu, "Empty");
        gtk_menu_item_set_label ((GtkMenuItem*) disca_menu, "Empty");
        gtk_menu_item_set_label ((GtkMenuItem*) discb_menu, "Empty");

        gchar* basename = g_path_get_basename (filename);
        gtk_menu_item_set_label (menu, basename);
        g_free (basename);
      }
    }
  }
  g_free (contents);
}


static gchar*
dialog_run (GtkWidget* dialog)
{
  gchar* filename = NULL;
  session_user (0x0F00);

  if (gtk_dialog_run ((GtkDialog*) dialog) == ((gint) GTK_RESPONSE_ACCEPT))
    filename = gtk_file_chooser_get_filename ((GtkFileChooser*) dialog);

  gtk_widget_destroy (dialog);
  session_user (0x0F00);

  return filename;
}


static gchar*
dialog_save_file (const gchar* title, const gchar* current_name)
{
  gchar* filename = NULL;
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SAVE;

  GtkWidget *dialog = gtk_file_chooser_dialog_new (title,
                                                   (GtkWindow*) mainwindow,
                                                   action,
                                                   "_Save",
                                                   GTK_RESPONSE_ACCEPT,
                                                   "_Cancel",
                                                   GTK_RESPONSE_CANCEL,
                                                   NULL);


  gtk_file_chooser_set_do_overwrite_confirmation ((GtkFileChooser*) dialog, TRUE);
  gtk_file_chooser_set_current_name ((GtkFileChooser*) dialog, current_name);

  return dialog_run (dialog);
}


static gchar*
dialog_load_file (const gchar* title,
                  gchar** patterns,
                  const gchar* filter_name)
{
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;

  GtkWidget *dialog = gtk_file_chooser_dialog_new (title,
                                                   (GtkWindow*) mainwindow,
                                                   action,
                                                   "_Open",
                                                   GTK_RESPONSE_ACCEPT,
                                                   "_Cancel",
                                                   GTK_RESPONSE_CANCEL,
                                                   NULL);

  GtkFileFilter *filter = gtk_file_filter_new ();

  guint i = 0;
  while (patterns[i] != NULL)
  {
    gtk_file_filter_add_pattern (filter, patterns[i]);
    i++;
  }

  gtk_file_filter_set_name (filter, filter_name);
  gtk_file_chooser_add_filter ((GtkFileChooser*) dialog, filter);

  GtkFileFilter *filter2 = gtk_file_filter_new ();
  gtk_file_filter_add_pattern (filter2, "*");

  gtk_file_filter_set_name (filter2, "All files (*.*)");
  gtk_file_chooser_add_filter ((GtkFileChooser*) dialog, filter2);

  return dialog_run (dialog);
}


static int
get_disc_drive (gpointer item)
{
  const gchar *label = gtk_menu_item_get_label((GtkMenuItem*) item);

  return (label[5] == 'A') ? 0 : 1;
}


void
disca_insert (GtkWidget *object, gpointer data)
{
  gchar* patterns[] = {"*.dsk", NULL};
  gchar* filename = dialog_load_file ("Select a disc...", patterns, "Disc files (*.dsk)");

  if (filename != NULL)
  {
    int d = get_disc_drive (data);
    if (disc_open (filename, d, 0))
      printf ("Cannot open disc!\n");
    else
    {
      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((d == 0) ? (GtkMenuItem*) disca_menu : (GtkMenuItem*) discb_menu, basename);
      g_free (basename);
    }
    g_free (filename);
  }
}


void
disca_create (GtkWidget *object, gpointer data)
{
  gchar* filename = dialog_save_file ("New disc...", "Untitled.dsk");

  if (filename != NULL)
  {
    if (disc_create (filename))
      printf ("Cannot save disc!\n");
    else
    {
      int d = get_disc_drive (data);
      disc_open (filename, d, 1);

      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((d == 0) ? (GtkMenuItem*) disca_menu : (GtkMenuItem*) discb_menu, basename);
      g_free (basename);
    }
    g_free (filename);
  }
}


void
disca_flip (GtkCheckMenuItem* self, gpointer data)
{
  int d = get_disc_drive (data);
  disc_flip_sides (d);
}


void
disca_remove (GtkWidget *object, gpointer data)
{
  int d = get_disc_drive (data);
  disc_close (d);

  gtk_menu_item_set_label ((d == 0) ? (GtkMenuItem*) disca_menu : (GtkMenuItem*) discb_menu, "Empty");
}


void
tape_remove (GtkWidget *object, gpointer data)
{
  session_user (0x0800);
  gtk_menu_item_set_label ((GtkMenuItem*) tape_menu, "Empty");
}


void
tape_record_menu (GtkWidget *object, gpointer data)
{
  gchar* filename = dialog_save_file ("New tape...", "Untitled.csw");
  if (filename != NULL)
  {
    if (tape_create (filename))
      printf ("Cannot save tape!\n");
    else
    {
      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((GtkMenuItem*) tape_menu, basename);
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
      printf ("Cannot open tape!\n");
    else
    {
      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((GtkMenuItem*) tape_menu, basename);
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
      printf ("Error\n");
    else
    {
      gchar* basename = g_path_get_basename (filename);
      gtk_menu_item_set_label ((GtkMenuItem*) snap_menu, basename);
      g_free (basename);
    }

    g_free (filename);
  }
}


static void
load_any_file (gchar* filename)
{
  int e = 0;
  if (filename != NULL)
    e = any_load (filename, 1);

  if (e != 0)
    printf ("Error\n");
  else
    check_menu (filename);
}


void
load_any_menu (GtkWidget *object, gpointer data)
{
  gchar* patterns[] = {"*.sna", "*.dsk", "*.cpr", "*.rom", "*.cdt", "*.tzx", "*.csw", "*.wav", NULL};
  gchar* filename = dialog_load_file ("Select any file...", patterns, "CPC files (*.sna, *.dsk, *.cpr, *.rom, *.cdt, *.tzx, *.csw, *.wav)");
  if (filename != NULL)
  {
    load_any_file (filename);
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
    load_any_file (filename);
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
gtk_quit (GtkWidget *object, gpointer data)
{
  quit = TRUE;
}


void
show_about (GtkWidget *object, gpointer window)
{
  session_user (0x0F00);
  gtk_dialog_run (GTK_DIALOG (window));
  gtk_widget_hide (window);
  session_user (0x0F00);
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
  GtkWidget *blackbox;
  GtkWidget *drawing_area;
  GtkTargetEntry targetentries[] = {"text/uri-list", 0, 0};

  gtk_init (NULL, NULL);

  GtkBuilder* builder = gtk_builder_new();
  gtk_builder_add_from_file (builder, "./share/glade/cpceg.ui", NULL);

  mainwindow = GTK_WIDGET (gtk_builder_get_object(builder, "main_window"));
  blackbox = GTK_WIDGET (gtk_builder_get_object(builder, "black_box"));
  drawing_area = GTK_WIDGET (gtk_builder_get_object(builder, "drawing"));
  sbar1 = GTK_WIDGET (gtk_builder_get_object(builder, "sbar_1"));
  sbar3 = GTK_WIDGET (gtk_builder_get_object(builder, "sbar_3"));

  snap_menu = GTK_WIDGET (gtk_builder_get_object(builder, "snap_menu"));
  disca_menu = GTK_WIDGET (gtk_builder_get_object(builder, "disca_menu"));
  discb_menu = GTK_WIDGET (gtk_builder_get_object(builder, "discb_menu"));
  tape_menu = GTK_WIDGET (gtk_builder_get_object(builder, "tape_menu"));

  gtk_builder_connect_signals (builder, NULL);
  g_object_unref(builder);

  gtk_widget_set_size_request (drawing_area, NATIVE_RES_X, NATIVE_RES_Y);

  GtkCssProvider *css = gtk_css_provider_new();
  gtk_css_provider_load_from_data(css, "* {background-color:black;}", -1, NULL);
  GtkStyleContext *context = gtk_widget_get_style_context (blackbox);
  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_USER);
  g_object_unref (css);

  gtk_drag_dest_set (mainwindow, GTK_DEST_DEFAULT_DROP, targetentries, 1, GDK_ACTION_LINK);

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
