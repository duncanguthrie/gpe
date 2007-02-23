/*
 * Initial main.c file generated by Glade. Edit as required.
 * Glade will not overwrite this file.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <libintl.h>
#include <unistd.h>
#include <sys/types.h>

#include <gpe/init.h>
#include <gpe/pixmaps.h>
#include <gpe/smallbox.h>
#include <gpe/errorbox.h>

#include "interface.h"
#include "support.h"
#include "db.h"
#include "../include/callbacks.h"

// Define macro for gettext if not done previous (e.g. by glade)
#ifndef _
# define _(x) gettext(x)
#endif

#define MY_PIXMAPS_DIR PREFIX "/share/gpe/pixmaps/"

struct gpe_icon my_icons[] = {
  {"delete"},
  {"new"},
  {"ok"},
  {"cancel"},
  {"exit"},
  {"question"},
  {"icon", MY_PIXMAPS_DIR "/gpe-dataconf.png"},
  {NULL, NULL}
};


GtkWidget *wdcmain;		// making this global makes some things easier...
GtkWidget *bNew;
GtkWidget *bDelete;


int
main (int argc, char *argv[])
{
  GtkWidget *vbox1;
  GtkWidget *toolbar;
  GtkWidget *pw;
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
  GtkListStore *liststore;
  GtkTreeView *treeview;
  GtkTreeViewColumn *tcolumn;
  GtkCellRenderer *renderer;
	
  if (gpe_application_init (&argc, &argv) == FALSE)
    exit (1);

  if (gpe_load_icons (my_icons) == FALSE)
    exit (1);

  wdcmain = create_wdcmain ();

  /*
   * Now add gpe specific widget generation here
   */
  gtk_widget_realize (wdcmain);

  if (gpe_find_icon_pixmap ("icon", &pixmap, &bitmap))
    gdk_window_set_icon (wdcmain->window, NULL, pixmap, bitmap);

  vbox1 = lookup_widget (wdcmain, "vbox1");

  toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);

  gtk_box_pack_start (GTK_BOX (vbox1), toolbar, FALSE, FALSE, 0);
  gtk_box_reorder_child (GTK_BOX (vbox1), toolbar, 0);
  gtk_widget_show (toolbar);

  pw = gtk_image_new_from_pixbuf (gpe_find_icon ("new"));
  bNew = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("New rule"),
			   _("New rule"), _("New rule"), pw,
			   (GtkSignalFunc) on_bNew_clicked, NULL);

  pw = gtk_image_new_from_pixbuf(gpe_find_icon ("delete"));
  bDelete = gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Delete rule"),
			   _("Delete rule"), _("Delete rule"), pw,
			   (GtkSignalFunc) on_bDelete_clicked, NULL);
  
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
  
  pw = gtk_image_new_from_pixbuf(gpe_find_icon ("exit"));
  gtk_toolbar_append_item (GTK_TOOLBAR (toolbar), _("Close application"),
			   _("Close application"), _("Close application"), pw,
			   (GtkSignalFunc) gtk_main_quit, NULL);

  // build treeview
  treeview = GTK_TREE_VIEW (lookup_widget (wdcmain, "tvACL"));
  
  // Store scheme:
  // | username | read allowed | write allowed | uid |
  liststore =
    gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
			G_TYPE_INT);

  gtk_tree_view_set_model (treeview, GTK_TREE_MODEL (liststore));
  gtk_tree_view_set_rules_hint (treeview, TRUE);

  renderer = gtk_cell_renderer_text_new ();
  tcolumn = gtk_tree_view_column_new_with_attributes (_("User"),
						      renderer,
						      "text", COLUMN_USER, NULL);
  gtk_tree_view_column_set_min_width(tcolumn,100);
  gtk_tree_view_append_column (treeview, tcolumn);

  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer),FALSE);

  g_signal_connect (G_OBJECT (renderer), "toggled",G_CALLBACK (list_toggle_read), liststore);
    
  tcolumn = gtk_tree_view_column_new_with_attributes (_("read"),
						      renderer,
						      "active", COLUMN_READ, FALSE, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (tcolumn),
				   GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (tcolumn), 50);


  gtk_tree_view_append_column (treeview, tcolumn);
  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer),FALSE);
  g_signal_connect (G_OBJECT (renderer), "toggled",G_CALLBACK (list_toggle_write), liststore);
  tcolumn = gtk_tree_view_column_new_with_attributes (_("write"),
						      renderer,
						      "active", COLUMN_WRITE, FALSE, NULL);
  gtk_tree_view_column_set_sizing (GTK_TREE_VIEW_COLUMN (tcolumn),
				   GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (GTK_TREE_VIEW_COLUMN (tcolumn), 50);
  gtk_tree_view_append_column (treeview, tcolumn);

  // disable some feature if we are not root
  if (geteuid() > 0)  // if true we don't have root permissions
  {
	pw = lookup_widget(wdcmain,"cbPolicy");
	gtk_widget_set_sensitive(pw,FALSE);
#ifdef USE_USQLD
	gtk_widget_set_sensitive(bNew,FALSE);
	gtk_widget_set_sensitive(bDelete,FALSE);
	pw = lookup_widget(wdcmain,"tvACL");
	gtk_widget_set_sensitive(pw,FALSE);
#endif
  }
  // now show main window
  gtk_widget_show (wdcmain);
  gtk_main ();
  return 0;
}