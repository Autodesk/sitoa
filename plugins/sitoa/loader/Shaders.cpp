/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCommon.h"
#include "common/ParamsShader.h"
#include "common/Tools.h"
#include "loader/Loader.h"
#include "loader/PathTranslator.h"
#include "loader/Shaders.h"
#include "renderer/IprShader.h"
#include "renderer/Renderer.h"
#include "renderer/RenderTree.h"

#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_shaderarrayparameter.h>

/////////////////////////////////////
/////////////////////////////////////
// CShaderMap
/////////////////////////////////////
/////////////////////////////////////

// Push into map by AtNode and key
//
// @param in_shader   the Arnold shader to push
// @param in_key      key
//
void CShaderMap::Push(AtNode *in_shader, AtShaderLookupKey in_key)
{
   m_map.insert(pair<AtShaderLookupKey, AtNode*> (in_key, in_shader));
}


// Push into map by Softimage shader, Arnold shader, frame time
//
// @param in_xsiShader  the Softimage shader to push
// @param in_shader     the Arnold shader to push
// @param in_frame      the frame time
//
void CShaderMap::Push(ProjectItem in_xsiShader, AtNode *in_shader, double in_frame)
{
   Push(in_shader, AtShaderLookupKey(in_xsiShader.GetObjectID(), in_frame));
}


// Find the shader in the map
//
// @param in_xsiShader    the xsi shader to find
// @param in_frame        the frame time
//
// @return  found AtNode* shader, or NULL if not found
//
AtNode* CShaderMap::Get(const ProjectItem in_xsiShader, double in_frame)
{
   map <AtShaderLookupKey, AtNode*>::iterator iter = m_map.find(AtShaderLookupKey(in_xsiShader.GetObjectID(), in_frame));
   if (iter != m_map.end())
      return iter->second;
   return NULL;
}

// Erase a shader node from the map
//
// @param in_shader    the shader node to be erased
//
void CShaderMap::EraseExportedNode(AtNode *in_shader)
{
   map <AtShaderLookupKey, AtNode*>::iterator it;
   for (it=m_map.begin(); it!=m_map.end(); it++)
   {
      if (it->second == in_shader)
      {
         m_map.erase(it);
         break;
      }
   }
}


// Update all the shaders in the scene, when in flythrough mode
//
void CShaderMap::FlythroughUpdate()
{
   map <AtShaderLookupKey, AtNode*>::iterator it;
   for (it=m_map.begin(); it!=m_map.end(); it++)
   {
      Shader shader(Application().GetObjectFromID(it->first.m_id));
      if (!shader.IsValid())
         continue;

      UpdateShader(shader, GetRenderInstance()->GetFrame());
   }
}


// Clear the map
//
void CShaderMap::Clear()
{
   m_map.clear();
}


/////////////////////////////////////
/////////////////////////////////////
// CIceTextureProjectionAttribute: this class stores the name and uv wrapping settings of a given ICE texture projection attribute
/////////////////////////////////////
/////////////////////////////////////

// Construct by attribute name
//
// @param in_name  The ICE attribute name
//
CIceTextureProjectionAttribute::CIceTextureProjectionAttribute(const CString &in_name)
{
   m_uWrap = m_vWrap = false;
   m_name = in_name;
}


// Evaluate the wrapping attributes, if available. 
// As from the doc, they are named by the attribute name followed by "_u_wrap" and "_v_wrap"
// If the attrubutes are not available, the wrapping members stay false, as from the constructors
//
// @param in_xsiGeo  The geometry
//
void CIceTextureProjectionAttribute::EvaluateWrapping(Geometry &in_xsiGeo)
{
   CIceAttribute wrapAttribute;

   wrapAttribute = in_xsiGeo.GetICEAttributeFromName(m_name + L"_u_wrap");
   if (wrapAttribute.IsValid())
      if (wrapAttribute.Update())
         m_uWrap = wrapAttribute.m_bData[0];

   wrapAttribute = in_xsiGeo.GetICEAttributeFromName(m_name + L"_v_wrap");
   if (wrapAttribute.IsValid())
      if (wrapAttribute.Update())
         m_vWrap = wrapAttribute.m_bData[0];
}


AtNode* LoadMaterial(const Material &in_material, unsigned int in_connection, double in_frame, const CRef &in_ref)
{
   AtNode* mainShaderNode = NULL;
   if (in_connection == LOAD_MATERIAL_SURFACE)
   {
      // Load shaders attached to surface param material
      Shader surfaceShader = GetConnectedShader(ParAcc_GetParameter(in_material, L"surface"));

      if (surfaceShader.IsValid())
      {
         mainShaderNode = GetRenderInstance()->ShaderMap().Get(surfaceShader, in_frame);
         // If not found we have to parse the whole material
         if (!mainShaderNode)
            mainShaderNode = LoadShader(surfaceShader, in_frame, in_ref, RECURSE_FALSE);

         return mainShaderNode;
      }
   }
   else if (in_connection == LOAD_MATERIAL_DISPLACEMENT)
   {
      Shader displacementShader = GetConnectedShader(ParAcc_GetParameter(in_material, L"displacement"));
      if (displacementShader.IsValid())
      {
         mainShaderNode = GetRenderInstance()->ShaderMap().Get(displacementShader, in_frame);
         if (!mainShaderNode)
            mainShaderNode = LoadShader(displacementShader, in_frame, in_ref, RECURSE_FALSE);

         return mainShaderNode;
      }
   }
   else if (in_connection == LOAD_MATERIAL_ENVIRONMENT)
   {
      Shader environmentShader = GetConnectedShader(ParAcc_GetParameter(in_material, L"environment"));
      if (environmentShader.IsValid())
      {
         mainShaderNode = GetRenderInstance()->ShaderMap().Get(environmentShader, in_frame);
         if (!mainShaderNode)
            mainShaderNode = LoadShader(environmentShader, in_frame, in_ref, RECURSE_FALSE);

         return mainShaderNode;
      }
   }

   return NULL;
}


AtNode* LoadShader(const Shader &in_shader, double in_frame, const CRef &in_ref, bool in_recursively)
{
   // Let's make sure we get the shader from its library to get the name like Sources.Material...
   Shader xsiShader(Application().GetObjectFromID(CObjectUtilities().GetId(in_shader)));

   CString shaderName = GetShaderNameFromProgId(xsiShader.GetProgID());
   if (shaderName == L"vector_displacement") // vector_displacement is just an alias for vector_map, showing with float output
      shaderName = "vector_map";             // to make it pluggable into the displacement port of the material
   CString shaderFullName = xsiShader.GetFullName();

   // this shader was already found not being installed, just quit.
   // So, a missing shader error is logged only once (#1263)
   if (GetRenderInstance()->MissingShaderMap().Find(shaderName))
      return NULL;

   // The same shader could be connected to different parameters of a node so we dont need to parse it again
   bool newNode(false);

   AtNode* shaderNode = GetRenderInstance()->ShaderMap().Get(xsiShader, in_frame);

   if (!shaderNode) // Not exported yet, we have to create it
   {
      shaderNode = AiNode(shaderName.GetAsciiString());
   
      if (!shaderNode)
      {
         GetMessageQueue()->LogMsg(L"[sitoa]: Unable to load " + shaderName + L" from the Arnold plugins (first occurrence: " + shaderFullName + L")", siErrorMsg);
         GetRenderInstance()->MissingShaderMap().Add(shaderName);
         return shaderNode;
      }
      else // new node, push it into the shaders map as usual
         GetRenderInstance()->ShaderMap().Push(xsiShader, shaderNode, in_frame);

      CString shaderName = CStringUtilities().MakeSItoAName(xsiShader, in_frame, L"", true);
      CNodeUtilities().SetName(shaderNode, shaderName);

      newNode = true;
   }

   if (newNode || (shaderNode!=NULL && in_recursively))
   {
      LoadShaderParameters(shaderNode, xsiShader.GetParameters(), in_frame, in_ref, in_recursively);
      LoadTextureLayers(shaderNode, xsiShader, in_frame, in_recursively);
   }

   return shaderNode;
}


CPathString GetClipSourceFileName(ImageClip2 &in_xsiImageClip, double in_frame)
{
   bool timeSource = false;
   CString attributeName = ParAcc_GetValue(in_xsiImageClip, L"TimeSource", in_frame).GetAsText();

   if (!attributeName.IsEmpty())
      timeSource = true;

   // Getting evaluated frame if no TimeSource given.
   CString sourceFileName = ParAcc_GetValue(in_xsiImageClip, L"SourceFileName", DBL_MAX).GetAsText();

   if (!timeSource)
      sourceFileName = in_xsiImageClip.GetFileName();

   bool substituteTx = GetRenderOptions()->m_use_existing_tx_files;

   // Get FileName from ImageClip (if the image is a sequence XSI will give us the correct filename)
   if (GetRenderOptions()->m_save_texture_paths) // using absolute paths
   {
      // Translate path
      CPathString result(CPathTranslator::TranslatePath(sourceFileName.GetAsciiString(), substituteTx));
      return result;
   }

   vector <CPathString> texturesSearchPaths;
   vector <CPathString>::iterator searchPathsIt;

   if (GetRenderInstance()->GetTexturesSearchPath().GetPaths(texturesSearchPaths))
   {
      // translate the full texture path, also substituting .tx, in case
      const char *translatedPath = CPathTranslator::TranslatePath(sourceFileName.GetAsciiString(), substituteTx);
      bool isPathTranslatorInitialized = CPathTranslator::IsInitialized(); // using linktab?
      unsigned int pathTranslatorMode = CPathTranslator::GetTranslationMode(); // w2l (default) or l2w?

      bool windowsSlash(false); // we always use "/", except if on linux and exporting for windows (what a mess)
      if (CUtils::IsLinuxOS() && isPathTranslatorInitialized && pathTranslatorMode == TRANSLATOR_LINUX_TO_WIN)
         windowsSlash = true;
      // loop the texture search paths and break on the first successfully conversion to relative path.
      for (searchPathsIt = texturesSearchPaths.begin(); searchPathsIt != texturesSearchPaths.end(); searchPathsIt++)
      {
         // also translate the texture_path, which is a directory, so don't bother about the tx file existence blabla
         const char *searchPath = CPathTranslator::TranslatePath(searchPathsIt->GetAsciiString(), false);
         // #1162
         CPathString fullPath(translatedPath), relDir(searchPath);

         CPathString relPath = fullPath.GetRelativeFilename(relDir, windowsSlash);

         if (relPath != L"") // relative path replacement successfull
            return relPath;
      }
   }

   // the conversion to relative path failed, return the plain file name
   CStringArray pathParts(sourceFileName.Split(CUtils::Slash()));
   return pathParts[pathParts.GetCount()-1];
}


AtNode* LoadImageClip(ImageClip2 &in_xsiImageClip, double in_frame)
{
   // The same shader could be connected to different parameters of a node
   // so we dont need to parse again
   AtNode* shaderNode = GetRenderInstance()->ShaderMap().Get(in_xsiImageClip.GetRef(), in_frame);

   if (shaderNode)
      return shaderNode;

   // not found, let's create it
   shaderNode = AiNode("sib_image_clip");
   if (!shaderNode)
      return NULL;

   // new node, push it into the shaders map
   GetRenderInstance()->ShaderMap().Push(in_xsiImageClip, shaderNode, in_frame);

   CString shaderName = CStringUtilities().MakeSItoAName(in_xsiImageClip, in_frame, L"", true);
   CNodeUtilities().SetName(shaderNode, shaderName);

   CRef tempRef; // no CRef available for the object

   LoadShaderParameters(shaderNode, in_xsiImageClip.GetParameters(), in_frame, tempRef, RECURSE_FALSE);

   //Julian Dubuisson : initial patch for #1471, texture_options
   CRefArray textureProperties = in_xsiImageClip.GetProperties();
   Property textureOptionsProperty;
   textureProperties.Find(L"arnold_texture_options", textureOptionsProperty);

   int  filter, mipmapBias(0), sWrap(0), tWrap(0);
   bool swapST(false);
   if (textureOptionsProperty.IsValid())
   {
      filter     = (int) ParAcc_GetValue(textureOptionsProperty, L"filter", in_frame);
      mipmapBias = (int) ParAcc_GetValue(textureOptionsProperty, L"mipmap_bias", in_frame);
      swapST     = (bool)ParAcc_GetValue(textureOptionsProperty, L"swap_uv", in_frame);
      sWrap      = (int) ParAcc_GetValue(textureOptionsProperty, L"u_wrap", in_frame);
      tWrap      = (int) ParAcc_GetValue(textureOptionsProperty, L"v_wrap", in_frame);
   }
   else
      filter = GetRenderOptions()->m_texture_filter;

   // setting the data that will be managed by sib_image_clip.cpp
   CNodeSetter::SetInt (shaderNode, "filter",      filter);
   CNodeSetter::SetInt (shaderNode, "mipmap_bias", mipmapBias);
   CNodeSetter::SetBoolean(shaderNode, "swap_st",     swapST);
   // note that 0 here mean "no wrap",
   CNodeSetter::SetInt (shaderNode, "s_wrap",      sWrap);
   CNodeSetter::SetInt (shaderNode, "t_wrap",      tWrap);
   // end of #1471

   // get the name of the TimeSource attribute
   CPathString sourceFileName = GetClipSourceFileName(in_xsiImageClip, in_frame);
   CNodeSetter::SetString(shaderNode, "SourceFileName", sourceFileName.GetAsciiString());

   return shaderNode;
}


CStatus LoadPassShaders(double in_frame, bool in_selectionOnly)
{   
   CStatus status;

   if (in_selectionOnly)
      return CStatus::OK;

   Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());

   CString suffix = L".Item";
   CRef envParamRef;
   envParamRef.Set(pass.GetFullName() + L".EnvironmentShaderStack" + suffix);
   Parameter passParam(envParamRef);
   AtNode* options = AiUniverseGetOptions();

   Shader backgroundShader = GetConnectedShader(passParam);
   if (backgroundShader.IsValid())
   {            
      AtNode* shaderNode = LoadShader(backgroundShader, in_frame, pass.GetRef(), RECURSE_FALSE);
      // If we have added the shader we will link it
      if (shaderNode)
         CNodeSetter::SetPointer(options, "background", shaderNode);
   }         
   
   // Support for 'AOV shaders' putting this into 'output' shader type
   CRef outputStackRef;
   outputStackRef.Set(pass.GetFullName() + L".OutputShaderStack");
   ShaderArrayParameter arrayParam = ShaderArrayParameter(outputStackRef);
   CRefArray outputShadersArray;

   if (arrayParam.GetCount() > 0)
   {   
      for (LONG i=0; i<arrayParam.GetCount(); i++)
      {
         passParam = Parameter(arrayParam[i]);
         Shader outputShader = GetConnectedShader(passParam);
         if (outputShader.IsValid())
         {
            outputShadersArray.Add(outputShader.GetRef());
         }
      }

      if (outputShadersArray.GetCount() > 0)
      {
         AtArray* aovShadersArray = AiArrayAllocate(outputShadersArray.GetCount(), 1, AI_TYPE_NODE);
         for (LONG i=0; i<outputShadersArray.GetCount(); i++)
         {
            Shader outputShader(outputShadersArray[i]);

            AtNode* shaderNode = UpdateShader(outputShader, in_frame);
            AiArraySetPtr(aovShadersArray, i, shaderNode);
         }
         AiNodeSetArray(options, "aov_shaders", aovShadersArray);
      }
   }

   CRef atmParamRef;
   atmParamRef.Set(pass.GetFullName() + L".VolumeShaderStack" + suffix);
   passParam = Parameter(atmParamRef);

   Shader atmosphereShader = GetConnectedShader(passParam);
   if (atmosphereShader.IsValid())
   {            
      AtNode* shaderNode = LoadShader(atmosphereShader, in_frame, pass.GetRef(), RECURSE_FALSE);
      CNodeSetter::SetPointer(options, "atmosphere", shaderNode);
   }         
   
   return status;
}


CStatus LoadTextureLayers(AtNode* shaderNode, const Shader &xsiShader, double frame, bool in_recursively)
{
   CStatus status;

   CRefArray textureLayersArray(xsiShader.GetTextureLayers());
   LONG nlayers = textureLayersArray.GetCount();
   bool soloActive = false;

   TextureLayer textureLayer;
   TextureLayerPort texturePort;

   // Preprocess to get "solo" information
   for (LONG i=0; i<nlayers && !soloActive; i++)
   {            
      textureLayer.SetObject(textureLayersArray[i]);
      soloActive = (bool)ParAcc_GetValue(textureLayer,L"solo", frame);   
   }

   for (LONG ilayer=0; ilayer<nlayers; ilayer++)
   {
      // Get texture layer 
      textureLayer.SetObject(textureLayersArray[ilayer]);
      bool solo = (bool) ParAcc_GetValue(textureLayer,L"solo", frame);                  

      // Ignoring layers 
      if (soloActive && !solo)
         continue;

      // Get texture layer ports
      CRefArray texturePortsArray = textureLayer.GetTextureLayerPorts();
         
      LONG nports = texturePortsArray.GetCount();
      for (LONG iport=0; iport<nports; iport++)
      {
         texturePort.SetObject(texturePortsArray[iport]);
         Parameter targetParam = texturePort.GetTarget();    

         // Sometimes the target is not valid (some inconsistencies in XSI)
         if (!targetParam.IsValid())
            continue;

         // New Shader (layer port)
         // Let's make up the name by hand, instead that CStringUtilities().MakeSItoAName, because here we insert
         // also the targetparam name
         CString layerName = textureLayer.GetFullName() + L"." + targetParam.GetName() + L".SItoA." + CString(CTimeUtilities().FrameTimes1000(frame));

         AtNode* layerNode = AiNodeLookUpByName(layerName.GetAsciiString());
         AtNode* previousLayerNode = GetPreviousLayerPort(textureLayersArray, targetParam.GetName(), ilayer, soloActive, frame);

         if (layerNode == NULL)
            layerNode = AiNode("sib_texturelayer");

         // Created ok or previously found
         if (layerNode!=NULL)
         {
            layerName = layerName + L"." + (CString)GetRenderInstance()->GetUniqueId();
            CNodeUtilities().SetName(layerNode, layerName);

            // Load Ports parameters (it is important to parse ports parameter in first place, "mute" param can override layer "mute")
            CRef tempRef;
            LoadShaderParameters(layerNode, texturePort.GetParameters(), frame, tempRef, in_recursively);

            // Load Layer parameters
            LoadShaderParameters(layerNode, textureLayer.GetParameters(), frame, tempRef, in_recursively);

            // The order of mixing layers is from top to bottom so the output of the layer must be 
            // attached to basecolor of the new layer (see XSI help about texturelayers)
            if (previousLayerNode==NULL)
            {
               AtNode* linkedNode = AiNodeGetLink(shaderNode, targetParam.GetName().GetAsciiString());

               // If we have the parameter linked to a shader we will link that node to basecolor
               if (linkedNode!=NULL)
                  AiNodeLink(linkedNode, "basecolor", layerNode);
               // If not linked, we will get the value of the parameter and set it to basecolor
               else
               {
                  int paramType = GetArnoldParameterType(shaderNode, targetParam.GetName().GetAsciiString());

                  if (paramType==AI_TYPE_RGB)
                  {
                     AtRGB baseColor = AiNodeGetRGB(shaderNode, targetParam.GetName().GetAsciiString());     
                     CNodeSetter::SetRGBA(layerNode, "basecolor", baseColor.r, baseColor.g, baseColor.b, 1.0f);
                  }
                  else if (paramType==AI_TYPE_RGBA)
                  {
                     AtRGBA baseColor = AiNodeGetRGBA(shaderNode, targetParam.GetName().GetAsciiString());     
                     CNodeSetter::SetRGBA(layerNode, "basecolor", baseColor.r, baseColor.g, baseColor.b, baseColor.a);
                  }
                  else if (paramType==AI_TYPE_VECTOR)
                  {                  
                     AtVector baseColor = AiNodeGetVec(shaderNode, targetParam.GetName().GetAsciiString());     
                     CNodeSetter::SetRGB(layerNode, "basecolor", (float) baseColor.x, (float) baseColor.y, (float) baseColor.z);
                  }
               }
            }
            else // Link previous node with basecolor parameter of this new node
               AiNodeLink(previousLayerNode, "basecolor", layerNode);
            
            // Link to Shader
            AiNodeLink(layerNode, targetParam.GetName().GetAsciiString(), shaderNode);
         }
      }
   }
   
   return status;
}


AtNode* GetPreviousLayerPort(const CRefArray &textureLayersArray, const CString &targetParamName, LONG layerIdx, bool soloActive, double frame)
{
   // Will search (from first layer to our layer (specified by layerIdx))
   // the layers that are affecting to the same port (diffuse, ambient, etc)

   AtNode* previousLayerNode = NULL;
 
   TextureLayer textureLayer;
   TextureLayerPort texturePort;

   for (LONG ilayer=0; ilayer<layerIdx; ilayer++)
   {
      // Get texture layer 
      textureLayer.SetObject(textureLayersArray[ilayer]);
      bool solo = (bool) ParAcc_GetValue(textureLayer,L"solo", frame);

      // Ignore layer (see comments on LoadTextureLayers() about ignore layer)
      if (soloActive && !solo)
        continue;

      // Get texture layer ports
      CRefArray texturePortsArray = textureLayer.GetTextureLayerPorts();
         
      LONG nports = texturePortsArray.GetCount();
      for (LONG iport=0; iport<nports; iport++)
      {
         texturePort.SetObject(texturePortsArray[iport]);
         Parameter targetParam = texturePort.GetTarget();    

         if (targetParam.GetName().IsEqualNoCase(targetParamName))
         {
            CString layerName = textureLayer.GetFullName() + L"." + targetParam.GetName() + L".SItoA." + CString(CTimeUtilities().FrameTimes1000(frame));
            previousLayerNode = AiNodeLookUpByName(layerName.GetAsciiString());
         }
      }
   }

   return previousLayerNode;
}


// Return the Texture_Projection_Def (where, for instance, the srt of the proj resides) of the texture projection property
//
// @param in_textureProjection  the texture projection
//
// @return  the projection definition primitive
//
Primitive GetTextureProjectionDefFromTextureProjection(const ClusterProperty &in_textureProjection)
{
   Primitive prim;
   // we can't do
   // prim = (Primitive)in_textureProjection.GetNestedObjects().GetItem(L"Texture_Projection_Def");
   // because Texture_Projection_Def could have been renamed by the user
   CRefArray nestedObjects(in_textureProjection.GetNestedObjects());
   LONG nbNestedObjects = nestedObjects.GetCount();

   for (LONG i=0; i<nbNestedObjects; i++)
   {
      if (nestedObjects[i].GetClassID() == siPrimitiveID)
      {
         prim = nestedObjects[i];
         break;
      }
   }
   return prim;
}


// Set the wrappings settings of the projections used in the shaders, and manage the map_lookup parameters with instance values
//
// @param in_shapeNode                         The Arnold polymesh node
// @param in_objRef                            The Softimge object CRef
// @param in_material                          The object's material
// @param in_uvsArray                          The array of UV projections
// @param in_iceTextureProjectionAttributes    The array of ICE UV projections
// @param in_frame                             The frame time
//
CStatus SetWrappingAndInstanceValues(AtNode* in_shapeNode, const CRef &in_objRef, const Material &in_material, const CRefArray &in_uvsArray, 
                                     vector <CIceTextureProjectionAttribute> *in_iceTextureProjectionAttributes, double in_frame)
{
   CRefArray shadersArray, textureShaders, mapLookupShaders, vertexColorShaders;

   CRenderTree().FindAllShadersUnderMaterial(in_material, shadersArray);

   LONG nbShaders = shadersArray.GetCount();
   for (LONG i=0; i<nbShaders; i++)
   {
      Shader shader(shadersArray[i]);
      CString progID = shader.GetProgID();

      if ( (progID.FindString(L"txt2d") != UINT_MAX) ||
           (progID.FindString(L"sib_texproj_lookup") != UINT_MAX) ||
           (progID.FindString(L"sib_texture_marble") != UINT_MAX) ||
           (progID.FindString(L"txt3d-checkerboard") != UINT_MAX) )
      {
         // Let's make sure we get the shader from its library regetting it from ID
         // to get the Name of Shader like Sources.Material...
         Shader xsiShader(Application().GetObjectFromID(CObjectUtilities().GetId(shader)));
         textureShaders.Add(xsiShader.GetRef());
      }
      else if (progID.FindString(L"map_lookup_") != UINT_MAX) // manage the map_lookup shaders
      {
         Shader xsiShader(Application().GetObjectFromID(CObjectUtilities().GetId(shader)));
         mapLookupShaders.Add(xsiShader.GetRef());
      }
      else if (progID.FindString(L"sib_vertex_color_alpha") != UINT_MAX) // manage the sib_vertex_color shaders
      {
         Shader xsiShader(Application().GetObjectFromID(CObjectUtilities().GetId(shader)));
         vertexColorShaders.Add(xsiShader.GetRef());
      }
   }

   nbShaders = textureShaders.GetCount();
   for (LONG i=0; i<nbShaders; i++)
   {
      Shader shader(textureShaders[i]);
      AtNode* shaderNode = GetRenderInstance()->ShaderMap().Get(shader, in_frame);

      if (shaderNode)
         SetWrappingSettings(in_shapeNode, shaderNode, in_objRef, shader, in_uvsArray, in_iceTextureProjectionAttributes, in_frame);
   }

   nbShaders = mapLookupShaders.GetCount();
   for (LONG i=0; i<nbShaders; i++)
   {
      Shader shader(mapLookupShaders[i]);
      AtNode* shaderNode = GetRenderInstance()->ShaderMap().Get(shader, in_frame);

      if (shaderNode)
         ExportInstanceValueAsUserData(in_shapeNode, shaderNode, in_objRef, shader, in_uvsArray, L"map", in_frame);
   }

   nbShaders = vertexColorShaders.GetCount();
   for (LONG i=0; i<nbShaders; i++)
   {
      Shader shader(vertexColorShaders[i]);
      AtNode* shaderNode = GetRenderInstance()->ShaderMap().Get(shader, in_frame);

      if (shaderNode) // vprop is the "map" parameter name in Vertex_rgba (?!)
         ExportInstanceValueAsUserData(in_shapeNode, shaderNode, in_objRef, shader, in_uvsArray, L"vprop", in_frame);
   }

   return CStatus::OK;
}


// Set the wrappings settings of the projections used in a shader
//
// @param in_shapeNode                         The Arnold polymesh node
// @param in_shaderNode                        The Arnold shader node
// @param in_objRef                            The Softimge object CRef
// @param in_xsiShader                         The Softimge shader
// @param in_uvsArray                          The array of UV projections
// @param in_iceTextureProjectionAttributes    The array of ICE UV projections
// @param in_frame                             The frame time
//
void SetWrappingSettings(AtNode* in_shapeNode, AtNode* in_shaderNode, const CRef &in_objRef, const Shader &in_xsiShader, const CRefArray &in_uvsArray, 
                         vector <CIceTextureProjectionAttribute> *in_iceTextureProjectionAttributes, double in_frame)
{
   CString projectionName, map;
   Parameter tspace_id;

   // Get Parameter where the projection resides
   tspace_id = ParAcc_GetParameter(in_xsiShader, L"tspace_id");
   // Some XSI shaders have a strange name parameter (tspaceid instead of tspace_id)
   if (!tspace_id.IsValid())
      tspace_id = ParAcc_GetParameter(in_xsiShader, L"tspaceid");                  

   // Get the projection used in this object (when the material is shared between different objects)
   projectionName = tspace_id.GetInstanceValue(in_objRef, false).GetAsText();

   // first, look in the ICE texture projection list for this object
   if (in_iceTextureProjectionAttributes)
   {
      vector <CIceTextureProjectionAttribute>::iterator it;
      for (it = in_iceTextureProjectionAttributes->begin(); it != in_iceTextureProjectionAttributes->end(); it++)
      {
         if (projectionName == it->m_name) // found
         {
            CNodeSetter::SetString(in_shaderNode, tspace_id.GetName().GetAsciiString(), projectionName.GetAsciiString()); 
            CString nodeName = CNodeUtilities().GetName(in_shapeNode);
            CString objProjectionName = nodeName + L"_tspace_id";
            CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, objProjectionName, projectionName);

            // wrapping
            CString paramName = projectionName + L"_wrap";

            if (!AiNodeLookUpUserParameter(in_shapeNode, paramName.GetAsciiString()))
               AiNodeDeclare(in_shapeNode, paramName.GetAsciiString(), "constant ARRAY BOOL");
            if (AiNodeLookUpUserParameter(in_shapeNode, paramName.GetAsciiString()))
            {
               AtArray* wrapArray = AiArrayAllocate(2, 1, AI_TYPE_BOOLEAN);
               AiArraySetBool(wrapArray, 0, it->m_uWrap);
               AiArraySetBool(wrapArray, 1, it->m_vWrap);
               AiNodeSetArray(in_shapeNode, paramName.GetAsciiString(), wrapArray);
            }

            return; // done with this texture shader
         }
      }
   }

   int nuvs = in_uvsArray.GetCount(); 
   for (int k=0; k<nuvs; k++)
   {
      ClusterProperty in_uvProperty(in_uvsArray[k]); 

      // In case we have only 1 projection and the user has left Texture Projection combo in blank 
      // we will get the wrap settings from that projection
      if (nuvs==1)
      {
         // Assigning projection name to image shader (perhaps it is empty because the user has not selected the projection)
         // Also we are reassigning projectionName from uvcluster instead of the parameter of the shader (because it can be empty)
         projectionName = in_uvProperty.GetName();
         // setting txt2d_image_explicit.tspace_id
         CNodeSetter::SetString(in_shaderNode, tspace_id.GetName().GetAsciiString(), projectionName.GetAsciiString()); 
      }
      if (nuvs==1 || (in_uvProperty.GetType()==siClsUVSpaceTxtType && projectionName.IsEqualNoCase(in_uvProperty.GetName())))
      {
         // #505. In the texture shader, declare a string attribute named polymesh.name + "_tspace_id", and 
         // set it to the projection name. If the same texture shader is shared by several objects, we'll have
         // one of such entries for each object
         CString nodeName = CNodeUtilities().GetName(in_shapeNode);
         CString objProjectionName = nodeName + L"_tspace_id";
         CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, objProjectionName, projectionName);

         // The definition where the wrapping parameters reside
         Primitive definition = GetTextureProjectionDefFromTextureProjection(in_uvProperty);
         if (definition.IsValid())
         {
            bool wrap_u = ParAcc_GetValue(definition, L"wrap_u", in_frame);
            bool wrap_v = ParAcc_GetValue(definition, L"wrap_v", in_frame);
                        
            // wrapping
            CString paramName = projectionName + L"_wrap";
            // in the polymesh node, declare a bool array attribute named projection name + "_wrap", and set
            // there the wrapping flags of the projection. So, for each projection of the same object, we have
            // all the wrapping set on the polymesh.
            if (!AiNodeLookUpUserParameter(in_shapeNode, paramName.GetAsciiString()))
               AiNodeDeclare(in_shapeNode, paramName.GetAsciiString(), "constant ARRAY BOOL");

            // Rechecking if it was created ok to update it
            if (AiNodeLookUpUserParameter(in_shapeNode, paramName.GetAsciiString()))
            {
               AtArray* wrapArray = AiArrayAllocate(2, 1, AI_TYPE_BOOLEAN);
               AiArraySetBool(wrapArray, 0, wrap_u);
               AiArraySetBool(wrapArray, 1, wrap_v);
               AiNodeSetArray(in_shapeNode, paramName.GetAsciiString(), wrapArray);
            }
            break;
         }
      }
   }
}


// Export a clip's parameters as constant user data for the input shader node
//
// @param in_shaderNode      The Arnold map_lookup shader
// @param in_shapeNodeName   The Arnold shape node owning the shader
// @param in_map             The name of the texture map
// @param in_ref             The Softimage object owner of the shader
// @param in_uvsArray        The array of UV projections
// @param in_frame           The frame time
//
void ExportTextureMapAsUserData(AtNode *in_shaderNode, CString in_shapeNodeName, CString in_map, const CRef &in_ref, const CRefArray &in_uvsArray, double in_frame)
{
   X3DObject shaderOwnerObject(in_ref);
   // Only texture map properties should pass this test, since the other maps like weightmaps are owned by clusters.
   // Instead, texture maps are owned by the object
   Property textureMapProperty = shaderOwnerObject.GetProperties().GetItem(in_map);
   if (textureMapProperty.IsValid())
   {
      // early test, just to be sure the property is a texture map property indeed
      Parameter tspace_id = textureMapProperty.GetParameter(L"UVReference");
      if (!tspace_id.IsValid())
         return;

      CRefArray nestedObjects = textureMapProperty.GetNestedObjects();
      for (LONG nObj=0; nObj<nestedObjects.GetCount(); nObj++)
      {
         ImageClip2 clip(nestedObjects[nObj]);
         if (!clip.IsValid())
            continue;

         // first, let's export all the image clip data, the ones that, for the standard image shader, 
         // are exported as parameters of sib_image_clip
         CString attributeName;
         CStringArray stringParams, floatParams, intParams, boolParams;

         stringParams.Add(L"TimeSource");
         stringParams.Add(L"RenderColorProfile");
         // SourceFileName is managed below

         boolParams.Add(L"GrayScale");
         boolParams.Add(L"FlipX");
         boolParams.Add(L"FlipY");

         floatParams.Add(L"Hue");
         floatParams.Add(L"Saturation");
         floatParams.Add(L"Gain");
         floatParams.Add(L"Brightness");
         floatParams.Add(L"Xmin");
         floatParams.Add(L"Xmax");
         floatParams.Add(L"Ymin");
         floatParams.Add(L"Ymax");
         floatParams.Add(L"Exposure");
         floatParams.Add(L"RenderGamma");

         intParams.Add(L"ImageDefinitionType");

         for (LONG i=0; i<stringParams.GetCount(); i++)
         {
            CString pValue = ParAcc_GetValue(clip, stringParams[i], DBL_MAX).GetAsText();
            attributeName = in_shapeNodeName + L"_" + stringParams[i];
            CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, pValue);
         }
         for (LONG i=0; i<boolParams.GetCount(); i++)
         {
            bool pValue = (bool)ParAcc_GetValue(clip, boolParams[i], DBL_MAX);
            attributeName = in_shapeNodeName + L"_" + boolParams[i];
            CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, pValue);
         }
         for (LONG i=0; i<intParams.GetCount(); i++)
         {
            int pValue = (int)ParAcc_GetValue(clip, intParams[i], DBL_MAX);
            attributeName = in_shapeNodeName + L"_" + intParams[i];
            CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, pValue);
         }
         for (LONG i=0; i<floatParams.GetCount(); i++)
         {
            float pValue = (float)ParAcc_GetValue(clip, floatParams[i], DBL_MAX);
            attributeName = in_shapeNodeName + L"_" + floatParams[i];
            CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, pValue);
         }

         // Get the name of the TimeSource attribute, as wed do for the clip shader
         CPathString sourceFileName = GetClipSourceFileName(clip, in_frame);
         CString pValue(sourceFileName);
         attributeName = in_shapeNodeName + L"_SourceFileName";
         CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, pValue);

         CRefArray textureProperties = clip.GetProperties();
         Property textureOptionsProperty;
         textureProperties.Find(L"arnold_texture_options", textureOptionsProperty);

         int  filter, mipmapBias(0), sWrap(0), tWrap(0);
         bool swapST(false);
         if (textureOptionsProperty.IsValid())
         {
            filter     = (int) ParAcc_GetValue(textureOptionsProperty, L"filter", in_frame);
            mipmapBias = (int) ParAcc_GetValue(textureOptionsProperty, L"mipmap_bias", in_frame);
            swapST     = (bool)ParAcc_GetValue(textureOptionsProperty, L"swap_uv", in_frame);
            sWrap      = (int) ParAcc_GetValue(textureOptionsProperty, L"u_wrap", in_frame);
            tWrap      = (int) ParAcc_GetValue(textureOptionsProperty, L"v_wrap", in_frame);
         }
         else
            filter = GetRenderOptions()->m_texture_filter;

         attributeName = in_shapeNodeName + L"_filter";
         CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, filter);

         attributeName = in_shapeNodeName + L"_mipmap_bias";
         CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, mipmapBias);

         attributeName = in_shapeNodeName + L"_swap_st";
         CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, swapST);

         attributeName = in_shapeNodeName + L"_s_wrap";
         CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, sWrap);

         attributeName = in_shapeNodeName + L"_t_wrap";
         CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, tWrap);

         // next, export the texture projection settings, so the tspace_id and its wrapping settings
         CString projectionName = tspace_id.GetInstanceValue(in_ref, false).GetAsText();

         int nuvs = in_uvsArray.GetCount(); 
         for (int k=0; k<nuvs; k++)
         {
            ClusterProperty in_uvProperty(in_uvsArray[k]); 
            if (in_uvProperty.GetType()==siClsUVSpaceTxtType && projectionName.IsEqualNoCase(in_uvProperty.GetName()))
            {
               // The definition where the wrapping parameters reside
               Primitive definition = GetTextureProjectionDefFromTextureProjection(in_uvProperty);
               if (definition.IsValid())
               {
                  attributeName = in_shapeNodeName + L"_tspace_id";
                  CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, projectionName);
                  
                  bool wrap_u = ParAcc_GetValue(definition, L"wrap_u", DBL_MAX);
                  bool wrap_v = ParAcc_GetValue(definition, L"wrap_v", DBL_MAX);

                  attributeName = in_shapeNodeName + L"_wrap";

                  if (!AiNodeLookUpUserParameter(in_shaderNode, attributeName.GetAsciiString()))
                     AiNodeDeclare(in_shaderNode, attributeName.GetAsciiString(), "constant ARRAY BOOL");

                  AtArray* wrapArray = AiArrayAllocate(2, 1, AI_TYPE_BOOLEAN);
                  AiArraySetBool(wrapArray, 0, wrap_u);
                  AiArraySetBool(wrapArray, 1, wrap_v);
                  AiNodeSetArray(in_shaderNode, attributeName.GetAsciiString(), wrapArray);

                  break;
               }
            }
         }
   
         return; // done
      }
   }
}


// Export the values of the parameters with instance values (map_lookup_*) as user data
//
// @param in_shapeNode         The Arnold polymesh node
// @param in_shaderNode        The Arnold shader node
// @param in_objRef            The Softimge object CRef
// @param in_xsiShader         The Softimge map_lookup_* shader
// @param in_uvsArray          The array of UV projections
// @param in_paramName         The name of the parameter with instance value (for example "map" for the map_lookup shaders)
// @param in_frame             The frame time
//
void ExportInstanceValueAsUserData(AtNode* in_shapeNode, AtNode* in_shaderNode, const CRef &in_objRef, const Shader &in_xsiShader, const CRefArray &in_uvsArray, const CString &in_paramName, double in_frame)
{
   // all the map_lookup_* shaders have the map parameter
   Parameter mapP = ParAcc_GetParameter(in_xsiShader, in_paramName);
   // get the instance value for in_objRef
   CString map = mapP.GetInstanceValue(in_objRef, false).GetAsText();
   // name of the polymesh node
   CString shapeNodeName = CNodeUtilities().GetName(in_shapeNode);
   // attaching a user data to the lookup shader == Arnold shape node name + "_map"
   // in short, we mimic the instance value feature by assigning to a map_lookup shader the value
   // of "map" for each object sharing the shader. Then, it will be on the map shaders to retrieve
   // the correct map attribute, based, on the shader's owner (sg->Op)
   CString attributeName = shapeNodeName + L"_" + in_paramName;
   CNodeUtilities().DeclareConstantUserParameter(in_shaderNode, attributeName, map);
   // export the user data if map points to a texture map property
   ExportTextureMapAsUserData(in_shaderNode, shapeNodeName, map, in_objRef, in_uvsArray, in_frame);
}

