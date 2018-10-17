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
package provide ReadoutGUIPanel 1.0;     # Yeah double but ...
##
# @file testharness.tcl
# @brief Test harness to allow testing outside of full ReadoutGui.
# @author Ron Fox <fox@nscl.msu.edu>
#

##
#  Provides enough of a test harness to support testing the software
#  outside of the full ReadoutShell application.


# Stub for logging.  Just log to terminal


namespace eval ReadoutGUIPanel {}
proc ReadoutGUIPanel::log {from sev data} {
    puts "[clock format [clock seconds]] - $from $sev $data"
}

proc stripdict dict {
    set result [dict create]
    dict for {key value} $dict {
        dict set result $key [lindex $value 2]

    }
    return $result
}

set instdir $env(PREFIX)

source [file join $instdir TclLibs utilities.tcl]
source [file join $instdir TclLibs getPrompt.tcl]
source [file join $instdir TclLibs getProvider.tcl]


# If config.dat exists read the configuration from it:

if {[file exists config.dat]} {
    set f [open config.dat r]
    set descrip [gets $f]
    close $f
} else {
    puts "You need to create the configuration file and save it in config.dat"
    set raw [GET::promptParameters]
    set descrip [stripdict $raw]
    dict set descrip sourceid 0
    set f [open config.dat w]
    puts $f $descrip
    close $f
}

puts "The variable 'descrip' has the data source description dict."

