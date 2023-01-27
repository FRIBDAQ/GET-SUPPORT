/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2014-2022.

    You may use this software under the terms of the GNU public license
    (GPL).  The terms of this license are described at:

     http://www.gnu.org/licenses/gpl.txt

     Authors:
             Ron Fox
             Jeromy Tompkins 
             Genie Jhang
	     Facility for Rare Isotope Beams
	     Michigan State University
	     East Lansing, MI 48824-1321
*/

/**
 * @file daqcontrol.cpp
 * @brief Start/stop a run in the GET electronics.
 *        Supports Multiple CoBos.
 * @author Genie Jhang <changj@frib.msu.edu>
 */

#include "daqcontrolOpts.h"
#include "EccClient.h"

#include <iostream>
#include <string>
#include <stdlib.h>

int
main(int argc, char** argv)
{
  if (argc == 1) {
    cmdline_parser_print_help();

    exit(EXIT_FAILURE);
  }

  gengetopt_args_info args;
  cmdline_parser(argc, argv, &args);

  bool isStart = false;

  if (!args.command_given) {
    std::cerr << " == Control command not provided! Doing nothing!" << std::endl;

    exit(EXIT_FAILURE);
  } else {
    for (int index = 0; index < strlen(args.command_arg); index++) {
        args.command_arg[index] = tolower(args.command_arg[index]);
    }

    if (strcmp(args.command_arg, "start") == 0) {
      isStart = true;
    } else if (strcmp(args.command_arg, "stop") == 0) {
    } else {
      std::cerr << " == Wrong control command provided!: \"" << args.command_arg << "\"" << std::endl;

      exit(EXIT_FAILURE);
    }
  }

  if (args.targets_given == 0) {
    std::cerr << " == No CoBo address specified provided! Doing nothing!" << std::endl;

    exit(EXIT_FAILURE);
  }

  bool isPulsing = strcmp(args.pulser_arg, "on") == 0;

  // Make the EccClient:
  std::cout << " == Connect to ECC server: " << args.ecc_arg << std::endl;
  std::string eccServerAddress = args.ecc_arg;
  EccClient client(eccServerAddress);

  for (int iTarget = 0; iTarget < args.targets_given; iTarget++) {
    std::string targetAddress = args.targets_arg[iTarget];

    // Connect to our hardware
    client.connectNode(targetAddress);

    for (int index = 0; index < strlen(args.pulser_arg); index++) {
        args.pulser_arg[index] = tolower(args.pulser_arg[index]);
    }

    if (isPulsing && isStart) {
      client.ecc() -> startAsAdPeriodicPulser(); // Start the asad pulser
    } else {
      // We can stop it just fine if it was never started.
      client.ecc() -> stopAsAdPeriodicPulser();
    }

    if (isStart) {
      std::cout << std::endl;
      std::cout << " ============================================" << std::endl;
      std::cout << " == CoBo " << iTarget << ": " << targetAddress << std::endl;
      std::cout << "    Start DAQ" << (isPulsing ? " with pulsing" : "") << std::endl;
      std::cout << " ============================================" << std::endl;
      std::cout << std::endl;

      // Start run with timestamp reset.
      client.daqCtrl(true, true);
    } else {
      std::cout << std::endl;
      std::cout << " ============================================" << std::endl;
      std::cout << " == CoBo " << iTarget << ": " << targetAddress << std::endl;
      std::cout << "    Stop DAQ and pulsing if started" << std::endl;
      std::cout << " ============================================" << std::endl;
      std::cout << std::endl;

      // stop taking data.
      client.daqCtrl(false);
    }
  }

  client.destroy();
  
  exit(EXIT_SUCCESS);
}
