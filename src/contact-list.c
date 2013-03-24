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

#define DEBUG_FLAG GADU_DEBUG_FLAG_CONTACTS

#include "connection.h"
#include "contact.h"
#include "contact-list.h"
#include "debug.h"

static void gadu_contact_list_group_list_iface_init (TpContactGroupListInterface *iface);

G_DEFINE_TYPE_WITH_CODE (GaduContactList, gadu_contact_list, TP_TYPE_BASE_CONTACT_LIST,
	G_IMPLEMENT_INTERFACE (TP_TYPE_CONTACT_GROUP_LIST,
			       gadu_contact_list_group_list_iface_init))

#define GADU_CONTACT_LIST_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GADU_TYPE_CONTACT_LIST, GaduContactListPrivate))


struct _GaduContactListPrivate {
	GHashTable *contacts; // TpHandle => GaduContact
	GHashTable *groups; // gchar* => TpHandleSet
	
	gulong status_changed_id;
};

enum {
	GG_FIELD_NICKNAME = 3,
	GG_FIELD_PHONE = 4,
	GG_FIELD_GROUPS = 5,
	GG_FIELD_UIN = 6,
	GG_FIELD_EMAIL = 7
};

gchar *
gadu_contact_list_get_nickname (GaduContactList *self, TpHandle handle)
{
	GaduContact *contact = NULL;
	gchar *nickname = NULL;

	g_return_val_if_fail (GADU_IS_CONTACT_LIST (self), NULL);
	
	contact = g_hash_table_lookup (self->priv->contacts,
				       GUINT_TO_POINTER (handle));
	
	if (contact) {
		g_object_get (contact, "nickname", &nickname, NULL);
	}
	
	if (nickname == NULL) {
		TpBaseConnection *base_conn = NULL;
		TpHandleRepoIface *contact_repo = NULL;
		
		base_conn = tp_base_contact_list_get_connection (TP_BASE_CONTACT_LIST (self), NULL);
		contact_repo = tp_base_connection_get_handles (base_conn, TP_HANDLE_TYPE_CONTACT);
		
		nickname = g_strdup (tp_handle_inspect (contact_repo, handle));
	}
	
	return nickname;
}

static TpHandleSet *
gadu_contact_list_dup_contacts (TpBaseContactList *base)
{
	GaduContactList *self = GADU_CONTACT_LIST (base);
	GList *key = NULL, *keys = NULL;
	TpBaseConnection *base_conn;
	TpHandleRepoIface *contact_repo;
	TpHandleSet *handles;
	
	base_conn = tp_base_contact_list_get_connection (base, NULL);
	contact_repo = tp_base_connection_get_handles (base_conn, TP_HANDLE_TYPE_CONTACT);
	
	handles = tp_handle_set_new (contact_repo);
	
	keys = g_hash_table_get_keys (self->priv->contacts);
	
	for (key = keys; key != NULL; key = key->next) {
		tp_handle_set_add (handles, GPOINTER_TO_UINT (key->data));
	}
	
	g_list_free (keys);
	
	return handles;
}

static void
gadu_contact_list_dup_states (TpBaseContactList *base,
			      TpHandle handle,
			      TpSubscriptionState *subscribe,
			      TpSubscriptionState *publish,
			      gchar **publish_request)
{
	GaduContactList *self = GADU_CONTACT_LIST (base);

	if (g_hash_table_contains (self->priv->contacts, GUINT_TO_POINTER (handle))) {
		if (subscribe)
			*subscribe = TP_SUBSCRIPTION_STATE_YES;
		if (publish)
			*publish = TP_SUBSCRIPTION_STATE_YES;
		if (publish_request)
			*publish_request = g_strdup ("");
	} else {
		if (subscribe)
			*subscribe = TP_SUBSCRIPTION_STATE_NO;
		if (publish)
			*publish = TP_SUBSCRIPTION_STATE_NO;
		if (publish_request)
			*publish_request = NULL;
	}
}

typedef struct {
	GSimpleAsyncResult *result;
	GaduContactList	   *self;
	gulong		    handler_id;
} DownloadAsyncCtx;

static void
userlist_received_cb (GaduConnection *conn, struct gg_event *evt, DownloadAsyncCtx *ctx)
{
	TpHandleRepoIface *contact_repo = tp_base_connection_get_handles (
      						TP_BASE_CONNECTION (conn),
      						TP_HANDLE_TYPE_CONTACT);
      	GaduContactListPrivate *priv = ctx->self->priv;
      	const gchar *userlist = NULL;
      	TpHandle handle;

	g_signal_handler_disconnect (conn, ctx->handler_id);

	userlist = evt->event.userlist100_reply.reply;
				     
	gadu_debug ("User list:\n%s", userlist);
	
	gchar **data = g_strsplit (userlist, "\r\n", -1);
	guint i;
	
	g_hash_table_remove_all (priv->contacts);
	g_hash_table_remove_all (priv->groups);
	
	GArray *uins = g_array_new (TRUE, TRUE, sizeof(uin_t));
	
	for (i = 0; data[i] != NULL; i++) {
		GaduContact *contact = NULL;
		gchar **user_record = g_strsplit (data[i], ";", 12);
		gchar **user_groups = NULL;
		gint groupid;
		
		if (g_strv_length (user_record) < 12) {
			g_strfreev (user_record);
			continue;
		}
		
		user_groups = g_strsplit (user_record[GG_FIELD_GROUPS], ",", -1);
		
		contact = gadu_contact_new (user_record[GG_FIELD_UIN]);
		
		g_object_set (contact,
			      "nickname", user_record[GG_FIELD_NICKNAME],
			      "phone", user_record[GG_FIELD_PHONE],
			      "email", user_record[GG_FIELD_EMAIL],
			      "groups", user_groups,
			      NULL);
		
		uin_t uin = (uin_t)atoi (user_record[GG_FIELD_UIN]);
		g_array_append_val (uins, uin);
		
		handle = tp_handle_ensure (contact_repo, user_record[GG_FIELD_UIN], NULL, NULL);
		g_hash_table_insert (priv->contacts, GUINT_TO_POINTER (handle), contact);
		
		gint ngroups = g_strv_length (user_groups);
		for (groupid = 0; groupid < ngroups; groupid++) {
			TpHandleSet *groupset = NULL;
			
			groupset = g_hash_table_lookup (priv->groups, user_groups[groupid]);
			
			if (groupset) {
				tp_handle_set_add (groupset, handle);
			} else {
				groupset = tp_handle_set_new_containing (contact_repo, handle);
				g_hash_table_insert (priv->groups,
						     g_strdup (user_groups[groupid]),
						     groupset);
			}
		}
		
		g_strfreev (user_groups);
		g_strfreev (user_record);
	}
	
	gg_notify (conn->session, (uin_t *) uins->data, uins->len);
	
	g_array_free (uins, TRUE);
	
	g_strfreev (data);

	tp_base_contact_list_set_list_received (TP_BASE_CONTACT_LIST (ctx->self));
	
	g_simple_async_result_complete (ctx->result);
	g_object_unref (ctx->result);
	g_object_unref (ctx->self);
	g_free (ctx);
}

static void
gadu_contact_list_download_async (TpBaseContactList *base,
				  GAsyncReadyCallback callback,
				  gpointer data)
{
	TpBaseConnection *base_conn = tp_base_contact_list_get_connection (base, NULL);
	GaduConnection *conn = GADU_CONNECTION (base_conn);
	DownloadAsyncCtx *ctx = g_new0 (DownloadAsyncCtx, 1);
	GSimpleAsyncResult *result;
	gadu_debug ("Sending userlist request");
	ctx->self = g_object_ref (GADU_CONTACT_LIST (base));
	ctx->result = g_simple_async_result_new (G_OBJECT (base),
						 callback,
						 data,
						 gadu_contact_list_download_async);
	
	ctx->handler_id = g_signal_connect (conn, "userlist-received", G_CALLBACK (userlist_received_cb), ctx);
	
	gg_userlist100_request (conn->session, GG_USERLIST100_GET, 0, GG_USERLIST100_FORMAT_TYPE_GG70, NULL);
}

static void
gadu_contact_list_init (GaduContactList *self)
{
	self->priv = GADU_CONTACT_LIST_GET_PRIVATE (self);

	self->priv->contacts = g_hash_table_new_full (g_direct_hash,
						      g_direct_equal,
						      NULL,
						      g_object_unref);
	self->priv->groups = g_hash_table_new_full (g_str_hash,
						    g_str_equal,
						    g_free,
						    (GDestroyNotify)tp_handle_set_destroy);
}
static void on_download_finished (GObject *obj, GAsyncResult *result, gpointer user_data)
{
	TpBaseContactList *base = TP_BASE_CONTACT_LIST (obj);

	if (!tp_base_contact_list_download_finish (base, result, NULL))
		gadu_error ("Failed to get contacts list");
}
static void
connection_status_changed_cb (GaduConnection *conn,
			      guint status,
			      guint reason,
			      GaduContactList *self)
{
	switch (status) {
		case TP_CONNECTION_STATUS_CONNECTED:
			gadu_contact_list_download_async (TP_BASE_CONTACT_LIST (self),
							  on_download_finished,
							  NULL);
			break;
	}
}

static GStrv
gadu_contact_list_dup_groups (TpBaseContactList *base)
{
	GaduContactList *self = GADU_CONTACT_LIST (base);
	GPtrArray *result = NULL;
	GList *item = NULL, *keys = NULL;
	
	result = g_ptr_array_sized_new (1);
	keys = g_hash_table_get_keys (self->priv->groups);
	
	for (item = keys; item; item = item->next) {
		g_ptr_array_add (result, g_strdup (item->data));
	}
	
	g_ptr_array_add (result, NULL);
	
	g_list_free (keys);
	
	return (GStrv) g_ptr_array_free (result, FALSE);
}

static TpHandleSet *
gadu_contact_list_dup_group_members (TpBaseContactList *base,
				     const gchar *group)
{
	GaduContactList *self = GADU_CONTACT_LIST (base);
	TpHandleSet *members = NULL;
	TpHandleSet *result = NULL;
	
	members = g_hash_table_lookup (self->priv->groups, group);
	
	if (members) {
		result = tp_handle_set_copy (members);
	}
	
	return result;
}

static GStrv
gadu_contact_list_dup_contact_groups (TpBaseContactList *base,
				      TpHandle handle)
{
	GaduContactList *self = GADU_CONTACT_LIST (base);
	GaduContact *contact = NULL;
	gchar **result = NULL;
	
	contact = g_hash_table_lookup (self->priv->contacts, GUINT_TO_POINTER (handle));
	
	if (contact) {
		g_object_get (contact, "groups", &result, NULL);
	}
	
	return result;
}

static void
gadu_contact_list_constructed (GObject *object)
{
	GaduContactList *self = GADU_CONTACT_LIST (object);
	TpBaseContactList *base = TP_BASE_CONTACT_LIST (object);
	GaduConnection *conn;
	
	void (*parent_constructed) (GObject *) = G_OBJECT_CLASS (gadu_contact_list_parent_class)->constructed;
	
	if (parent_constructed != NULL)
		parent_constructed (object);
	
	conn = GADU_CONNECTION (tp_base_contact_list_get_connection (base, NULL));
	
	self->priv->status_changed_id = g_signal_connect (conn, "status-changed",
							  G_CALLBACK (connection_status_changed_cb),
							  self);
}

static void
gadu_contact_list_dispose (GObject *obj)
{
	GaduContactList *self = GADU_CONTACT_LIST (obj);
	GaduContactListPrivate *priv = self->priv;
	
	if (priv->contacts) {
		g_hash_table_unref (priv->contacts);
		priv->contacts = NULL;
	}
	
	if (priv->groups) {
		g_hash_table_unref (priv->groups);
		priv->groups = NULL;
	}

	G_OBJECT_CLASS (gadu_contact_list_parent_class)->dispose (obj);
}

static void
gadu_contact_list_class_init (GaduContactListClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	TpBaseContactListClass *base_class = TP_BASE_CONTACT_LIST_CLASS (klass);

	object_class->constructed = gadu_contact_list_constructed;
	object_class->dispose = gadu_contact_list_dispose;

	g_type_class_add_private (klass, sizeof (GaduContactListPrivate));
	
	base_class->dup_contacts = gadu_contact_list_dup_contacts;
	base_class->dup_states = gadu_contact_list_dup_states;
	base_class->download_async = gadu_contact_list_download_async;

}

static void
gadu_contact_list_group_list_iface_init (TpContactGroupListInterface *iface)
{
	iface->dup_groups = gadu_contact_list_dup_groups;
	iface->dup_group_members = gadu_contact_list_dup_group_members;
	iface->dup_contact_groups = gadu_contact_list_dup_contact_groups;
}

GaduContactList *
gadu_contact_list_new (GaduConnection *connection)
{
	return g_object_new (GADU_TYPE_CONTACT_LIST,
			     "connection", connection,
			     NULL);
}

