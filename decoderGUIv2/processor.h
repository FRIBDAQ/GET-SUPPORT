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

/** @file:  processor.h
 *  @brief: Defines a ring item processing class.
 */

#ifndef PROCESSOR_H
#define PROCESSOR_H

/**
 * This file defines a class that accepts specific ring item types.
 * See process.cpp in this directory to see how to feed this beast.
 */

// Forward class type definitions:

class CRingScalerItem;
class CRingStateChangeItem;
class CRingTextItem;
class CPhysicsEventItem;
class CRingPhysicsEventCountItem;
class CDataFormatItem;
class CGlomParameters;
class CRingItem;

/**
 * The concept of this class is really simple.  A virtual method for each
 * ring item type that we differentiate between.  Similarly a virtual
 * method for ring item types that we don't break out.
 */

class CRingItemProcessor
{
public:
    virtual void processScalerItem(CRingScalerItem& item);
    virtual void processStateChangeItem(CRingStateChangeItem& item);
    virtual void processTextItem(CRingTextItem& item);
    virtual void processEvent(CPhysicsEventItem& item);
    virtual void processEventCount(CRingPhysicsEventCountItem& item);
    virtual void processFormat(CDataFormatItem& item);
    virtual void processGlomParams(CGlomParameters& item);
    virtual void processUnknownItemType(CRingItem& item);
};



#endif
