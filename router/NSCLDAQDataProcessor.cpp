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

/** @file:  NSCLDAQDataProcessor.cpp
 *  @brief: Implements the NSCLDAQ ringbuffer data processor.
 */
#include "NSCLDAQDataProcessor.h"
#include <CRingBuffer.h>
#include <CPhysicsEventItem.h>
#include <stdint.h>
#include <string.h>
#include <mfm/Frame.h>
#include <mfm/Field.h>
#include <mfm/Header.h>
#include <mfm/Common.h>


const size_t RING_BUFFER_SIZE(32*1024*1024);

/**
 *  Constructor
 *     Initialize our data and construct the base class:
 */

NSCLDAQDataProcessor::NSCLDAQDataProcessor() : m_sourceId(0), m_useTimestamp(false),
    FrameStorage()
{}

/**
 * destructor
 *    Using a unique_ptr ensures the ring buffer is destroyed.
 *    All other member data either are well behaved with respect to object
 *    destruction (std::string) or primitive types.
 *
 */
NSCLDAQDataProcessor::~NSCLDAQDataProcessor()
{}

/**
 * processFrame
 *    This is where the action is.  The frame data are wrapped in a ring item
 *    and the body headers of that ring item set appropriately.
 *    The resulting ring item (a PHYSICS_EVENT) is submitted to the ring buffer
 *    which must have been set by now else bad things happen.
 *    
 *  @param frame - reference to the frame data.
 */
void
NSCLDAQDataProcessor::processFrame(mfm::Frame& frame)
{
    // If no ringbuffer has been set, for now ignore the frame
    
    if (m_RingBuffer.get() == nullptr) {
        return;
    }
    
    // To turn the frame into a ring item we need to know:
    //  The value we'll put in the timestamp, and the frame size.
    
    uint64_t timestamp;                    // Could be trigger id too.
    size_t   itempos;
    size_t   itemsize;
    if (m_useTimestamp) {
        frame.findHeaderField(std::string("eventTime"), itempos, itemsize);
    } else {
        frame.findHeaderField(std::string("eventIdx"),  itempos, itemsize);
    }
    mfm::Field f = frame.headerField(itempos, itemsize);
    timestamp = f.value<uint64_t>();
    
    size_t      nbytes= sizeFrame(frame);
    const mfm::Byte* pData = frame.data();
    
    // Now we have enough to build the physics item + 1024 is overly big body
    // header size.  I suppose we could include DataFormat.h and use the actual
    // size but I'm lazy today.
    
    CPhysicsEventItem item(timestamp, m_sourceId, 0, nbytes + 1024);
    void* dest = item.getBodyCursor();
    memcpy(dest, pData, nbytes);          // Frame -> ring item.
    dest = static_cast<void*>(static_cast<uint8_t*>(dest) + nbytes);
    item.setBodyCursor(dest);
    item.updateSize();                      // Not sure I need this but...
    
    CRingBuffer* pR = m_RingBuffer.get();
    item.commitToRing(*pR);
}
/**
 *   setRingBufferName
 *      If necessary creates the ringbuffer and then becomes the producer.
 *      We're going to make the ring buffer bigger than usual (see RING_BUFFER_SIZE
 *      const).  If the ring buffer is successfully created and we are
 *      successfully the producer, m_ringName is updated.  If not, all is
 *      as before excetp an exception is thrown.
 *      if m_RingBuffer is already set the CRingBuffer pointed to by it is
 *      destroyed (the object not the ring), so someone else can produce into it.
 */
void
NSCLDAQDataProcessor::setRingBufferName(const std::string& ringName)
{

    CRingBuffer* pNewRing = CRingBuffer::createAndProduce(ringName,  RING_BUFFER_SIZE);
    m_ringName = ringName;
    m_RingBuffer.reset(pNewRing);
   
}

/**
 * setSourceId
 *    Set a new sourceid.  From now on, this sourceid will be used for the
 *    body header of ringItems created.
 *
 * @param sid - new source id.
 */
void
NSCLDAQDataProcessor::setSourceId(unsigned sid)
{
    m_sourceId = sid;
}

/**
 * useTimestamp
 *    from now on the timestamp field of the frame will be the timestamp for the
 *    body header.
 */
void
NSCLDAQDataProcessor::useTimestamp()
{
    m_useTimestamp = true;
}
/**
 *  useTriggerId
 *    From now on the trigger id field of the frame will be used as the timestamp.
 */
void
NSCLDAQDataProcessor::useTriggerId()
{
    m_useTimestamp= false;
}

/**
 * getRingBufferName
 *    Return the name of the current ring buffer.
 *  @return std::string
 *  @retval - empty string means there's no current ring buffer.
 */
std::string
NSCLDAQDataProcessor::getRingBufferName() const
{
    return m_ringName;
}
/**
 * getSourceId
 *    Return the current source id.
 * @return unsigned
 */
unsigned
NSCLDAQDataProcessor::getSourceId() const
{
    return m_sourceId;
}
/**
 * usingTimestamp
 *
 * @return bool -if true the timestamp is used in the body header, if false,
 *               the trigger id is used instead.
 */
bool
NSCLDAQDataProcessor::usingTimestamp() const
{
    return m_useTimestamp;
}
/*-----------------------------------------------------------------------------
 *  Private utilities.
 */
/**
 * sizeFrame
 *    If I understand the Frame class correctly,   What we need to do
 *    is get a const reference to the header (supported by Frame).  Ask the
 *    header to tell us its size and the size of the frame body that follows.
 *
 *  @param frame - const reference to the frame we're dissecting.
 *  @return size_t - size of the frame + header all in one.  I hope this
 *                   includes the meta type field. else we'll miss a byte.
 */
size_t
NSCLDAQDataProcessor::sizeFrame(const mfm::Frame& frame)
{
    mfm::Header const & h = frame.header();
    size_t result = h.headerSize_B() + h.dataSize_B();
    
    return result;
}