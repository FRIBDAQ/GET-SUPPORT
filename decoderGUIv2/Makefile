##
# Makefile for process
#
#   Assumes an daqsetup.bash has been sourced so that DAQROOT etc.
#   are defined:


all: GETmePlots 

GETmePlots: processor.cpp processor.h AnalyzeFrame.cpp AnalyzeFrame.h GETDecoder.cpp GETDecoder.h GETmePlots.cxx 
	g++ -o GETmePlots GETmePlots.cxx processor.cpp AnalyzeFrame.cpp GETDecoder.cpp -I$(DAQROOT)/include -L$(DAQLIB) -ldataformat -ldaqio -lException -Wl,-rpath=$(DAQLIB) -std=c++11 -L/usr/opt/GET/lib -I/usr/opt/GET/include -Wl,-rpath=/usr/opt/GET/lib -lMultiFrame -g -I$(ROOTSYS)/include -L$(ROOTSYS)/lib -lGui -lCore -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread -lMultiProc -pthread -Wl,-rpath=$(ROOTSYS)/lib


install:
	install GETmePlots $(PREFIX)/bin/GETmePlots
	install -d $(PREFIX)/include
	install AnalyzeFrame.h $(PREFIX)/include
	install GETDecoder.h $(PREFIX)/include
	install ZoomClass.h $(PREFIX)/include

clean:
	rm -f GETmePlots
