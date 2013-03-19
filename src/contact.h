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

#ifndef __GADU_CONTACT_H
#define __GADU_CONTACT_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GADU_TYPE_CONTACT		(gadu_contact_get_type ())
#define GADU_CONTACT(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GADU_TYPE_CONTACT, GaduContact))
#define GADU_IS_CONTACT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GADU_TYPE_CONTACT))
#define GADU_CONTACT_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GADU_TYPE_CONTACT, GaduContactClass))
#define GADU_IS_CONTACT_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GADU_TYPE_CONTACT))
#define GADU_CONTACT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GADU_TYPE_CONTACT, GaduContactClass))

typedef struct _GaduContact		GaduContact;
typedef struct _GaduContactClass	GaduContactClass;
typedef struct _GaduContactPrivate	GaduContactPrivate;

struct _GaduContact
{
	GObject parent;
	
	GaduContactPrivate *priv;
};

struct _GaduContactClass
{
	GObjectClass parent_class;
};

GType		gadu_contact_list_get_type	(void);

GaduContact    *gadu_contact_new		(const gchar *id);

G_END_DECLS

#endif /* __GADU_CONTACT_H */

