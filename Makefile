# See LICENSE file for copyright and license details.

include config.mk

SRC = autoreload.c commands.c image.c main.c options.c thumbs.c util.c window.c
OBJ = $(SRC:.c=.o)

all: options sxiv

options:
	@echo sxiv build options:
	@echo "CFLAGS  = $(SXIVCFLAGS)"
	@echo "LDFLAGS = $(SXIVLDFLAGS)"
	@echo "CC      = $(CC)"

config.h:
	cp config.def.h $@


.c.o:
	$(CC) $(SXIVCFLAGS) -c -o $@ $<

window.o: icon/data.h

$(OBJ): commands.lst sxiv.h config.h config.mk

sxiv: $(OBJ)
	$(CC) -o $@ $(OBJ) $(SXIVLDFLAGS)

clean:
	rm -f sxiv $(OBJ) sxiv-$(VERSION).tar.gz *.o *.orig *.rej

dist: clean
	mkdir -p sxiv-${VERSION}
	cp -R LICENSE Makefile README config.def.h config.mk\
		sxiv.1 sxiv.h util8.h util.h ${SRC} sxiv.png sxiv-${VERSION}
	tar -cf sxiv-${VERSION}.tar sxiv-${VERSION}
	gzip sxiv-${VERSION}.tar
	rm -rf sxiv-${VERSION}

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
