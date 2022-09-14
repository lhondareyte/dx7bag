#
#	Makefile for dx7bag
#	AYM 2002-07-26
#


# Variables that you may want to change
PRG     = dx7getb
PREFIX  = /usr/local
OBJECTS = common.o dx7getb.o 

# Variables that you don't want to change
VERSION != cat VERSION
CFLAGS  = -Wall -O2


all: version.h dx7getb doc

$(PRG): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

version.h: 
	@printf 'const char version[] = "%s";\n' "$(VERSION)" >$@ || rm -f $@

doc: 
	scripts/process man/$(PRG).1 > $(PRG).1

install: all
	install -m 755  $(PRG) $(PREFIX)/bin/
	install -m 644  $(PRG).1 $(PREFIX)/man/man1/

clean:
	rm -f $(OBJECTS) version.h $(PRG) $(PRG).1

.PHONY man
