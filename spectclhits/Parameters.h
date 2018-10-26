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

/** @file:  Parameters.h
 *  @brief: Define the tree parameters for the program.
 */
#ifndef PARAMETERS_H
#define PARAMETERS_H
#include <TreeParameter.h>



// A Cobo has 4 Asads, each with 4 AGETS, each with 68 channels.
// each channel has a time, height and an integral:

typedef struct _CoBo {
    CTreeParameterArray  s_time[4][4];
    CTreeParameterArray  s_height[4][4];
    CTreeParameterArray  s_integral[4][4];
} CoBo, *pCoBo;




extern CoBo parameters;

#endif