SUBDIRS = libdebug \
          libhttps \

all     :
	@for x in $(SUBDIRS); do cd $$x; make; cd ..; done

clean   :
	@for x in $(SUBDIRS); do cd $$x; make clean; cd ..; done

install:
	@for x in $(SUBDIRS); do cp lib*.a ../../lib ; cp ..include/*.h ./../include; done
