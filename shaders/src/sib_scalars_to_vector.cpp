/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBScalarsToVectorMethods);

#define CHANNEL_NO_CONTRIBUTION  0
#define CHANNEL_USE_INPUT_X      1
#define CHANNEL_USE_INPUT_Y      2
#define CHANNEL_USE_INPUT_Z      3

#define MODE_ADD                 0
#define MODE_SUBSTRACT           1
#define MODE_MULTIPLY            2
#define MODE_REPLACE             3

enum SIBScalarsToVectorParams
{
   p_inputX,
   p_inputY,
   p_inputZ,
   p_modeX,   
   p_modeY,   
   p_modeZ,   
   p_math_op
};

node_parameters
{
   AiParameterFlt( "inputX" , 0.0f );
   AiParameterFlt( "inputY" , 0.0f );
   AiParameterFlt( "inputZ" , 0.0f );
   AiParameterInt( "modeX"  , 1    );
   AiParameterInt( "modeY"  , 2    );
   AiParameterInt( "modeZ"  , 3    );   
   AiParameterInt( "math_op", 0    );   // "This parameter determines how two or more channels redirected on the same destination should be handled"
}

namespace {

typedef struct 
{
   int modeX, modeY, modeZ, math_op;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->modeX   = AiNodeGetInt(node, "modeX");
   data->modeY   = AiNodeGetInt(node, "modeY");
   data->modeZ   = AiNodeGetInt(node, "modeZ");
   data->math_op = AiNodeGetInt(node, "math_op");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   AtVector result = AI_V3_ZERO;

   float inputX = AiShaderEvalParamFlt(p_inputX);
   float inputY = AiShaderEvalParamFlt(p_inputY);
   float inputZ = AiShaderEvalParamFlt(p_inputZ);

   if (data->modeX == CHANNEL_USE_INPUT_X)
   {
      switch(data->math_op)
      {
         case MODE_ADD:          result.x = inputX + result.x;    break;
         case MODE_SUBSTRACT:    result.x = inputX - result.x;    break;
         case MODE_MULTIPLY:     result.x = inputX * result.x;    break;
         case MODE_REPLACE:      result.x = inputX;               break;
      }
   }
   else if (data->modeX == CHANNEL_USE_INPUT_Y)
   {
      switch(data->math_op)
      {
         case MODE_ADD:          result.y = inputX + result.y;    break;
         case MODE_SUBSTRACT:    result.y = inputX - result.y;    break;
         case MODE_MULTIPLY:     result.y = inputX * result.y;    break;
         case MODE_REPLACE:      result.y = inputX;               break;
      }
   }
   else if (data->modeX == CHANNEL_USE_INPUT_Z) 
   {
      switch(data->math_op)
      {
         case MODE_ADD:          result.z = inputX + result.z;    break;
         case MODE_SUBSTRACT:    result.z = inputX - result.z;    break;
         case MODE_MULTIPLY:     result.z = inputX * result.z;    break;
         case MODE_REPLACE:      result.z = inputX;               break;
      }
   }
   
   if (data->modeY == CHANNEL_USE_INPUT_X)
   {
      switch(data->math_op)
      {
         case MODE_ADD:          result.x = inputY + result.x;    break;
         case MODE_SUBSTRACT:    result.x = inputY - result.x;    break;
         case MODE_MULTIPLY:     result.x = inputY * result.x;    break;
         case MODE_REPLACE:      result.x = inputY;               break;
      }
   }
   else if (data->modeY == CHANNEL_USE_INPUT_Y)
   {
      switch(data->math_op)
      {
         case MODE_ADD:          result.y = inputY + result.y;    break;
         case MODE_SUBSTRACT:    result.y = inputY - result.y;    break;
         case MODE_MULTIPLY:     result.y = inputY * result.y;    break;
         case MODE_REPLACE:      result.y = inputY;               break;
      }
   }
   else if (data->modeY == CHANNEL_USE_INPUT_Z) 
   {
      switch(data->math_op)
      {
         case MODE_ADD:          result.z = inputY + result.z;    break;
         case MODE_SUBSTRACT:    result.z = inputY - result.z;    break;
         case MODE_MULTIPLY:     result.z = inputY * result.z;    break;
         case MODE_REPLACE:      result.z = inputY;               break;
      }
   }

   if (data->modeZ == CHANNEL_USE_INPUT_X)
   {
      switch(data->math_op)
      {
         case MODE_ADD:          result.x = inputZ + result.x;    break;
         case MODE_SUBSTRACT:    result.x = inputZ - result.x;    break;
         case MODE_MULTIPLY:     result.x = inputZ * result.x;    break;
         case MODE_REPLACE:      result.x = inputZ;               break;
      }
   }
   else if (data->modeZ == CHANNEL_USE_INPUT_Y)
   {
      switch(data->math_op)
      {
         case MODE_ADD:          result.y = inputZ + result.y;    break;
         case MODE_SUBSTRACT:    result.y = inputZ - result.y;    break;
         case MODE_MULTIPLY:     result.y = inputZ * result.y;    break;
         case MODE_REPLACE:      result.y = inputZ;               break;
      }
   }
   else if (data->modeZ == CHANNEL_USE_INPUT_Z) 
   {
      switch(data->math_op)
      {
         case MODE_ADD:          result.z = inputZ + result.z;    break;
         case MODE_SUBSTRACT:    result.z = inputZ - result.z;    break;
         case MODE_MULTIPLY:     result.z = inputZ * result.z;    break;
         case MODE_REPLACE:      result.z = inputZ;               break;
      }
   }
   
   sg->out.VEC() = result;
}
