# sxiv version (adding +1 on every big change I make)
VERSION = 26.1

# Customize below to fit your preferences

# paths
#srcdir = .
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# autoreload backend (inotify/nop)
AUTORELOAD = inotify

# giflib and libexif, comment if you don't want them
# why the "HAVE_", remove them next commit?
HAVE_GIFLIB = -lgif
GIFLIBFLAGS = -DHAVE_GIFLIB
HAVE_LIBEXIF = -lexif
LIBEXIFFLAGS = -DHAVE_LIBEXIF
#HAVE_ = -DHAVE_GIFLIB -DHAVE_LIBEXIF

# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2
#FREETYPEINC = $(PREFIX)/include/freetype2
# OpenBSD (uncomment)
#FREETYPEINC = /usr/X11R6/include/freetype2

# includes and libs
INCS = -I$(FREETYPEINC)
LIBS = $(LDLIBS) -lImlib2 -lX11 $(FREETYPELIBS) $(HAVE_LIBEXIF) $(HAVE_GIFLIB)
#ldlibs = $(LDLIBS) -lImlib2 -lX11 $(FREETYPELIBS) $(HAVE_LIBEXIF) $(HAVE_GIFLIB)

# flags
SXIVCPPFLAGS = -DVERSION=\"$(VERSION)\" -D_XOPEN_SOURCE=700 $(GIFLIBFLAGS) $(LIBEXIFFLAGS) $(INCS)
SXIVCFLAGS = -std=c99 -pedantic -Wall $(INCS) $(SXIVCPPFLAGS) $(CPPFLAGS) $(CFLAGS)
SXIVLDFLAGS = $(LIBS) $(LDFLAGS)
#cflags = -std=c99 -Wall -pedantic $(CFLAGS)
#cppflags = -I. $(CPPFLAGS) -DVERSION=\"$(VERSION)\" -D_XOPEN_SOURCE=700 \
  $(GIFLIBFLAGS) $(LIBEXIFFLAGS) $(INCS)

# compiler and linker
CC = cc
