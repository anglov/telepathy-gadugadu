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

#include "connection.h"
#include "connection-aliasing.h"
#include "contact-list.h"

static void
gadu_connection_get_alias_flags (TpSvcConnectionInterfaceAliasing *iface,
				 DBusGMethodInvocation *context)
{
	TpBaseConnection *base = TP_BASE_CONNECTION (iface);
	
	TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);
	
	tp_svc_connection_interface_aliasing_return_from_get_alias_flags (
		context, TP_CONNECTION_ALIAS_FLAG_USER_SET);
}

static void
gadu_connection_request_aliases (TpSvcConnectionInterfaceAliasing *iface,
				 const GArray *contacts,
				 DBusGMethodInvocation *context)
{
	GaduConnection *self = GADU_CONNECTION (iface);
	GaduContactList *contact_list = GADU_CONTACT_LIST (gadu_connection_get_contact_list (self));
	TpBaseConnection *base = TP_BASE_CONNECTION (iface);
	TpHandleRepoIface *contact_handles = NULL;
	GPtrArray *aliases = NULL;
	GError *error = NULL;
	gchar **result = NULL;
	guint i;

	TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

	contact_handles = tp_base_connection_get_handles (base, TP_HANDLE_TYPE_CONTACT);

	if (!tp_handles_are_valid (contact_handles, contacts, FALSE, &error)) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}

	aliases = g_ptr_array_sized_new (contacts->len);

	for (i = 0; i < contacts->len; i++) {
		TpHandle handle = g_array_index (contacts, TpHandle, i);
		gchar *alias = NULL;
		
		alias = gadu_contact_list_get_nickname (contact_list, handle);
		
		g_ptr_array_add (aliases, alias);
	}
	
	result = (gchar **)g_ptr_array_free (aliases, FALSE);
	
	tp_svc_connection_interface_aliasing_return_from_request_aliases (
		context, (const gchar **) result);
		
	g_strfreev (result);
}

static void
gadu_connection_get_aliases (TpSvcConnectionInterfaceAliasing *iface,
				      const GArray *contacts,
				      DBusGMethodInvocation *context)
{
	GaduConnection *self = GADU_CONNECTION (iface);
	GaduContactList *contact_list = GADU_CONTACT_LIST (gadu_connection_get_contact_list (self));
	TpBaseConnection *base = TP_BASE_CONNECTION (iface);
	TpHandleRepoIface *contact_handles = NULL;
	GHashTable *result = NULL;
	GError *error = NULL;
	guint i;

	TP_BASE_CONNECTION_ERROR_IF_NOT_CONNECTED (base, context);

	contact_handles = tp_base_connection_get_handles (base, TP_HANDLE_TYPE_CONTACT);

	if (!tp_handles_are_valid (contact_handles, contacts, FALSE, &error)) {
		dbus_g_method_return_error (context, error);
		g_error_free (error);
		return;
	}
	
	result = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
	
	for (i = 0; i < contacts->len; i++) {
		TpHandle handle = g_array_index (contacts, TpHandle, i);
		gchar *alias = NULL;
		
		alias = gadu_contact_list_get_nickname (contact_list, handle);
		
		g_hash_table_insert (result, GUINT_TO_POINTER (handle), alias);
	}
	
	tp_svc_connection_interface_aliasing_return_from_get_aliases (context, result);
	
	g_hash_table_unref (result);
}

static void
gadu_connection_aliasing_fill_contact_attributes (GObject *obj,
						  const GArray *contacts,
						  GHashTable *attributes_hash)
{
	GaduConnection *self = GADU_CONNECTION (obj);
	GaduContactList *contact_list = GADU_CONTACT_LIST (gadu_connection_get_contact_list (self));
	guint i;

	for (i = 0; i < contacts->len; i++) {
		TpHandle handle = g_array_index (contacts, TpHandle, i);
		GValue *val = tp_g_value_slice_new (G_TYPE_STRING);
		
		g_value_take_string (val, gadu_contact_list_get_nickname (contact_list, handle));
		
		tp_contacts_mixin_set_contact_attribute (attributes_hash,
			handle, TP_TOKEN_CONNECTION_INTERFACE_ALIASING_ALIAS,
			val);
	}
}

void
gadu_connection_aliasing_init (GObject *object)
{
	tp_contacts_mixin_add_contact_attributes_iface (object,
		TP_IFACE_CONNECTION_INTERFACE_ALIASING,
		gadu_connection_aliasing_fill_contact_attributes);
}

void
gadu_connection_aliasing_iface_init (gpointer g_iface, gpointer unused)
{
	TpSvcConnectionInterfaceAliasingClass *klass = g_iface;
	
	#define IMPLEMENT(x) tp_svc_connection_interface_aliasing_implement_##x (\
				klass, gadu_connection_##x)
	
	IMPLEMENT(get_alias_flags);
	IMPLEMENT(request_aliases);
	IMPLEMENT(get_aliases);
	//IMPLEMENT(set_aliases);
	
	#undef IMPLEMENT
}

