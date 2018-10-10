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


namespace eval GET {}

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

snit::widgetadaptor GET::HostPrompts {
    option -spdaq     -default localhost 
    option -privateip -default 0.0.0.0 \
        -cgetmethod _getPrivateIp -configuremethod _setPrivateIp
    option -datasvc   -default 46005
    
    constructor args {
        installhull using ttk::frame;               # Install stuff in themed frame.
        
        ttk::label $win.publicttk::label -text "Public IP DNS name"
        ttk::entry $win.publicip    -textvariable [myvar options(-spdaq)] -width 25
        
        ttk::label $win.privateipttk::label -text "Private GET IP address"
        $self _createOctet $win.octet1
        ttk::label $win.octet1delim  -text . 
        $self _createOctet $win.octet2 
        ttk::label $win.octet2delim  -text .
        $self _createOctet $win.octet3 
        ttk::label $win.octet3delim  -text .
        $self _createOctet $win.octet4
        
        ttk::label $win.servicedelim -text :
        ttk::entry $win.dataservice -textvariable [myvar options(-datasvc)] -width 6
             
        $self configurelist $args
        
        # Layout the widgets.
        
        grid $win.publicttk::label
        grid $win.publicip -row 0 -column 1 -columnspan 9
        
        grid $win.privateipttk::label $win.octet1 $win.octet1delim                      \
            $win.octet2 $win.octet2delim $win.octet3 $win.octet3delim $win.octet4 \
            $win.servicedelim $win.dataservice
        
        $self configure -privateip $options(-privateip);   #load the entry.
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
    # _isOctet
    #   Valid octets must be integers in the range [0, 255].
    #
    # @param w - entry widget being validated.
    # @return bool -true if the contents of the entry are a valid octet.
    #
    method _isOctet {w} {
        set value [$w get]
        if {![string is integer -strict $value]} {
            $w delete 0 end
            $w insert end 0
            return 0
        }
        
        set result [expr {($value >= 0) && ($value <= 255)}]
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
        foreach octet $octets                    \
            window [list $win.octet1 $win.octet2 $win.octet3 $win.octet4] {
            
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
snit::widgetadaptor GET::RingPrompts {
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