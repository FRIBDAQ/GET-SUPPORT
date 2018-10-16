#!/bin/sh
# -*- tcl -*-
# The next line is executed by /bin/sh, but not tcl \
exec tclsh "$0" ${1+"$@"}

#    This software is Copyright by the Board of Trustees of Michigan
#    State University (c) Copyright 2014.
#
#    You may use this software under the terms of the GNU public license
#    (GPL).  The terms of this license are described at:
#
#     http://www.gnu.org/licenses/gpl.txt
#
#    Authors:
#             Ron Fox
#             Jeromy Tompkins 
#	     NSCL
#	     Michigan State University
#	     East Lansing, MI 48824-1321

package provide GETTestHarness 1.0
package provie ReadoutGUIPanel 1.0;     # Yeah double but ...
##
# @file testharness.tcl
# @brief Test harness to allow testing outside of full ReadoutGui.
# @author Ron Fox <fox@nscl.msu.edu>
#

##
#  Provides enough of a test harness to support testing the software
#  outside of the full ReadoutShell application.


# Stub for logging.  Just log to terminal


namepace eval ReadoutGUIPanel {}
proc ReadoutGUIPanel::log {from sev data} {
    puts "[clock format [clock seconds]] - $from $sev $data"
}

