/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_imageclip2.h>
#include <xsi_pass.h>
#include <xsi_shader.h>

#include <ai_nodes.h>

using namespace XSI;

// Update an object's material. It's called when, in ipr, an object is moved to/out-of a group with a material
void UpdateObjectMaterial(const X3DObject &in_obj, double in_frame);
// Update a material
void UpdateMaterial(const Material &in_material, double in_frame);
// Update Shaders links attached to Material for IPR
void UpdateMaterialLinks(const Material &in_material, AtNode* in_surfaceNode, AtNode* in_environmentNode, double in_frame);
// Update Shader for IPR
AtNode* UpdateShader(const Shader &in_xsiShader, double in_frame);
// Update ImageClip for IPR
void UpdateImageClip(const ImageClip2 &in_xsiImageClip, double in_frame);
// Update Wrapping Settings
void UpdateWrappingSettings(const CRef &in_ref, double in_frame);
// Update the ShaderStack of Pass
void UpdatePassShaderStack(const Pass &in_pass, double in_frame);
// Assign to updateMaterial, the Material passed as parameter with the same material getted from Object
bool GetMaterialFromObject(const X3DObject &xsiObj, const Material &material, Material &updateMaterial);


// This utility class is to be used to compare two branches of a shading tree.
// The Fill method traverses backward a branch, starting from a given node, and packs
// all the parameter values into a raw buffer, whose structure is left undefined.
// The == operator compares two buffers, so returning true if two Fill's found exactly 
// the same connections along the branch, and the same values for all the parameters in the branch
//
// Currently, it is used in ParamLight.cpp to detect if a changed happened during ipr for the branch
// connected to the skydome color (or to just the color itself). If so, we have to flush the background cache.
//
class CShaderBranchDump
{
public:
   int  m_size;    // the buffer size
   char *m_buffer; // the buffer

   CShaderBranchDump()
   {
      m_size = 0;
      m_buffer = NULL;
   }

   ~CShaderBranchDump()
   {
      if (m_buffer)
         free(m_buffer);
   }

   bool Fill(AtNode* in_node, const char* in_paramName, bool in_recurse=true);
   bool operator==(const CShaderBranchDump &in_other) const;
};

