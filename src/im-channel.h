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

#ifndef __GADU_IM_CHANNEL_H
#define __GADU_IM_CHANNEL_H

#include <glib-object.h>
#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

#define GADU_TYPE_IM_CHANNEL		(gadu_im_channel_get_type ())
#define GADU_IM_CHANNEL(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GADU_TYPE_IM_CHANNEL, GaduImChannel))
#define GADU_IS_IM_CHANNEL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GADU_TYPE_IM_CHANNEL))
#define GADU_IM_CHANNEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GADU_TYPE_IM_CHANNEL, GaduImChannelClass))
#define GADU_IS_IM_CHANNEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GADU_TYPE_IM_CHANNEL))
#define GADU_IM_CHANNEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GADU_TYPE_IM_CHANNEL, GaduImChannelClass))


typedef struct _GaduImChannel		GaduImChannel;
typedef struct _GaduImChannelClass	GaduImChannelClass;
typedef struct _GaduImChannelPrivate	GaduImChannelPrivate;

struct _GaduImChannel
{
	TpBaseChannel	parent;
	TpMessageMixin	message_mixin;
	
	GaduImChannelPrivate *priv;
};

struct _GaduImChannelClass
{
	TpBaseChannelClass parent_class;
};


GType		 gadu_im_channel_get_type	(void);
GaduImChannel	*gadu_im_channel_new		(GaduConnection *connection,
						 TpHandle handle,
						 TpHandle initiator,
						 gboolean requested);
void		 gadu_im_channel_receive	(GaduImChannel *self,
						 time_t timestamp,
			 			 const gchar *text);
void		 gadu_im_channel_type_notify	(GaduImChannel *self,
						 gint length);

G_END_DECLS

#endif /* __GADU_IM_CHANNEL_H */
