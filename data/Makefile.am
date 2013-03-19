EXTRA_DIST = gadugadu.service.in

servicedir = $(datadir)/dbus-1/services
service_DATA = org.freedesktop.Telepathy.ConnectionManager.gadugadu.service

org.freedesktop.Telepathy.ConnectionManager.gadugadu.service: gadugadu.service.in \
		Makefile
	sed -e "s/@libexecdir@/$(libexecdir)/" $< > $@

CLEANFILES = $(service_DATA)

