/* event-menu.c - Event menu implementation.
   Copyright (C) 2006 Neal H. Walfield <neal@walfield.org>

   This file is part of GPE.

   GPE is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GPE is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */

#include <gpe/question.h>
#include <gpe/pixmaps.h>
#include "event-menu.h"
#include "globals.h"
#include "export-vcal.h"
#include "event-ui.h"
#include "calendars-widgets.h"
#include "calendar-edit-dialog.h"
#include "calendar-delete-dialog.h"

static void
edit_event_cb (GtkWidget *widget, gpointer d)
{
  GtkWidget *w = edit_event (EVENT (d));
  gtk_widget_show (w);
}

static void
edit_calendar_cb (GtkWidget *widget, gpointer d)
{
  GtkWidget *w = calendar_edit_dialog_new (EVENT_CALENDAR (d));
  gtk_window_set_transient_for (GTK_WINDOW (w), GTK_WINDOW (main_window));
  g_signal_connect (G_OBJECT (w), "response",
		    G_CALLBACK (gtk_widget_destroy), NULL);
  gtk_widget_show (w);
}

static void
delete_event_cb (GtkWidget *widget, gpointer d)
{
  Event *ev = EVENT (d);

  gboolean removed = FALSE;
  char *summary = event_get_summary (ev);

  if (event_is_recurrence (ev))
    {
      char *s = g_strdup_printf (_("Delete all occurrences of %s?"), summary);
      g_free (summary);
      switch (gpe_question_ask (s,
				_("Question"), "question",
				_("Delete this occurrence"), "!gtk-delete",
				_("Delete all occurrences"), "!gtk-delete",
				"!gtk-cancel", NULL,
				NULL))
	{
	case 0:
	  event_add_recurrence_exception (ev, event_get_start (ev));
	  event_flush (ev);
	  removed = TRUE;
	  break;
	case 1:
	  event_remove (ev);
	  removed = TRUE;
	  break;
	default:
	  break;
	}
      g_free (s);
    }
  else
    {
      char *s = g_strdup_printf (_("Delete %s?"), summary);
      g_free (summary);
      if (gpe_question_ask (s, _("Question"), "question",
			    "!gtk-delete", NULL, "!gtk-cancel", NULL, NULL)
	  == 0)
	{
	  event_remove (ev);
	  removed = TRUE;
	}
      g_free (s);
    }

  if (removed)
    update_view ();
}

static void
delete_calendar_cb (GtkWidget *widget, gpointer d)
{
  EventCalendar *ec = EVENT_CALENDAR (d);
  GtkWidget *w = calendar_delete_dialog_new (ec);
  gtk_window_set_transient_for
    (GTK_WINDOW (w),
     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (widget))));
  gtk_widget_show (w);
}

static void
save_cb (GtkWidget *widget, gpointer d)
{
  export_event_save_as_dialog (EVENT (d));
}

static void
save_calendar_cb (GtkWidget *widget, gpointer d)
{
  export_calendar_save_as_dialog (EVENT_CALENDAR (d));
}

static void
only_show_cb (GtkWidget *widget, EventCalendar *ec)
{
  GSList *l = event_db_list_event_calendars (event_db);
  GSList *i;
  for (i = l; i; i = i->next)
    {
      if (event_calendar_get_uid (i->data) != event_calendar_get_uid (ec))
	event_calendar_set_visible (i->data, FALSE);
      g_object_unref (i->data);
    }
  g_slist_free (l);

  EventCalendar *p = event_calendar_get_parent (ec);
  while (p)
    {
      event_calendar_set_visible (p, TRUE);
      g_object_unref (p);
    }
}

static void
hide_cb (GtkWidget *widget, gpointer d)
{
  event_calendar_set_visible (EVENT_CALENDAR (d), FALSE);
}

static void
send_ir_cb (GtkWidget *widget, gpointer d)
{
  vcal_do_send_irda (EVENT (d));
}

static void
send_bt_cb (GtkWidget *widget, gpointer d)
{
  vcal_do_send_bluetooth (EVENT (d));
}

static void
move_event_to (EventCalendar *ec, gpointer user_data)
{
  Event *ev = EVENT (user_data);
  event_set_calendar (ev, ec);
  update_view ();
}

struct info
{
  Event *ev;
  EventCalendar *ec;
};

static void
event_menu_destroy (GtkWidget *widget, gpointer d)
{
  struct info *info = d;
  g_object_unref (info->ev);
  g_object_unref (info->ec);
  g_free (info);
  gtk_widget_destroy (widget);
}

GtkMenu *
event_menu_new (Event *ev, gboolean show_summary)
{
  struct info *info = g_malloc (sizeof (*info));

  info->ev = ev;
  g_object_ref (ev);

  EventCalendar *ec = event_get_calendar (ev);
  info->ec = ec;
  char *title = event_calendar_get_title (ec);
  char *s;

  GtkMenu *menu = GTK_MENU (gtk_menu_new ());
  GtkWidget *item;
  int i = 0;

  /* The event title.  */
  if (show_summary)
    {
      struct tm start_tm, end_tm;

      time_t start = event_get_start (ev);
      localtime_r (&start, &start_tm);
      char *strstart = strftime_strdup_utf8_locale (TIMEFMT, &start_tm);

      time_t end = start + event_get_duration (ev);
      gchar *strend;
      if (end == start)
	strend = 0;
      else
	{
	  localtime_r (&end, &end_tm);
	  strend = strftime_strdup_utf8_locale (TIMEFMT, &end_tm);
	}

      char buffer[64];
      char *summary = event_get_summary (ev);
      char *description = event_get_description (ev);
      int l = snprintf (buffer, 64, "%s %s%s%s %s%s%s",
			summary,
			strstart, strend ? "-" : "", strend ?: "",
			event_get_alarm (ev) ? "(A)" : "",
			description ? "\n" : "",
			description ? description : "");
      g_free (summary);
      g_free (description);
      g_free(strstart);
      g_free(strend);
      buffer[64] = 0;
      if (l > sizeof (buffer))
	l = sizeof (buffer);
      l --;
      while (buffer[l] == '\n')
	buffer[l --] = 0;
    
      char *tbuffer = g_locale_to_utf8 (buffer, -1, NULL, NULL, NULL);
      if (tbuffer || buffer)
	{
	  GtkWidget *event_menu_info
	    = gtk_menu_item_new_with_label (tbuffer ?: buffer);
	  gtk_widget_show (event_menu_info);
	  gtk_menu_attach (menu, event_menu_info, 0, 1, i, i + 1);
	  i ++;
	}

      if (tbuffer)
	g_free (tbuffer);
    }

  /* Create an edit button.  */
  GtkWidget *edit;

#ifdef IS_HILDON
  edit = gtk_menu_item_new_with_label (_("Edit"));
#else
  edit = gtk_image_menu_item_new_from_stock (GTK_STOCK_EDIT, NULL);
#endif
  gtk_widget_show (edit);
  gtk_menu_attach (menu, edit, 0, 1, i, i + 1);
  i ++;

  GtkMenu *sub = GTK_MENU (gtk_menu_new ());
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (edit), GTK_WIDGET (sub));
  gtk_widget_show (GTK_WIDGET (sub));
  
  int j = 0;
  item = gtk_menu_item_new_with_label (_("Edit Event"));
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (edit_event_cb), ev);
  gtk_widget_show (item);
  gtk_menu_attach (sub, item, 0, 1, j, j + 1);
  j ++;

  item = gtk_menu_item_new_with_label ("");
  s = g_strdup_printf (_("Edit <i>%s</i>"), title);
  gtk_label_set_markup (GTK_LABEL (GTK_BIN (item)->child), s);
  g_free (s);
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (edit_calendar_cb), ec);
  gtk_widget_show (item);
  gtk_menu_attach (sub, item, 0, 1, j, j + 1);
  j ++;

  /* And a save button.  */
#ifdef IS_HILDON
  item = gtk_menu_item_new_with_label (_("Delete"));
#else
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
#endif
  gtk_widget_show (item);
  gtk_menu_attach (menu, item, 0, 1, i, i + 1);
  i ++;

  sub = GTK_MENU (gtk_menu_new ());
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), GTK_WIDGET (sub));
  gtk_widget_show (GTK_WIDGET (sub));
  
  j = 0;
  item = gtk_menu_item_new_with_label (_("Delete Event"));
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (delete_event_cb), ev);
  gtk_widget_show (item);
  gtk_menu_attach (sub, item, 0, 1, j, j + 1);
  j ++;

  item = gtk_menu_item_new_with_label ("");
  s = g_strdup_printf (_("Delete <i>%s</i>"), title);
  gtk_label_set_markup (GTK_LABEL (GTK_BIN (item)->child), s);
  g_free (s);
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (delete_calendar_cb), ec);
  gtk_widget_show (item);
  gtk_menu_attach (sub, item, 0, 1, j, j + 1);
  j ++;

  /* And a save button.  */
#ifdef IS_HILDON
  item = gtk_menu_item_new_with_label (_("Save"));
#else
  item = gtk_image_menu_item_new_from_stock (GTK_STOCK_SAVE, NULL);
#endif
  gtk_widget_show (item);
  gtk_menu_attach (menu, item, 0, 1, i, i + 1);
  i ++;

  sub = GTK_MENU (gtk_menu_new ());
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), GTK_WIDGET (sub));
  gtk_widget_show (GTK_WIDGET (sub));
  
  j = 0;
  item = gtk_menu_item_new_with_label (_("Save Event"));
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (save_cb), ev);
  gtk_widget_show (item);
  gtk_menu_attach (sub, item, 0, 1, j, j + 1);
  j ++;

  item = gtk_menu_item_new_with_label ("");
  s = g_strdup_printf (_("Save <i>%s</i>"), title);
  gtk_label_set_markup (GTK_LABEL (GTK_BIN (item)->child), s);
  g_free (s);
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (save_calendar_cb), ec);
  gtk_widget_show (item);
  gtk_menu_attach (sub, item, 0, 1, j, j + 1);
  j ++;

  /* Create a Send via infra-red button if infra-red is available.  */
  if (export_irda_available ())
    {
      GtkWidget *send_ir_button;

      send_ir_button = gtk_menu_item_new_with_label (_("Send via Infra-red"));
      g_signal_connect (G_OBJECT (send_ir_button), "activate",
			G_CALLBACK (send_ir_cb), ev);
      gtk_widget_show (send_ir_button);
      gtk_menu_attach (menu, send_ir_button, 0, 1, i, i + 1);
      i ++;
    }

  /* Create a Send via Bluetooth button if bluetooth is available.  */
  if (export_bluetooth_available ())
    {
      GtkWidget *send_bt_button;

      send_bt_button = gtk_menu_item_new_with_label (_("Send via Bluetooth"));
      g_signal_connect (G_OBJECT (send_bt_button), "activate",
			G_CALLBACK (send_bt_cb), ev);
      gtk_widget_show (send_bt_button);
      gtk_menu_attach (menu, send_bt_button, 0, 1, i, i + 1);
      i ++;
    }

  /* And a "Move to..." sub menu.  */
  GtkWidget *calendars = calendars_menu (move_event_to, ev);
  if (calendars)
    {
      gtk_widget_show (calendars);

      GtkWidget *item
	= gtk_menu_item_new_with_label (_("Move to Calendar..."));
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), calendars);
      gtk_menu_attach (menu, item, 0, 1, i, i + 1);
      gtk_widget_show (item);
      i ++;
    }

  /* Only show this calendar.  */
  item = gtk_menu_item_new_with_label ("");
  s = g_strdup_printf (_("Only Show <i>%s</i>"), title);
  gtk_label_set_markup (GTK_LABEL (GTK_BIN (item)->child), s);
  g_free (s);
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (only_show_cb), ec);
  gtk_widget_show (item);
  gtk_menu_attach (menu, item, 0, 1, i, i + 1);
  i ++;

  /* Hide calendar.  */
  item = gtk_menu_item_new_with_label ("");
  s = g_strdup_printf (_("Hide <i>%s</i>"), title);
  gtk_label_set_markup (GTK_LABEL (GTK_BIN (item)->child), s);
  g_free (s);
  g_signal_connect (G_OBJECT (item), "activate",
		    G_CALLBACK (hide_cb), ec);
  gtk_widget_show (item);
  gtk_menu_attach (menu, item, 0, 1, i, i + 1);
  i ++;

  g_signal_connect (G_OBJECT (menu), "selection-done",
		    G_CALLBACK (event_menu_destroy), info);

  g_free (title);
 
  return menu;
}
