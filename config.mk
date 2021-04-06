# sxiv version (adding +1 on every big change I make)
VERSION = 26.25

# Customize below to fit your preferences

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# autoreload backend (INOTIFY/NOP)
AUTORELOAD = INOTIFY

# giflib and libexif, comment if you don't want them
HAVE_GIFLIB = -lgif
GIFLIBFLAGS = -DHAVE_GIFLIB
HAVE_LIBEXIF = -lexif
LIBEXIFFLAGS = -DHAVE_LIBEXIF

# Prefix-keys, see readme
#ENABLE_PREFIX_KEYS = -DENABLE_PREFIX_KEYS

# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2
# OpenBSD (uncomment)
#FREETYPEINC = /usr/X11R6/include/freetype2

# includes and libs
INCS = -I$(FREETYPEINC)
LIBS = $(LDLIBS) -lImlib2 -lX11 $(FREETYPELIBS) $(HAVE_LIBEXIF) $(HAVE_GIFLIB)

# flags
SXIVCPPFLAGS = -D_XOPEN_SOURCE=700 -DVERSION=\"$(VERSION)\" $(GIFLIBFLAGS) -DAUTO_$(AUTORELOAD) $(LIBEXIFFLAGS) $(ENABLE_PREFIX_KEYS)
SXIVCFLAGS = -std=c99 -pedantic -Wall $(INCS) $(SXIVCPPFLAGS) $(CPPFLAGS) $(CFLAGS)
SXIVLDFLAGS = $(LIBS) $(LDFLAGS)

# compiler and linker
CC = gcc
#CC = cc
