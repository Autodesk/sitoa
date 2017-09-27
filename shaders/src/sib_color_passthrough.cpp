/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBColorPassThroughMethods);

enum SIBColorPassThroughParams
{
   p_input,
   p_channel1,
   p_channel2,
   p_channel3,
   p_channel4,
   p_channel5,
   p_channel6,
   p_channel7,
   p_channel8
};

node_parameters
{
   AiParameterRGBA( "input"   , 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "channel1", 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "channel2", 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "channel3", 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "channel4", 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "channel5", 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "channel6", 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "channel7", 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterRGBA( "channel8", 0.0f, 0.0f, 0.0f, 0.0f );
}

node_initialize
{
   AiNodeSetLocalData(node, AiMalloc(8 * sizeof(bool)));
}

node_update
{
   bool *activeChannel =  (bool*)AiNodeGetLocalData(node);
   // check if a channel must be evaluated (#1097)
   activeChannel[0] = AiNodeGetLink(node, "channel1") != NULL;
   activeChannel[1] = AiNodeGetLink(node, "channel2") != NULL;
   activeChannel[2] = AiNodeGetLink(node, "channel3") != NULL;
   activeChannel[3] = AiNodeGetLink(node, "channel4") != NULL;
   activeChannel[4] = AiNodeGetLink(node, "channel5") != NULL;
   activeChannel[5] = AiNodeGetLink(node, "channel6") != NULL;
   activeChannel[6] = AiNodeGetLink(node, "channel7") != NULL;
   activeChannel[7] = AiNodeGetLink(node, "channel8") != NULL;
}

node_finish
{
   AiFree(AiNodeGetLocalData(node));
}

shader_evaluate
{
   bool *activeChannel = (bool*)AiNodeGetLocalData(node);
   // Users often use these parameters to store values into AOVs that should not affect the shader's output
   for (int i = 0; i < 8; i++)
      if (activeChannel[i])
         AiShaderEvalParamRGBA(p_channel1 + i);   

   sg->out.RGBA() = AiShaderEvalParamRGBA(p_input);
}
