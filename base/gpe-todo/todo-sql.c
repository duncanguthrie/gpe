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
#include <stdio.h>
#include <sqlite.h>

#include "todo.h"

static sqlite *sqliteh;

static char *dname = "/.gpe/tasklist";

static int
list_callback (void *arg, int argc, char **argv, char **names)
{
  if (argc == 2)
    {
      new_list (atoi (argv[0]), g_strdup (argv[1]));
    }
  return 0;
}

int
sql_start (void)
{
  const char *home = getenv ("HOME");
  char *buf;
  char *err;
  size_t len;
  if (home == NULL) 
    home = "";
  len = strlen (home) + strlen (dname);
  buf = g_malloc (len);
  strcpy (buf, home);
  strcat (buf, dname);
  sqliteh = sqlite_open (buf, 0, &err);
  if (sqliteh == NULL)
    {
      fprintf (stderr, "%s\n", err);
      free (err);
      return -1;
    }

  sqlite_exec (sqliteh, "select id,description from gpe_tasklists",
	       list_callback, NULL, NULL);
}

void
sql_close (void)
{
  sqlite_close (sqliteh);
}

void
sql_add_list (int id, const char *title)
{
  char buf[256];
  snprintf (buf, sizeof(buf), 
	    "insert into gpe_tasklists values(%d,'%s')", 
	    id, title);
  sqlite_exec (sqliteh, buf, NULL, NULL, NULL);
}
