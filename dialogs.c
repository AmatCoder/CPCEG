/*
 * dialogs.c
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


void session_user(int k);


void show_error (const gchar* message)
{
   GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
   GtkWidget* dialog = gtk_message_dialog_new (NULL, //(GtkWindow*) mainwindow,
                                               GTK_DIALOG_DESTROY_WITH_PARENT,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_CLOSE,
                                               "Error: %s",
                                               message);

   gtk_dialog_run (GTK_DIALOG (dialog));
   gtk_widget_destroy (dialog);
}


static gchar*
dialog_run (GtkWidget* dialog)
{
  gchar* filename = NULL;
  session_user (0x0F00);

  if (gtk_dialog_run ((GtkDialog*) dialog) == ((gint) GTK_RESPONSE_ACCEPT))
    filename = gtk_file_chooser_get_filename ((GtkFileChooser*) dialog);

  gtk_widget_destroy (dialog);
  session_user (0x8F00);

  return filename;
}


gchar*
dialog_save_file (const gchar* title, const gchar* current_name)
{
  gchar* filename = NULL;

  GtkWidget *dialog = gtk_file_chooser_dialog_new (title,
                                                   NULL, //(GtkWindow*) mainwindow, 
                                                   GTK_FILE_CHOOSER_ACTION_SAVE,
                                                   "_Save",
                                                   GTK_RESPONSE_ACCEPT,
                                                   "_Cancel",
                                                   GTK_RESPONSE_CANCEL,
                                                   NULL);


  gtk_file_chooser_set_do_overwrite_confirmation ((GtkFileChooser*) dialog, TRUE);
  gtk_file_chooser_set_current_name ((GtkFileChooser*) dialog, current_name);

  return dialog_run (dialog);
}


gchar*
dialog_load_file (const gchar* title,
                  gchar** patterns,
                  const gchar* filter_name)
{
  GtkWidget *dialog = gtk_file_chooser_dialog_new (title,
                                                   NULL, //(GtkWindow*) mainwindow,
                                                   GTK_FILE_CHOOSER_ACTION_OPEN,
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
