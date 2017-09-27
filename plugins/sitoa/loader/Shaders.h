/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "renderer/AtNodeLookup.h"

#include <xsi_material.h>
#include <xsi_primitive.h>
#include <xsi_shader.h>
#include <xsi_status.h>
#include <xsi_string.h>

#include <ai_nodes.h>

#include <map>

using namespace std;
using namespace XSI;

// Defines that indicates what network shaders will parse on LoadMaterial()
#define LOAD_MATERIAL_SURFACE          0
#define LOAD_MATERIAL_DISPLACEMENT     1
#define LOAD_MATERIAL_ENVIRONMENT      2

#define RECURSE_TRUE true
#define RECURSE_FALSE false

///////////////////////////////
///////////////////////////////
// The map that we use to cache the exported shader nodes, instead of using 
// the one used for other node types RenderInstance.m_exportedNodes as we did before.
// This is handier to just manage the shaders
///////////////////////////////
///////////////////////////////

class CShaderMap
{
private:
   map <AtShaderLookupKey, AtNode*> m_map;
public:
   // Default constructor
   CShaderMap()
   {}

   // Default destructor
   ~CShaderMap()
   {
      m_map.clear();
   }

   // Push into map by AtNode and key
   void Push(AtNode *in_shader, AtShaderLookupKey in_key);
   // Push into map by Softimage shader, Arnold shader, frame time
   void Push(ProjectItem in_xsiShader, AtNode *in_shader, double in_frame);
   // Find the node in the map
   AtNode* Get(const ProjectItem in_xsiShader, double in_frame);
   // Erase a shader node from the map
   void EraseExportedNode(AtNode *in_shader);
   // Update all the shaders in the scene, when in flythrough mode
   void FlythroughUpdate();
   // clear the map
   void Clear();
};



///////////////////////////////
///////////////////////////////
// This class stores the name and uv wrapping settings of a given ICE texture projection attribute
///////////////////////////////
///////////////////////////////
class CIceTextureProjectionAttribute
{
public:
   CString m_name;
   bool m_uWrap, m_vWrap;

   CIceTextureProjectionAttribute() : m_uWrap(false), m_vWrap(false)
   {}
   // copy constructor
   CIceTextureProjectionAttribute(const CIceTextureProjectionAttribute &in_arg) : m_name(in_arg.m_name), m_uWrap(in_arg.m_uWrap), m_vWrap(in_arg.m_vWrap)
   {}
   // Construct by attribute name
   CIceTextureProjectionAttribute(const CString &in_name);
   // Evaluate the wrapping attributes, if available. 
   void EvaluateWrapping(Geometry &in_xsiGeo);
};


// Begins to parse the whole networking shading attached to a mesh into Arnold
AtNode* LoadMaterial(const Material &in_material, unsigned int in_connection, double in_frame, const CRef &in_ref=CRef());
// Load a Shader into Arnold
AtNode* LoadShader(const Shader &in_shader, double in_frame, const CRef &in_ref, bool in_recursively);
// Load the shaders attached on Pass Shader Stack like sky, sky_hdri, volume_scatering, etc
CStatus LoadPassShaders(double in_frame, bool in_selectionOnly = false);
// Parse XSI ImageClip Node. (Internally on XSI is not a shader, but for us it is)
AtNode* LoadImageClip(ImageClip2 &in_xsiImageClip, double in_frame);
// Parse all Layers from Shader 
CStatus LoadTextureLayers(AtNode* shaderNode, const Shader &xsiShader, double frame, bool recursively);
// Returns the previous layer that is using the specified target param to link with other layer
AtNode* GetPreviousLayerPort(const CRefArray &textureLayersArray, const CString &targetParamName, LONG layerIdx, bool soloActive, double frame);
// Return the Texture_Projection_Def (where, for instance, the srt of the proj resides) of the texture projection property
Primitive GetTextureProjectionDefFromTextureProjection(const ClusterProperty &in_textureProjection);
// Set the wrappings settings of the projections used in the shaders, and manage the parameters with instance values
CStatus SetWrappingAndInstanceValues(AtNode* in_shapeNode, const CRef &in_objRef, const Material &in_material, const CRefArray &in_uvsArray, vector <CIceTextureProjectionAttribute> *in_iceTextureProjectionAttributes, double in_frame);
// Set the wrappings settings of the projections used in a shader
void SetWrappingSettings(AtNode* in_shapeNode, AtNode* in_shaderNode, const CRef &in_objRef, const Shader &in_xsiShader, const CRefArray &in_uvsArray, vector <CIceTextureProjectionAttribute> *in_iceTextureProjectionAttributes, double in_frame);
// Export a clip's parameters as constant user data for the input shader node
void ExportTextureMapAsUserData(AtNode *in_shaderNode, CString in_shapeNodeName, CString in_map, const CRef &in_ref, const CRefArray &in_uvsArray, double in_frame);
// Export the values of the parameters with instance values (map_lookup_*) as user data
void ExportInstanceValueAsUserData(AtNode* in_shapeNode, AtNode* in_shaderNode, const CRef &in_objRef, const Shader &in_xsiShader, const CRefArray &in_uvsArray, const CString &in_paramName, double in_frame);

