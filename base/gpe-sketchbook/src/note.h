/* $Id$ */
/* gpe-sketchbook -- a sketches notebook program for PDA
 * Copyright (C) 2002 Luc Pionchon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef NOTE_H
#define NOTE_H

#include <gtk/gtk.h>

typedef struct
{
  gchar     * fullpath_filename;
  GtkWidget * thumbnail;
  //gchar     * label;??? (text in clist)
  //gint type; SKETCH, AUDIO, TEXT
  //gint id; ???
  //creation date, last update
  //category
} Note;

Note * note_new();
void   note_delete (Note *   note);
void   note_destroy(gpointer note);//the same for gtk_clist_set_row_data_full()

#endif
