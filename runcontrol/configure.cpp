/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2014-2022.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Authors:
             Genie Jhang
	     Facility for Rare Isotope Beams
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/**
 * @file configure.cpp
 * @brief Loading/configuring multiple CoBos with provided configuration files
 * @author Genie Jhang <changj@frib.msu.edu>
 */

#include "GetController.h"
#include <iostream>
#include <string>
#include <stdlib.h>

/**
 * TBW
 */

int
main(int argc, char** argv)
{
  // To be rewritten using gengetopt
  // 1. File path can be either relative or absolute
  // 2. CoBo addresses and datarouter addresses will be at last 
  //    so that use can add as many boards as they want
  std::string endpoints[2] = {"10.65.30.51:46001", "10.65.30.52:46001"};
  std::string drEndpoints[2] = {"10.65.31.154:46005", "10.65.31.154:46055"};
  std::string hwDesc = "/user/0400x/s800_get_test_101322/hardwareDescription_fullCoBoStandAlone.xcfg";
  std::string cfgFile = "/user/0400x/s800_get_test_101322/configure-internalPulser-2cobos1asad.xcfg";

  int numCobos = atoi(argv[1]);

  // GetController instance created with a configuration file path
  auto getController = new GetController(cfgFile);

  // Must remove all loaded/configured nodes before do anything.
  getController -> removeAllNodes();

  // Load the hardware first.
  for (int iCobo = 0; iCobo < numCobos; iCobo++) {
    getController -> loadHw(iCobo, endpoints[iCobo], drEndpoints[iCobo], hwDesc);
  }

  // Configure the boards with configuration file provided to the constructor.
  for (int iCobo = 0; iCobo < numCobos; iCobo++) {
    getController -> configure(iCobo, endpoints[iCobo]);
  }

  exit(EXIT_SUCCESS);
}
