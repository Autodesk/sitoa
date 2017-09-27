/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "color_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBColorBalanceMethods);

enum SIBColorBalanceParams
{
   p_color,
   p_shadows_red,
   p_shadows_green,
   p_shadows_blue,
   p_midtones_red,
   p_midtones_green,
   p_midtones_blue,
   p_highlights_red,
   p_highlights_green,
   p_highlights_blue,
   p_preserve_value
};

node_parameters
{
   AiParameterRGBA ( "color"             , 1.0f , 1.0f , 1.0f, 1.0f );
   AiParameterFlt  ( "shadows_red"       , 0.0f );
   AiParameterFlt  ( "shadows_green"     , 0.0f );
   AiParameterFlt  ( "shadows_blue"      , 0.0f );
   AiParameterFlt  ( "midtones_red"      , 0.0f );
   AiParameterFlt  ( "midtones_green"    , 0.0f );
   AiParameterFlt  ( "midtones_blue"     , 0.0f );
   AiParameterFlt  ( "highlights_red"    , 0.0f );
   AiParameterFlt  ( "highlights_green"  , 0.0f );
   AiParameterFlt  ( "highlights_blue"   , 0.0f );
   AiParameterBool ( "preserve_value"    , false );          
}

namespace {

typedef struct 
{
   float shadows_red;
   float shadows_green;
   float shadows_blue;
   float midtones_red;
   float midtones_green;
   float midtones_blue;
   float highlights_red;
   float highlights_green;
   float highlights_blue;
   bool  preserve_value;
} ShaderData;

}

node_initialize
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->shadows_red      = AiNodeGetFlt(node, "shadows_red")      / 255.0f;
   data->shadows_green    = AiNodeGetFlt(node, "shadows_green")    / 255.0f;
   data->shadows_blue     = AiNodeGetFlt(node, "shadows_blue")     / 255.0f;
   data->midtones_red     = AiNodeGetFlt(node, "midtones_red")     / 255.0f;
   data->midtones_green   = AiNodeGetFlt(node, "midtones_green")   / 255.0f;
   data->midtones_blue    = AiNodeGetFlt(node, "midtones_blue")    / 255.0f;
   data->highlights_red   = AiNodeGetFlt(node, "highlights_red")   / 255.0f;
   data->highlights_green = AiNodeGetFlt(node, "highlights_green") / 255.0f;
   data->highlights_blue  = AiNodeGetFlt(node, "highlights_blue")  / 255.0f;
   data->preserve_value   = AiNodeGetBool(node, "preserve_value");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   AtRGBA result;
    
   AtRGBA color = AiShaderEvalParamRGBA(p_color);

   result.r = BalanceChannel(color.r, data->shadows_red,   data->midtones_red,   data->highlights_red);
   result.g = BalanceChannel(color.g, data->shadows_green, data->midtones_green, data->highlights_green);
   result.b = BalanceChannel(color.b, data->shadows_blue,  data->midtones_blue,  data->highlights_blue);
   result.a = color.a;

   if (data->preserve_value) 
   {
      // Get the luminosity of the original input color and substitute that in place of the balanced luminosity.
      AtRGBA hls = RGBAtoHLS(result);

      float max = AiMax( AiMax( color.r, color.g ), color.b);
      float min = AiMin( AiMin( color.r, color.g ), color.g);

      hls.g = (max + min) * 0.5f;

      result = HLStoRGBA(hls);
   }
   
   sg->out.RGBA() = result;
}
