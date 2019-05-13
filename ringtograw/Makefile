

CXXFLAGS=-I$(DAQROOT)/include -g
LDFLAGS=-L$(DAQROOT)/lib -ldaqio -ldataformat -lException -Wl,-rpath=$(DAQROOT)/lib


ring2graw: ring2graw.cpp
	$(CXX) -o ring2graw $(CXXFLAGS) ring2graw.cpp $(LDFLAGS)

clean:
	rm -f ring2graw


all: ring2graw

install: all
	install ring2graw $(PREFIX)/bin

