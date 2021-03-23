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
HAVE_GIFLIB = -lgif
HAVE_LIBEXIF = -lexif

# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2
#FREETYPEINC = $(PREFIX)/include/freetype2
# OpenBSD (uncomment)
#FREETYPEINC = /usr/X11R6/include/freetype2

# includes and libs
INCS = -I$(FREETYPEINC)
ldlibs = $(LDLIBS) -lImlib2 -lX11 $(FREETYPELIBS) $(HAVE_LIBEXIF) $(HAVE_GIFLIB)

# flags
cflags = -std=c99 -Wall -pedantic $(CFLAGS)
cppflags = -I. $(CPPFLAGS) -DVERSION=\"$(VERSION)\" -D_XOPEN_SOURCE=700 \
  -DHAVE_GIFLIB=$(HAVE_GIFLIB) -DHAVE_LIBEXIF=$(HAVE_LIBEXIF) $(INCS)

# compiler and linker
CC = cc
