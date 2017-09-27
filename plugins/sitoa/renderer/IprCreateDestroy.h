/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "loader/Hairs.h"
#include "loader/Polymeshes.h"
#include "renderer/Renderer.h"

#include <xsi_application.h>
#include <xsi_model.h>


////////////////////////////////////////////////////////////////////////////
// This methods to handle the creation and destruction of objects and lights 
// during ipr
////////////////////////////////////////////////////////////////////////////


class CIprCreateDestroy
{
public:
   // Lights destruction

   // Destroy (during ipr) the light nodes that were created for an array of Softimage lights
   void DestroyLights(CValueArray in_value, double in_frame);
   // Destroy (during ipr) a vector of light nodes, each element generated because of a Softimage light instance
   void DestroyInstancedLights(vector <AtNode*> &in_nodes, double in_frame);

   // Object destruction

   // Destroy (during ipr) an object
   void DestroyObjects(CValueArray in_value, double in_frame);
   // Destroy the nodes associated to a CGroup.
   void DestroyGroupObjects(CStringArray &in_names, double in_frame);

   // Lights creation

   // Create (during ipr) a set of light nodes for a newly created Softimage light set (for example because the user loaded a model made of lights)
   void CreateLights(const CRefArray &in_lights, double in_frame);
   // Create (during ipr) a set of mesh objects
   void CreateObjects(const CRefArray &in_objects, double in_frame);
   // void CreateModelInstances(const CRefArray &in_objects, const Property &in_arnoldOptions, double in_frame);
   // Create (during ipr) a set of hair objects
   void CreateHairs(const CRefArray &in_objects, double in_frame);

private:
   // Lights destruction

   // Destroy (during ipr) all the filter nodes of a light
   void DestroyLightFilters(AtNode *in_node);
   // Destroy (during ipr) the light nodes that were created for a Softimage light
   void DestroyLight(CValue in_value, double in_frame);
   // Destroy (during ipr) a light node that was generated because of a Softimage light instance
   void DestroyInstancedLight(AtNode *in_node);

   // Object destruction

   // Set to NULL the mesh pointer of all the mesh lights pointing to the input mesh node
   void ResetMeshLightsObject(AtNode *in_meshNode);

   // Destroy (during ipr) an array of object
   void DestroyObject(CValue in_value, double in_frame);
};


