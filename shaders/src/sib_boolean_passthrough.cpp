/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>

AI_SHADER_NODE_EXPORT_METHODS(SIBBooleanPassThroughMethods);

enum SIBBooleanPassThroughParams
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
   AiParameterBool( "input"   , false );
   AiParameterBool( "channel1", false );
   AiParameterBool( "channel2", false );
   AiParameterBool( "channel3", false );
   AiParameterBool( "channel4", false );
   AiParameterBool( "channel5", false );
   AiParameterBool( "channel6", false );
   AiParameterBool( "channel7", false );
   AiParameterBool( "channel8", false );
}

node_initialize
{
   AiNodeSetLocalData(node, AiMalloc(8*sizeof(bool)));
}

node_update
{
   bool *activeChannel = (bool*)AiNodeGetLocalData(node);
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
         AiShaderEvalParamBool(p_channel1 + i);

   sg->out.BOOL() = AiShaderEvalParamBool(p_input);
}
