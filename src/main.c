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

#include <telepathy-glib/telepathy-glib.h>

#include "connection-manager.h"

#define VERSION "0.0.1"

int
main (int argc, char **argv)
{
	if (g_getenv ("GADUGADU_TIMING") != NULL)
		g_log_set_default_handler (tp_debug_timestamped_log_handler, NULL);

	if (g_getenv ("GADUGADU_PERSIST") != NULL)
		tp_debug_set_persistent (TRUE);

	return tp_run_connection_manager ("telepathy-gadugadu",
					  VERSION,
					  gadu_connection_manager_new,
					  argc,
					  argv);
}

