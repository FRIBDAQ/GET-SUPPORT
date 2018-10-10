GET=/usr/opt/GET
COBOFORMATFILE=$(GET)/share/get-bench/format/CoboFormats.xcfg
NSCLDAQ=/usr/opt/nscldaq/11.3-004



NSCLDAQ_CXXFLAGS=-I$(NSCLDAQ)/include
NSCLDAQ_LDFLAGS=-L$(NSCLDAQ)/lib -Wl,-rpath=$(NSCLDAQ)/lib -ldataformat -lDataFlow -lException

ICE_CPPFLAGS = -DICEUTIL_HEADER\(header\)="<IceUtil/header>" -DICE_EDITION=Ice -DICE_HEADER\(header\)="<Ice/header>"
ICE_EDITION = Ice
ICE_LIBS = -lIce -lIceUtil

CXXFLAGS=$(NSCLDAQ_CXXFLAGS) -std=c++11
CXXLDFLAGS=$(NSCLDAQ_LDFLAGS)

CPPUNITLDFLAGS=-lcppunit


insertstatechange: statechange.h insertstatechange.o statechange.o
	$(CXX) -o insertstatechange insertstatechange.o statechange.o $(CXXLDFLAGS)

statechange.h: statechange.ggo
	gengetopt -Fstatechange <statechange.ggo
	gcc -c statechange.c

statechange.o: statechange.ggo
	gengetopt -Fstatechange <statechange.ggo
	gcc -c statechange.c

clean:
	rm -f *.o statechange.h statechange.c

