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

#include <ai_color.h>
#include <ai_driver_utils.h>


using namespace XSI;

// Class that allocate a tile fragment of arnold bucket size 
// XSI will paint this tile into display
class RenderTile : public RendererImageFragment
{
public:

   // Constructor
   RenderTile(unsigned int in_offX, unsigned int in_offY, unsigned int in_width, unsigned int in_height, AtRGBA in_rgbBuffer[], bool in_dither);

   // Getters
   unsigned int GetOffsetX() const;
   unsigned int GetOffsetY() const;
   unsigned int GetWidth()   const;
   unsigned int GetHeight()  const;

   // Method that Softimage calls to get the buffer to paint into the render window
   bool GetScanlineRGBA(unsigned int uiRow, siImageBitDepth bitDepth, unsigned char *outScanline) const;

private:

   // Functions for pixel bit depth conversion
   void writeInteger8(void * outPixel, AtRGBA value, unsigned int row, unsigned int column) const;
   void writeInteger16(void * outPixel, AtRGBA value, unsigned int row, unsigned int column) const;
   void writeFloat32(void * outPixel, AtRGBA value, unsigned int row, unsigned int column) const;
   
   // Offsets & dimension of Bucket
   unsigned int   m_offX, m_offY, m_width, m_height;
   
   // Pixel Buffer of the bucket to paint
   AtRGBA   *m_buffer;

   // Dithering for 8-bit output
   bool m_dither;
};


