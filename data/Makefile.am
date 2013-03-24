EXTRA_DIST = gadugadu.service.in gadugadu.manager

servicedir = $(datadir)/dbus-1/services
service_DATA = org.freedesktop.Telepathy.ConnectionManager.gadugadu.service

managerdir = $(datadir)/telepathy/managers
manager_DATA = gadugadu.manager

org.freedesktop.Telepathy.ConnectionManager.gadugadu.service: gadugadu.service.in \
		Makefile
	sed -e "s|@libexecdir@|$(libexecdir)|" $< > $@

CLEANFILES = $(service_DATA)

