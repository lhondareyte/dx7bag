#
#	Makefile for dx7bag
#	AYM 2002-07-26
#


# Variables that you may want to change
PREFIX = /usr/local
OBJECTS = common.o dx7getb.o 

# Variables that you don't want to change
VERSION != cat VERSION
ARC      = dx7bag-$(VERSION)
CFLAGS  = -Wall -O2


all: version.h dx7getb doc/dx7getb.1

dx7getb: $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

install:
	install -m 755  dx7getb $(PREFIX)/bin/
	install -m 644  doc/dx7getb.1 $(PREFIX)/man/man1/

clean:
	rm -f $(OBJECTS) version.h

doc/dx7getb.1: docsrc/dx7getb.1 VERSION scripts/process
	scripts/process docsrc/dx7getb.1 >$@


version.h: 
	@printf 'const char version[] = "%s";\n' "$(VERSION)" >$@ || rm -f $@

