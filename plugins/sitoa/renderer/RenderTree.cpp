/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "renderer/RenderTree.h"


// Returns if in_shader is a compound
//
// @param in_shader                 The input shader
// @return true if in_shader is a compound, else false
//
bool CRenderTree::IsCompound(const Shader &in_shader)
{
   CString families = in_shader.GetFamilies();
   return families.FindString(L"Shader Compounds") != UINT_MAX;
};


// Returns true if the in_parameter source is a shader or a compound
// @param in_parameter     The input parameter
// @param out_sourceShader The returned shader
// @return true if the source is a valid shader or compound
//
bool CRenderTree::GetParameterShaderSource(const Parameter &in_parameter, Shader &out_sourceShader)
{
   Parameter param;

   CRef source = in_parameter.GetSource();
   if (!source.IsValid())
      return false; //no source

   // Check if the source shader is in fact an output of a compound.
   // Another case is multi output shader, although unsupported in Arnold
   // A nastier case is when a compound output plugs into a compound input.
   // In this case, the while will cycle 2 times instead of 1
   while (source.IsValid() && (source.GetClassIDName() == L"Parameter" || source.GetClassIDName() == L"ShaderParameter"))
   {
      param = (Parameter)source;
      // the source is the owner shader (or compound)
      source = param.GetParent();
   }

   out_sourceShader = (Shader)source;

   return out_sourceShader.IsValid();
}


// For a given in_shader, return the displacement shader (if any) attached to the material
// regardless if in_shader is part of the displacement branch
//
// @param in_shader     The input shader
// @param out_shader    The returned displacement shader
// @return true if a displacement shader was found, else false
//
bool CRenderTree::GetDisplacementShader(Shader in_shader, Shader &out_shader)
{
   // in_shader could be nested into a compound, so let's get it
   while (in_shader.GetShaderContainer().GetClassIDName() == L"Shader")
      in_shader = (Shader)in_shader.GetShaderContainer();

   // get the material
   Material material = in_shader.GetParent();
   // get what is connected to the displacement slot
   CRef source = material.GetParameter(L"displacement").GetSource();
   if (!source.IsValid())
      return false;

   // check if the displacement shader is in fact an output of a compound.
   // another case is multi output shader, although unsopported in Arnold
   if (source.GetClassIDName() == L"Parameter" || source.GetClassIDName() == L"ShaderParameter")
   {
      Parameter outParameter(source);
      // the source is the owner shader (or compound)
      source = outParameter.GetParent();
   }

   out_shader = (Shader)source;
	return true;
}


// Returns all the parameters (input and output) of a shader
//
// @param in_shader     The input shader
// @return the CParameterRefArray of parameters
//
CParameterRefArray CRenderTree::GetShaderParameters(const Shader &in_shader)
{
   CParameterRefArray parameters;

	if (IsCompound(in_shader))
   {
      CRefArray nestedObjects = in_shader.GetNestedObjects();
      for (LONG i=0; i<nestedObjects.GetCount(); i++)
	   {
         if (nestedObjects.GetItem(i).GetClassIDName() == L"Parameter" || nestedObjects.GetItem(i).GetClassIDName() == L"ShaderParameter")
            parameters.Add(nestedObjects.GetItem(i));
      }
   }
   else
      parameters = in_shader.GetParameters();

   return parameters;
}


// Return whether or not this is an input parameter of the given shader
//
// @param in_shader    The shader
// @param in_param     The shader's parameter
//
// @return true if in_param is an input parameter, false if it's an output parameter
//
bool CRenderTree::IsParameterInput(const Shader &in_shader, const Parameter &in_param)
{
   bool result;
   siShaderParameterType shaderparamType = in_shader.GetShaderParameterType(in_param.GetName(), result);
   // GetShaderParameterType always return true as its second argument for compounds.
   // However, if in_param is an output parameter, it returns siUnknownParameterType.
   // In case they fix GetShaderParameterType, let's remember to be back here.
   if (shaderparamType == siUnknownParameterType)
      return false;
   return result;
}


// Returns all the input or output parameters of a shader
//
// @param in_shader     The input shader
// @param input         If true, return the input parameters, else the output ones
// @return the CParameterRefArray of parameters
//
CParameterRefArray CRenderTree::GetShaderInputOutputParameters(const Shader &in_shader, bool in_input)
{
   CParameterRefArray parameters;
   Parameter param;
   bool isInput;

	if (IsCompound(in_shader))
   {
      CRefArray nestedObjects = in_shader.GetNestedObjects();
      for (LONG i=0; i<nestedObjects.GetCount(); i++)
	   {
         if (nestedObjects.GetItem(i).GetClassIDName() == L"Parameter" || nestedObjects.GetItem(i).GetClassIDName() == L"ShaderParameter")
         {
            param = (Parameter)nestedObjects.GetItem(i);
            isInput = IsParameterInput(in_shader, param);
            if (isInput == in_input)
               parameters.Add(nestedObjects.GetItem(i));
         }
      }
   }
   else
   {
      CParameterRefArray tempArray = in_shader.GetParameters();
      for (LONG i=0; i<tempArray.GetCount(); i++)
	   {
         param = (Parameter)tempArray[i];
         isInput = IsParameterInput(in_shader, param);
         if (isInput == in_input)
            parameters.Add(tempArray[i]);
      }
   }

   return parameters;
}


// Returns all the input parameters of a shader
//
// @param in_shader     The input shader
// @return the CParameterRefArray of parameters
//
CParameterRefArray CRenderTree::GetShaderInputParameters(const Shader &in_shader)
{
   return GetShaderInputOutputParameters(in_shader, true);
}


// Returns all the output parameters of a shader
//
// @param in_shader     The input shader
// @return the CParameterRefArray of parameters
//
CParameterRefArray CRenderTree::GetShaderOutputParameters(const Shader &in_shader)
{
   return GetShaderInputOutputParameters(in_shader, false);
}


// Returns all the shaders connected to the input parameters of a shader
//
// @param in_shader     The input shader
// @return the CRefArray of shaders
//
CRefArray CRenderTree::GetShaderInputShaders(const Shader &in_shader)
{
   CRefArray result;
   Parameter param;
   CRef source;

   // collect all the input parameters

   CParameterRefArray parameters = GetShaderInputParameters(in_shader);
  
   for (LONG i=0; i<parameters.GetCount(); i++)
   {
      param = (Parameter)parameters[i];

      source = param.GetSource();
      if (!source.IsValid())
         continue;

      while (source.IsValid() && (source.GetClassIDName() == L"Parameter" || source.GetClassIDName() == L"ShaderParameter"))
      {
         param = (Parameter)source;
         // the source is the owner shader (or compound)
         source = param.GetParent();
      }

      Shader shader = (Shader)source;

      if (shader.IsValid()) // other source type could be expression, fcurve, etc
         result.Add(source);
   }

   return result;
}


// Recursively search for in_shaderToFind in the branch starting (right to left) from in_shader
//
// @param in_shader         The branch rightmost node
// @param in_shaderToFind   The shader to find
// @param out_found         The result
//
void CRenderTree::FindBackward(const Shader in_shader, const Shader in_shaderToFind, bool &out_found)
{
   CRefArray nestedObjects;
   Parameter param;
   CRef source;

 	if (in_shader == in_shaderToFind)
   {
		out_found = true;
      return;
   }
   // The shader to find is in fact a compound, but we cycle the shaders.
   // So, we must check against the shaders' container (ie the containing compound, if any)
   if (IsCompound(in_shaderToFind))
   {
      if (in_shader.GetShaderContainer() == in_shaderToFind.GetRef())
      {
		   out_found = true;
         return;
      }
   }
   // and viceversa
   if (IsCompound(in_shader))
   {
      if (in_shaderToFind.GetShaderContainer() == in_shader.GetRef())
      {
		   out_found = true;
         return;
      }
   }

   // collect all the input shaders and recurse
   CRefArray inputShaders = GetShaderInputShaders(in_shader);
   for (LONG i=0; i<inputShaders.GetCount() && !out_found; i++)
	{
      Shader shader(inputShaders.GetItem(i));
      FindBackward(shader, in_shaderToFind, out_found);
   }
}


// Recursively search for all the shaders by prog id in the branch starting (right to left) from in_shader
//
// @param in_shader         The branch rightmost node
// @param in_progId         The shader prog id to find
// @param out_array         The found shaders
//
void CRenderTree::FindAllShadersByProgIdBackward(const Shader in_shader, const CString in_progId, CRefArray &out_array)
{
   CString progId = in_shader.GetProgID();
   if (progId.FindString(in_progId) != UINT_MAX)
      out_array.Add(in_shader.GetRef());

   // collect all the input shaders and recurse
   CRefArray inputShaders = GetShaderInputShaders(in_shader);
   for (LONG i=0; i<inputShaders.GetCount(); i++)
	{
      Shader shader(inputShaders.GetItem(i));
      FindAllShadersByProgIdBackward(shader, in_progId, out_array);
   }
}


// Recursively search for all the shaders nested under a compound
//
// @param in_compound       The input shaders compound
// @param out_shadersArray  The result CRef array of shaders
//
void CRenderTree::FindAllShadersUnderCompound(const Shader &in_compound, CRefArray &out_shadersArray)
{
   CRefArray shaders(in_compound.GetAllShaders());
   for (LONG i=0; i<shaders.GetCount(); i++)
   {
      Shader shader(shaders[i]);
      if (IsCompound(shader))
         FindAllShadersUnderCompound(shader, out_shadersArray);
      else
         out_shadersArray.Add(shader.GetRef());
   }
}


// Search for all the shaders nested under a material
//
// @param in_material       The input material
// @param out_shadersArray  The result CRef array of shaders
//
void CRenderTree::FindAllShadersUnderMaterial(const Material &in_material, CRefArray &out_shadersArray)
{
   CRefArray shaders(in_material.GetAllShaders());
   for (LONG i=0; i<shaders.GetCount(); i++)
   {
      Shader shader(shaders[i]);
      if (IsCompound(shader))
         FindAllShadersUnderCompound(shader, out_shadersArray);
      else
         out_shadersArray.Add(shader.GetRef());
   }
}

