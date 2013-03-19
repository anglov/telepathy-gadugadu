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
#include <dbus/dbus-protocol.h>

#include "connection.h"
#include "im-manager.h"
#include "protocol.h"


G_DEFINE_TYPE (GaduProtocol, gadu_protocol, TP_TYPE_BASE_PROTOCOL)


static TpCMParamSpec gadu_params[] = {
	{
	  "account",
	  DBUS_TYPE_STRING_AS_STRING,
	  G_TYPE_STRING,
	  TP_CONN_MGR_PARAM_FLAG_REQUIRED | TP_CONN_MGR_PARAM_FLAG_REGISTER,
	  NULL,
	  0,
	  tp_cm_param_filter_string_nonempty,
	  NULL
	},
	{
	  "password",
	  DBUS_TYPE_STRING_AS_STRING,
	  G_TYPE_STRING,
	  TP_CONN_MGR_PARAM_FLAG_REQUIRED | TP_CONN_MGR_PARAM_FLAG_REGISTER | TP_CONN_MGR_PARAM_FLAG_SECRET,
	  NULL,
	  0,
	  tp_cm_param_filter_string_nonempty,
	  NULL
	},
	{ NULL, NULL, 0, 0, NULL, 0 }
};


static const TpCMParamSpec *
get_parameters (TpBaseProtocol *self)
{
	return gadu_params;
}


static TpBaseConnection *
new_connection (TpBaseProtocol *protocol,
				GHashTable *asv,
				GError **error)
{
	return gadu_connection_new (tp_asv_get_string (asv, "account"),
								tp_asv_get_string (asv, "password"),
								tp_base_protocol_get_name (protocol));
}


static gchar *
normalize_contact (TpBaseProtocol *protocol,
				   const gchar *contact,
				   GError **error)
{
	return g_strdup (contact);
}


static gchar *
identify_account (TpBaseProtocol *protocol,
				 GHashTable *asv,
				 GError **error)
{
	const gchar *account = tp_asv_get_string (asv, "account");
	
	return g_strdup (account);
}


static void
get_connection_details (TpBaseProtocol *protocol,
						GStrv *connection_interfaces,
						GType **channel_managers,
						gchar **icon_name,
						gchar **english_name,
						gchar **vcard_field)
{
	if (connection_interfaces != NULL) {
		const gchar * const *implemented_interfaces = gadu_connection_get_implemented_interfaces (); 
	
		*connection_interfaces = g_strdupv ((gchar **)implemented_interfaces);
	}
	
	if (channel_managers != NULL) {
		GType types[] = {
			GADU_TYPE_IM_MANAGER,
			G_TYPE_INVALID
		};
		
		*channel_managers = g_memdup (types, sizeof (types));
	}
	
	if (icon_name != NULL) {
		*icon_name = g_strdup ("im-gadugadu");
	}
	
	if (english_name != NULL) {
		*english_name = g_strdup ("Gadu-Gadu");
	}
	
	if (vcard_field != NULL) {
		*vcard_field = g_strdup ("");
	}
}


static GPtrArray *
get_interfaces_array (TpBaseProtocol *self)
{
	GPtrArray *interfaces;
	
	interfaces = TP_BASE_PROTOCOL_CLASS (gadu_protocol_parent_class)->get_interfaces_array (self);

	g_ptr_array_add (interfaces, TP_IFACE_PROTOCOL_INTERFACE_PRESENCE);
	
	return interfaces;
}


static const TpPresenceStatusSpec *
get_presence_statuses (TpBaseProtocol *self)
{
	return gadu_connection_get_statuses ();
}


static void
gadu_protocol_init (GaduProtocol *self)
{
}


static void
gadu_protocol_class_init (GaduProtocolClass *klass)
{
	TpBaseProtocolClass *base_class = (TpBaseProtocolClass *) klass;
	
	base_class->get_parameters = get_parameters;
	base_class->new_connection = new_connection;
	base_class->normalize_contact = normalize_contact;
	base_class->identify_account = identify_account;
	base_class->get_connection_details = get_connection_details;
	base_class->get_interfaces_array = get_interfaces_array;
	base_class->get_statuses = get_presence_statuses;
}


TpBaseProtocol *
gadu_protocol_new (void)
{
	return g_object_new (GADU_TYPE_PROTOCOL,
			     "name", "gadugadu",
			     NULL);
}

