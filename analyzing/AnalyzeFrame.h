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

/** @file:  AnalyzeFrame.h
 *  @brief: Header to provide a function for analyzing a frame.
 */

#ifndef ANALYZEFRAME_H
#define ANALYZEFRAME_H

#include <vector>
#include <stddef.h>

namespace NSCLGET {

/*
 * This structure defines what we extract for each hit.  A hit is computed
 * from the data that are returned from a channel that is not suppressed.
 */

typedef struct _hit {
    
    // Channel identification.
    
    unsigned s_cobo;              // Cobo index (0 usually).
    unsigned s_asad;              // ASAD index (board within cobo).
    unsigned s_aget;              // AGET chip within an ASAD.
    unsigned s_chan;              // Channel number within an AGET.
    
    // Extracted data:
    
    double  s_time;               // Centroid of the pulse in time.
    double  s_peak;               // Height of peak (3 point parabolic interpolation).
    double  s_integral;           // Integral of the trace.
    
} Hit, *pHit;

/**
 * Hand this method a pointer to the frame and it'll analyze all traces in the
 * frame:
 */

std::vector<Hit> analyzeFrame(size_t  bodySize, const void* pFrame);
    
};

#endif