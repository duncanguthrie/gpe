/* GPE Mileage
 * Copyright (C) 2005  Rene Wagner <rw@handhelds.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __GPE_MILEAGE_PREFERENCES_H__
#define __GPE_MILEAGE_PREFERENCES_H__

#include "units.h"

enum _PreferencesDomain
{
  PREFERENCES_DOMAIN_UNITS
};
typedef enum _PreferencesDomain PreferencesDomain;

typedef void (*PreferencesNotifyFunc) (PreferencesDomain, gpointer);

gint preferences_init ();
gint preferences_shutdown ();

gint preferences_get_unit (UnitType);
gboolean preferences_set_unit (UnitType, gint);

void preferences_notify_add (PreferencesDomain, PreferencesNotifyFunc, gpointer);

#endif /* _GPE_MILEAGE_PREFERENCES_H__ */