GET=/usr/opt/GET
NSCLDAQ=/usr/opt/nscldaq/11.3-029
GENGENOPT=/usr/bin/gengetopt

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

BINARIES = daqstart daqstop configure daqcontrol

all: $(BINARIES)

configureOpts.o: configureOpts.ggo
	$(GENGENOPT) --output-dir=$(PWD) --input=$^ -F $(patsubst %.ggo,%,$^)
	$(CXX) -c $(patsubst %.ggo,%.c,$^)

configure: EccClient.o GetController.o configureOpts.o configure.o 
	$(CXX) -o $@ $^ $(LDFLAGS)

daqcontrolOpts.o: daqcontrolOpts.ggo
	$(GENGENOPT) --output-dir=$(PWD) --input=$^ -F $(patsubst %.ggo,%,$^)
	$(CXX) -c $(patsubst %.ggo,%.c,$^)

daqcontrol: EccClient.o daqcontrolOpts.o daqcontrol.o 
	$(CXX) -o $@ $^ $(LDFLAGS)

daqstart: daqstart.o EccClient.o
	$(CXX) -o $@ $^ $(LDFLAGS)

daqstop: daqstop.o EccClient.o
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(BINARIES) configureOpts.h configureOpts.c daqcontrolOpts.h daqcontrolOpts.c

install: all
	install $(BINARIES)  $(PREFIX)/bin
