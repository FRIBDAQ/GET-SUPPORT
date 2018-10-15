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


##
# @file utilities.tcl
# @brief A few generic utilities.
# @author Ron Fox <fox@nscl.msu.edu>
#

package provide RemoteUtilities 1.0
package require ssh

##
#  This package provides a few utilities for creating, finding and
#  manipulating remote processes.  Remote processes, in this case,
#  are intended (but not limited) to those that have been created by SSH pipes.
#  (e.g. the ssh package.)
#
#   This is challenging because the pids we can easily get hold of are
#   those in this process. The ssh client for example.  Programs run on ssh
#   pipes actually have, as their ppid, the daemon side of the beast.
#

namespace eval RemoteUtil {
    set myUserName $::tcl_platform(user)
}
##
# remotePid
#    Returns the PID of a command running in a remote system.
#
# @param host - host the system is running int.
# @param command - command string.
# @return possibly empty list of matching PIDS.
# @retval "" if there's no matching process
#
proc RemoteUtil::remotePid {host command} {
    set processes [ssh::ssh \
        $host [list ps -C $command -o user=,command=,pid=]  \
    ]
    set processes [split $processes "\n"]
    
    set result [list]
    foreach process $processes {
        puts $process
        set user [lindex $process 0]
        set pid [lindex $process end]
        if {$user eq $::RemoteUtil::myUserName} {
            lappend result $pid
        }
    }
    
    return $result
}
##
# RemoteUtil::kill
#    Kill a remote process given its PID. Usually used in conjunction
#    with RemoteUtil::remotePid to query the pid of a specific process
#    we're trying to kill.
#
# @param host - name of the host the process lives in.
# @param pid  - Process id to try to kill in that host.
# @param signal - Optional signal to use to kill the process.
#
proc RemoteUtil::kill {host pid {signal 9}} {
    ssh::ssh $host [list kill -$signal $pid ]
}