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
# @file getPrompt.tcl
# @brief Provides a prompter for a GET electronics data source
# @author Ron Fox <fox@nscl.msu.edu>
#
package provide GET_Prompter 1.0
package require Tk
package require snit

package require DataSourceUI
package require Iwidgets

namespace eval GET {
    
    # Directory defs:
    #
    #  - installedIn - the directory we're in $prefix/TclLibs/
    #  - getBinDir   - Directory the GET software runnables are in.
    #  - getNsclDaqBindir - Directory our binaries are in.
    #  - daqbin      - Where the NSCLDAQ programs are installed.
    #

    variable installedIn  [file dirname [info script]];  #Helpdir too.
    variable getNsclDaqBindir [file normalize [file join $installedIn .. bin]]
    #
    #  This defines the set of configurable parameters for get data sources.
    #  It's a template of the dict that will be used to initialize/parameterize
    #  the source.  Keys are parameter names, values are a list containing
    #  the meaning of the parameter that is assigned to that key
    #
    variable parameterization [dict create \
        spdaq [list {Host that is connected to GET uTCA crate}] \
        eccip [list {IP address on private subnet of ecc server}] \
        dataip    [list {IP Address on the private subnet of the data flow service}]    \
        controlip [list {Ip Address of router control port}] \
        controlservice [list {Service number of router control port}] \
        coboip      [list {CoBo IP address}]                          \
        coboservice [list {CoBo control port}] \
        dataservice [list {Service number for the data flow service}] \
        eccservice [list {Service on which the ecc server listens for requests}] \
        datauri     [list {URI of ring into which GET frames are put as ring items}] \
        stateuri    [list {URI of ring into which run state change items are put}] \
        outputring  [list {Ring name into which datauri and stateuri items are merged}] \
        timestampsource [list {Identifies what's put in to the timestamp of GET ring items}] \
        datasourceid    [list {Source Id put into body headers for this source}]   \
        workdir         [list {Workdir containing configuration-*.xml files}] \
    ]
    #
    #  This provides a lookup table between the keys above and option names in
    #  the combined prompter widget
    #

}
array set ::GET::optionlookup [list                                      \
    spdaq -spdaq eccip -eccip eccservice -eccservice               \
    datauri -datauri stateuri -stateuri outputring -outputring           \
    timestampsource -timestampsource datasourceid -sourceid                   \
    dataip -dataip dataservice -dataservice                                 \
    controlip -controlip controlservice -controlservice                      \
    coboip -coboip coboservice -coboservice                              \
    workdir -workdir \
]

##
# GET::Service
#    Prompts for a dotted IP address and service number.
#
#
snit::widgetadaptor ::GET::Service {
    option -service -default 0
    option -ip      -default 0.0.0.0 -configuremethod _setIp -cgetmethod _getIp
    
    constructor args {
        installhull using ttk::frame
        
        set widgetlist [list]
        foreach octet {1 2 3 4} delim {. . . :} {
            $self _createOctet $win.octet$octet
            label $win.delim$octet -text $delim
            $win.octet$octet insert end 0
            lappend widgetlist $win.octet$octet $win.delim$octet
        }
        
        # The service:
        
        entry $win.service -textvariable [myvar options(-service)] -width 6
        bind $win.octet4 <colon> [list after 100 [mymethod _focusService]]
        
        grid {*}$widgetlist $win.service 
    }
    #
    # _createOctet
    #   Creates an entry for octests.
    #    @param w - window path.
    #    @return win
    #
    method _createOctet w {
        set w [ttk::entry $w                                              \
            -width 3 -validate focus                                       \
            -validatecommand [mymethod _isOctet %W]                        \
        ]
        bind $w <period> [list after 100 [mymethod _focusnext $w]]
        
        return $w
    }
    ##
    # _validOctet
    #   See _isOctet
    #
    # @param value - value proposed for an octet.
    # @return bool - true if ok false if not.
    #
    proc _validOctet value {
       
        if {![string is integer -strict $value]} {
            return 0
        }
        
        set result [expr {($value >= 0) && ($value <= 255)}]
        
        return $result
    }
    ##
    # _isOctet
    #   Valid octets must be integers in the range [0, 255].
    #
    # @param w - entry widget being validated.
    # @return bool -true if the contents of the entry are a valid octet.
    #
    method _isOctet {w} {
        set value [$w get]
        set result [_validOctet $value]
        
        if {!$result} {
            $w delete 0 end
            $w insert end 0
        }

        return $result
    }
    ##
    # _focusnext
    #    Transfer focus to the next octet in the IP.  IF we're at the last
    #    one leave it there.  This is bound to the .
    #    note that we need to remove the period from the entry.
    #
    # @param w - widget currently having focus.
    #
    method _focusnext w {
        
        set s [$w get]
        set i [expr {[string length $s] - 1}]
        $w delete $i;           # removes the .

    
        set octetNum [string index $w end]
        incr octetNum
        if {$octetNum <= 4} {
            set nextw [string replace $w end end $octetNum]
            focus $nextw
        }
    }
    
    ##
    # _focusService
    #   Removes the last character from the last octet entry and
    #   focuses on $win.service
    #
    method _focusService {} {
        set s [$win.octet4 get]
        set i [expr {[string length $s] - 1}]
        $win.octet4 delete $i;           # removes the .

        focus $win.service
    }
    
    ##
    # _setIp
    #     Called to configure the IP address.
    #     Throws an error if the proposed IP is not valid.
    #
    #  @param optname - name of the option being processed (not used).
    #  @param value   - proposed new value.
    #
    method _setIp {optname value} {
        set octets [split $value .]
        if {[llength $octets] ne 4} {
            error "$value is not a valid IP address"
        }
        # validate them all first.
        
        foreach octet $octets {
            if {![_validOctet $octet]} {
                error "$value is not a valid IP address"
            }
        }
        #  Set the text entries:
        
        foreach octet $octets       \
            w [list $win.octet1 $win.octet2 $win.octet3 $win.octet4] {
            $w delete 0 end
            $w insert end $octet
        }
    }
    ##
    # _getIp
    #    build the IP address from the four text entry fields.
    #
    # @param optname - name of the option being fetched (ignored).
    #
    method _getIp optname {
        set octetList [list]
        foreach w [list $win.octet1 $win.octet2 $win.octet3 $win.octet4]  {
            lappend octetList [$w get]
        }
        return [join $octetList .]
    }
    

}

##
#   GET parameters are into three parts divided:
#     * host parameters - IP addresses and DNSnames along with service ports.
#     * Ring buffer parameters.
#     * Miscellaneous parameters.
#
#  Each of these gets its own megawidget to prompt for its parameters.
#

##
# Get::HostPrompts
#
#  Prompt for host parameters we need:
#   -   spdaq dns name - public network name of the GET interface host.
#   -   Get network IP - Private IP address of the GET interface host.
#   -   Data service (defaults to 46005)
#

snit::widgetadaptor ::GET::HostPrompts {
    component eccservice
    component controlservice
    component dataservice
    component coboservice
    
    
    option -spdaq     -default localhost
    
    delegate option -eccip to eccservice as -ip
    delegate option -eccservice to eccservice as -service
    
    delegate option -controlip to controlservice as -ip
    delegate option -controlservice to controlservice as -service
    
    delegate option -dataip to dataservice as -ip
    delegate option -dataservice to dataservice as -service
    
    delegate option -coboip to coboservice as -ip
    delegate option -coboservice to coboservice as -service

    option -workdir

#    option -dataservice -default 46005
# (controlservice    option -eccservice   -default 46003
    
    constructor args {
        installhull using ttk::frame;               # Install stuff in themed frame.
        
        ttk::label $win.publiclabel -text "Public IP DNS name"
        ttk::entry $win.publicip    -textvariable [myvar options(-spdaq)] -width 25
        
        ttk::label $win.ecclabel -text "getEccSoapServer"
        install eccservice using GET::Service $win.eccservice
        
        ttk::label $win.ctllabel -text "Data router control"
        install controlservice using GET::Service $win.controlservive
        
        ttk::label $win.datalabel -text "Data router data"
        install dataservice using GET::Service $win.dataservice
        
        ttk::label $win.cobolabel -text "Cobo target service"
        install coboservice using GET::Service $win.coboservice

        ttk::label $win.workdirlabel -text "Working dir"
        ttk::entry $win.workdir      -textvariable [myvar options(-workdir)] -width 25
        
        ##
        #  Lay stuff out.
        
        grid $win.publiclabel $win.publicip
        grid $win.ecclabel $eccservice
        grid $win.ctllabel  $controlservice
        grid $win.datalabel $dataservice
        grid $win.cobolabel $coboservice
        grid $win.workdirlabel $win.workdir
        
        
        # Set defaults for service numbers.
        
        $self configure -eccservice 46002
        $self configure -coboservice 46001
        $self configure -controlservice 46003
        $self configure -dataservice 46005
        
        # Configure optinos supplied.
        
        $self configurelist $args
    }
}

##
#  GET::RingPrompts
#    Prompts for the data ring, the state change ring and the output ring.
#    The two input rings must be URLs while the output rings need not be.
#
#
snit::widgetadaptor ::GET::RingPrompts {
    option -datauri     -default tcp://localhost/data  -configuremethod _setUri
    option -stateuri    -default tcp://localhost/state -configuremethod _setUri
    option -outputring  -default GET
    
    constructor args {
        installhull using ttk::frame
        
        ttk::label $win.datalabel -text "URI Of GET data ring"
        $self _makeUri $win.data -datauri
        
        ttk::label $win.statelabel -text "URI of Begin/End run rings"
        $self _makeUri $win.state  -stateuri
        
        ttk::label $win.outputlabel -text "Name of final data ring (in localhost)"
        ttk::entry $win.output -textvariable [myvar options(-outputring)] -width 26
        
        $self configurelist $args
        
        grid $win.datalabel   $win.data
        grid $win.statelabel  $win.state
        grid $win.outputlabel $win.output
    }
    #---------------------------------------------------------------------------
    #   Local methods.
    
    ##
    # makUri
    #   Make a URI prompting entry.
    # @param w  - complete windowp ath.
    # @param opt - name of the option controlled by this entry.
    # @return w
    #
    method _makeUri {w opt} {
        set result [ttk::entry $w         \
            -width 26 -textvariable [myvar options($opt)]              \
            -validate focus -validatecommand [mymethod _validateUri $w] \
        ]
        return $result
    }
    ##
    # _setUri
    #   configuremethod to change a URI option
    #   - validate the proposed URI
    #   - Toss an error if it's bad,
    #   - set the option if not.
    #
    # @param optname - name of the option being modified.
    # @param value   - Proposed new value.
    # 
    method _setUri {optname value} {
        if {[_isUri $value]} {
            set options($optname) $value
        } else {
            error "Invalid proposed URI for $optname: $value"
        }
    }
    ##
    # _validateUri
    #    Validation script for URI inputs.
    #    $w is the widget.
    #    If fails, the default string is restored.
    #
    # @param w - widget being validated.
    method _validateUri {w} {
        set v [$w get]
        if {[_isUri $v]} {
            return 1
        } else {
            $w delete 0 end
            $w insert end tcp://somehost/somering
            return 0
        }
    }
    
    #--------------------------------------------------------------------------
    # Local procsg
    #
    
    ##
    # _isUri
    #   Ensure the string is a tcp URI.
    #
    # @param s - the string to check
    # @return bool 1 if so.
    #
    #  @note URIs are assumed to be of the form tcp://host/ring.
    #
    proc _isUri {s} {
        set components [split $s :/];    #there's two empty elements for good ones.
        
        if {[llength $components] != 5} {return 0}
        if {[lindex $components 0] ne "tcp"} {return 0}
        if {[lindex $components 1] ne "" } {return 0}
        if {[lindex $components 2] ne ""}  {return 0}
    
        # we don't bother to analyze the host and ring name.
        
        return 1
    }
}
##
# GET::MiscellaneousPrompts
#
#   Prompts for:
#      - Timestamp source (one of timestamp or trigger-id)
#      - soruce-id   The source id to put into ring items.
#
#
snit::widgetadaptor  ::GET::MiscellaneousPrompts {
    option -timestampsource -default timestamp -configuremethod _setTimestampSource
    option -sourceid        -default 0 \
        -configuremethod _setSourceId -cgetmethod _getSourceId
    
    constructor args {
        installhull using ttk::frame
        
        ttk::label $win.tslabel -text {Timestamp source:}
        ttk::combobox $win.ts   \
            -state readonly -values [list timestamp trigger_number] \
            -textvariable [myvar options(-timestampsource)]
        
        ttk::label $win.sidlabel -text {Source Id:}
        ttk::spinbox $win.sid    -state readonly -from 0 -to 10000 -increment 1
        $win.sid set $options(-sourceid)
        
        grid $win.tslabel $win.ts    -sticky w
        grid $win.sidlabel $win.sid  -sticky w
        
        $self configurelist $args
    }
    #--------------------------------------------------------------------------
    # Private methods:
    
    ##
    # _setTimestampSource
    #     Validates a new timestamp source before setting it in the option.
    #     the timestamp source must be one the elments inthe
    #     -values list of $win.ts
    #
    # @param optname - name of the option controlled.
    # @param value   - proposed new value
    #
    method _setTimestampSource {optname value} {
        set validValues [$win.ts cget -values]
        if {$value ni $validValues} {
            error "$value is not a valid value for $optname must be one of [join $validValues]"
        }
        set options($optname) $value
    }
    ##
    # _setSourceId
    #   There's no way to bind a variable to a spinbox so we use this
    #   to catch configuring the option.  This allows us also to validate
    #   that the option is a non-negative integer as well.
    #
    # @param optname - name of the configuration option bound to the spinbox
    # @param value   - value proposed for the option.
    #
    method _setSourceId        {optname value} {
        if {[string is integer -strict $value]} {
            if {$value >= 0} {
                $win.sid set $value
            }
        }
        error "$optname value must be a non-negative integer was: $value"
    }
    ##
    # _getSourceId
    #
    # @return integer - the contents of the source id spinbox.
    #
    method _getSourceId        {optname} {
        return [$win.sid get]
    }
}

##
# GET::PromptForm
#    This megawidget is the complete prompt form.  Each of the
#    prompt forms above is pasted into a ttk::labelframe.
#    The options of all of the forms are exported via delegation by this
#    megawidget.
#
snit::widgetadaptor ::GET::GetPromptForm {
    
    # A component for each sub form.
    
    component networkParameters
    component ringParameters
    component miscParameters
    
    # Make the network parameter options visible:
    
    delegate option -spdaq     to networkParameters
    delegate option -eccip to networkParameters
    delegate option -eccservice   to networkParameters
    delegate option -dataip    to networkParameters
    delegate option -dataservice  to networkParameters
    delegate option -controlip  to networkParameters
    delegate option -controlservice to networkParameters
    delegate option -coboip     to networkParameters
    delegate option -coboservice to networkParameters
    delegate option -workdir     to networkParameters
    
    #
    
    # Make the ring buffer parameter options visible:
    
    delegate option -datauri    to ringParameters
    delegate option -stateuri   to ringParameters
    delegate option -outputring to ringParameters
    
    # Make the miscellaneous options visible:
    
    delegate option -timestampsource to miscParameters
    delegate option -sourceid        to miscParameters
 
    constructor args {
        installhull using ttk::frame
        
        ttk::labelframe $win.net  -text {Network parameters}
        ttk::labelframe $win.ring -text {Ringbuffer Parameters}
        ttk::labelframe $win.misc -text {Miscellaneous parameters}
        
        install networkParameters using ::GET::HostPrompts $win.net.form
        install ringParameters    using ::GET::RingPrompts $win.ring.form
        install miscParameters    using ::GET::MiscellaneousPrompts $win.misc.form
        
        grid $win.net.form  -sticky w
        grid $win.ring.form -sticky w
        grid $win.misc.form -sticky w
        
        grid $win.net   -sticky ew
        grid $win.ring  -sticky ew
        grid $win.misc -sticky ew
    }
}

#------------------------------------------------------------------------------
#
#   Procs that drive the prompting from the ReadoutGUI point of view:
#


##
# GET::showHelp
#   Displays the help in an iwidgets help widget.
#
proc ::GET::showHelp {} {
    puts "Showhelp"
    if {![winfo exists .gethelp]} {
	puts "Instantiate, helpdir $::GET::installedIn"
	puts "[glob $::GET::installedIn/*.html]"
        iwidgets::hyperhelp .gethelp \
            -topics index -helpdir $::GET::installedIn
        
    }
    
    .gethelp showtopic index
    .gethelp activate
}

##
#  Called by the ReadoutGUI to prompt for parameters for the GET
#  readout:
#
#  @return dict keys are data source parameter names and values are
#          three elements lists containing in order 
#          long prompt, dummy widget name and parameter value
#
proc GET::promptParameters {} {
    destroy .getparamtoplevel
    toplevel .getparamtoplevel
    set dlg [DialogWrapper .getparamtoplevel.dialog]
    set container [$dlg controlarea]

    set frame [ttk::frame $container.frame]
    set form [GET::GetPromptForm $frame.f]
    button $frame.help -text Help -command ::GET::showHelp
    grid $form -sticky nsew
    grid $frame.help -sticky w

    
    $dlg configure -form $frame


    
    pack $dlg -fill both -expand 1
    

    
    set result [$dlg modal]
    set paramdict [dict create]
    if {$result eq "Ok"} {
        # The user wants to accept this; initialize the result from the
        # parameterization dict:
        
        set paramdict $::GET::parameterization
         
         # Augment the dict with the values in the form:
         
         dict for {key value} $paramdict {
            set val [$form cget $::GET::optionlookup($key)]
            dict lappend paramdict $key [list] [string trim $val]
         }
    } 
    
    destroy .getparamtoplevel
    return $paramdict
}

