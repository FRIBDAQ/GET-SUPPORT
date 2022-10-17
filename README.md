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

### Build notes:

There are C++ compiler version issues:  GET base software cannot be compiled
with current langauge standards because it uses std::auto_ptr which is
deprecated.  Current versions of Root, however require c++-17.
This left, in our version inventory:

Root 6.10.02 - which is too old for GetMePlot - missing header
Root 6.18.04 - which is just right and *must* be used.
