SBINDIR=/usr/sbin

all: triggerd

clean:
	rm -f *.o

triggerd: triggerd.o
	$(CC) -o triggerd triggerd.o 

triggerd.o: triggerd.c
	$(CC) -c triggerd.c

install:
	mkdir -p $(SBINDIR)
	install -m0755 triggerd $(SBINDIR)/triggerd

dist: clean
	cp -pr ../src ../triggerd
	find ../triggerd -depth -name .svn -exec rm -rf {} \;
	rm -f ../binaries/triggerd.tar.gz
	cd .. && gtar -czvf binaries/triggerd.tar.gz triggerd
	rm -rf ../triggerd