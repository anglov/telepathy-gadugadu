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

#include "debug.h"

/* NOTE: keep in the same order as in GaduDebugFlags */
static GDebugKey keywords[] = {
	{ "connection",	GADU_DEBUG_FLAG_CONNECTION },
	{ "contacts",	GADU_DEBUG_FLAG_CONTACTS },
	{ "im",		GADU_DEBUG_FLAG_IM },
	{ "misc", 	GADU_DEBUG_FLAG_MISC },
	{ "presence",	GADU_DEBUG_FLAG_PRESENCE },
	{ NULL,		GADU_DEBUG_FLAG_LAST }
};

static const gchar *domains_cache[GADU_DEBUG_FLAG_LAST] = { NULL };

static gboolean _verbose = FALSE;

static TpDebugSender *debug_sender = NULL;


void
gadu_debug_init ()
{
	guint i;
	
	for (i = 0; i < GADU_DEBUG_FLAG_LAST; i++) {
		domains_cache[i] = g_strdup_printf ("%s/%s", G_LOG_DOMAIN, keywords[i].key);
	}
	
	debug_sender = tp_debug_sender_dup ();
}

void
gadu_debug_finalize ()
{
	guint i;
	
	for (i = 0; i < GADU_DEBUG_FLAG_LAST; i++) {
		g_free ((gchar *) domains_cache[i]);
	}
	
	g_object_unref (debug_sender);
}

void
gadu_debug_set_verbose (gboolean verbose)
{
	_verbose = verbose;
}

static void
gadu_log_debug_sender (GLogLevelFlags log_level, GaduDebugFlags flag, const gchar *message)
{
	TpDebugSender *debug_sender = NULL;
	GTimeVal now;
	
	debug_sender = tp_debug_sender_dup ();
	
	g_get_current_time (&now);
	
	tp_debug_sender_add_message (debug_sender, &now, domains_cache[flag], log_level, message);
	
	g_object_unref (debug_sender);
}

void
gadu_log (GLogLevelFlags log_level, GaduDebugFlags flag, const gchar *format, ...)
{
	gchar *message;
	va_list args;
	
	va_start (args, format);
	message = g_strdup_vprintf (format, args);
	va_end (args);
	
	gadu_log_debug_sender (log_level, flag, message);
	
	if (log_level & G_LOG_LEVEL_DEBUG || log_level & G_LOG_LEVEL_WARNING) {
		if (_verbose == TRUE) {
			if (log_level & G_LOG_LEVEL_DEBUG) {
				g_print ("DEBUG: %s\n", message);
			} else if (log_level & G_LOG_LEVEL_WARNING) {
				g_print ("WARN: %s\n", message);
			}
		}
	} else if (log_level & G_LOG_LEVEL_ERROR) {
		g_print ("ERROR: %s\n", message);
	} else {
		g_print ("%s\n", message);
	}
	
	g_free (message);
}
