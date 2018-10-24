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
#include <stdexcept>
#include <math.h>

static const int COMPRESSED_FRAME=1;          // GET data are 'compressed'.
static const int UNCOMPRESSED_FRAME=2;        // GET data are uncompressed.
static const int AGETS_PER_ASAD = 4; 
static const int CHANNELS_PER_AGET = 68;

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
private:
    void processCompressedFrame(
        mfm::Frame& frame, uint32_t nsamples, uint8_t cobo, uint8_t asad
    );
    void processUncompressedFrame(
        mfm::Frame& frame, uint32_t nsamples, uint8_t cobo, uint8_t asad
    );
    void addHitFromCompressedData(
        uint8_t cobo, uint8_t asad, unsigned asadchan ,
        const std::vector<std::pair<unsigned, unsigned>> &chsamples
    );
    double interpolatePeak(
        const std::vector<std::pair<unsigned, unsigned>>&  data,
        unsigned maxpos, double offset
    );
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

    
    // We need to know the frameType field from the cobo
    // to know how to decode.  We also need the item count.
    
    mfm::Field ftypeField = frame.headerField(5,2);
    uint16_t ftype   = ftypeField.value<uint16_t>();

    
    std::cout << "---------------- Frame --------------\n";
    std::cout << "Frame type: " << ftype
        << " Item count = " << nSamples << std::endl;

    // Figure out the cobo/ASAD numbers too:
    
    mfm::Field coboField = frame.headerField(26, 1);
    mfm::Field asadField = frame.headerField(27,1);
    uint8_t cobo    = coboField.value<uint8_t>();
    uint8_t asad    = asadField.value<uint8_t>();
    
    std::cout << "Cobo: " << (unsigned)cobo << " ASAD: " << (unsigned)asad << std::endl;
    
    if (ftype == COMPRESSED_FRAME) {
        processCompressedFrame(frame, nSamples, cobo, asad);
    } else if (ftype == UNCOMPRESSED_FRAME) {
        processUncompressedFrame(frame, nSamples, cobo, asad);
    }
    
    

}

/**
 * processCompressedFrame
 *    Process data from a compressed frame.  These frames
 *    have data in 32 bit values with bit fields:
 *
 *    *  0-11 - Sample value.
 *    *  14-22 - Sample index (bucket number).
 *    *  23-29 - Channel index.
 *    *  30-31 - AGET number.
 *
 *  @param frame - References the frame with the data.
 *  @param nSamples - Total number of samples in the frame.
 *  @param cobo     - Number of the cobo giving us data (0).
 *  @param asad     - Index of that COBO's ASAD board.
 */
void
CreateHits::processCompressedFrame(
    mfm::Frame& frame, uint32_t samples, uint8_t cobo, uint8_t asad
)
{

    // This array has a vector for each asad/channel set.
    // Contents are channel number/value pairs.
    
    std::vector<std::pair<unsigned, unsigned>> allsamples[AGETS_PER_ASAD*CHANNELS_PER_AGET];
    
    for (int i = 0; i < samples; i++) {
        mfm::Item sample = frame.itemAt(i);
        int size = sample.size_B();
        if (size != sizeof(uint32_t)) {
            std::cerr <<
                " Something seriously wrong samples should be uint32 but size was "
                << size << std::endl;
            throw std::logic_error("Frame not right");
        }
        mfm::Field fsample = sample.field(0, size);
        uint32_t fvalue = fsample.value<uint32_t>();
        
        // Break out the sample into its fields:
        
        unsigned aget =    (fvalue &  0xc0000000) >> 30;
        unsigned chan =    (fvalue &  0x3f800000) >> 23;
        unsigned bucket  = (fvalue &  0x007fc000) >> 14;
        unsigned adcval  = (fvalue &  0x00000fff);
        
        //Compute the channel/cobo value and push the bucket/value
        // back in the right vector of allsamples:
        
        unsigned abschan = aget * CHANNELS_PER_AGET + chan;
        allsamples[abschan].push_back(std::pair<unsigned, unsigned>(bucket, adcval));
    }
    // Now we can figure out a hit for each hit channel:
    
    for (unsigned i = 0; i < AGETS_PER_ASAD*CHANNELS_PER_AGET; i++) {
        if (!allsamples[i].empty()) {
            std::vector<std::pair<unsigned, unsigned>>& chsamples(allsamples[i]);
            addHitFromCompressedData(cobo, asad, i, chsamples);    
        }
    }
}
/**
 * processUncompressedFrame
 *
 *    The data themselves are just 16 bit integers.  Each value is bit
 *    encoded as follows:
 *
 *    * 0-11   The sample value.
 *    * 12-15  AGET index.
 *   
 *   The sample order is such that we get samples from all of the
 *   channels of each of the 68 channels in an AGET before
 *   we increment the sample number.
 *
 * @param frame  - References the frame we're unpacking.
 * @param samples - Total number of samples we have.
 * @param cobo    - Index of the cobo providing data.
 * @param asad    - index of the ASAD board providing data.
 */
void
CreateHits::processUncompressedFrame(
    mfm::Frame& frame, uint32_t samples, uint8_t cobo, uint8_t asad
)
{
    // accumulate the traces in this array of vectors.
    // .first is the bin number and .second the trace height.
    // this is done even here so we can use addHitFromCompressedData
    // as we did with compressed data.
    
    std::vector<std::pair<unsigned, unsigned>> allsamples[AGETS_PER_ASAD*CHANNELS_PER_AGET];
    unsigned agetChan[AGETS_PER_ASAD];
    for (int i =0; i < AGETS_PER_ASAD; i++) {
        agetChan[i] = 0;
    }
    
    
    // Inner loop is over all channels in an aget, collect the waveforms
    
    int itemIndex = 0;

    while (samples) {
    
        mfm::Item sampleItem = frame.itemAt(itemIndex);
        uint64_t itemSize    = sampleItem.size_B();
        mfm::Field sampleField= sampleItem.field(0, itemSize);
        uint16_t item         = sampleField.value<uint16_t>();
        
        // Break the item up into AGET number and sample value:
        
        unsigned aget = (item & 0xc000) >> 14;
        unsigned value = item & 0xfff;
        
      
        // agetChan is the channel inside this aget to use:
        
        unsigned chan = agetChan[aget];
        agetChan[aget] = (agetChan[aget] + 1) % CHANNELS_PER_AGET;
        
        //Figure out the global channel and store the hit.  The bin number
        // will just be the index number in the vector as this is unsuppressed.
        
        unsigned asadchan = aget * CHANNELS_PER_AGET + chan;
        unsigned bin = allsamples[asadchan].size();
        allsamples[asadchan].push_back(std::pair<unsigned, unsigned>(bin, value));
        
        itemIndex++;
        samples--;
    }
    // Now add the hits.
    
    for (unsigned i = 0; i < CHANNELS_PER_AGET*AGETS_PER_ASAD; i++) {
        std::vector<std::pair<unsigned, unsigned>>& c(allsamples[i]);
        if (c.size()) {
            addHitFromCompressedData(cobo, asad, i, c);
        }
    }
}
/**
 * addHitFromCompressedData
 *    Given a vector of sample bucket numbers and values,
 *    -  Estimates the offset from the min of both ends of the data.
 *    -  computes the bucket at which the centroid lives.
 *    -  computes the integral of the data.
 *    -  Using a three point parabolic interpolation around the
 *       peak (if possible), figures out the peak height.
 *    -  Creates a hit struct and adds that to our object's m_results vector.
 *
 * @param cobo - the cobo index from which the data come.
 * @param asad - the asad board within the cobo.
 * @param asadchan - the channel within the asad
 * @param chsamples - the data for that channel.
 */
void
CreateHits::addHitFromCompressedData(
    uint8_t cobo, uint8_t asad, unsigned asadchan,
    const std::vector<std::pair<unsigned, unsigned>> &chsamples
)
{
    // Reconstruct the asad# and the channel inside it.
    
    unsigned aget = asadchan / CHANNELS_PER_AGET;
    unsigned chan = asadchan % CHANNELS_PER_AGET;
    
    // figure out the offset from the min of left most and right most
    // The assumption is that between these is a nice peak:
    
    size_t nSamples = chsamples.size();
    double left = chsamples[0].second;
    double right = chsamples.back().second;
    double offset = fmin(left, right);
    
    // Figure out where the centroid is
    // and the integral of the data:
    
    double sum = 0.0;
    double wsum = 0.0;

    double maxval = 0.0;
    unsigned maxpos = 0;
    
    for (unsigned i = 0; i < nSamples; i++) {
        double bin = chsamples[i].first;
        double ht  = chsamples[i].second - offset;
        
        sum += ht;
        wsum += bin*ht;
        
        if (ht > maxval) {
            maxval = ht;
            maxpos = i;
        }
    }
    double centroid = wsum/sum;
    double peak     = interpolatePeak(chsamples, maxpos, offset);
    
    //Make and save the hit:
    
    NSCLGET::Hit h;
    h.s_cobo = cobo;
    h.s_asad = asad;
    h.s_aget = aget;
    h.s_chan = chan;
    h.s_time = centroid;
    h.s_peak = peak;
    h.s_integral = sum;
    
    m_results.push_back(h);
}
/**
 * interpolatePeak
 *    If we have sufficient points, we do a 3 point parabolic interpolation
 *    to get a 'high quality' peak value.  If not we just take the
 *    value of the data at the integerized centroid.
 *    To interpolate we need a point before and after the index of the max value.
 *
 * @param data - the data array of bin number/value pairs.
 * @param maxpos - the _index_ (not bucket) of the maximum value.
 * @param offset - estimated baseline.
 * @return double - peak height computed as described above.
 */
double
CreateHits::interpolatePeak(
        const std::vector<std::pair<unsigned, unsigned>>&  data,
        unsigned maxpos, double offset
)
{
    double peak = data[maxpos].second  - offset;
    
    // Do we have enough points to do the interpolation and peak solve?
    
    if ((maxpos > 0) && ((maxpos + 1)< data.size())) {
        // can interpolate.
        
        // Get the three points in the parabola:
        
        double x1 = data[maxpos-1].first;
        double y1 = data[maxpos-1].second - offset;
        
        double x2 = data[maxpos].first;
        double y2 = data[maxpos].second - offset;
        
        double x3 = data[maxpos+1].first;
        double y3 = data[maxpos+1].second - offset;
        
        // See http://fourier.eng.hmc.edu/e176/lectures/NM/node25.html for
        // the derivation of below.  Note that if the denominator
        // of this messy thing is zero, then again we just default
        // to the maxval in the data:
        // Note this is just a specific case of Lagrange interpolation
        // where we use L2 as the interpolating polynomial.
        //
        double denom = (y1 - y2)*(x3-x2) + (y3-y2)*(x2-x1);
        if (denom != 0.0) {            // Don't think this can happen but..
            double num = (y1-y2)*(x3-x2)*(x3-x2) - (y3-y2)*(x2-x1)*(x2-x1);
            
            double peakpos = num/(2*denom) + x2;
            
            // Now plug this back inot the L2 polynomial:
            // Again protecting against division by zero:
            
            double d1 = (x1-x2)*(x1-x3);
            double d2 = (x2-x3)*(x2-x1);
            double d3 = (x3-x1)*(x3-x2);
            if (d1*d2*d3 != 0) {
                // No denominators are zero:
                
                double n1 = (peakpos - x2)*(peakpos - x3);
                double n2 = (peakpos - x3)*(peakpos - x1);
                double n3 = (peakpos - x1)*(peakpos - x2);
                
                peak = y1*n1/d1 + y2*n2/d2 + y3*n3/d3;
            }
            
        }
        
    }
    
    return peak;
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