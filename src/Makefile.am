INCLUDES = \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(EVAS_CFLAGS) \
	$(EDJE_CFLAGS) \
	$(ELEMENTARY_CFLAGS) \
	$(FSO_CFLAGS) \
	$(PHONEUTILS_CFLAGS) \
	$(MOKOSUITE_CFLAGS) \
	-DLOCALEDIR=\""$(localedir)"\" \
	-DDATADIR=\""$(datadir)"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-Wall

bin_PROGRAMS = mokomessages

mokomessages_SOURCES = \
	main.c \
	globals.h \
	threadwin.c \
	threadwin.h \
	msglist.c \
	msglist.h

mokomessages_LDADD = \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(EVAS_LIBS) \
	$(EDJE_LIBS) \
	$(ELEMENTARY_LIBS) \
	$(FSO_LIBS) \
	$(PHONEUTILS_LIBS) \
	$(MOKOSUITE_LIBS)

MAINTAINERCLEANFILES = \
	Makefile.in
