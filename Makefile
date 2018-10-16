##
# PREFIX is the installation point of this software.
#

subdirs=ringtograw runcontrol insertstatechange ringmerge router readoutgui

all:
	for d in $(subdirs) ; do (cd $$d; make all) ;done

clean:
	for d in $(subdirs) ; do (cd $$d; make clean) ;done

install:
	mkdir -p $(PREFIX)/bin
	mkdir -p $(PREFIX)/TclLibs
	for d in $(subdirs) ; do (cd $$d; make install PREFIX=$(PREFIX)) ;done	
