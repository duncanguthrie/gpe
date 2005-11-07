/*
 * Copyright (C) 2004 Philip Blundell <philb@gnu.org>
 *               2005 Florian Boor <florian@kernelconcepts.de> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <fcntl.h>

#include "export-vcal.h"

#include <gpe/vevent.h>
#include <gpe/errorbox.h>
#include <mimedir/mimedir.h>

#ifdef USE_DBUS
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>

static DBusConnection *connection;

#define BLUETOOTH_SERVICE_NAME   "org.handhelds.gpe.bluez"
#define IRDA_SERVICE_NAME   "org.handhelds.gpe.irda"
#endif /* USE_DBUS */

#define _(x) gettext(x)

static gchar *
export_to_vcal (event_t event)
{
  MIMEDirVEvent *vevent;
  gchar *str;
    
  vevent = vevent_from_event_t (event);

  str = mimedir_vevent_write_to_string (vevent);
  g_object_unref (vevent);

  return str;
}

void
vcal_do_send_bluetooth (event_t event)
{
#ifdef USE_DBUS
  gchar *vcal;
  DBusMessage *message;
  DBusMessageIter iter;

  vcal = export_to_vcal (event);

  message = dbus_message_new_method_call (BLUETOOTH_SERVICE_NAME,
					  "/org/handhelds/gpe/bluez/OBEX",
					  BLUETOOTH_SERVICE_NAME ".OBEX",
					  "ObjectPush");

  dbus_message_append_iter_init (message, &iter);

  dbus_message_iter_append_string (&iter, "GPE.vcf");
  dbus_message_iter_append_string (&iter, "application/x-vcal");
  dbus_message_iter_append_byte_array (&iter, vcal, strlen (vcal));

  dbus_connection_send (connection, message, NULL);

  g_free (vcal);
#endif /* USE_DBUS */
}

void
vcal_do_send_irda (event_t event)
{
#ifdef USE_DBUS
  gchar *vcal;
  DBusMessage *message;
  DBusMessageIter iter;

  vcal = export_to_vcal (event);

  message = dbus_message_new_method_call (IRDA_SERVICE_NAME,
					  "/org/handhelds/gpe/irda/OBEX",
					  IRDA_SERVICE_NAME ".OBEX",
					  "ObjectPush");

  dbus_message_append_iter_init (message, &iter);

  dbus_message_iter_append_string (&iter, "GPE.vcf");
  dbus_message_iter_append_string (&iter, "application/x-vcal");
  dbus_message_iter_append_byte_array (&iter, vcal, strlen (vcal));

  dbus_connection_send (connection, message, NULL);

  g_free (vcal);
#endif /* USE_DBUS */
}

gboolean
save_to_file(event_t event, const gchar *filename)
{
  gchar *vcal;
  int fd;
  
  vcal = export_to_vcal (event);

  fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0)
    goto error;

  if (write (fd, vcal, strlen (vcal)) < strlen (vcal))
    goto error;
  
  if (close (fd))
    goto error;

  g_free (vcal);
  return FALSE;
  
 error:
  g_free (vcal);
  return TRUE;
}


static void
select_file_done (GtkWidget *w, GtkWidget *filesel)
{
  event_t event;
  const gchar *filename;

  event = (event_t)g_object_get_data (G_OBJECT (filesel), "event");

  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));
  
  if (save_to_file(event, filename))
    goto error;
  
  gtk_widget_destroy (filesel);
  return;

 error:
  gpe_perror_box (filename);
  gtk_widget_destroy (filesel);
}

void
vcal_do_save (event_t event)
{
  GtkWidget *filesel;

  filesel = gtk_file_selection_new (_("Save as..."));

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), "GPE.vcal");

  g_signal_connect_swapped (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button), 
			    "clicked", G_CALLBACK (gtk_widget_destroy), filesel);
  
  g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button), 
		    "clicked", G_CALLBACK (select_file_done), filesel);

  g_object_set_data (G_OBJECT (filesel), "event", (gpointer)event);
  
  gtk_widget_show_all (filesel);
}

gboolean
export_bluetooth_available (void)
{
#ifdef USE_DBUS
  dbus_bool_t r;

  if (connection == NULL)
    return FALSE;

  r = dbus_bus_service_exists (connection, BLUETOOTH_SERVICE_NAME, NULL);

  return r ? TRUE : FALSE;
#else
  return FALSE;
#endif /* USE_DBUS */
}

gboolean
export_irda_available (void)
{
#ifdef USE_DBUS
  dbus_bool_t r;

  if (connection == NULL)
    return FALSE;

  r = dbus_bus_service_exists (connection, IRDA_SERVICE_NAME, NULL);

  return r ? TRUE : FALSE;
#else
  return FALSE;
#endif /* USE_DBUS */
}

void
vcal_export_init (void)
{
#ifdef USE_DBUS
  DBusError error;

  dbus_error_init (&error);

  connection = dbus_bus_get (DBUS_BUS_SESSION, &error);
  if (connection)
    dbus_connection_setup_with_g_main (connection, NULL);
#endif /* USE_DBUS */
}
