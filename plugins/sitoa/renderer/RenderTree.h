/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <xsi_material.h>
#include <xsi_shader.h>

using namespace XSI;

// Class hosting utilities to explore a render tree
class CRenderTree
{
public:

   CRenderTree() {}
   ~CRenderTree() {}

   // Returns if shader is a compound
   bool IsCompound(const Shader &in_shader);
   // returns true if the in_parameter source is a shader or a compound
   bool GetParameterShaderSource(const Parameter &in_parameter, Shader &out_sourceShader);
   // For a given in_shader, return the displacement shader (if any) attached to the material
   // regardless if in_shader is part of the displacement branch
   bool GetDisplacementShader(Shader in_shader, Shader &out_shader);
   // Returns all the parameters (input and output) of a shader
   CParameterRefArray GetShaderParameters(const Shader &in_shader);
   // Return whether or not this is an input parameter of the given shader
   bool IsParameterInput(const Shader &in_shader, const Parameter &in_param);
   // Returns all the input or output parameters of a shader
   CParameterRefArray GetShaderInputOutputParameters(const Shader &in_shader, bool in_input);
   // Returns all the input parameters of a shader
   CParameterRefArray GetShaderInputParameters(const Shader &in_shader);
   // Returns all the output parameters of a shader
   CParameterRefArray GetShaderOutputParameters(const Shader &in_shader);
   // Returns all the shaders connected to the input parameters of a shader
   CRefArray GetShaderInputShaders(const Shader &in_shader);
   // Recursively search for in_shaderToFind in the branch starting (right to left) from in_shader
   void FindBackward(const Shader shader, const Shader shaderToFind, bool &found);
   // Recursively search for all the shaders by prog id in the branch starting (right to left) from in_shader
   void FindAllShadersByProgIdBackward(const Shader in_shader, const CString in_progId, CRefArray &out_array);
   // Recursively search for all the shaders nested under in_compound
   void FindAllShadersUnderCompound(const Shader &in_compound, CRefArray &out_shadersArray);
   // Search for all the shaders nested under in_material
   void FindAllShadersUnderMaterial(const Material &in_material, CRefArray &out_shadersArray);
};


