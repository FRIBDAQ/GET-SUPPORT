##
# Makefile for process
#
#   Assumes an daqsetup.bash has been sourced so that DAQROOT etc.
#   are defined:


all: process

process: process.cpp processor.cpp processor.h AnalyzeFrame.cpp AnalyzeFrame.h
	g++ -o process process.cpp processor.cpp AnalyzeFrame.cpp	\
	-I$(DAQROOT)/include -L$(DAQLIB)	\
	-ldataformat -ldaqio -lException -Wl,-rpath=$(DAQLIB) -std=c++11 \
	-L/usr/opt/GET/lib -I/usr/opt/GET/include -Wl,-rpath=/usr/opt/GET/lib \
	-lMultiFrame -g

install:
	install process $(PREFIX)/bin/hitmaker
	install -d $(PREFIX)/include
	install AnalyzeFrame.h $(PREFIX)/include


clean:
	rm -f process
