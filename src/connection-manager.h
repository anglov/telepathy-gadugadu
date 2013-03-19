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

#ifndef __GADU_CONNECTION_MANAGER_H
#define __GADU_CONNECTION_MANAGER_H

#include <glib-object.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

#define GADU_TYPE_CONNECTION_MANAGER		(gadu_connection_manager_get_type ())
#define GADU_CONNECTION_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GADU_TYPE_CONNECTION_MANAGER, GaduConnectionManager))
#define GADU_IS_CONNECTION_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GADU_TYPE_CONNECTION_MANAGER))
#define GADU_CONNECTION_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GADU_TYPE_CONNECTION_MANAGER, GaduConnectionManagerClass))
#define GADU_IS_CONNECTION_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GADU_TYPE_CONNECTION_MANAGER))
#define GADU_CONNECTION_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GADU_TYPE_CONNECTION_MANAGER, GaduConnectionManagerClass))


typedef struct _GaduConnectionManager		GaduConnectionManager;
typedef struct _GaduConnectionManagerClass	GaduConnectionManagerClass;

struct _GaduConnectionManager
{
	TpBaseConnectionManager parent;
};

struct _GaduConnectionManagerClass
{
	TpBaseConnectionManagerClass parent_class;
};


GType				 gadu_connection_manager_get_type	(void);
TpBaseConnectionManager		*gadu_connection_manager_new		(void);


G_END_DECLS

#endif /* __GADU_CONNECTION_MANAGER_H */

