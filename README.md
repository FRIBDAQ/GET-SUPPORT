# GET-SUPPORT

Capture code and documentation related to interfacing GET electronics/Readout to NSCLDAQ.

To build set up the versions of root and NSCLDAQ the software should build, and
define the version of SpecTcl that will be used for the spectcl hit decoder
make file as SPECTCLHOME.
then:

make clean
make
make install PREFIX=/usr/opt/NSCLGET    # standard installation

Note the sample Makefile for spectchits will have $(SPECTCLHOME) and users
copying it will either need to edit the file or define the appropriate value.