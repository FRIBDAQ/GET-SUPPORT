GET=/usr/opt/GET
NSCLDAQ=/usr/opt/nscldaq/11.3-004

ICE_CPPFLAGS=-DICEUTIL_HEADER\(header\)="<IceUtil/header>" \
	-DICE_EDITION=Ice -DICE_HEADER\(header\)="<Ice/header>"
ICE_EDITION=Ice
ICE_LIBS=-lIce 

GETCPPFLAGS=-I$(GET)/include

QTCXXFLAGS=-I/usr/include/qt4/QtCore/ -I/usr/include/qt4

CXXFLAGS=$(GETCPPFLAGS) $(ICE_CPPFLAGS) $(QTCXXFLAGS) -g -pthread
LDFLAGS=-L$(GET)/lib \
	-lgetbench-eccclient -lgetbench-cfg -lmdaq-utl -lmdaq-interfaces -lUtilities \
	-lCompoundConfig   \
	-lQtCore  \
	-pthread  \
	-Wl,-rpath=$(GET)/lib $(ICE_LIBS) -g

all: daqstart daqstop configure

configure: configure.o EccClient.o GetController.o
	$(CXX) -o configure configure.o EccClient.o GetController.o $(LDFLAGS)

daqstart: daqstart.o EccClient.o
	$(CXX) -o daqstart daqstart.o EccClient.o $(LDFLAGS)


daqstop: daqstop.o EccClient.o
	$(CXX) -o daqstop daqstop.o EccClient.o $(LDFLAGS)


clean:
	rm -f *.o daqstart daqstop configure


install: all
	install daqstart  $(PREFIX)/bin
	install daqstop   $(PREFIX)/bin
	install configure $(PREFIX)/bin
