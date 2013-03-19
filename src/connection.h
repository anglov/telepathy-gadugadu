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

#ifndef __GADU_CONNECTION_H
#define __GADU_CONNECTION_H

#include <glib-object.h>
#include <telepathy-glib/telepathy-glib.h>
#include <libgadu.h>

G_BEGIN_DECLS

#define GADU_TYPE_CONNECTION		(gadu_connection_get_type ())
#define GADU_CONNECTION(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GADU_TYPE_CONNECTION, GaduConnection))
#define GADU_IS_CONNECTION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GADU_TYPE_CONNECTION))
#define GADU_CONNECTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GADU_TYPE_CONNECTION, GaduConnectionClass))
#define GADU_IS_CONNECTION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GADU_TYPE_CONNECTION))
#define GADU_CONNECTION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GADU_TYPE_CONNECTION, GaduConnectionClass))


typedef struct _GaduConnection		GaduConnection;
typedef struct _GaduConnectionClass	GaduConnectionClass;
typedef struct _GaduConnectionPrivate	GaduConnectionPrivate;
typedef struct _GaduConnectionPresencePrivate GaduConnectionPresencePrivate;

struct _GaduConnection
{
	TpBaseConnection		 parent;
	TpContactsMixin			 contacts_mixin;
	TpPresenceMixin			 presence_mixin;
	
	GaduConnectionPrivate		*priv;
	
	GaduConnectionPresencePrivate   *presence_priv;
	
	struct gg_session		*session;
};

struct _GaduConnectionClass
{
	TpBaseConnectionClass	parent_class;
	TpContactsMixinClass	contacts_mixin;
	TpPresenceMixinClass	presence_mixin;
};


GType			 gadu_connection_get_type	(void);
TpBaseConnection	*gadu_connection_new		(const gchar *account,
							 const gchar *password,
							 const gchar *protocol);

const gchar * const	*gadu_connection_get_implemented_interfaces (void);

TpBaseContactList *gadu_connection_get_contact_list (GaduConnection *self);
const TpPresenceStatusSpec *gadu_connection_get_statuses (void);

G_END_DECLS

#endif /* __GADU_CONNECTION_H */

