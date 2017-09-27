/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "FCurve.h"

// Construct by a float AtArray, as exported by the plugin (ParamsCommon.cpp)
//
// @param *in_a     the arnold float array
//
void CFCurve::Init(AtArray *in_a)
{
   m_headerSize = 3;
   m_nbKeys = (AiArrayGetNumElements(in_a) - m_headerSize) / 2;
 
   m_extrapolation = (FCurveExtrapolation)(int)AiArrayGetFlt(in_a, 0);
   m_a = in_a;
   GetTime(0, &m_startTime);
   GetTime(m_nbKeys-1, &m_endTime);
   // start != end, else the plugin would have exported a constant curve with just 1 key
   if (m_nbKeys>2)
   {
      float t1;
      GetTime(1, &t1);
      m_timeDelta = t1 - m_startTime;
   }
}


// Get the time of the index-th key
//
// @param in_index          The array index
// @param *out_result    The time result
//
// @return          false for invalid index, true elsewhere.
//
bool CFCurve::GetTime(int in_index, float *out_result)
{
   if (in_index < 0 || in_index > m_nbKeys-1)
      return false;
   *out_result = AiArrayGetFlt(m_a, m_headerSize + in_index * 2);
   return true;
}


// Get the value of the index-th key
//
// @param in_index       The array index
// @param *out_result    The value result
//
// @return          false for invalid index, true elsewhere.
//
bool CFCurve::GetValue(int in_index, float *out_result)
{
   if (in_index < 0 || in_index > m_nbKeys-1)
      return false;
   *out_result = AiArrayGetFlt(m_a, m_headerSize + in_index * 2 + 1);
   return true;
}


#define FCURVE_EPS 0.000000f
// Get the derivative at the starting point
//
// @return          the derivative.
//
float CFCurve::GetStartDerivative()
{
   return AiArrayGetFlt(m_a, 1);
   /* unused, first attempt of retriving the derivative before getting it from the xsi curve itself
   float v0, v1;
   GetValue(0, &v0);
   GetValue(1, &v1);
   if (ABS(v1 - v0) < FCURVE_EPS)
      return 0.0f; //a better way would be to use the first index whose difference with v0 is > EPS
   return (v1 - v0) / this->m_timeDelta;
   */
}


// Get the derivative at the ending point
//
// @return          the derivative.
//
float CFCurve::GetEndDerivative()
{
   return AiArrayGetFlt(m_a, 2);
   /* unused, first attempt of retriving the derivative before getting it from the xsi curve itself
   float v0, v1;
   GetValue(m_nbKeys-2, &v0);
   GetValue(m_nbKeys-1, &v1);
   if (ABS(v1 - v0) < FCURVE_EPS)
      return 0.0f; //a better way would be to use the first index whose difference with v0 is > EPS
   return (v1 - v0) / this->m_timeDelta;
   */
}


// Evaluate the curve by linear interpolation of the samples
//
// Extrapolation is also supported.
//
// @param in_time     The evaluation time
//
// @return         The fcurve value.
//
float CFCurve::Eval(float in_time)
{
   float t = 0.0f, t0 = 0.0f, v0 = 0.0f, v1 = 0.0f, result = 0.0f;

   if (m_nbKeys == 1) //constant curve
   {
      GetValue(0, &result);
      return result;
   }

   float dist = 0.0f, deriv = 0.0f, startValue = 0.0f, endValue = 0.0f;

   // manage the extrapolation
   if (in_time < m_startTime)
   {
      dist = in_time - m_startTime; // negative time distance
      GetValue(0, &startValue);
      GetValue(m_nbKeys-1, &endValue);
      switch (m_extrapolation)
      {
         case ConstantExtrapolation:
            in_time = m_startTime;
            break;
         case LinearExtrapolation:
            in_time = m_startTime;
            deriv = GetStartDerivative();
            result = deriv * dist;
            break;
         case PeriodicExtrapolation:
         case PeriodicRelativeExtrapolation:
            // not elagant but well readable
            while (in_time < m_startTime)
            {
               in_time+= (m_endTime - m_startTime);
               if (m_extrapolation == PeriodicRelativeExtrapolation)
                  result-= (endValue - startValue);
            }
            break;
      }
   }
   else if (in_time > m_endTime)
   {
      dist = in_time - m_endTime; // positive time distance
      GetValue(0, &startValue);
      GetValue(m_nbKeys-1, &endValue);
      switch (this->m_extrapolation)
      {
         case ConstantExtrapolation:
            in_time = m_endTime;
            break;
         case LinearExtrapolation:
            in_time = m_endTime;
            deriv = GetEndDerivative();
            result = deriv * dist;
            break;
         case PeriodicExtrapolation:
         case PeriodicRelativeExtrapolation:
            // not elagant but well readable
            while (in_time > m_endTime)
            {
               in_time-= (m_endTime - m_startTime);
               if (m_extrapolation == PeriodicRelativeExtrapolation)
                  result+= (endValue - startValue);
            }
            break;
      }
   }

   if (in_time == m_startTime)
   {
      GetValue(0, &v0);
      result+= v0;
      return result;
   }
   if (in_time == m_endTime)
   {
      GetValue(m_nbKeys-1, &v0);
      result+= v0;
      return result;
   }

   // linear interpolation between time and (time + m_timeDelta)
   int i0 = (int)((in_time - m_startTime) / m_timeDelta);
   // for small m_timeDelta, there could be precision problem, so...
   i0 = AiClamp(i0, 0, m_nbKeys-1);
   int i1 = i0+1;
   i1 = AiClamp(i1, 0, m_nbKeys-1);
   GetTime(i0, &t0);
   GetValue(i0, &v0);
   GetValue(i1, &v1);

   t = (in_time - t0) / m_timeDelta;
   result+= AiLerp(t, v0, v1);

   return result;
}
