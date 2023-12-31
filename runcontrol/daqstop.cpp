/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2014-2018.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Authors:
             Ron Fox
             Jeromy Tompkins 
	     NSCL
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/**
 * @file daqstop.cpp
 * @brief Start a run in the GET electronics.
 * @author Ron Fox <fox@nscl.msu.edu>
 */

#include "EccClient.h"
#include <iostream>
#include <string>
#include <stdlib.h>

/**
 *  This program starts a run with the GET electronics.
 *  Usage is:
 *    daqstop hwnode eccserver routerdata
 *
 *  Where:
 *     hwnode - is the hardware node we're going to start (e.g. CoBo[0]).
 *     eccserver is the IP/Service on which getEccServer is listening for a connection.
 *         Normaly, this is getspdaqIP:46002
 *    routerdata is the IP/Service on which the nscldatarouter is expecting data.  Normally this is
 *             getspdaqIP:46005
 * Assumptions:
 *   Prior to running this, GetController must have selected a configuration (Test) and
 *   loaded/configured the hardware.
 *
 *  That means all we need to do is:
 *   -  Select our module.
 *   -  try/catch disconnect the router.
 *   -  connect the router.
 *   -  start the run.
 */
/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2014-2018.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Authors:
             Ron Fox
             Jeromy Tompkins 
	     NSCL
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/**
 * @file daqstop.cpp
 * @brief Start a run in the GET electronics.
 * @author Ron Fox <fox@nscl.msu.edu>
 */

/**
 *  This program starts a run with the GET electronics.
 *  Usage is:
 *    daqstop  hwnode eccserver 
 *
 *  Where:
 *     hwnode - is the hardware node control port we're going to connect.
 *              this is the ip of the COBO and the port 46001 e.g.
 *
 *     eccserver is the IP/Service on which getEccServer is listening for a connection.
 *         Normaly, this is getspdaqIP:46002
 *    routerdata is the IP/Service on which the nscldatarouter is expecting data.  Normally this is
 *             getspdaqIP:46005
 * Assumptions:
 *   Prior to running this, GetController must have selected a configuration (Test) and
 *   loaded/configured the hardware.
 *
 *  That means all we need to do is:
 *   -  Select our module.
 *   -  try/catch disconnect the router.
 *   -  connect the router.
 *   -  start the run.
 */
static void
usage(std::ostream& o)
{
  o << "Usage: \n";
  o << "    daqstop hwnode eccserver\n";
  o << "Where:\n";
  o << "   hwnode - The IP:port of the Cobo to be started e.g.\n";
  o << "            10.50.100.2:46001\n";
  o << "   eccserver - Is the node:port the ECC server for that crate.  The port\n";
  o << "            is typically 46002 e.g. 10.50.200.2:46002\n";
  exit(EXIT_FAILURE);

}



/**
 *  Entry point
 */

int
main(int argc, char** argv)
{
  // Ensure we have the right number of parameters; then stringify them:

  if (argc != 3) {
    usage(std::cerr);
  }
  std::string cobo       = argv[1];
  std::string eccServer  = argv[2];

  // Make the EccClient:

  EccClient client(eccServer);

  // Connect to our hardware

  client.connectNode(cobo);
  
  // We can stop it just fine if it was never started.

  client.ecc()->stopAsAdPeriodicPulser();
  
  // stop taking data.


  client.daqCtrl(false);     // Start run with timestamp reset.
  
  exit(EXIT_SUCCESS);
}
