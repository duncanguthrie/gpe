/*
 * Copyright (C) 2001, 2002 Damien Tanner <dctanner@magenet.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <time.h>
#include <gtk/gtk.h>
#include <libintl.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

#include <gpe/init.h>
#include <gpe/errorbox.h>
#include <gpe/render.h>
#include <gpe/pixmaps.h>
#include <gpe/picturebutton.h>
#include <gpe/question.h>

#include "mime-sql.h"

#define _(x) dgettext(PACKAGE, x)

#define COMPLETED 0
#define LAST_SIGNAL 1
#define SPINNER_TIMEOUT 3

GtkWidget *window;
GtkWidget *spinner;
GtkWidget *view_scrolld;
GtkWidget *view_widget;

GdkPixbuf *default_pixbuf;

guint screen_w, screen_h;

GtkWidget *current_button=NULL;
int current_button_is_down=0;

/* For not starting an app twice after a double click */
int ignore_press = 0;

int loading_directory = 0;

int history_place = 0;
GList *history = NULL;

gchar *current_directory = "";
gchar *current_view = "icons";

int current_spinner_num = 0;
int num_spinner_files = 30;
gchar *spinner_file[] = {"001", "002", "003", "004", "005", "006", "007", "008", "009", "010", "011", "012", "013", "014", "015", "015", "016", "017", "018", "019", "020", "021", "022", "023", "024", "025", "026", "027", "028", "029", "030"};
GdkPixbuf *spinner_pixbuf[30], *spinner_rest_pixbuf;
GtkStyle *spinner_style;

struct gpe_icon my_icons[] = {
  { "left", "left" },
  { "right", "right" },
  { "up", "up" },
  { "refresh", "refresh" },
  { "stop", "stop" },
  { "home", "home" },
  { "dir-up", "dir-up" },
  { "ok", "ok" },
  { "cancel", "cancel" },
  { "question", "question" },
  { "error", "error" },
  { "icon", PREFIX "/share/pixmaps/gpe-filemanager.png" },
  {NULL, NULL}
};

guint window_x = 240, window_y = 310;

static void set_directory (gchar *new_directory);
static void update_spinner (void);
static void set_spinner_rest (void);

static void
safety_check (void)
{
  if (loading_directory == 1)
  {
    loading_directory = 0;
    sleep (0.2);
  }
}

/* Limit the title to two lines at <68px wide
   Eeeevil and Uuuugly but it works just fine AFAIK */
void make_nice_title (GtkWidget *label, GdkFont *font, char *full_title)
{
	int i;
	char *title;
	char *last_break=NULL;

	g_assert (label != NULL);
	g_assert (font != NULL);
	g_assert (full_title != NULL);

	title = g_strdup (full_title);
	for (i=0;i<strlen(title);i++)
	{
		if (gdk_text_width (font, title, i) >= 68)
		{
			if (last_break)
			{
				char *t;
				int k;
				int done=0;
				*last_break = '\n';
				t = last_break+1;
				for (k=0;k<strlen(t) && !done;k++)
				{
					if (gdk_text_width (font, t, k) >= 68)
					{
						/* Cut the title short & make it end in "..." */
						int j;
						if (k > 0)
							*(t + --k) = 0;
						for (j=0;j<3;j++)
							*(t + --k) = '.';
						done = 1;
					}
				}
				break;
			} else
			{
				/* Cut the title short & make it end in "..." */
				int j;
				if (i > 0)
					*(title + --i) = 0;
				for (j=0;j<3;j++)
					*(title + --i) = '.';
				break;
			}
		}

		/* Note if this character can be changed to a newline safely.
		 * This is done now not at the top because bad wrapping can
		 * occur if it is the "breakable character" pushes the string
		 * past the 70 pixel width. */
		if (title[i] == ' ')
			last_break = title + i;
	}

	gtk_label_set_text (GTK_LABEL (label), title);
	g_free (title);
}

GtkWidget *create_icon_pixmap (GtkStyle *style, char *fn, int size)
{
  GdkPixbuf *pixbuf, *spixbuf;
  GtkWidget *w;
  pixbuf = gdk_pixbuf_new_from_file (fn);
  if (pixbuf == NULL)
    return NULL;

  spixbuf = gdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_BILINEAR);
  gdk_pixbuf_unref (pixbuf);
  w = gpe_render_icon (style, spixbuf);
  gdk_pixbuf_unref (spixbuf);

  return w;
}

static gchar *
get_file_extension (gchar *filename)
{
  int i;
  gchar *extension;

  for (i = strlen (filename); i > 0; i--)
  {
    if (filename[i] == '.')
      break;
  }

  if (i == strlen (filename))
  {
    return NULL;
  }
  else
  {
    extension = g_malloc (strlen (filename) - i);
    extension = g_strdup (filename + i + 1);
    return extension;
  }
}

static gint
unignore_press (gpointer data)
{
  ignore_press = 0;
  return FALSE;
}

/* Callback for selecting a program to run */
static gint
button_released (GtkObject *button, gpointer data)
{
  struct mime_type *mime;
  gchar *path, *type;

  gtk_widget_set_state (GTK_WIDGET(button), GTK_STATE_NORMAL);

  /* Clear the variables stating what button is down etc. 
   * and close if we're not on the original button */
  if (GTK_WIDGET (button) != current_button || !current_button_is_down)
  {
    current_button = NULL;
    current_button_is_down = 0;
    return TRUE;
  }

  if (!GTK_IS_EVENT_BOX (button)) /* Could be the notebook! */
    return TRUE;

  if (ignore_press)
    return TRUE;

  /* So we ignore a second press within 1000msec */
  ignore_press = 1;
  gtk_timeout_add (500, unignore_press, NULL);

  mime = (struct mime_type *) gtk_object_get_data (GTK_OBJECT (button), "mime");
  path = (gchar *) gtk_object_get_data (GTK_OBJECT (button), "path");
  type = (gchar *) gtk_object_get_data (GTK_OBJECT (button), "type");

  if (strcmp (type, "directory") == 0)
    set_directory (path);

  return TRUE;	
}

static gint
button_pressed (GtkWidget *button, GdkEventButton *event, gpointer data)
{
  /* We only want left mouse button events */
  if (event && (!(event->button == 1)))
    return TRUE;

  if (ignore_press) /* Ignore double clicks! */ 
  {
    return TRUE;
  }

  current_button = button;
  current_button_is_down = 1;

  gtk_widget_set_state (button, GTK_STATE_SELECTED);

  return TRUE;	
}

static gint
button_enter (GtkWidget *button, GdkEventCrossing *event)
{
  /* We only want left mouse button events */
  if (!(event->state & 256))
  {
    current_button = NULL;
    return TRUE;
  }

  /* If we're moving onto the button that was last selected,
     do the same as if we've just started pressing on it again */
  if (button == current_button)
    button_pressed (button, NULL, NULL);

  return TRUE;
}

static gint
button_leave (GtkWidget *button, GdkEventCrossing *event)
{
  gtk_widget_set_state (button, GTK_STATE_NORMAL);
  current_button_is_down = 0;
  return TRUE;
}

/* Make the contents for the current directory. */
static void
set_icons_view (void)
{
  struct dirent *d;
  struct mime_type *file_mime;
  DIR *dir;
  int i = 0;
  int cols;
  int loop_num = 0;
  gchar *fp, *buf, *extension, *file_type;
  gchar *previous_extension;
  GdkFont *font; /* Font in the label */
  char *icon;
  GdkPixbuf *pixbuf = NULL, *spixbuf;
  GtkWidget *pixmap;
  GtkWidget *table, *button, *label, *vbox;

  loading_directory = 1;

  if (view_widget)
    gtk_widget_destroy (view_widget);

  /* Get the number of columns assuming 70px per item, 20px scrollbar */
  /* This should really find out how much space it *can* use */
  if (window->allocation.width - 20 > 70)
    cols = (window->allocation.width - 20) / 70;
  else
    cols = 1;

  table = gtk_table_new (3, cols, TRUE);
  view_widget = table;
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (view_scrolld), table);
  gtk_widget_show (table);

  dir = opendir (current_directory);
  if (dir)
  {
    while (d = readdir (dir), d != NULL)
    {
      if (loading_directory == 0)
        break;

      if (d->d_name[0] != '.')
      {
        struct stat s;
        fp = g_strdup_printf ("%s", d->d_name);

        if (strcmp (current_directory, "/") == 0)
          buf = g_strdup_printf ("/%s", fp);
        else
          buf = g_strdup_printf ("%s/%s", current_directory, fp);

        if (stat (buf, &s) == 0)
        {
          if (loop_num > SPINNER_TIMEOUT)
          {
            loop_num = 0;
            update_spinner ();

            while (gtk_events_pending ())
              gtk_main_iteration ();
          }
          loop_num++;

          if (S_ISDIR (s.st_mode))
          {
            file_type = g_strdup_printf ("directory");
            pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/directory.png");
            previous_extension = g_strdup_printf ("");
          }
          else
          {
            file_type = g_strdup_printf ("file");
            extension = get_file_extension (fp);

            if (mime_types)
            {
              GSList *iter;
              struct mime_type *file_mime;

              for (iter = mime_types; iter; iter = iter->next)
              {
                struct mime_type *c = iter->data;

                if (loading_directory == 0)
                  break;

                if (strcmp (c->extension, extension) == 0)
                {
                  file_mime = c;
                  break;
                }
                else
                {
                  file_mime = NULL;
                }
              }
              if (file_mime == NULL)
              {
                pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/regular.png");
                previous_extension = g_strdup_printf ("");
              }
              else if (strcmp (previous_extension, file_mime->extension))
              {
                  pixbuf = gdk_pixbuf_new_from_file (g_strdup_printf ("%s/share/gpe/pixmaps/default/gpe-filemanager/document-icons/gnome-%s.png", PREFIX, file_mime->icon));
                  previous_extension = g_strdup_printf ("%s", file_mime->extension);
              }
            }
            else
            {
              pixbuf = gdk_pixbuf_new_from_file (PREFIX "/share/gpe/pixmaps/default/gpe-filemanager/document-icons/regular.png");
              previous_extension = g_strdup_printf ("");
            }
          }

          if (loading_directory == 0)
            break;

          /* Button label */
          label = gtk_label_new ("");

          /* Get the font used by the label */
          if (gtk_rc_get_style (label))
            font = gtk_rc_get_style (label)->font;
          else
            font = gtk_widget_get_style (label)->font;

          make_nice_title (label, font, fp);

          /* Button VBox */
          vbox = gtk_vbox_new (0,0);
          gtk_box_pack_end_defaults (GTK_BOX (vbox), label);

          spixbuf = gdk_pixbuf_scale_simple (pixbuf, 32, 32, GDK_INTERP_BILINEAR);

          pixmap = gpe_render_icon (view_scrolld->style, spixbuf);
          gdk_pixbuf_unref (spixbuf);
          gtk_box_pack_end_defaults (GTK_BOX (vbox), pixmap);

          /* The 'button' itself */
          button = gtk_event_box_new ();
          /* The "Ay" is arbitrary; I figure'd it'd give a decent approximation at the
             max. height of the font */
          //gtk_widget_set_usize (button, 70, 55 + 2*gdk_text_height(font, "Ay", 2));

          gtk_object_set_data (GTK_OBJECT (button), "mime", (gpointer) file_mime);
          gtk_object_set_data (GTK_OBJECT (button), "path", (gpointer) buf);
          gtk_object_set_data (GTK_OBJECT (button), "type", (gpointer) file_type);

          gtk_signal_connect( GTK_OBJECT (button), "button_release_event", GTK_SIGNAL_FUNC (button_released), NULL);
          gtk_signal_connect( GTK_OBJECT (button), "button_press_event", GTK_SIGNAL_FUNC (button_pressed), NULL);
          gtk_signal_connect( GTK_OBJECT (button), "enter_notify_event", GTK_SIGNAL_FUNC (button_enter), NULL);
          gtk_signal_connect( GTK_OBJECT (button), "leave_notify_event", GTK_SIGNAL_FUNC (button_leave), NULL);

          gtk_container_add (GTK_CONTAINER (button), vbox);
          gtk_table_attach_defaults (GTK_TABLE (table), button, i%cols,i%cols+1,i/cols,i/cols+1);

          gtk_widget_show (vbox);
          gtk_widget_show (label);
          gtk_widget_show (pixmap);
          gtk_widget_show (button);

          i++;
        }
      }
    }
    closedir (dir);
    set_spinner_rest ();
  }
  loading_directory = 0;
}

static void
set_list_view (void)
{
  
}

static void
set_view (GtkWidget *button, gpointer data)
{
  if (strcmp (current_view, (gchar *) data) == 0)
  {
    //gtk_widget_destroy (view_widget);

    if (strcmp ((gchar *) data, "icons") == 0)
    {
      current_view = g_strdup_printf ("icons");
      set_icons_view ();
    }
    else if (strcmp ((gchar *) data, "list") == 1)
    {
      current_view = g_strdup_printf ("list");
      set_list_view ();
    }
  }
}

static void
refresh_view (void)
{
  set_view (NULL, current_view);
}

static void
set_directory (gchar *new_directory)
{
  struct stat s;
  GtkWidget *entry;

  if (stat (new_directory, &s) == 0)
  {
    if (S_ISDIR (s.st_mode))
    {
      entry = (GtkWidget *) gtk_object_get_data (GTK_OBJECT (window), "entry");
      current_directory = g_strdup_printf ("%s", new_directory);
      gtk_entry_set_text (GTK_ENTRY (entry), current_directory);
      history = g_list_append (history, current_directory);
      history_place = g_list_length (history) - 1;
      refresh_view ();
    }
    else
    {
      gpe_error_box ("This file isn't a directory.");
    }
  }
  else
  {
    gpe_error_box ("No such file or directory.");
  }
}

static void
goto_directory (GtkWidget *widget, GtkWidget *entry)
{
  struct stat s;
  gchar *new_directory;

  safety_check ();

  new_directory = gtk_entry_get_text (GTK_ENTRY (entry));
  set_directory (new_directory);
}

static void
set_directory_home (GtkWidget *widget, gpointer data)
{
  set_directory (g_get_home_dir ());
}

static void
up_one_level (GtkWidget *widget, GtkWidget *entry)
{
  int i;
  int found_slash = 0;
  gchar *new_directory;

  safety_check ();

  new_directory = gtk_entry_get_text (GTK_ENTRY (entry));

  for (i = strlen (new_directory); i > 0; i--)
  {
    if (new_directory[i] == '/')
    {
      if (found_slash ==1)
        break;

      found_slash = 1;
      new_directory[i] = 0;
    }

    if (found_slash == 0)
    {
      new_directory[i] = 0;
    }
  }

  set_directory (new_directory);
}

static void
history_back (GtkWidget *widget, GtkWidget *entry)
{
  gchar *new_place;

  safety_check ();

  if (history_place > 0)
  {
    history_place--;

    new_place = g_list_nth_data (history, history_place);
    gtk_entry_set_text (GTK_ENTRY (entry), new_place);
    current_directory = g_strdup_printf ("%s", new_place);
    refresh_view ();
  }
}

static void
history_forward (GtkWidget *widget, GtkWidget *entry)
{
  gchar *new_place;

  safety_check ();

  if (history_place < g_list_length (history) - 1)
  {
    history_place++;

    new_place = g_list_nth_data (history, history_place);
    gtk_entry_set_text (GTK_ENTRY (entry), new_place);
    current_directory = g_strdup_printf ("%s", new_place);
    refresh_view ();
  }
}

static void
update_spinner (void)
{
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;

  if (current_spinner_num == num_spinner_files)
    current_spinner_num = 0;

  if (spinner_pixbuf[current_spinner_num] != NULL)
  {
    gpe_render_pixmap (spinner_style ? &spinner_style->bg[GTK_STATE_NORMAL] : NULL,  spinner_pixbuf[current_spinner_num], &pixmap, &bitmap);
  
    gtk_pixmap_set (GTK_PIXMAP (spinner), pixmap, bitmap);
  }
  current_spinner_num++;
}

static void
set_spinner_rest (void)
{
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;

  gpe_render_pixmap (spinner_style ? &spinner_style->bg[GTK_STATE_NORMAL] : NULL,  spinner_rest_pixbuf, &pixmap, &bitmap);
  gtk_pixmap_set (GTK_PIXMAP (spinner), pixmap, bitmap);
}

int
main (int argc, char *argv[])
{
  GtkWidget *vbox, *hbox, *hbox2, *toolbar, *toolbar2, *entry, *view_option_menu, *view_menu, *view_menu_item;
  GdkPixbuf *p;
  GtkWidget *pw;
  GdkPixmap *pmap;
  GdkBitmap *bmap;
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *rest_pixbuf = NULL;
  GdkPixmap *rest_pixmap;
  GdkBitmap *rest_bitmap;
  int i;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  setlocale (LC_ALL, "");

  bindtextdomain (PACKAGE, PACKAGE_LOCALE_DIR);
  textdomain (PACKAGE);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "GPE Filemanager");
  gtk_widget_set_usize (GTK_WIDGET (window), window_x, window_y);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_exit), NULL);

  gtk_widget_realize (window);

  vbox = gtk_vbox_new (FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  hbox2 = gtk_hbox_new (FALSE, 0);

  entry = gtk_entry_new ();
  gtk_object_set_data (GTK_OBJECT (window), "entry", (gpointer) entry);

  view_scrolld = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view_scrolld), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  view_option_menu = gtk_option_menu_new ();
  view_menu = gtk_menu_new ();
  gtk_option_menu_set_menu(GTK_OPTION_MENU (view_option_menu) ,view_menu);

  view_menu_item = gtk_menu_item_new_with_label ("Icon view");
  gtk_widget_show (view_menu_item);
  gtk_menu_append (GTK_MENU (view_menu), view_menu_item);
  gtk_signal_connect (GTK_OBJECT (view_menu_item), "activate", 
     (GtkSignalFunc) set_view, "icons");

  view_menu_item = gtk_menu_item_new_with_label ("List view");
  gtk_widget_show (view_menu_item);
  gtk_menu_append (GTK_MENU (view_menu), view_menu_item);
  gtk_signal_connect (GTK_OBJECT (view_menu_item), "activate", 
     (GtkSignalFunc) set_view, "list");

  toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_SPACE_LINE);

  toolbar2 = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_button_relief (GTK_TOOLBAR (toolbar2), GTK_RELIEF_NONE);
  gtk_toolbar_set_space_style (GTK_TOOLBAR (toolbar2), GTK_TOOLBAR_SPACE_LINE);

  p = gpe_find_icon ("left");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Back"), 
			   _("Back"), _("Back"), pw, history_back, entry);

  p = gpe_find_icon ("right");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Forward"), 
			   _("Forward"), _("Forward"), pw, history_forward, entry);

  p = gpe_find_icon ("up");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Up one level"), 
			   _("Up one level"), _("Up one level"), pw, up_one_level, entry);

  p = gpe_find_icon ("stop");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Stop"), 
			   _("Stop"), _("Stop"), pw, safety_check, NULL);

  p = gpe_find_icon ("refresh");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Refresh"), 
			   _("Refresh"), _("Refresh"), pw, refresh_view, NULL);

  p = gpe_find_icon ("home");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Home"), 
			   _("Home"), _("Home"), pw, set_directory_home, NULL);

  p = gpe_find_icon ("dir-up");
  pw = gpe_render_icon (window->style, p);
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar2), _("Goto Location"), 
			   _("Goto Location"), _("Goto Location"), pw, goto_directory, entry);

  spinner_style = hbox2->style;

  rest_pixbuf = gdk_pixbuf_new_from_file (g_strdup_printf ("%s/share/gpe/pixmaps/default/gpe-filemanager/spinner/rest.png", PREFIX));
  spinner_rest_pixbuf =  gdk_pixbuf_scale_simple (rest_pixbuf, 16, 16, GDK_INTERP_BILINEAR);
  gpe_render_pixmap (spinner_style ? &spinner_style->bg[GTK_STATE_NORMAL] : NULL,  spinner_rest_pixbuf, &rest_pixmap, &rest_bitmap);
  spinner = gtk_pixmap_new (rest_pixmap, rest_bitmap);

  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (vbox));
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), view_option_menu, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), spinner, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), toolbar2, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), view_scrolld, TRUE, TRUE, 0);

  if (gpe_find_icon_pixmap ("icon", &pmap, &bmap))
    gdk_window_set_icon (window->window, NULL, pmap, bmap);

  gtk_widget_show (window);
  gtk_widget_show (vbox);
  gtk_widget_show (hbox);
  gtk_widget_show (hbox2);
  gtk_widget_show (toolbar);
  gtk_widget_show (toolbar2);
  gtk_widget_show (entry);
  gtk_widget_show (view_option_menu);
  gtk_widget_show (view_menu);
  gtk_widget_show (view_scrolld);
  gtk_widget_show (spinner);

  if (sql_start ())
    exit (1);

  set_directory_home (NULL, entry);
  history_place--;

  gtk_option_menu_set_history (GTK_OPTION_MENU (view_option_menu), 0);

  if (mime_types)
  {
    GSList *iter;
    for (iter = mime_types; iter; iter = iter->next)
    {
      struct mime_type *c = iter->data;
      printf ("\n--\nSQL > mime_name: %s\nSQL > extension: %s\n--\n", c->mime_name, c->extension);
    }
  }

  while (i <= num_spinner_files)
  {
    pixbuf = gdk_pixbuf_new_from_file (g_strdup_printf ("%s/share/gpe/pixmaps/default/gpe-filemanager/spinner/%s.png", PREFIX, spinner_file[i]));
    spinner_pixbuf[i] =  gdk_pixbuf_scale_simple (pixbuf, 16, 16, GDK_INTERP_BILINEAR);

    i++;
  }

  gtk_main();

  return 0;
}