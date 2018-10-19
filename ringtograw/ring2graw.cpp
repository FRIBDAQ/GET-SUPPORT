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

#include <iostream>
#include <stdlib.h>
#include <CDataSource.h>
#include <CDataSourceFactory.h>
#include <CRingItem.h>
#include <Exception.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <DataFormat.h>

/** @file: ring2graw.cpp
 *  @brief: Filter ring items -> graw output.
 */

static void
usage(std::ostream& o, std::string msg)
{
    o << msg << std::endl;
    o << "Usage\n";
    o << "   ring2graw data-source-url\n";
    o << "Where\n";
    o << "  data-source-url - URI from which ring item data is taken\n";
    exit(EXIT_FAILURE);
}


int main(int argc, char** argv)
{
    if (argc != 2) {
        usage(std::cerr, "Incorrect number of command parameters");
    }
    std::string sourceUrl = argv[1];
    
    // Create the data source from it's uri:
    
    CDataSource* pSource;
    try {
        std::vector<uint16_t> empty;
        pSource = CDataSourceFactory::makeSource(sourceUrl, empty, empty);
    }
    catch (CException& e) {
        usage(std::cerr, e.ReasonText());
    }
    catch (...) {
        usage(std::cerr, "Exception caught making the data source");
    }
    
    // Process the ring items:
    
    
    CRingItem* pItem;
    while(pItem = pSource->getItem()) {
      if (pItem->type() == PHYSICS_EVENT) {
        void* pData  = pItem->getBodyPointer();
        size_t nByte = pItem->getBodySize();
        std::cout.write(static_cast<char*>(pData), nByte);
      }
      delete pItem;
    }
    
    exit(EXIT_SUCCESS);
}
