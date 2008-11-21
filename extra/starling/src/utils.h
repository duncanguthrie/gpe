/*
 * Copyright (C) 2006 Alberto García Hierro
 *      <skyhusker@handhelds.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef UTILS_H
#define UTILS_H

#include <glib/gtypes.h>

#include <sqlite.h>

gboolean has_db_table (sqlite *db, const gchar *name);

gint sqlite_bind_int (sqlite_vm *vm, gint index, gint value);

/* Escape the string STRING for inclusion in a URI.  */
extern char *uri_escape_string (const char *string);

#endif
