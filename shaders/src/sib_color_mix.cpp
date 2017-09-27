/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBColorMixMethods);

#define COLORMIX_MODE_ALPHA              15     // Not Implemented
#define COLORMIX_MODE_DECAL              14
#define COLORMIX_MODE_RGB_MODULATE       16
#define COLORMIX_MODE_MIX                1
#define COLORMIX_MODE_RGB_INTENSITY      6
#define COLORMIX_MODE_MULTIPLY           4
#define COLORMIX_MODE_MULTIPLY_BOUND     5
#define COLORMIX_MODE_ADD                2
#define COLORMIX_MODE_ADD_COMPENSATE     0
#define COLORMIX_MODE_ADD_BOUND          3
#define COLORMIX_MODE_LIGHTER            8
#define COLORMIX_MODE_DARKER             7
#define COLORMIX_MODE_SOFT_LIGHT         13
#define COLORMIX_MODE_HARD_LIGHT         10
#define COLORMIX_MODE_DIFFERENCE         9
#define COLORMIX_MODE_HUE_OFFSET         11
#define COLORMIX_MODE_SCREEN             12

inline float _SCL(float r, float s1, float e1, float s2, float e2)
{
   return (((r - s1/(e1-s1)) * (e2-s2)) + s2);
}

inline AtRGBA ColorMixCompensate(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r =  color_1.r * (1.0f - color_2.r * weight.r) + color_2.r * weight.r;
   out.g =  color_1.g * (1.0f - color_2.g * weight.g) + color_2.g * weight.g;
   out.b =  color_1.b * (1.0f - color_2.b * weight.b) + color_2.b * weight.b;
   out.a =  color_1.a * (1.0f - color_2.a * weight.a) + color_2.a * weight.a;
   return out;
}

inline AtRGBA ColorMixMix(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r =  color_1.r * (1.0f - weight.r) + color_2.r * weight.r;
   out.g =  color_1.g * (1.0f - weight.g) + color_2.g * weight.g;
   out.b =  color_1.b * (1.0f - weight.b) + color_2.b * weight.b;
   out.a =  color_1.a * (1.0f - weight.a) + color_2.a * weight.a;
   return out;
}

inline AtRGBA ColorMixAdd(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r =  color_1.r + (color_2.r * weight.r);
   out.g =  color_1.g + (color_2.g * weight.g);
   out.b =  color_1.b + (color_2.b * weight.b);
   out.a =  color_1.a + (color_2.a * weight.a);
   return out;
}

inline AtRGBA ColorMixAddBound(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r =  AiClamp(color_1.r + (color_2.r * weight.r), 0.0f, 1.0f);
   out.g =  AiClamp(color_1.g + (color_2.g * weight.g), 0.0f, 1.0f);
   out.b =  AiClamp(color_1.b + (color_2.b * weight.b), 0.0f, 1.0f);
   out.a =  AiClamp(color_1.a + (color_2.a * weight.a), 0.0f, 1.0f);
   return out;
}

inline AtRGBA ColorMixMultiply(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r =  color_1.r * (color_2.r * weight.r);
   out.g =  color_1.g * (color_2.g * weight.g);
   out.b =  color_1.b * (color_2.b * weight.b);
   out.a =  color_1.a * (color_2.a * weight.a);
   return out;
}

inline AtRGBA ColorMixMultiplyBound(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r =  AiClamp(color_1.r * (color_2.r * weight.r), 0.0f, 1.0f);
   out.g =  AiClamp(color_1.g * (color_2.g * weight.g), 0.0f, 1.0f);
   out.b =  AiClamp(color_1.b * (color_2.b * weight.b), 0.0f, 1.0f);
   out.a =  AiClamp(color_1.a * (color_2.a * weight.a), 0.0f, 1.0f);
   return out;
}

inline AtRGBA ColorMixDarker(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r =  AiMin(color_1.r, color_2.r * weight.r);
   out.g =  AiMin(color_1.g, color_2.g * weight.g);
   out.b =  AiMin(color_1.b, color_2.b * weight.b);
   out.a =  AiMin(color_1.a, color_2.a * weight.a);
   return out;
}

inline AtRGBA ColorMixLighter(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r =  AiMax(color_1.r, color_2.r * weight.r);
   out.g =  AiMax(color_1.g, color_2.g * weight.g);
   out.b =  AiMax(color_1.b, color_2.b * weight.b);
   out.a =  AiMax(color_1.a, color_2.a * weight.a);
   return out;
}

inline AtRGBA ColorMixDifference(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r =  std::abs(color_1.r - color_2.r * weight.r);
   out.g =  std::abs(color_1.g - color_2.g * weight.g);
   out.b =  std::abs(color_1.b - color_2.b * weight.b);
   out.a =  std::abs(color_1.a - color_2.a * weight.a);
   return out;
}

inline AtRGBA ColorMixHardLight(const AtRGBA& color_1, const AtRGBA& color_2, const AtRGBA&  weight)
{
   AtRGBA out;
   out.r = (color_2.r < 0.5f) ? (2.0f * weight.r * color_1.r) : 1.0f - 2.0f * (1.0f - weight.r) * (1.0f - color_1.r);
   out.g = (color_2.g < 0.5f) ? (2.0f * weight.g * color_1.g) : 1.0f - 2.0f * (1.0f - weight.g) * (1.0f - color_1.g);
   out.b = (color_2.b < 0.5f) ? (2.0f * weight.b * color_1.b) : 1.0f - 2.0f * (1.0f - weight.b) * (1.0f - color_1.b);
   out.a = (color_2.a < 0.5f) ? (2.0f * weight.a * color_1.a) : 1.0f - 2.0f * (1.0f - weight.a) * (1.0f - color_1.a);
   return out;
}

inline AtRGBA ColorMixScreen(const AtRGBA& color_1, const AtRGBA& color_2)
{
   AtRGBA out;
   out.r = 1.0f - ((1.0f - color_1.r) * (1.0f - color_2.r));
   out.g = 1.0f - ((1.0f - color_1.g) * (1.0f - color_2.g));
   out.b = 1.0f - ((1.0f - color_1.b) * (1.0f - color_2.b));
   out.a = 1.0f - ((1.0f - color_1.a) * (1.0f - color_2.a));
   return out;
}

inline AtRGBA ColorMixSoftLight(const AtRGBA& color_1, const AtRGBA& color_2)
{
   AtRGBA out;
   out.r = (color_2.r < 0.5f) ? 2.0f * _SCL(color_1.r, 0.0f, 1.0f, 0.25f, 0.75f) * color_2.r : 1.0f - ( 2.0f * (1.0f - _SCL(color_1.r, 0.0f, 1.0f, 0.25f, 0.75f) * (1.0f - color_2.r)));
   out.g = (color_2.g < 0.5f) ? 2.0f * _SCL(color_1.g, 0.0f, 1.0f, 0.25f, 0.75f) * color_2.g : 1.0f - ( 2.0f * (1.0f - _SCL(color_1.g, 0.0f, 1.0f, 0.25f, 0.75f) * (1.0f - color_2.g)));
   out.b = (color_2.b < 0.5f) ? 2.0f * _SCL(color_1.b, 0.0f, 1.0f, 0.25f, 0.75f) * color_2.b : 1.0f - ( 2.0f * (1.0f - _SCL(color_1.b, 0.0f, 1.0f, 0.25f, 0.75f) * (1.0f - color_2.b)));
   out.a = (color_2.a < 0.5f) ? 2.0f * _SCL(color_1.a, 0.0f, 1.0f, 0.25f, 0.75f) * color_2.a : 1.0f - ( 2.0f * (1.0f - _SCL(color_1.a, 0.0f, 1.0f, 0.25f, 0.75f) * (1.0f - color_2.a)));
   return out;
}

inline bool RgbaIsSmall(const AtRGBA& rgba, float epsilon = AI_EPSILON)
{
   return std::abs(rgba.r) < epsilon && std::abs(rgba.g) < epsilon && std::abs(rgba.b) < epsilon && std::abs(rgba.a) < epsilon;
}

inline AtRGBA RgbaScale(const AtRGBA& color, float k)
{
   AtRGBA out;
   out.r = color.r * k;
   out.g = color.g * k;
   out.b = color.b * k;
   out.a = color.a * k;
   return out;
}

inline AtRGBA RgbaColorMult(const AtRGBA& color_1, const AtRGBA& color_2)
{
   AtRGBA out;
   out.r = color_1.r * color_2.r;
   out.g = color_1.g * color_2.g;
   out.b = color_1.b * color_2.b;
   out.a = color_1.a * color_2.a;
   return out;
}

enum SIBColorMixParams
{
   p_mixersize,
   p_base_color,
   p_inuse1,
   p_color1,
   p_weight1,
   p_mode1,
   p_alpha1,
   p_inuse2,
   p_color2,
   p_weight2,
   p_mode2,
   p_alpha2,
   p_inuse3,
   p_color3,
   p_weight3,
   p_mode3,
   p_alpha3,
   p_inuse4,
   p_color4,
   p_weight4,
   p_mode4,
   p_alpha4,
   p_inuse5,
   p_color5,
   p_weight5,
   p_mode5,
   p_alpha5,
   p_inuse6,
   p_color6,
   p_weight6,
   p_mode6,
   p_alpha6,
   p_inuse7,
   p_color7,
   p_weight7,
   p_mode7,
   p_alpha7
};

node_parameters
{
   AiParameterInt ( "mixersize" , 2 );
   AiParameterRGBA( "base_color", 1.0, 1.0, 1.0, 1.0 );
   AiParameterBool( "inuse1"    , false );
   AiParameterRGBA( "color1"    , 1.0, 1.0, 1.0, 1.0 );
   AiParameterRGBA( "weight1"   , 0.5, 0.5, 0.5, 0.5);
   AiParameterInt ( "mode1"     , COLORMIX_MODE_MIX );
   AiParameterBool( "alpha1"    , false );
   AiParameterBool( "inuse2"    , false );
   AiParameterRGBA( "color2"    , 1.0, 1.0, 1.0, 1.0 );
   AiParameterRGBA( "weight2"   , 0.5, 0.5, 0.5, 0.5);
   AiParameterInt ( "mode2"     , COLORMIX_MODE_MIX );
   AiParameterBool( "alpha2"    , false );
   AiParameterBool( "inuse3"    , false );
   AiParameterRGBA( "color3"    , 1.0, 1.0, 1.0, 1.0 );
   AiParameterRGBA( "weight3"   , 0.5, 0.5, 0.5, 0.5);
   AiParameterInt ( "mode3"     , COLORMIX_MODE_MIX );
   AiParameterBool( "alpha3"    , false );
   AiParameterBool( "inuse4"    , false );
   AiParameterRGBA( "color4"    , 1.0, 1.0, 1.0, 1.0 );
   AiParameterRGBA( "weight4"   , 0.5, 0.5, 0.5, 0.5);
   AiParameterInt ( "mode4"     , COLORMIX_MODE_MIX );
   AiParameterBool( "alpha4"    , false );
   AiParameterBool( "inuse5"    , false );
   AiParameterRGBA( "color5"    , 1.0, 1.0, 1.0, 1.0 );
   AiParameterRGBA( "weight5"   , 0.5, 0.5, 0.5, 0.5);
   AiParameterInt ( "mode5"     , COLORMIX_MODE_MIX );
   AiParameterBool( "alpha5"    , false );
   AiParameterBool( "inuse6"    , false );
   AiParameterRGBA( "color6"    , 1.0, 1.0, 1.0, 1.0 );
   AiParameterRGBA( "weight6"   , 0.5, 0.5, 0.5, 0.5);
   AiParameterInt ( "mode6"     , COLORMIX_MODE_MIX );
   AiParameterBool( "alpha6"    , false );
   AiParameterBool( "inuse7"    , false );
   AiParameterRGBA( "color7"    , 1.0, 1.0, 1.0, 1.0 );
   AiParameterRGBA( "weight7"   , 0.5, 0.5, 0.5, 0.5);
   AiParameterInt ( "mode7"     , COLORMIX_MODE_MIX );
   AiParameterBool( "alpha7"    , false );
}

namespace {

typedef struct 
{
   int  mixersize;
   bool inuse[7], alpha[7];
   int  mode[7];
} ShaderData;

}

node_initialize 
{
   AiNodeSetLocalData(node, AiMalloc(sizeof(ShaderData)));
}

node_update 
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   data->mixersize = AiNodeGetInt(node, "mixersize");

   data->inuse[0] = AiNodeGetBool(node, "inuse1");
   data->alpha[0] = AiNodeGetBool(node, "alpha1");
   data->mode[0]  = AiNodeGetInt(node, "mode1");

   data->inuse[1] = AiNodeGetBool(node, "inuse2");
   data->alpha[1] = AiNodeGetBool(node, "alpha2");
   data->mode[1]  = AiNodeGetInt(node, "mode2");

   data->inuse[2] = AiNodeGetBool(node, "inuse3");
   data->alpha[2] = AiNodeGetBool(node, "alpha3");
   data->mode[2]  = AiNodeGetInt(node, "mode3");

   data->inuse[3] = AiNodeGetBool(node, "inuse4");
   data->alpha[3] = AiNodeGetBool(node, "alpha4");
   data->mode[3]  = AiNodeGetInt(node, "mode4");

   data->inuse[4] = AiNodeGetBool(node, "inuse5");
   data->alpha[4] = AiNodeGetBool(node, "alpha5");
   data->mode[4]  = AiNodeGetInt(node, "mode5");

   data->inuse[5] = AiNodeGetBool(node, "inuse6");
   data->alpha[5] = AiNodeGetBool(node, "alpha6");
   data->mode[5]  = AiNodeGetInt(node, "mode6");

   data->inuse[6] = AiNodeGetBool(node, "inuse7");
   data->alpha[6] = AiNodeGetBool(node, "alpha7");
   data->mode[6]  = AiNodeGetInt(node, "mode7");
}

node_finish 
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   AtRGBA  base_color, color, weight;
   bool    alpha, inuse;
   int     mode;

   base_color = AiShaderEvalParamRGBA(p_base_color);
   AtRGBA result = base_color;

   for (int i = 0; i < data->mixersize-1; i++)
   {
      inuse = data->inuse[i];
      weight = AiShaderEvalParamRGBA(4 + (5*i));

      if (inuse && !RgbaIsSmall(weight))
      {
         color = AiShaderEvalParamRGBA(3 + (5*i));
         mode = data->mode[i];
         alpha = data->alpha[i];

         if (alpha)
            weight = RgbaScale(weight, color.a);

         switch(mode)
         {
            case COLORMIX_MODE_ADD_COMPENSATE:
            {
               result = ColorMixCompensate(base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_MIX:
            {
               result = ColorMixMix(base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_ADD:
            {
               result = ColorMixAdd(base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_ADD_BOUND:
            {
               result = ColorMixAddBound(base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_MULTIPLY:
            {
               result = ColorMixMultiply(base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_MULTIPLY_BOUND:
            {
               result = ColorMixMultiplyBound(base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_RGB_INTENSITY:
            {
               float tmp = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
               AtRGBA tmpColor = RgbaScale(weight, tmp);
               result = ColorMixMix(base_color, color, tmpColor);
               break;
            }
            case COLORMIX_MODE_DARKER:
            {
               result = ColorMixDarker(base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_LIGHTER:
            {
               result = ColorMixLighter(base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_DIFFERENCE:
            {
               result = ColorMixDifference(base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_HARD_LIGHT:
            {
               AtRGBA tmpColor = RgbaColorMult(color, weight);
               result = ColorMixHardLight(base_color, color, tmpColor);
               break;
            }
            case COLORMIX_MODE_HUE_OFFSET:
            {
               AtRGBA tmpColor = RgbaColorMult(color, weight);
               result = ColorMixHardLight(base_color, base_color, tmpColor);
               break;
            }
            case COLORMIX_MODE_SCREEN:
            {
               AtRGBA tmpColor = RgbaColorMult(color, weight);
               result = ColorMixScreen(base_color, tmpColor);
               break;
            }
            case COLORMIX_MODE_SOFT_LIGHT:
            {
               AtRGBA tmpColor = RgbaColorMult(color, weight);
               result = ColorMixSoftLight(base_color, tmpColor);
               break;
            }
            case COLORMIX_MODE_DECAL:
            {
               float tmp = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
               if (tmp >= AI_EPSILON)
                  result = ColorMixMix( base_color, color, weight);
               break;
            }
            case COLORMIX_MODE_ALPHA:
            {
               result.r = base_color.r * (1.0f - weight.r * color.a) + color.r * weight.r * color.a;
               result.g = base_color.g * (1.0f - weight.g * color.a) + color.g * weight.g * color.a;
               result.b = base_color.b * (1.0f - weight.b * color.a) + color.b * weight.b * color.a;
               result.a = base_color.a * (1.0f - weight.a) + color.a * weight.a;
               break;
            }
            case COLORMIX_MODE_RGB_MODULATE:
            {
               float tmp = 0.299f * color.r + 0.587f * color.g + 0.114f * color.b;
               if (tmp >= AI_EPSILON)
               {
                  result.r *= color.r * weight.r;
                  result.g *= color.g * weight.g;
                  result.b *= color.b * weight.b;
               }
               break;
            }
         }

         base_color = result;
      }
   }

   sg->out.RGBA() = result;
}
