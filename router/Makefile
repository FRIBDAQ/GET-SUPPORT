GET=/usr/opt/GET
NSCLDAQ=/usr/opt/nscldaq/11.3-004

NSCLDAQ_CXXFLAGS=-I$(NSCLDAQ)/include
NSCLDAQ_LDFLAGS=-L$(NSCLDAQ)/lib -Wl,-rpath=$(NSCLDAQ)/lib -ldataformat -lDataFlow -lException

ICE_CPPFLAGS = -DICEUTIL_HEADER\(header\)="<IceUtil/header>" -DICE_EDITION=Ice -DICE_HEADER\(header\)="<Ice/header>"
ICE_EDITION = Ice
ICE_LIBS = -lIce -lIceUtil

CXXFLAGS=-I$(GET)/include $(ICE_CPPFLAGS) $(NSCLDAQ_CXXFLAGS) -std=c++11 \
	-Wno-deprecated-declarations
LDFLAGS=-L$(GET)/lib -lgetbench-daq -lmdaq-daq -lMultiFrame  -lmdaq-utl \
	-lUtilities				\
	$(NSCLDAQ_LDFLAGS)  \
	$(ICE_LIBS)	\
	-Wl,-rpath=$(GET)/lib

nscldatarouter: dataRouterMain.o DataRouter.o datarouterargs.o NSCLDAQDataProcessor.o
	$(CXX) -o nscldatarouter \
		dataRouterMain.o \
		DataRouter.o \
		NSCLDAQDataProcessor.o \
		datarouterargs.o  \
		$(LDFLAGS)

dataRouterMain.o: DataRouter.cpp datarouterargs.o

datarouterargs.o: datarouter.ggo
	gengetopt <datarouter.ggo -F datarouterargs
	$(CC) -c datarouterargs.c

clean:
	rm -f nscldatarouter *.o datarouterargs.h datarouterargs.c
