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

#ifndef __GADU_DEBUG_H
#define __GADU_DEBUG_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
	GADU_DEBUG_FLAG_CONNECTION = 0,
	GADU_DEBUG_FLAG_CONTACTS,
	GADU_DEBUG_FLAG_IM,
	GADU_DEBUG_FLAG_MISC,
	GADU_DEBUG_FLAG_PRESENCE,
	
	GADU_DEBUG_FLAG_LAST
} GaduDebugFlags;

void gadu_debug_init (void);
void gadu_debug_finalize (void);

void gadu_debug_set_verbose (gboolean verbose);

void gadu_log (GLogLevelFlags log_level, GaduDebugFlags flag, const gchar *format, ...);

#define gadu_debug_full(flag, format, ...) \
	gadu_log (G_LOG_LEVEL_DEBUG, flag, "%s: " format, \
		G_STRFUNC, ##__VA_ARGS__)
#define gadu_error_full(flag, format, ...) \
	gadu_log (G_LOG_LEVEL_ERROR, flag, "%s: " format, \
		G_STRFUNC, ##__VA_ARGS__)
#define gadu_warn_full(flag, format, ...) \
	gadu_log (G_LOG_LEVEL_WARNING, flag, "%s: " format, \
		G_STRFUNC, ##__VA_ARGS__)

#ifdef DEBUG_FLAG
#define gadu_debug(format, ...) \
	gadu_log (G_LOG_LEVEL_DEBUG, DEBUG_FLAG, "%s: " format, \
		G_STRFUNC, ##__VA_ARGS__)
#define gadu_error(format, ...) \
	gadu_log (G_LOG_LEVEL_ERROR, DEBUG_FLAG, "%s: " format, \
		G_STRFUNC, ##__VA_ARGS__)
#define gadu_warn(format, ...) \
	gadu_log (G_LOG_LEVEL_WARNING, DEBUG_FLAG, "%s: " format, \
		G_STRFUNC, ##__VA_ARGS__)
#else
#define gadu_debug(format, ...) \
	gadu_log (G_LOG_LEVEL_DEBUG, GADU_DEBUG_FLAG_MISC, "%s: " format, \
		G_STRFUNC, ##__VA_ARGS__)
#define gadu_error(format, ...) \
	gadu_log (G_LOG_LEVEL_ERROR, GADU_DEBUG_FLAG_MISC, "%s: " format, \
		G_STRFUNC, ##__VA_ARGS__)
#define gadu_warn(format, ...) \
	gadu_log (G_LOG_LEVEL_WARNING, GADU_DEBUG_FLAG_MISC, "%s: " format, \
		G_STRFUNC, ##__VA_ARGS__)
#endif /* DEBUG_FLAG */

G_END_DECLS

#endif /* __GADU_DEBUG_H */

