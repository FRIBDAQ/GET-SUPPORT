GET=/usr/opt/GET
COBOFORMATFILE=$(GET)/share/get-bench/format/CoboFormats.xcfg
NSCLDAQ=$(DAQROOT)



NSCLDAQ_CXXFLAGS=-I$(NSCLDAQ)/include
NSCLDAQ_LDFLAGS=-L$(NSCLDAQ)/lib -Wl,-rpath=$(NSCLDAQ)/lib -ldataformat -lDataFlow -lException

ICE_CPPFLAGS = -DICEUTIL_HEADER\(header\)="<IceUtil/header>" -DICE_EDITION=Ice -DICE_HEADER\(header\)="<Ice/header>"
ICE_EDITION = Ice
ICE_LIBS = -lIce -lIceUtil

CXXFLAGS=$(NSCLDAQ_CXXFLAGS) -std=c++11
CXXLDFLAGS=$(NSCLDAQ_LDFLAGS)

CPPUNITLDFLAGS=-lcppunit

ringmerge: ringmerge.o  ringMergeMain.o 
	$(CXX) -o ringmerge ringMergeMain.o ringmerge.o $(CXXLDFLAGS)


ringMergeMain.o: ringMergeMain.cpp ringmerge.h

ringmerge.o: ringmerge.ggo
	gengetopt <ringmerge.ggo -Fringmerge
	$(CC) -c ringmerge.c

ringmerge.h: ringmerge.ggo
	gengetopt <ringmerge.ggo -Fringmerge

all: ringmerge tests

clean:
	rm -f *.o ringmerge.h ringmerge.h ringmerge.c ringmerge
	rm -f unittests

install: all
	install ringmerge $(PREFIX)/bin


tests: unittests ringmerge
	./unittests

unittests: TestRunner.o mergetests.o
	$(CXX) -o unittests TestRunner.o mergetests.o $(CXXLDFLAGS) $(CPPUNITLDFLAGS)
