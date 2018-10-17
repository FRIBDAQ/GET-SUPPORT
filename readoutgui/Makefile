all:

clean:

install:
	install index.html getPrompt.tcl getProvider.tcl utilities.tcl $(PREFIX)/TclLibs
	echo pkg_mkIndex -verbose $(PREFIX)/TclLibs "*.tcl" | tclsh
