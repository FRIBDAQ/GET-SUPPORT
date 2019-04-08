TEMPLATE = app

QT += widgets

QMAKE_CXXFLAGS += -std=c++11
CONFIG += qt warn_on thread console

INCLUDEPATH += $(ROOTSYS)/include $(DAQROOT)/include /usr/opt/GET/include
LIBS += -L$(ROOTSYS)/lib -lCore -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lPostscript -lMatrix -lPhysics -lGui -lRGL -lMathCore -lThread -lMultiProc -pthread -L$(DAQLIB) -ldataformat -ldaqio -lException -Wl,-rpath=$(DAQLIB) -L/usr/opt/GET/lib -Wl,-rpath=/usr/opt/GET/lib -lMultiFrame -g

HEADERS += GETmePlots.h AnalyzeFrame.h GETDecoder.h processor.h ZoomClass.h
SOURCES += GETmePlots.cpp main.cpp AnalyzeFrame.cpp GETDecoder.cpp processor.cpp

