NULL =

AM_CPPFLAGS =						\
	$(DBUS_CFLAGS)					\
	$(GLIB_CFLAGS)					\
	$(TP_GLIB_CFLAGS)				\
	$(LIBGADU_CFLAGS)				\
	-DG_LOG_DOMAIN=\"gadugadu\"			\
	$(NULL)

libexec_PROGRAMS = telepathy-gadugadu

telepathy_gadugadu_SOURCES =				\
	connection.c					\
	connection.h					\
	connection-aliasing.c				\
	connection-aliasing.h				\
	connection-manager.c				\
	connection-manager.h				\
	connection-presence.c				\
	connection-presence.h				\
	contact.c					\
	contact.h					\
	contact-list.c					\
	contact-list.h					\
	debug.c						\
	debug.h						\
	im-channel.c					\
	im-channel.h					\
	im-manager.c					\
	im-manager.h					\
	main.c						\
	protocol.c					\
	protocol.h					\
	$(NULL)

telepathy_gadugadu_CFLAGS =				\
	$(WARNINGFLAGS)					\
	$(NULL)

telepathy_gadugadu_LDADD =				\
	$(DBUS_LIBS)					\
	$(GLIB_LIBS)					\
	$(TP_GLIB_LIBS)					\
	$(LIBGADU_LIBS)					\
	$(NULL)

