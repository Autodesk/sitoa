/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "ai.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBColorInRangeMethods);

enum SIBColorInRangeParams 
{
   p_input,
   p_min_thresh,
   p_max_thresh,
   p_alpha,
   p_negate
};

node_parameters
{
   AiParameterRGB ( "input"     , 0.0f, 1.0f, 0.0f );
   AiParameterRGB ( "min_thresh", -0.05f, 0.95f, -0.05f );
   AiParameterRGB ( "max_thresh", 0.05f, 1.05f, 0.05f );
   AiParameterBool( "alpha"     , false );                    // Not implemented
   AiParameterBool( "negate"    , false );
}

namespace {

typedef struct 
{
   bool negate;
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->negate = AiNodeGetBool(node, "negate");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   AtRGB input = AiShaderEvalParamRGB(p_input);
   AtRGB min_thresh = AiShaderEvalParamRGB(p_min_thresh);
   AtRGB max_thresh = AiShaderEvalParamRGB(p_max_thresh);

   bool result = true;
   if (input.r < min_thresh.r || input.g < min_thresh.g || input.b < min_thresh.b) 
      result = false;
   else if (input.r > max_thresh.r || input.g > max_thresh.g || input.b > max_thresh.b) 
      result = false;

   sg->out.BOOL() = data->negate ? !result : result;
}
