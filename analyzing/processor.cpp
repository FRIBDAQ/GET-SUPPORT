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

/** @file:  processor.cpp
 *  @brief: Implements the CRingItemProcessor class.
 */

// See the header; processor.h for more information about the class
// as a whole.

#include "processor.h"
#include "AnalyzeFrame.h"

// NSCLDAQ includes:

#include <CRingItem.h>
#include <CRingScalerItem.h>
#include <CRingTextItem.h>
#include <CRingStateChangeItem.h>
#include <CPhysicsEventItem.h>
#include <CRingPhysicsEventCountItem.h>
#include <CDataFormatItem.h>
#include <CGlomParameters.h>


// Standard library headers:

#include <iostream>
#include <map>
#include <string>

#include <ctime>

// Map of timestamp policy codes to strings:

static std::map<CGlomParameters::TimestampPolicy, std::string> glomPolicyMap = {
    {CGlomParameters::first, "first"},
    {CGlomParameters::last, "last"},
    {CGlomParameters::average, "average"}
};



/**
 * processScalerItem
 *    Output an abbreviated scaler dump.
 *    Your own processing should create a new class and override
 *    this if you want to process scalers.
 *
 *    Note that this and all ring item types have a toString() method
 *    that returns the string the NSCL DAQ Dumper outputs for each item type.
 *
 * @param scaler - Reference to the scaler ring item to process.
 */
void
CRingItemProcessor::processScalerItem(CRingScalerItem& item)
{
}
/**
 * processStateChangeItem.
 *    Processes a run state change item.  Again we're just going to
 *    do a partial dump:
 *    BEGIN/END run we'll give the timestamp, run number and title, and time
 *        offset into the run.
 *    PAUSE_RESUME - we'll just give the time and time into the run.
 *
 * @param item  - Reference to the state change item.
 */
void
CRingItemProcessor::processStateChangeItem(CRingStateChangeItem& item)
{
    time_t tm = item.getTimestamp();
    std::cout << item.typeName() << " item recorded for run "
        << item.getRunNumber() << std::endl;
    std::cout << "Title: " << item.getTitle() << std::endl;
    std::cout << "Occured at: " << std::ctime(&tm)
        << " " << item.getElapsedTime() << " sec. into the run\n";
}
/**
 * processTextItem
 *    Text items are items that contain documentation information in the
 *    form of strings.  The currently defined text items are:
 *    -   PACKET_TYPE - which contain documentation of any data packets
 *                      that might be present.  This is used by the SBS readout
 *                      framework.
 *    -  MONITORED_VARIABLES - used by all frameworks to give the values of tcl
 *                      variables that are being injected during the run or are
 *                      constant throughout the run.
 *
 *   Again we format a dump of the item.
 *
 * @param item - refereinces the CRingTextItem we got.
 */
void
CRingItemProcessor::processTextItem(CRingTextItem& item)
{
}
/**
 * processEvent
 *    Process physics events.  For now we'll just output a dump of the event.
 *
 *  @param item - references the physics event item that we are 'analyzing'.
 */
void
CRingItemProcessor::processEvent(CPhysicsEventItem& item)
{
    std::vector<NSCLGET::Hit> hits =
        NSCLGET::analyzeFrame(item.getBodySize(), item.getBodyPointer());
}
/**
 * processEventCount
 *    Event count items are used to describe, for a given data source,
 *    The number of triggers that occured since the last instance of that
 *    item.  This can be used both to determine the rough event rate as well
 *    as the fraction of data analyzed (more complicated for built events) in a
 *    program sampling physics events.  We'll dump out information about the
 *    item.
 *
 *  @param - references the CPhysicsEventCountItem being dumped.
 */
void
CRingItemProcessor::processEventCount(CRingPhysicsEventCountItem& item)
{
}
/**
 * processFormat
 *    11.x runs have, as their first record an event format record that
 *    indicates that the data format is for 11.0.
 *
 * @param item - references the format item.
 */
void
CRingItemProcessor::processFormat(CDataFormatItem& item)
{
}
/**
 * processGlomParams
 *     When the data source is the output of an event building pipeline,
 *     the glom stage of that pipeline will insert a glom parameters record
 *     into the output data.  This record type will indicate whether or not
 *     glom is building events (or acting in passthrough mode) and the
 *     coincidence interval in clock ticks used when in build mode, as well
 *     as how the timestamp is computed from the fragments that make up each
 *     event.
 *
 *  @param item -  References the glom parameter record.
 */
void
CRingItemProcessor::processGlomParams(CGlomParameters& item)
{
}
/**
 * processUnknownItemType
 *    Process a ring item with an  unknown item type.   This can happen if
 *    we're seeing a ring item that we've not specified a handler for (unlikely)
 *    or the item types have expanded but the data format is the same (possible),
 *    or the user has defined and is using their own ring item type.
 *    We'll just dump the item.
 *
 * @param item - CRingItem reference for the event.
 */
void
CRingItemProcessor::processUnknownItemType(CRingItem& item)
{
}