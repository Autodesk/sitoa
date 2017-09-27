/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(Math_Int_FmodMethods);

enum FModParams
{
   p_left,
   p_right
};

node_parameters
{
   AiParameterInt("left",  0);
   AiParameterInt("right", 0);
}

node_initialize {}
node_update {}
node_finish {}

shader_evaluate
{
   int left  = AiShaderEvalParamInt(p_left);
   int right = AiShaderEvalParamInt(p_right);
   if (right == 0)
      right = 1;
   sg->out.INT() = left % right;
}
