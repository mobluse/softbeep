PREFIX=/usr/local
SO=libsoftbeep.so
SOURCES=libsoftbeep.c
CFLAGS=-Wall -g -O2
VERSION=0.3
TAR=softbeep-$(VERSION).tar.gz
PACKAGE=softbeep

all: $(SO) softbeep

$(SO): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(SO) -DPREFIX=$(PREFIX) -shared -rdynamic -ldl

sbtest: test.c
	$(CC) $(CFLAGS) test.c -o sbtest

clean:
	rm -f $(SO) *~ sbtest softbeep README debug
	rm -f *.tar.gz

softbeep: softbeep.in
	PREFIX="$(PREFIX)" perl -pe 's/__PREFIX__/$$ENV{"PREFIX"}/g' < softbeep.in > softbeep
	chmod ug+rx softbeep

install: all
	install -groot -m755 -oroot softbeep $(PREFIX)/bin
	install -groot -m755 -oroot sb-beep $(PREFIX)/bin
	install -groot -m644 -oroot -s $(SO) $(PREFIX)/lib

README: README.in
	sed s/VERSION/$(VERSION)/ < README.in > README

$(TAR): clean README
	tar -C.. --exclude=$(PACKAGE)/$(TAR) -czvf $(TAR) $(PACKAGE)

tar: $(TAR)

web: $(TAR) README
	cp README $(TAR) ../../homepage/lennart/projects/$(PACKAGE)/ && $(MAKE) -C ../../homepage/lennart/projects/$(PACKAGE)

upload: web
	$(MAKE) -C ../../homepage/lennart upload


.PHONY: all clean install tar web upload 
