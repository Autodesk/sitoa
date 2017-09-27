/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <ai.h>

// Copied from xsi_decl.h
typedef enum FCurveExtrapolation
{	
   ConstantExtrapolation	= 1,		// Constant extrapolation */
   LinearExtrapolation	= 2,		// Linear extrapolation */
   PeriodicExtrapolation	= 3,		// Periodic extrapolation */
   PeriodicRelativeExtrapolation	= 4	// Constant extrapolation relative to an offset */
} 	FCurveExtrapolation;

// FCurve class.
//
// Initialized by a float AtArray, as exported by the plugin (ParamsCommon.cpp)
// Format of the array: 
// array[0] == extrapolation type, array[1] = derivative at curve start, array[2] = derivative at curve end
// array[3+i*2] == time of i-th sample, array[3+i*2+1] == value of i-th sample
//
// @param m_nbKeys       number of time/value couples
// @param m_headerSize   number of extra data in the array (3 atm)
// @param *m_a           arrays of time/value couples, plus the initial header. So, full size is m_headerSize + m_nbKeys*2
// @param m_startTime    time of the first key
// @param m_endTime      time of the last key
// @param m_timeDelta    time delta between keys
//
class CFCurve
{
private:
   FCurveExtrapolation m_extrapolation;
   int m_nbKeys;
   int m_headerSize;
   AtArray *m_a;
   float m_startTime, m_endTime;
   float m_timeDelta;

   // Get the time of the index-th key
   bool GetTime(int index, float *result);
   // Get the value of the index-th key
   bool GetValue(int index, float *result);
   // Get the derivative at the starting point
   float GetStartDerivative();
   // Get the derivative at the ending point
   float GetEndDerivative();

public:
   CFCurve()
   {
      m_nbKeys = 0;
      m_a = NULL;
   }

   void Init(AtArray *in_a);

   ~CFCurve()
   {}

   // Evaluate at time
   float Eval(float in_time);
};
