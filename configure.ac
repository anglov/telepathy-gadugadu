#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([telepathy-gadugadu], [0.1.0], [marcin.banasiak@gmail.com])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz])

AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC
AC_HEADER_STDC
AM_PROG_CC_C_O

if test "$GCC" = "yes"; then
	WARNINGFLAGS="$WARNINGFLAGS -Wall"
	WARNINGFLAGS="$WARNINGFLAGS -Wformat-security"
fi
AC_SUBST(WARNINGFLAGS)

# Checks for libraries.
PKG_CHECK_MODULES(GLIB, [glib-2.0 gobject-2.0 gio-2.0])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(TP_GLIB, [telepathy-glib >= 0.20.0])
AC_SUBST(TP_GLIB_CFLAGS)
AC_SUBST(TP_GLIB_LIBS)

PKG_CHECK_MODULES(DBUS, [dbus-1 >= 1.1.0 dbus-glib-1 >= 0.82])
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(LIBGADU, [libgadu >= 0.11.2])
AC_SUBST(LIBGADU_CFLAGS)
AC_SUBST(LIBGADU_LIBS)

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([
Makefile
data/Makefile
src/Makefile
])
AC_OUTPUT
