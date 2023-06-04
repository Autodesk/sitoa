/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsShader.h"
#include "renderer/IprShader.h"
#include "renderer/Renderer.h"
#include "loader/Operators.h"

#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_passcontainer.h>
#include <xsi_pass.h>
#include <xsi_shaderarrayparameter.h>

// Update the operators connected to a RenderPass into Arnold
//
// @return CStatus::OK if all went well, else the error code
//
CStatus UpdatePassOperator(const Pass &in_pass, double in_frame)
{
   CStatus status(CStatus::OK);

   CRef outputStackRef;
   outputStackRef.Set(in_pass.GetFullName() + L".OutputShaderStack");
   ShaderArrayParameter arrayParam = ShaderArrayParameter(outputStackRef);

   AtNode* options = AiUniverseGetOptions(NULL);

   if (arrayParam.GetCount() > 0)
   {
      Shader operatorShader;
      for (LONG i=0; i<arrayParam.GetCount(); i++)
      {
         Parameter param = Parameter(arrayParam[i]);
         Shader outputShader = GetConnectedShader(param);
         if (outputShader.IsValid())
         {
            // find the first 'operator' shader, dummy shader node for supporting arnold operators
            if (GetShaderNameFromProgId(outputShader.GetProgID()) == L"operator")
            {
               operatorShader = outputShader.GetRef();
               break;
            }
         }
      }

      if (operatorShader.IsValid())
      {
         // get what's connected to that dummy shader's operator parameter
         Parameter operatorParam = operatorShader.GetParameter(L"operator");
         operatorShader = GetShaderFromSource(operatorParam.GetSource());
         if (operatorShader.IsValid())
         {
            AtNode* operatorNode = UpdateShader(operatorShader, in_frame);

            if (operatorNode)
               CNodeSetter::SetPointer(options, "operator", operatorNode);
         }
         else
            CNodeSetter::SetPointer(options, "operator", NULL);
      }
      else
         CNodeSetter::SetPointer(options, "operator", NULL);
   }
   return status;
}
