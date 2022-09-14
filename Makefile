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


all: version.h dx7getb doc/dx7getb.1

install:
	cp dx7getb $(PREFIX)/bin/
	touch -r dx7getb $(PREFIX)/bin/dx7getb
	cp doc/dx7getb.1 $(PREFIX)/man/man1/
	touch -r doc/dx7getb.1 $(PREFIX)/man/man1/dx7getb.1

clean:
	rm -f $(OBJECTS) version.h

dist:
	rm -rf $(ARC)
	mkdir $(ARC)
	tar cf -		\
	    CHANGES		\
	    COPYING		\
	    Makefile		\
	    README		\
	    TODO		\
	    VERSION		\
	    common.c		\
	    common.h		\
	    doc/dx7getb.1	\
	    docsrc/dx7getb.1	\
	    dx7getb.c		\
	    scripts/process	\
	    version.c		\
	  | (cd $(ARC) && tar xf -)
	tar -cf - $(ARC) | gzip >$(ARC).tar.gz


common.o: common.h

dx7getb: $(OBJECTS)

doc/dx7getb.1: docsrc/dx7getb.1 VERSION scripts/process
	scripts/process docsrc/dx7getb.1 >$@

dx7getb.o: common.h

version.h: VERSION
	printf 'const char version[] = "%s";\n' "$(VERSION)" >$@ || rm -f $@

