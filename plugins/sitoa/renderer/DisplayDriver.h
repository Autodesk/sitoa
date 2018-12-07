/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_renderercontext.h>

using namespace XSI;


// This class to manage the rendering overscan vs the Softimage render view
// 
class CDisplayDriverData
{
private:
   bool    m_overscan;       // is overscan enabled ?
   AtBBox2 m_display_window; // the render view window, without overscan
   AtBBox2 m_data_window;    // can be negative if overscan enabled

public:
   CDisplayDriverData() : m_overscan(false),
                          m_progressivePasses(1)
   {}

   // Initialize the driver data
   void Init(const AtBBox2 &in_display_window, const AtBBox2 & in_data_window);
   // Check if a bucket is completely outside the render view, ie all inside the overscan frame
   bool IsBucketOutsideView(const int in_bucket_xo, const int in_bucket_yo, 
                            const int in_bucket_size_x, const int in_bucket_size_y);
   // Check if a bucket is completely inside the render view, ie with no intersection with the overscan frame
   bool IsBucketInsideView(const int in_bucket_xo, const int in_bucket_yo, 
                           const int in_bucket_size_x, const int in_bucket_size_y);
   // Check if a pixel is inside the render view, ie not in the overscan frame
   bool IsPixelInsideView(const int in_x, const int in_y);
   // Compute the intersection between the renderview and a bucket
   unsigned int BucketInViewSize(const int in_bucket_xo, const int in_bucket_yo, 
                                 const int in_bucket_size_x, const int in_bucket_size_y,
                                 unsigned int &out_bucket_xo, unsigned int &out_bucket_yo, 
                                 unsigned int &out_bucket_size_x, unsigned int &out_bucket_size_y);

   int m_progressivePasses;
};


class DisplayDriver
{
public:
   // Create the display driver
   void CreateDisplayDriver();
   // Reset the rendered display area
   void ResetAreaRendered();
   // Update the render context in order to reuse the same Arnold driver
   // with another render session
   void UpdateDisplayDriver(RendererContext& in_rendererContext, unsigned int in_displayArea,
                            const bool in_filterColorAov, const bool in_filterNumericAov,
                            const bool in_useOptixOnMain, const bool in_onlyShowDenoise);
   // Sets the dithering
   void SetDisplayDithering(bool in_dither);

   RendererContext m_renderContext;
   bool  m_dither;
   int   m_renderHeight;
   int   m_displayArea;
   int   m_paintedDisplayArea;
   bool  m_useOptixOnMain;
   bool  m_onlyShowDenoise;
};


