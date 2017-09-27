/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "renderer/RenderTile.h"


RenderTile::RenderTile(unsigned int in_offX, unsigned int in_offY, unsigned int in_width, unsigned int in_height, AtRGBA in_rgbBuffer[], bool in_dither)
{
   m_buffer = in_rgbBuffer;
   m_offX   = in_offX;
   m_offY   = in_offY;
   m_width  = in_width;
   m_height = in_height;
   m_dither = in_dither;
}

unsigned int RenderTile::GetOffsetX() const 
{ 
   return m_offX; 
}

unsigned int RenderTile::GetOffsetY() const 
{ 
   return m_offY; 
}

unsigned int RenderTile::GetWidth() const 
{ 
   return m_width; 
}

unsigned int RenderTile::GetHeight() const 
{ 
   return m_height; 
}

void RenderTile::writeInteger8(void * outPixel, AtRGBA value, unsigned int row, unsigned int column) const
{
   uint8_t* pixel = (uint8_t*)outPixel;
   pixel[0] = AiQuantize8bit(row, column, 0, value.r, m_dither);
   pixel[1] = AiQuantize8bit(row, column, 0, value.g, m_dither);
   pixel[2] = AiQuantize8bit(row, column, 0, value.b, m_dither);
   pixel[3] = (uint8_t)(value.a * 255);
}

void RenderTile::writeInteger16(void * outPixel, AtRGBA value, unsigned int row, unsigned int column) const
{
   uint16_t* pixel = (uint16_t*)outPixel;
   pixel[0] = AiQuantize16bit(row, column, 0, value.r, m_dither);
   pixel[1] = AiQuantize16bit(row, column, 0, value.g, m_dither);
   pixel[2] = AiQuantize16bit(row, column, 0, value.b, m_dither);
   pixel[3] = (uint16_t)(value.a * 65535);
}

void RenderTile::writeFloat32(void * outPixel, AtRGBA value, unsigned int row, unsigned int column) const
{
   AtRGBA * pixel = (AtRGBA *)outPixel;
   *pixel = value;
}

typedef void (RenderTile::*bitDepthCopy)(void * outPixel, AtRGBA value, unsigned int row, unsigned int column) const;

bool RenderTile::GetScanlineRGBA(unsigned int uiRow, siImageBitDepth bitDepth, unsigned char *outScanline) const
{
   // Softimage doesn't honor the set types so we need to support 8 bit as well as float
   bitDepthCopy convertPixel = 0;
   int pixelSize = 0;

   switch (bitDepth)
   {
      case siImageBitDepthInteger8:
         convertPixel = &RenderTile::writeInteger8;
         pixelSize = 4;
         break;
         
      case siImageBitDepthInteger16:
         convertPixel = &RenderTile::writeInteger16;
         pixelSize = 8;
         break;
         
      case siImageBitDepthFloat32:
         convertPixel = &RenderTile::writeFloat32;
         pixelSize = 16;
         break;
         
      default:
         return false;
   }
   
   int idx = (m_height-uiRow-1)*m_width;

   void *scanline;
   for (unsigned int i=0; i<m_width; i++)
   {
      scanline = (void *)(&outScanline[pixelSize * i]);
      (this->*convertPixel)(scanline, m_buffer[idx+i], uiRow + m_offY, i + m_offX);
   }

   return true;
}

