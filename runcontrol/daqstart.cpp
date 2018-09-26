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
 * @file daqstart.cpp
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
 *    daqstart hwnode eccserver routerdata
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

/**
 *  Entry point
 */

int
main(int argc, char** argv)
{
  // Ensure we have the right number of parameters; then stringify them:

  if (argc != 4) {
    usage(std::cerr);
  }
  std::string cobo       = argv[1];
  std::string eccServer  = argv[2];
  std::string dataRouter = argv[3];

  // Make the EccClient:

  EccClient client(eccServr, "BeginRun");

  // Connect to our hardware

  client.ecc().selectNodeById(cobo);
  
  // Disconnect/connect the router.

  try {
    client.daqDisconnect();
  }
  catch (...) {}                   // assume errors mean already disconnected.

  client.daqConnect(dataRouter, std::string("TCP"));
  
  // start taking data.

  client.daqctrl(true, true);     // Start run with timestamp reset.

  exit(EXIT_SUCCESS);
}
