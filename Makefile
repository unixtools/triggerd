VERSION=1.6

SBINDIR=/usr/sbin
CC=gcc -O2 -Wall

all: triggerd

clean:
	rm -f *.o *.tar.gz triggerd

triggerd: triggerd.o tcp.o util.o debug.o
	$(CC) -o triggerd triggerd.o tcp.o util.o debug.o -lpthread

install:
	mkdir -p $(SBINDIR)
	install -m0755 triggerd $(SBINDIR)/triggerd

dist: clean
	rm -rf /tmp/triggerd-$(VERSION)
	mkdir /tmp/triggerd-$(VERSION)
	cp -pr . /tmp/triggerd-$(VERSION)
	cd /tmp/triggerd-$(VERSION) && rm -rf *.gz .git .gitignore
	tar -C/tmp -czvf triggerd-$(VERSION).tar.gz triggerd-$(VERSION)
	rm -rf /tmp/triggerd-$(VERSION)

indent:
	indent -linux -nut -ts4 -i4 *.c *.h
	rm -f *~
