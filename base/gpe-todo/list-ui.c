/*
 * Copyright (C) 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <libintl.h>

#include <gtk/gtk.h>

#include <gpe/pixmaps.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>

#include "todo.h"

#define _(_x) gettext(_x)

static GdkPixbuf *tick_image;

static guint ystep;
static guint xcol = 18;

static GtkWidget *g_draw;

static GtkWidget *g_option;
static GSList *display_items;
static struct todo_category *selected_category;

static void
set_category (GtkWidget *w, gpointer user_data)
{
  selected_category = user_data;
  refresh_items ();
}

void
categories_menu (void)
{
  GtkWidget *menu = gtk_menu_new ();
  GSList *l;
  GtkWidget *i;

  i = gtk_menu_item_new_with_label (_("All items"));
  gtk_menu_append (GTK_MENU (menu), i);
  gtk_signal_connect (GTK_OBJECT (i), "activate", (GtkSignalFunc)set_category, NULL);
  gtk_widget_show (i);

  for (l = categories; l; l = l->next)
    {
      struct todo_category *t = l->data;
      i = gtk_menu_item_new_with_label (t->title);
      gtk_menu_append (GTK_MENU (menu), i);
      gtk_signal_connect (GTK_OBJECT (i), "activate", (GtkSignalFunc)set_category, t);
      gtk_widget_show (i);
    }

  gtk_widget_show (menu);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (g_option), menu);
}

static void
new_todo_item (GtkWidget *w, gpointer user_data)
{
  GtkWidget *todo = edit_item (NULL, selected_category);
  gtk_widget_show_all (todo);
}

static void
purge_completed (GtkWidget *w, gpointer list)
{
  if (gpe_question_ask (_("Permanently delete all completed items?"), _("Confirm"), 
			"question", _("Delete"), "ok", _("Cancel"), "cancel", NULL) == 0)
    {
      GSList *iter = items;

      while (iter)
	{
	  struct todo_item *i = iter->data;
	  GSList *new_iter = iter->next;
	  if (i->state == COMPLETED)
	    delete_item (i);
	  
	  iter = new_iter;
	}
    }
  
  refresh_items ();
}

static void
show_hide_completed (GtkWidget *w, gpointer list)
{
  GSList *iter = items;

  for (iter = items; iter; iter = iter->next)
    {
      struct todo_item *i = iter->data;
      i->was_complete =  (i->state == COMPLETED) ? TRUE : FALSE;
    }
    
  refresh_items ();
}

#if GTK_MAJOR_VERSION >= 2
PangoLayout *
item_layout (struct todo_item *i)
{
  if (i->layout == NULL)
    i->layout = gtk_widget_create_pango_layout (g_draw, i->summary);

  return i->layout;
}
#endif

static void
draw_item (GdkDrawable *drawable, GtkWidget *widget, guint xcol, guint y, struct todo_item *i, guint skew, GdkEventExpose *event)
{
  GdkGC *black_gc = widget->style->black_gc;
#if GTK_MAJOR_VERSION < 2
  GdkFont *font = widget->style->font;
#endif
  guint width, height;

  if (i->summary)
    {
#if GTK_MAJOR_VERSION < 2
      gdk_draw_text (drawable, font, black_gc, xcol, y + font->ascent, 
		     i->summary, strlen (i->summary));
      width = gdk_string_width (font, i->summary);
      height = font->ascent + font->descent;
#else
      PangoLayout *l = item_layout (i);
      
      gtk_paint_layout (widget->style,
			widget->window,
			GTK_WIDGET_STATE (widget),
			FALSE,
			&event->area,
			widget,
			"label",
			xcol, y,
			l);
      
      pango_layout_get_size (l, &width, &height);
      width /= PANGO_SCALE;
      height /= PANGO_SCALE;
#endif
    }
  else
    width = 0;
  
  gdk_draw_rectangle (drawable, widget->style->black_gc, FALSE, 2, y + skew, 12, 12);
  if (i->state == COMPLETED)
    {
      gdk_draw_line (drawable, black_gc, xcol, 
		     y + height / 2, 
		     18 + width, 
		     y + height / 2);

      gdk_pixbuf_render_to_drawable_alpha (tick_image, drawable, 
					   0, 0, 3, y + skew + 1, 10, 10, GDK_PIXBUF_ALPHA_BILEVEL, 128,
					   GDK_RGB_DITHER_NONE, 0, 0);
    }
}

static gint
draw_expose_event (GtkWidget *widget,
		   GdkEventExpose  *event,
		   gpointer   user_data)
{
  GtkDrawingArea *darea;
  GdkDrawable *drawable;
  GdkGC *white_gc;
  guint max_width;
  guint max_height;
  guint y;
  guint skew = 2;
  GSList *iter;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  darea = GTK_DRAWING_AREA (widget);
  drawable = widget->window;
  white_gc = widget->style->white_gc;
  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  ystep = 14;

  if (! tick_image)
    tick_image = gpe_find_icon ("tick");
  
  gdk_draw_rectangle (drawable, white_gc, TRUE, 0, 0, max_width, max_height);

  y = skew;

  for (iter = display_items; iter; iter = iter->next)
    {
      struct todo_item *i = iter->data;

      if (! i->was_complete)
	{
	  draw_item (drawable, widget, xcol, y, i, skew, event);
	  
	  i->pos=y/ystep;
	  y += ystep;
	}
    }

  for (iter = display_items; iter; iter = iter->next)
    {
      struct todo_item *i = iter->data;

      if (i->was_complete)
	{
	  draw_item (drawable, widget, xcol, y, i, skew, event);
	  
	  i->pos=y/ystep;
	  y += ystep;
	}
    }


  return TRUE;
}

static void
draw_click_event (GtkWidget *widget,
		  GdkEventButton *event,
		  gpointer user_data)
{
  unsigned int idx = event->y/ystep;

  if (event->type == GDK_BUTTON_PRESS && event->x < xcol)
    {
      GSList *iter;
      for (iter = display_items; iter; iter = iter->next)
	{
	  struct todo_item *ti = iter->data;
	  if (idx == ti->pos)
	    {
	      if (ti->state == COMPLETED)
		ti->state = NOT_STARTED;
	      else
		ti->state = COMPLETED;
	      
	      push_item (ti);
	      gtk_widget_draw (g_draw, NULL);
	      break;
	    }
	}
    }
  else if (event->type == GDK_BUTTON_PRESS && event->x > 18)
    {
      GSList *iter;
      for (iter = display_items; iter; iter = iter->next)
	{
	  struct todo_item *ti = iter->data;
	  if (idx == ti->pos)
	    {
	      gtk_widget_show_all (edit_item (ti, NULL));
	      break;
	    }
	}
    }
}

void
refresh_items (void)
{
  GSList *iter;
  guint nitems = 0;

  if (display_items)
    {
      g_slist_free (display_items);
      display_items = NULL;
    }

  for (iter = items; iter; iter = iter->next)
    {
      struct todo_item *i = iter->data;
      if (selected_category == NULL 
	  || g_slist_find (i->categories, selected_category))
	{
	  display_items = g_slist_append (display_items, i);
	  nitems++;
	}
    }

  gtk_widget_set_usize (g_draw, -1, 14 * nitems);
  gtk_widget_draw (g_draw, NULL);
}

GtkWidget *
top_level (GtkWidget *window)
{
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
  GtkWidget *sep = gtk_vseparator_new ();
  GtkWidget *toolbar, *toolbar2;
  GtkWidget *option = gtk_option_menu_new ();
  GtkWidget *pw;
  GtkWidget *draw = gtk_drawing_area_new();
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);

#if GTK_MAJOR_VERSION < 2  
  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
					GTK_TOOLBAR_ICONS);
  toolbar2 = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
					GTK_TOOLBAR_ICONS);
#else
  toolbar = gtk_toolbar_new ();
  toolbar2 = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar2), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_ICONS);
#endif

  g_option = option;
  categories_menu ();

#if GTK_MAJOR_VERSION < 2
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2), GTK_RELIEF_NONE);
#endif

  pw = gpe_render_icon (window->style, gpe_find_icon ("new"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("New"), 
			   _("Add a new item"), 
			   _("Add a new item"),
			   pw, (GtkSignalFunc)new_todo_item, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("properties"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Configure"), 
			   _("Configure categories"), 
			   _("Configure categories"),
			   pw, (GtkSignalFunc)configure, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("clean"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Purge"), 
			   _("Purge completed items"), 
			   _("Purge completed items"),
			   pw, (GtkSignalFunc)purge_completed, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("hide"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), 
			   _("Re-sort"), 
			   _("Move completed items to the end of the list"), 
			   _("Move completed items to the end of the list"),
			   pw, (GtkSignalFunc)show_hide_completed, NULL);

  pw = gpe_render_icon (window->style, gpe_find_icon ("exit"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), 
			   _("Exit"), 
			   _("Exit the program"), 
			   _("Exit the program"),
			   pw, (GtkSignalFunc)gtk_exit, NULL);

  gtk_widget_show (toolbar);
  gtk_widget_show (toolbar2);
  gtk_widget_show (sep);
  gtk_widget_show (option);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), sep, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), option, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar2, FALSE, FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  
  gtk_signal_connect (GTK_OBJECT (draw), "expose_event",
		      GTK_SIGNAL_FUNC (draw_expose_event),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (draw), "button_press_event",
		      GTK_SIGNAL_FUNC (draw_click_event), NULL);

  gtk_widget_add_events (GTK_WIDGET (draw), GDK_BUTTON_PRESS_MASK);

  gtk_widget_show (draw);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
					 draw);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
				  GTK_POLICY_NEVER,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 0);

  gtk_widget_show (vbox);

  g_draw = draw;

  refresh_items ();

  return vbox;
}
