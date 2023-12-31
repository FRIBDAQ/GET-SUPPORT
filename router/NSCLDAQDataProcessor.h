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

/** @file:  NSCLDAQDataprocessor.h
 *  @brief: Process frames and stick them into ring buffers.
 */
#ifndef NSCLDAQDATAPROCESSOR_H
#define NSCLDAQDATAPROCESSOR_H

#include <get/daq/FrameStorage.h>
#include <string>

#include <memory>

class CRingBuffer;

/**
 * @class NSCLDAQDataprocessor
 *     This is a GET data processor that turns frames into ring items
 *     and outputs them to a ring buffer.
 *
 *  -  Either the frame id or the frame timestamp can be set as the timestamp.
 *  -  The Source is settable from the outside world.
 *  -  We can steal frame assembly from the FrameStorage processor just
 *     by overriding processFrame as that guy knows perfectly well how to
 *     receive and assemble it into frames.
 */

class NSCLDAQDataProcessor : public get::daq::FrameStorage
{
public:
    // Canonicals
    
    NSCLDAQDataProcessor();
    virtual ~NSCLDAQDataProcessor();

    // overrides from FrameStorage:
    
    virtual void processFrame(mfm::Frame& frame);
    
    // Things that get/set local data.
    
    void setRingBufferName(const std::string& ringName);
    void setSourceId(unsigned sid);
    void useTimestamp();
    void useTriggerId();
    
    std::string getRingBufferName() const;
    unsigned    getSourceId()       const;
    bool        usingTimestamp()    const;

    // This is just done to disable the debugging log
    // message from each frame header:

    void processHeader(const mfm::PrimaryHeader& header) {}
    
    // Forbidden canonicals:
private:
    NSCLDAQDataProcessor(const NSCLDAQDataProcessor& r);
    NSCLDAQDataProcessor& operator=(const NSCLDAQDataProcessor& r);

    // Utility methods:

private:
    size_t sizeFrame(const mfm::Frame& frame);
    
    // Object data:
    
private:
    std::string   m_ringName;
    std::unique_ptr<CRingBuffer>   m_RingBuffer;
    unsigned      m_sourceId;
    bool          m_useTimestamp;
    
};

#endif
