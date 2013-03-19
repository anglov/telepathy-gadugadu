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
#include "protocol.h"


G_DEFINE_TYPE (GaduConnectionManager, gadu_connection_manager, TP_TYPE_BASE_CONNECTION_MANAGER)


static void
gadu_connection_manager_init (GaduConnectionManager *self)
{
}


static void
gadu_connection_manager_constructed (GObject *obj)
{
	GaduConnectionManager *self = GADU_CONNECTION_MANAGER (obj);
	TpBaseConnectionManager *base = TP_BASE_CONNECTION_MANAGER (self);
	TpBaseProtocol *protocol;
	
	void (*constructed) (GObject *) = ((GObjectClass *) gadu_connection_manager_parent_class)->constructed;

	if (constructed != NULL)
		constructed (obj);

	protocol = gadu_protocol_new ();
	tp_base_connection_manager_add_protocol (base, protocol);
	g_object_unref (protocol);
}


static void
gadu_connection_manager_class_init (GaduConnectionManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	TpBaseConnectionManagerClass *base_class = (TpBaseConnectionManagerClass *) klass;
	
	object_class->constructed = gadu_connection_manager_constructed;
	
	base_class->cm_dbus_name = "gadugadu";
}


TpBaseConnectionManager *
gadu_connection_manager_new (void)
{
	return g_object_new (GADU_TYPE_CONNECTION_MANAGER, NULL);
}

