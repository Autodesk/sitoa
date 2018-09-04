/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCommon.h"
#include "common/ParamsShader.h"
#include "loader/Shaders.h"
#include "renderer/Renderer.h"

#include <xsi_fcurve.h>
#include <xsi_expression.h>
#include <xsi_shaderarrayparameter.h>

// Load the source of a parameter
//
// Loads the source of a parameter checking whether it's a compound
// to ensure we pass through to the real shader node
// 
// @param in_param This is the parameter to get the source of
//
// @return A CRef to param's source, or itself if the param was not connected
CRef GetParameterSource(const Parameter& in_param)
{
   CRef source = in_param.GetSource();
   // Trivial case, the parameter is not connected
   // return the CRef of the param
   if (!source.IsValid())
   {
      CRef target = in_param.GetRef();

      // We will check if the parameter is from a shader connection
      // If so, we will return the shader instead of the parameter
      // We have also to avoid doing this check in shader Compounds
      // and in that case just return the parameter

      // In Softimage < 2011, instead of a parameter we get the shader connection 
      // already so we return
      if (target.IsA(siParameterID))
      {
         Parameter shaderParam(target);
         CRef shaderRef = shaderParam.GetParent();

         if (shaderRef.IsA(siShaderID))
         {
            Shader shader(shaderRef);            
            // Only return the shader if it is a normal shader (not compound or comment)
            if (shader.GetShaderType() == siShader)
            {
               // we detect the output by the name out and result which is the most common case
               if (shaderParam.GetScriptName() == L"out" || shaderParam.GetScriptName() == L"result" )
                  target = shader.GetRef();
            }

         }
      }
      return target;
   }   
   else if (source.IsA(siParameterID))
   {
      Parameter sourceParam(source);
      return GetParameterSource(sourceParam);   
   }
   else
      return source;
}


// Loads into a shader node all the parameters from a xsi shader
//
// @param in_node                 Arnold Node where to load the parameters.
// @param in_paramsArray          Array that contains all parameters XSI has on the shader.
// @param in_frame                The frame time.
// @param in_ref                  The object owning the shader.
// @param in_recursively          Recursively load of the Parameters
//
// @return                        CStatus::OK success / CStatus::Fail is returned in case of failure.
//
CStatus LoadShaderParameters(AtNode* in_node, CRefArray &in_paramsArray, double in_frame, 
                             const CRef &in_ref, bool in_recursively)
{
   bool isLight = CNodeUtilities().GetEntryType(in_node) == L"light";

   CString entryName = CNodeUtilities().GetEntryName(in_node);

   LONG nbParameters = in_paramsArray.GetCount();

   for (LONG i=0; i<nbParameters; i++)
   {
      Parameter param(in_paramsArray[i]);

      if (!param.IsValid())
         continue;
      // Ignoring not textureable and non inspectable parameters
      if (!(param.GetCapabilities() & siTexturable))
      if (param.GetCapabilities() & siNotInspectable)
         continue;

      // Ignoring Softimage nonused and output parameters
      CString paramScriptName = param.GetScriptName();
      if (paramScriptName.IsEqualNoCase(L"Name") || paramScriptName.IsEqualNoCase(L"out") || paramScriptName.IsEqualNoCase(L"result") )
         continue;

      // skip lights' filter plugs, they are loaded separately
      if (isLight)
      {
         bool skipMe(false);
         for (int j=1; j<=MAX_FILTERS; j++)
         {
            if (paramScriptName == L"filter" + (CString)j)
            {
               skipMe = true;
               break;
            }
         }
         if (skipMe)
            continue;
      }

      LoadShaderParameter(in_node, entryName, param, in_frame, in_ref, in_recursively);
   }

   return CStatus::OK;
}


// Loads into a shader node the specified parameter 
// The method checks the Source of the parameter to parse it (if it is another Shader or ImageClip) or
// evaluate the parameter directly to assign it to the Node parameter.
//
// @param *in_node                Arnold Node where to load the parameters.
// @param in_entryName            the node entry name
// @param &in_param               The Shader Parameter.
// @param in_frame                The frame time.
// @param in_ref                  The object owning the shader.
// @param in_recursively          Recursively load of the Parameters
// @param in_arrayParamName       The original name of the Array Parameter
// @param in_arrayElement         Set the array element (-1 if the parameter is not an element of an array)
//
// @return                        CStatus::OK success / CStatus::Fail is returned in case of failure.
//
CStatus LoadShaderParameter(AtNode* in_node, const CString &in_entryName, Parameter &in_param, double in_frame, const CRef &in_ref, 
                            bool in_recursively, const CString& in_arrayParamName, int in_arrayElement)
{
   // Note: For all Parameters we must to get their names with GetScriptName()
   CStatus status;

   CRef source = GetParameterSource(in_param);
   siClassID sourceID = source.GetClassID();

   AtNode* shaderLinked;

   if (sourceID == siShaderID || sourceID == siTextureID)
   {
      Shader shader(source);
      shaderLinked = LoadShader(shader, in_frame, in_ref, in_recursively);

      // abort if we can't load the linked shader
      if (!shaderLinked)
         return status;

      CString paramScriptName(in_param.GetScriptName());

      // Checking if the Arnold param is a NODE.
      // If it is, we need to do a CNodeSetter::SetPointer instead AiNodeLink()
      int paramType = GetArnoldParameterType(in_node, paramScriptName.GetAsciiString());

      if (paramType == AI_TYPE_NODE)
         CNodeSetter::SetPointer(in_node, paramScriptName.GetAsciiString(), shaderLinked);
      else
      {
         if (in_arrayElement != -1)
            paramScriptName = in_arrayParamName + L"[" + CString(CValue(in_arrayElement).GetAsText()) + L"]";

         AiNodeLink(shaderLinked, paramScriptName.GetAsciiString(), in_node);
      }
   }
   else if (sourceID == siShaderArrayParameterID)
   {
      ShaderArrayParameter paramArray(in_param.GetRef());

      // in certain cases, like 'lights' in the toon shader,
      // we have an array parameter in the shaderdef but the node input in Arnold is a string
      // let's itterate over the array and build a semicolon separated string of the objects
      int paramType = GetArnoldParameterType(in_node, in_param.GetScriptName().GetAsciiString());
      if (paramType == AI_TYPE_STRING)
      {
         const char* aiParamName = in_param.GetScriptName().GetAsciiString();
         CString paramValue = L"";

         for (LONG i=0; i<paramArray.GetCount(); i++)
         {
            Parameter theParam(paramArray[i]);
            CValue value = theParam.GetValue(in_frame);

            X3DObject xsiObj(value);
            if (xsiObj.IsValid())
            {
               // Add semicolon before every item except the first
               if (i > 0)
                  paramValue += L";";

               AtNode* objNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiObj, in_frame);
               if (objNode)
                  paramValue += CNodeUtilities().GetName(objNode);
               else
                  paramValue += xsiObj.GetFullName();
            }
         }
         CNodeSetter::SetString(in_node, aiParamName, paramValue.GetAsciiString());
      }
      else
      {
         AtArray *values(NULL);
         if (in_entryName == L"BooleanSwitch")
            values = AiArrayAllocate(paramArray.GetCount(), 1, AI_TYPE_BOOLEAN);
         else if (in_entryName == L"Color4Switch")
            values = AiArrayAllocate(paramArray.GetCount(), 1, AI_TYPE_RGBA);
         else if (in_entryName == L"IntegerSwitch")
            values = AiArrayAllocate(paramArray.GetCount(), 1, AI_TYPE_INT);
         else if (in_entryName == L"ScalarSwitch")
            values = AiArrayAllocate(paramArray.GetCount(), 1, AI_TYPE_FLOAT);
         else if (in_entryName == L"Vector3Switch")
            values = AiArrayAllocate(paramArray.GetCount(), 1, AI_TYPE_VECTOR);
   
         // entering here for the "values" parameter only. Together with "values", we also push the "index" array
         if (values)
         {
            AtArray *index  = AiArrayAllocate(paramArray.GetCount(), 1, AI_TYPE_INT);
            AiNodeSetArray(in_node, "values", values);
            AiNodeSetArray(in_node, "index", index);
         }
         else
         {
            // array type
            int paramType = GetArnoldParameterType(in_node, in_param.GetScriptName().GetAsciiString(), true);
            AtArray* entries = AiArrayAllocate(paramArray.GetCount(), 1, (uint8_t)paramType);
            AiNodeSetArray(in_node, in_param.GetScriptName().GetAsciiString(), entries);
         }
   
         // Iterate through all the parameters of the parameters array
         for (LONG i=0; i<paramArray.GetCount(); i++)
         {
            Parameter theParam(paramArray[i]);
            LoadShaderParameter(in_node, in_entryName, theParam, in_frame, in_ref, in_recursively, paramArray.GetScriptName(), i);
         }
      }
   }
   // ImageClips are not shaders so we need to parse it with a different method
   else if (sourceID == siImageClipID)
   {
      ImageClip2 toLoad(source);
      shaderLinked = LoadImageClip(toLoad, in_frame);
      AiNodeLink(shaderLinked, in_param.GetScriptName().GetAsciiString(), in_node);
   }
   else if (source.IsA(siParameterID) || sourceID == siCustomOperatorID)
   {
      Parameter paramSource(source);

      // Special case to handle FCurve parameters
      if (paramSource.GetValue().GetAsText() == L"FCurve") // sample the fcurve with fixed 200 steps
         AiNodeSetArray(in_node, in_param.GetScriptName().GetAsciiString(), GetFCurveArray((FCurve)paramSource.GetValue(), 200));
      else
      {
         CString paramName(in_param.GetScriptName());
         if (in_arrayElement != -1)
            paramName = in_arrayParamName;

         LoadParameterValue(in_node, in_entryName, paramName, paramSource, in_frame, in_arrayElement, in_ref);

         // #1906: image shader ? If requested, substitute the filename with its tx version, if found
         if (GetRenderOptions()->m_use_existing_tx_files && in_entryName == L"image" && paramName == L"filename")
         {
            const char *filename = AiNodeGetStr(in_node, "filename");
            filename = CPathTranslator::TranslatePath(filename, true);
            AiNodeSetStr(in_node, "filename", filename);
         }
      }
   }
   else if (sourceID == siFCurveID)
   {
      FCurve fc(source);
      Parameter paramSource(fc.GetParent());
      LoadParameterValue(in_node, in_entryName, in_param.GetScriptName(), paramSource, in_frame, -1, in_ref);
   }
   else if (sourceID == siExpressionID)
   {
      Expression expression(source);
      Parameter paramSource(expression.GetParent());
      LoadParameterValue(in_node, in_entryName, in_param.GetScriptName(), paramSource, in_frame, -1, in_ref);
   }
   else
      GetMessageQueue()->LogMsg(L"[sitoa] Can't load " + in_param.GetFullName() + L". It is connected with an incompatible source of type " + CString(sourceID) + L". Please contact the SItoA developers.", siErrorMsg);

   return status;
}


// Load the n-th element of the array parameters of the array switcher shaders. A dedicated function
// is needed, because the array has elements of struct type (index-value) that can't be parsed otherwise
//
// @param *in_node                the Arnold shader node
// @param in_param                the n-th Item parameter of the switcher shader's array
// @param in_frame                the frame time.
// @param in_arrayElement         the index of the array, so the n that in_param refers to
// @param in_ref                  the object owning the shader.
//
// @return                        CStatus::OK
//
CStatus LoadArraySwitcherParameter(AtNode *in_node, const Parameter &in_param, double in_frame, int in_arrayElement, CRef in_ref)
{
   AtArray *values = AiNodeGetArray(in_node, "values");
   AtArray *index  = AiNodeGetArray(in_node, "index");

   // this gets the Item container, with the index-value pair
   CParameterRefArray paramsArray = in_param.GetParameters();

   for (LONG i=0; i<paramsArray.GetCount(); i++)
   {
      Parameter p(paramsArray[i]);
      CString pName = p.GetName();
      // if the pair item is the index, store it in the index array
      if (pName == L"index")
      {
         int value = p.GetValue(in_frame);
         AiArraySetInt(index, in_arrayElement, value);
         continue;
      }

      // else load the value
      CRef source = GetParameterSource(p);
      siClassID sourceID = source.GetClassID();

      if (sourceID == siShaderID || sourceID == siTextureID)
      {
         Shader shader(source);
         AtNode *shaderLinked = LoadShader(shader, in_frame, in_ref, true);

         if (!shaderLinked)
            break;

         CString arrayLink = L"values[" + (CValue(in_arrayElement).GetAsText()) + L"]";
         AiNodeLink(shaderLinked, arrayLink.GetAsciiString(), in_node);
         continue;
      }

      CValue::DataType dataType = p.GetValueType();

      switch(dataType)
      {
         case CValue::siBool:
         {
            bool value = p.GetValue(in_frame);
            AiArraySetBool(values, in_arrayElement, value);
            break;
         }

         case CValue::siFloat:
         {
            float value = p.GetValue(in_frame);
            AiArraySetFlt(values, in_arrayElement, value);
            break;
         }

         case CValue::siInt4:
         {
            int value = p.GetValue(in_frame);
            AiArraySetInt(values, in_arrayElement, value);
            break;
         }

         // color4 and vector3, they come in as empty types (?!)
         case CValue::siEmpty:
         {
            CParameterRefArray paramsArray = p.GetParameters();
            LONG count = paramsArray.GetCount();
            if (count < 3) // protect against unsupported types
               break;

            Parameter p[4];
            p[0] = Parameter(paramsArray[0]);
            p[1] = Parameter(paramsArray[1]);
            p[2] = Parameter(paramsArray[2]);
            if (count == 4) // color4
            {
               p[3] = Parameter(paramsArray[3]);
               AtRGBA value = AtRGBA((float)p[0].GetValue(in_frame), (float)p[1].GetValue(in_frame), (float)p[2].GetValue(in_frame), (float)p[3].GetValue(in_frame));
               AiArraySetRGBA(values, in_arrayElement, value);
            }
            else // vector3
            {
               AtVector value = AtVector((float)p[0].GetValue(in_frame), (float)p[1].GetValue(in_frame), (float)p[2].GetValue(in_frame));
               AiArraySetVec(values, in_arrayElement, value);
            }

            break;
         }

         default:
            break;
      }
   }

   return CStatus::OK;
}


Shader GetShaderFromSource(const CRef &in_refCnxSrc)
{
   
   if (in_refCnxSrc.IsA(siShaderID))
      return in_refCnxSrc;

   // If the source is a parameter of any type, get the parent, and attempt to return it as a shader.
   if (in_refCnxSrc.IsA(siParameterID))
   {
      Parameter prm(in_refCnxSrc);
      return GetShaderFromSource(prm.GetParent());
   }

   // No idea.
   return Shader();
}


Shader GetConnectedShader(const Parameter& in_param)
{
   return GetShaderFromSource(GetParameterSource(in_param));
}
