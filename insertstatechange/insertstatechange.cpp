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

/** @file: insertstatechange.cpp
 *  @brief: insert state change items to ring buffers.
 */

#include "statechange.h"
#include <CRingBuffer.h>
#include <CRingStateChangeItem.h>
#include <iostream>
#include <stdlib.h>

#include <Exception.h>
#include <stdexcept>
#include <string>
#include <time.h>

#include <memory>
#include <fragment.h>

int
main(int argc, char** argv)
{
    gengetopt_args_info parsedArgs;
    cmdline_parser(argc, argv, &parsedArgs);
    
    // Figure out the type of state change from type_arg:
    
    uint32_t itemType;
    uint32_t barrierType;
    if (std::string("begin") == parsedArgs.type_arg) {
        itemType = BEGIN_RUN;
	barrierType = 1;
    } else if (std::string("end") == parsedArgs.type_arg) {
        itemType = END_RUN;
	barrierType = 2;
    } else {
        std::cerr << "Invalid state change --type '" << parsedArgs.type_arg
            << "' must be 'begin' or 'end'\n";
        exit(EXIT_FAILURE);
    }
    
    // Pull out all the other stuff:
    
    std::string ringName = parsedArgs.ring_arg;
    std::string title    = parsedArgs.title_arg;
    uint32_t    runNum   = parsedArgs.run_arg;
    uint32_t    sourceId = parsedArgs.source_id_arg;
    
    uint32_t offset;
    if (itemType == BEGIN_RUN) {
        offset = 0;
        if (parsedArgs.offset_given) {
            std::cerr << "--offset is only legal for end run item types\n";
            exit(EXIT_FAILURE);
        }
    } else {
        offset   = parsedArgs.offset_arg;        
    }
    
    
    // The absolute timestamp is now:
    
    time_t now = time(nullptr);
    
    
    // Connect to the ring buffer, create the item and commit it to the ring.
    
    try {
        std::unique_ptr<CRingBuffer>
            ring(new CRingBuffer(ringName, CRingBuffer::producer));
        CRingStateChangeItem item(
            NULL_TIMESTAMP, sourceId, barrierType, //
            itemType, runNum, offset, now, title
        );
        item.commitToRing(*ring);
    }
    catch (CException& e) {
        
    }
    catch (std::exception& e) {
        
    }
    catch (const char* msg) {
        
    }
    catch (std::string msg) {
        
    }
    catch (...) {
        
    }
    
    
    exit(EXIT_SUCCESS);
}
