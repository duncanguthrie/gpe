/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>

#include <glib/gerror.h>
#include <glib/gstrfuncs.h>

#include <pango/pango-attributes.h>

#include <gtk/gtkwindow.h>
#include <gtk/gtkstock.h>
#include <gtk/gtkhscale.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkvseparator.h>
#include <gtk/gtkhseparator.h>
#include <gtk/gtkalignment.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtknotebook.h>
#include <gtk/gtktextview.h>
#include <gtk/gtkentry.h>

#include "interface.h"
#include "callbacks.h"

#define BUTTONSIZE 32

#define button_init(widget, stock, container) \
    do { \
        widget = gtk_button_new(); \
        gtk_container_add (GTK_CONTAINER (widget), \
        gtk_image_new_from_stock (stock, GTK_ICON_SIZE_SMALL_TOOLBAR)); \
        gtk_widget_set_size_request (widget, -1, BUTTONSIZE); \
        gtk_box_pack_start (GTK_BOX (container), widget, TRUE, TRUE, 0); \
    } while(0)


#define label_init(label, attrs) \
    do { \
        label = gtk_label_new (g_strdup (" ")); \
        gtk_misc_set_alignment (GTK_MISC (label), 0.1, 0.5); \
        if (attrs) \
            gtk_label_set_attributes (GTK_LABEL (label), attrs); \
    } while(0)
    


void
player_init (Starling *st)
{
    st->pl = play_list_new ();

    play_list_set_random (st->pl, TRUE);
    //play_list_set_sink (st->pl, "esdsink");
}

void
interface_init (Starling *st)
{
    GtkWidget *p_button_box;
    GtkWidget *l_button_box;
    GtkWidget *hbox;
    GtkWidget *hbox1;
    GtkWidget *hbox2;
    GtkWidget *vbox;
    GtkWidget *alignment;
    GtkWidget *separator;
    GtkWidget *label;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *scrolled;
    PangoAttribute *attribute;
    PangoAttrList *attrs;
    
    st->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (st->window), "Starling");
    
    st->scale = gtk_hscale_new_with_range (0, 100, 1);
    gtk_scale_set_draw_value (GTK_SCALE (st->scale), FALSE);
    GTK_WIDGET_UNSET_FLAGS (st->scale, GTK_CAN_FOCUS);
    
    p_button_box = gtk_hbox_new (TRUE, 0);
    button_init (st->prev, GTK_STOCK_MEDIA_PREVIOUS, p_button_box);
    button_init (st->playpause, GTK_STOCK_MEDIA_PLAY, p_button_box);
    button_init (st->next, GTK_STOCK_MEDIA_NEXT, p_button_box);
    
    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), p_button_box, FALSE, FALSE, 0);
    
    
    st->random = gtk_check_button_new_with_label (_("Random"));
    alignment = gtk_alignment_new (0.8, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (alignment), st->random);
    gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
    
    vbox = gtk_vbox_new (FALSE, 5);

    attrs = pango_attr_list_new ();
    attribute = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
    pango_attr_list_insert (attrs, attribute);
    label_init (st->artist, attrs);
    gtk_label_set_text (GTK_LABEL (st->artist), _("Not playing"));
    
    attrs = pango_attr_list_new ();
    attribute = pango_attr_style_new (PANGO_STYLE_OBLIQUE);
    pango_attr_list_insert (attrs, attribute);
    label_init (st->title, attrs);
    
    label_init (st->time, NULL);
    
    gtk_box_pack_start (GTK_BOX (vbox), st->artist, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), st->title, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), st->time, FALSE, FALSE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), st->scale, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    
    separator = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (vbox), separator, FALSE, FALSE, 3);
    
    st->store = gtk_list_store_new (COL_NUMCOLS, G_TYPE_STRING, 
                                    G_TYPE_INT, G_TYPE_POINTER);
    st->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (st->store));
    g_object_unref (st->store);
    
    gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (st->treeview), FALSE);
    gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (st->treeview), TRUE);
    renderer = gtk_cell_renderer_text_new ();
    //g_object_set(renderer, "weight", PANGO_WEIGHT_BOLD, "weight-set", FALSE, NULL);
    gtk_tree_view_insert_column_with_attributes ( GTK_TREE_VIEW ( st->treeview ),
                -1, _( "Title" ), renderer, "text", COL_TITLE, NULL);
    /* XXX
    gtk_tree_view_insert_column_with_attributes ( GTK_TREE_VIEW ( st->treeview ),
                20, _( "Length" ), renderer, "text", COL_TIME, NULL);
    */
    
    column = gtk_tree_view_get_column (GTK_TREE_VIEW (st->treeview), COL_TITLE);
    gtk_tree_view_column_add_attribute (column, renderer, "weight", COL_FONT_WEIGHT);

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    gtk_container_add (GTK_CONTAINER (scrolled), st->treeview);
    gtk_container_add (GTK_CONTAINER (vbox), scrolled);
    
    l_button_box = gtk_hbox_new (TRUE, 0);
    button_init (st->add, GTK_STOCK_ADD, l_button_box);
    button_init (st->remove, GTK_STOCK_REMOVE, l_button_box);
    separator = gtk_vseparator_new();
    gtk_box_pack_start (GTK_BOX (l_button_box), separator, FALSE, FALSE, 0);
    
    button_init (st->up, GTK_STOCK_GO_UP, l_button_box);
    button_init (st->down, GTK_STOCK_GO_DOWN, l_button_box);
    separator = gtk_vseparator_new();
    gtk_box_pack_start (GTK_BOX (l_button_box), separator, FALSE, FALSE, 0);
    
    button_init (st->clear, GTK_STOCK_NEW, l_button_box);
    button_init (st->save, GTK_STOCK_SAVE_AS, l_button_box);
    
    gtk_box_pack_start (GTK_BOX (vbox), l_button_box, FALSE, FALSE, 0);
    
    st->notebook = gtk_notebook_new ();
    gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
        gtk_label_new (_("Player")));

    gtk_container_add (GTK_CONTAINER (st->window), st->notebook);

    /* Lyrics tab */

    vbox = gtk_vbox_new (FALSE, 5);
    st->textview = gtk_text_view_new ();
    GTK_TEXT_VIEW (st->textview)->editable = FALSE;

    scrolled = gtk_scrolled_window_new (NULL, NULL);
    
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
            GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

    gtk_container_add (GTK_CONTAINER (scrolled), st->textview);

    gtk_box_pack_start (GTK_BOX (vbox), scrolled, TRUE, TRUE, 3);

    gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
        gtk_label_new (_("Lyrics")));

    /* Web services tab (for now, last.fm) */

    hbox1 = gtk_hbox_new (FALSE, 2);
    label = gtk_label_new (_("Username:"));
    st->webuser_entry = gtk_entry_new ();

    gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox1), st->webuser_entry, TRUE, TRUE, 0);
    
    hbox2 = gtk_hbox_new (FALSE, 2);
    label = gtk_label_new (_("Password:"));
    st->webpasswd_entry = gtk_entry_new ();
    gtk_entry_set_visibility (GTK_ENTRY (st->webpasswd_entry), FALSE);
    
    gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hbox2), st->webpasswd_entry, TRUE, TRUE, 0);
    
    /*gtk_box_pack_start (GTK_BOX (hbox), st->webuser_entry, TRUE, TRUE, 0);

    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    */
    vbox = gtk_vbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (vbox), hbox1, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);

    st->web_count = gtk_label_new ("");
    st->web_submit = gtk_button_new_with_label (_("Submit"));
    gtk_box_pack_start (GTK_BOX (vbox), st->web_count, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), st->web_submit, FALSE, FALSE, 0);
    
    gtk_notebook_append_page (GTK_NOTEBOOK (st->notebook), vbox,
        gtk_label_new (_("last.fm")));


    /* Init some variables */

    st->bolded = -1;
    st->scale_pressed = FALSE;

    /* Just make sure */
    st->fs = NULL;
    st->fs_last_path = NULL;

    st->current_length = -1;
    st->has_lyrics = FALSE;
    st->enqueued = FALSE;
    
    gtk_widget_show_all (st->window);
}

