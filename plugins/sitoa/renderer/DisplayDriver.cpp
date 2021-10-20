/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/Tools.h"
#include "renderer/DisplayDriver.h"
#include "renderer/Drivers.h"
#include "renderer/Renderer.h"
#include "renderer/RenderTile.h"

#include <ai_render.h>

#include <xsi_framebuffer.h>
#include <xsi_renderchannel.h>

AI_DRIVER_NODE_EXPORT_METHODS(display_driver_mtd);


// Initialize the driver data
//
// @param in_display_window    the render view, without overscan
// @param in_data_window       the render data view, including overscan
//
void CDisplayDriverData::Init(const AtBBox2 &in_display_window, const AtBBox2 &in_data_window)
{
   m_display_window = in_display_window;
   m_data_window    = in_data_window;
   m_overscan = m_display_window.minx != m_data_window.minx || m_display_window.miny != m_data_window.miny || 
                m_display_window.maxx != m_data_window.maxx || m_display_window.maxy != m_data_window.maxy;

   // get parameters necessary for progressive progress bar
   AtNode* options = AiUniverseGetOptions();
   bool progressive = AiNodeGetBool(options, "enable_progressive_render");
   int aaSamples = AiNodeGetInt(options, "AA_samples");
   bool adaptiveSampling = AiNodeGetBool(options, "enable_adaptive_sampling");
   int aaSamplesMax = AiNodeGetInt(options, "AA_samples_max");

   if (progressive)
   {
    if (adaptiveSampling && (aaSamplesMax > aaSamples))
        m_progressivePasses = aaSamplesMax * aaSamplesMax;
    else
        m_progressivePasses = aaSamples * aaSamples;
   }

}


// Check if a bucket is completely outside the render view, ie all inside the overscan frame
//
// @param in_bucket_xo        the bucket's min x
// @param in_bucket_yo        the bucket's min y
// @param in_bucket_size_x    the bucket's width
// @param in_bucket_size_y    the bucket's height
//
// @return true if the bucket is completely contained into the overscan (external) frame, alse false
//
bool CDisplayDriverData::IsBucketOutsideView(const int in_bucket_xo, const int in_bucket_yo, 
                                             const int in_bucket_size_x, const int in_bucket_size_y)
{
   if (!m_overscan)
      return false;

   return in_bucket_xo + in_bucket_size_x < 0  || // right-most corner is on the left edge of view
          in_bucket_xo > m_display_window.maxx || // left-most corner is on the right edge of view
          in_bucket_yo + in_bucket_size_y < 0  || // top-most corner is below the bottom edge of view
          in_bucket_yo > m_display_window.maxy;   // bottom-most corner is above the top edge of view
}


// Check if a bucket is completely inside the render view, ie with no intersection with the overscan frame
//
// @param in_bucket_xo        the bucket's min x
// @param in_bucket_yo        the bucket's min y
// @param in_bucket_size_x    the bucket's width
// @param in_bucket_size_y    the bucket's height
//
// @return true if the bucket is completely contained into the render view, alse false
//
bool CDisplayDriverData::IsBucketInsideView(const int in_bucket_xo, const int in_bucket_yo, 
                                            const int in_bucket_size_x, const int in_bucket_size_y)
{
   if (!m_overscan)
      return true;

   return in_bucket_xo >= 0 && in_bucket_xo + in_bucket_size_x <= m_display_window.maxx && 
          in_bucket_yo >= 0 && in_bucket_yo + in_bucket_size_y <= m_display_window.maxy;
}


// Check if a pixel is inside the render view, ie not in the overscan frame
//
// @param in_x        the pixel's x
// @param in_y        the pixel's y
//
// @return true if the pixel is part of the render view, alse false
//
bool CDisplayDriverData::IsPixelInsideView(const int in_x, const int in_y)
{
   if (!m_overscan)
      return true;

   return in_x >= 0 && in_x <= m_display_window.maxx && in_y >= 0 && in_y <= m_display_window.maxy;
}


// Compute the intersection between the renderview and a bucket
//
// @param in_bucket_xo         the bucket's min x
// @param in_bucket_yo         the bucket's min y
// @param in_bucket_size_x     the bucket's width
// @param in_bucket_size_y     the bucket's height
// @param out_bucket_xo        the returned bucket's min x
// @param out_bucket_yo        the returned bucket's min y
// @param out_bucket_size_x    the returned bucket's width
// @param out_bucket_size_y    the returned bucket's height
//
// @return the size of the returned bucket
//
unsigned int CDisplayDriverData::BucketInViewSize(const int in_bucket_xo, const int in_bucket_yo, 
                                                  const int in_bucket_size_x, const int in_bucket_size_y,
                                                  unsigned int &out_bucket_xo, unsigned int &out_bucket_yo, 
                                                  unsigned int &out_bucket_size_x, unsigned int &out_bucket_size_y)
{
   if (IsBucketOutsideView(in_bucket_xo, in_bucket_yo, in_bucket_size_x, in_bucket_size_y))
       return 0;

   if (IsBucketInsideView(in_bucket_xo, in_bucket_yo, in_bucket_size_x, in_bucket_size_y))
   {
      out_bucket_xo = in_bucket_xo;
      out_bucket_yo = in_bucket_yo;
      out_bucket_size_x = in_bucket_size_x;
      out_bucket_size_y = in_bucket_size_y;
   }
   else
   {
      out_bucket_xo = AiMax(0, in_bucket_xo);
      unsigned int right_bound = AiMin(m_display_window.maxx, in_bucket_xo + in_bucket_size_x - 1);
      out_bucket_yo = AiMax(0, in_bucket_yo);
      unsigned int top_bound = AiMin(m_display_window.maxy, in_bucket_yo + in_bucket_size_y - 1);
      out_bucket_size_x = right_bound - out_bucket_xo + 1;
      out_bucket_size_y = top_bound   - out_bucket_yo + 1;
   }

   return out_bucket_size_x * out_bucket_size_y;
}


node_parameters {}

node_initialize
{
   CDisplayDriverData *ddData = new CDisplayDriverData;
   AiNodeSetLocalData(node, ddData);
   AiDriverInitialize(node, true);
}

node_update {}

driver_supports_pixel_type
{
   return pixel_type == AI_TYPE_RGB    || pixel_type == AI_TYPE_RGBA  || 
          pixel_type == AI_TYPE_VECTOR || pixel_type == AI_TYPE_FLOAT;
}

driver_extension { return NULL; }

driver_open
{
   CDisplayDriverData *ddData = (CDisplayDriverData*)AiNodeGetLocalData(node);
   ddData->Init(display_window, data_window);
}


driver_prepare_bucket {}

driver_process_bucket
{
   const void*   bucket_data;
   int           i, j, pixel_type;
   AtRGB         rgb;
   AtRGBA        rgba;
   AtString      aov_name;

   CDisplayDriverData *ddData = (CDisplayDriverData*)AiNodeGetLocalData(node);
      
   CRenderInstance* renderInstance = GetRenderInstance();
   DisplayDriver* displayDriver = renderInstance->GetDisplayDriver();
   

   if (renderInstance->InterruptRenderSignal())
      return;

   if (!AiOutputIteratorGetNext(iterator, &aov_name, &pixel_type, &bucket_data))
      return;

   // Progress bar
   displayDriver->m_paintedDisplayArea += (bucket_size_x * bucket_size_y);
   int percent = (int)((displayDriver->m_paintedDisplayArea / (float)displayDriver->m_displayArea) * 100.0f);
   // if in progressive render mode we need to divide percent by number of progressive passes
   if (ddData->m_progressivePasses > 1)
      percent = percent / ddData->m_progressivePasses;
   displayDriver->m_renderContext.ProgressUpdate(CValue(percent).GetAsText() + L"%   Rendered", L"Rendering", percent);

   // if the Arnold bucket is completely in the overscan frame, don't
   // send it to the Softimage render view
   if (ddData->IsBucketOutsideView(bucket_xo, bucket_yo, bucket_size_x, bucket_size_y))
       return;

   // compute the Softimage render view bucket corners, in case the Arnold bucket
   // intersects the Softimage render view. If the bucket is NOT in the overscan
   // frame, but entirely in the Softimage view, then view_bucket* == bucket_*
   unsigned int view_bucket_xo, view_bucket_yo, view_bucket_size_x, view_bucket_size_y;
   int view_bucket_size = ddData->BucketInViewSize(bucket_xo, bucket_yo, bucket_size_x, bucket_size_y,
                                                   view_bucket_xo, view_bucket_yo, view_bucket_size_x, view_bucket_size_y);
   // buffer to be sent to Softimage
   AtRGBA *buffer = new AtRGBA[view_bucket_size];

   int buffer_index(0); // index for the Softimage buffer
   int bucket_column, bucket_zero_column;

   int bucket_max_x = bucket_xo + bucket_size_x;
   int bucket_max_y = bucket_yo + bucket_size_y;

   switch (pixel_type)
   {
      case AI_TYPE_RGBA:
         for (j = bucket_yo; j < bucket_max_y; j++)
         {
            bucket_zero_column = (j - bucket_yo) * bucket_size_x;
            for (i = bucket_xo; i < bucket_max_x; i++)
            {
               // if all is correct, buffer_index will never overtake view_bucket_size, 
               // however let's protect the render view buffer. Also, this will avoid 
               // (because eventually buffer_index == view_bucket_size) the check below
               // for the remaining pixels, that should all be outside the view.
               if (buffer_index >= view_bucket_size)
                   break;
               // skip pixels in the overscan frame
               if (!ddData->IsPixelInsideView(i, j))
                   continue;

               bucket_column = i - bucket_xo; // always positive
               buffer[buffer_index] = ((AtRGBA*)bucket_data)[bucket_zero_column + bucket_column];
               buffer_index++;
            }
         }
         break;

      case AI_TYPE_VECTOR:
      case AI_TYPE_RGB:
         for (j = bucket_yo; j < bucket_max_y; j++)
         {
            bucket_zero_column = (j - bucket_yo) * bucket_size_x;
            for (i = bucket_xo; i < bucket_max_x; i++)
            {
               if (buffer_index >= view_bucket_size)
                   break;
               if (!ddData->IsPixelInsideView(i, j))
                   continue;

               bucket_column = i - bucket_xo;
               rgb = ((AtRGB*)bucket_data)[bucket_zero_column + bucket_column];
               buffer[buffer_index].r = rgb.r;
               buffer[buffer_index].g = rgb.g;
               buffer[buffer_index].b = rgb.b;
               buffer[buffer_index].a = 1.0f;
               buffer_index++;
            }
         }
         break;

      case AI_TYPE_FLOAT:
         rgba.a = 1.0f;
         for (j = bucket_yo; j < bucket_max_y; j++)
         {
            bucket_zero_column = (j - bucket_yo) * bucket_size_x;
            for (i = bucket_xo; i < bucket_max_x; i++)
            {
               if (buffer_index >= view_bucket_size)
                   break;
               if (!ddData->IsPixelInsideView(i, j))
                   continue;

               bucket_column = i - bucket_xo;
               rgba.r = rgba.g = rgba.b = ((float*)bucket_data)[bucket_zero_column + bucket_column];
               buffer[buffer_index] = rgba;
               buffer_index++;
            }
         }
         break;
   }

   if (!renderInstance->InterruptRenderSignal())
   {
      RenderTile fragment(view_bucket_xo, displayDriver->m_renderHeight - view_bucket_yo - view_bucket_size_y , 
                          view_bucket_size_x, view_bucket_size_y, buffer, displayDriver->m_dither);
       
      displayDriver->m_renderContext.NewFragment(fragment);
   }

   delete[] buffer;
}


driver_close {}


driver_needs_bucket { return true; }


driver_write_bucket {}


node_finish
{
   CDisplayDriverData *ddData = (CDisplayDriverData*)AiNodeGetLocalData(node);
   delete ddData;
}


void DisplayDriver::CreateDisplayDriver()
{   
   AtNode *driver;

   if (!AiNodeEntryLookUp("display_driver"))
   {
      AiNodeEntryInstall(AI_NODE_DRIVER, AI_TYPE_NONE, "display_driver", NULL, (AtNodeMethods*) display_driver_mtd, AI_VERSION);
   
      driver = AiNode("display_driver");
      CNodeUtilities().SetName(driver, "xsi_driver");
   }
}


void DisplayDriver::UpdateDisplayDriver(RendererContext& in_rendererContext, unsigned int in_displayArea, 
                                        const bool in_filterColorAov, const bool in_filterNumericAov)
{
   m_renderContext = in_rendererContext;
   m_displayArea = in_displayArea;     
   m_renderHeight = (int)m_renderContext.GetAttribute(L"ImageHeight");

   AtNode* options = AiUniverseGetOptions();   
   AtArray *outputs, *new_outputs;

   CString displayDriver;

   // Duplicate the old outputs array
   outputs = AiNodeGetArray(options, "outputs");

   // Determine the format of the Render Channel that will be displayed
   Framebuffer frameBuffer = m_renderContext.GetDisplayFramebuffer();
   RenderChannel renderchannel = frameBuffer.GetRenderChannel();
   CString layerName = GetLayerName(renderchannel.GetName());
   CString layerdataType;

   if (m_renderContext.GetAttribute(L"FileOutput"))
   {          
      // Display Driver format will use the DataType of the MAIN framebuffer Softimage
      // returns us from m_renderContext.GetDisplayFramebuffer() to avoid black images in render window (trac #780)
      layerdataType = ParAcc_GetValue(frameBuffer, L"DataType", DBL_MAX).GetAsText();
      if (layerdataType.IsEqualNoCase(L"RGB") || layerdataType.IsEqualNoCase(L"RGBA"))
      {
         if (in_filterColorAov)
            displayDriver = layerName + L" " + layerdataType + L" sitoa_output_filter xsi_driver";
         else
            displayDriver = layerName + L" " + layerdataType + L" sitoa_closest_filter xsi_driver";
      }
      else
      {
         if (in_filterNumericAov)
            displayDriver = layerName + L" " + layerdataType + L" sitoa_output_filter xsi_driver";
         else
            displayDriver = layerName + L" " + layerdataType + L" sitoa_closest_filter xsi_driver";
      }

      // Maintain the current output driver and add at the end the display driver.
      new_outputs = AiArrayAllocate(AiArrayGetNumElements(outputs) + 1, 1, AI_TYPE_STRING);
      for (unsigned int i = 0; i < AiArrayGetNumElements(outputs); ++i)
         AiArraySetStr(new_outputs, i, CStringUtilities().Strdup(AiArrayGetStr(outputs, i)));

      AiArraySetStr(new_outputs, AiArrayGetNumElements(outputs), CStringUtilities().Strdup(displayDriver.GetAsciiString()));
   }
   else
   {
      // Display Driver format will use the layer data type from the renderchannel
      layerdataType = GetDriverLayerDataTypeByName(layerName);

      // If empty get dataType from RenderChannel.ChannelType.
      if (layerdataType.IsEqualNoCase(L""))
         layerdataType = GetDriverLayerChannelType((LONG)renderchannel.GetChannelType());

      if (layerdataType.IsEqualNoCase(L"RGB") || layerdataType.IsEqualNoCase(L"RGBA"))
      {
         if (in_filterColorAov)
            displayDriver = layerName + L" " + layerdataType + L" sitoa_output_filter xsi_driver";
         else
            displayDriver = layerName + L" " + layerdataType + L" sitoa_closest_filter xsi_driver";
      }
      else
      {
         if (in_filterNumericAov)
            displayDriver = layerName + L" " + layerdataType + L" sitoa_output_filter xsi_driver";
         else
            displayDriver = layerName + L" " + layerdataType + L" sitoa_closest_filter xsi_driver";
      }
      
      new_outputs = AiArray(1, 1, AI_TYPE_STRING, CStringUtilities().Strdup(displayDriver.GetAsciiString()));
   }

   AiNodeSetArray(options, "outputs", new_outputs);
}


void DisplayDriver::ResetAreaRendered()
{
   m_paintedDisplayArea = 0;
}


void DisplayDriver::SetDisplayDithering(bool in_dither)
{
   m_dither = in_dither;
}
