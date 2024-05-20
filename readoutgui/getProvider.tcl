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
package provide GETBundle    1.0;      # Handle state transitions.

package require ssh
package require Wait
package require ReadoutGUIPanel
package require GET_Prompter
package require Tk
package require RemoteUtilities
package require RunstateMachine

# Note that when we're loaded the user interface is not quite set up
# so add our GUI frame 1/2 second after this:

after 500 {GET::SetupGui}

#
#   We have some stuff (besides code) to put in the GET namespace:
#

namespace eval ::GET {    
    variable getBinDir    [file join /usr opt GET bin]
    variable daqbin       $::env(DAQBIN)

    # Storage for shared information

    variable sharedInfo
    array set sharedInfo [list]
    set sharedInfo(numGetEccServers) 0
    set sharedInfo(getpids) [list]
    set sharedInfo(routerpids) [list]
    set sharedInfo(ringmergepids) [list]
    set sharedInfo(pipepids) [list]
    set sharedInfo(status) 1

    # Variables for procs responding to checks by DatasourceManager

    variable checkPeriod
    variable checkEnabled
    variable checkingPids
    # Period in ms
    set checkPeriod 300000
    set checkEnabled 1
    set checkingPids [list]

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
#        * (target)  daqcontrolpipe
#        * (target)  statepipe
#        * (local)   ringmerge
#    -  Remind the user they need to click 'Configure CoBo(s)' button to
#       load firmware and configuration for the GET hardware.
#
#    If all of this works, we store the definition and state for
#    this source in activeProviders.
#
#    Some information is stored in sharedInfo array. See namespace definition
#    to learn what information is stored there.
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

    #  runStatus dictionary key for global run status control

    dict append ::GET::activeProviders($id) runStatus 0

    #  For checking if all begin item are emitted

    dict append ::GET::activeProviders($id) beginEmitted 0

    #  Create the ringbuffers:
    
    ::GET::createRingBuffers $id

    #  Initial clean up persistent processes

    ::GET::initialProcessCleanup $id

    #  Start the processes for each source we need:
    
    ::GET::startPersistentProcesses $id
    
    #  Success - remind the user to configure CoBo(s)
}

##
#  ::GET::check
#
#   liveness means that all of the persistent processes needed to run
#   GET are alive (getEccServer, nsclrouter and ringmerge).
#   
#   The variable is updated periodically set in $::GET::checkPeriod
#   with OR condition. If any one fails, status variable becomes 0.
#
#      @return bool - true if the source is still live.
#
proc ::GET::check id {
    return $::GET::sharedInfo(status)
}
##
#  ::GET::stop
#    Stop a data source - just stops the persistent processes,
#    closes pipes for the processes and kills the connection.
#
# @param id - source id to stop.
#
proc ::GET::stop id {
    if {[array names ::GET::activeProviders $id] eq ""} {
        error "There's no GET data source with the id $id to stop"
    }

    set ::GET::checkEnabled 0 
    set ::GET::checkingPids [list]
    
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
# @param id      - id of the source to begin.
# @param runNum  - Number of the run being started.
# @param title   - Title of the run being started.
#
proc ::GET::begin {id runNum title} {
    set state [::GET::getActiveSource $id];     # Really just to check id's ok.
    
    #  Emit the begin run item:
    
    ::GET::emitBegin $id $runNum $title
        
    ::GET::startAcquisition $id
    
}

##
# Pause and resume are not supported in GET runs:
# 
# Using daqcontrol, one can implement pause and resume.
# Just start/stop the GET DAQ without resetting timestamp.

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
proc ::GET::end id {
    set state [::GET::getActiveSource $id]
    
    ::GET::stopAcquisition $id
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
    ::GET::killPersistentProcesses $id
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
#
proc ::GET::getActiveSource id {
    if {[array  names ::GET::activeProviders $id] eq $id} {
        return $::GET::activeProviders($id)
    } else {
        error "There is no active source with the id $id"
    }
}

##
# ::GET::initialProcessCleanup
#    Checks if the persistent processes are running under this username.
#    If found and they're not started by other datasource within this DAQ,
#    kill those processes.
#
#    Processes started by this DAQ are managed by "remote PID" in
#    arrays having those PIDs.
#
# @param id - Id to lookup
#
proc ::GET::initialProcessCleanup id {
    set info [::GET::getActiveSource $id]
    set params [dict get $info parameterization]

    set gethost [dict get $params spdaq]
    set routerhost $gethost
    set thishost [exec hostname]

    set getpids [RemoteUtil::remotePid $gethost getEccSoapServer]
    set routerpids [RemoteUtil::remotePid $routerhost nscldatarouter]
    set ringmergepids [RemoteUtil::remotePid $thishost ringmerge]

    set getpids [::GET::removeOwnedPids $getpids getpids]
    set routerpids [::GET::removeOwnedPids $routerpids routerpids]
    set ringmergepids [::GET::removeOwnedPids $ringmergepids ringmergepids]

    set totalProcesses [expr [llength $getpids] + [llength $routerpids] + [llength $ringmergepids]]

    if {$totalProcesses > 0} {
#        set response [tk_messageBox -icon question -type yesno \
#                    -title "Persistent processes found!" \
#                    -message "Do you want to kill process(es)?" \
#                    -detail "Following process(es) are found in each host:\n
#getEccServer: [llength $getpids] process(es)\n
#nscldatarouter: [llength $routerpids] process(es)\n
#ringmerge: [llength $ringmergepids] process(es)"]
        
#        if {$response eq yes} {
            puts "Killing getEccSoapServer(s)...."
            RemoteUtil::kill $gethost $getpids
            puts "Killing nscldatarouter(s)...."
            RemoteUtil::kill $routerhost $routerpids
            puts "Killing ringmerge(s)...."
            RemoteUtil::kill $thishost $ringmergepids
#        }
    }
}

##
# ::GET::killPersistentProcesses
#
#   Stops the persistent processes and pipes that make up an id.
#   If the dict that defines the source has pids set, then
#   we do surgical strikes killing by PID -- and then
#   removing the PID dict keys. Closing the pipes created for them.
#
# @param id - the source id that specifies the provider.  This
#             provides hostnames as well as potentially pids in those hostnames.
#
proc ::GET::killPersistentProcesses id {
    set info [::GET::getActiveSource $id]
    set params [dict get $info parameterization]

    set gethost [dict get $params spdaq]
    set routerhost $gethost
    set thishost [exec hostname]
    set statehost [::GET::ringhost [dict get $params stateuri]]
    
    # Now do the kills.
    
    ::GET::killProcess $info $gethost geteccserver eccserveroutput \
                                        getEccSoapServer getpids
    ::GET::killProcess $info $routerhost nsclrouter routeroutput \
                                        nscldatarouter routerpids
    ::GET::killProcess $info $thishost ringmerge  mergeout \
                                        ringmerge ringmergepids
    ::GET::killProcess $info $gethost daqcontrol daqcontrolfd \
                                        daqcontrolpipe pipepids
    ::GET::killProcess $info $statehost state statefd \
                                        statepipe pipepids

    after 1000;     # Wait for them to die.
}

##
# ::GET::startPersistentProcesses
#    Starts the persistent GET processes for the source id specified.
#    These include:
#    - getEccServer   - which must be run in the spdaq host.
#    - nsclrouter     - which is run in the system specified by the data ring uri.
#    - daqcontrolpipe - SSH pipe to spdaq host keep open until exit
#                       to submit DAQ start/stop
#    - statepipe      - SSH pipe to this host keep open until exit
#                       to emit begin/end RingItem to rings
#    - ringmerge      - which must be run in this host.
#
#    Once all the processes start up, each PID is stored in sharedInfo(pidarray).
#    This is prepared to enable multiple CoBo use.
#
proc ::GET::startPersistentProcesses id {
    set info [::GET::getActiveSource $id]
    set params [dict get $info parameterization]
    
    set result [::GET::startEccSoapServer $params]
    dict set info geteccserver [lindex $result 0]
    dict set info eccserveroutput [lindex $result 1]

    set result [::GET::startDaqcontrolPipe $params]
    dict set info daqcontrol [lindex $result 0]
    dict set info daqcontrolfd [lindex $result 1]

    set result [::GET::startStatePipe $params]
    dict set info state [lindex $result 0]
    dict set info statefd [lindex $result 1]

    set result [::GET::startNsclRouter $params]
    dict set info nsclrouter [lindex $result 0]
    dict set info routeroutput [lindex $result 1]

    set result [::GET::startRingMerge $params]
    dict set info ringmerge [lindex $result 0]
    dict set info mergeout [lindex $result 1]

    set ::GET::activeProviders($id) $info;         # Update the dict.

    after 5000;                                    # Wait for the start

    # Since it takes time to run stuff, updating comes at the end

    ::GET::checkAndUpdatePid [dict get $params eccip] getEccSoapServer getpids
    ::GET::checkAndUpdatePid [dict get $params spdaq] nscldatarouter routerpids
    ::GET::checkAndUpdatePid [exec hostname] ringmerge ringmergepids
    lappend ::GET::sharedInfo(pipepids) {*}[::GET::removeDuplicates \
    [list [dict get $info daqcontrol] [dict get $info state]] $::GET::sharedInfo(pipepids)]
}

##
# ::GET::startPeriodicCheck
#    Starting asynchronous periodic process check for DatasourceManager
#
proc ::GET::startPeriodicCheck {host name pid} {
    puts "Initiate process check of $name ($pid) in $host every $::GET::checkPeriod ms"

    ::GET::checkProcess $host $name $pid
}

##
# ::GET::checkProcesses
#    Checks if the process having matching name and pid is 
#    running in the given host. The result is ORed with the other
#    processes' results, then updates global status variable
#
# @param host - the host where the process is running
# @param name - process name supposed to be running in the hose
# @param pid  - process pid supposed to be running in the host
#
proc ::GET::checkProcess {host name pid} {
    if {!$::GET::checkEnabled} {
        return
    }

    set status 1

    set remotePids [RemoteUtil::remotePid $host $name]

    set index [lsearch $remotePids $pid]
    if {$index < 0} {
        set status 0
    }
    
    set ::GET::sharedInfo(status) [expr $::GET::sharedInfo(status) & $status]

    if {$::GET::checkEnabled} {
        after $::GET::checkPeriod [list ::GET::checkProcess $host $name $pid]
    }
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
    
    set statefd [dict get $info statefd]
    set statehost [::GET::ringhost [dict get $params stateuri]]
    set statering [::GET::ringname [dict get $params stateuri]]
    set sourceid  [dict get $params datasourceid]
    
    dict set info runstarttime [clock seconds]
    dict set info runtitle $title
    dict set info runNumber $num
    
    set path [file join $::GET::getNsclDaqBindir insertstatechange]

    set command [join [list $path --ring=$statering --run=$num --title=\"$title\" \
            	--source-id=$sourceid --type=begin]]
    
    puts "Emitting begin run on '$statehost' using '$command'"
    puts $statefd $command
    
    # If that worked we can update the sources dict:

    dict set info beginEmitted 1

    set ::GET::activeProviders($id) $info

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
proc ::GET::changeAcquisitionState {id program control} {
    set info [::GET::getActiveSource $id]
    set params [dict get $info parameterization]

    set daqcontrolfd [dict get $info daqcontrolfd]
    set ecchost [dict get $params eccip]
    set eccsvc [dict get $params eccservice]

    set program [file join $::GET::getBinDir $program]
    set command [list $program --host=$ecchost --port=$eccsvc $control]

    puts $daqcontrolfd $command
}
##
# ::GET::startAcquisition
#    Starts the data acquisition in the GET modules to the data router.
#    The assumption is that any begin run item that was needed is already
#    on its way through the system...at least has been seen by the merge program.
#
# @param params - data source parameterization.
#                 We need the coboip, coboservice the spdaq and eccip and
#                 eccservice
#                 
proc ::GET::startAcquisition id {
    set readyToStart 1
    set alreadyStarted 0

    foreach otherid [array names ::GET::activeProviders] {
        if {$id ne $otherid} {
            set otherSource [::GET::getActiveSource $otherid]

	    if {[dict get $otherSource beginEmitted] eq 0} {
                set readyToStart 0
	    }
        }
    }


    set source [::GET::getActiveSource $id]
    if {[dict get $source beginEmitted] eq 1 && $readyToStart eq 1} {
        foreach otherid [array names ::GET::activeProviders] {
            if {$id ne $otherid} {
                set otherSource [::GET::getActiveSource $otherid]

                if {[dict get $otherSource runStatus] eq 0} {
                    dict set otherSource runStatus 1
                    set ::GET::activeProviders($otherid) $otherSource
                } else {
                    set alreadyStarted 1
                }
            }
        }
    }


    if {$alreadyStarted eq 0 && $readyToStart eq 1} {
        ::GET::changeAcquisitionState $id getEccSoapClient start

        dict set source runStatus 1
        dict set source beginEmitted 0
        set ::GET::activeProviders($id) $source

        foreach otherid [array names ::GET::activeProviders] {
            if {$id ne $otherid} {
                set otherSource [::GET::getActiveSource $otherid]

                dict set otherSource beginEmitted 0
                set ::GET::activeProviders($otherid) $otherSource
            }
        }
    }
}
##
# ::GET::stopAcquisition
#    Stop acquisition in the GET hardware.
#
# @param params - the data source parameters.
proc ::GET::stopAcquisition id {
    set alreadyStopped 0

    foreach otherid [array names ::GET::activeProviders] {
        if {$id ne $otherid} {
            set otherSource [::GET::getActiveSource $otherid]

            if {[dict get $otherSource runStatus] eq 1} {
		    dict set otherSource runStatus 0
		    set ::GET::activeProviders($otherid) $otherSource
            } else {
		    set alreadyStopped 1
	    }
        }
    }

    if {$alreadyStopped eq 0} {
        ::GET::changeAcquisitionState $id getEccSoapClient stop

        set source [::GET::getActiveSource $id]

        dict set source runStatus 0
        set ::GET::activeProviders($id) $source

        after 5000;                  # Let this run down.
    }
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
    
    set statefd [dict get $state statefd]
    set statehost [::GET::ringhost [dict get $params stateuri]]
    set statering [::GET::ringname [dict get $params stateuri]]
    set sourceid  [dict get $params datasourceid]
    set title [dict get $state runtitle]
    set num   [dict get $state runNumber]
    set duration [expr {[clock seconds] - [dict get $state runstarttime]}]

    set path [file join $::GET::getNsclDaqBindir insertstatechange]
    
    set command [join [list $path --ring=$statering --run=$num \
                --title=\"$title\" --source-id=$sourceid --type=end\
                --offset $duration]]

    puts "Emitting end run at: '$statehost' using '[list $path --ring-$statering]'"
    puts $statefd $command
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
#     Kills a process. Close pipe and kill SSH session if none uses them.
#
# @param info - the dict that represents the state and parameterization of the
#               data source.
# @param host - The host in which the process is running.
# @param pidkey,pipekey - The info keys that have the pid and pipe fd for the
#               process.  If these are set we'll try to use them to do a targeted
#               kill.
# @param processname - If pidkey and/or pipekey are not defined, we'll use this
#              to try to do a killall of processname (nuclear option - pun intended).
# @param arrayid - array key to sharedInfo having list of pids of the processname 
#                  started within this DAQ
#              
# @note - if pidkey and pipekey are defined, then they are unset from
#         the info dict and that's used to update the source's dict.
#         Note that the source id is [dict get $info parameterization sourceid]
#
proc ::GET::killProcess {info host pidkey pipekey processname arrayid} {
    #  Figure out the PID of the remote process
    if {$processname ne "statepipe" && $processname ne "daqcontrolpipe"} {
        puts "Killing $processname"
        set pidList [::RemoteUtil::remotePid $host $processname]

        foreach pid $pidList {
            puts "Killing $pid in $host ($processname)"
            RemoteUtil::kill $host $pid

            set index [lsearch $::GET::sharedInfo($arrayid) $pid]
            if {$index >= 0} {
                set $::GET::sharedInfo($arrayid) \
                [lreplace $::GET::sharedInfo($arrayid) $index $index]
            }
        }
    }

    if {[dict exists $info $pipekey]} {
        set pipepid [dict get $info $pidkey]

        # The pipe fd can be closed:
        if {$processname eq "statepipe" || $processname eq "daqcontrolpipe"} {
            set index [lsearch $::GET::sharedInfo($arrayid) $pipepid]

            if {$index >= 0} {
                set $::GET::sharedInfo($arrayid) \
                    [lreplace $::GET::sharedInfo($arrayid) $index $index]
            }
        }

        # Ignore error while killing
        catch {
            puts "Killing pipe process $pidkey ($pipepid) for $processname"
            exec kill -9 $pipepid
        }

        # Ignore error while closing
        catch {
            puts "Closing pipe process $pidkey ($pipepid) for $processname"
            close [dict get $info $pipekey]
        }

        dict unset $info $pidkey
        dict unset $info $pipekey
    }
}
##
# ::GET::startEccSoapServer
#     Starts the getEccServer program. If multiple datasources share
#     the same getEccServer host, starts only one, then shares the information.
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
proc ::GET::startEccSoapServer params {
    set host [dict get $params eccip]
    set port [dict get $params eccservice]

    set sourceid [dict get $params sourceid]

    foreach id [array names ::GET::activeProviders] {
        if {$id ne $sourceid} {
            set otherSource [::GET::getActiveSource $id]
            set otherParams [dict get $otherSource parameterization]

            if {[dict get $otherParams eccip] eq $host
                && [dict get $otherParams eccservice] eq $port} {
                    set pid [dict get $otherSource geteccserver]
                    set fd [dict get $otherSource eccserveroutput]

                    return [list $pid $fd]
            }
        }
    }

    set command [file join $::GET::getBinDir getEccSoapServer]
    
    set info [ssh::sshpid $host [list $command --port=$port --config-repo-url=[dict get $params workdir]]]
    
    set pid [lindex $info 0]
    set fd  [lindex $info 1]

    set serverIndex $::GET::sharedInfo(numGetEccServers)

    ::GET::setupOutputHandler getEccServer_$serverIndex $fd

    incr ::GET::sharedInfo(numGetEccServers)

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

#                 -  eccservice - The service number on which e  Server accepts connections.  
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
    
    set controlip [dict get $params controlip]
    set controlsvc [dict get $params controlservice]
    
    set dataip [dict get $params dataip]
    set datasvc [dict get $params dataservice]
    
    set ringname [::GET::ringname [dict get $params datauri]]
    set srcid    [dict get $params datasourceid]
    set tsSrc    [dict get $params timestampsource]

    
    set program \
        [file normalize [file join $::GET::getNsclDaqBindir nscldatarouter]]
    
    set info [ssh::sshpid $host [list $program \
	--controlservice $controlip:$controlsvc \
	--dataservice $dataip:$datasvc        \
        --ring $ringname --id $srcid --timestamp $tsSrc --outputtype RingBuffer]]
    
    set pid [lindex $info 0]
    set fd [lindex $info 1]

    set outring  [dict get $params outputring]
    ::GET::setupOutputHandler $outring $fd

    return [list $pid $fd]
}

proc ::GET::checkAndUpdatePid {host process arrayid} {
    set remotePids [RemoteUtil::remotePid $host $process]
    
    set remotePids [::GET::removeDuplicates $remotePids $::GET::sharedInfo($arrayid)]

    if {[llength $remotePids] > 0} {
        lappend ::GET::sharedInfo($arrayid) $remotePids
    }

    foreach pid $remotePids {
        set index [lsearch $::GET::checkingPids $pid]
        if {$index < 0} {
            ::GET::startPeriodicCheck $host $process $pid
        }
    }
}

##
# ::GET::removeOwnedPids
#    Remove PIDs matching with ones in sharedInfo(arrayid)
#    which means matching processes in PIDs are started by this DAQ
#
# @param pids - list of PIDs whose PIDs will be removed if they're
#               from this DAQ
# @param arrayid - array key for the list in ::GET::sharedInfo
#                  having PIDs for a process
#
proc ::GET::removeOwnedPids {pids arrayid} {
    set pids [::GET::removeDuplicates $pids $::GET::sharedInfo($arrayid)]

    return $pids
}

##
# ::GET::removeDuplicates
#    lista - listb. If no matching element exists in lista,
#    lista is returned.
#
# @param lista - source list
# @param listb - list having elements to be deleted from list a
proc ::GET::removeDuplicates {lista listb} {
    foreach elem $listb {
        set index [lsearch $lista $elem]
        if {$index >= 0} {
            set lista [lreplace $lista $index $index]
        }
    }

    return $lista
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
        --output $outring]                 \
    ]
    
    set pid [lindex $info 0]
    set fd [lindex $info 1]
    ::GET::setupOutputHandler $outring $fd
    
    return [list $pid $fd]
}

##
# ::GET::startlPipe
#    Starts a SSH pipe for multiple command sending
#
# @param       name - identifier for the pipe pid
# @param     fdname - identifier for the pipe fd
# @param     params - the parameterization dictionary of the data source.
# @param comparator - parameter in params to compare for a unique pipe
# @param        uri - 1 if URI is provided for comparator
#
proc ::GET::startPipe {name fdname params comparator uri} {
    set sourceid [dict get $params sourceid]

    set host [dict get $params $comparator]
    if {$uri} {
        set host [::GET::ringhost $host]
    }

    foreach id [array names ::GET::activeProviders] {
        if {$id ne $sourceid} {
            set otherSource [::GET::getActiveSource $id]
            set otherParams [dict get $otherSource parameterization]
            set otherHost [dict get $otherParams $comparator]
            if {$uri} {
                set otherHost [::GET::ringhost $otherHost]
            }

            if {$host eq $otherHost} {
                set pid [dict get $otherSource $name]
                set fd [dict get $otherSource $fdname]

                return [list $pid $fd]
            }
        }
    }

    set info [ssh::sshpid $host bash]

    set pid [lindex $info 0]
    set fd [lindex $info 1]

    ::GET::setupOutputHandler "$name Log" $fd

    return $info
}

##
# ::GET::startDaqcontrolPipe
#     @param params - the parameterization dictionary of the data source
#
proc ::GET::startDaqcontrolPipe params {
    return [::GET::startPipe daqcontrol daqcontrolfd $params spdaq 0]
}

##
# ::GET::startStatePipe
#     @param params - the parameterization dictionary of the data source
#
proc ::GET::startStatePipe params {
    return [::GET::startPipe state statefd $params stateuri 1]
}

##
# ::GET::makeRing
#     Make a ring buffer in a remote host.
#
# @param host - name/IP of the host in which to create the ringbufer
# @param name - name of the ring buffer being created.
#
proc ::GET::makeRing {host name} {
    set program [file join $::GET::daqbin  ringbuffer]
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
proc ::GET::setupOutputHandler {tabtitle fd} {
    fconfigure $fd -buffering line -blocking 0
    fileevent $fd readable [list ::GET::handleOutput $tabtitle $fd]
}

##
# ::GET::handleOutput
#    - If the fd is at eof, unset the file event handler.  The fd will
#      be closed by other code.
#    - Otherwise, read a line from the file and
#      submit it as a log message to the console.
#
# @param fd - File descriptor that is readable.
proc ::GET::handleOutput {tabtitle fd} {
    if {[eof $fd]} {
        fileevent $fd readable ""
    } else {
        set line [gets $fd]
        if {[string match -nocase "*error*" $line]} {
            ReadoutGUIPanel::Log $tabtitle error $line
        } elseif {[string match -nocase "*warning*" $line]} {
            ReadoutGUIPanel::Log $tabtitle warning $line
        } else {
            ReadoutGUIPanel::Log $tabtitle log $line
        }
    }
}

#------------------------------------------------------------------------------
#
#  User interface.
#    We need to provide control over whether or not the pulser is started.
#

##
# GET::SetupGui
#
#    Add a frame at the bottom of . that's labeled for GET and
#    contains a checkbutton that supports enabling and disabling the
#    ASAD/CoBo test pulser.  This must be checked when doing pulser runs
#    or else you won't get any data.  It must be unchecked when not doing
#    pulser runs or else the results are not known.
#
#    Additionally, essential parts of GETController are adopted like
#     - Selecting/modifying hardware description file
#     - Selecting/modifying CoBo configuration file
#     - Configuting CoBo using the information provided by those above
#
#    Widgets:
#     - GET::startPulser will reflect the state of the button and
#     - GET::pulserCheckButton will be the widget path to the checkbutton.
#     - GET::hwdesc & GET::configuration
#       two above have TextField, BrowserButton, and ModifyButton as postfix
#       for displayiing selected file path, for selecting of file, and for
#       executing configuration editor, respectively.
#     - GET::startConfiguringButton executes GET::startConfiguring
#       to configure CoBo(s) using provided configuration files.
#

proc GET::SetupGui {} {
    ttk::labelframe .getcontrols -text {GET controls}
    ttk::frame .getcontrols.statecontrols

    set ::cconfiged        [file join $::GET::getBinDir cconfiged]

    set ::GET::configurationBrowserButton [ttk::button .getcontrols.configurationBrowserButton \
    -text "Select Configuration" -state disabled \
    -command {GET::openFileBrowser "Select Configuration File" GET::configurationFile}]
    set ::GET::configurationTextField [ttk::entry .getcontrols.configurationTextField \
    -textvariable GET::configurationFile -state readonly]

    set ::GET::datalinksBrowserButton [ttk::button \
    .getcontrols.datalinksBrowserButton -text "Select Datalinks file" -state disabled \
    -command {GET::openFileBrowser "Select Datalinks File" GET::datalinksFile}]
    set ::GET::datalinksTextField [ttk::entry \
    .getcontrols.datalinksTextField \
    -textvariable GET::datalinksFile -state readonly]

    set ::GET::describeButton [ttk::button \
    .getcontrols.statecontrols.describeButton -text "Describe" -state disabled \
    -command {GET::getEccSoapClientCommand "describe"}]

    set ::GET::undoButton [ttk::button \
    .getcontrols.statecontrols.undoButton -text "Undo" -state disabled \
    -command {GET::getEccSoapClientCommand "undo"}]

    set ::GET::prepareButton [ttk::button \
    .getcontrols.statecontrols.prepareButton -text "Prepare" -state disabled \
    -command {GET::getEccSoapClientCommand "prepare"}]

    set ::GET::breakupButton [ttk::button \
    .getcontrols.statecontrols.breakupButton -text "Breakup" -state disabled \
    -command {GET::getEccSoapClientCommand "breakup"}]

    set ::GET::configureButton [ttk::button \
    .getcontrols.statecontrols.configureButton -text "Configure" -state disabled \
    -command {GET::getEccSoapClientCommand "configure"}]

    set padx 1
    set pady 1

    grid columnconfigure .getcontrols 0 -weight 1

    grid $::GET::configurationTextField     -row 0 -column 0 -columnspan 3 -padx $padx -sticky we
    grid $::GET::configurationBrowserButton -row 0 -column 3 -padx $padx -pady $pady -sticky we

    grid $::GET::datalinksTextField     -row 1 -column 0 -columnspan 3 -padx $padx -sticky we
    grid $::GET::datalinksBrowserButton -row 1 -column 3 -padx $padx -pady $pady -sticky we

    set padx 10
    set pady 40
    set revertGap 30

    grid .getcontrols.statecontrols -row 2 -column 0 -columnspan 5 -sticky nsew
    grid columnconfigure .getcontrols.statecontrols 0 -weight 1
    grid columnconfigure .getcontrols.statecontrols 2 -weight 1
    grid columnconfigure .getcontrols.statecontrols 4 -weight 1
    grid $::GET::describeButton  -row 0 -column 0 -padx $padx -pady $pady -sticky we
    grid $::GET::undoButton      -row 0 -column 1 -padx $revertGap -pady $pady -sticky we
    grid $::GET::prepareButton   -row 0 -column 2 -padx $padx -pady $pady -sticky we
    grid $::GET::breakupButton   -row 0 -column 3 -padx $revertGap -pady $pady -sticky we
    grid $::GET::configureButton -row 0 -column 4 -padx $padx -pady $pady -sticky we

    grid .getcontrols -sticky nsew

    ttk::labelframe .getpartialcontrols -text {GET partial controls}

    set ::GET::beginPartialButton    [ttk::button .getpartialcontrols.beginPartialButton \
    -text "Begin" -state disabled \
    -command {GET::getEccSoapClientCommand "begin-partial"}]
    set ::GET::coboCSVListLabel      [ttk::label .getpartialcontrols.coboCSVListLabel \
    -text "CoBo List:" -justify right]
    set ::GET::coboCSVListField      [ttk::entry .getpartialcontrols.coboCSVListField \
    -textvariable GET::coboCSVList -state disabled]
    set ::GET::breakupPartialButton  [ttk::button .getpartialcontrols.breakupPartialButton \
    -text "Breakup" -state disabled \
    -command {GET::getEccSoapClientCommand "breakup-partial" $::GET::coboCSVList}]
    set ::GET::preparePartialButton  [ttk::button .getpartialcontrols.preparePartialButton \
    -text "Prepare" -state disabled \
    -command {GET::getEccSoapClientCommand "prepare-partial" $::GET::coboCSVList}]
    set ::GET::configurePartialButton  [ttk::button .getpartialcontrols.configurePartialButton \
    -text "Configure" -state disabled \
    -command {GET::getEccSoapClientCommand "configure-partial" $::GET::coboCSVList}]
    set ::GET::finishPartialButton  [ttk::button .getpartialcontrols.finishPartialButton \
    -text "Finish" -state disabled \
    -command {GET::getEccSoapClientCommand "finish-partial"}]

    set padx 1
    set pady 1

    grid columnconfigure .getpartialcontrols 2 -weight 1

    grid $::GET::beginPartialButton     -row 0 -column 0 -padx $padx -pady $pady -sticky we
    grid $::GET::coboCSVListLabel       -row 0 -column 1 -padx $padx -pady $pady -sticky we
    grid $::GET::coboCSVListField       -row 0 -column 2 -padx $padx -pady $pady -sticky we
    grid $::GET::breakupPartialButton   -row 0 -column 3 -padx $padx -pady $pady -sticky we
    grid $::GET::preparePartialButton   -row 0 -column 4 -padx $padx -pady $pady -sticky we
    grid $::GET::configurePartialButton -row 0 -column 5 -padx $padx -pady $pady -sticky we
    grid $::GET::finishPartialButton    -row 0 -column 6 -padx $padx -pady $pady -sticky we

    grid .getpartialcontrols -sticky nsew
}

##
# GET::checkFileSelection
#    Checks if user ends up with canceling the popup window, which returns
#    an empty string instead of leaving the previous file selection unchanged.
#    If an empty string is provided, leave the filename variable unchanged.
#
proc GET::checkFileSelection {filename returnValue} {
    # User selected a file
    if {$returnValue ne ""} {
        return $returnValue
    } 

    # User cancellation
    if {[info exists $filename]} {
        return [set $filename]
    }

    # Default value is empty
    return {}
}

##
# GET::openFileBrowser
#    Wrapper for tk_getOpenFile with file type configuration
#
proc GET::openFileBrowser {title returnVariable} {
    set ::filetypes {
        {{XML Files} {.xml}}
        {{All Files}  *      }
    }

    set $returnVariable [GET::checkFileSelection $returnVariable \
                         [tk_getOpenFile -title $title -filetypes $::filetypes]]
}

##
# GET::getEccSoapClientCommand
#   Sending command using getEccSoapClient to getEccSoapServer
#
proc GET::getEccSoapClientCommand {control {coboids -1}} {
    set info [::GET::getActiveSource 0]
    set params [dict get $info parameterization]

    set daqcontrolfd [dict get $info daqcontrolfd]
    set ecchost [dict get $params eccip]
    set eccsvc [dict get $params eccservice]

    set configuration [exec cat [file join $::GET::configurationFile] | tr -d '\t' | tr -d '\n']
    set datalinks [exec cat [file join $::GET::datalinksFile] | tr -d '\t' | tr -d '\n']

    set program [file join $::GET::getBinDir getEccSoapClient]
    if {$control in [list "breakup-partial" "prepare-partial" "configure-partial"]} {
      set coboids [string map {"," " "} $coboids]
      foreach coboid $coboids {
        set command [list $program --host=$ecchost --port=$eccsvc $control '$configuration' $coboid]

        puts $daqcontrolfd [join $command]
      }
    } else {
      set command [list $program --host=$ecchost --port=$eccsvc $control '$configuration' '$datalinks']
      puts $daqcontrolfd [join $command]
      if {$control eq "breakup"} {
        set command [list $program --host=$ecchost --port=$eccsvc undo '$configuration' '$datalinks']

        puts $daqcontrolfd [join $command]
      }
    }
}

proc ::GET::attach state {}

##
#  Now we need a bundle to enable/disable the widgets in GET group
#

proc ::GET::enter {from to} {
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::configurationBrowserButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::datalinksBrowserButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::describeButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::undoButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::prepareButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::breakupButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::configureButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::beginPartialButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::coboCSVListField
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::breakupPartialButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::preparePartialButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::configurePartialButton
    ::GET::changeWidgetStateWhenEnter $from $to $::GET::finishPartialButton

    if {$from eq "Starting" && $to eq "Halted"} {
        tk_messageBox \
            -icon info -title "GET datasource" -type ok \
            -message "Action Needed!" \
            -detail "Remember to configure CoBo(s) and check the daqcontrol tab status!"
    }
}
##
# GET::changeWidgetStateWhenEnter
#    If widget exists and run state changes to Active,
#    change the widget state to disabled.
#    All the other run states, widget is in enabled state.
#
# @param from - initial run state
# @param to - target run state
# @param widget - widget to change state
#
proc ::GET::changeWidgetStateWhenEnter {from to widget} {
    if {[winfo exists $widget]} {
        if {$to eq "Active"} {
            $widget configure -state disabled
        } else {
            $widget configure -state enabled
        }
    }
}

proc ::GET::leave {from to} {}

#  Need to export the namespace procs and methods.

namespace eval ::GET {
    namespace export attach enter leave
    namespace ensemble create
}


# need to regtister the bundle with the state machine.... if its' not already
# registered.

set GET::sm [RunstateMachineSingleton %AUTO%]
if {"GET" ni [$::GET::sm listCalloutBundles]} {
    $GET::sm addCalloutBundle GET    
}
