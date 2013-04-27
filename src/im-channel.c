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
#include <stdlib.h>

#define DEBUG_FLAG GADU_DEBUG_FLAG_IM

#include "connection.h"
#include "debug.h"
#include "im-channel.h"

static void destroyable_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (GaduImChannel, gadu_im_channel, TP_TYPE_BASE_CHANNEL,
	G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_TYPE_TEXT, tp_message_mixin_text_iface_init)
	G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_MESSAGES, tp_message_mixin_messages_iface_init)
	G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_CHAT_STATE, tp_message_mixin_chat_state_iface_init)
	G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CHANNEL_INTERFACE_DESTROYABLE, destroyable_iface_init))

#define GADU_IM_CHANNEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GADU_TYPE_IM_CHANNEL, GaduImChannelPrivate))

struct _GaduImChannelPrivate
{
	gint last_type_length;
};

static void
send_message (GObject *object,
	      TpMessage *message,
	      TpMessageSendingFlags flags)
{
	GaduImChannel *self = GADU_IM_CHANNEL (object);
	TpBaseChannel *base = TP_BASE_CHANNEL (self);
	TpBaseConnection *base_conn = tp_base_channel_get_connection (base);
	GaduConnection *conn = GADU_CONNECTION (base_conn);
	gchar *msg = NULL;
	
	TpHandle target = tp_base_channel_get_target_handle (base);
	TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base_conn, TP_HANDLE_TYPE_CONTACT);
	const gchar *id = tp_handle_inspect (contact_repo, target);
	uin_t uin = (uin_t) atoi (id);

	msg = tp_message_to_text (message, NULL);

	gadu_debug ("Sending message to uin=%d", uin);

	gg_send_message (conn->session, GG_CLASS_CHAT, uin, (unsigned char *) msg);
	
	tp_message_mixin_sent (object,
			       message,
			       flags,
			       "",
			       NULL);
	
	g_free (msg);
}

void
gadu_im_channel_receive (GaduImChannel *self,
			 time_t timestamp,
			 const gchar *text)
{
	TpBaseChannel *base = TP_BASE_CHANNEL (self);
	TpBaseConnection *base_conn = tp_base_channel_get_connection (base);
	TpHandle sender;
	TpMessage *msg;
	
	sender = tp_base_channel_get_target_handle (base);
	
	msg = tp_cm_message_new (base_conn, 2);
	tp_cm_message_set_sender (msg, sender);
	tp_message_set_uint32 (msg, 0, "message-type", TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL);
	tp_message_set_int64 (msg, 0, "message-sent", timestamp);
	tp_message_set_int64 (msg, 0, "message-received", time (NULL));
	
	tp_message_set_string (msg, 1, "content-type", "text/plain");
	tp_message_set_string (msg, 1, "content", text);
	
	tp_message_mixin_change_chat_state (G_OBJECT (self), sender, TP_CHANNEL_CHAT_STATE_ACTIVE);
	
	tp_message_mixin_take_received (G_OBJECT (self), msg);
}

void
gadu_im_channel_type_notify (GaduImChannel *self,
			     gint length)
{
	GaduImChannelPrivate *priv = self->priv;
	TpChannelChatState state;
	TpHandle handle;
	
	handle = tp_base_channel_get_target_handle (TP_BASE_CHANNEL (self));
	
	if (length == 0) {
		state = TP_CHANNEL_CHAT_STATE_ACTIVE;
	} else if (priv->last_type_length == length) {
		state = TP_CHANNEL_CHAT_STATE_PAUSED;
	} else {
		state = TP_CHANNEL_CHAT_STATE_COMPOSING;
	}
	
	priv->last_type_length = length;
	
	tp_message_mixin_change_chat_state (G_OBJECT (self), handle, state);
}

static gboolean
gadu_im_channel_send_chat_state (GObject *object,
				 TpChannelChatState state,
				 GError **error)
{
	TpBaseChannel *base = TP_BASE_CHANNEL (object);
	TpBaseConnection *base_conn = tp_base_channel_get_connection (base);
	GaduConnection *conn = GADU_CONNECTION (base_conn);
	gint ret = 0;
	
	gadu_debug ("send chat state: %d", state);
	
	TpHandle target = tp_base_channel_get_target_handle (base);
	TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (base_conn, TP_HANDLE_TYPE_CONTACT);
	const gchar *id = tp_handle_inspect (contact_repo, target);
	uin_t uin = (uin_t) atoi (id);
	
	switch (state) {
		case TP_CHANNEL_CHAT_STATE_ACTIVE:
		case TP_CHANNEL_CHAT_STATE_GONE:
		case TP_CHANNEL_CHAT_STATE_PAUSED:
			ret = gg_typing_notification (conn->session, uin, 0);
			break;
		case TP_CHANNEL_CHAT_STATE_COMPOSING:
			ret = gg_typing_notification (conn->session, uin, g_random_int_range (1, 1024));
			break;
		case TP_CHANNEL_CHAT_STATE_INACTIVE:
			break;
	}
	
	return ret == 0 ? TRUE : FALSE;
}

static void
constructed (GObject *obj)
{
	GaduImChannel *self = GADU_IM_CHANNEL (obj);
	TpBaseChannel *base = TP_BASE_CHANNEL (self);
	TpChannelTextMessageType types[] = {
		TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL
	};
	const gchar * supported_content_types[] = {
		"text/plain",
		NULL
	};
	
	void (*parent_constructed) (GObject *) = G_OBJECT_CLASS (gadu_im_channel_parent_class)->constructed;
	
	if (parent_constructed != NULL)
		parent_constructed (obj);
	
	tp_message_mixin_init (obj, G_STRUCT_OFFSET (GaduImChannel, message_mixin), tp_base_channel_get_connection (base));

	tp_message_mixin_implement_sending (obj, send_message,
					    G_N_ELEMENTS (types), types, 0,
					    TP_DELIVERY_REPORTING_SUPPORT_FLAG_RECEIVE_FAILURES,
					    supported_content_types);
	
	tp_message_mixin_implement_send_chat_state (obj, gadu_im_channel_send_chat_state);
}

static void
finalize (GObject *object)
{
	tp_message_mixin_finalize (object);

	G_OBJECT_CLASS (gadu_im_channel_parent_class)->finalize (object);
}

static GPtrArray *
gadu_im_channel_get_interfaces (TpBaseChannel *chan)
{
	GPtrArray *interfaces;
	
	interfaces = TP_BASE_CHANNEL_CLASS (gadu_im_channel_parent_class)->get_interfaces (chan);
	
	g_ptr_array_add (interfaces, TP_IFACE_CHANNEL_INTERFACE_CHAT_STATE);
	g_ptr_array_add (interfaces, TP_IFACE_CHANNEL_INTERFACE_MESSAGES);
	g_ptr_array_add (interfaces, TP_IFACE_CHANNEL_INTERFACE_DESTROYABLE);
	
	return interfaces;
}


static void
gadu_im_channel_close (TpBaseChannel *chan)
{
	GObject *object = G_OBJECT (chan);
	
	tp_message_mixin_maybe_send_gone (object);
	
	if (!tp_base_channel_is_destroyed (chan)) {
		TpHandle first_sender;
		
		if (tp_message_mixin_has_pending_messages (object, &first_sender)) {
			tp_message_mixin_set_rescued (object);
			tp_base_channel_reopened (chan, first_sender);
		} else {
			tp_base_channel_destroyed (chan);
		}
	}
}


static void
gadu_im_channel_fill_immutable_properties (TpBaseChannel *chan,
					   GHashTable *properties)
{
	TpBaseChannelClass *klass = TP_BASE_CHANNEL_CLASS (gadu_im_channel_parent_class);
	
	klass->fill_immutable_properties (chan, properties);
	
	tp_dbus_properties_mixin_fill_properties_hash (G_OBJECT (chan), properties,
		TP_IFACE_CHANNEL_INTERFACE_MESSAGES, "MessagePartSupportFlags",
		TP_IFACE_CHANNEL_INTERFACE_MESSAGES, "DeliveryReportingSupport",
		TP_IFACE_CHANNEL_INTERFACE_MESSAGES, "SupportedContentTypes",
		TP_IFACE_CHANNEL_INTERFACE_MESSAGES, "MessageTypes",
		NULL);
}

static void
destroyable_destroy (TpSvcChannelInterfaceDestroyable *iface,
		     DBusGMethodInvocation *context)
{
	TpBaseChannel *chan = TP_BASE_CHANNEL (iface);
	
	tp_message_mixin_clear (G_OBJECT (chan));
	gadu_im_channel_close (chan);
	
	tp_svc_channel_interface_destroyable_return_from_destroy (context);
}

static void
destroyable_iface_init (gpointer iface, gpointer user_data)
{
	TpSvcChannelInterfaceDestroyableClass *klass = iface;
	
	tp_svc_channel_interface_destroyable_implement_destroy (klass, destroyable_destroy);
}

static void
gadu_im_channel_init (GaduImChannel *self)
{
	self->priv = GADU_IM_CHANNEL_GET_PRIVATE (self);
	
	self->priv->last_type_length = 0;
}

static void
gadu_im_channel_class_init (GaduImChannelClass *klass)
{
	TpBaseChannelClass *base_class = TP_BASE_CHANNEL_CLASS (klass);
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->constructed = constructed;
	object_class->finalize = finalize;
	
	base_class->channel_type = TP_IFACE_CHANNEL_TYPE_TEXT;
	base_class->target_handle_type = TP_HANDLE_TYPE_CONTACT;
	base_class->get_interfaces = gadu_im_channel_get_interfaces;
	base_class->close = gadu_im_channel_close;
	base_class->fill_immutable_properties = gadu_im_channel_fill_immutable_properties;
	
	g_type_class_add_private (klass, sizeof (GaduImChannelPrivate));
	
	tp_message_mixin_init_dbus_properties (object_class);
}

GaduImChannel *
gadu_im_channel_new (GaduConnection *connection,
		     TpHandle handle,
		     TpHandle initiator,
		     gboolean requested)
{
	return g_object_new (GADU_TYPE_IM_CHANNEL,
			     "connection", connection,
			     "handle", handle,
			     "initiator-handle", initiator,
			     "requested", requested,
			     NULL);
}

