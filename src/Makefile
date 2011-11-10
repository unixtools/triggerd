all: triggerd

clean:
	rm *.o

triggerd: triggerd.o
	$(CC) -o triggerd triggerd.o 

triggerd.o: triggerd.c
	$(CC) -c triggerd.c

dist: clean
	cp -pr ../src ../triggerd
	find ../triggerd -depth -name .svn -exec rm -rf {} \;
	rm -f ../binaries/triggerd.tar.gz
	cd .. && gtar -czvf binaries/triggerd.tar.gz triggerd
	rm -rf ../triggerd