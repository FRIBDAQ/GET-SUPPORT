GET=/usr/opt/GET
COBOFORMATFILE=$(GET)/share/get-bench/format/CoboFormats.xcfg
NSCLDAQ=/usr/opt/nscldaq/11.3-004



NSCLDAQ_CXXFLAGS=-I$(NSCLDAQ)/include
NSCLDAQ_LDFLAGS=-L$(NSCLDAQ)/lib -Wl,-rpath=$(NSCLDAQ)/lib -ldataformat -lDataFlow -lException

ICE_CPPFLAGS = -DICEUTIL_HEADER\(header\)="<IceUtil/header>" -DICE_EDITION=Ice -DICE_HEADER\(header\)="<Ice/header>"
ICE_EDITION = Ice
ICE_LIBS = -lIce -lIceUtil


ringmerge.o: ringmerge.ggo
	gengetopt <ringmerge.ggo -Fringmerge
	$(CC) -c ringmerge.c
