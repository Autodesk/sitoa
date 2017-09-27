/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "map_lookup.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBImageClipMethods);

node_parameters
{
   AiParameterStr ( "SourceFileName"              , ""    );
   AiParameterStr ( "SourceName"                  , ""    );      // Not Implemented
   AiParameterFlt ( "Radius"                      , 0.0f  );      // Not Implemented
   AiParameterFlt ( "Amount"                      , 1.0f  );      // Not Implemented
   AiParameterBool( "BlurAlpha"                   , false );      // Not Implemented
   AiParameterFlt ( "Hue"                         , 0.0f  );
   AiParameterFlt ( "Saturation"                  , 100.0f);
   AiParameterFlt ( "Gain"                        , 100.0f);
   AiParameterFlt ( "Brightness"                  , 0.0f  );
   AiParameterFlt ( "Xmin"                        , 0.0f  );       
   AiParameterFlt ( "Xmax"                        , 1.0f  );       
   AiParameterFlt ( "Ymin"                        , 0.0f  );       
   AiParameterFlt ( "Ymax"                        , 1.0f  );       
   AiParameterBool( "UsingNormalized"             , false );      // Not Implemented
   AiParameterBool( "GrayScale"                   , false );
   AiParameterBool( "SixteenBitsPerChannel"       , false );      // Not Implemented
   AiParameterBool( "EnableMipMap"                , false );      // Not Implemented
   AiParameterFlt ( "MipMapScale"                 , 1.0f  );      // Not Implemented
   AiParameterBool( "EnableMemoryMapping"         , false );      // Not Implemented
   AiParameterInt ( "ImageDefinitionType"         , 0     );
   AiParameterInt ( "ResX"                        , 128   );      // Not Implemented
   AiParameterInt ( "ResY"                        , 128   );      // Not Implemented
   AiParameterInt ( "Type"                        , 1     );      // Not Implemented
   AiParameterBool( "FlipX"                       , false );       
   AiParameterBool( "FlipY"                       , false );       
   AiParameterBool( "Image"                       , false );      // Not Implemented
   AiParameterInt ( "oglminfilter"                , 9729  );      // Not Implemented
   AiParameterInt ( "oglmagfilter"                , 9729  );      // Not Implemented
   AiParameterBool( "oglmipmap"                   , false );      // Not Implemented
   AiParameterInt ( "oglmaxsize"                  , 1024  );      // Not Implemented
   AiParameterFlt ( "incroglmaxsize"              , 0.0f  );      // Not Implemented
   AiParameterFlt ( "decroglmaxsize"              , 0.0f  );      // Not Implemented
   AiParameterInt ( "FieldType"                   , 0     );      // Not Implemented
   AiParameterInt ( "SourceTrack"                 , 0     );      // Not Implemented
   AiParameterFlt ( "Exposure"                    , 0.0f  );       
   AiParameterFlt ( "DisplayGamma"                , 2.2f  );      // Not Implemented
   AiParameterBool( "DisplayGammaAffectsRendering", false );      // Not Implemented
   AiParameterStr ( "TimeSource"                  , ""    );
   AiParameterStr ( "RenderColorProfile"          , "Linear" );   
   AiParameterFlt ( "RenderGamma"                 , 2.2f  );
   AiParameterInt ( "filter"                      , 1 );
   AiParameterInt ( "mipmap_bias"                 , 0 );
   AiParameterBool( "swap_st"                     , false );
   AiParameterInt ( "s_wrap"                      , 0 );
   AiParameterInt ( "t_wrap"                      , 0 );
}

namespace {

typedef struct 
{
   const char*       filename;
   AtString          timeSource;
   AtTextureHandle*  textureHandle;
   ImageSequence     imageSequence;
   CTokenFilename    tokenFilename;    // to resolve the <udim> and <tile> tokens (#1325)
   float             gamma;
   float             fstop;
   float             hue, saturation, gain;
   float             brightness;
   float             xmin, xmax;
   float             ymin, ymax;
   int               sWrap, tWrap;
   bool              flipx, flipy;
   bool              applyColorCorrection;
   bool              applyCroppingFlip;
   bool              needEvaluation;   // if true, the texture path depends on the current uv or time, so we can't use texture handles
   AtString          color_space;

   AtTextureParams   tmap_params;
} ShaderData;

}

node_initialize
{
   ShaderData *data = new ShaderData;
   data->textureHandle = NULL;
   AiNodeSetLocalData(node, data);
} 

node_update
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
   
   data->sWrap      = AiNodeGetInt(node, "s_wrap");
   data->tWrap      = AiNodeGetInt(node, "t_wrap");
   data->filename   = AiNodeGetStr(node, "SourceFileName");
   data->timeSource = AiNodeGetStr(node, "TimeSource");

   AiTextureParamsSetDefaults(data->tmap_params);
   data->tmap_params.swap_st = AiNodeGetBool(node, "swap_st");
   data->tmap_params.filter = AiNodeGetInt(node, "filter");
   data->tmap_params.mipmap_bias = AiNodeGetInt(node, "mipmap_bias");

   if (data->textureHandle)
   {
      AiTextureHandleDestroy(data->textureHandle);
      data->textureHandle = NULL;
   }

   data->tokenFilename.Init(data->filename);
   data->needEvaluation = data->tokenFilename.IsValid() || (!data->timeSource.empty());

   data->gamma = 1.0f;
   data->color_space = s_auto;
   const char* colorProfile = AiNodeGetStr(node, "RenderColorProfile");

   if (!strcmp(colorProfile, "Automatic"))
      data->color_space = s_auto;
   if (!strcmp(colorProfile, "Linear"))
      data->color_space = s_linear;
   else if (!strcmp(colorProfile, "sRGB"))
      data->color_space = s_sRGB;
   else if (!strcmp(colorProfile, "User Gamma"))
   {  // apply a custom (inverse) gamma value
      data->color_space = s_linear;
      data->gamma = 1.0f / AiNodeGetFlt(node, "RenderGamma");
   }

   const char* filename = data->filename;
   if (data->needEvaluation)
   {
      float dummyF = 0.5f;

      if (data->tokenFilename.IsValid())
         filename = data->tokenFilename.Resolve(NULL, dummyF, dummyF);
      else
      {
         GetSequenceData(data->filename, data->imageSequence);
         filename = ResolveSequenceAtFrame(NULL, data->filename, 0, data->imageSequence, true);
      }

   }
   else
      data->textureHandle = AiTextureHandleCreate(filename, data->color_space);

   if (data->needEvaluation)
      AiFree((void*)filename);

   data->fstop = powf(2.0f, AiNodeGetFlt(node, "Exposure"));
   // effects enabled ? 0 == enabled, 1 == disabled
   bool effectsEnabled = AiNodeGetInt(node, "ImageDefinitionType") == 0;

   float hue=0.0f, saturation=100.0f, gain=100.0f, brightness=0.0f;
   bool  grayscale=false;

   if (effectsEnabled)
   {
      hue        = AiNodeGetFlt(node, "Hue");
      saturation = AiNodeGetFlt(node, "Saturation");
      gain       = AiNodeGetFlt(node, "Gain");
      brightness = AiNodeGetFlt(node, "Brightness");
      grayscale  = AiNodeGetBool(node, "GrayScale");
   }

   data->applyColorCorrection = effectsEnabled && 
                                (grayscale || hue != 0.0f || saturation != 100.f || gain != 100.0f || brightness != 0.0f);

   if (data->applyColorCorrection)
   {
      data->hue = fmod(-hue, 360.0f);
      data->saturation = grayscale ? 0.0f : saturation / 100.0f;
      data->gain = gain / 100.0f;
      data->brightness = brightness / 100.0f;
   }

   if (effectsEnabled) // cropping and flip
   {
      data->xmin = AiNodeGetFlt(node, "Xmin");
      data->xmax = AiNodeGetFlt(node, "Xmax");
      data->ymin = AiNodeGetFlt(node, "Ymin");
      data->ymax = AiNodeGetFlt(node, "Ymax");
      data->flipx = AiNodeGetBool(node, "FlipX");
      data->flipy = AiNodeGetBool(node, "FlipY");
   }

   data->applyCroppingFlip = effectsEnabled && 
                             ( data->flipx || data->flipy || data->xmin != 0.0f || data->xmax != 1.0f || data->ymin != 0.0f || data->ymax != 1.0f);
}

node_finish
{
   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
   if(data->textureHandle)
      AiTextureHandleDestroy(data->textureHandle);
   delete data;
}

shader_evaluate
{
   sg->out.RGBA() = AI_RGBA_ZERO;

   // for UVs < 0, return black also for <tile> or <udim> (#1542)
   if (sg->u < 0.0f || sg->v < 0.0f)
      return;

   ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);

   bool udimmed = data->needEvaluation && data->tokenFilename.IsValid();
   // if we're above 1, and this is NOT a <tile> or <udim> filename, return black
   if ( (!udimmed) && ( (data->sWrap <= 0 && sg->u > 1.0f) || (data->tWrap <= 0 && sg->v > 1.0f) ) )
      return;

   BACKUP_SG_UVS;

   if (udimmed) // wrap by CLAMP if this is a <udim> texture
      data->tmap_params.wrap_s = data->tmap_params.wrap_t = AI_WRAP_CLAMP;
   else
   {
      data->tmap_params.wrap_s = data->sWrap > 0 ? data->sWrap - 1 : 0;
      data->tmap_params.wrap_t = data->tWrap > 0 ? data->tWrap - 1 : 0;
   }

   // flip and crop
   if (data->applyCroppingFlip)
   {
      sg->u = data->flipx ? AiLerp(sg->u, data->xmax, data->xmin) : AiLerp(sg->u, data->xmin, data->xmax);
      sg->v = data->flipy ? AiLerp(sg->v, data->ymax, data->ymin) : AiLerp(sg->v, data->ymin, data->ymax);
      // also, multiplying the uv derivatives by the LERP derivative
      float uDelta = data->flipx ? data->xmin - data->xmax : data->xmax - data->xmin;
      float vDelta = data->flipy ? data->ymin - data->ymax : data->ymax - data->ymin;
      sg->dudx*= uDelta;
      sg->dudy*= uDelta;
      sg->dvdx*= vDelta;
      sg->dvdy*= vDelta;
   }

   AtRGBA color = AI_RGBA_ZERO;

   if (data->needEvaluation) // Deprecated lookup, needed with variable texture name
   {
      if (data->tokenFilename.IsValid())
      {
         // get the <udim>-ed filename, out of the current u,v
         AtString filename(data->tokenFilename.Resolve(sg, sg->u, sg->v));
         if (!filename.empty())
            color = AiTextureAccess(sg, filename, data->color_space, data->tmap_params);
      }
      else
      {
         int frame;
         float framef;

         if (AiUDataGetInt(data->timeSource, frame))
         {
            AtString filename(ResolveSequenceAtFrame(sg, data->filename, frame, data->imageSequence));
            if (!filename.empty())
               color = AiTextureAccess(sg, filename, data->color_space, data->tmap_params);
         }
         else if (AiUDataGetFlt(data->timeSource, framef))
         {
            frame = (int)floor(framef);
            framef-= floor(framef);
            AtString filename0(ResolveSequenceAtFrame(sg, data->filename, frame, data->imageSequence));
            AtString filename1(ResolveSequenceAtFrame(sg, data->filename, frame + 1, data->imageSequence));
            if (!filename0.empty() && !filename1.empty())
            {
               AtRGBA c0 = AiTextureAccess(sg, filename0, data->color_space, data->tmap_params);
               AtRGBA c1 = AiTextureAccess(sg, filename1, data->color_space, data->tmap_params);
               color = AiLerp(framef, c0, c1);
            }
         }
      }
   }
   else
      color = AiTextureHandleAccess(sg, data->textureHandle, data->tmap_params);

   if (data->gamma != 1.0f) // User Gamma case only
      RGBAGamma(&color, data->gamma);

   if (data->applyColorCorrection)
   {  // hue, saturation, gain, brightness
      color = TransformHSV(color, data->hue, data->saturation, data->gain);
      color.r+= data->brightness;
      color.g+= data->brightness;
      color.b+= data->brightness;
   }

   // It's a tough call when is the appropriate time to do this - it should definitely be after we've got the image to linear space
   // Though perhaps it should be before we apply brightness, but after the HSV color transform?
   sg->out.RGBA().r = color.r * data->fstop;
   sg->out.RGBA().g = color.g * data->fstop;
   sg->out.RGBA().b = color.b * data->fstop;
   sg->out.RGBA().a = color.a;

   RESTORE_SG_UVS;
}


