GET=/usr/opt/GET
NSCLDAQ=/usr/opt/nscldaq/11.3-004

ICE_CPPFLAGS=-DICEUTIL_HEADER\(header\)="<IceUtil/header>" \
	-DICE_EDITION=Ice -DICE_HEADER\(header\)="<Ice/header>"
ICE_EDITION=Ice
ICE_LIBS=-lIce -lIceUtil

GETCPPFLAGS=-I$(GET)/include

QTCXXFLAGS=-I/usr/include/qt4/QtCore/ -I/usr/include/qt4

CXXFLAGS=$(GETCPPFLAGS) $(ICE_CPPFLAGS) $(QTCXXFLAGS) -g -pthread
LDFLAGS=-L$(GET)/lib \
	-lgetbench-eccclient -lmdaq-utl -lmdaq-interfaces -lUtilities \
	-lCompoundConfig   \
	-pthread		\
	-Wl,-rpath=$(GET)/libs $(ICE_LIBS) -g

all: daqstart daqstop


daqstart: daqstart.o EccClient.o
	$(CXX) -o daqstart daqstart.o EccClient.o $(LDFLAGS)


daqstop: daqstop.o EccClient.o
	$(CXX) -o daqstop daqstop.o EccClient.o $(LDFLAGS)


clean:
	rm -f *.o daqstart daqstop

