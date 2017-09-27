/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_framebuffer.h>
#include <xsi_pass.h>
#include <xsi_property.h>
#include <xsi_status.h>

#include <ai_nodes.h>

using namespace XSI;


// Frame Buffer class
class CFrameBuffer
{
public:
   Framebuffer m_fb; // the Softimage fb
   CString     m_name, m_fileName, m_fullName;
   CString     m_driverName, m_layerName;
   CString     m_layerDataType, m_driverBitDepth;
   CString     m_format;

   // standard constructor
   CFrameBuffer() {}
   // copy constructor
   CFrameBuffer(const CFrameBuffer& in_arg)
      : m_name(in_arg.m_name), m_fileName(in_arg.m_fileName), m_fullName(in_arg.m_fullName), m_driverName(in_arg.m_driverName), m_layerName(in_arg.m_layerName), 
      m_layerDataType(in_arg.m_layerDataType), m_driverBitDepth(in_arg.m_driverBitDepth), m_format(in_arg.m_format)
   {}
   // Construct by a Softimage framebuffer
   CFrameBuffer(Framebuffer in_fb, double in_frame, bool in_checkLightsAov);
   // standard constructor
   ~CFrameBuffer() {}
   // Check if the format is ok. If not, for the main channel, default to tiff
   bool IsValid(CString in_mainFormat);
   // return whether this fb is an exr
   bool IsExr();
   // return true if this is a half float precision framebuffer
   bool IsHalfFloat();
   // Compare the bitdepth with another framebuffer one, and prompt a warning if they differ
   void CheckBitDepth(CFrameBuffer in_fb);
   // Log all the string data
   void Log();
};

// Export the frame and fps into the options node
void LoadPlayControlData(AtNode* in_optionsNode, double in_frame);
// Load the output filters
bool LoadFilters();
// Load the drivers
bool LoadDrivers(AtNode *in_optionsNode, Pass &in_pass, double in_frame, bool in_flythrough);
// Load the options parameters
void LoadOptionsParameters(AtNode* in_optionsNode, const Property &in_arnoldOptions, double in_frame);
// Load the options node
CStatus LoadOptions(const Property &in_arnoldOptions, double in_frame, bool in_flythrough=false);
// Post load options.subdiv_dicing_camera, because it needs the camera Arnold node
CStatus PostLoadOptions(const Property &in_optionsNode, double in_frame);

