# See LICENSE file for copyright and license details.

include config.mk

SRC = $(AUTORELOAD).c commands.c image.c main.c options.c thumbs.c util.c window.c
OBJ = $(SRC:.c=.o)

all: options sxiv

options:
	@echo sxiv build options:
	@echo "AUTORELOAD   = $(AUTORELOAD)"
	@echo "HAVE_GIFLIB  = $(HAVE_GIFLIB)"
	@echo "HAVE_LIBEXIF = $(HAVE_LIBEXIF)"
	@echo "cflags       = $(cflags)"
	@echo "cppflags     = $(cppflags)"
	@echo "CC           = $(CC)"

#.SUFFIXES:
#.SUFFIXES: .c .o
#$(V).SILENT:

sxiv: $(OBJ)
	$(CC) -o $@ $(OBJ) $(ldlibs) $(LDFLAGS)

$(OBJ): commands.lst sxiv.h config.h config.mk
#options.o: version.h
window.o: icon/data.h

.c.o:
	$(CC) $(cflags) $(cppflags) -c -o $@ $<

config.h:
	cp config.def.h $@

#version.h: Makefile .git/index
#	@echo "GEN $@"
#	v="$$(cd $(srcdir); git describe 2>/dev/null)"; \
#	echo "#define VERSION \"$$(v:-$(version))\"" >$@

.git/index:

clean:
	rm -f sxiv $(OBJ) sxiv-$(VERSION).tar.gz

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f sxiv $(DESTDIR)$(PREFIX)/bin/
	chmod 755 $(DESTDIR)$(PREFIX)/bin/sxiv
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s!PREFIX!$(PREFIX)!g; s!VERSION!$(VERSION)!g" sxiv.1 >$(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/sxiv.1
	mkdir -p $(DESTDIR)$(PREFIX)/share/sxiv/exec
	cp exec/* $(DESTDIR)$(PREFIX)/share/sxiv/exec/
	chmod 755 $(DESTDIR)$(PREFIX)/share/sxiv/exec/*

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/sxiv\
		rm -f $(DESTDIR)$(MANPREFIX)/man1/sxiv.1\
		rm -rf $(DESTDIR)$(PREFIX)/share/sxiv

.PHONY: all clean install uninstall
