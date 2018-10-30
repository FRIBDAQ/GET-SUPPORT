CC      = g++
GETROOT = $(PWD)/GETDecoder/build
CFLAGS  = -std=c++11 -g -I$(ROOTSYS)/include -I$(GETROOT)/../include
LDFLAGS = -pthread -Wall -lrt  -lzmq  -g -L$(ROOTSYS)/lib -lGui -lCore -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -Wl,-rpath=$(ROOTSYS)/lib -L$(GETROOT) -lGETDecoder -Wl,-rpath=$(GETROOT)

all:  $(GETROOT)/libGETDecoder.so GETmePlots

GETmePlots.o: GETmePlots.cxx
	$(CC) -c $(CFLAGS) $<

$(GETROOT)/libGETDecoder.so:
	$(MAKE) -C $(GETROOT)

.PHONY: clean cleanest

clean:
	rm *.o GETmePlots
	cd $(GETROOT) && $(MAKE) clean
