/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsShader.h"
#include "loader/Cameras.h"
#include "loader/Shaders.h"
#include "renderer/IprCamera.h"
#include "renderer/IprShader.h"
#include "renderer/Renderer.h"

#include <xsi_geometryaccessor.h>
#include <xsi_polygonmesh.h>
#include <xsi_shaderarrayparameter.h>

// Update an object's material. It's called when, in ipr, an object is moved to/out-of a group with a material
//
// @param in_obj                 The object
// @param in_frame               The frame
//
void UpdateObjectMaterial(const X3DObject &in_obj, double in_frame)
{
   if (!in_obj.IsValid())
      return;
   Material mat = in_obj.GetMaterial();
   if (mat.IsValid())
      UpdateMaterial(mat, in_frame);
}


// Update a material.
//
// @param in_obj                 The object
// @param in_frame               The frame
//
void UpdateMaterial(const Material &in_material, double in_frame)
{  
   Application app;
   Material updateMaterial = in_material;

   AtNode* surfaceNode = NULL;
   AtNode* environmentNode = NULL;

   // Regetting material from the object
   // When we receive events of Material when assigning new material from Material Manager
   // we receive "sphere.Material1" instead "Sources.Materials.DefaultLib.Material2" and we have not
   // access to its shaders... xsi bug?
   // Also if we receive an event from partition change.. material name is 
   // Passes.Default_Pass.Background_Objects_Partition.Scene_Material
   int classID = in_material.GetParent().GetClassID();
   if (classID==siPartitionID || classID==siGroupID)
   {
      CRefArray list = in_material.GetOwners();
      LONG ncount = list.GetCount();
      bool found = false;
      for (LONG i=0; i<ncount && !found; i++)
      {
         if (list[i].GetClassID()==siX3DObjectID)
         {
            X3DObject xsiObj(list[i]);
            found = GetMaterialFromObject(xsiObj, in_material, updateMaterial);
         }
      }
   }
   else if (classID!=siMaterialLibraryID)
   {
      X3DObject xsiObj(in_material.GetParent());
      GetMaterialFromObject(xsiObj, in_material, updateMaterial);
   }

   Shader xsiShader, surfaceShader, environmentShader;

   // Updating shaders attached to surface param material
   xsiShader = GetConnectedShader(ParAcc_GetParameter(updateMaterial, L"surface"));
   surfaceShader = app.GetObjectFromID(CObjectUtilities().GetId(xsiShader));
   if (surfaceShader.IsValid())
      surfaceNode = UpdateShader(surfaceShader, in_frame);

   // Updating shaders attached to environment param material
   xsiShader = GetConnectedShader(ParAcc_GetParameter(updateMaterial,L"environment"));
   environmentShader = Shader(app.GetObjectFromID(CObjectUtilities().GetId(xsiShader)));
   if (environmentShader.IsValid())
      environmentNode = UpdateShader(environmentShader, in_frame);

   // Updating Shader Links of Material
   // We are sending all shaders of material supported instead doing a generic method
   // to avoid loop over all owners of this material several times.
   UpdateMaterialLinks(updateMaterial, surfaceNode, environmentNode, in_frame);
}


void UpdateMaterialLinks(const Material &in_material, AtNode* in_surfaceNode, AtNode* in_environmentNode, double in_frame)
{
   if (!in_surfaceNode)
      return;

   // When we assign materials to objects through Material Manager or drag&drop
   // the parent of the material will be the object and not the library
   // We need to know it to force the assigning because if not, we'd exit the loop
   // because our optimization when we change the surface shader of a common material
   // (we only detect the first case)
   bool materialAssigning = in_material.GetParent().GetClassID() != siMaterialLibraryID;
   CString materialName = in_material.GetFullName();

   CRefArray ownersList(in_material.GetUsedBy());
   LONG nbOwners = ownersList.GetCount();
   bool exitLoop = false;

   for (LONG j=0; j<nbOwners && !exitLoop; j++)
   {
      X3DObject xsiObj;
      int ownerID = ownersList[j].GetClassID();
    
      // Getting the material's j-th owner
      if (ownerID == siClusterID)
         xsiObj = X3DObject(Cluster(ownersList[j]).GetParent3DObject());
      else if (ownerID == siX3DObjectID)
         xsiObj = X3DObject(ownersList[j]);

      if (xsiObj.IsValid())
      {
         vector <AtNode*> nodes;
         vector <AtNode*> ::iterator nodeIter;

         AtNode* node = GetRenderInstance()->NodeMap().GetExportedNode(xsiObj, in_frame);
         if (node)
            nodes.push_back(node);
         else // if the node is a node group we will loop over all its members
         {
            vector <AtNode*> *tempV;
            if (GetRenderInstance()->GroupMap().GetGroupNodes(&tempV, xsiObj, in_frame))
               nodes = *tempV;
         }

         for (nodeIter=nodes.begin(); nodeIter!=nodes.end() && !exitLoop; nodeIter++)
         {
            node = *nodeIter;
            // don't update ginstances, that use the master's material
            CString nodeName = CNodeUtilities().GetName(node);
            if (nodeName.FindString(L" ") != UINT_MAX)
               continue;

            // RELINKING ENVIRONMENT SHADER            
            if (!AiNodeLookUpUserParameter(node, "environment"))
               AiNodeDeclare(node, "environment", "constant NODE");
            if (AiNodeLookUpUserParameter(node, "environment"))
               CNodeSetter::SetPointer(node, "environment", in_environmentNode);
            
            // RELINKING SURFACE SHADER
            AtArray* shaders = AiNodeGetArray(node, "shader");
            if (!shaders)
               break;
       
            bool found = false;

            // looping the shaders, that can be several in case shdidxs is in use
            if (!materialAssigning)
            for (unsigned int k=0; k<AiArrayGetNumElements(shaders) && !exitLoop; k++)
            {
               AtNode* currentShaderNode = (AtNode*)AiArrayGetPtr(shaders, k);

               if (currentShaderNode)
               {
                  CString currentShaderNodeName = CNodeUtilities().GetName(currentShaderNode);

                  if (strstr(currentShaderNodeName.GetAsciiString(), materialName.GetAsciiString()) != NULL)
                  {
                     // the current shader is "under" the material we've edited.
                     // Ex. "Sources.Materials.DefaultLib.Material" -> "Sources.Materials.DefaultLib.Material.utility.SItoA.1000.1"

                     // Checking if the shader node is different to re-assign. If 
                     // it is the same, it means that we haven't changed the shader so we can
                     // break the loop. (because all objects use this same material)
                     if (in_surfaceNode == currentShaderNode)
                        exitLoop = true;                   
                     else
                        AiArraySetPtr(shaders, k, in_surfaceNode);

                     found = true;
                  }
               }
            }

            // If we havent found the shader for a non clustered object,
            // it means that we have assigned a new material to it.
            if (!found && ownerID==siX3DObjectID)
               AiArraySetPtr(shaders, 0, in_surfaceNode);
         }

         nodes.clear();
      }
   }
}


AtNode* UpdateShader(const Shader &in_xsiShader, double in_frame)
{   
   double frame(in_frame);
   // "frame" is use to look up the existing shader node (if any). If we are in flythrough mode,
   // the node was created at time flythrough_frame, and never destroyed since then.
   if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Flythrough)
      frame = GetRenderInstance()->GetFlythroughFrame();

   AtNode* shaderNode = GetRenderInstance()->ShaderMap().Get(in_xsiShader, frame);
   // Updating existing shader
   if (shaderNode)
   {
      // Updating Shader Parameters (recursively to parse all subshaders params)
      CRef tempRef; // no CRef available for the object
      LoadShaderParameters(shaderNode, in_xsiShader.GetParameters(), in_frame, tempRef, RECURSE_TRUE);
      // Updating Texture Layers
      LoadTextureLayers(shaderNode, in_xsiShader, in_frame, RECURSE_TRUE);
   }
   // Creating new Shader Node (we have to link its output to its source)
   else
   {
      CRef tempRef; // no CRef available for the object
      shaderNode = LoadShader(in_xsiShader, in_frame, tempRef, RECURSE_TRUE);
   }

   // Return if shader node is null
   if (!shaderNode)
      return NULL;

   // We will update where is connected the output of this shader
   CRefArray linkedToList(in_xsiShader.GetShaderParameterTargets(L""));
   int nlinks = linkedToList.GetCount();
   for (int i=0; i<nlinks; i++)
   {
      Parameter linkedParam(linkedToList[i]);
      int sourceID = linkedParam.GetParent().GetClassID();

      // Will link the params to their parents
      if (sourceID==siShaderID)
      {
         Shader xsiLinkedShader(linkedParam.GetParent());      
         CString shaderName = xsiLinkedShader.GetFullName();

         AtNode* linkedShader = GetRenderInstance()->ShaderMap().Get(xsiLinkedShader, frame);
         if (linkedShader)
            AiNodeLink(shaderNode, linkedParam.GetName().GetAsciiString(), linkedShader);
      }
   }

   return shaderNode;
}


void UpdateImageClip(const ImageClip2 &in_xsiImageClip, double in_frame)
{   
   double frame(in_frame);
   // "frame" is use to look up the existing sshader node (if any). If we are in flythrough mode,
   // the node was created at time flythrough_frame, and never destroyed since then.
   if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Flythrough)
      frame = GetRenderInstance()->GetFlythroughFrame();

   AtNode* shaderNode = GetRenderInstance()->ShaderMap().Get(in_xsiImageClip, frame);

   // Updating existing shader
   if (shaderNode)
   {
      CRef tempRef; // no CRef available for the object
      LoadShaderParameters(shaderNode, in_xsiImageClip.GetParameters(), in_frame, tempRef, false);
      return;
   }
   // If it doesnt exist we will destroy the Scene
   // For now is very diffucult to create dynamically ImageClips
   if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Manual)
      GetMessageQueue()->LogMsg(L"[sitoa] Incompatible IPR event detected (by " + in_xsiImageClip.GetFullName() + L"). Not destroying the scene because in manual rebuild mode");
   else
      GetRenderInstance()->DestroyScene(false);
}


void UpdatePassShaderStack(const Pass &in_pass, double in_frame)
{
   CRef envParamRef;
   envParamRef.Set(in_pass.GetFullName() + L".EnvironmentShaderStack.Item");
   Parameter passParam(envParamRef);
   AtNode* options = AiUniverseGetOptions();

   Shader backgroundShader = GetConnectedShader(passParam);
   if (backgroundShader.IsValid())
   {
      AtNode* shaderNode = UpdateShader(backgroundShader, in_frame);
      CNodeSetter::SetPointer(options, "background", shaderNode);
   }
   else
      CNodeSetter::SetPointer(options, "background", NULL);
   
   // Support for 'AOV shaders' putting this into 'output' shader type
   CRef outputStackRef;
   outputStackRef.Set(in_pass.GetFullName() + L".OutputShaderStack");
   ShaderArrayParameter arrayParam = ShaderArrayParameter(outputStackRef);
   
   if (arrayParam.GetCount()>0)
   {
      AtArray* aovShadersArray = AiArrayAllocate(arrayParam.GetCount(), 1, AI_TYPE_NODE);   
      for (LONG i=0; i<arrayParam.GetCount(); i++)
      {
         passParam = Parameter(arrayParam[i]);
         Shader outputShader = GetConnectedShader(passParam);
         if (outputShader.IsValid())
         {
            AtNode* shaderNode = UpdateShader(outputShader, in_frame);
            AiArraySetPtr(aovShadersArray, i, shaderNode);
         }
      }
      AiNodeSetArray(options, "aov_shaders", aovShadersArray);
   }
   else
      CNodeSetter::SetPointer(options, "aov_shaders", NULL);

   CRef atmParamRef;
   atmParamRef.Set(in_pass.GetFullName() + L".VolumeShaderStack.Item");
   passParam = Parameter(atmParamRef);

   Shader atmosphereShader = GetConnectedShader(passParam);
   if (atmosphereShader.IsValid())
   {
      AtNode* shaderNode = UpdateShader(atmosphereShader, in_frame);
      CNodeSetter::SetPointer(options, "atmosphere", shaderNode);
   }
   else
      CNodeSetter::SetPointer(options, "atmosphere", NULL);
}


bool GetMaterialFromObject(const X3DObject &xsiObj, const Material &material, Material &updateMaterial)
{
   CRefArray objMaterials(xsiObj.GetMaterials());
   LONG nmaterials = objMaterials.GetCount();

   for (LONG imaterial=0; imaterial<nmaterials; imaterial++)
   {
      Material checkMaterial(objMaterials[imaterial]);

      if (material.GetName().IsEqualNoCase(checkMaterial.GetName()))
      {
         updateMaterial = checkMaterial;
         return true;
      }
   }
   
   return false;
}


void UpdateWrappingSettings(const CRef &in_ref, double in_frame)
{
   X3DObject xsiObj(in_ref);
   AtNode* polymeshNode = GetRenderInstance()->NodeMap().GetExportedNode(xsiObj, in_frame);
   
   if (polymeshNode)
   {
      PolygonMesh polyMesh = CObjectUtilities().GetGeometryAtFrame(xsiObj, in_frame);
      CRefArray polyProperties = xsiObj.GetProperties();
      Property geoProperty = polyProperties.GetItem(L"Geometry Approximation");
      bool discontinuity = (bool)ParAcc_GetValue(geoProperty,L"gapproxmoad", in_frame);
      double angle = (double)ParAcc_GetValue(geoProperty,L"gapproxmoan", in_frame);
      CGeometryAccessor geometryAccessor = polyMesh.GetGeometryAccessor(siConstructionModeSecondaryShape, siCatmullClark, 0, false, discontinuity, angle);

      CRefArray uvsArray = geometryAccessor.GetUVs();
      
      CRefArray materialsArray(geometryAccessor.GetMaterials());
      LONG nbMaterials = materialsArray.GetCount();

      for (LONG i=0; i<nbMaterials; i++)
      {
         Material material(materialsArray[i]);
         // Set wrapping settings and parameters instance values
         SetWrappingAndInstanceValues(polymeshNode, xsiObj.GetRef(), material, uvsArray, NULL, in_frame);
      }
   }
}



// This utility class is to be used to compare two branches of a shading tree.
// Currently, it is used in ParamLight.cpp to detect if a changed happened during ipr for the branch
// connected to the skydome color (or to just the color itself). If so, we have to flush the background cache.
//


// Traverse backward a branch, starting from a given node, and packs all the parameter values into m_buffer.
// By now, only the most common param types are explored (for example, no matrices/enum yet), but it can be easibly extended
//
// @param in_node             The arnold shader node
// @param in_paramName        The name of the parameter where the branch starts, or NULL to get all the parameters
// @param in_recurse          True if the branch has to be recursively traversed, false to parse only the parameters of in_node
//
// @return                    true, or false if something went wrong (null in_node, etc.)
//
bool CShaderBranchDump::Fill(AtNode* in_node, const char* in_paramName, bool in_recurse)
{
   if (!in_node)
      return false;

   const AtNodeEntry* nodeEntry = AiNodeGetNodeEntry(in_node);
   AtParamIterator* pIter = AiNodeEntryGetParamIterator(nodeEntry);

   int      pI;
   bool     pB;
   float    pF;
   AtRGB    pRGB;
   AtRGBA   pRGBA;
   AtVector pVector;
   AtVector pPoint;
   void     *voidP=NULL;

   // loop the parameters
   while (!AiParamIteratorFinished(pIter))
   {
      const AtParamEntry *pEntry = AiParamIteratorGetNext(pIter);
      const char* pName = AiParamGetName(pEntry);

      // if in_paramName is not NULL, we must skip all the params with name other than in_paramName
      if (in_paramName)
         if (strcmp(in_paramName, pName)) // not the input param?
            continue;

      AtNode* linkedNode = AiNodeGetLink(in_node, pName);
      if (linkedNode) // this parameter has a link
      {
         if (in_recurse) // recurse with the linked node, and NULL as the param name, since we must get the params
            Fill(linkedNode, NULL);
         continue; // next parameter
      }

      // here, we only have unlinked parameters, so we get their values

      int pType = AiParamGetType(pEntry);
      int pSize(0);

      switch (pType)
      {
         case AI_TYPE_INT:
            pSize = sizeof(int);               // size
            pI = AiNodeGetInt(in_node, pName); // value
            voidP = (void*)&pI;                // void pointer to the value
            break;
         case AI_TYPE_BOOLEAN:
            pSize = sizeof(bool);
            pB = AiNodeGetBool(in_node, pName);
            voidP = (void*)&pB;
            break;
         case AI_TYPE_FLOAT:
            pSize = sizeof(float);
            pF = AiNodeGetFlt(in_node, pName);
            voidP = (void*)&pF;
            break;
         case AI_TYPE_RGB:
            pSize = sizeof(AtRGB);
            pRGB = AiNodeGetRGB(in_node, pName);
            voidP = (void*)&pRGB;
            break;
         case AI_TYPE_RGBA:
            pSize = sizeof(AtRGBA);
            pRGBA = AiNodeGetRGBA(in_node, pName);
            voidP = (void*)&pRGBA;
            break;
         case AI_TYPE_VECTOR:
            pSize = sizeof(AtVector);
            pVector = AiNodeGetVec(in_node, pName);
            voidP = (void*)&pVector;
            break;
         default:
            break;
      }

      if (pSize) // a valid param type was found
      {
         m_buffer = (char*)realloc(m_buffer, m_size+pSize); // realloc by pSize
         memcpy((void*)&m_buffer[m_size], voidP, pSize);    // copy the param value in the newly allocated chunk
         m_size+=pSize;                                     // update the buffer size
      }
   }

   AiParamIteratorDestroy(pIter);
   return true;
}


// Compares two buffers, so returning true if two Fill's found exactly 
// the same connections along the branch, and the same values for all the parameters in the branch
//
bool CShaderBranchDump::operator==(const CShaderBranchDump &in_other) const 
{
   if (m_size != in_other.m_size)
      return false;
   // if the size is the same, compare the buffers
   return memcmp(m_buffer, in_other.m_buffer, m_size) == 0;
}

