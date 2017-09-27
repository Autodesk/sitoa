/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include <cstdio>
#include <string.h>
#include "shader_utils.h"
#include <vector>
#include <algorithm>
using namespace std;

AI_SHADER_NODE_EXPORT_METHODS(txt2d_gradient_v2Methods);

enum eGradientType
{
   VERTICAL = 0,
   HORIZONTAL,
   RADIAL,
   SCOPE,
   DIAGONAL_DOWN,
   DIAGONAL_UP
};

enum eInputType
{
   SCALAR_INPUT = 0,
   VECTOR_INPUT,
   VECTOR_X,
   VECTOR_Y,
   VECTOR_Z
};

enum eInterpolation
{
   LINEAR = 0,
   CUBIC
};

enum txt2d_gradient_v2Params 
{
   p_gradient_type,        
   p_invert,
   p_clip,
   p_enable_alpha_gradient,
   p_range_min,
   p_range_max,
   p_rgba_interpolation,
   p_color1,
   p_pos_color1,
   p_mid_color1,
   p_color2,
   p_pos_color2,
   p_mid_color2,
   p_color3,
   p_pos_color3,
   p_mid_color3,
   p_color4,
   p_pos_color4,
   p_mid_color4,
   p_color5,
   p_pos_color5,
   p_mid_color5,
   p_color6,
   p_pos_color6,
   p_mid_color6,
   p_color7,
   p_pos_color7,
   p_mid_color7,
   p_color8,
   p_pos_color8,
   p_mid_color8,
   p_alpha_interpolation,
   p_alpha1,
   p_pos_alpha1,
   p_mid_alpha1,
   p_alpha2,
   p_pos_alpha2,
   p_mid_alpha2,
   p_alpha3,
   p_pos_alpha3,
   p_mid_alpha3,
   p_alpha4,
   p_pos_alpha4,
   p_mid_alpha4,
   p_alpha5,
   p_pos_alpha5,
   p_mid_alpha5,
   p_alpha6,
   p_pos_alpha6,
   p_mid_alpha6,
   p_alpha7,
   p_pos_alpha7,
   p_mid_alpha7,
   p_alpha8,
   p_pos_alpha8,
   p_mid_alpha8,
   p_tspace_id,
   p_repeats,
   p_alt_x,
   p_alt_y,
   p_alt_z,
   p_min,
   p_max,
   p_step,
   p_factor,
   p_torus_u,
   p_torus_v,
   p_bump_inuse,
   p_alpha,
   p_alpha_output,
   p_alpha_factor,                         
};

node_parameters
{
   AiParameterInt ( "gradient_type"        , 0 );
   AiParameterBool( "invert"               , false );
   AiParameterBool( "clip"                 , false );
   AiParameterBool( "enable_alpha_gradient", false );
   AiParameterFlt ( "range_min"            , 0.0f );
   AiParameterFlt ( "range_max"            , 1.0f );
   AiParameterInt ( "rgba_interpolation"   , 1 );
   AiParameterRGBA( "color1"               , 1.0f, 0.0f, 0.0f, 1.0 );  
   AiParameterFlt ( "pos_color1"           , 0.0f );  
   AiParameterFlt ( "mid_color1"           , 0.5f );  
   AiParameterRGBA( "color2"               , 1.0f, 0.0f, 1.0f, 1.0 );  
   AiParameterFlt ( "pos_color2"           , 0.2f );  
   AiParameterFlt ( "mid_color2"           , 0.5f );  
   AiParameterRGBA( "color3"               , 0.0f, 0.0f, 1.0f, 1.0 );  
   AiParameterFlt ( "pos_color3"           , 0.35f );  
   AiParameterFlt ( "mid_color3"           , 0.5f );  
   AiParameterRGBA( "color4"               , 0.0f, 1.0f, 1.0f, 1.0 );  
   AiParameterFlt ( "pos_color4"           , 0.5f );  
   AiParameterFlt ( "mid_color4"           , 0.5f );  
   AiParameterRGBA( "color5"               , 0.0f, 1.0f, 0.0f, 1.0 );  
   AiParameterFlt ( "pos_color5"           , 0.65f );  
   AiParameterFlt ( "mid_color5"           , 0.5f );  
   AiParameterRGBA( "color6"               , 1.0f, 1.0f, 0.0f, 1.0 );  
   AiParameterFlt ( "pos_color6"           , 0.8f );  
   AiParameterFlt ( "mid_color6"           , 0.5f );  
   AiParameterRGBA( "color7"               , 0.0f, 0.0f, 0.0f, 1.0 );  
   AiParameterFlt ( "pos_color7"           , -1.0f );  
   AiParameterFlt ( "mid_color7"           , 0.5f );  
   AiParameterRGBA( "color8"               , 0.0f, 0.0f, 0.0f, 1.0 );  
   AiParameterFlt ( "pos_color8"           , -1.0f );  
   AiParameterFlt ( "mid_color8"           , 0.5f );  
   AiParameterInt ( "alpha_interpolation"  , 1 );
   AiParameterFlt ( "alpha1"               , 0.0f );  
   AiParameterFlt ( "pos_alpha1"           , 0.0f );  
   AiParameterFlt ( "mid_alpha1"           , 0.5f );
   AiParameterFlt ( "alpha2"               , 1.0f );  
   AiParameterFlt ( "pos_alpha2"           , 1.0f );  
   AiParameterFlt ( "mid_alpha2"           , 0.5f );
   AiParameterFlt ( "alpha3"               , 0.0f );  
   AiParameterFlt ( "pos_alpha3"           , -1.0f );  
   AiParameterFlt ( "mid_alpha3"           , 0.5f );
   AiParameterFlt ( "alpha4"               , 0.0f );  
   AiParameterFlt ( "pos_alpha4"           , -1.0f );  
   AiParameterFlt ( "mid_alpha4"           , 0.5f );
   AiParameterFlt ( "alpha5"               , 0.0f );  
   AiParameterFlt ( "pos_alpha5"           , -1.0f );  
   AiParameterFlt ( "mid_alpha5"           , 0.5f );
   AiParameterFlt ( "alpha6"               , 0.0f );  
   AiParameterFlt ( "pos_alpha6"           , -1.0f );  
   AiParameterFlt ( "mid_alpha6"           , 0.5f );
   AiParameterFlt ( "alpha7"               , 0.0f );  
   AiParameterFlt ( "pos_alpha7"           , -1.0f );  
   AiParameterFlt ( "mid_alpha7"           , 0.5f );
   AiParameterFlt ( "alpha8"               , 0.0f );  
   AiParameterFlt ( "pos_alpha8"           , -1.0f );  
   AiParameterFlt ( "mid_alpha8"           , 0.5f );
   AiParameterStr ( "tspace_id"            , "" );                
   AiParameterVec ( "repeats"              , 1, 1, 1 );           
   AiParameterBool( "alt_x"                , false );             
   AiParameterBool( "alt_y"                , false );             
   AiParameterBool( "alt_z"                , false );             
   AiParameterVec ( "min"                  , 0, 0, 0);            
   AiParameterVec ( "max"                  , 1, 1, 1 );           
   AiParameterVec ( "step"                 , 0.002f, 0.002f, 0.002f );  // Not Implemented
   AiParameterFlt ( "factor"               , 5.0f );                    // Not Implemented
   AiParameterBool( "torus_u"              , false );             
   AiParameterBool( "torus_v"              , false );             
   AiParameterBool( "bump_inuse"           , false );                   // Not Implemented
   AiParameterBool( "alpha"                , false );                   // Not Implemented
   AiParameterBool( "alpha_output"         , false );
   AiParameterFlt ( "alpha_factor"         , 1 );
}

namespace {

struct GradientKey
{
   float position;
   int   index;

   GradientKey(float in_position = 0.0f, int in_index = 0) :
      position(in_position),
      index(in_index)
   {
   }

   bool operator < (const GradientKey& pt) const { return position < pt.position; }
};

// Get the indices of the keys whose position encloses the input position
// Return true if a valid pair was found, else false.
//
bool GetBounds(const vector <GradientKey> &in_keys, const float in_x, int &out_prev_index, int &out_next_index)
{
   unsigned int sz = (unsigned int)in_keys.size();
   for (unsigned int i = 1; i < sz; i++)
   {
      if (in_x <= in_keys[i].position)
      {
         out_prev_index = i - 1;
         out_next_index = i;
         return true;
      }
   }

   return false;
}


// Use AtArray's to store colors that are enabled
typedef struct
{
   int   gradient_type;
   bool  enable_alpha_gradient, invert, clip;
   bool  rgba_inter_linear, alpha_inter_linear;

   vector <float> mid_rgb_pos, mid_alpha_pos;    // the mid point positions
   vector <GradientKey> rgb_keys, alpha_keys;    // the gradient key points

   float rgb_keys_min_pos, rgb_keys_max_pos;     // rgb keys min and max position
   float alpha_keys_min_pos, alpha_keys_max_pos; // alpha keys min and max position

   bool alt_x, alt_y;
   bool torus_u, torus_v;
   bool alpha_output;
   AtString projection_wrap;
} ShaderData;

}

// rgba gradient
const string g_pos_color_names[] = { "pos_color1", "pos_color2", "pos_color3", "pos_color4", 
                                     "pos_color5", "pos_color6", "pos_color7", "pos_color8" };

const int g_color_indices[] = { p_color1, p_color2, p_color3, p_color4, p_color5, p_color6, p_color7, p_color8 };

const string g_mid_color_names[] = { "mid_color1", "mid_color2", "mid_color3", "mid_color4", 
                                     "mid_color5", "mid_color6", "mid_color7", "mid_color8" };
// alpha gradient
const string g_pos_alpha_names[] = { "pos_alpha1", "pos_alpha2", "pos_alpha3", "pos_alpha4", 
                                     "pos_alpha5", "pos_alpha6", "pos_alpha7", "pos_alpha8" };

const int g_alpha_indices[] = { p_alpha1, p_alpha2, p_alpha3, p_alpha4, p_alpha5, p_alpha6, p_alpha7, p_alpha8 };

const string g_mid_alpha_names[] = { "mid_alpha1", "mid_alpha2", "mid_alpha3", "mid_alpha4", 
                                     "mid_alpha5", "mid_alpha6", "mid_alpha7", "mid_alpha8" };


node_initialize
{
   ShaderData *data = new ShaderData;
   data->mid_rgb_pos.resize(8);
   data->mid_alpha_pos.resize(8);
   AiNodeSetLocalData(node, data);
}

node_update
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);

   data->enable_alpha_gradient = AiNodeGetBool(node, "enable_alpha_gradient");
   data->gradient_type = AiNodeGetInt(node, "gradient_type");
   data->invert = AiNodeGetBool(node, "invert");
   data->clip = AiNodeGetBool(node, "clip");
   data->rgba_inter_linear = AiNodeGetInt(node, "rgba_interpolation") == LINEAR;
   data->alpha_inter_linear = AiNodeGetInt(node, "alpha_interpolation") == LINEAR;

   data->rgb_keys.clear();
   data->alpha_keys.clear();

   GradientKey gk;
   for (unsigned int i = 0; i < 8; i++)
   {
      gk.position = AiNodeGetFlt(node, g_pos_color_names[i].c_str());
      if (gk.position != -1)
      {
         gk.index = i;
         data->rgb_keys.push_back(gk);
      }
      if (data->enable_alpha_gradient) // also insert the alpha
      {
         gk.position = AiNodeGetFlt(node, g_pos_alpha_names[i].c_str());
         if (gk.position != -1)
         {
            gk.index = i;
            data->alpha_keys.push_back(gk);
         }
      }
   }

   // sort by increasing positions
   sort(data->rgb_keys.begin(), data->rgb_keys.end());
   sort(data->alpha_keys.begin(), data->alpha_keys.end());

   data->rgb_keys_min_pos = data->rgb_keys.begin()->position;
   data->rgb_keys_max_pos = data->rgb_keys.rbegin()->position;
   if (data->enable_alpha_gradient)
   {
       data->alpha_keys_min_pos = data->alpha_keys.begin()->position;
       data->alpha_keys_max_pos = data->alpha_keys.rbegin()->position;
   }

   for (unsigned int i = 0; i < 8; i++)
   {
      data->mid_rgb_pos[i]   = AiNodeGetFlt(node, g_mid_color_names[i].c_str());
      data->mid_alpha_pos[i] = AiNodeGetFlt(node, g_mid_alpha_names[i].c_str());
   }

   data->alt_x   = AiNodeGetBool(node, "alt_x");
   data->alt_y   = AiNodeGetBool(node, "alt_y");
   data->torus_u = AiNodeGetBool(node, "torus_u");
   data->torus_v = AiNodeGetBool(node, "torus_v");

   data->alpha_output = AiNodeGetBool(node, "alpha_output");
   
   const char* tspace_id = AiNodeGetStr(node, "tspace_id");
   char* s = (char*)alloca(strlen(tspace_id) + 6);
   sprintf(s, "%s_wrap", tspace_id);
   data->projection_wrap = AtString(s);
}

node_finish
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
   data->rgb_keys.clear();
   data->alpha_keys.clear();
   data->mid_rgb_pos.clear();
   data->mid_alpha_pos.clear();
   delete data;
}

shader_evaluate
{
   float    input;
   AtRGBA   out_color = AI_RGBA_ZERO;
   int      prev_index, next_index;
   bool     wrap_u(false), wrap_v(false);
   AtVector2 uvPoint;

   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);

   float u(sg->u), v(sg->v);

   if (AiUDataGetVec2(AiShaderEvalParamStr(p_tspace_id), uvPoint))
   {
      u = uvPoint.x;
      v = uvPoint.y;
      // we don't care about UV derivatives here
   }

   AtArray* wrap_settings;
   if (AiUDataGetArray(data->projection_wrap, wrap_settings))
   {
      wrap_u = AiArrayGetBool(wrap_settings, 0);
      wrap_v = AiArrayGetBool(wrap_settings, 1);
   }

   AtVector repeats = AiShaderEvalParamVec(p_repeats);
   AtVector min     = AiShaderEvalParamVec(p_min);
   AtVector max     = AiShaderEvalParamVec(p_max);

   compute_uv(u, v, repeats, min, max, wrap_u || data->torus_u, wrap_v || data->torus_v, data->alt_x, data->alt_y);

   switch (data->gradient_type)
   {
      case VERTICAL:
         input = v; break;
      case HORIZONTAL:
         input = u; break;
      case RADIAL:
         input = sqrtf( ((u - 0.5f) * (u - 0.5f) + (v - 0.5f) * (v - 0.5f)) * 2.0f ); 
         break;
      case SCOPE:
         input = std::abs(atan2(u - 0.5f, -v + 0.5f) / AI_PI); break;
      case DIAGONAL_DOWN:
         input = 0.5f * (v + u); break;
      case DIAGONAL_UP:
         input = 0.5f * (u + (1.0f - v)); break;
      default:
         input = v;
   }

   if (data->invert)
      input = 1.0f - input;

   // RGB
   if (input <= data->rgb_keys_min_pos) // before first key
      out_color = data->clip ? AI_RGBA_ZERO : AiShaderEvalParamRGBA(g_color_indices[data->rgb_keys.begin()->index]);
   else if (input >= data->rgb_keys_max_pos) // after key
      out_color = data->clip ? AI_RGBA_ZERO : AiShaderEvalParamRGBA(g_color_indices[data->rgb_keys.rbegin()->index]);
   else if (GetBounds(data->rgb_keys, input, prev_index, next_index)) // get the bounding keys and interpolate
   {
      GradientKey *gkA = &data->rgb_keys[prev_index]; // left side key
      GradientKey *gkB = &data->rgb_keys[next_index]; // right side key
      // get the keys colors
      AtRGBA blendA = AiShaderEvalParamRGBA(g_color_indices[gkA->index]);
      AtRGBA blendB = AiShaderEvalParamRGBA(g_color_indices[gkB->index]);

      float range = gkB->position - gkA->position;
      float rerange = (input - gkA->position) / (range == 0.0f ? 1.0f : range);

      if (rerange < data->mid_rgb_pos[gkA->index])
         rerange = (rerange / data->mid_rgb_pos[gkA->index]) / 2.0f;
      else
         rerange = 1.0f - ((1.0f - rerange) / (2.0f * (1.0f - data->mid_rgb_pos[gkA->index])));

      out_color = data->rgba_inter_linear ? AiLerp(rerange, blendA, blendB) : AiHerp(rerange, blendA, blendB);
   }

   if (data->enable_alpha_gradient) // alpha. Same as above, using the alpha keys
   {
       if (input <= data->alpha_keys_min_pos)
          out_color.a = data->clip ? 0.0f : AiShaderEvalParamFlt(g_alpha_indices[data->alpha_keys.begin()->index]);
       else if (input >= data->alpha_keys_max_pos)
          out_color.a = data->clip ? 0.0f : AiShaderEvalParamFlt(g_alpha_indices[data->alpha_keys.rbegin()->index]);
      else if (GetBounds(data->alpha_keys, input, prev_index, next_index))
      {
         GradientKey *gkA = &data->alpha_keys[prev_index];
         GradientKey *gkB = &data->alpha_keys[next_index];
    
         float blendA = AiShaderEvalParamFlt(g_alpha_indices[gkA->index]);
         float blendB = AiShaderEvalParamFlt(g_alpha_indices[gkB->index]);

         float range = gkB->position - gkA->position;
         float rerange = (input - gkA->position) / (range == 0.0f ? 1.0f : range);

         if (rerange < data->mid_alpha_pos[gkA->index])
            rerange = (rerange / data->mid_alpha_pos[gkA->index]) / 2.0f;
         else
            rerange = 1.0f - ((1.0f - rerange) / (2.0f * (1.0f - data->mid_alpha_pos[gkA->index])));

         out_color.a = data->alpha_inter_linear ? AiLerp(rerange, blendA, blendB) : AiHerp(rerange, blendA, blendB);
      }
   }

   if (data->alpha_output)
   {
      // clone the mray behavior: multiply 
      out_color.a*= AiShaderEvalParamFlt(p_alpha_factor);
      out_color.r = out_color.g = out_color.b = out_color.a;
   }

   sg->out.RGBA() = out_color;
}

