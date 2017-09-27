/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "FCurve.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBColorMathCurveMethods);

enum SIBColorMathCurveParams 
{
   p_input,
   p_rcurve,
   p_gcurve,
   p_bcurve,
   p_acurve,
   p_use_alpha
};

node_parameters
{
   AiParameterRGBA ("input",     0.5f, 0.5f, 0.5f, 0.5f);
   AiParameterArray("rcurve",    AiArrayAllocate(1, 1, AI_TYPE_FLOAT));
   AiParameterArray("gcurve",    AiArrayAllocate(1, 1, AI_TYPE_FLOAT));
   AiParameterArray("bcurve",    AiArrayAllocate(1, 1, AI_TYPE_FLOAT));
   AiParameterArray("acurve",    AiArrayAllocate(1, 1, AI_TYPE_FLOAT));
   AiParameterBool ("use_alpha", false );
}

class CColorMathCurveLocalData 
{
public:
   CFCurve rFc, gFc, bFc, aFc;
   bool    use_alpha;

   CColorMathCurveLocalData()
   {}

   void Init(AtArray *ra, AtArray *ga, AtArray *ba, AtArray *aa, bool in_use_alpha)
   {
      rFc.Init(ra);
      gFc.Init(ga);
      bFc.Init(ba);

      use_alpha = in_use_alpha;
      // if use_alpha is off, the plugin does not export the alpha curve, 
      // so acurve (which exists as part of the shader node) defaults as
      // defined in node_parameters, so to size=1
      if (use_alpha && AiArrayGetNumElements(aa) > 1)
         aFc.Init(aa);
   }

   ~CColorMathCurveLocalData()
   {}
};

node_initialize 
{
   AiNodeSetLocalData(node, new CColorMathCurveLocalData());
}

node_update
{
   CColorMathCurveLocalData* data = (CColorMathCurveLocalData*)AiNodeGetLocalData(node);
   AtArray *rcurve = AiNodeGetArray(node, "rcurve");
   AtArray *gcurve = AiNodeGetArray(node, "gcurve");
   AtArray *bcurve = AiNodeGetArray(node, "bcurve");
   AtArray *acurve = AiNodeGetArray(node, "acurve");
   data->Init(rcurve, gcurve, bcurve, acurve, AiNodeGetBool(node, "use_alpha"));
}

node_finish
{
   delete (CColorMathCurveLocalData*)AiNodeGetLocalData(node);
}

shader_evaluate
{
   CColorMathCurveLocalData* data = (CColorMathCurveLocalData*)AiNodeGetLocalData(node);

   AtRGBA input = AiShaderEvalParamRGBA(p_input);

   sg->out.RGBA().r = data->rFc.Eval(input.r);
   sg->out.RGBA().g = data->gFc.Eval(input.g);
   sg->out.RGBA().b = data->bFc.Eval(input.b);
   sg->out.RGBA().a = data->use_alpha ? data->aFc.Eval(input.a) : 1.0f;
}
