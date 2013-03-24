/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Marcin Banasiak <marcin.banasiak@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <telepathy-glib/telepathy-glib.h>

#include "connection-manager.h"
#include "debug.h"

int
main (int argc, char **argv)
{
	gint ret;

	g_type_init ();
	
	gadu_debug_init ();

	if (g_getenv ("GADUGADU_VERBOSE") != NULL)
		gadu_debug_set_verbose (TRUE);

	if (g_getenv ("GADUGADU_PERSIST") != NULL)
		tp_debug_set_persistent (TRUE);

	ret = tp_run_connection_manager ("telepathy-gadugadu",
					 VERSION,
					 gadu_connection_manager_new,
					 argc,
					 argv);
	
	gadu_debug_finalize ();
	
	return ret;
}

