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
# @file getProvider.tcl 
# @brief data source provider for GET electronics.
# @author Ron Fox <fox@nscl.msu.edu>
#
package provide GET_Provider 1.0
package require ssh
package require Wait
package require ReadoutGUIPanel
package require GET_Prompter
package require Tk
#
#   We have some stuff (besides code) to put in the GET namespace:
#

namespace eval ::GET {
    
    
    ##
    #   activeProviders is an array indexed by sourceid.
    #   It's contents are dicts.  The dicts contain:
    #
    #     -  parameterization - the paramterization for an id.
    #     -  geteccserver - PID of the GET ECC Server in spdaq.
    #     -  eccserveroutput - Fd connected to stdout/error of eccserver
    #     -  nsclrouter   - PID of the nscl data router in spdaq
    #     -  routeroutput - fd connected to nsclrouter stdout/err.
    #     -  ringmerge    - PID of the ring merge in localhost.
    #     -  mergeout     - fd connected to merger output/error
    #     -  runstarttime - [clock seconds] at which the last run started.
    #     -  runtitle     - Title of the last run.
    #     -  runNumber    - Number of the last run.
    #     -  ... @todo add to this as we need additional stuff.
    #
    array set activeProviders [list]
}

#------------------------------------------------------------------------------
#  Providers must implement the procs in this section.
#

##
# ::GET::parameters
#
#  Returns the dict that describes the parameters acceptable by this
#  data source type.
#  @note the GET_Prompter actually has the dict that we can return
#  with that information.
#
proc ::GET::parameters {} {
    return $::GET::parameterization
}
##
# ::GET::start
#    Starts the data source.  This means the data source is brought from
#    the notready state to the halted state from which a run can be started.
#    -   Get the parameterization for the source.
#    -   Stop any GET processes in the target node that we might be
#        running (note we're sol if there are processes run by a different user).
#        This is mostly in case an exiting ReadoutGUI left something hanging.
#        The other user problem -- that can be solved by rebooting the GET interface
#        system.
#    -   Make the ring buffers we need (if they don't exist) in the GET and local
#        nodes:
#        *  (target) Get RAW data ring.
#        *  (target) State transition ring.
#        *  (local)  merged ring.
#    -   Start the remote and local persistent processes:
#        * (target)  getEccServer
#        * (target)  nsclrouter
#        * (local)   ringmerge
#    -  Remind the user they need to run GetController at least once to
#       load firmware and configuration for the GET hardware.
#
#    If all of this works, we store the definition and state for
#    this source in activeProviders.
#
#    IF this id already has a definition it is replaced and it is assumed
#    a stop was already done on that source.
#    
# @param params - the parameterization dict.  Note that the source id has
#          been added to the parameterization.
#
proc ::GET::start params {
    set id [dict get $params sourceid]
    
    
    #  Save this now because we need to modify the initial dict from inside
    #  our utility procs.
    
    set ::GET::activeProviders($id) [dict create parameterization $params]
    
    #  Kill off the existing get procs:
    
    ::GET::killPersistentProcesses $id
    
    #  Create the ringbuffers:
    
    ::GET::createRingBuffers $id
    
    #  Start the persistent processes we need:
    
    ::GET::startPersistentProcesses $id
    
    #  Success - remind the user to run GetController
    
    tk_messageBox    \
        -icon info -title "GetController" -type ok                          \
        -message "Remember to run GetController to load/configure the firmware
and connect the nsclrouter to the datatflow."
}

##
#  ::GET::check
#
#   liveness means that all of the persistent processes needed to run
#   GET are alive (getEccServer, nsclrouter and ringmerge).
#
#      @return bool - true if the source is still live.
#
proc ::GET::check id {
    
    set state [::GET::getActiveSource $id]
    set params [dict get $state parameterization]
    
    return [GET::checkPersistentProcesses $params]
}
##
#  ::GET::stop
#    Stop a data source - just stops the persistent processes.
#
# @param id - source id to stop.
#
proc ::GET::stop id {
    if {[array names ::GET::activeProviders $id] eq ""} {
        error "There's no GET data source with the id $id to stop"
    }
    
    killPersistentProcesses $id
    array unset ::GET::activeProviders $id
}
##
# GET::begin
#    Begin a run in a GET data source.
#    - We need to send a begin run item into the state transition item.
#    - Store start time
#    - Start the run in the GET electronics..
#
# @param source  - id of the source to begin.
# @param runNum  - Number of the run being started.
# @param title   - Title of the run being started.
#
proc ::GET::begin {source runNum title} {
    set state [::GET::getActiveSource $source];     # Really just to check id's ok.
    
    #  Emit the begin run item:
    
    ::GET::emitBegin $id $runNum $title
    
    dict set state runstartime [clock seconds]
    set ::GET::activeProvide($source) $state;      # Update state with start time
    
    ::GET::startAcquisition  $state
    
}

##
# Pause and resume are not supported in GET runs:

proc ::GET::pause id {
    error "GET data sources do not support pause"
}
proc ::GET::reseume id {
    error "GET data sources do not support resume"
}
##
# GET::end
#   End the run:
#   - stop data taking.
#   - emit an end run item.
#
proc ::GET::end source {
    set state [::GET::getActiveSource $source]
    
    ::GET::stopAcquisition $state
    ::GET::emitEndRun $state
}

##
# GET::init
#    Re-initializes.  This kills off the persistent processes and
#    restarts them.  Note that the user will need to use GetController to
#    do the firmware thing again.
#
# @param id - source id of the source to re-init.
#
proc ::GET::init id {
    set state [::GET::getActiveSource $id]
    ::GET::killActiveprocesses $state;          # Can use the PIDS and close fds.
    ::GET::startPersistentProcesses $id
}
##
# ::GET::capabilities
#    Return the capabilities of a GET data source:
#
#  *  canPause false
#  *  runsHaveTitles true
#  *  runsHaveNumnbers true
#
proc::GET::capabilities {} {
    return [dict create canPause false runsHaveTitles true runsHaveNumbers true]
}
#-------------------------------------------------------------------------------
# Utility procs that make the world go 'round.

proc ::GET::killPersistentProcesses id {};       # stub.
proc ::GET::startPersistentProcesses id {};      # stub.
proc ::GET::checkPersistentProcesses params {};  # stub
proc ::GET::createRingBuffers id {};             # stub
proc ::GET::getActiveSource id {};                   # stub
proc ::GET::emitBegin {id num title} {};         # stub.
proc ::GET::startAcquisition state {};           # stub.
proc ::GET::stopAcquisition state {}            ; #stub
proc ::GET::emitEndRun state {}                 ; #stub