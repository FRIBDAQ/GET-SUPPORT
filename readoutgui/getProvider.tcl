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
package require RemoteUtilities
#
#   We have some stuff (besides code) to put in the GET namespace:
#

namespace eval ::GET {
    variable getBinDir    [file join /usr opt GET bin]
    variable daqbin       $::env(DAQBIN)
    
    
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
    variable activeProviders
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
    
    puts kill
    ::GET::killPersistentProcesses $id
    
    #  Create the ringbuffers:
    
    puts createrings
    ::GET::createRingBuffers $id
    
    #  Start the persistent processes we need:
    
    puts start
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
    return [GET::checkPersistentProcesses $state]
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
    
    ::GET::startAcquisition  [dict get $state parameterization]
    
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
    
    ::GET::stopAcquisition [dict get $state parameterization]
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
    ::GET::killPersistentProcesses $state;          # Can use the PIDS and close fds.
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
proc  GET::capabilities {} {
    return [dict create canPause false runsHaveTitles true runsHaveNumbers true]
}
#-------------------------------------------------------------------------------
# Utility procs that make the world go 'round.

##
# ::GET::getActiveSource
#    Return the state/parameter dict corresponding to the specified
#    source id.
#
# @param  id   - Id to looukup.
# @return dict = See comments in activeProviders for information about
#                the keys/values.
# @throw - if the id is not an active source id
proc ::GET::getActiveSource id {
    if {[array  names ::GET::activeProviders $id] eq $id} {
        return $::GET::activeProviders($id)
    } else {
        error "There is no active source with the id $id"
    }
}
##
# ::GET::killPersistsentProcesses
#
#   Stops the persistent processes that make up an id.
#   If the dict that defines the source has pids set, then
#   we do surgical strikes killing by PID -- and then
#   removing the PID dict keys.  If not we assume there's really
#   only one guy that can use the GET interface system so we do killalls
#   by process name.  The success of that will, of course be limited to
#   processes this user started.
#
# @param id - the source id that specifies the provider.  This
#             provides hostnames as well as potentially pids in those hostnames.
#
proc ::GET::killPersistentProcesses id {
    set info [::GET::getActiveSource $id]
    set  params [dict get $info parameterization]
    set  gethost [dict get $params spdaq]
    set  routerhost $gethost
    set thishost [exec hostname]
#    set  routerhost [::GET::ringhost [dict get $params datauri]]
    
    # Now do the kills.
    
    ::GET::killProcess $info $gethost geteccserver eccserveroutput getEccServer
    ::GET::killProcess $info $routerhost nsclrouter routeroutput   nsclrouter
    ::GET::killProcess $info $thishost   ringmerge  mergeout       ringmerge
    
}


##
# ::GET::startPersistentProcesses
#    Starts the persistent GET processes for the source id specified.
#    These include:
#    - getEccServer - which must be run in the spdaq host.
#    - nsclrouter   - which is run in the system specified by the data ring uri.
#    - ringmerge    - which must be run in this host.
#
proc ::GET::startPersistentProcesses id {
    set info [::GET::getActiveSource $id]
    set params [dict get $info parameterization]
    
    set result [::GET::startEccServer $params]
    dict set info geteccserver [lindex $result 0] eccserveroutput [lindex $result 1]
    
    set result [::GET::startNsclRouter $params]
    dict set info nsclrouter [lindex $result 0] routeroutput [lindex $result  1]
    
    set result [::GET::startRingMerge $params]
    dict set info ringmerge [lindex $result 0] mergeout [lindex $result 1]
    
    set activeProviders($id) $info;            # Update the dict.
}
##
# GET::checkPersistentProcesses
#    Given the pids of the persistent processes (in the info block) and the
#    hosts in which they are running, determines if all of them are running.
#
# @param infoi - information dict about the current run.
# @return bool - true if all persistent processes are running.
#
proc ::GET::checkPersistentProcesses info {
    set  params [dict get $info parameterization]
    set  gethost [dict get $params spdaq]
    set routerhost $gethost
    set thisHost [exec hostname]
    
    #set  routerhost [::GET::ringhost [dict get $params datauri]]
    
    set eccok    [::GET::checkProcess getEccServer $gethost]
    set routerok [::GET::checkProcess nscldatarouter $routerhost]
    set mergeok  [::GET::checkProcess ringmerge $thisHost]
    
    return [expr {$eccok && $routerok && $mergeok}]
    
}
##
# GET::createRingBuffers
#    Create the ring buffers associated with a data source.
#
# @param id  - id of the data source.
#
proc ::GET::createRingBuffers id {
    set info [::GET::getActiveSource $id]
    set params [dict get $info parameterization]
    
    set routerhost [::GET::ringhost [dict get $params datauri]]
    set routerring [::GET::ringname [dict get $params datauri]]
    ::GET::makeRing $routerhost $routerring
    
    set statehost [::GET::ringhost [dict get $params stateuri]]
    set statering [::GET::ringname [dict get $params stateuri]]
    ::GET::makeRing $statehost $statering
    
    ::GET::makeRing localhost [dict get $params outputring]
}
##
# ::GET::emitBegin
#     Runs the insertstatechange program in the host that has the
#     state transition ring so that it will insert a begin run in that ring.
#
# @param id    - data source id.
# @param num   - run number.
# @param title - Run title.
#
proc ::GET::emitBegin {id num title} {
    set info [::GET::getActiveSource $id]
    set params [dict get $info parameterization]
    
    set statehost [::GET::ringhost [dict get $params stateuri]]
    set statering [::GET::ringname [dict get $params stateuri]]
    set sourceid  [dict get $params sourceid]
    dict set info runstarttime [clock seconds]
    dict set info runtitle $title
    dict set info runNumber $num
    set path [file join $::GET::getBinDir insertstatechange]
    
    ssh::ssh $statehost [list \
        $path --ring=$statering --run=$num --title=$title --source-id=$sourcid \
        --type=begin]
    
    # If that worked we can update the sources dict:
    
    set $activeProviders($id) $info
    
    #  Let the begin run record percolate through the system.
    
    after 500
}
##
# changeAcquisitionState
#    change the acqusition status.
#
# @param params - the parameterization of the data source.
# @param command - The command tail of the program to do this
#
proc ::GET::changeAcquisitionState {params program} {
    set coboip [dict get $params coboip]
    set cobosvc [dict get $params coboservice]
    set arg1 $coboip:$cobosvc
    
    set ecchost [dict get $params privateip]
    set eccsvc [dcit get $params eccservice]
    set args $ecchost:$eccsvc
    
    set spdaq [dict get $params spdaq]
    set path [file join $::GET::getBinDir $program]
    
    ssh::ssh $spdaq [list $path $arg1 $arg2]
    
}
##
# ::GET::startAcquisition
#    Starts the data acquisition in the GET modules to the data router.
#    The assumption is that any begin run item that was needed is already
#    on its way through the system...at least has been seen by the merge program.
#
# @param params - data source parameterization.
#                 We need the coboip, coboservice the spdaq and privateip and
#                 eccservice
#                 
proc ::GET::startAcquisition params {
    ::GET::changeAcquisitionState $params daqstart
}
##
# ::GET::stopAcquisition
#    Stop acquisition in the GET hardware.
#
# @param params - the data source parameters.
proc ::GET::stopAcquisition params {
    ::GET::changeAcquisitionState  $params daqstop
    after 500;                  # Let this run down.
}
##
# ::GET::emitEndRun
#    Emits the end run item to the state ring.
#    Note that the run time is computed from now - runstarttime
#    The title and run number for the record are also gotten
#    from the state dict.
#
proc ::GET::emitEndRun state {

    set params [dict get $state parameterization]
    
    set statehost [::GET::ringhost [dict get $params stateuri]]
    set statering [::GET::ringname [dict get $params stateuri]]
    set sourceid  [dict get $params sourceid]
    set title [dict get $state runtitle]
    set num   [dict get $state runNumber]
    set duration [expr {[clock seconds] - [dict get $state runstarttime]}]

    set path [file join $::GET::getBinDir insertstatechange]
    
    ssh::ssh $statehost [list \
        $path --ring=$statering --run=$num --title=$title --source-id=$sourcid \
        --type=end --offset $duration]    

}
##
# ::GET::ringhost
#
#   Given a ring uri of the form tcp://host/ring
#   Returns the host.
#
# @param uri - ringbuffer uri
# @return string  hostname part of the uri.
#
proc ::GET::ringhost uri {
    set parts [split $uri /]
    return [lindex $parts 2]
}
##
# ::GET::ringname
#
#
#   Given a ring uri of the form tcp://host/ring
#   Returns the ring name.
#
# @param uri - ringbuffer uri
# @return string  ring name part of the uri.
#
proc ::GET::ringname uri {
    set parts [split $uri /]
    return [lindex $parts 3]
    
}
##
# ::GET::killProcess
#     Kills a process.
#
# @param info - the dict that represents the state and parameterization of the
#               data source.
# @param host - The host in which the process is running.
# @param pidkey,pipekey - The info keys that have the pid and pipe fd for the
#               process.  If these are set we'll try to use them to do a targeted
#               kill.
# @param processname - If pidkey and/or pipekey are not defined, we'll use this
#              to try to do a killall of processname (nuclear option - pun intended).
# @note - if pidkey and pipekey are defined, then they are unset from
#         the info dict and that's used to update the source's dict.
#         Note that the source id is [dict get $info parameterization sourceid]
# @note - processes are started on an ssh pipe.  which  means much of what
#         we wanted to do we can't acutally do.
#
proc ::GET::killProcess {info host pidkey pipekey processname} {
    if {[dict exists $info $pipekey]} {
        # The pipe fd can be closed:
        
        close [dicte get $info $pipekey]
        dict unset $info $pipekey
        dict unset $info $pidkey;        # SSH pid.
    }
    #  Figure out the PID of the remote process
    
    set pidList [::RemoteUtil::remotePid $host $processname]
    foreach pid $pidList {
        RemoteUtil::kill $host $pid
    }
}
##
# ::GET::startEccServer
#
#     Starts the getEccServer program.
#
# @param params - the parameters dict for the source.
#                 We need:
#               - spdaq from that dict as that's were we're going
#                 to put the ECC server.
#               - ::GET::getBindir which is where getEccServer is installed.
#  We return a two element list consisting of the ssh pid and fd open
#  on the ssh pipe.
#
# @note -sstdout and stderr from the ssh pipe are caught by our
#        ::GET::handleOuptut proc.
# 
proc ::GET::startEccServer params {
    set host [dict get $params spdaq]
    set command [file join $::GET::getBinDir getEccServer]
    set info [ssh::sshpid $host $command]
    
    set pid [lindex $info 0]
    set fd  [lindex $info 1]
    
    ::GET::setupOutputHandler $fd
    
    return [list $pid $fd]
}
##
#    ::GET::startNsclRouter
#
#  Starts the NSCL GET data router.  This is a version of the GET dataRouter
#  program that can insert GET frames as ring items in an NSCLDAQ ring buffer.
#
# @param params - The dict that has the parameterization of the
#                 data source.  We need the following items:
#                 -  spdaq - host in which this is going to be run.
#                 -  eccip the private subnet IP in which the ecc server is running.
#                 -  eccservice - The service number on which eccServer accepts connections.
#                 -  dataip - the Private subnet IP in which the router runs.
#                 -  dataservice - The service number on which the router listens.
#                 -  datauri - the URI of the ring in which the nsclrouter puts data
#                 -  sourceid - the data source id the ring items have.
#                 - timestampsource - what to put in for the timestamp.
#
# @return list [list pid pipefd]
#
proc ::GET::startNsclRouter params {
    set host [dict get $params spdaq]
    
    set eccip [dict get $params eccip]
    set eccsvc [dict get $params eccservice]
    
    set dataip [dict get $params dataip]
    set datasvc [dict get $params dataservice]
    
    set ringname [::GET::ringname [dict get $params datauri]]
    set srcid    [dict get $params sourceid]
    set tsSrc    [dict get $params timestampsource]
    
    set program \
        [file normalize [file join $::GET::getNsclDaqBindir nscldatarouter]]
    
    set info [ssh::sshpid $host [list $program \
        --controlservice $eccip:$eccsvc --dataservice $dataip:$datasvc        \
        --ringname $ringname --id $srcid --timestamp $tsSrc]]
    
    set pid [lindex $info 0]
    set fd [lindex $info 1]
    ::GET::setupOutputHandler $fd
    
    return [list $pid $fd]
}
##
# ::GET::startRingMerge
#
#     Starts the ring merge program
#
# @param params - the parameterization dictionary of the data source.
#                 We need:
#                 -   datauri - URI of the ring in which data items are put.
#                 -   stateuri - URI of the ring in which state change items are
#                                put.
#                 -   outputring - Name of the output ring.
#
# @note - The ring merge program is run in the host the script is running int.
#
#            
proc ::GET::startRingMerge params {
    set ourHost [exec hostname]
    set dataring  [dict get $params datauri]
    set statering [dict get $params stateuri]
    set outring   [dict get $params outputring]
    
    set program [file normalize [file join $::GET::getNsclDaqBindir ringmerge]]
    
    set info [ssh::sshpid \
        $ourHost [list $program --input $dataring --input $statering   \
        --output $outputring]                 \
    ]
    
    set pid [lindex $info 0]
    set fd [lindex $info 1]
    ::GET::setupOutputHandler $fd
    
    return [list $pid $fd]
}
##
# ::GET::checkProcess
#     @param command - name of the command to check.
#     @param host    - Host in which the command is running.
#
#     @return bool - true if the process is found in the remote system.
#
proc ::GET::checkProcess {command host} {
    set pids [::RemoteUtil::remotePid  $host $command]
    
    return [expr {[llength $pids] > 0}]
}
##
# ::GET::makeRing
#     Make a ring buffer in a remote host.
#
# @param host - name/IP of the host in which to create the ringbufer
# @param name - name of the ring buffer being created.
#
proc ::GET::makeRing {host name} {
    set program [file join $::GET::daqbin  ringbufer]
    set command [list $program create $name]
    ssh::ssh $host $command;    # Errors caught in ssh::ssh.
}
##
# ::GET::setupOutputHandler
#     Sets up an fd to  use ::GET::handleOutput whenever
#     input is available.
#
#   @note that fd is set in non blocking mode.
#    
proc ::GET::setupOutputHandler {fd} {
    fconfigure $fd -blocking 0
    fileevent $fd readable [list ::GET::handleOutput $fd]
}
##
# ::GET::handleOutput
#    - If the fd is at eof, unset the file event handler.  The fd will
#      be closed by other code.
#    - Otherwise, read a line from the file and
#      submit it as a log message to the console.
#
# @param fd - File descriptor that is readable.
proc ::GET::handleOutput {fd} {
    if {[eof $fd]} {
        fileevent $fd readable ""
    } else {
        set line [gets $fd]
        ReadoutGUIPanel::log GET info $line       
    }
}
