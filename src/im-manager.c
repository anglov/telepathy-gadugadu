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
#include <libgadu.h>

#define DEBUG_FLAG GADU_DEBUG_FLAG_IM

#include "connection.h"
#include "debug.h"
#include "im-channel.h"
#include "im-manager.h"

static void channel_manager_iface_init (gpointer, gpointer);
static void message_received_cb (GaduConnection *connection, struct gg_event *evt, GaduImManager *self);
static void typing_notification_cb (GaduConnection *connection, struct gg_event *evt, GaduImManager *self);

G_DEFINE_TYPE_WITH_CODE (GaduImManager, gadu_im_manager, G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE (TP_TYPE_CHANNEL_MANAGER, channel_manager_iface_init))

#define GADU_IM_MANAGER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GADU_TYPE_IM_MANAGER, GaduImManagerPrivate))

struct _GaduImManagerPrivate
{
	GaduConnection *connection;

	// handle => channel map
	GHashTable *channels;
};

enum {
	PROP_CONNECTION = 1,
	
	LAST_PROP
};


static void
gadu_im_manager_close_all (GaduImManager *self)
{
	if (self->priv->channels != NULL) {
		g_hash_table_unref (self->priv->channels);
		self->priv->channels = NULL;
	}
}

static void
gadu_im_manager_init (GaduImManager *self)
{
	self->priv = GADU_IM_MANAGER_GET_PRIVATE (self);
	
	self->priv->channels = g_hash_table_new_full (g_direct_hash, g_direct_equal,
						      NULL, g_object_unref);
}

static void
get_property (GObject *object,
	      guint prop_id,
	      GValue *value,
	      GParamSpec *pspec)
{
	GaduImManager *self = GADU_IM_MANAGER (object);
	
	switch (prop_id) {
		case PROP_CONNECTION:
			g_value_set_object (value, self->priv->connection);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
set_property (GObject *object,
	      guint prop_id,
	      const GValue *value,
	      GParamSpec *pspec)
{
	GaduImManager *self = GADU_IM_MANAGER (object);
	
	switch (prop_id) {
		case PROP_CONNECTION:
			self->priv->connection = g_value_get_object (value);
			g_signal_connect (self->priv->connection, "message-received",
					  G_CALLBACK (message_received_cb), self);
			g_signal_connect (self->priv->connection, "typing-notification",
					  G_CALLBACK (typing_notification_cb), self);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
dispose (GObject *object)
{
	GaduImManager *self = GADU_IM_MANAGER (object);
	
	gadu_im_manager_close_all (self);
	
	((GObjectClass *) gadu_im_manager_parent_class)->dispose (object);
}

static void
gadu_im_manager_class_init (GaduImManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->dispose = dispose;
	
	g_type_class_add_private (klass, sizeof (GaduImManagerPrivate));
	
	g_object_class_install_property (object_class,
					 PROP_CONNECTION,
					 g_param_spec_object ("connection", "Connection object",
							      "The connection that owns this channel manager.",
							      GADU_TYPE_CONNECTION,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
							      G_PARAM_STATIC_STRINGS));
}


GObject *
gadu_im_manager_new (GaduConnection *connection)
{
	return g_object_new (GADU_TYPE_IM_MANAGER,
			     "connection", connection,
			     NULL);
}


static void
im_channel_closed_cb (GaduImChannel *channel, gpointer user_data)
{
	GaduImManager *self = GADU_IM_MANAGER (user_data);
	TpBaseChannel *base_channel = TP_BASE_CHANNEL (channel);
	TpHandle handle = tp_base_channel_get_target_handle (base_channel);

	if (tp_base_channel_is_registered (base_channel)) {
		tp_channel_manager_emit_channel_closed_for_object (self, TP_EXPORTABLE_CHANNEL (channel));
	}
	
	if (self->priv->channels) {
		if (tp_base_channel_is_destroyed (base_channel)) {
			g_hash_table_remove (self->priv->channels, GUINT_TO_POINTER (handle));
		} else {
			tp_channel_manager_emit_new_channel (self,
							     TP_EXPORTABLE_CHANNEL (channel),
							     NULL);
		}
	}
}


static GaduImChannel *
new_im_channel (GaduImManager *self,
		TpHandle handle,
		gpointer request_token)
{
	GaduImChannel *channel = NULL;
	GSList *request_tokens;
	TpHandle initiator;
	
	g_return_val_if_fail (handle != 0, NULL);
	
	if (request_token == NULL) {
		initiator = tp_base_connection_get_self_handle (TP_BASE_CONNECTION (self->priv->connection));
	} else {
		initiator = handle;
	}
	
	gadu_debug ("Creating new IM Channel: handle=%d initiator=%d",
		    handle, initiator);
	
	channel = gadu_im_channel_new (self->priv->connection,
				       handle,
				       initiator,
				       (handle != initiator));
	
	tp_base_channel_register (TP_BASE_CHANNEL (channel));
	
	g_signal_connect (channel, "closed", (GCallback) im_channel_closed_cb, self);
	
	g_hash_table_insert (self->priv->channels, GUINT_TO_POINTER (handle), channel);
	
	if (request_token == NULL) {
		request_tokens = NULL;
	} else {
		request_tokens = g_slist_prepend (NULL, request_token);
	}
	
	tp_channel_manager_emit_new_channel (self, TP_EXPORTABLE_CHANNEL (channel), request_tokens);
	
	g_slist_free (request_tokens);
	
	return channel;
}	

static GaduImChannel *
get_channel_for_incoming_message (GaduImManager *self,
				  const gchar *uid)
{
	TpBaseConnection *conn = TP_BASE_CONNECTION (self->priv->connection);
	TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (conn,
						TP_HANDLE_TYPE_CONTACT);
	TpHandle handle;
	GaduImChannel *channel;
	
	g_return_val_if_fail (uid != NULL, NULL);
	
	handle = tp_handle_ensure (contact_repo, uid, NULL, NULL);
	if (handle == 0)
		return NULL;
	
	channel = g_hash_table_lookup (self->priv->channels, GUINT_TO_POINTER (handle));
	
	if (channel != NULL)
		return channel;
	else
		return new_im_channel (self, handle, NULL);
}

static void
message_received_cb (GaduConnection *connection, struct gg_event *evt, GaduImManager *self)
{
	GaduImChannel *channel;
	gchar *uid;
	
	uid = g_strdup_printf ("%d", evt->event.msg.sender);
	channel = get_channel_for_incoming_message (self, uid);
	g_free (uid);
	
	gadu_im_channel_receive (channel,
				 evt->event.msg.time,
				 (char *) evt->event.msg.message);
}

static void
typing_notification_cb (GaduConnection *connection,
			struct gg_event *evt,
			GaduImManager *self)
{
	TpBaseConnection *base_conn = TP_BASE_CONNECTION (connection);
	TpHandleRepoIface *contact_repo = NULL;
	TpHandle handle;
	GaduImChannel *channel = NULL;
	gchar *uid;
	
	uid = g_strdup_printf ("%d", evt->event.typing_notification.uin);
	contact_repo = tp_base_connection_get_handles (base_conn, TP_HANDLE_TYPE_CONTACT);
	handle = tp_handle_lookup (contact_repo, uid, NULL, NULL);
	g_free (uid);
	
	if (handle == 0)
		return;
	
	channel = g_hash_table_lookup (self->priv->channels, GUINT_TO_POINTER (handle));
	
	if (channel != NULL) {
		gadu_im_channel_type_notify (channel, evt->event.typing_notification.length);
	}
}

static void
foreach_channel (TpChannelManager *manager,
		 TpExportableChannelFunc func,
		 gpointer user_data)
{
	GaduImManager *self = GADU_IM_MANAGER (manager);
	GHashTableIter iter;
	gpointer handle, channel;
	
	g_hash_table_iter_init (&iter, self->priv->channels);
	
	while (g_hash_table_iter_next (&iter, &handle, &channel)) {
		func (TP_EXPORTABLE_CHANNEL (channel), user_data);
	}
}


static const gchar * const fixed_properties[] = {
	TP_PROP_CHANNEL_CHANNEL_TYPE,
	TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
	NULL
};

static const gchar * const allowed_properties[] = {
	TP_PROP_CHANNEL_TARGET_HANDLE,
	TP_PROP_CHANNEL_TARGET_ID,
	NULL
};

static void
type_foreach_channel_class (GType type,
			    TpChannelManagerTypeChannelClassFunc func,
			    gpointer user_data)
{
	GHashTable *table = tp_asv_new (
		TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
		TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
		NULL);
	
	func (type, table, allowed_properties, user_data);
	
	g_hash_table_unref (table);
}


static gboolean
handle_request (GaduImManager *self,
		gpointer request_token,
		GHashTable *request_properties,
		gboolean require_new)
{
	TpHandle handle;
	TpExportableChannel *chan;
	GError *error = NULL;

	if (tp_strdiff (tp_asv_get_string (request_properties, TP_PROP_CHANNEL_CHANNEL_TYPE),
			TP_IFACE_CHANNEL_TYPE_TEXT)) {
		return FALSE;
	}
	
	if (tp_asv_get_uint32 (request_properties,
			       TP_PROP_CHANNEL_TARGET_HANDLE_TYPE,
			       NULL) != TP_HANDLE_TYPE_CONTACT) {
		return FALSE;
	}
	
	handle = tp_asv_get_uint32 (request_properties, TP_PROP_CHANNEL_TARGET_HANDLE, NULL);
	
	if (tp_channel_manager_asv_has_unknown_properties (request_properties,
							   fixed_properties,
							   allowed_properties,
							   &error)) {
		goto out;
	}

	chan = g_hash_table_lookup (self->priv->channels, GUINT_TO_POINTER (handle));

	if (chan == NULL) {
		new_im_channel (self, handle, request_token);
	} else if (require_new) {
		g_set_error (&error, TP_ERROR, TP_ERROR_NOT_AVAILABLE,
			     "Already chatting with contact #%u in another channel", handle);
		goto out;
	} else {
		tp_channel_manager_emit_request_already_satisfied (self, request_token, chan);
	}
	
out:
	if (error) {
		tp_channel_manager_emit_request_failed (self, request_token, error->domain,
							error->code, error->message);
		g_error_free (error);
	}
	
	return TRUE;
}


static gboolean
create_channel (TpChannelManager *manager,
		gpointer request_token,
		GHashTable *request_properties)
{
	return handle_request (GADU_IM_MANAGER (manager),
			       request_token,
			       request_properties,
			       TRUE);
}


static gboolean
request_channel (TpChannelManager *manager,
		 gpointer request_token,
		 GHashTable *request_properties)
{
	return handle_request (GADU_IM_MANAGER (manager),
			       request_token,
			       request_properties,
			       FALSE);
}


static gboolean
ensure_channel (TpChannelManager *manager,
		gpointer request_token,
		GHashTable *request_properties)
{
	return handle_request (GADU_IM_MANAGER (manager),
			       request_token,
			       request_properties,
			       FALSE);
}


static void
channel_manager_iface_init (gpointer g_iface, gpointer data)
{
	TpChannelManagerIface *iface = g_iface;
	
	iface->foreach_channel = foreach_channel;
	iface->type_foreach_channel_class = type_foreach_channel_class;
	iface->create_channel = create_channel;
	iface->request_channel = request_channel;
	iface->ensure_channel = ensure_channel;
}

