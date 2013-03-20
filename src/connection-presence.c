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
#include <libgadu.h>

#include "connection.h"
#include "connection-presence.h"

struct _GaduConnectionPresencePrivate
{
	GaduPresence *self_presence;

	GHashTable *presence_cache;
};

static GaduStatusEnum
map_gg_status_to_gadu (gint ggstatus)
{
	switch (GG_S (ggstatus)) {
		case GG_STATUS_NOT_AVAIL:
		case GG_STATUS_NOT_AVAIL_DESCR:
			return GADU_STATUS_OFFLINE;
		case GG_STATUS_FFC:
		case GG_STATUS_FFC_DESCR:
		case GG_STATUS_AVAIL:
		case GG_STATUS_AVAIL_DESCR:
			return GADU_STATUS_AVAILABLE;
		case GG_STATUS_BUSY:
		case GG_STATUS_BUSY_DESCR:
			return GADU_STATUS_AWAY;
		case GG_STATUS_DND:
		case GG_STATUS_DND_DESCR:
			return GADU_STATUS_BUSY;
		case GG_STATUS_INVISIBLE:
		case GG_STATUS_INVISIBLE_DESCR:
			return GADU_STATUS_HIDDEN;
		default:
			g_message ("Unknown status");
			return GADU_STATUS_UNKNOWN;
			
	}
}

static gint
map_gadu_status_to_gg (GaduStatusEnum gadu_status, gboolean has_message)
{
	g_message ("%d", gadu_status);

	switch (gadu_status) {
		case GADU_STATUS_OFFLINE:
			if (has_message)
				return GG_STATUS_NOT_AVAIL_DESCR;
			else
				return GG_STATUS_NOT_AVAIL;
		case GADU_STATUS_AVAILABLE:
			if (has_message)
				return GG_STATUS_AVAIL_DESCR;
			else
				return GG_STATUS_AVAIL;
		case GADU_STATUS_AWAY:
			if (has_message)
				return GG_STATUS_BUSY_DESCR;
			else
				return GG_STATUS_BUSY;
		case GADU_STATUS_BUSY:
			if (has_message)
				return GG_STATUS_DND_DESCR;
			else
				return GG_STATUS_DND;
		case GADU_STATUS_HIDDEN:
			if (has_message)
				return GG_STATUS_INVISIBLE_DESCR;
			else
				return GG_STATUS_INVISIBLE;
		default:
			g_message ("Unknown gadu status. Falling back to AVAILABLE");
			if (has_message)
				return GG_STATUS_AVAIL_DESCR;
			else
				return GG_STATUS_AVAIL;
	}
}

GaduPresence *
gadu_presence_new (GaduStatusEnum status, const gchar *message)
{
	GaduPresence *presence = NULL;
	
	g_return_val_if_fail (status >= 0 && status < GADU_STATUS_LAST, NULL);
	
	presence = g_new0 (GaduPresence, 1);
	presence->status = status;
	presence->message = g_strdup (message);
	
	return presence;
}

TpPresenceStatus *
gadu_presence_get_tp_presence_status (GaduPresence *presence)
{
	TpPresenceStatus *status = NULL;

	if (presence) {
		GHashTable *args = NULL;
		
		if (presence->message) {
			GValue *message = tp_g_value_slice_new (G_TYPE_STRING);
		
			args = g_hash_table_new_full (g_str_hash,
						      g_str_equal,
						      NULL,
						      (GDestroyNotify) tp_g_value_slice_free);
			
			g_value_set_string (message, presence->message);
			
			g_hash_table_insert (args, "message", message);
		}
	
		status = tp_presence_status_new (presence->status,
						 args);
		
		if (args)
			g_hash_table_unref (args);
	} else {
		status = tp_presence_status_new (GADU_STATUS_UNKNOWN, NULL);
	}
	
	return status;
}

void
gadu_presence_free (GaduPresence *presence)
{
	g_free (presence->message);
	g_free (presence);
}

GaduPresence *
gadu_presence_parse (gint status, const gchar *descr)
{
	GaduStatusEnum gadu_status;
	GaduPresence *presence = NULL;
	
	gadu_status = map_gg_status_to_gadu (status);
	
	if (GG_S_D (status) && descr) {
		presence = gadu_presence_new (gadu_status, descr);
	} else {
		presence = gadu_presence_new (gadu_status, NULL);
	}
	
	return presence;
}

static guint
get_maximum_status_message_length_cb (GObject *obj)
{
	return GG_STATUS_DESCR_MAXSIZE;
}

static GHashTable *
get_contact_statuses (GObject *obj,
		      const GArray *contacts,
		      GError **error)
{
	GaduConnection *self = GADU_CONNECTION (obj);
	GaduConnectionPresencePrivate *priv = self->presence_priv;
	GHashTable *result = NULL;
	guint i;
	
	result = g_hash_table_new_full (g_direct_hash,
					g_direct_equal,
					NULL,
					(GDestroyNotify) tp_presence_status_free);
	
	for (i = 0; i < contacts->len; i++) {
		TpHandle contact = g_array_index (contacts, TpHandle, i);
		TpPresenceStatus *status = NULL;
		GaduPresence *presence = NULL;
		
		g_message ("GetContactStatuses(handle=%d)", contact);
		
		if (contact == tp_base_connection_get_self_handle (TP_BASE_CONNECTION (obj))) {
			presence = priv->self_presence;
		} else {
			presence = g_hash_table_lookup (priv->presence_cache,
							GUINT_TO_POINTER (contact));
		}
		
		status = gadu_presence_get_tp_presence_status (presence);
		
		g_hash_table_insert (result,
				     GUINT_TO_POINTER (contact),
				     status);
	}
	
	return result;
}

static gboolean
emit_self_presence_update (GaduConnection *self)
{
	GaduConnectionPresencePrivate *priv = self->presence_priv;
	GaduPresence *presence = priv->self_presence;
	TpPresenceStatus *status = NULL;
	TpHandle self_handle;
	gint gg_status;
	
	g_return_val_if_fail (self->session != NULL, FALSE);
	
	gg_status = map_gadu_status_to_gg (presence->status, (presence->message != NULL));
	
	if (presence->message) {
		gchar *message_truncated = NULL;
		
		message_truncated = g_strndup (presence->message, get_maximum_status_message_length_cb (G_OBJECT (self)));
		
		if (gg_change_status_descr (self->session, gg_status, message_truncated) < 0) {
			g_message ("Failed to set own status");
			g_free (message_truncated);
			return FALSE;
		}
		
		g_free (message_truncated);
	} else {
		if (gg_change_status (self->session, gg_status) < 0) {
			g_message ("Failed to set own status");
			return FALSE;
		}
	}
	
	self_handle = tp_base_connection_get_self_handle (TP_BASE_CONNECTION (self));
	status = gadu_presence_get_tp_presence_status (presence);
	
	tp_presence_mixin_emit_one_presence_update (G_OBJECT (self), self_handle, status);
	
	return TRUE;
}

static gboolean
set_own_status (GObject *obj,
		const TpPresenceStatus *status,
		GError **error)
{
	GaduConnection *self = GADU_CONNECTION (obj);
	GaduConnectionPresencePrivate *priv = self->presence_priv;

	if (status) {
		GHashTable *arguments = status->optional_arguments;
		GValue *message = NULL;
		GaduPresence *presence = NULL;
		const gchar *message_str = NULL;
		
		if (arguments) {
			message = g_hash_table_lookup (arguments, "message");
		}
		
		if (message) {
			if (!G_VALUE_HOLDS_STRING (message)) {
				g_set_error (error, TP_ERROR, TP_ERROR_INVALID_ARGUMENT,
					     "Status argument 'message' requires a string");
				return FALSE;
			}
		
			message_str = g_value_get_string (message);
		}
		
		gadu_presence_free (priv->self_presence);
		priv->self_presence = gadu_presence_new (status->index, message_str);
	}

	/* postpone status emission until getting connected */
	if (tp_base_connection_get_status (TP_BASE_CONNECTION (self)) !=
		TP_CONNECTION_STATUS_CONNECTED) {
		return TRUE;
	}

	return emit_self_presence_update (self);
}

static void
connection_status_changed_cb (GaduConnection *self,
			      TpConnectionStatus status,
			      TpConnectionStatusReason reason,
			      gpointer user_data)
{
	if (status == TP_CONNECTION_STATUS_CONNECTED) {
		emit_self_presence_update (self);
	}
}

void
gadu_connection_presence_cache_add (GaduConnection *self,
				TpHandle contact,
				GaduPresence *presence)
{
	GaduConnectionPresencePrivate *priv = self->presence_priv;
	
	g_return_if_fail (presence != NULL);
	
	g_hash_table_insert (priv->presence_cache, GUINT_TO_POINTER (contact), presence);
}

void
gadu_connection_presence_init (GObject *object)
{
	GaduConnection *self = GADU_CONNECTION (object);
	
	self->presence_priv = g_new0 (GaduConnectionPresencePrivate, 1);
	self->presence_priv->self_presence = gadu_presence_new (GADU_STATUS_AVAILABLE, NULL);
	self->presence_priv->presence_cache = g_hash_table_new_full (g_direct_hash,
								g_direct_equal,
								NULL,
								(GDestroyNotify) gadu_presence_free);

	g_signal_connect (self, "status-changed",
		G_CALLBACK (connection_status_changed_cb), NULL);

	tp_presence_mixin_init (object, G_STRUCT_OFFSET (GaduConnection, presence_mixin));
	tp_presence_mixin_simple_presence_register_with_contacts_mixin (object);
}

void
gadu_connection_presence_class_init (GaduConnectionClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	TpPresenceMixinClass *presence_mixin_class = NULL;

	tp_presence_mixin_class_init (object_class,
				      G_STRUCT_OFFSET (GaduConnectionClass, presence_mixin),
				      NULL,
				      get_contact_statuses,
				      set_own_status,
				      gadu_connection_get_statuses ());
	
	presence_mixin_class = TP_PRESENCE_MIXIN_CLASS (klass);
	presence_mixin_class->get_maximum_status_message_length = 
		get_maximum_status_message_length_cb;

	tp_presence_mixin_simple_presence_init_dbus_properties (object_class);
}

void
gadu_connection_presence_finalize (GObject *object)
{
	GaduConnection *self = GADU_CONNECTION (object);

	gadu_presence_free (self->presence_priv->self_presence);
	g_hash_table_unref (self->presence_priv->presence_cache);
	g_free (self->presence_priv);

	tp_presence_mixin_finalize (object);
}


