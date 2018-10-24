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

/** @file:  CGetHitUnpacker.cpp
 *  @brief: Implement unpacking of get hits for a single cobo.
 */

#include "CGetHitUnpacker.h"
#include "Parameters.h"
#include <AnalyzeFrame.h>

#include <stdint.h>
#include <iostream>


/**
 * operator()
 *    Unpack the event.  The assumption is that the pBody parameter
 *    points to a uint32_t that says how many hits there are followed
 *    by a series of NSCLGET::Hit structs.
 *
 *  @param pBody - pointer to the ring item body.
 *  @param event - unused since we use tree parameters.
 *  @param a     - unused analyzer.
 *  @param d     - unused buffer decoder.
 *  @return Bool_t kfTRUE unless the cobo isn't zero for someone.
 */
Bool_t
CGetHitUnpacker::operator()(
        const Address_t pBody, CEvent& event, CAnalyzer& a, CBufferDecoder& d
)
{
    const uint32_t* pCount = static_cast<uint32_t*>(pBody);
    uint32_t items = *pCount++;
    
    const NSCLGET::Hit*  pHit = reinterpret_cast<const NSCLGET::Hit*>(pCount);
    
    for (int i = 0; i < items; i++) {
        if (pHit->s_cobo != 0) {
            std::cerr << " Got a hit with cobo not zero: " << pHit->s_cobo << std::endl;
            return kfFALSE;
        }
        unsigned asad = pHit->s_asad;
        unsigned aget = pHit->s_aget;
        unsigned chan = pHit->s_chan;
        
        parameters.s_time[asad][aget][chan] = pHit->s_time;
        parameters.s_height[asad][aget][chan] = pHit->s_peak;
        parameters.s_integral[asad][aget][chan] = pHit->s_integral;
    
        pHit++;             // Next hits.
    }
    
    
    return kfTRUE;
}