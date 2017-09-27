/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <ai.h>
#include "color_utils.h"
#include "shader_utils.h"
#include <map>
#include <vector>

using namespace std;

static const AtString s_auto("auto");
static const AtString s_linear("linear");
static const AtString s_sRGB("sRGB");

enum eColorProfile
{
   COLORPROFILE_AUTOMATIC = 0,
   COLORPROFILE_LINEAR,
   COLORPROFILE_SRGB,
   COLORPROFILE_USER
};

// Data to speed up the resolution on time-dependent image sequences
// ex.: sequence.[1..10;3].png
typedef struct
{
   int   start, end;     // 1, 10
   char  paddingFormat[8]; // "%.3d"
   int   baseNameLength;   // 9 (== strlen("sequence."))
   const char* extension;  // ".png"
   size_t extensionLength;
} ImageSequence;

// This is taken from sib_image_clip, keeping just the necessary fields.
class CClipData
{
private:
   const char*       m_filename;
   AtString          m_timeSource;
   AtString          m_tspace_id;
   AtTextureHandle*  m_textureHandle;
   ImageSequence     m_imageSequence;
   CTokenFilename    m_tokenFilename;
   int               m_filter;
   float             m_gamma;
   float             m_fstop;
   float             m_hue, m_saturation, m_gain;
   float             m_brightness;
   float             m_xmin, m_xmax;
   float             m_ymin, m_ymax;
   // Texture Options Property
   int               m_mipmap_bias;
   int               m_sWrap, m_tWrap;
   bool              m_wrap_u, m_wrap_v;
   bool              m_swapST;
   bool              m_needEvaluation;
   bool              m_flipx, m_flipy;
   bool              m_applyColorCorrection;
   bool              m_applyCroppingFlip;
   AtString          m_colorSpace;

public:
   bool              m_isValid;

   CClipData() : 
      m_tspace_id(NULL), m_textureHandle(NULL), m_gamma(1.0f), 
      m_wrap_u(false), m_wrap_v(false),
      m_colorSpace(s_auto), m_isValid(false)
   {}

   CClipData(const CClipData &in_arg) :
      m_filename(in_arg.m_filename), m_timeSource(in_arg.m_timeSource), m_tspace_id(in_arg.m_tspace_id),
      m_textureHandle(in_arg.m_textureHandle), m_imageSequence(in_arg.m_imageSequence), m_tokenFilename(in_arg.m_tokenFilename),
      m_filter(in_arg.m_filter), m_gamma(in_arg.m_gamma), 
      m_fstop(in_arg.m_fstop), m_hue(in_arg.m_hue), 
      m_saturation(in_arg.m_saturation), m_gain(in_arg.m_gain), m_brightness(in_arg.m_brightness),
      m_xmin(in_arg.m_xmin), m_xmax(in_arg.m_xmax), m_ymin(in_arg.m_ymin), 
      m_ymax(in_arg.m_ymax), m_mipmap_bias(in_arg.m_mipmap_bias),m_sWrap(in_arg.m_sWrap), 
      m_tWrap(in_arg.m_tWrap), m_wrap_u(in_arg.m_wrap_u), m_wrap_v(in_arg.m_wrap_v), 
      m_swapST(in_arg.m_swapST), m_needEvaluation(in_arg.m_needEvaluation), m_flipx(in_arg.m_flipx), 
      m_flipy(in_arg.m_flipy),m_applyColorCorrection(in_arg.m_applyColorCorrection), m_applyCroppingFlip(in_arg.m_applyCroppingFlip),  
      m_colorSpace(in_arg.m_colorSpace), m_isValid(in_arg.m_isValid)
   {}

   ~CClipData()
   {}

   // Get all the clip data from a map_lookup shader user data
   void GetData(const AtNode *in_node, const string &in_ownerName);
   // Lookup the texture map, honoring all the lookup parameters
   AtRGBA LookupTextureMap(AtShaderGlobals *sg);
   void DestroyTextureHandle();
};


typedef struct 
{
   AtString  map;
   CClipData clip_data;
} MapLookupUserData;

typedef pair<const string, MapLookupUserData> ObjectName_UserData_Pair;
typedef map <const string, MapLookupUserData> ObjectName_UserData_Map;

typedef struct 
{
   AtString                map;
   bool                    hasUserData;
   ObjectName_UserData_Map userData;
} MapLookupShaderData;

inline float fToLin(float in_value)
{
   return in_value <= 0.04045f ? in_value / 12.92f : 
                                 powf((in_value + 0.055f) / 1.055f, 2.4f);
}

void RGBToLin(AtRGBA &io_color);
AtRGBA TransformHSV(const AtRGBA &in_color, const float H, const float S, const float V);
// Collect the data to speed up string concatenation in case we're using a picture sequence (for instance "seq.[1..10;3].png")
bool GetSequenceData(const char *in_s, ImageSequence &out_sequence);
// Resolve a picture sequence (for instance "seq.[1..10;3].png") at a given frame time.
const char* ResolveSequenceAtFrame(const AtShaderGlobals *in_sg, const char *in_s, const int in_frame, const ImageSequence &in_sequence, bool in_atStartFrame=false);
// Return the pointer to the user data associated with the current rendering object (sg->Op)
MapLookupUserData *GetLookupUserData(const AtShaderGlobals *in_sg, MapLookupShaderData *in_data);
// Get the user data associated with all the objects with instance values, and store them into map, keyed by the object's name.
void SetUserData(const AtNode *in_node, MapLookupShaderData *io_data, const char *in_mapSuffix);
// Destroy all the open texture handles stored in the shader data
void DestroyTextureHandles(MapLookupShaderData *in_data);





