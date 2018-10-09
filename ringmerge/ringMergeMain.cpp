/*
    This software is Copyright by the Board of Trustees of Michigan
    State University (c) Copyright 2017.

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

/** @file:  ringMergeMain.cpp
 *  @brief: Merge data from several input rings.
 */

#include "ringmerge.h"
#include <CRemoteAccess.h>
#include <CRingBuffer.h>
#include <CRingItem.h>
#include <CAllButPredicate.h>
#include <vector>
#include <stdexcept>
#include <Exception.h>
#include <string>
#include <iostream>
#include <stdlib.h>


const size_t RING_BUFFER_SIZE(32*1024*1024);

/**
 * main
 *    Parse the command line arguments.
 *    Build the input ring buffer vector.
 *    Build the output ring buffer.
 *    Process data from the input buffers when it arrives.
 */
int main(int argc, char** argv)
{
    gengetopt_args_info parsedArgs;
    cmdline_parser(argc, argv, &parsedArgs);
    std::vector<CRingBuffer*> inputRings;
    CRingBuffer*              outputRing;
    CAllButPredicate          all;         //no exceptions gives all items.
    std::string ringName;   // So we have it for exceptions.
    
    // Build up the vector of input ring buffers.
    
    try {
        if (parsedArgs.input_given > 0) {
            for (int i =0; i < input_given; i++) {
                ringName = ring_arg[i];
                inputRings.push_back(CRingAccess::daqConsumeFrom(ringName));
            }
        } else {
            throw std::string("There must be at least one input ringbuffer URI");
        }
    }
    catch (std::exception& e)  {
        std::cerr << "Unable to attach to ring as consumer " << ringName << " : "
            << e.what() << std::endl;
        exit(EXIT_FALURE);
    }
    catch (CException& e) {
        std::cerr << "Unable to attach to ring as consumer " << ringName << " :  "
            << e.ReasonText() << std::endl;
        exit(EXIT_FALURE);
    }
    catch (std::string& msg) {
        std::cerr << msg << std::endl;
        exit(EXIT_FALURE);
    }
    catch (const char* msg) {
        std::cerr << msg << std::endl;
        exit(EXIT_FALURE);
    }
    catch (...) {
        std::cerr << "Unexpected exception type processing input rings\n";
        exit(EXIT_FALURE);
    }
    // Make the output ring buffer:
    

    try {
        ringName = output_given;
        ouptputRing = CRingBuffer::createAndProduce(ringName, RING_BUFFER_SIZE);
    }
    catch (std::exception& e)  {
        std::cerr << "Unable to attach to ring as producer " << ringName << " : "
            << e.what() << std::endl;
        exit(EXIT_FALURE);
    }
    catch (CException& e) {
        std::cerr << "Unable to attach to ring as producer " << ringName << " :  "
            << e.ReasonText() << std::endl;
        exit(EXIT_FALURE);
    }
    catch (std::string& msg) {
        std::cerr << "Unable to attach to ring as producer " << ringName << " :  "
            << msg << std::endl;
        exit(EXIT_FALURE);
    }
    catch (const char* msg) {
        std::cerr << "Unable to attach to ring as producer " << ringName << " :  "
            << msg << std::endl;
        exit(EXIT_FALURE);
    }
    catch (...) {
        std::cerr << "Unexpected exception type processing output ring\n";
        exit(EXIT_FALURE);
    }

    //  Now we can forward data as we get it from the input ring buffers
    // to the output ring buffers.  The assumption is that once data is available
    // to the ring, at least one ring item is or soon will be available.
    //  Note that this is a spin loop which ain't great but gets the job done.
    
    while (true) {
        for (int i =0; i < inputRings.size(); i++) {  // Cycle over the inputs.
            if (inputRings[i]->availableData()) {     // Can get a ring item.
                CRingItem* pItem = CRingItem::getFromRing(*inputRings[i], all);
                pItem->commitToRing(*outputRing);
                delete pItem;
            }
        }
    }
    
    exit(EXIT_FAILURE);    
}