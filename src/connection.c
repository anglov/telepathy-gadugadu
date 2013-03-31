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
#include <string.h>

#define DEBUG_FLAG GADU_DEBUG_FLAG_CONNECTION

#include "debug.h"
#include "connection.h"
#include "connection-aliasing.h"
#include "connection-presence.h"
#include "contact-list.h"
#include "im-manager.h"

G_DEFINE_TYPE_WITH_CODE (GaduConnection, gadu_connection, TP_TYPE_BASE_CONNECTION,
	G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_ALIASING,
			       gadu_connection_aliasing_iface_init);
	G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACTS,
			       tp_contacts_mixin_iface_init);
	G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_LIST,
			       tp_base_contact_list_mixin_list_iface_init);
	G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_CONTACT_GROUPS,
			       tp_base_contact_list_mixin_groups_iface_init);
	G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
			       tp_presence_mixin_simple_presence_iface_init))

#define GADU_CONNECTION_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GADU_TYPE_CONNECTION, GaduConnectionPrivate))

#define PING_INTERVAL 60


struct _GaduConnectionPrivate
{
	gchar *account;
	gchar *password;
	
	struct gg_session *session;
	guint pinger_id;
	
	guint event_loop_id;
	
	GHashTable *presence_cache;
	GaduContactList *contact_list;
	TpSimplePasswordManager *password_manager;
};


enum
{
	PROP_ACCOUNT = 1,
	PROP_PASSWORD,
	
	PROP_LAST
};

enum
{
	SIGNAL_MESSAGE_RECEIVED,
	SIGNAL_USERLIST_RECEIVED,
	
	SIGNAL_LAST
};

static guint signals [SIGNAL_LAST] = { 0 };

static const gchar *interfaces_always_present[] = {
	TP_IFACE_CONNECTION_INTERFACE_ALIASING,
	TP_IFACE_CONNECTION_INTERFACE_CONTACTS,
	TP_IFACE_CONNECTION_INTERFACE_CONTACT_LIST,
	TP_IFACE_CONNECTION_INTERFACE_CONTACT_GROUPS,
	TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
	TP_IFACE_CONNECTION_INTERFACE_SIMPLE_PRESENCE,
	NULL
};

static const TpPresenceStatusOptionalArgumentSpec gadu_statuses_arguments[] = {
	{ "message", "s", NULL, NULL },
	{ NULL, NULL, NULL, NULL }
};

/* NOTE: Keep in sync with GaduStatusEnum from connection-presence.h */
static const TpPresenceStatusSpec gadu_statuses[] = {
	{ "offline", TP_CONNECTION_PRESENCE_TYPE_OFFLINE, TRUE, gadu_statuses_arguments, NULL, NULL },
	{ "available", TP_CONNECTION_PRESENCE_TYPE_AVAILABLE, TRUE, gadu_statuses_arguments, NULL, NULL },
	{ "away", TP_CONNECTION_PRESENCE_TYPE_AWAY, TRUE, gadu_statuses_arguments, NULL, NULL },
	{ "dnd", TP_CONNECTION_PRESENCE_TYPE_BUSY, TRUE, gadu_statuses_arguments, NULL, NULL },
	{ "hidden", TP_CONNECTION_PRESENCE_TYPE_HIDDEN, TRUE, gadu_statuses_arguments, NULL, NULL },
	{ "unknown", TP_CONNECTION_PRESENCE_TYPE_UNKNOWN, FALSE, gadu_statuses_arguments, NULL, NULL 	},
	{ NULL, 0, FALSE, NULL, NULL, NULL }
};


const TpPresenceStatusSpec *
gadu_connection_get_statuses ()
{
	return gadu_statuses;
}

const gchar * const *
gadu_connection_get_implemented_interfaces ()
{
	return interfaces_always_present;
}


static GPtrArray *
get_interfaces_always_present (TpBaseConnection *conn)
{
	GPtrArray *interfaces;
	guint i;
	
	interfaces = TP_BASE_CONNECTION_CLASS (gadu_connection_parent_class)->get_interfaces_always_present (conn);

	for (i = 0; interfaces_always_present[i] != NULL; i++)
		g_ptr_array_add (interfaces, (gchar *) interfaces_always_present[i]);
	
	return interfaces;
}


static void
create_handle_repos (TpBaseConnection *conn,
		     TpHandleRepoIface *repos[TP_NUM_HANDLE_TYPES])
{
	repos[TP_HANDLE_TYPE_CONTACT] = tp_dynamic_handle_repo_new (TP_HANDLE_TYPE_CONTACT,
								    NULL,
								    NULL);
}


static GPtrArray *
create_channel_managers (TpBaseConnection *conn)
{
	GaduConnection *self = GADU_CONNECTION (conn);
	GPtrArray *ret = g_ptr_array_sized_new (1);
	
	g_ptr_array_add (ret, gadu_im_manager_new (self));
	
	self->priv->contact_list = gadu_contact_list_new (self);
	g_ptr_array_add (ret, self->priv->contact_list);
	
	self->priv->password_manager = tp_simple_password_manager_new (conn);
	g_ptr_array_add (ret, self->priv->password_manager);
	
	return ret;
}

static gboolean
pinger (gpointer data)
{
	GaduConnection *self = GADU_CONNECTION (data);

	gg_ping (self->session);

	return TRUE;
}

static TpHandle
get_handle_from_uin (TpHandleRepoIface *repo, uin_t uin)
{
	TpHandle handle;
	gchar *uin_str = NULL;
	
	uin_str = g_strdup_printf ("%d", uin);
	handle = tp_handle_lookup (repo, uin_str, NULL, NULL);
	
	g_free (uin_str);
	
	return handle;
}

static void
status_received_cb (GaduConnection *self,
		uin_t uin,
		gint gg_status,
		const gchar *descr)
{
	TpHandleRepoIface *handles_repo = NULL;
	GaduPresence *presence = NULL;
	TpPresenceStatus *status = NULL;
	TpHandle contact;
	
	handles_repo = tp_base_connection_get_handles (TP_BASE_CONNECTION (self),
						       TP_HANDLE_TYPE_CONTACT);
	
	presence = gadu_presence_parse (gg_status, descr);
	contact = get_handle_from_uin (handles_repo, uin);
	gadu_connection_presence_cache_add (self, contact, presence);
	
	status = gadu_presence_get_tp_presence_status (presence);
	
	tp_presence_mixin_emit_one_presence_update (G_OBJECT (self), contact, status);
	
	tp_presence_status_free (status);
}

static void
notify_received_cb (GaduConnection *self, struct gg_event *e)
{
	TpHandleRepoIface *handles_repo = NULL;
	GHashTable *result = NULL;
	gint i;

	handles_repo = tp_base_connection_get_handles (TP_BASE_CONNECTION (self),
						       TP_HANDLE_TYPE_CONTACT);
	
	result = g_hash_table_new_full (g_direct_hash,
					g_direct_equal,
					NULL,
					(GDestroyNotify) tp_presence_status_free);

	for (i = 0; e->event.notify60[i].uin != 0; i++) {
		GaduPresence *presence = NULL;
		TpPresenceStatus *status = NULL;
		TpHandle contact;
		
		presence = gadu_presence_parse (e->event.notify60[i].status,
						e->event.notify60[i].descr);
		contact = get_handle_from_uin (handles_repo, e->event.notify60[i].uin);
		gadu_connection_presence_cache_add (self, contact, presence);
		
		status = gadu_presence_get_tp_presence_status (presence);
		
		g_hash_table_insert (result, GUINT_TO_POINTER (contact), status);
	}
	
	tp_presence_mixin_emit_presence_update (G_OBJECT (self), result);
	
	g_hash_table_unref (result);
}

static gboolean
gadu_listener_cb (GIOChannel *source, GIOCondition cond, gpointer data)
{
	GaduConnection *self = GADU_CONNECTION (data);

	struct gg_event *e;
	
	e = gg_watch_fd (self->session);
	
	if (!e) {
		gadu_error ("Network error: Unable to read from socket");
		tp_base_connection_change_status (TP_BASE_CONNECTION (self),
						  TP_CONNECTION_STATUS_DISCONNECTED,
						  TP_CONNECTION_STATUS_REASON_NETWORK_ERROR);
		return FALSE;
	}
	
	switch (e->type) {
		case GG_EVENT_NONE:
			break;
		case GG_EVENT_MSG:
			gadu_debug_full (GADU_DEBUG_FLAG_IM,
					 "EVENT_MSG: Received message from uid=%d",
					 e->event.msg.sender);
			g_signal_emit (self, signals[SIGNAL_MESSAGE_RECEIVED], 0, e);
			break;
		case GG_EVENT_STATUS60:
			gadu_debug_full (GADU_DEBUG_FLAG_PRESENCE,
					 "EVENT_STATUS60: uid=%d sets status=%d",
					 e->event.status60.uin, e->event.status60.status);
			status_received_cb (self,
					  e->event.status60.uin,
					  e->event.status60.status,
					  e->event.status60.descr);
			break;
		case GG_EVENT_NOTIFY60:
			gadu_debug_full (GADU_DEBUG_FLAG_PRESENCE,
					 "EVENT_NOTIFY60: Received contacts statuses");
			notify_received_cb (self, e);
			break;
		case GG_EVENT_NOTIFY:
			gadu_error_full (GADU_DEBUG_FLAG_PRESENCE,
					 "EVENT_NOTIFY: Not implemented");
			break;
		case GG_EVENT_USERLIST100_REPLY:
			if (e->event.userlist100_reply.type == GG_USERLIST100_REPLY_LIST) {
				gadu_debug_full (GADU_DEBUG_FLAG_CONTACTS,
					 	 "EVENT_USERLIST100_REPLY: Received contacts list");
				g_signal_emit (self, signals[SIGNAL_USERLIST_RECEIVED], 0, e);
			}
			break;
		case GG_EVENT_USERLIST:
			gadu_error_full (GADU_DEBUG_FLAG_CONTACTS,
					 "EVENT_USERLIST: Not implemented");
			break;
	}
	
	gg_event_free (e);
	
	return TRUE;
}


static gboolean
login_cb (GIOChannel *source, GIOCondition cond, gpointer data)
{
	GaduConnection *self = GADU_CONNECTION (data);
	
	struct gg_event *evt;
	
	evt = gg_watch_fd (self->session);
	
	if (!evt) {
		gadu_error ("Connection interrupted");
		self->priv->event_loop_id = 0;
		return FALSE;
	}

	gadu_debug ("fd=%d check=%d state=%d", self->session->fd, self->session->check, self->session->state);

	switch (evt->type) {
		case GG_EVENT_NONE:
			break;
		case GG_EVENT_CONN_SUCCESS:
			gadu_debug ("Connected");
			tp_base_connection_change_status (TP_BASE_CONNECTION (self),
							  TP_CONNECTION_STATUS_CONNECTED,
							  TP_CONNECTION_STATUS_REASON_REQUESTED);
			
			gg_notify(self->session, NULL, 0);
			
			self->priv->pinger_id = g_timeout_add_seconds (PING_INTERVAL, pinger, self);
			self->priv->event_loop_id = g_io_add_watch (source, G_IO_IN | G_IO_ERR | G_IO_HUP,
								    gadu_listener_cb, self);
			goto out;
			break;
		case GG_EVENT_CONN_FAILED:
			gadu_debug ("Connection failed");
			tp_base_connection_change_status (TP_BASE_CONNECTION (self),
							  TP_CONNECTION_STATUS_DISCONNECTED,
							  TP_CONNECTION_STATUS_REASON_NETWORK_ERROR);
			self->priv->event_loop_id = 0;
			goto out;
			break;
		case GG_EVENT_MSG:
			gadu_debug ("Received message while connecting");
			break;
		default:
			gadu_warn ("Unknown event type: %d", evt->type);
	}

	self->priv->event_loop_id = g_io_add_watch (source, (self->session->check == 1) ?
							G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL :
							G_IO_IN | G_IO_HUP | G_IO_ERR,
							login_cb, self);

out:
	gg_event_free (evt);
	
	return FALSE;
}

static gboolean
gadu_connection_establish_connection (GaduConnection *self)
{
	TpBaseConnection *base = TP_BASE_CONNECTION (self);
	struct gg_login_params login_params;
	
	memset (&login_params, 0, sizeof (login_params));
	login_params.uin = (uin_t) atoi (self->priv->account);
	login_params.password = self->priv->password;
	login_params.async = 1;
	login_params.status = GG_STATUS_AVAIL;
	login_params.encoding = GG_ENCODING_UTF8;
	
	gadu_debug ("Connecting (uin=%d)", login_params.uin);
	
	self->session = gg_login (&login_params);
	
	if (self->session == NULL) {
		tp_base_connection_change_status (base,
						  TP_CONNECTION_STATUS_DISCONNECTED,
						  TP_CONNECTION_STATUS_REASON_NETWORK_ERROR);
		return FALSE;
	}
	
	GIOChannel *channel = g_io_channel_unix_new (self->session->fd);
	self->priv->event_loop_id = g_io_add_watch (channel, G_IO_IN | G_IO_ERR | G_IO_HUP, login_cb, self);
	g_io_channel_unref (channel);
	
	return TRUE;
}

static void
gadu_connection_password_manager_prompt_cb (GObject *source,
					    GAsyncResult *result,
					    gpointer user_data)
{
	GaduConnection *self = GADU_CONNECTION (user_data);
	TpBaseConnection *base = TP_BASE_CONNECTION (self);
	const GString *password = NULL;
	GError *error = NULL;
	
	password = tp_simple_password_manager_prompt_finish (TP_SIMPLE_PASSWORD_MANAGER (source),
							     result,
							     &error);
	
	if (error != NULL) {
		gadu_error ("Simple password manager failed: %s", error->message);
		
		tp_base_connection_change_status (base,
						  TP_CONNECTION_STATUS_DISCONNECTED,
						  TP_CONNECTION_STATUS_REASON_AUTHENTICATION_FAILED);
		
		return;
	}
	
	g_free (self->priv->password);
	self->priv->password = g_strdup (password->str);

	gadu_connection_establish_connection (self);
}

static gboolean
start_connecting (TpBaseConnection *conn,
		  GError **error)
{
	GaduConnection *self = GADU_CONNECTION (conn);
	TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (conn, TP_HANDLE_TYPE_CONTACT);
	TpHandle self_handle;
	
	self_handle = tp_handle_ensure (contact_repo, self->priv->account, NULL, NULL);
	
	tp_base_connection_set_self_handle (conn, self_handle);
	
	tp_base_connection_change_status (conn,
					  TP_CONNECTION_STATUS_CONNECTING,
					  TP_CONNECTION_STATUS_REASON_REQUESTED);
	
	if (!self->priv->password) {
		tp_simple_password_manager_prompt_async (self->priv->password_manager,
							 gadu_connection_password_manager_prompt_cb,
							 self);
	} else {
		return gadu_connection_establish_connection (self);
	}
	
	return TRUE;
}


static void
shut_down (TpBaseConnection *conn)
{
	GaduConnection *self = GADU_CONNECTION (conn);

	gadu_debug ("Disconnected");

	if (self->priv->pinger_id > 0) {
		g_source_remove (self->priv->pinger_id);
		self->priv->pinger_id = 0;
	}
	
	if (self->priv->event_loop_id > 0) {
		g_source_remove (self->priv->event_loop_id);
		self->priv->event_loop_id = 0;
	}

	gg_logoff (self->session);
	gg_free_session (self->session);
	self->session = NULL;

	tp_base_connection_finish_shutdown (conn);
}


static void
constructed (GObject *object)
{
	TpBaseConnection *base = TP_BASE_CONNECTION (object);

	void (*parent_constructed) (GObject *) = G_OBJECT_CLASS (gadu_connection_parent_class)->constructed;
	
	if (parent_constructed != NULL)
		parent_constructed (object);
	
	tp_contacts_mixin_init (object, G_STRUCT_OFFSET (GaduConnection, contacts_mixin));
	tp_base_connection_register_with_contacts_mixin (base);
	tp_base_contact_list_mixin_register_with_contacts_mixin (base);
	
	gadu_connection_presence_init (object);
	gadu_connection_aliasing_init (object);
}


static void
get_property (GObject *object,
	      guint prop_id,
	      GValue *value,
	      GParamSpec *pspec)
{
	GaduConnection *self = GADU_CONNECTION (object);
	
	switch (prop_id) {
		case PROP_ACCOUNT:
			g_value_set_string (value, self->priv->account);
			break;
		case PROP_PASSWORD:
			g_value_set_string (value, self->priv->password);
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
	GaduConnection *self = GADU_CONNECTION (object);
	
	switch (prop_id) {
		case PROP_ACCOUNT:
			g_free (self->priv->account);
			self->priv->account = g_value_dup_string (value);
			break;
		case PROP_PASSWORD:
			g_free (self->priv->password);
			self->priv->password = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}


static void
finalize (GObject *object)
{
	GaduConnection *self = GADU_CONNECTION (object);
	
	g_free (self->priv->account);
	g_free (self->priv->password);
	
	tp_contacts_mixin_finalize (object);
	gadu_connection_presence_finalize (object);
	
	G_OBJECT_CLASS (gadu_connection_parent_class)->finalize (object);
}

static void
dispose (GObject *obj)
{
	GaduConnection *self = GADU_CONNECTION (obj);
	GaduConnectionPrivate *priv = self->priv;
	
	if (priv->presence_cache) {
		g_hash_table_unref (priv->presence_cache);
		priv->presence_cache = NULL;
	}
	
	G_OBJECT_CLASS (gadu_connection_parent_class)->dispose (obj);
}

static void
gadu_connection_init (GaduConnection *self)
{
	self->priv = GADU_CONNECTION_GET_PRIVATE (self);
	
	self->priv->presence_cache = g_hash_table_new_full (g_direct_hash,
					g_direct_equal,
					NULL,
					(GDestroyNotify) tp_presence_status_free);
}

static void
gadu_connection_class_init (GaduConnectionClass *klass)
{
	TpBaseConnectionClass *base_class = (TpBaseConnectionClass *) klass;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->constructed = constructed;
	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->dispose = dispose;
	object_class->finalize = finalize;
	
	g_type_class_add_private (klass, sizeof (GaduConnectionPrivate));
	
	base_class->create_handle_repos = create_handle_repos;
	base_class->create_channel_managers = create_channel_managers;
	base_class->start_connecting = start_connecting;
	base_class->shut_down = shut_down;
	base_class->get_interfaces_always_present = get_interfaces_always_present;
	
	/* Properties */
	g_object_class_install_property (object_class,
					 PROP_ACCOUNT,
					 g_param_spec_string ("account", "Gadu-gadu account",
							      "The account used when authenticating.",
							      NULL,
							      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
							      G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
					 PROP_PASSWORD,
					 g_param_spec_string ("password", "Gadu-gadu password",
					 		      "The password used when authenticating.",
					 		      NULL,
					 		      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
					 		      G_PARAM_STATIC_STRINGS));

	/* Signals */
	signals[SIGNAL_MESSAGE_RECEIVED] = 
		g_signal_new ("message-received",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[SIGNAL_USERLIST_RECEIVED] =
		g_signal_new ("userlist-received",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);

	tp_contacts_mixin_class_init (object_class,
				      G_STRUCT_OFFSET (GaduConnectionClass, contacts_mixin));
	tp_base_contact_list_mixin_class_init (base_class);
	
	gadu_connection_presence_class_init (klass);	
}


TpBaseConnection *
gadu_connection_new (const gchar *account,
		     const gchar *password,
		     const gchar *protocol)
{
	return g_object_new (GADU_TYPE_CONNECTION,
			     "account", account,
			     "password", password,
			     "protocol", protocol,
			     NULL);
}

TpBaseContactList *
gadu_connection_get_contact_list (GaduConnection *self)
{
	g_return_val_if_fail (GADU_IS_CONNECTION (self), NULL);

	return TP_BASE_CONTACT_LIST (self->priv->contact_list);	
}

