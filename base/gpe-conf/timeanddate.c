/*
 * gpe-conf
 *
 * Copyright (C) 2002  Pierre TARDY <tardyp@free.fr>
 *	             2003, 2004  Florian Boor <florian.boor@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Time and date settings module.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <libintl.h>
#include <time.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>

#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include "applets.h"
#include "timeanddate.h"
#include "suid.h"
#include "misc.h"
#include "timezones.h"

#include <gpe/spacing.h>
#include <gpe/errorbox.h>
#include <gpe/gtkdatecombo.h>
#include <gpe/gpetimesel.h>
#include <gpe/infoprint.h>


/* --- local types and constants --- */

gchar *Ntpservers[6]=  {
	"time.handhelds.org",
	"time.apple.com",
	"ntp2.uni-siegen.de",
	"ptbtime1.ptb.de",
	"ntp2c.mcc.ac.uk",
	"ntppub.tamu.edu"
};

enum {
	COL_VIEW_ID,
	COL_VIEW_NAME,	
	N_VIEW_COLUMNS
};

#define SCREENSAVER_RESET_CMD "xset s reset"
#define TZ_MAXINDEX 10
  
typedef struct 
{
	char tzname[8];
	char dstname[8];
	int utcofs_h;
	int utcofs_m;
	int utcdstofs_h;
	int utcdstofs_m;
} tzinfo;

/* --- module global variables --- */
  
static struct 
{
	GtkWidget *categories;
	GtkWidget *catvbox1;
	GtkWidget *catlabel1;
	GtkWidget *catconthbox1;
	GtkWidget *catindentlabel1;
	GtkWidget *controlvbox1;
	GtkWidget *catvbox2;
	GtkWidget *catlabel2;
	GtkWidget *catconthbox2;
	GtkWidget *catindentlabel2;
	GtkWidget *controlvbox2;
	GtkWidget *catvbox3;
	GtkWidget *catlabel3;
	GtkWidget *catconthbox3;
	GtkWidget *catindentlabel3;
	GtkWidget *controlvbox3;
	GtkWidget *catvbox4;
	GtkWidget *catlabel4;
	GtkWidget *catconthbox4;
	GtkWidget *catindentlabel4;
	GtkWidget *controlvbox4;
	GtkWidget *cal;
	GtkWidget *hbox;
	GtkWidget *tsel;
	GtkWidget *ntpserver;
	GtkWidget *internet;
	GtkWidget *timezoneArea;
	GtkWidget *timezone;
	GtkWidget *usedst;
	GtkWidget *dsth;
	GtkWidget *dstm;
	GtkWidget *defaultdst;
	GtkWidget *dstlh;
	GtkWidget *dstlm;
	GtkWidget *offsetlabel;
} self;

static int trc = 0;  // countdown for waiting for time change
static int tid = 0;  // timeout id
static int isdst;    // is a dst active?
static int need_warning = FALSE;
static int nonroot_mode = FALSE;

/* --- local intelligence --- */

static void update_enabled_widgets(GtkWidget *sender)
{
	gboolean usedst = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.usedst));
	gboolean defaultdst = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.defaultdst));
	
	gtk_widget_set_sensitive(self.offsetlabel,usedst);
	gtk_widget_set_sensitive(self.defaultdst,usedst);
	gtk_widget_set_sensitive(self.dsth,(usedst && !defaultdst));
	gtk_widget_set_sensitive(self.dstm,(usedst && !defaultdst));
	gtk_widget_set_sensitive(self.dstlm,(usedst && !defaultdst));
	gtk_widget_set_sensitive(self.dstlh,(usedst && !defaultdst));
}

/* find current timezone definition */

char* 
get_current_tz(void)
{
	char* fprof;
	char *result;	
	static char tzbuf[31];
	char *cont = NULL;
	int len;
	GError *err = NULL;

	tzbuf[0] = 0;
	
	if (geteuid()) 
		fprof = g_strdup_printf("%s/.profile", g_get_home_dir());
	else
		fprof = g_strdup("/etc/profile");
	
	if (g_file_get_contents(fprof, &cont, &len, &err))
	{
		char *p = strstr(cont, "TZ");
		if (p)
		{
			int i = 0;
			p = strstr(p, "=") + 1;
			while (p[0] == ' ' || p[0] == '\t') p++;
			while (((p + i) < (cont + len)) && (i < 30))
			{
				tzbuf[i] = p[i];
				i++;
				if (p[i] == '\n')
					break;
			}
			tzbuf[i] = 0;
		}
		g_free(cont);			
	}
	if (strlen(tzbuf))
		result = tzbuf;
	else
		result = getenv("TZ");

	g_free(fprof);
	printf("result = %s\n", result);
	return result;  
}

/* This function sets the system timezone */

void
set_timezone (gchar *zone)
{
	FILE *ftimezone;
	
	ftimezone = fopen ("/etc/timezone", "w");
	if (ftimezone)
	{
		fprintf (ftimezone, "%s\n", zone);
		fchmod(fileno(ftimezone), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		fclose (ftimezone);
	}
	else
		g_printerr("Error: Could not write to /etc/timezone\n");
}

/*
 *  This function parses the given TZ string.
 */
tzinfo get_tz_info(char *tzstr)
{
	int i = 0;
	int j = 0;
	tzinfo result;
	char numtmp[10]; 
	char numtmp2[10]; 
	
	/* init with defaults */
	sprintf(result.tzname,"UTC");
	result.dstname[0] = 0;
	result.utcofs_h = 0;
	result.utcofs_m = 0;
	result.utcdstofs_h = 0;
	result.utcdstofs_m = 0;	

	/* if no valid TZ is set, return defaults */
	if ((!tzstr) || (strlen(tzstr)<3)) return result;
	
	/* get timezone name */ 
	while ((i<strlen(tzstr)) && (isalpha(tzstr[i])) && (i < 5))
	{
		result.tzname[i] = tzstr[i];
		i++;
	}
	result.tzname[i] = 0;
	
	/* ignore whitespace */
	while ((i<strlen(tzstr)) && (isblank(tzstr[i])))
	{
		i++;
	}
	
	j = i;
	/* get tz utc offset */
	while ((i<strlen(tzstr)) && (isdigit(tzstr[i]) || ispunct(tzstr[i])))
	{
		numtmp[i-j] = tzstr[i];
		i++;
	}
	
	/* seperate hours/minutes in offset */
	if (strstr(numtmp,":"))
	{
		snprintf(numtmp2,strstr(numtmp,":")-numtmp-1,"%s",numtmp);
		result.utcofs_h = atoi(numtmp2);
		result.utcofs_m = atoi(strstr(numtmp,":")+1);
	}
	else
	{
		result.utcofs_h = atoi(numtmp);
	}
	memset(numtmp,' ',10);
	
	/* ignore whitespace */
	while ((i<strlen(tzstr)) && (isblank(tzstr[i])))
	{
		i++;
	}
	
	j = i;
	/* get dst name */ 
	while ((i<strlen(tzstr)) && (isalpha(tzstr[i])) && (i-j < 5))
	{
		result.dstname[i-j] = tzstr[i];
		i++;
	}
	result.dstname[i-j] = 0;
		
	/* ignore whitespace */
	while ((i<strlen(tzstr)) && (isblank(tzstr[i])))
	{
		i++;
	}
	
	j = i;
	/* get dst utc offset */
	while ((i<strlen(tzstr)) && (isdigit(tzstr[i]) || ispunct(tzstr[i])))
	{
		numtmp[i-j] = tzstr[i];
		i++;
	}
	
	/* seperate hours/minutes in offset */
	if (strstr(numtmp,":"))
	{
		snprintf(numtmp2,strstr(numtmp,":")-numtmp-1,"%s",numtmp);
		result.utcdstofs_h = atoi(numtmp2);
		result.utcdstofs_m = atoi(strstr(numtmp,":")+1);
	}
	else
	{
		result.utcdstofs_h = atoi(numtmp);
	}
	
	/*
		Add parser for dst switching here.
	*/
	
	return(result);
}

gboolean refresh_time()
{
	static char str[256];
	struct pollfd pfd[1];
	gboolean ret = FALSE;
	Display *dpy = GDK_DISPLAY();
		
	Time_Restore();
	memset(str, 0, 256);
	
	pfd[0].fd = suidinfd;
	pfd[0].events = (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI);
	while (poll(pfd, 1, 0))
	{
		if (fgets (str, 255, suidin))
		{
			if (strstr(str, "<success>"))
				gpe_popup_infoprint (dpy, 
			                         _("Time adjusted from network."));
			else
				gpe_error_box(_("Adjusting time from network failed."));

		}
		ret = TRUE;
	}

	trc--;
	if (!trc) 
		ret = TRUE;
	if (ret)
	{
		gtk_widget_set_sensitive(self.internet, TRUE);
		return FALSE;
	}
	system (SCREENSAVER_RESET_CMD);
	return (TRUE);
}
/*
 * version for gtkComboBox
void GetInternetTime()
{
	gchar	*srcSelected;
	
	gtk_widget_set_sensitive(self.internet,FALSE);
	srcSelected	= gtk_combo_box_get_active_text(self.ntpserver);
	printf("selection = %s\n", srcSelected);
	suid_exec("NTPD", srcSelected);
	trc = 60;
	tid = gtk_timeout_add(500, refresh_time, NULL);
}
*/

/**
 * Version for GtkCombo
 */
void GetInternetTime()
{
  gtk_widget_set_sensitive(self.internet,FALSE);
  suid_exec("NTPD",gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (self.ntpserver)->entry)));
  trc = 60;
  tid = gtk_timeout_add(500, refresh_time, NULL);
}

static void
do_tabchange (GtkNotebook * notebook,
	      GtkNotebookPage * page, guint page_num, gpointer user_data)
{
	if (page_num == 1)
		need_warning = TRUE;
}

gchar* getSelectedTimezoneText()
{
	gint		curIndex;
	gint		curAreaIndex;
	guint		idx;
	gchar		**timezonesPointer;
	gchar		*retVal;
	
	retVal		= NULL;
	curAreaIndex	= gtk_combo_box_get_active(GTK_COMBO_BOX(self.timezoneArea));
	curIndex	= gtk_combo_box_get_active(GTK_COMBO_BOX(self.timezone));
	printf("curAreaIndex = %d, curIndex = %d\n", curAreaIndex, curIndex);
	if ((curAreaIndex > -1) && (curIndex > -1))
	{
		idx			= 0;
		timezonesPointer	= timezoneAreaArray[curAreaIndex];		
    		while(*timezonesPointer != NULL) {
    			//printf("t = %s\n", *timezonesPointer);
			if (idx == curIndex)
			{
				retVal	= *timezonesPointer;
				break;
			}
			timezonesPointer++;
			idx++;
		}
	}
	return retVal;
}

void timezonearea_combo_changed(GtkComboBox *widget, gpointer user_data)
{
	GtkListStore	*timezoneModel;
	GtkTreeIter	timezoneIter;
	gint		curIndex;
	gchar		**timezonesPointer;
	guint		idx;

	printf("timezonearea_combo_changed()\n");
	curIndex	= gtk_combo_box_get_active(GTK_COMBO_BOX(self.timezoneArea));
	printf("selected index = %d\n", curIndex);
	if (curIndex > -1)
	{
		timezoneModel		= (GtkListStore *)gtk_combo_box_get_model(GTK_COMBO_BOX(self.timezone));
		// clear previous timezone texts
		gtk_list_store_clear(timezoneModel);
		
		// add new timezone texts
		idx			= 0;
		timezonesPointer	= timezoneAreaArray[curIndex];		
    		while(*timezonesPointer != NULL) {
    			//printf("t = %s\n", *timezonesPointer);
       			gtk_list_store_append(timezoneModel, &timezoneIter);
			gtk_list_store_set(timezoneModel, &timezoneIter,
					COL_VIEW_ID, idx,
					COL_VIEW_NAME, *timezonesPointer,
					-1);
			timezonesPointer++;
			idx++;
		}
		// select first from the list by default
		gtk_combo_box_set_active(self.timezone, 0);
	}	
}

/*
* Method for finding timezone area index by using timezone name as a key.
* Method could be optimized by only searching from "Other" area and
* if not found from there then comparing the timezone area names with the start part of the
* timezone area name. But this method is not done very often and this way it was easier to implement
* for me so...
*
* If area is not found -1 is returned.
*/
int getTimezoneAreaIndexByTimezoneName(gchar *tzNameParam)
{
	int		retVal;
	gchar		**timezoneAreasPointer;
	guint		idx;
	gboolean	curFound;

	retVal		= -1;
	curFound	= FALSE;
	idx		= 0;
	printf("getTimezoneAreaIndexByTimezoneName(), searching val = %s\n", tzNameParam);
	while(timezoneNameArray[idx] != NULL) {
		timezoneAreasPointer	= timezoneAreaArray[idx];
    		while(*timezoneAreasPointer != NULL) {
			//printf("t = %s\n", *timezoneAreasPointer);
    			if (strstr(*timezoneAreasPointer, tzNameParam)) {
				retVal		= idx;
    				curFound	= TRUE;
    				break;
    			}
			timezoneAreasPointer++;
		}
		if (curFound == TRUE)
		{
			break;
		}
		idx++;
	}
	printf("getTimezoneAreaIndexByTimezoneName(), retVal = %d\n", retVal);
	return retVal;
}

/**
* If timezone is not found from the timezone area, -1 is returned.
*/
int getTimezoneIndexByTimezoneAreaIndexAndTimezoneName(guint tzAreaIndexParam, gchar *tzNameParam)
{
	int	retVal;
	gchar	**timezoneAreasPointer;
	guint	idx;

	retVal	= -1;
	idx	= 0;
	printf("getTimezoneIndexByTimezoneAreaIndexAndTimezoneName(), searching val = %s\n", tzNameParam);
	if (tzAreaIndexParam >= 0)
	{
    		timezoneAreasPointer	= timezoneAreaArray[tzAreaIndexParam];
    		while(*timezoneAreasPointer != NULL) {
    			//printf("t = %s\n", *timezoneAreasPointer);
    			if (strstr(*timezoneAreasPointer, tzNameParam)) {
    				retVal	= idx;
    				break;
    			}
			timezoneAreasPointer++;
			idx++;
		}
	}
	printf("getTimezoneIndexByTimezoneAreaIndexAndTimezoneName(), retVal = %d\n", retVal);
	return retVal;
}

/* --- gpe-conf interface --- */

GtkWidget *Time_Build_Objects(gboolean nonroot)
{  
	guint idx;
	GList *ntpsrv 		= NULL;
	GList *tzoneAreas	= NULL;
	GList *tzones 		= NULL;
 
	GtkWidget *table;
	GtkObject *adj;
	GtkTooltips *tooltips;
	GtkWidget *notebook, *label, *hbox;
	time_t t;
	struct tm *tsptr;
	struct tm ts;     /* gtk_cal seems to modify the ptr
				returned by localtime, so we duplicate it.. */

	guint gpe_border     = gpe_get_border ();
	guint gpe_boxspacing = gpe_get_boxspacing ();
  
	gchar *fstr = NULL;
	guint mark = 0;
	guint tzoneAreaMark = 0;
  
	tzinfo tzi;
	
	guint		selTimezoneAreaIndx;
	guint		selTimezoneIndx;
	gchar 		**timezoneAreasPointer;	
	GtkListStore 	*timezoneAreaModel;
	GtkTreeIter  	timezoneAreaIter;
	GtkCellRenderer *timezoneAreaRend;
	
	GtkListStore 	*timezoneModel;
	GtkTreeIter  	timezoneIter;
	GtkCellRenderer *timezoneRend;	
  
	nonroot_mode = nonroot;
  
	// get the time and the date.
	time(&t);
	tsptr = localtime(&t);
	ts = *tsptr;
	ts.tm_year+=1900;

	if(ts.tm_year < 2002)
		ts.tm_year=2002;
	isdst = ts.tm_isdst;

	tooltips = gtk_tooltips_new ();
  
	notebook = gtk_notebook_new();
	gtk_container_set_border_width (GTK_CONTAINER (notebook), gpe_border);

	if (!nonroot_mode)
	{
		label = gtk_label_new(_("Time & Date"));
		self.categories =  table = gtk_table_new (6, 3, FALSE);
		gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
		gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
		gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);
		gtk_widget_set_size_request(notebook,140,-1);
	  
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook),self.categories,label);
	  
		g_object_set_data(G_OBJECT(notebook), "tooltips", tooltips);
	  
		gtk_container_set_border_width (GTK_CONTAINER (self.categories), 0);
	
		/* -------------------------------------------------------------------------- */
	
		self.catlabel1 = gtk_label_new (NULL); 
		fstr = g_strdup_printf("%s %s %s","<b>",_("Date"),"</b>");
		gtk_label_set_markup (GTK_LABEL(self.catlabel1),fstr); 
		g_free(fstr);
		gtk_table_attach (GTK_TABLE (table), self.catlabel1, 
				0, 3, 
				0, 1,
				GTK_FILL  | GTK_EXPAND,
				GTK_FILL  | GTK_EXPAND,
				0,
				0);
	
		gtk_misc_set_alignment (GTK_MISC (self.catlabel1), 0.0, 0.9);
	  
		self.cal = gtk_date_combo_new ();
		gtk_entry_set_activates_default(GTK_ENTRY(GTK_DATE_COMBO(self.cal)->entry), TRUE);
		gtk_calendar_select_month (GTK_CALENDAR (GTK_DATE_COMBO(self.cal)->cal), ts.tm_mon, ts.tm_year);
		gtk_calendar_select_day (GTK_CALENDAR (GTK_DATE_COMBO(self.cal)->cal), ts.tm_mday);
		gtk_table_attach (GTK_TABLE (table), self.cal, 
				0, 3, 
				1, 2,
				GTK_FILL, 0,
				3, 0);
	  
		gtk_tooltips_set_tip (tooltips, self.cal, _("Enter current date here or use button to select."), NULL);
	
		/* -------------------------------------------------------------------------- */
	
		self.catlabel2 = gtk_label_new (NULL); 
		fstr = g_strdup_printf("%s %s %s","<b>",_("Time"),"</b>");
		gtk_label_set_markup (GTK_LABEL(self.catlabel2),fstr); 
		g_free(fstr);
		gtk_table_attach (GTK_TABLE (table), self.catlabel2, 
				0, 3, 
				2, 3,
				GTK_FILL,GTK_FILL  | GTK_EXPAND,
				0, 0);
		gtk_misc_set_alignment (GTK_MISC (self.catlabel2), 0.0, 0.9);
	
		self.tsel = gpe_time_sel_new();
		gpe_time_sel_set_time(GPE_TIME_SEL(self.tsel),(guint)ts.tm_hour, (guint)ts.tm_min);
		gtk_table_attach (GTK_TABLE (table), self.tsel, 
				0, 3, 
				3, 4,
				GTK_FILL,
				0,
				3,
				0);
		gtk_entry_set_activates_default(GTK_ENTRY(GPE_TIME_SEL(self.tsel)->hour_spin), TRUE);
		gtk_entry_set_activates_default(GTK_ENTRY(GPE_TIME_SEL(self.tsel)->minute_spin), TRUE);
		/* -------------------------------------------------------------------------- */
	
		self.catlabel3 = gtk_label_new (NULL);
		fstr = g_strdup_printf("%s %s %s","<b>",_("Network"),"</b>");
		gtk_label_set_markup (GTK_LABEL(self.catlabel3),fstr); 
		g_free(fstr);
	  
		gtk_table_attach (GTK_TABLE (table), self.catlabel3, 
				0, 3, 
				4, 5,
				GTK_FILL,
				GTK_FILL  | GTK_EXPAND,
				0,
				0);
		gtk_misc_set_alignment (GTK_MISC (self.catlabel3), 0.0, 0.9);
	
/*	
		self.ntpserver = gtk_combo_box_new_text ();
		for (idx=0; idx<5; idx++) {
			//ntpsrv = g_list_append (ntpsrv, Ntpservers[idx]);
			gtk_combo_box_append_text (GTK_COMBO_BOX (self.ntpserver), Ntpservers[idx]);
		}
*/		
	  	self.ntpserver = gtk_combo_new ();
		for (idx=0; idx<5; idx++) {
			ntpsrv = g_list_append (ntpsrv, Ntpservers[idx]);
		}
		gtk_combo_set_popdown_strings (GTK_COMBO (self.ntpserver), ntpsrv);
		
		gtk_table_attach (GTK_TABLE (table), 
				self.ntpserver, 
				0, 3, 
				5, 6,
				GTK_FILL, 0,
				3, 0);
 
		gtk_tooltips_set_tip (tooltips, self.ntpserver, _("Select the timeserver to use to set the clock."), NULL);
	
		self.internet = gtk_button_new_with_label(_("Get time from network"));
		g_signal_connect (G_OBJECT(self.internet), "clicked",
				  G_CALLBACK(GetInternetTime), NULL);
		gtk_widget_set_sensitive(self.internet,is_network_up());
	  
		gtk_table_attach (GTK_TABLE (table), 
				self.internet, 
				0, 3, 
				6, 7,
				GTK_FILL,
				0,
				gpe_border,
				gpe_border);
		gtk_tooltips_set_tip (tooltips, self.internet, _("If connected to the Internet, you may press this button to set the time on this device using the timeserver above."), NULL);
	} // root_mode
  
	/*------------------------------*/

	label = gtk_label_new(_("Timezone"));
	self.catvbox4 = table = gtk_table_new (8, 3, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), gpe_border);
	gtk_table_set_row_spacings (GTK_TABLE (table), gpe_boxspacing);
	gtk_table_set_col_spacings (GTK_TABLE (table), gpe_boxspacing);
  
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),self.catvbox4,label);
	
	// create TimeZone label
	self.catlabel4 = gtk_label_new (NULL);
	fstr = g_strdup_printf("<b>%s</b>",_("Timezone"));
	gtk_label_set_markup (GTK_LABEL(self.catlabel4),fstr); 
	g_free(fstr);  
	gtk_misc_set_alignment (GTK_MISC (self.catlabel4), 0.0, 0.9);
	gtk_table_attach (GTK_TABLE (table), self.catlabel4, 
			0, 4, 
			0, 1,
			GTK_FILL,GTK_FILL | GTK_EXPAND,
			0,
			0);
			
	// timezone area combobox
	fstr	= g_strdup(get_current_tz());
   	if ((fstr == NULL) || (strlen(fstr) <= 0))
   	{
     		fstr	= g_strdup("UTC");
   	}
	selTimezoneAreaIndx	= getTimezoneAreaIndexByTimezoneName(fstr);
	if (selTimezoneAreaIndx == -1)
	{
		g_free(fstr);
		fstr=g_strdup("Europe/Helsinki");
		selTimezoneAreaIndx	= getTimezoneAreaIndexByTimezoneName(fstr);
	}
	if (selTimezoneAreaIndx == -1)
	{
		selTimezoneAreaIndx	= 0;
		selTimezoneIndx		= 0;
	}
	else
	{
		selTimezoneIndx	= getTimezoneIndexByTimezoneAreaIndexAndTimezoneName(selTimezoneAreaIndx, fstr);
	}
	g_free(fstr);
	
	timezoneAreaModel	= gtk_list_store_new(N_VIEW_COLUMNS,
						G_TYPE_INT,
						G_TYPE_STRING);
	idx	= 0;
	while(timezoneNameArray[idx] != NULL) {
     		gtk_list_store_append(timezoneAreaModel, &timezoneAreaIter);
	       	gtk_list_store_set(timezoneAreaModel, &timezoneAreaIter,
               			COL_VIEW_ID, idx,
                             	COL_VIEW_NAME, timezoneNameArray[idx],
                              -1);
		idx++;
	}
	self.timezoneArea	= gtk_combo_box_new_with_model(GTK_TREE_MODEL(timezoneAreaModel));
	timezoneAreaRend	= gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self.timezoneArea), 
				timezoneAreaRend, 
				TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self.timezoneArea), 
				timezoneAreaRend,
               			"text", COL_VIEW_NAME,
               			NULL);
	gtk_combo_box_set_active(self.timezoneArea, selTimezoneAreaIndx);
	//gtk_combo_set_popdown_strings (GTK_COMBO (self.timezoneArea), tzoneAreas);	
	gtk_tooltips_set_tip(tooltips, self.timezoneArea, _("Select your current timezone area here. The setting applies after the next login."), NULL);
	gtk_table_attach (GTK_TABLE (table), self.timezoneArea, 
			0, 1, 
			1, 2,
			GTK_FILL|GTK_EXPAND, 
			0, gpe_boxspacing,
			0);
	gulong sig = g_signal_connect(G_OBJECT(GTK_COMBO_BOX(self.timezoneArea)),
				 "changed",
				 G_CALLBACK(timezonearea_combo_changed),
				 NULL);
	//g_object_set_data(G_OBJECT(combo), COMBO_KEY_CHANGED_SIGNAL, (gpointer)sig);			
	//gtk_list_select_item(GTK_LIST(GTK_COMBO(self.timezone)->list), tzoneAreaMark);
	
	// timezone combobox
	timezoneModel	= gtk_list_store_new(N_VIEW_COLUMNS,
					G_TYPE_INT,
					G_TYPE_STRING);
	idx			= 0;
	timezoneAreasPointer	= timezoneAreaArray[selTimezoneAreaIndx];
    	while(*timezoneAreasPointer != NULL) {
    		//printf("t = %s\n", *timezoneAreasPointer);
		gtk_list_store_append(timezoneModel, &timezoneIter);
		gtk_list_store_set(timezoneModel, &timezoneIter,
          			COL_VIEW_ID, idx,
				COL_VIEW_NAME, *timezoneAreasPointer,
				-1);
		timezoneAreasPointer++;
		idx++;
	}
	self.timezone	= gtk_combo_box_new_with_model(GTK_TREE_MODEL(timezoneModel));
	timezoneRend	= gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(self.timezone),
				timezoneRend, 
				TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(self.timezone),
				timezoneRend,
				"text", COL_VIEW_NAME,
				NULL);	
	//self.timezone = gtk_combo_box_new_text ();
	//gtk_combo_set_popdown_strings (GTK_COMBO (self.timezone), tzones);
	gtk_tooltips_set_tip (tooltips, self.timezone, _("Select your current timezone here. The setting applies after the next login."), NULL);
	gtk_table_attach (GTK_TABLE (table), self.timezone, 
			1, 2, 
			1, 2,
			GTK_FILL|GTK_EXPAND,
			0,
			gpe_boxspacing,
			0);
	gtk_combo_box_set_active(self.timezone, selTimezoneIndx);

	//gtk_list_select_item(GTK_LIST(GTK_COMBO(self.timezone)->list),mark);

	label = gtk_label_new (NULL);
	gtk_misc_set_alignment(GTK_MISC(label),0.0,0.9);
	fstr = g_strdup_printf("<b>%s</b>",_("Daylight Saving"));
	gtk_label_set_markup (GTK_LABEL(label),fstr); 
	g_free(fstr);
	gtk_table_attach (GTK_TABLE (table), label, 
			0, 3, 
			2, 3,
			GTK_FILL,
			GTK_FILL  | GTK_EXPAND,
			0,
			0);

	/* ---- */  
	self.usedst = gtk_check_button_new_with_label(_("Use daylight saving time"));
	gtk_table_attach (GTK_TABLE (table), self.usedst, 0, 3, 3, 4,
		    	GTK_FILL,0,gpe_boxspacing,0);
  
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.usedst), strlen(tzi.dstname));
	g_signal_connect (G_OBJECT(self.usedst), "toggled",
		      	G_CALLBACK(update_enabled_widgets), NULL);
	gtk_tooltips_set_tip (tooltips, self.usedst, _("Check this box if you want your device to handle daylight saving time for you."), NULL);

	self.offsetlabel = gtk_label_new(_("Offset:"));
	gtk_misc_set_alignment(GTK_MISC(self.offsetlabel),0.0,0.5);
	gtk_table_attach (GTK_TABLE (table), self.offsetlabel, 0, 1, 4, 5,
			GTK_FILL,GTK_FILL,gpe_boxspacing,0);

	hbox = gtk_hbox_new (FALSE, gpe_boxspacing);
	gtk_table_attach (GTK_TABLE (table), hbox, 0, 3, 5, 6,
			GTK_FILL,0,gpe_boxspacing,0);
      
	self.defaultdst = gtk_check_button_new_with_label(_("Default"));
	gtk_table_attach (GTK_TABLE (table), self.defaultdst, 1, 3, 4, 5,
			GTK_FILL,0,gpe_boxspacing,0);
			
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.defaultdst), !(tzi.utcdstofs_h || tzi.utcdstofs_m));
	g_signal_connect (G_OBJECT(self.defaultdst), "toggled",
			G_CALLBACK(update_enabled_widgets), NULL);
	gtk_tooltips_set_tip (tooltips, self.defaultdst, _("Use default DST offset of one hour?"), NULL);

	adj = gtk_adjustment_new(tzi.utcdstofs_h,-12,12,1,6,6);
	self.dsth = gtk_spin_button_new (GTK_ADJUSTMENT(adj),1,0);
	gtk_box_pack_start (GTK_BOX (hbox), self.dsth, FALSE, TRUE, gpe_boxspacing);
	self.dstlh = gtk_label_new(_("Hours"));
	gtk_misc_set_alignment(GTK_MISC(self.dstlh),0.0,0.5);
	gtk_box_pack_start (GTK_BOX (hbox), self.dstlh, FALSE, TRUE, gpe_boxspacing);
	gtk_tooltips_set_tip (tooltips, self.dsth, _("Select daylight saving time offset hours here."), NULL);

	adj = gtk_adjustment_new(tzi.utcdstofs_m,0,55,5,15,15);
	self.dstm = gtk_spin_button_new (GTK_ADJUSTMENT(adj),1,0);
	gtk_box_pack_start (GTK_BOX (hbox), self.dstm, FALSE, TRUE, 0);
	self.dstlm = gtk_label_new(_("Minutes"));
	gtk_misc_set_alignment(GTK_MISC(self.dstlm),0.0,0.5);
	gtk_box_pack_start (GTK_BOX (hbox), self.dstlm, FALSE, TRUE, 0);
	gtk_tooltips_set_tip (tooltips, self.dstm, _("Select daylight saving time offset minutes here."), NULL);
  
	/* -------------------------------------------------------------------------- */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self.usedst),strlen(tzi.dstname));

	update_enabled_widgets(NULL);
	g_signal_connect_after (G_OBJECT (notebook), "switch-page",
				G_CALLBACK (do_tabchange), NULL);
	gtk_widget_show_all(notebook);
  
	return notebook;
}

void Time_Free_Objects()
{
}

void Time_Save()
{
	guint year,month,day, h, m,s;
	struct tm tm;
	time_t t;
	char* par;
	tzinfo tz;
	GtkWidget *dialog;	

  	par	= getSelectedTimezoneText();
	/* set timezone */
	if (par != NULL) {
		printf("timezone to save = %s\n", par);
		suid_exec("STZO", par);
  	}
	if (!nonroot_mode)
	{
		tz = get_tz_info(par);
		year = GTK_DATE_COMBO(self.cal)->year;
		month = GTK_DATE_COMBO(self.cal)->month;
		day = GTK_DATE_COMBO(self.cal)->day;
		gpe_time_sel_get_time(GPE_TIME_SEL(self.tsel),&h,&m);
		s = 0;	 
		tm.tm_mday=day;
		tm.tm_mon=month;
		tm.tm_year=year-1900;
		if (isdst)
		{
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self.defaultdst)))
			{
				tm.tm_hour=(h+24+(tz.utcofs_h+24-1)%24) % 24; // defaults to one hour ahead
				tm.tm_min=(m+60+tz.utcofs_m) % 60;
			}
			else
			{
				tm.tm_hour=(h+24+tz.utcdstofs_h) % 24;
				tm.tm_min=(m+60+tz.utcdstofs_m) % 60;
			}
		}
		else
		{
			tm.tm_hour=(h+24+tz.utcofs_h) % 24;
			tm.tm_min=(m+60+tz.utcofs_m) % 60;
		}
		tm.tm_sec=s;
		tm.tm_isdst = isdst;
		t = timegm(&tm);
		/* set time */ 
		par = malloc(100);
		snprintf(par, 99, "%ld", t);
		suid_exec("STIM", par);  
		free(par);
		usleep(300000);
		system (SCREENSAVER_RESET_CMD);
	}
	if (need_warning)
	{
		dialog = gtk_message_dialog_new (GTK_WINDOW (mainw),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_WARNING,
						 GTK_BUTTONS_OK,
						 _("To make timezone settings take effect, "\
               					"you'll need to log out and log in again."));
						gtk_dialog_run(GTK_DIALOG(dialog));
	}
}

void Time_Restore()
{
	time_t t;
	struct tm *tsptr;
	struct tm ts;     /* gtk_cal seems to modify the ptr
			     returned by localtime, so we duplicate it.. */
	if (!nonroot_mode)
	{
		time(&t);
		tsptr = localtime(&t);
		ts = *tsptr;
		gtk_calendar_select_month(GTK_CALENDAR(GTK_DATE_COMBO(self.cal)->cal),ts.tm_mon,ts.tm_year+1900);
		gtk_calendar_select_day(GTK_CALENDAR(GTK_DATE_COMBO(self.cal)->cal),ts.tm_mday);
		gpe_time_sel_set_time(GPE_TIME_SEL(self.tsel),(guint)ts.tm_hour, (guint)ts.tm_min);
	}
}
