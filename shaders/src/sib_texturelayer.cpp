/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "ai.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBTextureLayerMethods);

#define TextureLayerHardLight(_r, _c, _b)                                                             \
{ (_r).r = (float) (((_c).r<0.5f) ? (2.0f*(_c).r*(_b).r) : 1.0-2.0f*(1.0f-(_c).r)*(1.0f-(_b).r));   \
  (_r).g = (float) (((_c).g<0.5f) ? (2.0f*(_c).g*(_b).g) : 1.0-2.0f*(1.0f-(_c).g)*(1.0f-(_b).g));   \
  (_r).b = (float) (((_c).b<0.5f) ? (2.0f*(_c).b*(_b).b) : 1.0-2.0f*(1.0f-(_c).b)*(1.0f-(_b).b)); }

enum SIBTextureLayerParams 
{
   p_color,
   p_weight,
   p_mode,
   p_mute,
   p_basecolor,
   p_solo,
   p_invert,
   p_scale,
   p_mask,
   p_maskmode,
   p_maskthreshold,
   p_invertmask,
   p_colorsource,
   p_ignorecoloralpha,
   p_colorpremulted,
   p_invertcoloralpha,
   p_alphacolor,
   p_red,
   p_green,
   p_blue,
   p_alpha
};

#define MODE_OVER             0
#define MODE_IN               1
#define MODE_OUT              2
#define MODE_PLUS             3
#define MODE_PLUSCLAMP        4
#define MODE_MULTIPLY         5
#define MODE_MULTIPLYCLAMP    6
#define MODE_DIFFERENCE       7
#define MODE_DARKEN           8
#define MODE_LIGHTEN          9
#define MODE_HARDLIGHT        10
#define MODE_SOFTLIGHT        11
#define MODE_SCREEN           12
#define MODE_OVERLAY          13
#define MODE_BLEND            14

#define MASK_NOMASK                    0
#define MASK_USE_MASK_CONNECTION       1
#define MASK_LAYER_ALPHA_AS_MASK       2
#define MASK_LAYER_INTENSITY_AS_MASK   3
#define MASK_LAYER_THRESHOLD_AS_MASK   4

#define COLORSOURCE_USE_COLOR          0
#define COLORSOURCE_USE_ALPHA          1

namespace {

typedef struct 
{ 
   float weight;
   int   mode;
   bool  mute;
   bool  solo;
   bool  invert;
   float scale;
   int   maskmode;
   float maskthreshold;
   bool  invertmask;
   int   colorsource;
   bool  ignorecoloralpha;
   bool  colorpremulted;
   bool  invertcoloralpha;
   bool  alphacolor;
   float red, green, blue, alpha;

   bool  colorlinked;
   float weightTimesScale; // weight * scale
} ShaderData; 

}

static void PrepareLayerColor(AtRGBA *layerColor, bool useAlphaColor, AtRGBA *colorOrig);
static void ApplyMask(float *channel, float mask, int maskmode, float maskthreshold, bool invert, AtRGBA *layerColor);

node_parameters
{
   AiParameterRGBA( "color"               , 0.7f, 0.7f, 0.7f, 1.0f );
   AiParameterFlt ( "weight"              , 1.0f );
   AiParameterInt ( "mode"                , MODE_OVER );
   AiParameterBool( "mute"                , false );
   AiParameterRGBA( "basecolor"           , 0.0f, 0.0f, 0.0f ,1.0f  );
   AiParameterBool( "solo"                , false );
   AiParameterBool( "invert"              , false );
   AiParameterFlt ( "scale"               , 1.0f );
   AiParameterFlt ( "mask"                , 1.0f );
   AiParameterInt ( "maskmode"            , MASK_NOMASK );
   AiParameterFlt ( "maskthreshold"       , 0.0f );
   AiParameterBool( "invertmask"          , false );
   AiParameterInt ( "colorsource"         , COLORSOURCE_USE_COLOR );
   AiParameterBool( "ignorecoloralpha"    , false );
   AiParameterBool( "colorpremulted"      , false );
   AiParameterBool( "invertcoloralpha"    , false );
   AiParameterBool( "alphacolor"          , false );
   AiParameterFlt ( "red"                 , 0.0f );
   AiParameterFlt ( "green"               , 0.0f );
   AiParameterFlt ( "blue"                , 0.0f );
   AiParameterFlt ( "alpha"               , 0.0f );
}

node_initialize
{
   ShaderData* data = (ShaderData*)AiMalloc(sizeof(ShaderData)); 
   AiNodeSetLocalData(node, data); 
}

node_update
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);
   // 1-time only eval of params
   data->weight           = AiNodeGetFlt (node, "weight");
   data->mode             = AiNodeGetInt (node, "mode");
   data->mute             = AiNodeGetBool(node, "mute");
   data->solo             = AiNodeGetBool(node, "solo");
   data->invert           = AiNodeGetBool(node, "invert");
   data->scale            = AiNodeGetFlt (node, "scale");
   data->maskmode         = AiNodeGetInt (node, "maskmode");
   data->maskthreshold    = AiNodeGetFlt (node, "maskthreshold");
   data->invertmask       = AiNodeGetBool(node, "invertmask");
   data->colorsource      = AiNodeGetInt (node, "colorsource");
   data->ignorecoloralpha = AiNodeGetBool(node, "ignorecoloralpha");
   data->colorpremulted   = AiNodeGetBool(node, "colorpremulted");
   data->invertcoloralpha = AiNodeGetBool(node, "invertcoloralpha");
   data->alphacolor       = AiNodeGetBool(node, "alphacolor");
   data->red              = AiNodeGetFlt (node, "red");
   data->green            = AiNodeGetFlt (node, "green");
   data->blue             = AiNodeGetFlt (node, "blue");
   data->alpha            = AiNodeGetFlt (node, "alpha");
   // accellerators
   data->colorlinked      = AiNodeIsLinked(node, "color");
   data->weightTimesScale = data->weight * data->scale;
}


node_finish 
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node); 
   AiFree(data); 
}

shader_evaluate
{
   ShaderData* data = (ShaderData*)AiNodeGetLocalData(node);

   AtRGBA    color, basecolor, layerColor;
   AtRGB     out_color = AI_RGB_BLACK;
   float     weight = 1.0f;

   basecolor = AiShaderEvalParamRGBA(p_basecolor);

   // Mute?
   if (data->mute)
   {
      out_color.r = basecolor.r;
      out_color.g = basecolor.g;
      out_color.b = basecolor.b;
   }
   else
   {
      // If color is not linked, color will be defined in red, green and  blue parameters
      if (data->colorlinked)
         color = AiShaderEvalParamRGBA(p_color);
      else
         color = AtRGBA(data->red, data->green, data->blue, data->alpha);

      // Premultiplied Alpha
      if (data->colorpremulted && color.a != 0.0f)
      {      
         color.r /= color.a;
         color.g /= color.a;
         color.b /= color.a;
      }
      
      if (data->ignorecoloralpha)
         color.a = 1.0f;

      // Prepare alpha
      if (data->invertcoloralpha)
         color.a = 1.0f - color.a;
    
      if (data->colorsource == COLORSOURCE_USE_ALPHA)
         color.r = color.g = color.b = color.a;

      // Invert
      if (data->invert)
      {
         color.r = 1 - color.r;
         color.g = 1 - color.g;
         color.b = 1 - color.b;
      }
  
      // Prepare Color Layer
      PrepareLayerColor(&layerColor, data->alphacolor, &color);
      // Apply Mask
      ApplyMask(&layerColor.a, AiShaderEvalParamFlt(p_mask), data->maskmode, data->maskthreshold, data->invertmask, &color);

      AtRGB layerColorRGB = AtRGB(layerColor.r, layerColor.g, layerColor.b);
      // Premultiplying colors with alpha (as XSI doc says for its formulas)
      layerColorRGB = layerColorRGB * layerColor.a;

      AtRGB basecolorRGB = AtRGB(basecolor.r, basecolor.g, basecolor.b);

      basecolorRGB = basecolorRGB * basecolor.a;
      
      switch (data->mode)
      {
         case MODE_OVER:
            out_color = layerColorRGB + basecolorRGB * (1.0f - layerColor.a);
            break;
         case MODE_IN:    
            out_color = layerColorRGB * basecolor.a;
            break;
         case MODE_OUT:   
            out_color = layerColorRGB * (1.0f - basecolor.a);
            break;
         case MODE_PLUS:  
         case MODE_PLUSCLAMP:  
            out_color = layerColorRGB + basecolorRGB;
            break;
         case MODE_MULTIPLY:    
         case MODE_MULTIPLYCLAMP:  
            out_color = layerColorRGB * basecolorRGB;
            break;
         case MODE_DIFFERENCE:  
            out_color = layerColorRGB - basecolorRGB;
            out_color = AiColorABS(out_color);
            break;
         case MODE_DARKEN:  
            out_color.r = AiMin(layerColorRGB.r, basecolorRGB.r);
            out_color.g = AiMin(layerColorRGB.g, basecolorRGB.g);
            out_color.b = AiMin(layerColorRGB.b, basecolorRGB.b);
            break;
         case MODE_LIGHTEN:
            out_color.r = AiMax(layerColorRGB.r, basecolorRGB.r);
            out_color.g = AiMax(layerColorRGB.g, basecolorRGB.g);
            out_color.b = AiMax(layerColorRGB.b, basecolorRGB.b);
            break;
         case MODE_HARDLIGHT:
            TextureLayerHardLight(out_color, layerColorRGB, basecolorRGB);
            break;
         case MODE_SOFTLIGHT:
         {
            AtRGB base = basecolorRGB * 0.5f;
            base += 0.25f;
            TextureLayerHardLight(out_color, layerColorRGB, basecolorRGB);
            break;
         }
         case MODE_SCREEN:
         {
            AtRGB mult;
            out_color = layerColorRGB + basecolorRGB;
            mult = layerColorRGB * basecolorRGB;
            out_color = out_color - mult;
            break;
         }
         case MODE_OVERLAY:
            TextureLayerHardLight(out_color, basecolorRGB, layerColorRGB);
            break;
         case MODE_BLEND:
            out_color.r = layerColorRGB.r + (basecolorRGB.r * (1.0f - layerColorRGB.r));
            out_color.g = layerColorRGB.g + (basecolorRGB.g * (1.0f - layerColorRGB.g));
            out_color.b = layerColorRGB.b + (basecolorRGB.b * (1.0f - layerColorRGB.b));
            break;
      }
      
      // Port values
      weight = data->weightTimesScale;

      // Apply weight
      out_color.r = out_color.r * weight + (basecolorRGB.r * (1.0f - weight));
      out_color.g = out_color.g * weight + (basecolorRGB.g * (1.0f - weight));
      out_color.b = out_color.b * weight + (basecolorRGB.b * (1.0f - weight));
   } 
   
   sg->out.RGBA().r = out_color.r;
   sg->out.RGBA().g = out_color.g;
   sg->out.RGBA().b = out_color.b;
   sg->out.RGBA().a = basecolor.a * weight + (basecolor.a * (1.0f - weight));
}

// Applying Mask
static void ApplyMask(float *channel, float mask, int maskmode, float maskthreshold, bool invert, AtRGBA *layerColor)
{
   switch(maskmode)
   {
      case MASK_NOMASK:
      {
         mask = 1.0f;
         break;
      }
      case MASK_USE_MASK_CONNECTION:
      {
         break;
      }
      case MASK_LAYER_ALPHA_AS_MASK:
      {
         mask = layerColor->a;
         break;
      }
      case MASK_LAYER_INTENSITY_AS_MASK:
      {
         // Differs a bit from XSI/MRay Shader
         mask = (0.299f*layerColor->r + 0.587f*layerColor->g + 0.114f*layerColor->b);
         break;
      }
      case MASK_LAYER_THRESHOLD_AS_MASK:
      {
         mask = ((0.299f*layerColor->r + 0.587f*layerColor->g + 0.114f*layerColor->b) > maskthreshold)?1.0f:0.0f;
         break;
      }
   }

   if(invert && maskmode!=MASK_NOMASK)
      mask = 1.0f - mask;

   *channel *= mask;
}


// Preparing Color
static void PrepareLayerColor(AtRGBA *layerColor, bool useAlphaColor, AtRGBA *colorOrig)
{
   if (!useAlphaColor)
      *layerColor = *colorOrig;
   else 
   {
      layerColor->r = layerColor->g = layerColor->b = colorOrig->a;
      layerColor->a = 1.0f;
   }
}
