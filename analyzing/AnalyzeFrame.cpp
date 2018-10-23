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

/** @file:  AnalyzeFrame.cpp
 *  @brief: Implement code to analyze  frame.  See AnalyzeFrame.h
 */


#include "AnalyzeFrame.h"

// GET Includes:

#include <mfm/FrameBuilder.h>
#include <mfm/Frame.h>
#include <mfm/Field.h>


// System includes.

#include <iostream>

/**
 * The way GET analysis works we need to make a frame-builder class.
 * It's processFrame method is going to do the actual frame analysis
 * and squirrel the results away where we can get it from the outside.
 * We do it this way because we don't know if the frame lives after
 * processFrame is done.
 */
class CreateHits : public mfm::FrameBuilder
{
private:
    std::vector<NSCLGET::Hit> m_results;

public:
    std::vector<NSCLGET::Hit> getResults() const {
        return m_results;
    }
protected:
    virtual void processFrame(mfm::Frame& frame);
    
};


/**
 * processFrame
 *    Reset the hits and compute new ones.
 *
 * @#param mfm::Frame - reference to the frame.
 */
void
CreateHits::processFrame(mfm::Frame& frame)
{
    m_results.clear();
    uint32_t nSamples = frame.itemCount();

    
    std::cout << " Item count = " << nSamples << std::endl;
    
    // Processing the item:
    
    for (int i = 0; i < nSamples; i++) {
        mfm::Item sample = frame.itemAt(i);
        std::cout << " Item " << i << " Is " << sample.size_B() <<
            " bytes big\n";
        mfm::Field fsample = sample.field(0, 4);
        uint32_t fvalue = fsample.value<uint32_t>();
        std::cout << " Value: " << std::hex << fvalue << std::dec << std::endl;
    }
}

/////////////////// Public entry ///////////////////////////////////

/**
 * analyzeFrame
 *    Returns the analyzed hits for each frame of data.
 *
 * @param[in] bodySize - number of bytes in the body.
 * @param[in] pFrame pointer to the frame (ring item body)
 * @return std::vector<Hit> - vector of unpacked hit information.
 */
std::vector<NSCLGET::Hit>
NSCLGET::analyzeFrame(size_t bodySize, const void* pFrame)
{
    
    CreateHits hitMaker;
    
    // Add the frame as a chunk.
    
    const mfm::Byte* begin = static_cast<const mfm::Byte*>(pFrame);
    const mfm::Byte* end   = begin + bodySize;
    hitMaker.addDataChunk(begin, end);
    
    // Since we have a complete frame, process frame should have been called.
    
    return hitMaker.getResults();
}