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

/** @file:  CGetHitUnpacker.h
 *  @brief: Define an event processor that can unpack GET hit data.
 */
#ifndef CGETHITUNPACKER_H
#define CGETHITUNPACKER_H

#include <EventProcessor.h>


class CGetHitUnpacker : public CEventProcessor
{
public:
    Bool_t operator()(
        const Address_t pBody, CEvent& event, CAnalyzer& a, CBufferDecoder& d
    );
};

#endif