all: nsclgetdocs.xml
	docbook2html -o docs nsclgetdocs.xml

install:
	install -d $(PREFIX)/share
	install -d $(PREFIX)/share/docs
	install -d $(PREFIX)/share/docs/figures
	install docs/* $(PREFIX)/share/docs
	install figures/* $(PREFIX)/share/docs/figures

