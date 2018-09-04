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
#include "renderer/Renderer.h"

#include <xsi_expression.h>
#include <xsi_inputport.h>
#include <xsi_fcurve.h>

#include <cstring> 


// Loads into Arnold node given parameter its evaluate value
//
// Will ask for the type of the given parameter and set its value into
// Arnold node with the correct method.
//
// @param *in_node         Arnold node where we will set the parameter value.
// @param in_entryName     the node entry name
// @param in_paramName     The name of the parameter where the method will load the value.
// @param in_param         Parameter object of XSI with the value to load.
// @param in_frame         Frame to evaluate.
// @param in_arrayElement  Set the array element (-1 if the parameter is not an element of an array)
// @param in_ref           CRef of the owning object
//
// @return                 CStatus::OK success / CStatus::Fail is returned in case of failure.
//
CStatus LoadParameterValue(AtNode *in_node, const CString &in_entryName, const CString &in_paramName, const Parameter &in_param, double in_frame, int arrayElement, CRef in_ref)
{
   CStatus status;
   int aiParamType=AI_TYPE_NONE;

   // lights exceptions for saving backward compatibility for #1534 (Rename bounces and bounce_factor parameters to max_bounces and indirect)
   bool lightException(false);

   bool isLight = CNodeUtilities().GetEntryType(in_node) == L"light";

   if (isLight)
   {
      if (in_paramName == L"bounces") // spdl param name, Arnold light's attribute is now called max_bounces
      {
         aiParamType = GetArnoldParameterType(in_node, "max_bounces", true);
         lightException = true;
      }
      else if (in_paramName == L"bounce_factor") // spdl param name, Arnold light's attribute is now called indirect
      {
         aiParamType = GetArnoldParameterType(in_node, "indirect", true);
         lightException = true;
      }
   }

   // Get node param type
   const char* aiParamName = in_paramName.GetAsciiString();
   if (!lightException) // regular parameters, where the Softimage parameter name matches the Arnold node parameter name
      aiParamType = GetArnoldParameterType(in_node, aiParamName, true);

   // We have to force the unlink if the parameter was previously linked
   // because doing an AiNodeSet* will not unlink it and will ignore the new value
   // But only do this if the parameter is not an array
   if (arrayElement == -1 && aiParamType != AI_TYPE_NONE)
      AiNodeUnlink(in_node, aiParamName);

   if (arrayElement != -1)
   {
      if (in_paramName == L"values")
      if (in_entryName == L"BooleanSwitch" || 
          in_entryName == L"Color4Switch" ||
          in_entryName == L"IntegerSwitch" ||
          in_entryName == L"ScalarSwitch" ||
          in_entryName == L"Vector3Switch")
         return LoadArraySwitcherParameter(in_node, in_param, in_frame, arrayElement, in_ref);
   }

   // Compound param (with subcomponents)
   CParameterRefArray paramsArray = in_param.GetParameters();

   switch (aiParamType)
   {
      case AI_TYPE_RGB:
      {
         Parameter r, g, b;

         // If we detect we have more than rgba data in compound parameter (because expressions)
         // we will get what parameter we have to evaluate. 
         // (to avoid do extra-querys if there arent expressions)
         if (paramsArray.GetCount() > 4)
         {
            int idx = 0;
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), r);
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), g);
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), b);
         }
         else
         {
            r = Parameter(paramsArray[0]);
            g = Parameter(paramsArray[1]);
            b = Parameter(paramsArray[2]);
         }

         AtRGB theColor = AtRGB((float)r.GetValue(in_frame), (float)g.GetValue(in_frame), (float)b.GetValue(in_frame));

         if (arrayElement == -1)
            CNodeSetter::SetRGB(in_node, aiParamName,theColor.r, theColor.g, theColor.b);
         else
            AiArraySetRGB(AiNodeGetArray(in_node, aiParamName), arrayElement, theColor);

         break;
      }
      case AI_TYPE_RGBA:
      {
         Parameter r, g, b, a;
         
         // If we detect we have more than rgba data in compound parameter (because expressions)
         // we will get what parameter we have to evaluate. 
         // (to avoid do extra-querys if there arent expressions)
         if (paramsArray.GetCount() > 4)
         {
            int idx = 0;
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), r);
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), g);
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), b);
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), a);
         }
         else
         {
            r = Parameter(paramsArray[0]);
            g = Parameter(paramsArray[1]);
            b = Parameter(paramsArray[2]);
            a = Parameter(paramsArray[3]);
         }

         AtRGBA theColor = AtRGBA((float)r.GetValue(in_frame), (float)g.GetValue(in_frame), (float)b.GetValue(in_frame), (float)a.GetValue(in_frame));

         if (arrayElement == -1)
            CNodeSetter::SetRGBA(in_node, aiParamName, theColor.r, theColor.g, theColor.b, theColor.a);
         else
            AiArraySetRGBA(AiNodeGetArray(in_node, aiParamName), arrayElement, theColor);

         break;
      }
      case AI_TYPE_VECTOR:
      {
         Parameter x, y, z;
         
         // If we detect we have more than xyz data in compound parameter (because expressions)
         // we will get what parameter we have to evaluate. 
         // (to avoid do extra-querys if there arent expressions)
         if (paramsArray.GetCount() > 3)
         {
            int idx = 0;
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), x);
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), y);
            idx += GetEvaluatedExprParameter(Parameter(paramsArray[idx]), z);
         }
         else
         {
            x = Parameter(paramsArray[0]);
            y = Parameter(paramsArray[1]);
            z = Parameter(paramsArray[2]);
         }

         AtVector theVector = AtVector((float)x.GetValue(in_frame), (float)y.GetValue(in_frame), (float)z.GetValue(in_frame));

         if (arrayElement == -1)
            CNodeSetter::SetVector(in_node, aiParamName, theVector.x, theVector.y, theVector.z);
         else
            AiArraySetVec(AiNodeGetArray(in_node, aiParamName), arrayElement, theVector);

         break;
      }
      case AI_TYPE_MATRIX:
      {
         AtMatrix m;
         // Softimage >= 2011 gives us a matrix
         for (unsigned int i =0; i < (unsigned int)paramsArray.GetCount(); ++i)
            m[i/4][i%4] = (float)Parameter(paramsArray[i]).GetValue(in_frame);;

         if (arrayElement == -1)
            CNodeSetter::SetMatrix(in_node, aiParamName, m);
         else
            AiArraySetMtx(AiNodeGetArray(in_node, aiParamName), arrayElement, m);
         break;
      }

      case AI_TYPE_VECTOR2:
         break;

      case AI_TYPE_BYTE:
      {
         uint8_t theByte = (uint8_t)in_param.GetValue(in_frame);

         if (arrayElement == -1)
            CNodeSetter::SetByte(in_node, aiParamName, theByte);
         else
            AiArraySetByte(AiNodeGetArray(in_node, aiParamName), arrayElement, theByte);

         break;
      }

      case AI_TYPE_INT:
      {
         int theInt = (int)in_param.GetValue(in_frame);

         if (arrayElement == -1)
         {
            if (lightException) // unique possible exeption for a parameter of type int. Be careful if we add further in the future
               CNodeSetter::SetInt(in_node, "max_bounces", theInt);
            else
               CNodeSetter::SetInt(in_node, aiParamName, theInt);
         }
         else
            AiArraySetInt(AiNodeGetArray(in_node, aiParamName), arrayElement, theInt);

         break;
      }

      case AI_TYPE_UINT:
      {
         unsigned int theUInt = (unsigned int) ((int)in_param.GetValue(in_frame));

         if (arrayElement == -1)
            CNodeSetter::SetUInt(in_node, aiParamName, theUInt);
         else
            AiArraySetUInt(AiNodeGetArray(in_node, aiParamName), arrayElement, theUInt);

         break;
      }

      case AI_TYPE_BOOLEAN:
      {
         bool theBool = (bool)in_param.GetValue(in_frame);

         if (arrayElement == -1)
            CNodeSetter::SetBoolean(in_node, aiParamName, theBool);
         else
            AiArraySetBool(AiNodeGetArray(in_node, aiParamName), arrayElement, theBool);

         break;
      }

      case AI_TYPE_FLOAT:
      {
         if (arrayElement == -1)
         {
            if (lightException) // unique possible exeption for a parameter of type float. Be careful if we add further in the future
               CNodeSetter::SetFloat(in_node, "indirect", (float)in_param.GetValue(in_frame));
            else
               CNodeSetter::SetFloat(in_node, aiParamName, (float)in_param.GetValue(in_frame));
         }
         else
         {
            AtArray* paramArray = AiNodeGetArray(in_node, aiParamName);
            float theFloat = (float)in_param.GetValue(in_frame);
            AiArraySetFlt(paramArray, arrayElement, theFloat);
         }
         break;
      }

      case AI_TYPE_ENUM:
      case AI_TYPE_STRING:
      {
         if (arrayElement == -1)
         {
            CString paramValue;
            // #1361
            // Also, for #1423, let's test that the in_ref is valid. If not, it means this is being called
            // by IPR (and the shader could belong to several objects and have different instance values).
            // Not much we can do in this case, except reading the param value instead of the instance value
            if (in_paramName == L"tspace_id" && in_param.HasInstanceValue() && in_ref.IsValid())
              paramValue = in_param.GetInstanceValue(in_ref, false);
            else
            {
               CValue value = in_param.GetValue(in_frame);
               paramValue = value.GetAsText();

               // allow tokens for the ies filename of the photometric light or for the image's filename. 
               bool resolveTokens = isLight && AiNodeIs(in_node, ATSTRING::photometric_light) && in_paramName == L"filename";
               if (!resolveTokens)
                  resolveTokens = AiNodeIs(in_node, ATSTRING::image) && in_paramName == L"filename";

               if (resolveTokens)
               {
                  // special case #1666. If using a light profile parameter instead of a string, we must get the parameter subparam
                  CParameterRefArray parArray = in_param.GetParameters();
                  if (parArray.GetCount() > 0)
                     paramValue = (Parameter(parArray[0])).GetValue().GetAsText();

                  // resolve the tokens
                  CPathString resolvedPath(paramValue);
                  resolvedPath.ResolveTokensInPlace(in_frame);
                  paramValue = resolvedPath;
               }

               // Translate a CRef to SItoA name
               // https://github.com/Autodesk/sitoa/issues/24
               if (in_param.GetValueType() == CValue::siEmpty)  // CRef comes in as siEmpty ?!
               {
                  // this is just the same code as the one below
                  X3DObject xsiObj(value);
                  if (xsiObj.IsValid())
                  {
                     AtNode* objNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiObj, in_frame);
                     if (objNode)
                        paramValue = CNodeUtilities().GetName(objNode);
                     else
                        paramValue = xsiObj.GetFullName();
                  }
               }
            }

            CNodeSetter::SetString(in_node, aiParamName, paramValue.GetAsciiString());
         }
         else
         {
            // Luis patch for #1387
            CValue value = in_param.GetValue(in_frame); 
            CString paramValue = value.GetAsText(); 

            // Is it a 3DObject, for instance a light for the incidence shader ? We must export its fullname
            X3DObject xsiObj(value); 
            if (xsiObj.IsValid()) 
            { 
               AtNode* objNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiObj, in_frame); 
               if (objNode) 
                  paramValue = CNodeUtilities().GetName(objNode); 
               else 
                  paramValue = xsiObj.GetFullName(); 
            } 

            AiArraySetStr(AiNodeGetArray(in_node, aiParamName), arrayElement, paramValue.GetAsciiString()); 
         }
         break;
      }

      case AI_TYPE_NODE:
         if (in_entryName == L"camera_projection" && in_paramName == L"camera")
         {
            Camera xsiCamera(in_param.GetValue(in_frame)); 
            if (xsiCamera.IsValid()) 
            { 
               AtNode* cameraNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiCamera, in_frame);
               CNodeSetter::SetPointer(in_node, aiParamName, cameraNode);
            }
         }
         break;

      case AI_TYPE_UNDEFINED:
         break;
   }

   return status;
}


// Gets the Arnold Parameter Type
//
// Will ask to Arnold for the type of a parameter of a specific node
//
// @param in_node                        Arnold node owner of the parameter.
// @param in_paramName                   the name of the parameter
// @param in_checkInsideArrayParameter   if true, if the parameter is an array, return its type
//
// @return the AI_TYPE of the parameter.
//
int GetArnoldParameterType(AtNode *in_node, const char* in_paramName, bool in_checkInsideArrayParameter)
{
   const AtParamEntry* paramEntry = AiNodeEntryLookUpParameter(AiNodeGetNodeEntry(in_node), in_paramName);
   int paramType = AiParamGetType(paramEntry);

   if (paramType == AI_TYPE_ARRAY && in_checkInsideArrayParameter)
   {
      // Return the type of the array
      const AtParamValue* defaultValue =  AiParamGetDefault(paramEntry);
      paramType = AiArrayGetType(defaultValue->ARRAY());
   }

   return paramType;
}


unsigned int GetEvaluatedExprParameter(const Parameter &in_parameter, Parameter &out_parameter)
{
   CRef paramSource = in_parameter.GetSource();

   if (paramSource.IsValid())
   {
      Expression expr(paramSource);
      CRefArray list(expr.GetInputPorts());
      InputPort input(list[0]);
      out_parameter = Parameter(input.GetTarget());

      // Returning the index sum for compounds parameters
      // expression parameters increase the number of subparameters of a compound.
      return expr.GetParameters().GetCount() + 1;
   }
   else
   {
      out_parameter = in_parameter;
      return 1;
   }
}


// Converts an fcurve to a float array
//
// @param in_fc         The softimage fcurve.
// @param in_nbKeys     The number of (uniform) samples to sample the curve with.
//
// @return           The AtArray pointer.
//
AtArray* GetFCurveArray(const FCurve &in_fc, int in_nbKeys)
{
   int headerSize = 3; //the extrapolation token and the 2 derivatives

   // the fcurve keys
   LONG fcNbKeys = in_fc.GetNumKeys(); 
   if (fcNbKeys < 2) //constant curve, push just one couple in the array
      in_nbKeys = 1;

   float startTime = (float)in_fc.GetKeyTime(0);
   float endTime   = (float)in_fc.GetKeyTime(fcNbKeys-1);

   // another case of constant curve degeneration
   if (startTime == endTime)
      in_nbKeys = 1;

   // for each key, x and y. Plus
   // one float for extrapolation type
   // one float for starting and ending derivative
   int arraySize = in_nbKeys*2 + headerSize;

   AtArray *a = AiArrayAllocate(arraySize, 1, AI_TYPE_FLOAT);

   float extrapolation = (float)in_fc.GetExtrapolation();
   AiArraySetFlt(a, 0, extrapolation);

   double y;
   if (in_nbKeys == 1)
   {
      y = in_fc.Eval(0.0); //evaluate it at time 0
      AiArraySetFlt(a, headerSize, 0.0f);
      AiArraySetFlt(a, headerSize + 1, (float)y);
      return a;
   }

   float t, time, dx, dy, derivative;

   FCurveKey k, k1;

   // Let's compute the start/end derivatives
   // We must check for the start key interpolation type, and the end-1 key interpolation type.
   // If cubic , we take the start key's LeftTan and the end key's RightTan
   // Else, if linear, we subtract (start+1)-start and (end-1)-(end-2)
   // Else, if constant, the derivative is 0 (flat)

   // first key
   k = in_fc.GetKeyAtIndex(0);

   switch(k.GetInterpolation())
   {
      case siConstantKeyInterpolation:
         derivative = 0.0f;
         break;
      case siLinearKeyInterpolation:
         k1 = in_fc.GetKeyAtIndex(1);
         dx = (float)(k1.GetTime() - k.GetTime());
         if (fabs(dx) < AI_EPSILON)
            derivative = 0.0f;
         else
            derivative = ((float)k1.GetValue() - (float)k.GetValue()) / dx;
         break;
      case siCubicKeyInterpolation:
         dx = -(float)k.GetLeftTanX();
         dy = -(float)k.GetLeftTanY();
         derivative = fabs(dx) < AI_EPSILON ? 100000.0f : dy/dx;
         break;
      default:
         derivative = 0.0f;
   }
   AiArraySetFlt(a, 1, derivative);

   // last key -1
   k = in_fc.GetKeyAtIndex(fcNbKeys-2);
   switch(k.GetInterpolation())
   {
      case siConstantKeyInterpolation:
         derivative = 0.0f;
         break;
      case siLinearKeyInterpolation:
         k1 = in_fc.GetKeyAtIndex(fcNbKeys-1);
         dx = (float)(k1.GetTime() - k.GetTime());
         if (fabs(dx) < AI_EPSILON)
            derivative = 0.0f;
         else
            derivative = ((float)k1.GetValue() - (float)k.GetValue()) / dx;
         break;
      case siCubicKeyInterpolation:
         k1 = in_fc.GetKeyAtIndex(fcNbKeys-1);
         dx = (float)k1.GetRightTanX();
         dy = (float)k1.GetRightTanY();
         derivative = fabs(dx) < AI_EPSILON ? 100000.0f : dy/dx;
         break;
      default:
         derivative = 0.0f;
   }
   AiArraySetFlt(a, 2, derivative);
   // done with derivatives

   for (int i=0; i<in_nbKeys; i++)
   {
      t = (float)i / ((float)in_nbKeys - 1.0f);
	   time = startTime + (endTime - startTime) * t; // evaluation time
      y = in_fc.Eval(time); // fcurve value

      // set time and value. 
      // Note that saving the times is redundant, we could have saved only the start/end time once.
      // However, it's not much of a waste for 100 samples, this coding will useful if we decide to go 
      // for actual analytic interpolation of bezier curves
      AiArraySetFlt(a, headerSize + i*2, (float)time);
      AiArraySetFlt(a, headerSize + i*2 + 1, (float)y);
   }
   return a;
}


// Sample a fcurve into a float array. The array is made of pairs of time,value.
// If the fcurve is linear, return the pairs at the fcurve key times.
// Else, sample it uniformly with in_nbKeys samples
//
// @param in_fc         The softimage fcurve
// @param in_nbKeys     The number of samples
//
// @return              The array of time,value pairs
//
AtArray* GetFCurveRawArray(const FCurve &in_fc, const int in_nbKeys)
{
   LONG nbKeys = in_nbKeys;
   LONG fcNbKeys = in_fc.GetNumKeys();

   CTime cTime = in_fc.GetKeyTime(0);
   float startTime = (float)cTime.GetTime();
   cTime = in_fc.GetKeyTime(fcNbKeys-1);
   float endTime = (float)cTime.GetTime();
   siFCurveInterpolation curveInterpolation = in_fc.GetInterpolation();

   // the fcurve keys
   if (fcNbKeys < 2 || startTime == endTime) //constant curve, push just one couple in the array
      nbKeys = 1;
   else
   {
      if (curveInterpolation == siLinearInterpolation)
         nbKeys = fcNbKeys;
      else
         nbKeys = in_nbKeys;
   }

   // for each key, simply the time and value
   AtArray *a = AiArrayAllocate(nbKeys * 2, 1, AI_TYPE_FLOAT);

   if (nbKeys == 1)
   {
      float y = (float)in_fc.Eval(0.0); //evaluate it at time 0
      AiArraySetFlt(a, 0, 0.0f);
      AiArraySetFlt(a, 1, y);
      return a;
   }

   if (curveInterpolation == siLinearInterpolation) // just read the keys
   {
      for (LONG i=0; i<nbKeys; i++)
      {
         FCurveKey k = in_fc.GetKeyAtIndex(i);
         AiArraySetFlt(a, i*2, (float)k.GetTime());
         AiArraySetFlt(a, i*2 + 1, (float)k.GetValue());
      }
      return a;
   }

   float t, time, y;
   for (int i=0; i<nbKeys; i++)
   {
      t = (float)i / ((float)nbKeys - 1.0f);
	   time = startTime + (endTime - startTime) * t; // evaluation time
      y = (float)in_fc.Eval(time); // fcurve value
      AiArraySetFlt(a, i*2, time);
      AiArraySetFlt(a, i*2 + 1, y);
   }

   return a;
}

