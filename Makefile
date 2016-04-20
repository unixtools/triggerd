VERSION=1.6

SBINDIR=/usr/sbin
CC=gcc -O2 -Wall
TOP=`pwd`


all: triggerd

clean:
	rm -f *.o *.tar.gz triggerd

triggerd: triggerd.o tcp.o util.o debug.o
	$(CC) -o triggerd triggerd.o tcp.o util.o debug.o -lpthread

install:
	mkdir -p $(SBINDIR)
	install -m0755 triggerd $(SBINDIR)/triggerd

dist:
	rm -rf /tmp/triggerd-$(VERSION)
	mkdir /tmp/triggerd-$(VERSION)
	cp -pr . /tmp/triggerd-$(VERSION)
	cd /tmp/triggerd-$(VERSION) && rm -rf *.gz .git .gitignore
	tar -C/tmp -czvf ../triggerd-$(VERSION).tar.gz triggerd-$(VERSION)
	rm -rf /tmp/triggerd-$(VERSION)

deb: dist
	cp ../triggerd-$(VERSION).tar.gz ../triggerd_$(VERSION).orig.tar.gz
	dpkg-buildpackage -us -uc
	rm ../triggerd_$(VERSION).orig.tar.gz

indent:
	indent -linux -nut -ts4 -i4 *.c *.h
	rm -f *~

rpm: dist
	rm -rf rpmtmp
	mkdir -p rpmtmp/SOURCES rpmtmp/SPECS rpmtmp/BUILD rpmtmp/RPMS rpmtmp/SRPMS
	cp ../triggerd-$(VERSION).tar.gz rpmtmp/SOURCES/
	rpmbuild -ba -D "_topdir $(TOP)/rpmtmp" \
		-D "_builddir $(TOP)/rpmtmp/BUILD" \
		-D "_rpmdir $(TOP)/rpmtmp/RPMS" \
		-D "_sourcedir $(TOP)/rpmtmp/SOURCES" \
		-D "_specdir $(TOP)/rpmtmp/SPECS" \
		-D "_srcrpmdir $(TOP)/rpmtmp/SRPMS" \
		rpm/triggerd.spec
	cp $(TOP)/rpmtmp/RPMS/*/* ../
	cp $(TOP)/rpmtmp/SRPMS/* ../
	rm -rf rpmtmp

