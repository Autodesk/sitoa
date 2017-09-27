/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "map_lookup.h"

//////////////////////////////////////////////////
// shared by the lookup shaders and sib_image_clip
//////////////////////////////////////////////////

void RGBToLin(AtRGBA &io_color)
{
   io_color.r = fToLin(io_color.r);
   io_color.g = fToLin(io_color.g);
   io_color.b = fToLin(io_color.b);
   // alpha stays the same
}


AtRGBA TransformHSV(const AtRGBA &in_color, const float H, const float S, const float V)
{
   float H_rad = H * AI_DTOR;
   float VS = V * S;
   float VSU = VS * cos(H_rad);
   float VSW = VS * sin(H_rad);
   
   AtRGBA value;
   value.r = (0.299f*V + 0.701f*VSU + 0.168f*VSW) * in_color.r
           + (0.587f*V - 0.587f*VSU + 0.330f*VSW) * in_color.g
           + (0.114f*V - 0.114f*VSU - 0.497f*VSW) * in_color.b;
   value.g = (0.299f*V - 0.299f*VSU - 0.328f*VSW) * in_color.r
           + (0.587f*V + 0.413f*VSU + 0.035f*VSW) * in_color.g
           + (0.114f*V - 0.114f*VSU + 0.292f*VSW) * in_color.b;
   value.b = (0.299f*V - 0.3f*VSU   + 1.25f*VSW)  * in_color.r
           + (0.587f*V - 0.588f*VSU - 1.05f*VSW)  * in_color.g
           + (0.114f*V + 0.886f*VSU - 0.203f*VSW) * in_color.b;
   value.a = in_color.a;
   
   value.r = AiClamp(value.r, 0.0f, 1.0f);
   value.g = AiClamp(value.g, 0.0f, 1.0f);
   value.b = AiClamp(value.b, 0.0f, 1.0f);

   return value;
}


// Collect the data to speed up string concatenation in case we're using a picture sequence (for instance "seq.[1..10;3].png")
//
// @param in_s             The input sequence
// @param out_sequence     The structure were the info are save (type of extension, etc.)
//
// @return true if the input path is a picture sequence, else false
//
bool GetSequenceData(const char *in_s, ImageSequence &out_sequence)
{
   out_sequence.extension = NULL; // stays null if the sequence is invalid

   const char* dotC = in_s + strlen(in_s);
   while (dotC >= in_s && *dotC != '.')
      dotC--;

   if (dotC == in_s)
      return false;

   const char *closeC = dotC - 1;

   if (*closeC != ']')
      return false;

   const char *openC = closeC - 1;

   while (openC >= in_s && *openC != '[')
      openC--;

   if (openC == in_s)
      return false;

   out_sequence.extension = dotC;
   out_sequence.extensionLength = strlen(dotC);

   const char* c = openC + 1;
   // c -> "1..10;3].png"

   out_sequence.baseNameLength = (int)(openC - in_s);

   char *startS = (char*)alloca(closeC - openC);
   char* sP = startS;
   while (c < closeC && *c != '.')
   {
      *sP = *c;
      c++; sP++;
   }
   *sP = '\0';

   out_sequence.start = atoi(startS);

   c+=2; // -> 10;3].png
   char *endS = (char*)alloca(closeC - openC);
   sP = endS;
   while (c < closeC && *c != ';')
   {
      *sP = *c;
      c++; sP++;
   }
   *sP = '\0';

   out_sequence.end = atoi(endS);

   c++; // -> 3].png
   char *paddingS = (char*)alloca(closeC - openC);
   sP = paddingS;
   while (c < closeC && *c != ']')
   {
      *sP = *c;
      c++; sP++;
   }
   *sP = '\0';
 
   int padding = strlen(paddingS) > 0 ? atoi(paddingS) : 1;

   sprintf(out_sequence.paddingFormat, "%%.%dd", padding);

   return true;
}


// Resolve a picture sequence (for instance "seq.[1..10;3].png") at a given frame time.
//
// @param in_sg            The shader globals, or NULL if called at update time
// @param in_s             The input sequence
// @param in_frame         The frame to resolve the sequence at
// @param in_sequence      The sequence data to speed up the string concatenation
// @param in_atStartFrame  If true, use the sequence start frame instead of in_frame
//
// @return the resolved image name
//
const char* ResolveSequenceAtFrame(const AtShaderGlobals *in_sg, const char *in_s, const int in_frame, const ImageSequence &in_sequence, bool in_atStartFrame)
{
   if (!in_sequence.extension)
      return NULL;

   int frame = in_atStartFrame ? in_sequence.start : AiClamp(in_frame, in_sequence.start, in_sequence.end);

   uint32_t sz = (uint32_t)strlen(in_s) + 8; // large enough
   // if during eval, allocate by AiShaderGlobalsQuickAlloc, and the caller must NOT free the returned buffer
   char *result = in_sg ? (char*)AiShaderGlobalsQuickAlloc(in_sg, sz) : (char*)AiMalloc(sz); 
   // copy the base name
   memcpy((void*)result, (void*)in_s, in_sequence.baseNameLength);
   // add the frame
   int paddingLength = sprintf(&result[in_sequence.baseNameLength], in_sequence.paddingFormat, frame);
   // add the extension
   memcpy((void*)&result[in_sequence.baseNameLength + paddingLength], (void*)in_sequence.extension, in_sequence.extensionLength + 1);

   return result;
}


//////////////////////////////////////////////////
// map_lookup* shaders
//////////////////////////////////////////////////

// Return the pointer to the user data associated with the current rendering object (sg->Op)
//
MapLookupUserData *GetLookupUserData(const AtShaderGlobals *in_sg, MapLookupShaderData *in_data)
{
   MapLookupUserData* ud = NULL;
   if (in_data->hasUserData)
   {
      // get the object name and find its user data store in the shader data map
      string nodeName(GetShaderOwnerName(in_sg));
      ObjectName_UserData_Map::iterator it = in_data->userData.find(nodeName);
      if (it != in_data->userData.end())
         ud = &it->second;
   }

   return ud;
}

// Get the user data associated with all the objects with instance values, and store them
// into map, keyed by the object's name.
//
void SetUserData(const AtNode *in_node, MapLookupShaderData *io_data, const char *in_mapSuffix)
{
   // collect the list of the names of the objects that have instance parameter
   // values for this shader. We search for user data ending with mapSuffix.
   // in_mapSuffix is "_map" for the lookup shaders, "_vprop" for sib_vertex_color_alpha
   // If such a user data exist, its name is made of a object name + in_mapSuffix
   vector <string> objNames;
   AtUserParamIterator *iter = AiNodeGetUserParamIterator(in_node);
   // iterate the shader user data 
   while (!AiUserParamIteratorFinished(iter))
   {
      const AtUserParamEntry *upEntry = AiUserParamIteratorGetNext(iter);
      const char* upName = AiUserParamGetName(upEntry);
      const char* attrStart = strstr(upName, in_mapSuffix);
      if (attrStart)
      {
         if (strlen(attrStart) == strlen(in_mapSuffix))
         {
            // found mapSuffix as the suffix of the user data name
            size_t objNameLenght = strlen(upName) - strlen(attrStart);
            char* objName = (char*)alloca(objNameLenght + 1);
            memcpy(objName, upName, objNameLenght);
            objName[objNameLenght] = '\0';
            objNames.push_back(string(objName));
         }
      }
   }
   AiUserParamIteratorDestroy(iter);

   io_data->hasUserData = objNames.size() > 0;
   if (!io_data->hasUserData)
      return;

   string attr, suffix;
   for (vector <string>::iterator it = objNames.begin(); it != objNames.end(); it++)
   {
      MapLookupUserData ud;

      string mapAttributeName = *it + in_mapSuffix;
      if (AiNodeLookUpUserParameter(in_node, mapAttributeName.c_str()))
         ud.map = AiNodeGetStr(in_node, mapAttributeName.c_str());

      ud.clip_data.GetData(in_node, *it);
      // insert the user data in the map, using the object name as key
      io_data->userData.insert(ObjectName_UserData_Pair(*it, ud));
   }
}

// Destroy all the open texture handles stored in the shader data
//
void DestroyTextureHandles(MapLookupShaderData *in_data)
{
   if (!in_data->hasUserData)
      return;
   for (ObjectName_UserData_Map::iterator it = in_data->userData.begin(); it != in_data->userData.end(); it++)
      it->second.clip_data.DestroyTextureHandle();
}


// Get all the clip data from a map_lookup shader user data
// Copied from the sib_image_clip update function, but using the shape node as prefix for the clip attributes to get as user data
// It's copied once and only once per object per shader, so to get the full list of attributes to use to lookup the image in the evaluate
//
// @param in_node       The shader node
// @param in_ownerName  The shader's owner node name
//
void CClipData::GetData(const AtNode *in_node, const string &in_ownerName)
{
   string dataName;

   // let's do the test on the attributes existence on the first expected one.
   // If one exists, all the other should
   dataName = in_ownerName + "_filter";
   if (!AiNodeLookUpUserParameter(in_node, dataName.c_str()))
      return;
   m_isValid = true;

   m_filter = AiNodeGetInt(in_node, dataName.c_str());

   dataName = in_ownerName + "_mipmap_bias";
   m_mipmap_bias = AiNodeGetInt(in_node, dataName.c_str());
   dataName = in_ownerName + "_swap_st";
   m_swapST = AiNodeGetBool(in_node, dataName.c_str());
   dataName = in_ownerName + "_s_wrap";
   m_sWrap = AiNodeGetInt(in_node, dataName.c_str());
   dataName = in_ownerName + "_t_wrap";
   m_tWrap = AiNodeGetInt(in_node, dataName.c_str());

   dataName = in_ownerName + "_SourceFileName";
   m_filename = AiNodeGetStr(in_node, dataName.c_str());

   m_tokenFilename.Init(m_filename);

   dataName = in_ownerName + "_TimeSource";
   m_timeSource = AiNodeGetStr(in_node, dataName.c_str());
   m_needEvaluation = (strlen(m_timeSource) > 0) || m_tokenFilename.IsValid();

   dataName = in_ownerName + "_RenderColorProfile";
   const char* colorProfile = AiNodeGetStr(in_node, dataName.c_str());

   if (!strcmp(colorProfile, "Automatic"))
      m_colorSpace = s_auto;
   if (!strcmp(colorProfile, "Linear"))
      m_colorSpace = s_linear;
   else if (!strcmp(colorProfile, "sRGB"))
      m_colorSpace = s_sRGB;
   else if (!strcmp(colorProfile, "User Gamma"))
   {  // apply a custom (inverse) gamma value
      m_colorSpace = s_linear;
      dataName = in_ownerName + "_RenderGamma";
      m_gamma = 1.0f / AiNodeGetFlt(in_node, dataName.c_str());
   }

   const char* filename = m_filename;
   if (m_needEvaluation)
   {
      float dummyF = 0.5f;
      if (m_tokenFilename.IsValid())
         filename = m_tokenFilename.Resolve(NULL, dummyF, dummyF);
      else
      {
         GetSequenceData(m_filename, m_imageSequence);
         filename = ResolveSequenceAtFrame(NULL, m_filename, 0, m_imageSequence, true);
      }
   }
   else
      m_textureHandle = AiTextureHandleCreate(filename, m_colorSpace);

   if (m_needEvaluation)
      AiFree((void*)filename);

   dataName = in_ownerName + "_Exposure";
   m_fstop = powf(2.0f, AiNodeGetFlt(in_node, dataName.c_str()));

   dataName = in_ownerName + "_Hue";
   float hue = AiNodeGetFlt(in_node, dataName.c_str());
   dataName = in_ownerName + "_Saturation";
   float saturation = AiNodeGetFlt(in_node, dataName.c_str());
   dataName = in_ownerName + "_Gain";
   float gain = AiNodeGetFlt(in_node, dataName.c_str());
   dataName = in_ownerName + "_Brightness";
   float brightness = AiNodeGetFlt(in_node, dataName.c_str());
   dataName = in_ownerName + "_GrayScale";
   bool grayscale = AiNodeGetBool(in_node, dataName.c_str());

   m_applyColorCorrection = grayscale || hue != 0.0f || saturation != 100.f || gain != 100.f || brightness != 0.0f;

   if (m_applyColorCorrection)
   {
      m_hue = fmod(-hue, 360.0f);
      m_saturation = grayscale ? 0.0f : saturation / 100.0f;
      m_gain = gain / 100.0f;
      m_brightness = brightness / 100.0f;
   }

   // cropping and flip
   dataName = in_ownerName + "_Xmin";
   m_xmin = AiNodeGetFlt(in_node, dataName.c_str());
   dataName = in_ownerName + "_Xmax";
   m_xmax = AiNodeGetFlt(in_node, dataName.c_str());
   dataName = in_ownerName + "_Ymin";
   m_ymin = AiNodeGetFlt(in_node, dataName.c_str());
   dataName = in_ownerName + "_Ymax";
   m_ymax = AiNodeGetFlt(in_node, dataName.c_str());
   dataName = in_ownerName + "_FlipX";
   m_flipx = AiNodeGetBool(in_node, dataName.c_str());
   dataName = in_ownerName + "_FlipY";
   m_flipy = AiNodeGetBool(in_node, dataName.c_str());

   m_applyCroppingFlip = m_flipx || m_flipy || m_xmin != 0.0f || m_xmax != 1.0f || 
                         m_ymin != 0.0f || m_ymax != 1.0f;

   // The Softimage wrapping settings, taken from txt2d_image_explicit
   // The wrap array is set by the exporter on the map_lookup shader, and not on the owning object as for txt2d_image_explicit
   dataName = in_ownerName + "_wrap";
   AtArray* wrap_settings = AiNodeGetArray(in_node, dataName.c_str());
   if (wrap_settings)
   {
      m_wrap_u = AiArrayGetBool(wrap_settings, 0);
      m_wrap_v = AiArrayGetBool(wrap_settings, 1);
   }

   dataName = in_ownerName + "_tspace_id";
   if (AiNodeLookUpUserParameter(in_node, dataName.c_str()))
      m_tspace_id = AiNodeGetStr(in_node, dataName.c_str());
}


// Lookup the texture map, honoring all the lookup parameters
// This is basically a merge of the evaluate functions of txt2d_image_explicit and sib_image_clip,
// being the data stored in in_clipData
// From txt2d_image_explicit, we just need to take the Softimage wrapping part
// From sib_image_clip all the rest
//
// @param sg     The shader's global
//
// @return       the looked up color.
//
AtRGBA CClipData::LookupTextureMap(AtShaderGlobals *sg)
{
   AtRGBA result = AI_RGBA_ZERO;
   BACKUP_SG_UVS;

   if (m_tspace_id)
   {
      AtVector2 uvPoint;
      AtVector  uvwPoint;

      bool getDerivatives(false);
      bool is_homogenous = AiUserParamGetType(AiUDataGetParameter(m_tspace_id)) == AI_TYPE_VECTOR;
      if (is_homogenous)
      {
         if (AiUDataGetVec(m_tspace_id, uvwPoint))
         {
            // homogenous coordinates from camera projection, divide u and v by w
            sg->u = uvwPoint.x / uvwPoint.z;
            sg->v = uvwPoint.y / uvwPoint.z;

            AtVector altuv_dx, altuv_dy;
            if (AiUDataGetDxyDerivativesVec(m_tspace_id, altuv_dx, altuv_dy))
            {
               AtVector dx = uvwPoint + altuv_dx;
               AtVector dy = uvwPoint + altuv_dy;
               sg->dudx = dx.x / dx.z - sg->u;
               sg->dudy = dy.x / dy.z - sg->u;
               sg->dvdx = dx.y / dx.z - sg->v;
               sg->dvdy = dy.y / dy.z - sg->v;
            }
            else
               sg->dudx = sg->dudy = sg->dvdx = sg->dvdy = 0.0f;
         }
      }
      else if (AiUDataGetVec2(m_tspace_id, uvPoint))
      {
         sg->u = uvPoint.x;
         sg->v = uvPoint.y;
         getDerivatives = true;
      }

      if (getDerivatives)
      {
         AtVector2 altuv_dx, altuv_dy;
         if (AiUDataGetDxyDerivativesVec2(m_tspace_id, altuv_dx, altuv_dy))
         {
            sg->dudx = altuv_dx.x;
            sg->dudy = altuv_dy.x;
            sg->dvdx = altuv_dx.y;
            sg->dvdy = altuv_dy.y;
         }
         else
            sg->dudx = sg->dudy = sg->dvdx = sg->dvdy = 0.0f;
      }
   }

   if (m_wrap_u)
      sg->u-= floor(sg->u);
   if (m_wrap_v)
      sg->v-= floor(sg->v);

   // for UVs < 0, return black also for <tile> or <udim> (#1542)
   if (sg->u < 0.0f || sg->v < 0.0f)
   {
      RESTORE_SG_UVS;
      return result;
   }

   bool udimmed = m_needEvaluation && m_tokenFilename.IsValid();

   // if we're above 1, and this is NOT a <tile> or <udim> filename, return black
   if ( (!udimmed) && ( (m_sWrap <= 0 && sg->u > 1.0f) || (m_tWrap <= 0 && sg->v > 1.0f) ) )
   {
      RESTORE_SG_UVS;
      return result;
   }

   AtTextureParams tmap_params;
   AiTextureParamsSetDefaults(tmap_params);

   tmap_params.swap_st = m_swapST;
   tmap_params.filter = m_filter;
   tmap_params.mipmap_bias = m_mipmap_bias;

   if (udimmed) // wrap by CLAMP if this is a <udim> texture
      tmap_params.wrap_s = tmap_params.wrap_t = AI_WRAP_CLAMP;
   else
   {
      tmap_params.wrap_s = m_sWrap > 0 ? m_sWrap - 1 : 0;
      tmap_params.wrap_t = m_tWrap > 0 ? m_tWrap - 1 : 0;
   }

   // Flip & crop ?
   if (m_applyCroppingFlip)
   {
      sg->u = m_flipx ? AiLerp(sg->u, m_xmax, m_xmin) : AiLerp(sg->u, m_xmin, m_xmax);
      sg->v = m_flipy ? AiLerp(sg->v, m_ymax, m_ymin) : AiLerp(sg->v, m_ymin, m_ymax);
      // also, multiplying the uv derivatives by the LERP derivative
      float uDelta = m_flipx ? m_xmin - m_xmax : m_xmax - m_xmin;
      float vDelta = m_flipy ? m_ymin - m_ymax : m_ymax - m_ymin;
      sg->dudx*= uDelta;
      sg->dudy*= uDelta;
      sg->dvdx*= vDelta;
      sg->dvdy*= vDelta;
   }

   if (m_needEvaluation) // Deprecated lookup, needed with variable texture name
   {
      if (m_tokenFilename.IsValid())
      {
         // get the <udim>-ed filename, out of the current u,v
         AtString filename(m_tokenFilename.Resolve(sg, sg->u, sg->v));
         if (!filename.empty())
            result = AiTextureAccess(sg, filename, m_colorSpace, tmap_params);
      }
      else
      {
         int frame;
         float framef;

         if (AiUDataGetInt(m_timeSource, frame))
         {
            AtString filename(ResolveSequenceAtFrame(sg, m_filename, frame, m_imageSequence));
            if (!filename.empty())
               result = AiTextureAccess(sg, filename, m_colorSpace, tmap_params);
         }
         else if (AiUDataGetFlt(m_timeSource, framef))
         {
            frame = (int)floor(framef);
            framef-= floor(framef);
            AtString filename0(ResolveSequenceAtFrame(sg, m_filename, frame, m_imageSequence));
            AtString filename1(ResolveSequenceAtFrame(sg, m_filename, frame + 1, m_imageSequence));
            if (!filename0.empty() && !filename1.empty())
            {
               AtRGBA c0 = AiTextureAccess(sg, filename0, m_colorSpace, tmap_params);
               AtRGBA c1 = AiTextureAccess(sg, filename1, m_colorSpace, tmap_params);
               result = AiLerp(framef, c0, c1);
            }
         }
      }
   }
   else
      result = AiTextureHandleAccess(sg, m_textureHandle, tmap_params);

   if (m_gamma != 1.0f) // User Gamma case only
      RGBAGamma(&result, m_gamma);

   if (m_applyColorCorrection)
   {
      result = TransformHSV(result, m_hue, m_saturation, m_gain);
      result.r += m_brightness; 
      result.g += m_brightness; 
      result.b += m_brightness;
   }

   result.r*= m_fstop; 
   result.g*= m_fstop; 
   result.b*= m_fstop;

   RESTORE_SG_UVS;
   return result;
}


void CClipData::DestroyTextureHandle()
{
   if (m_textureHandle)
      AiTextureHandleDestroy(m_textureHandle);
}

