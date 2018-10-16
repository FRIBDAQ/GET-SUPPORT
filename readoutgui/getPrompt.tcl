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
        dataservice [list {Service number for the data flow service}] \
        eccservice [list {Service on which the ecc server listens for requests}] \
        datauri     [list {URI of ring into which GET frames are put as ring items}] \
        stateuri    [list {URI of ring into which run state change items are put}] \
        outputring  [list {Ring name into which datauri and stateuri items are merged}] \
        timestampsource [list {Identifies what's put in to the timestamp of GET ring items}] \
        sourceid    [list {Source Id put into body headers for this source}]   \
    ]
    #
    #  This provides a lookup table between the keys above and option names in
    #  the combined prompter widget
    #

}
array set ::GET::optionlookup [list                                      \
    spdaq -spdaq eccip -eccip eccservice -eccservice               \
    datauri -datauri stateuri -stateuri outputring -outputring           \
    timestampsource -timestampsource sourceid -sourceid                   \
    dataip -dataip dataservice -dataservice                                 \
]
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
    option -spdaq     -default localhost 
    option -eccip -default 0.0.0.0 \
        -cgetmethod _getPrivateIp -configuremethod _setPrivateIp
    option -dataip    -default 0.0.0.0 \
        -cgetmethod _getCoboIp   -configuremethod _setCoboIp
    option -dataservice -default 46005
    option -eccservice   -default 46003
    
    constructor args {
        installhull using ttk::frame;               # Install stuff in themed frame.
        
        ttk::label $win.publiclabel -text "Public IP DNS name"
        ttk::entry $win.publicip    -textvariable [myvar options(-spdaq)] -width 25
        
        ttk::label $win.privateiplabel -text "Private IP:service for eccserver"
        $self _createOctet $win.octet1
        ttk::label $win.octet1delim  -text . 
        $self _createOctet $win.octet2 
        ttk::label $win.octet2delim  -text .
        $self _createOctet $win.octet3 
        ttk::label $win.octet3delim  -text .
        $self _createOctet $win.octet4
        
        ttk::label $win.servicedelim -text :
        ttk::entry $win.dataservice -textvariable [myvar options(-eccservice)] -width 6
        
        ttk::label $win.coboiplabel -text "Private IP:sevice for dataflow"
        $self _createOctet $win.coctet1
        ttk::label $win.coctet1delim  -text . 
        $self _createOctet $win.coctet2 
        ttk::label $win.coctet2delim  -text .
        $self _createOctet $win.coctet3 
        ttk::label $win.coctet3delim  -text .
        $self _createOctet $win.coctet4
    
        ttk::label $win.cservicedelim -text :
        ttk::entry $win.cservice -textvariable [myvar options(-dataservice)] -width 6
             
        $self configurelist $args
        
        # Layout the widgets.
        
        grid $win.publiclabel
        grid $win.publicip -row 0 -column 1 -columnspan 9
        
        grid $win.privateiplabel $win.octet1 $win.octet1delim                      \
            $win.octet2 $win.octet2delim $win.octet3 $win.octet3delim $win.octet4 \
            $win.servicedelim $win.dataservice
        

        grid $win.coboiplabel $win.coctet1 $win.coctet1delim                      \
            $win.coctet2 $win.coctet2delim $win.coctet3 $win.coctet3delim   \
            $win.coctet4 $win.cservicedelim $win.cservice
        
        
        $self configure -eccip $options(-eccip);   #load the entry.
        $self configure -dataip $options(-dataip)
    }
    #--------------------------------------------------------------------------
    # Private methods:
    
    ##
    # _createOctet
    #   Creates an entry for octests.
    #    @param w - window path.
    #    @return win
    #
    method _createOctet w {
        return [ttk::entry $w                                              \
            -width 3 -validate focus                                       \
            -validatecommand [mymethod _isOctet %W]                        \
        ]
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
    
    #stubs.
    
    ##
    # Combines the private IP octet entries into a single dotted IP value.
    #
    # @param option name - ignored.
    # @return private IP gotten from the octet entries.
    #
    method _getPrivateIp {optname} {
        set octets [list]
        foreach window [list $win.octet1 $win.octet2 $win.octet3 $win.octet4] {
            lappend octets [$window get]
        }
        return [join $octets .]
    }
    ##
    # _setPrivateIp
    #
    #    Configure method for the private ip.
    #  @param optname - name of the option (ignored).
    #  @param value   - new dotted ip.
    #
    method _setPrivateIp {optname value} {
        set octets [split $value .]
        if {[llength $octets] != 4} {
            error "$value is not a valid numeric IP address - must have 4 octets."
        }
        #
        # validate
        #
        foreach octet $octets {
            if {![_validOctet $octet]} {
                error "$octet is not a valid IP octet in $value"
            }
        }
        #  set
        
        foreach octet $octets                    \
            window [list $win.octet1 $win.octet2 $win.octet3 $win.octet4] {
            
            $window delete 0 end
            $window insert end $octet
        }
    }
    ##
    # _getCoboIp
    #    Figure out the cobo IP address from its octetst.
    #
    # @param optname - option name (ignored).
    # @return string - dotted IP address of the cobo.
    #
    method _getCoboIp optname {
        set octets [list]
        foreach window [list $win.coctet1 $win.coctet2 $win.coctet3 $win.coctet4] {
            lappend octets [$window get]
        }
        return [join $octets .]        
    }
    ##
    # _setCoboIp
    #    Validate a proposed cobo IP and, if it's any good, load it into the
    #    octet entries.
    #
    # @param optname - option name being configured (ignored).
    # @param value   - proposed new dotted IP address.
    #
    method _setCoboIp {optname value} {
        set octets [split $value .]
        if {[llength $octets] != 4} {
            error "$value is not a valid numeric IP address - must have 4 octets."
        }
        # Validate:
        
        foreach octet $octets {
            if {![_validOctet $octet]} {
                error "$octet is not a valid octet in $value"
            }
        }
        
        # Set
        
        foreach octet $octets                    \
            window [list $win.coctet1 $win.coctet2 $win.coctet3 $win.coctet4] {
            
            $window delete 0 end
            $window insert end $octet
        }        
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
    if {[winfo exists .gethelp] eq ""} {
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
    
    set form [GET::GetPromptForm $container.f]
    $dlg configure -form $form
    pack $dlg -fill both -expand 1
    
    button .getparamtoplevel.help -text Help -command ::GET::showHelp
    pack .getparamtoplevel.help
    
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

