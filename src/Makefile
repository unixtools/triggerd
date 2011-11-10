all: triggerd

clean:
	rm *.o

triggerd: triggerd.o
	

dist: clean
	cp -pr ../src ../triggerd
	find ../triggerd -depth -name .svn -exec rm -rf {} \;
	rm -f ../binaries/triggerd.tar.gz
	cd .. && gtar -czvf binaries/triggerd.tar.gz triggerd
	rm -rf ../triggerd