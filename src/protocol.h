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

#ifndef __GADU_PROTOCOL_H
#define __GADU_PROTOCOL_H

#include <glib-object.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

#define GADU_TYPE_PROTOCOL		(gadu_protocol_get_type ())
#define GADU_PROTOCOL(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GADU_TYPE_PROTOCOL, GaduProtocol))
#define GADU_IS_PROTOCOL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GADU_TYPE_PROTOCOL))
#define GADU_PROTOCOL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GADU_TYPE_PROTOCOL, GaduProtocolClass))
#define GADU_IS_PROTOCOL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GADU_TYPE_PROTOCOL))
#define GADU_PROTOCOL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GADU_TYPE_PROTOCOL, GaduProtocolClass))


typedef struct _GaduProtocol		GaduProtocol;
typedef struct _GaduProtocolClass	GaduProtocolClass;

struct _GaduProtocol
{
	TpBaseProtocol parent;
};

struct _GaduProtocolClass
{
	TpBaseProtocolClass parent_class;
};


GType		 gadu_protocol_get_type	(void);
TpBaseProtocol	*gadu_protocol_new	(void);


G_END_DECLS

#endif /* __GADU_PROTOCOL_H */

