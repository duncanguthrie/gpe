/*
 *  main.c
 *	part of galculator
 *  	(c) 2002 Simon Floery (simon.floery@gmx.at)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "calc_basic.h"
#include "galculator.h"
#include "display.h"
#include "general_functions.h"

#include "interface.h"
#include "support.h"

#define MASK_NUMLOCK GDK_MOD2_MASK

int main (int argc, char *argv[])
{
	GtkWidget 		*main_window;

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	gtk_set_locale ();
	gtk_init (&argc, &argv);
	
	add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
   	
	/*
	 * The following code was added by Glade to create one of each component
	 * (except popup menus), just so that you see something after building
	 * the project. Delete any components that you don't want shown initially.
	 */
	main_window = create_main_window ();
	gtk_widget_show (main_window);
	 
	/* usually, only Shift, CTRL and ALT modifiers are paid attention to by accelerator code.
	 * add MOD2 (NUMLOCK allover the worl?) to the list. We have to do this for a working
	 * keypad.
	 */
	  
	gtk_accelerator_set_default_mod_mask (gtk_accelerator_get_default_mod_mask () | GDK_MOD2_MASK); 

	gtk_window_set_title ((GtkWindow *)main_window, g_strdup_printf ("%s v%s", PROG_NAME, PROG_VERSION));
	
	display_init (main_window);
	  
	statusbar_init (main_window);
	
	calc_tree_init (0);
	
	current_status.number = CS_DEC;
	current_status.angle = CS_DEG;
	current_status.fmod = 0;
	gtk_main ();
	return 0;
}

