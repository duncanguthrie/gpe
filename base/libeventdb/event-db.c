/*
 * Copyright (C) 2002, 2006 Philip Blundell <philb@gnu.org>
 *               2006, Florian Boor <florian@kernelconcepts.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>

#include <sqlite.h>

#include <gpe/errorbox.h>

#include <gpe/event-db.h>

static unsigned long dbversion;
static sqlite *sqliteh;

static GList *recurring_events, *one_shot_events;

static const char *fname = "/.gpe/calendar";

static unsigned long uid;

static const char *schema_str = 
"create table calendar (uid integer NOT NULL, tag text, value text)";
static const char *schema2_str = 
"create table calendar_urn (uid INTEGER PRIMARY KEY)";
static const char *schema_info = 
"create table calendar_dbinfo (version integer NOT NULL)";

extern gboolean convert_old_db (int oldversion, sqlite *);

#ifdef EVENT_DB_USE_MEMCHUNK
GMemChunk *event_chunk, *recur_chunk;
#endif

static gint
event_sort_func (const event_t ev1, const event_t ev2)
{
  if (!ev1->recur || !ev2->recur)
    return ev1->start - ev2->start;
  recur_t r1 = ev1->recur;
  recur_t r2 = ev2->recur;
  return r1->end - r2->end;
}

/* Add an event to the in-memory list */
static gboolean
event_db_add_internal (event_t ev)
{
  if (ev->uid >= uid)
    uid = ev->uid + 1;

  ev->list = ev->recur ? &recurring_events : &one_shot_events;      
  *ev->list = g_list_insert_sorted (*ev->list, ev, (GCompareFunc)event_sort_func); 
      
  return TRUE;
}

/* Here we create an globally unique eventid, which we
 * can use to reference this event in an vcal, etc. */
static gchar *
event_db_make_eventid (void)
{
  static char *hostname;
  static char buffer [512];

  if ((gethostname (buffer, sizeof (buffer) -1) == 0) &&
     (buffer [0] != 0))
    hostname = buffer;
  else
    hostname = "localhost";

  return g_strdup_printf ("%lu.%lu@%s",
                         (unsigned long) time (NULL),
                         (unsigned long) getpid(),
                         hostname); 
}

/* Remove an event from the in-memory list */
static gboolean
event_db_remove_internal (event_t ev)
{
  if (ev->recur)
    recurring_events = g_list_remove (recurring_events, ev);
  else
    one_shot_events = g_list_remove (one_shot_events, ev);

  return TRUE;
}

static gboolean
parse_date (char *s, time_t *t, gboolean *date_only)
{
  struct tm tm;

  char *p;
  memset (&tm, 0, sizeof (tm));
  p = strptime (s, "%Y-%m-%d", &tm);
  if (p == NULL)
    {
      fprintf (stderr, "Unable to parse date: %s\n", s);
      return FALSE;
    }

  p = strptime (p, " %H:%M", &tm);

  if (date_only)
    *date_only = (p == NULL) ? TRUE : FALSE;

  *t = timegm (&tm);
  return TRUE;
}

static int
load_data_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      event_t ev = arg;
     
      if (!strcasecmp (argv[0], "start"))
	{
	  gboolean untimed;

	  parse_date (argv[1], &ev->start, &untimed);

	  if (untimed)
	    {
	      ev->flags |= FLAG_UNTIMED;
	      ev->start += 12 * 60 * 60;
	    }
	}
      else if (!strcasecmp (argv[0], "eventid"))
	{
	  ev->eventid = g_strdup (argv[1]);
	}
      else if (!strcasecmp (argv[0], "rend"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  parse_date (argv[1], &r->end, NULL);
	}
      else if (!strcasecmp (argv[0], "rcount"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  r->count = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "rincrement"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  r->increment = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "rdaymask"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  r->daymask = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "rexceptions"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  long rmtime = (long)atoi (argv[1]);
	  r->exceptions = g_slist_append(r->exceptions,	(void *)rmtime);
	}
      else if (!strcasecmp (argv[0], "recur"))
	{
	  recur_t r = event_db_get_recurrence (ev);
	  r->type = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "duration"))
	{
	  ev->duration = atoi (argv[1]);
	}
      else if (!strcasecmp (argv[0], "alarm"))
	{
	  ev->alarm = atoi (argv[1]);
	  ev->flags |= FLAG_ALARM;
	}
    }

  return 0;
}

static int
load_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 1)
    {
      char *err;
      guint uid = atoi (argv[0]);
      event_t ev = event_db__alloc_event ();
      ev->uid = uid;
      if (sqlite_exec_printf (sqliteh, "select tag,value from calendar where uid=%d", 
			      load_data_callback, ev, &err, uid))
	{
	  gpe_error_box (err);
	  free (err);
	  event_db__free_event (ev);
	  return 1;
	}

      if (ev->recur && ev->recur->type == RECUR_NONE)
	{
	  /* Old versions of gpe-calendar dumped out a load of recurrence tags
	     even for a one-shot event.  */
	  event_db__free_recur (ev->recur);
	  ev->recur = NULL;
	}

      if (event_db_add_internal (ev) == FALSE)
	return 1;
    }
  return 0;
}

static int
dbinfo_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 1)
    {
      dbversion = atoi (argv[0]);
    }

  return 0;
}

/**
 * event_db_start:
 *
 * Initialise the event database for use. Needs to be called on application
 * start before acessing events.
 *
 * Returns: #TRUE on success, #FALSE otherwise.
 */
gboolean
event_db_start (void)
{
  const char *home = g_get_home_dir ();
  char *buf;
  char *err;
  size_t len;

#ifdef EVENT_DB_USE_MEMCHUNK
  event_chunk = g_mem_chunk_new ("event", sizeof (struct event_s), 4096, G_ALLOC_AND_FREE);
  recur_chunk = g_mem_chunk_new ("recur", sizeof (struct recur_s), 4096, G_ALLOC_AND_FREE);
#endif

  len = strlen (home) + strlen (fname) + 1;
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, fname);
  sqliteh = sqlite_open (buf, 0, &err);
  g_free (buf);
  if (sqliteh == NULL)
    {
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  sqlite_exec (sqliteh, schema_info, NULL, NULL, &err);
  
  if (sqlite_exec (sqliteh, "select version from calendar_dbinfo", dbinfo_callback, NULL, &err))
    {
      dbversion=0;
      gpe_error_box (err);
      free (err);
      return FALSE;
    }

  sqlite_exec (sqliteh, schema_str, NULL, NULL, &err);
  sqlite_exec (sqliteh, schema2_str, NULL, NULL, &err);
      
  if (dbversion==1) 
    {
      if (sqlite_exec (sqliteh, "select uid from calendar_urn", load_callback, NULL, &err))
        {
          gpe_error_box (err);
          free (err);
          return FALSE;
        }
    }
    
  else 
    {
      convert_old_db (dbversion, sqliteh);
      dbversion=1;
    }
    
  return TRUE;
}

static int
load_details_callback (void *arg, int argc, char *argv[], char **names)
{
  if (argc == 2)
    {
      event_details_t evd = arg;
      if (!strcasecmp (argv[0], "summary") && !evd->summary)
        evd->summary = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "description") && !evd->description)
        evd->description = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "location") && !evd->location)
        evd->location = g_strdup (argv[1]);
      else if (!strcasecmp (argv[0], "modified"))
        {
          if (strchr (argv[1], '-'))
            parse_date (argv[1], &evd->modified, NULL);
          else
            evd->modified = strtoul (argv[1], NULL, 10);
        }
      else if (!strcasecmp (argv[0], "sequence"))
        evd->sequence = atoi (argv[1]);
      else if (!strcasecmp (argv[0], "category"))
        evd->categories = g_slist_prepend (evd->categories, (gpointer)atoi (argv[1]));
    }
  return 0;
}

/**
 * event_db_get_details:
 * @ev: Event to get details for.
 * 
 * Retrieves detailed information for a given event and increments the 
 * reference counter of the #event_details_t structure which will be 
 * attached to the event. If the event details are already
 * there they will not be loaded again but the reference counter is increased. 
 * If you mant to make sure the event details are updated you need to use 
 * event_db_forget_details() first. 
 * 
 *
 * Returns: The #event_details_t event detail data attached to the event.
 */
event_details_t
event_db_get_details (event_t ev)
{
  char *err;
  event_details_t evd;

  if (ev->details)
    {
      evd = ev->details;
    }
  else
    {
      evd = g_malloc0 (sizeof (struct event_details_s));

      if (sqlite_exec_printf (sqliteh, "select tag,value from calendar where uid=%d",
			      load_details_callback, evd, &err, ev->uid))
        {
          gpe_error_box (err);
          free (err);
          g_free (evd);
          return NULL;
        }
      
      ev->details = evd;
    }

  evd->ref++;

  return evd;
}

/**
 * event_db_forget_details:
 * @ev: Event to get details for.
 * 
 * Decrease the reference counter of the event detail information in an #event_t. 
 * If the reference counter is #0 the detail information is freed.
 */
void
event_db_forget_details (event_t ev)
{
  if (ev->details)
    {
      if (ev->details->ref == 0)
        fprintf (stderr, "details refcount was already zero!\n");
      else
        ev->details->ref--;

      if (ev->details->ref == 0)
        {
          event_details_t evd = ev->details;
          if (evd->description)
            g_free (evd->description);
          if (evd->location)
            g_free (evd->location);
          if (evd->summary)
            g_free (evd->summary);
          g_slist_free (evd->categories);
          g_free (evd);
	    }
      ev->details = NULL;
    }
}

/**
 * event_db_stop:
 *
 * Closes the event database and frees all remaining events in memory.
 * 
 * Returns: #TRUE on success, #FALSE otherwise.
 */
gboolean
event_db_stop (void)
{
  GList *iter;

  for (iter = one_shot_events; iter; iter = g_list_next (iter))
    event_db_destroy (iter->data);
  g_list_free(one_shot_events);
  one_shot_events = NULL;

  for (iter = recurring_events; iter; iter = g_list_next (iter))
    event_db_destroy (iter->data);
  g_list_free (recurring_events);
  recurring_events = NULL;
	
  sqlite_close (sqliteh);
  return TRUE;
}

event_t
event_db_find_by_uid (guint uid)
{
  GList *iter;
    
  for (iter = one_shot_events; iter; iter = g_list_next (iter))
    {
      event_t ev = iter->data;
      
      if (ev->uid == uid) 
	return ev;
    }

  for (iter = recurring_events; iter; iter = g_list_next (iter))
    {
      event_t ev = iter->data;
       
      if (ev->uid == uid) 
	return ev;
    }

  return NULL;
}
/*link_warning(event_db_find_by_uid, 
	     "warning: event_db_find_by_uid is obsolescent: use the ev->cloned_ev pointer directly.");
*/

/**
 * event_db_list_destroy:
 * @l: Event list
 *
 * This function frees a given list of events including the list itself. 
 */
void
event_db_list_destroy (GSList *l)
{
  GSList *iter;
    
  for (iter = l; iter; iter = g_slist_next (iter))
    {
      event_t ev = iter->data;
      event_db__free_event (ev);
    }
	  
  g_slist_free (l);
}

/**
 * event_db_clone:
 * @ev: Event to clone.
 * 
 * Clones a given event. The new event is marked to be a clone instead of 
 * the original event by setting the #FLAG_CLONE flag.
 *
 * Returns: The event clone.
 */
event_t
event_db_clone (event_t ev)
{
  event_t n = event_db__alloc_event ();
  memcpy (n, ev, sizeof (struct event_s));
  n->cloned_ev = (void *)ev;
  n->flags |= FLAG_CLONE;
  return n;
}

static GSList *
event_db_list_for_period_internal (time_t start, time_t end, gboolean untimed, 
				   gboolean untimed_significant, gboolean alarms, 
				   guint max)
{
  GList *iter;
  GSList *iter2;
  GSList *list = NULL;
  struct tm tm_event, tm_display;
  long int month_diff;
  int events = 0;
	  
  if (end == 0)		/* ??? pb */
    {
      struct tm tm_current;

      localtime_r (&start, &tm_current);
  
      tm_current.tm_year+=1;
      end = mktime (&tm_current);
    }
  
  for (iter = one_shot_events; iter; iter = g_list_next (iter))
    {
      event_t ev = iter->data, clone;
      time_t event_start = ev->start;
      
      if (untimed_significant)
	{
	  /* Ignore events with wrong untimed status */
	  if (untimed != ((ev->flags & FLAG_UNTIMED) != 0))
	    continue;
	}

      /* Stop if event hasn't started yet */
      if (end && event_start > end)
	break;
      
      /* Skip events without alarms when applicable */
      if (alarms && !(ev->flags & FLAG_ALARM))
	continue;

      /* Skip events that have finished already */
      if ((event_start + ev->duration < start)
	  || (ev->duration && ((event_start + ev->duration == start))))
	continue;

      if (alarms) event_start-=ev->alarm;
      
      clone = event_db_clone(ev);
      clone->start = event_start;
      list = g_slist_insert_sorted (list, clone, (GCompareFunc)event_sort_func);
      if (++events == max)
	return list;
    }
  
  for (iter = recurring_events; iter; iter = g_list_next (iter))
    {
      event_t ev = iter->data, clone;
      time_t event_start = ev->start, clone_start;
      recur_t r = ev->recur;
      time_t window_start = start;
      
      if (untimed_significant)
	{
	  /* Ignore events with wrong untimed status */
	  if (untimed != ((ev->flags & FLAG_UNTIMED) != 0))
	    continue;
	}

      /* Stop if all remaining events are finished already */
      if (r->end && start > r->end)
	break;

      /* Skip events that haven't started yet */
      if (end && event_start > end)
	continue;
      
      /* Skip events without alarms when applicable */
      if (alarms && !(ev->flags & FLAG_ALARM))
	continue;

      localtime_r (&event_start, &tm_event);

      switch (r->type)
	{
	case RECUR_NONE:
	  abort ();
	  
	case RECUR_DAILY:
	  while (window_start < end)
	    {
	      localtime_r (&window_start, &tm_display);

	      if ((tm_event.tm_yday - tm_display.tm_yday) % r->increment ==0) 
		{
		  tm_event.tm_mon = tm_display.tm_mon;
		  tm_event.tm_year = tm_display.tm_year;
		  tm_event.tm_mday = tm_display.tm_mday;
		}

	      clone_start = mktime (&tm_event);

	      if (alarms) 
		clone_start -= ev->alarm;

	      if (clone_start <= end && (clone_start + ev->duration) > window_start)
		{
		  gboolean skip=FALSE;

		  if (alarms) 
		    clone_start += ev->alarm;

		  if (r->exceptions)
		    { 
		      for (iter2 = r->exceptions; iter2; iter2 = g_slist_next (iter2))
			{
			  if ((long)iter2->data == (long)clone_start) 
			    skip = TRUE;
			}
		    }

		  if (alarms && window_start>clone_start-ev->alarm) 
		    skip = TRUE;

		  if (!skip)
		    {
		      clone = event_db_clone (ev);

		      if (alarms) 
			clone->start -= ev->alarm;
		      else 
			clone->start = clone_start;
		      clone->flags |= FLAG_RECUR;
		      list = g_slist_insert_sorted (list, clone, (GCompareFunc)event_sort_func);
		      if (++events == max)
			return list;
		    }
		}

	      window_start += SECONDS_IN_DAY;
	    }
	  break;

	case RECUR_WEEKLY:
	  while (window_start < end)
	    {
	      localtime_r (&window_start, &tm_display);
	      if (((r->daymask & MON && tm_display.tm_wday==1) ||
		   (r->daymask & TUE && tm_display.tm_wday==2) ||
		   (r->daymask & WED && tm_display.tm_wday==3) ||
		   (r->daymask & THU && tm_display.tm_wday==4) ||
		   (r->daymask & FRI && tm_display.tm_wday==5) ||
		   (r->daymask & SAT && tm_display.tm_wday==6) ||
		   (r->daymask & SUN && tm_display.tm_wday==0))) 
		{
		  tm_event.tm_mon=tm_display.tm_mon;
		  tm_event.tm_year=tm_display.tm_year;
		  tm_event.tm_mday=tm_display.tm_mday;
		  clone_start=mktime(&tm_event);
		  if (alarms) clone_start-=ev->alarm;
		  
		  if (clone_start <= end && (clone_start + ev->duration) > window_start)
		    {
		      gboolean skip=FALSE;
		      if (alarms) clone_start+=ev->alarm;
		      if (r->exceptions) 
			for (iter2 = r->exceptions; iter2; iter2 = g_slist_next (iter2))
			  if ((long)iter2->data == (long)clone_start) skip=TRUE;

		      if (alarms && window_start > clone_start-ev->alarm) 
			skip = TRUE;

		      if (!skip)
			{
			  clone = event_db_clone(ev);
			  if (alarms) clone->start = clone_start-ev->alarm;
			  else clone->start = clone_start;
			  clone->flags |= FLAG_RECUR;
			  list = g_slist_insert_sorted (list, clone, (GCompareFunc)event_sort_func);
			  if (++events == max)
			    return list;
			}
		    }
		}
	      
	      window_start += 7 * SECONDS_IN_DAY;
	    }
	  break;

	case RECUR_MONTHLY:
	  while (window_start < end)
	    {
	      localtime_r (&window_start, &tm_display);
	      month_diff=tm_display.tm_mon-tm_event.tm_mon +
		(tm_display.tm_year-tm_event.tm_year)*12;
	      if ((tm_display.tm_mday==tm_event.tm_mday) &&
		  (month_diff%r->increment)==0) 
		{
		  tm_event.tm_mon=tm_display.tm_mon;
		  tm_event.tm_year=tm_display.tm_year;
		  tm_event.tm_mday=tm_display.tm_mday;
		}; 
	      clone_start=mktime(&tm_event);
	      if (alarms) clone_start-=ev->alarm;
	      if (clone_start <= end && (clone_start + ev->duration) > window_start) 
		{
		  gboolean skip=FALSE;
		  if (alarms) clone_start+=ev->alarm;
		  if (r->exceptions) 
		    for (iter2 = r->exceptions; iter2; iter2 = g_slist_next (iter2))
		      if ((long)iter2->data == (long)clone_start) skip=TRUE;
		  if(alarms && window_start>clone_start-ev->alarm) skip=TRUE;
		  if (!skip)
		    {
		      clone = event_db_clone(ev);
		      if (alarms) clone->start = clone_start-ev->alarm;
		      else clone->start = clone_start;
		      clone->flags |= FLAG_RECUR;
		      list = g_slist_insert_sorted (list, clone, (GCompareFunc)event_sort_func);
		      if (++events == max)
			return list;
		    }
		}
	      
	      tm_display.tm_mon++;
	      if (tm_display.tm_mon > 11)
		{
		  tm_display.tm_mon = 0;
		  tm_display.tm_year++;
		}
	      window_start = mktime (&tm_display);
	    }
	  break;

	case RECUR_YEARLY:
	  while (window_start < end)
	    {
	      localtime_r (&window_start, &tm_display);
	      if ((tm_display.tm_mon==tm_event.tm_mon) &&
		  (tm_display.tm_mday==tm_event.tm_mday) &&
		  ((tm_display.tm_year-tm_event.tm_year)%r->increment)==0) 
		{
		  tm_event.tm_mon=tm_display.tm_mon;
		  tm_event.tm_year=tm_display.tm_year;
		  tm_event.tm_mday=tm_display.tm_mday;
		}; 
	      clone_start=mktime(&tm_event);
	      if (alarms) clone_start-=ev->alarm;
	      if (clone_start <= end && (clone_start + ev->duration) > window_start)
		{
		  gboolean skip=FALSE;
		  if (alarms) clone_start+=ev->alarm;
		  if (r->exceptions)
		    {
		      for (iter2 = r->exceptions; iter2; iter2 = g_slist_next (iter2))
			if ((long)iter2->data == (long)clone_start) skip=TRUE;
		    }
		  if(alarms && window_start > clone_start-ev->alarm) 
		    skip = TRUE;
		  if (!skip)
		    {
		      clone = event_db_clone(ev);
		      if (alarms) clone->start = clone_start-ev->alarm;
		      else clone->start = clone_start;
		      clone->flags |= FLAG_RECUR;
		      list = g_slist_insert_sorted (list, clone, (GCompareFunc)event_sort_func);
		      if (++events == max)
			return list;
		    }
		}

	      tm_display.tm_year++;
	      window_start = mktime (&tm_display);
	    }
	  break;
  	}
    }

  return list;
}

/**
 * event_db_list_for_period:
 * @start: Start time
 * @end: End time
 * 
 * Create a list of all events for a given period.
 *
 * Returns: A list of events.
 */
GSList *
event_db_list_for_period (time_t start, time_t end)
{
  return event_db_list_for_period_internal (start, end, FALSE, FALSE, FALSE, 0);
}

/**
 * event_db_list_alarms_for_period:
 * @start: Start time
 * @end: End time
 * 
 * Create a list of all alarm events for a given period.
 *
 * Returns: A list of events.
 */
GSList *
event_db_list_alarms_for_period (time_t start, time_t end)
{
  GSList *return_list = NULL;
  time_t shifted_end;
  struct tm tm_shift;
  
  localtime_r (&start, &tm_shift);
  tm_shift.tm_mday++;
  tm_shift.tm_hour=0;
  tm_shift.tm_min=0;
  tm_shift.tm_sec=1;
  shifted_end=mktime(&tm_shift);
  return_list=event_db_list_for_period_internal (start, shifted_end, FALSE, FALSE, TRUE, 0);
  start=shifted_end;
   
  while (!return_list && start<end)
  {
    return_list=event_db_list_for_period_internal (start, start+SECONDS_IN_DAY, FALSE, FALSE, TRUE, 0);
    start+=SECONDS_IN_DAY;
  } 
  return(return_list);
}

/**
 * event_db_untimed_list_for_period:
 * @start: Start time
 * @end: End time
 * @yes:
 * 
 * Create a list of all untimed events for a given period.
 *
 * Returns: A list of events.
 */
GSList *
event_db_untimed_list_for_period (time_t start, time_t end, gboolean yes)
{
  return event_db_list_for_period_internal (start, end, yes, TRUE, FALSE, 0);
}

/**
 * event_db_list_for_future:
 * @start: Start time
 * @max: Maximum number of events to retrieve.
 * 
 * Get a list of all future events starting at a given time. The total amount 
 * of events may be limited by the @max parameter. Set this to 0 for an 
 * unlimited amount of events.
 *
 * Returns: A list of events.
 */
GSList *
event_db_list_for_future (time_t start, guint max)
{
  int NUMBER_OF_DAYS=14;
  int day_inc;
  GSList *return_list = NULL;
  time_t shifted_end;
  struct tm tm_shift;
  
  localtime_r (&start, &tm_shift);
  tm_shift.tm_mday++;
  tm_shift.tm_hour=0;
  tm_shift.tm_min=0;
  tm_shift.tm_sec=1;
  shifted_end=mktime(&tm_shift);
  return_list=event_db_list_for_period_internal (start, shifted_end, FALSE, FALSE, TRUE, 0);
  start=shifted_end;
   
  for (day_inc=0;day_inc<NUMBER_OF_DAYS;day_inc++) 
    {
      return_list = g_slist_concat (return_list, event_db_list_for_period_internal (start,
										    start+SECONDS_IN_DAY, FALSE, FALSE, FALSE, max));
      start+=SECONDS_IN_DAY;
    }
  
  return return_list;
}

#define insert_values(db, id, key, format, value)	\
	sqlite_exec_printf (db, "insert into calendar values (%d, '%q', '" format "')", \
			    NULL, NULL, err, id, key, value)

/* Dump out an event to the SQL database */
static gboolean
event_db_write (event_t ev, char **err)
{
  time_t modified;
  char buf_start[64], buf_end[64];
  struct tm tm;
  event_details_t ev_d = event_db_get_details (ev);
  gboolean rc = FALSE;
  GSList *iter;
  GList **list;

  gmtime_r (&ev->start, &tm);
  strftime (buf_start, sizeof (buf_start), 
	    (ev->flags & FLAG_UNTIMED) ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
	    &tm);  

  modified = time (NULL);

  if (!ev->eventid)
    {
      ev->eventid = event_db_make_eventid();
#ifdef DEBUG        
      printf ("Made new eventid: %s\n", ev->eventid);
#endif
    }
  
  if ((ev_d->summary && insert_values (sqliteh, ev->uid, "summary", "%q", ev_d->summary))
      || (ev_d->description && insert_values (sqliteh, ev->uid, "description", "%q", ev_d->description))
      || (ev_d->location && insert_values (sqliteh, ev->uid, "location", "%q", ev_d->location))
      || insert_values (sqliteh, ev->uid, "duration", "%d", ev->duration)
      || insert_values (sqliteh, ev->uid, "modified", "%lu", modified)
      || insert_values (sqliteh, ev->uid, "start", "%q", buf_start)
      || insert_values (sqliteh, ev->uid, "sequence", "%d", ev_d->sequence)
      || insert_values (sqliteh, ev->uid, "eventid", "%q", ev->eventid))
    goto exit;

  for (iter = ev_d->categories; iter; iter = iter->next)
    {
      if (insert_values (sqliteh, ev->uid, "category", "%d", (int)iter->data))
        goto exit;
    }

  if (ev->recur)
    {
      recur_t r = ev->recur;

      if (insert_values (sqliteh, ev->uid, "recur", "%d", r->type)
	  || insert_values (sqliteh, ev->uid, "rcount", "%d", r->count)
	  || insert_values (sqliteh, ev->uid, "rincrement", "%d", r->increment)
	  || insert_values (sqliteh, ev->uid, "rdaymask", "%d", r->daymask))
        goto exit;

      if (ev->recur->exceptions)
	{
	  GSList *iter;
	  for (iter = ev->recur->exceptions; iter; iter = g_slist_next (iter))
	    if (insert_values (sqliteh, ev->uid, "rexceptions", "%d",
			       (long) iter->data)) goto exit;
	}
      	      
      if (r->end != 0)
        {
          gmtime_r (&r->end, &tm);
          strftime (buf_end, sizeof (buf_end), 
                (ev->flags & FLAG_UNTIMED) ? "%Y-%m-%d" : "%Y-%m-%d %H:%M",
                &tm); 
          if (insert_values (sqliteh, ev->uid, "rend", "%q", buf_end)) 
            goto exit;
        }
    }

  if (ev->flags & FLAG_ALARM)
    {
      if (insert_values (sqliteh, ev->uid, "alarm", "%d", ev->alarm))
       goto exit;
    }

  rc = TRUE;

  /* update internal list */
  list = ev->recur ? &recurring_events : &one_shot_events;
  if (list != ev->list)
    {
      *ev->list = g_list_remove (*ev->list, ev);
      *list = g_list_insert_sorted (*list, ev, (GCompareFunc)event_sort_func);
      ev->list = list;
    }
  else
    {
      *ev->list = g_list_sort (*ev->list, (GCompareFunc)event_sort_func);
    }

exit:
  event_db_forget_details (ev);
  return rc;
}

/**
 * event_db_flush:
 * @ev: Event
 * 
 * Remove an event from the database.
 *
 * Returns: #TRUE on success, #FALSE otherwise.
 */
gboolean
event_db_flush (event_t ev)
{
  char *err;

  if (sqlite_exec (sqliteh, "begin transaction", NULL, NULL, &err))
    goto error;

  if (sqlite_exec_printf (sqliteh, "delete from calendar where uid=%d", NULL, NULL, &err,
			  ev->uid))
    goto error;

  if (event_db_write (ev, &err) == FALSE
      || sqlite_exec (sqliteh, "commit transaction", NULL, NULL, &err))
    goto error_and_rollback;

  return TRUE;

 error_and_rollback:
  sqlite_exec (sqliteh, "rollback transaction", NULL, NULL, NULL);
 error:
  gpe_error_box (err);
  free (err);
  return FALSE;
}

/**
 * event_db_add:
 * @ev: Event
 * 
 * Add an event to both the in-memory list and the SQL database. This makes 
 * sure the event is accessible by all application including the application
 * which adds the event instantly.
 *
 * Returns: #TRUE on success, #FALSE otherwise.
 */
gboolean
event_db_add (event_t ev)
{
  gchar *err = NULL;
  gboolean rollback = FALSE;

  if (sqlite_exec (sqliteh, "begin transaction", NULL, NULL, &err))
    goto error;

  rollback = TRUE;

  if (sqlite_exec (sqliteh, "insert into calendar_urn values (NULL)",
		   NULL, NULL, &err))
    goto error;

  ev->uid = sqlite_last_insert_rowid (sqliteh);
  ev->eventid = event_db_make_eventid();

  if (event_db_add_internal (ev) == FALSE)
    {
      err = g_strdup ("Could not insert event");
      goto error;
    }

  if (event_db_write (ev, &err) == FALSE
      || sqlite_exec (sqliteh, "commit transaction", NULL, NULL, &err))
    goto error;

  return TRUE;

 error:
  if (rollback)
    sqlite_exec (sqliteh, "rollback transaction", NULL, NULL, NULL);
  gpe_error_box (err);
  g_free (err);
  return FALSE;
}

/**
 * event_db_remove:
 * @ev: Event
 * 
 * This function removes an event from the event database. 
 * 
 * Returns: #TRUE on success, #FALSE otherwise.
 */
gboolean
event_db_remove (event_t ev)
{
  if (event_db_remove_internal (ev) == FALSE)
    return FALSE;

  sqlite_exec_printf (sqliteh, "delete from calendar where uid=%d", 
		      NULL, NULL, NULL, ev->uid);

  sqlite_exec_printf (sqliteh, "delete from calendar_urn where uid=%d", 
		      NULL, NULL, NULL, ev->uid);

  return TRUE;
}

/**
 * event_db_new:
 *
 * Create and initialise a new #event_t event structure.
 *
 * Returns: A new event.
 */
event_t
event_db_new (void)
{
  event_t ev = event_db__alloc_event ();
  return ev;
}

/**
 * event_db_destroy:
 * @ev: Event
 * 
 * Free a given event.
 */
void
event_db_destroy (event_t ev)
{
  event_db_forget_details (ev);

  if (ev->recur)
    event_db__free_recur (ev->recur);
  if (ev->eventid)
    g_free (ev->eventid);

  event_db__free_event (ev);
}

/**
 * event_db_alloc_details:
 * @ev: Event
 * 
 * Allocates a new, clean #event_details_t structure which is attached to
 * the given event.
 *
 * Returns: The new details structure.
 */
event_details_t
event_db_alloc_details (event_t ev)
{
  ev->details = (event_details_t) g_malloc0 (sizeof (struct event_details_s));
  ev->details->ref++;
  return ev->details;
}

/**
 * event_db_get_recurrence:
 * @ev: Event
 * 
 * Retrieves recurrence information for an event.
 *
 * Returns: Recurrence information for the given event.
 */
recur_t
event_db_get_recurrence (event_t ev)
{
  if (ev->recur == NULL)
    {
      ev->recur = event_db__alloc_recur ();

      ev->recur->type = RECUR_NONE;
    }

  return ev->recur;
}

/**
 * event_db_clear_recurrence:
 * @ev: Event
 * 
 * Frees the recurrence information of a given event.
 */
void
event_db_clear_recurrence (event_t ev)
{
  if (ev->recur)
    {
      event_db__free_recur (ev->recur);
      ev->recur = NULL;
    }
}

/**
 * event_db_refresh:
 *
 * Refresh event information from database on disk.
 *
 * Returns: #TRUE on success, #FALSE otherwise.
 */
gboolean
event_db_refresh (void)
{
  event_db_stop();

  return event_db_start();
}
