GET=/usr/opt/GET

ICE_CPPFLAGS = -DICEUTIL_HEADER\(header\)="<IceUtil/header>" -DICE_EDITION=Ice -DICE_HEADER\(header\)="<Ice/header>"
ICE_EDITION = Ice
ICE_LIBS = -lIce -lIceUtil

CXXFLAGS=-I$(GET)/include $(ICE_CPPFLAGS)
LDFLAGS=-L$(GET)/lib -lgetbench-daq -lmdaq-daq -lMultiFrame  -lmdaq-utl \
	-lUtilities $(ICE_LIBS)	\
	-Wl,-rpath=$(GET)/lib

nscldatarouter: dataRouterMain.o DataRouter.o datarouterargs.o
	$(CXX) -o nscldatarouter \
		dataRouterMain.o \
		DataRouter.o \
		datarouterargs.o  \
		$(LDFLAGS)

dataRouterMain.o: DataRouter.cpp datarouterargs.o

datarouterargs.o: datarouter.ggo
	gengetopt <datarouter.ggo -F datarouterargs
	$(CC) -c datarouterargs.c

clean:
	rm -f nscldatarouter *.o datarouterargs.h datarouterargs.c
