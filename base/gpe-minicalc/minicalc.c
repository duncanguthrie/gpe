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
#include <locale.h>
#include <pty.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "init.h"

#include <gmp.h>

static char buf[256];
static GtkWidget *display;

static mpf_t accum, dv;
static int base = 10;

static gboolean display_volatile;
static gboolean in_error;
static char binop;
static gboolean point;

void
update_display (void)
{
  gtk_label_set_text (GTK_LABEL (display), buf[0] ? buf : "0");
}

void
error (void)
{
  gtk_label_set_text (GTK_LABEL (display), "Error");
  in_error = TRUE;
}

void
display_to_val (void)
{
  char *p = g_malloc (strlen (buf) + 1);
  if (base == 2)
    {
      char *i = buf, *j = p;
      while (*i)
	{
	  if (*i != '\'')
	    *(j++) = *i;
	  i++;
	}
      *j = 0;
    }
  else
    strcpy (p, buf);
  if (mpf_set_str (dv, p, base))
    error ();
  g_free (p);
}

void
val_to_display (void)
{
  if (!in_error)
    {
      mp_exp_t exp;
      char *ss = mpf_get_str (NULL, &exp, base, sizeof (buf) - 32, dv);
      char *s = ss;
      size_t l = strlen (s);
      char *op = buf;
      size_t oplen = sizeof (buf) - 1;
      
      if (s[0] == '-')
	{
	  *op++ = '-';
	  oplen--;
	  l--;
	  s++;
	}
      
      if (exp == 0)
	{
	  op[0] = '0';
	  if (l)
	    {
	      op[1] = '.';
	      strcpy (op+2, s);
	    }
	  else
	    op[1] = 0;
	}
      else if (exp == l)
	{
	  if (base == 2)
	    {
	      /* insert ' spacers every four bits, per Colin Marquardt request */
	      int n = ((l + 3) & ~3) - l, i;
	      memset (op, '0', n);
	      op += n;
	      for (i = 0; i < l; i++)
		{
		  if (n && ((n & 3) == 0))
		    *op++ = '\'';
		  *op++ = s[i];
		  n++;
		}
	      *op++ = 0;
	    }
	  else
	    strncpy (op, s, oplen);
	}
      else if (exp > 0 && exp < l)
	{
	  strncpy (op, s, exp);
	  op[exp] = '.';
	  strcpy (op + exp + 1, s + exp);
	}
      else if (exp < 0)
	{
	  if (exp > -40)
	    {
	      int n;
	      strcpy (op, "0.");
	      for (n = exp; n < 0; n++)
		strcat (op, "0");
	      strcat (op, s);
	    }
	  else
	    {
	      char *p;
	      op[0] = s[0];
	      op[1] = '.';
	      p = stpcpy (op + 2, s);
	      snprintf (p, oplen - (p - op), "@%d", exp);
	    }
	}
      else
	{
	  if (exp <= 40)
	    {
	      int n;
	      strcpy (op, s);
	      for (n = 0; n < (exp - l); n++)
		strcat (op, "0");
	    }
	  else
	    {
	      char *p;
	      op[0] = s[0];
	      op[1] = '.';
	      p = stpcpy (op + 2, s);
	      snprintf (p, oplen - (p - op), "@%d", exp);
	    }
	}
      free (ss);
      display_volatile = TRUE;
      update_display ();
    }
}

void
push_char (char c)
{
  size_t s;
  if (in_error)
    return;
  if (display_volatile)
    {
      point = FALSE;
      s = 0;
      display_volatile = FALSE;
    }
  else
    s = strlen (buf);
  if (s < sizeof (buf) - 1)
    {
      buf[s] = c;
      buf[s+1] = 0;
      update_display ();
    }
}

void
push_digit (GtkWidget *w, int n)
{
  char c = "0123456789ABCDEF"[n];
  if (buf[0] != 0 || n != 0)
    push_char (c);
}

void
clear (gboolean all)
{
  point = FALSE;
  buf[0] = 0;
  update_display ();
  in_error = FALSE;
  if (all)
    binop = 0;
}

GtkWidget *
number_button (int n)
{
  GtkWidget *w;
  char buf [2];
  if (n < 10)
    buf[0] = n + '0';
  else
    buf[0] = n + 'A' - 10;
  buf[1] = 0;
  w = gtk_button_new_with_label (buf);
  gtk_widget_set_usize (w, 16, 16);
  gtk_signal_connect (GTK_OBJECT (w), "clicked",
		      GTK_SIGNAL_FUNC (push_digit),
		      (gpointer)n);
  return w;
}

void
set_base (GtkWidget *w, int n)
{
  display_to_val ();
  base = n;
  val_to_display ();
}

void
base_button (GtkWidget *box, char *string, int n)
{
  static GtkWidget *radio_group;
  GtkWidget *w;
  if (radio_group)
    w = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio_group), string);
  else
    {
      w = gtk_radio_button_new_with_label (NULL, string);
      radio_group = w;
    }
  gtk_signal_connect (GTK_OBJECT (w), "clicked", GTK_SIGNAL_FUNC (set_base), (gpointer)n);
  gtk_widget_show (w);
  gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
}

void
do_binop (GtkWidget *w, int op)
{
  binop = op;
  display_to_val ();
  mpf_set (accum, dv);
  display_volatile = TRUE;
}

void
do_clear (GtkWidget *w, int flag)
{
  clear (flag ? TRUE : FALSE);
}

void
do_equals (GtkWidget *w)
{
  if (binop)
    {
      display_to_val ();
      switch (binop)
	{
	case '+':
	  mpf_add (dv, dv, accum);
	  break;
	case '-':
	  mpf_sub (dv, accum, dv);
	  break;
	case '*':
	  mpf_mul (dv, dv, accum);
	  break;
	case '/':
	  mpf_div (dv, accum, dv);
	  break;
	}
      val_to_display ();
      binop = 0;
    }
}

void
do_point (GtkWidget *w)
{
  if (point == FALSE && strlen (buf) < (sizeof (buf) - 2))
    {
      point = TRUE;
      strcat (buf, ".");
    }
}

struct
{
  char *str;
  void *func;
  int arg;
} op_buttons[] = 
  {
    { NULL },
    { NULL },
    { "C", do_clear, 0 },
    { "AC", do_clear, 1 },

    { NULL },
    { NULL },
    { NULL },
    { NULL },

    { "+", do_binop, '+' },
    { "-", do_binop, '-' },
    { "*", do_binop, '*' },
    { "/", do_binop, '/' },

    { ".", do_point },
    { NULL }, //{ "+/-", do_plusminus },
    { NULL },
    { "=", do_equals },
  };

void
make_button (GtkWidget *table, char *str, int x, int y, void *func, int arg)
{
  if (str)
    {
      GtkWidget *w = gtk_button_new_with_label (str);
      gtk_widget_set_usize (w, 32, 16);
      gtk_table_attach_defaults (GTK_TABLE (table), w, x, x+1, y, y+1);
      if (func)
	gtk_signal_connect (GTK_OBJECT (w), "clicked", GTK_SIGNAL_FUNC (func), (gpointer)arg);
      gtk_widget_show (w);
    }
}

void
build_ops (GtkWidget *table)
{
  int x, y;
  for (y = 0; y < 4; y++)
    {
      for (x = 0; x < 4; x++)
	{
	  int n = x + (y * 4);
	  make_button (table, op_buttons[n].str, x, y, op_buttons[n].func, op_buttons[n].arg);
	}
    }
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *hbox;
  Atom window_type_atom, window_type_dock_atom;
  Display *dpy;
  Window xwindow;
  int x, y;
  GtkWidget *vbox_base;
  GtkWidget *optable;

  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (window);

  dpy = GDK_WINDOW_XDISPLAY (window->window);
  xwindow = GDK_WINDOW_XWINDOW (window->window);

  window_type_atom = XInternAtom (dpy, "_NET_WM_WINDOW_TYPE", False);
  window_type_dock_atom = XInternAtom (dpy,
				       "_NET_WM_WINDOW_TYPE_TOOLBAR", False);

  XChangeProperty (dpy, xwindow, 
		   window_type_atom, XA_ATOM, 32, 
		   PropModeReplace, (unsigned char *)
		   &window_type_dock_atom, 1);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  display = gtk_label_new ("");
  gtk_widget_show (display);
  gtk_misc_set_alignment (GTK_MISC (display), 1.0, 0.5);
  gtk_box_pack_start (GTK_BOX (vbox), display, FALSE, FALSE, 0);
  table = gtk_table_new (4, 4, TRUE);
  gtk_widget_show (table);
  hbox = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  for (y = 0; y < 4; y++)
    {
      for (x = 0; x < 4; x++)
	{
	  int n = x + ((3 - y) * 4);
	  GtkWidget *b = number_button (n);
	  gtk_widget_show (b);
	  gtk_table_attach_defaults (GTK_TABLE (table), b, x, x+1, y, y+1);
	}
    }

  gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

  vbox_base = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox_base);
  base_button (vbox_base, "dec", 10);
  base_button (vbox_base, "oct", 8);
  base_button (vbox_base, "hex", 16);
  base_button (vbox_base, "bin", 2);
  gtk_box_pack_end (GTK_BOX (hbox), vbox_base, FALSE, FALSE, 0);

  optable = gtk_table_new (4, 4, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), optable, FALSE, FALSE, 4);
  gtk_widget_show (optable);
  build_ops (optable);

  clear (TRUE);
  mpf_init (dv);
  mpf_init (accum);

  gtk_widget_show (window);
  
  gtk_main ();
  
  exit (0);
}
