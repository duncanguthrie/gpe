/*
 * Copyright (C) 2001, 2002 Philip Blundell <philb@gnu.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <gtk/gtk.h>
#include <time.h>
#include "event-db.h"

extern GList *times;
extern time_t viewtime;

#define VERSION "0.04"

extern void update_current_view (void);

extern guint days_in_month (guint year, guint month);
extern gboolean week_starts_monday;
extern gboolean day_view_combined_times;
extern void set_day_view (void);

extern GdkPixmap *close_pix, *day_pix;
extern GdkBitmap *close_mask, *day_mask;

extern GtkWidget *main_window, *pop_window;

extern GdkFont *yearfont, *datefont;

#define SECONDS_IN_DAY (24*60*60)

/* stuff that could perhaps be configurable */
#define TIMEFMT "%R"
#define MONTHTIMEFMT "%a %d"

#define MON  (1 << 0)
#define TUE  (1 << 1)
#define WED  (1 << 2)
#define THU  (1 << 3)
#define FRI  (1 << 4)
#define SAT  (1 << 5)
#define SUN  (1 << 6)
