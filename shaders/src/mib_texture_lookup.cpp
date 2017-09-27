/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include "shader_utils.h"

AI_SHADER_NODE_EXPORT_METHODS(MIBTextureLookupMethods);

enum MIBTextureLookupParams 
{
   p_tex,
   p_coord
};

node_parameters
{
   AiParameterRGBA( "tex"   , 0.0f, 0.0f, 0.0f, 0.0f );
   AiParameterVec ( "coord" , 0.0f, 0.0f, 0.0f );
}

node_initialize{}
node_update{}
node_finish{}

shader_evaluate
{
   BACKUP_SG_UVS;

   AtVector coord = AiShaderEvalParamVec(p_coord);
   sg->u = coord.x;
   sg->v = coord.y;
   sg->dudx = sg->dudy = sg->dvdx = sg->dvdy = 0.0f;

   sg->out.RGBA() = AiShaderEvalParamRGBA(p_tex);  

   RESTORE_SG_UVS;
}
