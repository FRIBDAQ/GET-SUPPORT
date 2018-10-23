GETDECODERROOT=$(PWD)/GETDecoder
CC      = g++
CFLAGS  = -std=c++11 -g -I$(ROOTSYS)/include -I$(GETDECODERROOT)/include
LDFLAGS = -pthread -Wall -lrt  -lzmq  -g -L$(ROOTSYS)/lib -lGui -lCore -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -Wl,-rpath=$(ROOTSYS)/lib -L$(GETDECODERROOT)/build -lGETDecoder -Wl,-rpath=$(GETDECODERROOT)/build

all:  GETmePlots

GETmePlots.o: GETmePlots.cxx
	$(CC) -c $(CFLAGS) $<

.PHONY: clean cleanest

clean:
	rm *.o  GETmePlots
