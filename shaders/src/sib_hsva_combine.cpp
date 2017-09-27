/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include "color_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBHSVACombineMethods);

enum SIBHSVACombineParams
{
   p_hue,
   p_sat,
   p_val,
   p_alpha
};

node_parameters
{
   AiParameterFlt ( "hue"   , 0.75f );
   AiParameterFlt ( "sat"   , 1.0f );
   AiParameterFlt ( "val"   , 1.0f );
   AiParameterFlt ( "alpha" , 1.0f );
}

node_initialize {}
node_update {}
node_finish {}

shader_evaluate
{
   AtRGBA  HSVcolor;
   HSVcolor.r = AiShaderEvalParamFlt(p_hue);
   HSVcolor.g = AiShaderEvalParamFlt(p_sat);
   HSVcolor.b = AiShaderEvalParamFlt(p_val);
   HSVcolor.a = AiShaderEvalParamFlt(p_alpha);

   sg->out.RGBA() = HSVtoRGBA(HSVcolor);
}
