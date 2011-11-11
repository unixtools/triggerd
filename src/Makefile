SBINDIR=/usr/sbin
CC=gcc -O2 -Wall

all: triggerd

clean:
	rm -f *.o

triggerd: triggerd.o tcp.o util.o debug.o
	$(CC) -o triggerd triggerd.o tcp.o util.o debug.o -lpthread

install:
	mkdir -p $(SBINDIR)
	install -m0755 triggerd $(SBINDIR)/triggerd

dist: clean
	cp -pr ../src ../triggerd
	find ../triggerd -depth -name .svn -exec rm -rf {} \;
	rm -f triggerd.tar.gz
	cd .. && gtar -czvf triggerd.tar.gz triggerd
	rm -rf ../triggerd

indent:
	indent -linux -nut -ts4 -i4 *.c *.h
	rm -f *~