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

#include "configureOpts.h"
#include "GetController.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <limits.h> /* PATH_MAX */
#include <stdlib.h>

/**
 * TBW
 */

int
main(int argc, char** argv)
{
  gengetopt_args_info args;
  cmdline_parser(argc, argv, &args);

  // Check if the hardware description file exists.
  // If it does, set the absolute path to 'hwDesc'.
  char pathBuffer[PATH_MAX];
  memset(pathBuffer, 0, PATH_MAX);
  char *realpathResult = realpath(args.hwdesc_arg, pathBuffer); 
  std::string hwDesc = "";

  std::cout << " == Connect to ECC server: " << args.ecc_arg << std::endl;
  
  if (realpathResult == NULL) {
    std::cerr << " == Unable to find the file \"" << args.hwdesc_arg << "\"!" << std::endl;

    exit(EXIT_FAILURE);
  } else {
    std::cout << " == Hardware description found: " << realpathResult << std::endl;

    hwDesc = realpathResult;
  }

  // Check if the configuration condition file exists.
  // If it does, set the absolute path to 'condition'.
  memset(pathBuffer, 0, PATH_MAX);
  realpathResult = realpath(args.cond_arg, pathBuffer); 
  std::string condition = "";

  if (realpathResult == NULL) {
    std::cerr << " == Unable to find the file \"" << args.cond_arg << "\"!" << std::endl;

    exit(EXIT_FAILURE);
  } else {
    std::cout << " == Configuration condition found: " << realpathResult << std::endl;

    condition = realpathResult;
  }

  // Parse target argument
  std::vector<std::string> coboAddresses;
  std::vector<std::string> ndrAddresses;

  for (int iTarget = 0; iTarget < args.targets_given; iTarget++) {
    std::stringstream oneTargetArgument(args.targets_arg[iTarget]);

    if (oneTargetArgument.str().find_first_of('-') == std::string::npos) {
      std::cerr << " == Wrong target address provided \"" << oneTargetArgument.str() << "\"!" << std::endl;

      exit(EXIT_FAILURE);
    }

    std::string item;
    std::getline(oneTargetArgument, item, '-');
    coboAddresses.push_back(item);
    std::getline(oneTargetArgument, item, '-');
    ndrAddresses.push_back(item);

    std::cout << " == CoBo " << iTarget << ": " << coboAddresses[iTarget];
    std::cout << " -> nscldatarouter: " << ndrAddresses[iTarget] << std::endl;
  }

  int numCobos = coboAddresses.size();


  // GetController instance created with a configuration file path
  std::cout << std::endl;
  std::cout << " ============================================" << std::endl;
  std::cout << " == Start GetController" << std::endl;
  std::cout << " ============================================" << std::endl;
  std::cout << std::endl;

  auto getController = new GetController(condition);
  getController -> setEccServerAddress(args.ecc_arg);

  // Must remove all loaded/configured nodes before do anything.
  std::cout << std::endl;
  std::cout << " ============================================" << std::endl;
  std::cout << " == Removing all configureed CoBos" << std::endl;
  std::cout << " ============================================" << std::endl;
  std::cout << std::endl;

  getController -> removeAllNodes();

  // Load the hardware first.
  for (int iCobo = 0; iCobo < numCobos; iCobo++) {
    std::cout << std::endl;
    std::cout << " ============================================" << std::endl;
    std::cout << " == Start loading hardware of CoBo " << iCobo << std::endl;
    std::cout << " ============================================" << std::endl;
    std::cout << std::endl;

    getController -> loadHw(iCobo, coboAddresses[iCobo], ndrAddresses[iCobo], hwDesc);
  }


  // Configure the boards with configuration file provided to the constructor.
  for (int iCobo = 0; iCobo < numCobos; iCobo++) {
    std::cout << std::endl;
    std::cout << " ============================================" << std::endl;
    std::cout << " == Start configuring Cobo " << iCobo << std::endl;
    std::cout << " ============================================" << std::endl;
    std::cout << std::endl;

    getController -> configure(iCobo, coboAddresses[iCobo]);
  }

  exit(EXIT_SUCCESS);
}
