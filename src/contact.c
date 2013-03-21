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

#include <glib.h>

#include "contact.h"


static void gadu_contact_finalize (GObject *object);


G_DEFINE_TYPE (GaduContact, gadu_contact, G_TYPE_OBJECT)

#define GADU_CONTACT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GADU_TYPE_CONTACT, GaduContactPrivate))

struct _GaduContactPrivate {
	gchar *id;
	gchar *nickname;
	gchar *phone;
	gchar *email;
	gchar **groups;
};

enum {
	PROP_ID = 1,
	PROP_NICKNAME = 2,
	PROP_PHONE = 3,
	PROP_EMAIL = 4,
	PROP_GROUPS = 5,
	PROP_LAST
};

static void
gadu_contact_init (GaduContact *self)
{
	self->priv = GADU_CONTACT_GET_PRIVATE (self);
}

static void
gadu_contact_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GaduContact *self = GADU_CONTACT (object);
	GaduContactPrivate *priv = self->priv;
	
	switch (prop_id) {
		case PROP_ID:
			g_value_set_string (value, priv->id);
			break;
		case PROP_NICKNAME:
			g_value_set_string (value, priv->nickname);
			break;
		case PROP_PHONE:
			g_value_set_string (value, priv->phone);
			break;
		case PROP_EMAIL:
			g_value_set_string (value, priv->email);
			break;
		case PROP_GROUPS:
			g_value_set_boxed (value, priv->groups);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
gadu_contact_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GaduContact *self = GADU_CONTACT (object);
	GaduContactPrivate *priv = self->priv;
	
	switch (prop_id) {
		case PROP_ID:
			g_free (priv->id);
			priv->id = g_value_dup_string (value);
			break;
		case PROP_NICKNAME:
			g_free (priv->nickname);
			priv->nickname = g_value_dup_string (value);
			break;
		case PROP_PHONE:
			g_free (priv->phone);
			priv->phone = g_value_dup_string (value);
			break;
		case PROP_EMAIL:
			g_free (priv->email);
			priv->email = g_value_dup_string (value);
			break;
		case PROP_GROUPS:
			g_strfreev (priv->groups);
			priv->groups = g_value_dup_boxed (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
gadu_contact_class_init (GaduContactClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GParamSpec *pspec;

	object_class->get_property = gadu_contact_get_property;
	object_class->set_property = gadu_contact_set_property;
	object_class->finalize = gadu_contact_finalize;

	pspec = g_param_spec_string ("id", NULL,
				     "The contact uin number, e.g. '123456'",
				     NULL,
				     G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE |
				     G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_ID, pspec);

	pspec = g_param_spec_string ("nickname", NULL,
				     "The contact nickname",
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_NICKNAME, pspec);

	pspec = g_param_spec_string ("phone", NULL,
				     "The contact phone number",
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_PHONE, pspec);

	pspec = g_param_spec_string ("email", NULL,
				     "The contact e-mail address",
				     NULL,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_EMAIL, pspec);
	
	pspec = g_param_spec_boxed ("groups", NULL,
				    "List of groups to which the contact belongs",
				    G_TYPE_STRV,
				    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_GROUPS, pspec);

	g_type_class_add_private (klass, sizeof (GaduContactPrivate));
}

static void
gadu_contact_finalize (GObject *object)
{
	GaduContact *self = GADU_CONTACT (object);
	GaduContactPrivate *priv = self->priv;
	
	g_free (priv->id);
	g_free (priv->nickname);
	g_free (priv->phone);
	g_free (priv->email);
	
	g_strfreev (priv->groups);
	
	G_OBJECT_CLASS (gadu_contact_parent_class)->finalize (object);
}

GaduContact *
gadu_contact_new (const gchar *id)
{
	GaduContact *contact;
	
	contact = g_object_new (GADU_TYPE_CONTACT, "id", id, NULL);
	
	return GADU_CONTACT (contact);
}

