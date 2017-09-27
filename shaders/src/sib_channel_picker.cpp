/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "color_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBChannelPickerMethods);

#define COLORSPACE_RGBA             1
#define COLORSPACE_HLSA             2
#define COLORSPACE_HSVA             3            

#define CHANNEL_RGBA_RED            1
#define CHANNEL_RGBA_GREEN          2
#define CHANNEL_RGBA_BLUE           3
#define CHANNEL_RGBA_ALPHA          4
#define CHANNEL_RGBA_AVERAGE_RGB    5
#define CHANNEL_RGBA_AVERAGE_RGBA   6
#define CHANNEL_RGBA_MAX_RGB        7
#define CHANNEL_RGBA_MAX_RGBA       9
#define CHANNEL_RGBA_MIN_RGB        8
#define CHANNEL_RGBA_MIN_RGBA       10

#define CHANNEL_HLSA_HUE            1
#define CHANNEL_HLSA_LUMINANCE      2
#define CHANNEL_HLSA_SATURATION     3
#define CHANNEL_HLSA_ALPHA          4

#define CHANNEL_HSVA_HUE            1
#define CHANNEL_HSVA_SATURATION     2
#define CHANNEL_HSVA_VALUE          3
#define CHANNEL_HSVA_ALPHA          4

enum SIBChannelPickerParams
{
   p_input,
   p_colspace,
   p_channel_rgba,
   p_channel_hlsa,
   p_channel_hsva,
   p_invert,
   p_alpha_multiply
};

node_parameters
{
   AiParameterRGBA( "input"         , 1.0f , 1.0f , 1.0f, 1.0f );
   AiParameterInt ( "colspace"      , 1 );
   AiParameterInt ( "channel_rgba"  , 4 );
   AiParameterInt ( "channel_hlsa"  , 1 );
   AiParameterInt ( "channel_hsva"  , 1 );
   AiParameterBool( "invert"        , false );
   AiParameterBool( "alphamultiply" , false );
}

namespace {

typedef struct 
{
   int  colspace;
   int  channel_rgba;
   int  channel_hlsa;
   int  channel_hsva;
   bool invert, alphamultiply;
} ShaderData;

}

node_initialize
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   data->colspace      = AiNodeGetInt(node,  "colspace");
   data->channel_rgba  = AiNodeGetInt(node,  "channel_rgba");
   data->channel_hlsa  = AiNodeGetInt(node,  "channel_hlsa");
   data->channel_hsva  = AiNodeGetInt(node,  "channel_hsva");
   data->invert        = AiNodeGetBool(node, "invert");
   data->alphamultiply = AiNodeGetBool(node, "alphamultiply");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   float  result = 0.0f;
   AtRGBA tempColor;
    
   AtRGBA input = AiShaderEvalParamRGBA(p_input);

   if (data->colspace == COLORSPACE_HLSA)
   {
      tempColor = RGBAtoHLS(input);

      if (data->channel_hlsa == CHANNEL_HLSA_HUE)
         result = tempColor.r;
      else if (data->channel_hlsa == CHANNEL_HLSA_LUMINANCE)
         result = tempColor.g;
      else if (data->channel_hlsa == CHANNEL_HLSA_SATURATION)
         result = tempColor.b;
      else
         result = tempColor.a;
   }
   else if (data->colspace == COLORSPACE_HSVA)
   {
      tempColor = RGBAtoHSV(input);

      if (data->channel_hsva  == CHANNEL_HSVA_HUE)
         result = tempColor.r;
      else if (data->channel_hsva  == CHANNEL_HSVA_SATURATION)
         result = tempColor.g;
      else if (data->channel_hsva  == CHANNEL_HSVA_VALUE)
         result = tempColor.b;
      else
         result = tempColor.a;
   }
   else if (data->colspace == COLORSPACE_RGBA)
   {
      if (data->channel_rgba == CHANNEL_RGBA_RED)
         result = input.r;
      else if (data->channel_rgba == CHANNEL_RGBA_GREEN)
         result = input.g;
      else if (data->channel_rgba == CHANNEL_RGBA_BLUE)
         result = input.b;
      else if (data->channel_rgba == CHANNEL_RGBA_ALPHA)
         result = input.a;
      else if (data->channel_rgba == CHANNEL_RGBA_AVERAGE_RGB)
         result = (input.r + input.g + input.b) / 3.0f;
      else if (data->channel_rgba == CHANNEL_RGBA_AVERAGE_RGBA)
         result = (input.r + input.g + input.b + input.a) / 4.0f;
      else if (data->channel_rgba == CHANNEL_RGBA_MAX_RGB)
         result = AiColorMaxRGB(input);
      else if (data->channel_rgba == CHANNEL_RGBA_MAX_RGBA)
         result = AiMax(AiColorMaxRGB(input), input.a);
      else if (data->channel_rgba == CHANNEL_RGBA_MIN_RGB)
         result = AiMin(input.r, input.g, input.b);
      else if (data->channel_rgba == CHANNEL_RGBA_MIN_RGBA)
         result = AiMin(input.r, input.g, input.b, input.a);
    }

    sg->out.FLT() = data->invert ? 1.0f - result : result;
}
