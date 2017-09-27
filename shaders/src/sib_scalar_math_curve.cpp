/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "FCurve.h"

AI_SHADER_NODE_EXPORT_METHODS(SIBScalarMathCurveMethods);

enum SIBScalarMathCurveParams
{
   p_input,
   p_curve,
};

node_parameters
{
   AiParameterFlt  ("input", 0.0f);
   AiParameterArray("curve", AiArrayAllocate(1, 1, AI_TYPE_FLOAT));
}

node_initialize
{
   AiNodeSetLocalData(node, new CFCurve());
}

node_update
{
   CFCurve *fc = (CFCurve*)AiNodeGetLocalData(node);
   AtArray *curve = AiNodeGetArray(node, "curve");
   fc->Init(curve);
}

node_finish
{
   delete (CFCurve*)AiNodeGetLocalData(node);
}

shader_evaluate
{
   CFCurve *fc = (CFCurve*)AiNodeGetLocalData(node);
   float input = AiShaderEvalParamFlt(p_input);
   sg->out.FLT() = fc->Eval(input);
}
