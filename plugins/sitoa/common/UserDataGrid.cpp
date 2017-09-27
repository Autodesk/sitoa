/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/Tools.h"
#include "common/UserDataGrid.h"
#include "loader/PathTranslator.h"
#include "renderer/Renderer.h"

#include <cstring> 


// Parse an int value in the input string
//
// @param in_s:      The input string
// @param out_value: The returned int value
//
void ParseAttributeString(CString in_s, int *out_value)
{
   *out_value = atoi(in_s.GetAsciiString());
}


// Parse a bool value in the input string
//
// @param in_s:      The input string
// @param out_value: The returned bool value
//
void ParseAttributeString(CString in_s, bool *out_value)
{
   *out_value = in_s.IsEqualNoCase(L"true") || in_s == L"1";
}
 

// Parse up to 4 float value in the input string
//
// @param in_s:        The input string
// @param out_value_0: The 1st returned float value
// @param out_value_1: The 2nd returned float value (if != NULL)
// @param out_value_2: The 3rd returned float value (if != NULL)
// @param out_value_3: The 4th returned float value (if != NULL)
//
void ParseAttributeString(CString in_s, float *out_value_0, float *out_value_1, float *out_value_2, float *out_value_3)
{
   CStringArray split = in_s.Split(L" ");
   int count = (int)split.GetCount();

   *out_value_0 = 0.0f;
   if (out_value_1)
      *out_value_1 = 0.0f;
   if (out_value_2)
      *out_value_2 = 0.0f;
   if (out_value_3)
      *out_value_3 = 0.0f;

   if (count > 0)
      *out_value_0 = (float)atof(split[0].GetAsciiString());

   if (out_value_1)
   if (count > 1)
      *out_value_1 = (float)atof(split[1].GetAsciiString());

   if (out_value_2)
   if (count > 2)
      *out_value_2 = (float)atof(split[2].GetAsciiString());

   if (out_value_3)
   if (count > 3)
      *out_value_3 = (float)atof(split[3].GetAsciiString());
}


// Parse a matrix value in the input string
//
// @param in_s:      The input string
// @param out_value: The returned matrix value
//
void ParseAttributeString(CString in_s, AtMatrix &out_value)
{
   out_value = AiM4Identity();
   CStringArray split = in_s.Split(L" ");
   if (split.GetCount() > 15)
   {
      for (int i=0; i<4; i++)
         for (int j=0; j<4; j++)
            out_value[i][j] = (float)atof(split[i*4+j].GetAsciiString());
   }
}


// Set a node attribute data, getting the data from an input string
// The elements for structures must be separated by " ".
// For instance, to define a point, the correct syntax is
// "1 2 3"
//
// @param in_node:      The input node
// @param in_name:      The attribute name
// @param in_paramType: The attribute type
// @param in_s:         The string to read the data from
//
void SetUserDataOnNode(AtNode *in_node, CString in_name, int in_paramType, CPathString in_s)
{
   int      iValue;
   bool     b;
   float    f;
   AtRGBA   rgba;
   AtVector point;
   AtMatrix matrix;

   switch (in_paramType)
   {
      case 0: // INT
         ParseAttributeString(in_s, &iValue);
         CNodeSetter::SetInt(in_node, in_name.GetAsciiString(), iValue);
         break;
      case 1: // BOOL
         ParseAttributeString(in_s, &b);
         CNodeSetter::SetBoolean(in_node, in_name.GetAsciiString(), b);
         break;
      case 2: // FLOAT
         ParseAttributeString(in_s, &f);
         CNodeSetter::SetFloat(in_node, in_name.GetAsciiString(), f);
         break;
      case 3: // RGB
         ParseAttributeString(in_s, &rgba.r, &rgba.g, &rgba.b);
         CNodeSetter::SetRGB(in_node, in_name.GetAsciiString(), rgba.r, rgba.g, rgba.b);
         break;
      case 4: // RGBA
         ParseAttributeString(in_s, &rgba.r, &rgba.g, &rgba.b, &rgba.a);
         CNodeSetter::SetRGBA(in_node, in_name.GetAsciiString(), rgba.r, rgba.g, rgba.b, rgba.a);
         break;
      case 5: // VECTOR
         ParseAttributeString(in_s, &point.x, &point.y, &point.z);
         CNodeSetter::SetVector(in_node, in_name.GetAsciiString(), point.x, point.y, point.z);
         break;
      case 6: // VECTOR2:
         ParseAttributeString(in_s, &point.x, &point.y);
         CNodeSetter::SetVector2(in_node, in_name.GetAsciiString(), point.x, point.y);
         break;
      case 7: // STRING:
         CNodeSetter::SetString(in_node, in_name.GetAsciiString(), in_s.GetAsciiString());
         break;
      case 8: // MATRIX:
         ParseAttributeString(in_s, matrix);
         CNodeSetter::SetMatrix(in_node, in_name.GetAsciiString(), matrix);
         break;
      default:
         break;
   }
}


// Set a node array attribute data, getting the data from an input string
// The array elements must be separated by ",".
// For instance, to define an array of 3 points, the correct syntax is
// "1 2 3, 4 5 6, 7 8 9"
//
// @param in_node:      The input node
// @param in_name:      The attribute name
// @param in_paramType: The attribute type
// @param in_s:         The string to read the data from
//
void SetArrayUserDataOnNode(AtNode *in_node, CString in_name, int in_paramType, CPathString in_s)
{
   // we split the array elements by ","
   CStringArray split = in_s.Split(L",");
   int nbArrayElements = (int)split.GetCount();

   if (nbArrayElements == 0)
      return;

   uint8_t aiType[9] = { AI_TYPE_INT, AI_TYPE_BOOLEAN, AI_TYPE_FLOAT, AI_TYPE_RGB, AI_TYPE_RGBA, 
                         AI_TYPE_VECTOR, AI_TYPE_VECTOR2, AI_TYPE_STRING, AI_TYPE_MATRIX };

   AtArray *dataArray = AiArrayAllocate(nbArrayElements, 1, aiType[in_paramType]);

   int      i, iValue;
   bool     b;
   float    f;
   AtRGB    rgb;
   AtRGBA   rgba;
   AtVector vec;
   AtVector point;
   AtVector2 point2;
   AtMatrix matrix;

   switch (in_paramType)
   {
      case 0: // INT:
         for (i=0; i<nbArrayElements; i++)
         {
            ParseAttributeString(split[i], &iValue);
            AiArraySetInt(dataArray, i, iValue);
         }
         break;

      case 1: // BOOL
         for (i=0; i<nbArrayElements; i++)
         {
            ParseAttributeString(split[i], &b);
            AiArraySetBool(dataArray, i, b);
         }
         break;

      case 2: // FLOAT
         for (i=0; i<nbArrayElements; i++)
         {
            ParseAttributeString(split[i], &f);
            AiArraySetFlt(dataArray, i, f);
         }
         break;

      case 3: // RGB
         for (i=0; i<nbArrayElements; i++)
         {
            ParseAttributeString(split[i], &rgb.r, &rgb.g, &rgb.b);
            AiArraySetRGB(dataArray, i, rgb);
         }
         break;

      case 4: // RGBA
         for (i=0; i<nbArrayElements; i++)
         {
            ParseAttributeString(split[i], &rgba.r, &rgba.g, &rgba.b, &rgba.a);
            AiArraySetRGBA(dataArray, i, rgba);
         }
         break;

      case 5: // VEC
         for (i=0; i<nbArrayElements; i++)
         {
            ParseAttributeString(split[i], &vec.x, &vec.y, &vec.z);
            AiArraySetVec(dataArray, i, vec);
         }
         break;

      case 6: // VEC2
         for (i=0; i<nbArrayElements; i++)
         {
            ParseAttributeString(split[i], &point2.x, &point2.y);
            AiArraySetVec2(dataArray, i, point2);
         }
         break;

      case 7: // STRING
         for (i=0; i<nbArrayElements; i++)
            AiArraySetStr(dataArray, i, split[i].GetAsciiString());
         break;

      case 8: // MATRIX
         for (i=0; i<nbArrayElements; i++)
         {
            ParseAttributeString(split[i], matrix);
            AiArraySetMtx(dataArray, i, matrix);
         }
         break;

      default:
          break;
  }

  AiNodeSetArray(in_node, in_name.GetAsciiString(), dataArray);
}


// Exports the data grid as user data
//
// @param in_node:          the input node
// @param in_grid:          the grid object containing the data
// @param in_resolveTokens: true if the data values must be token-resolved
// @param in_frame:         the frame time
//
void ExportUserDataGrid(AtNode *in_node, GridData &in_grid, bool in_resolveTokens, double in_frame)
{
   CValueArray gridData = in_grid.GetData();

   CStringArray paramTypeTable, exrMetadata;

   paramTypeTable.Add(L"INT");
   paramTypeTable.Add(L"BOOL");
   paramTypeTable.Add(L"FLOAT");
   paramTypeTable.Add(L"RGB");
   paramTypeTable.Add(L"RGBA");
   paramTypeTable.Add(L"VECTOR");
   paramTypeTable.Add(L"VECTOR2");
   paramTypeTable.Add(L"STRING");
   paramTypeTable.Add(L"MATRIX");

   for (LONG rowIndex=0; rowIndex<in_grid.GetRowCount(); rowIndex++)
   {
      CValueArray rowValues = in_grid.GetRowValues(rowIndex);
      CString paramName = rowValues[0].GetAsText();
      if (paramName.IsEmpty()) // no valid name
         continue;

      int paramStructure = (int)rowValues[1];
      int paramType      = (int)rowValues[2];
      CPathString paramValue = (CString)rowValues[3];

      if (paramValue.IsEmpty()) // no value set
         continue;

      if (in_resolveTokens)
         paramValue.ResolveTokensInPlace(in_frame);

      CString decl(L"constant ");
      if (paramStructure == 1)
         decl+= L"ARRAY ";
      decl+= paramTypeTable[paramType];

      AiNodeDeclare(in_node, paramName.GetAsciiString(), decl.GetAsciiString());

      if (paramStructure == 0) // single value
         SetUserDataOnNode(in_node, paramName, paramType, paramValue);
      else // array
         SetArrayUserDataOnNode(in_node, paramName, paramType, paramValue);
   }
}

