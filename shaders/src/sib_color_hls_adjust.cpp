/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "color_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBColorHLSAdjustMethods);

enum SIBColorHLSAdjustParams
{
   p_color,
   p_master_h,
   p_master_l, 
   p_master_s, 
   p_red_h, 
   p_red_l,
   p_red_s,
   p_green_h,
   p_green_l, 
   p_green_s, 
   p_blue_h, 
   p_blue_l, 
   p_blue_s, 
   p_cyan_h, 
   p_cyan_l, 
   p_cyan_s, 
   p_yellow_h, 
   p_yellow_l, 
   p_yellow_s, 
   p_magenta_h,
   p_magenta_l,
   p_magenta_s
};

node_parameters
{
   AiParameterRGBA( "color"    , 1.0f , 1.0f , 1.0f, 1.0f );
   AiParameterFlt ( "master_h" , 0.0f );
   AiParameterFlt ( "master_l" , 0.0f );
   AiParameterFlt ( "master_s" , 0.0f );
   AiParameterFlt ( "red_h"    , 0.0f );
   AiParameterFlt ( "red_l"    , 0.0f );
   AiParameterFlt ( "red_s"    , 0.0f );
   AiParameterFlt ( "green_h"  , 0.0f );
   AiParameterFlt ( "green_l"  , 0.0f );
   AiParameterFlt ( "green_s"  , 0.0f );
   AiParameterFlt ( "blue_h"   , 0.0f );
   AiParameterFlt ( "blue_l"   , 0.0f );
   AiParameterFlt ( "blue_s"   , 0.0f );
   AiParameterFlt ( "cyan_h"   , 0.0f );
   AiParameterFlt ( "cyan_l"   , 0.0f );
   AiParameterFlt ( "cyan_s"   , 0.0f );
   AiParameterFlt ( "yellow_h" , 0.0f );
   AiParameterFlt ( "yellow_l" , 0.0f );
   AiParameterFlt ( "yellow_s" , 0.0f );
   AiParameterFlt ( "magenta_h", 0.0f );
   AiParameterFlt ( "magenta_l", 0.0f );
   AiParameterFlt ( "magenta_s", 0.0f );
}

namespace {

typedef struct 
{
   float master_h;
   float master_l;
   float master_s;
   float red_h;
   float red_l;
   float red_s;
   float green_h;
   float green_l;
   float green_s;
   float blue_h;
   float blue_l;
   float blue_s;
   float cyan_h;
   float cyan_l;
   float cyan_s;
   float yellow_h;
   float yellow_l;
   float yellow_s;
   float magenta_h;
   float magenta_l;
   float magenta_s;
} ShaderData;

}

node_initialize
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   data->master_h  = AiNodeGetFlt(node, "master_h");
   data->master_l  = AiNodeGetFlt(node, "master_l");
   data->master_s  = AiNodeGetFlt(node, "master_s");
   data->red_h     = AiNodeGetFlt(node, "red_h");
   data->red_l     = AiNodeGetFlt(node, "red_l");
   data->red_s     = AiNodeGetFlt(node, "red_s");
   data->green_h   = AiNodeGetFlt(node, "green_h");
   data->green_l   = AiNodeGetFlt(node, "green_l");
   data->green_s   = AiNodeGetFlt(node, "green_s");
   data->blue_h    = AiNodeGetFlt(node, "blue_h");
   data->blue_l    = AiNodeGetFlt(node, "blue_l");
   data->blue_s    = AiNodeGetFlt(node, "blue_s");
   data->cyan_h    = AiNodeGetFlt(node, "cyan_h");
   data->cyan_l    = AiNodeGetFlt(node, "cyan_l");
   data->cyan_s    = AiNodeGetFlt(node, "cyan_s");
   data->yellow_h  = AiNodeGetFlt(node, "yellow_h");
   data->yellow_l  = AiNodeGetFlt(node, "yellow_l");
   data->yellow_s  = AiNodeGetFlt(node, "yellow_s");
   data->magenta_h = AiNodeGetFlt(node, "magenta_h");
   data->magenta_l = AiNodeGetFlt(node, "magenta_l");
   data->magenta_s = AiNodeGetFlt(node, "magenta_s");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   AtRGBA result = AiShaderEvalParamRGBA(p_color);

   // r3D February 16 2011 This shader is not finished!

   // HLS Corrections
   if (data->master_h!=0.0f || data->master_s!=0.0f || data->master_l!=0.0f)
   {
      AtRGBA hls = RGBAtoHLS(result);
      // H
      hls.r += (hls.r*360.0f) + data->master_h;
      hls.r /= 360.0f;
      // L
      hls.b += (hls.b*200.0f) + data->master_s;
      hls.b /= 200.0f;
      hls.b = AiClamp(hls.b, 0.0f ,1.0f);
      // S
      hls.g += (hls.g*200.0f) + data->master_l;
      hls.g /= 200.0f;
      hls.g = AiClamp(hls.g, 0.0f, 1.0f);

      result = HLStoRGBA(hls);
   }

   sg->out.RGBA() = result;
}
