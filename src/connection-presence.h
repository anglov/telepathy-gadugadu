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

#ifndef __GADU_CONNECTION_PRESENCE_H
#define __GADU_CONNECTION_PRESENCE_H

#include <telepathy-glib/telepathy-glib.h>

#include "connection.h"

G_BEGIN_DECLS

/* NOTE: Keep in sync with gadu_statuses from connection.c */
typedef enum
{
	GADU_STATUS_OFFLINE = 0,
	GADU_STATUS_AVAILABLE,
	GADU_STATUS_AWAY,
	GADU_STATUS_BUSY,
	GADU_STATUS_HIDDEN,
	GADU_STATUS_UNKNOWN,
	
	GADU_STATUS_LAST
} GaduStatusEnum;

typedef struct
{
	GaduStatusEnum	status;
	gchar	       *message;
} GaduPresence;

GaduPresence *gadu_presence_new (GaduStatusEnum status, const gchar *message);
void gadu_presence_free (GaduPresence *presence);

GaduPresence *gadu_presence_parse (gint status, const gchar *descr);
TpPresenceStatus *gadu_presence_get_tp_presence_status (GaduPresence *presence);

void gadu_connection_presence_init (GObject *object);
void gadu_connection_presence_class_init (GaduConnectionClass *klass);
void gadu_connection_presence_finalize (GObject *object);

void gadu_connection_presence_cache_add (GaduConnection *self,
					TpHandle contact,
					GaduPresence *presence);

G_END_DECLS

#endif /* __GADU_CONNECTION_PRESENCE_H */

