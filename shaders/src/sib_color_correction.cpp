/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "color_utils.h"
#include "ai_math.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBColorCorrectionMethods);

enum SIBColorCorrectionParams
{
   p_input,
   p_gamma,
   p_contrast,
   p_hue,
   p_saturation,
   p_level
};

node_parameters
{
   AiParameterRGBA( "input"         , 1.0f, 1.0f, 1.0f, 0.0f );
   AiParameterFlt ( "gamma"         , 1.0f );
   AiParameterFlt ( "contrast"      , 0.5f );
   AiParameterFlt ( "hue"           , 0.0f );
   AiParameterFlt ( "saturation"    , 0.0f );
   AiParameterFlt ( "level"         , 0.0f );
}

node_initialize{}
node_update{}
node_finish{}

shader_evaluate
{
   AtRGBA input     = AiShaderEvalParamRGBA(p_input);
   float gamma      = AiShaderEvalParamFlt(p_gamma);
   float contrast   = AiShaderEvalParamFlt(p_contrast);
   float hue        = AiShaderEvalParamFlt(p_hue);
   float saturation = AiShaderEvalParamFlt(p_saturation);
   float level      = AiShaderEvalParamFlt(p_level);

   AtRGBA result = input;

    // Gamma Correction
   RGBAGamma(&result, gamma);
    // Contrast
   result.r = AiGain(result.r, contrast);
   result.g = AiGain(result.g, contrast);
   result.b = AiGain(result.b, contrast);
   // HLS Corrections
   if (hue != 0.0f || saturation != 0.0f || level != 0.0f)
   {
      AtRGBA hls = RGBAtoHLS(result);
      // H
      hls.r = hls.r + hue/360.0f;
      // L
      hls.g += level;
      hls.g = AiClamp(hls.g , 0.0f, 1.0f);
      // S
      hls.b += saturation;
      hls.b = AiClamp(hls.b, 0.0f, 1.0f);

      result = HLStoRGBA(hls);
   }

   sg->out.RGBA() = result;
}
