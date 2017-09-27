/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/ParamsCommon.h"
#include "common/UserDataGrid.h"
#include "loader/Loader.h"
#include "loader/Properties.h"
#include "loader/Procedurals.h"
#include "renderer/Renderer.h"

#include <xsi_geometryaccessor.h>
#include <xsi_polygonmesh.h>

#include <iostream>
#include <fstream>

using namespace std;


// Get the bbox from a .asstoc file
//
// @param in_asstocFilename     the .asstoc file name
// @param out_min               the returned min corner of the bounding box 
// @param out_min               the returned max corner of the bounding box 
//
// @return true if the file was well read, else false
//
bool GetBoundingBoxFromScnToc(const CPathString &in_asstocFilename, CVector3f &out_min, CVector3f &out_max)
{
   float x, y, z, X, Y, Z;
   CVector3f center, extent;
   ifstream f;

   f.open(in_asstocFilename.GetAsciiString(), ios::in);
   if (f.fail())
      return false;
   string s;
   f >> s;
   if (f.fail())
      return false;
   f >> x;
   if (f.fail())
      return false;
   f >> y;
   if (f.fail())
      return false;
   f >> z;
   if (f.fail())
      return false;
   f >> X;
   if (f.fail())
      return false;
   f >> Y;
   if (f.fail())
      return false;
   f >> Z;
   if (f.fail())
      return false;
   f.close();

   out_min.Set(x, y, z);
   out_max.Set(X, Y, Z);
   // find the box center
   extent.Sub(out_max, out_min);
   extent.ScaleInPlace(0.5f);
   center.Add(out_min, extent);
   // translate to origin
   out_min.SubInPlace(center);
   out_max.SubInPlace(center);
   // translate back to center
   out_min.AddInPlace(center);
   out_max.AddInPlace(center);
   // 
   // GetMessageQueue()->LogMsg(L"Min = " + (CString)out_min.GetX() + L" " + (CString)out_min.GetY() + L" " + (CString)out_min.GetZ());
   // GetMessageQueue()->LogMsg(L"Min = " + (CString)out_max.GetX() + L" " + (CString)out_max.GetY() + L" " + (CString)out_max.GetZ());
   return true;
}


// Get the bbox from a Softimage object
//
// @param in_xsiObj     the object
// @param in_frame      the frame time
// @param in_scale      the scale to apply
// @param out_min       the returned min corner of the bounding box 
// @param out_min       the returned max corner of the bounding box 
//
void GetBoundingBoxFromObject(const X3DObject &in_xsiObj, const double in_frame, float in_scale, CVector3f &out_min, CVector3f &out_max)
{
   double c_x, c_y, c_z, ext_x, ext_y, ext_z;
   CTransformation transform;
   transform.SetIdentity();

   Geometry geo = CObjectUtilities().GetGeometryAtFrame(in_xsiObj, in_frame);
   geo.GetBoundingBox(c_x, c_y, c_z, ext_x, ext_y, ext_z, transform);
   CVector3f center((float)c_x,   (float)c_y,   (float)c_z);
   CVector3f extent((float)ext_x, (float)ext_y, (float)ext_z);
   // apply the scaling to the extent
   extent.ScaleInPlace(in_scale);
   extent.ScaleInPlace(0.5f);

   out_min.Sub(center, extent);
   out_max.Add(center, extent);
   // 
   // GetMessageQueue()->LogMsg(L"Min = " + (CString)out_min.GetX() + L" " + (CString)out_min.GetY() + L" " + (CString)out_min.GetZ());
   // GetMessageQueue()->LogMsg(L"Min = " + (CString)out_max.GetX() + L" " + (CString)out_max.GetY() + L" " + (CString)out_max.GetZ());
}


// Load a procedural
//
// @param in_xsiObj              the Softimage object owner of the procedural property
// @param in_frame               the frame time
// @param in_selectedObjs        the selected objs to render (if in_selectionOnly==true)
// @param in_selectionOnly       true is only in_selectedObjs must be rendered
//
CStatus LoadSingleProcedural(const X3DObject &in_xsiObj, double in_frame, CRefArray &in_selectedObjs, bool in_selectionOnly)
{  
   if (GetRenderInstance()->InterruptRenderSignal())
      return CStatus::Abort;

   if (GetRenderOptions()->m_ignore_procedurals)
      return CStatus::OK;

   LockSceneData lock;
   if (lock.m_status != CStatus::OK)
      return CStatus::Abort;

   CRefArray proceduralProperties;

   // check if this object is selected
   if (in_selectionOnly && !ArrayContainsCRef(in_selectedObjs, in_xsiObj.GetRef()))
      return CStatus::OK;

   proceduralProperties = in_xsiObj.GetProperties();

   // get the arnold_procedural property
   // existance test already done by the caller (LoadSinglePolymesh)
   Property proceduralInfo = Property(proceduralProperties.GetItem(L"arnold_procedural"));

   // If procedural is invisible we will not do anything
   if (!ParAcc_GetValue(Property(proceduralProperties.GetItem(L"Visibility")), L"rendvis", in_frame))
      return CStatus::OK;

   CPathString filename(ParAcc_GetValue(proceduralInfo, L"filename", in_frame).GetAsText());

   // To mark a procedural as "to be set by ICE" (#1248), we check if any of the strings are "ArnoldProcedural"
   // If so, just return quietely without exporting the procedural.
   if (filename.IsEqualNoCase(g_ArnoldProceduralAttributePrefix))
      return CStatus::OK;

   double sFrame = in_frame;
   if ((bool)ParAcc_GetValue(proceduralInfo, L"overrideFrame", in_frame))
      sFrame = (double)ParAcc_GetValue(proceduralInfo, L"frame", in_frame);

   filename.ResolveTokensInPlace(sFrame); // resolve the tokens
   // do not resolve the path anymore, after using local paths for procedurals (#1237)
   // filename.ResolvePathInPlace(); // in case we are migrating a scene between windows and linux

   bool useAbsolutePath = GetRenderOptions()->m_save_procedural_paths;

   // use absolute path also in case the flag does not existing, so loading old scenes
   if (useAbsolutePath) // translate it (#1238)
      filename.PutAsciiString(CPathTranslator::TranslatePath(filename.GetAsciiString(), false));
   else 
   {
      bool relativePathOk(false);

      vector <CPathString> proceduralsSearchPaths;
      vector <CPathString>::iterator searchPathsIt;

      if (GetRenderInstance()->GetProceduralsSearchPath().GetPaths(proceduralsSearchPaths))
      {
         // translate the paths
         const char *translatedPath = CPathTranslator::TranslatePath(filename.GetAsciiString(), false);
         bool isPathTranslatorInitialized = CPathTranslator::IsInitialized(); // using linktab?
         unsigned int pathTranslatorMode = CPathTranslator::GetTranslationMode(); // w2l (default) or l2w?

         bool windowsSlash(false); // we always use "/", except if on linux and exporting for windows (what a mess)
         if (CUtils::IsLinuxOS() && isPathTranslatorInitialized && pathTranslatorMode == TRANSLATOR_LINUX_TO_WIN)
            windowsSlash = true;
         // loop the procedural search paths and break on the first successfully conversion to relative path.
         for (searchPathsIt = proceduralsSearchPaths.begin(); searchPathsIt != proceduralsSearchPaths.end(); searchPathsIt++)
         {
            const char *searchPath = CPathTranslator::TranslatePath(searchPathsIt->GetAsciiString(), false);\
            // #1162
            CPathString fullPath(translatedPath), relDir(searchPath);

            CPathString relPath = fullPath.GetRelativeFilename(relDir, windowsSlash);

            if (!relPath.IsVoid()) // relative path replacement successfull
            {
               filename = relPath;
               relativePathOk = true;
               break;
            }
         }
      }

      if (!relativePathOk) // the conversion to relative path failed, use the plain file name
      {
         CStringArray pathParts(filename.Split(CUtils::Slash()));
         filename = pathParts[pathParts.GetCount()-1];
      }
   }

   if (filename.IsEmpty())
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Void procedural file, aborting procedural", siErrorMsg);
      return CStatus::OK;
   }
   if (!filename.IsProcedural())
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Invalid procedural file (" + filename + L"), aborting procedural", siErrorMsg);
      return CStatus::OK;
   }

   // Create the procedural node
   AtNode* procNode = AiNode("procedural");

   if (!procNode)
      return CStatus::OK; // not really ok...

   GetRenderInstance()->NodeMap().PushExportedNode(in_xsiObj, in_frame, procNode);

   CString name = CStringUtilities().MakeSItoAName(in_xsiObj, in_frame, L"", false);
   CNodeUtilities().SetName(procNode, name);
   CNodeSetter::SetString(procNode, "filename", filename.GetAsciiString());

   // Getting Motion Blur Data
   CDoubleArray keyFramesTransform;
   CDoubleArray keyFramesDeform;
   CSceneUtilities::GetMotionBlurData(in_xsiObj.GetRef(), keyFramesTransform, keyFramesDeform, in_frame);

   // Get matrix transform
   int nkeystrans = keyFramesTransform.GetCount();
   AtArray* matrices = AiArrayAllocate(1, (uint8_t)nkeystrans, AI_TYPE_MATRIX);

   for (int ikey=0; ikey<nkeystrans; ikey++)
   {
      // Get the transform matrix
      AtMatrix matrix;
      CUtilities().S2A(in_xsiObj.GetKinematics().GetGlobal().GetTransform(keyFramesTransform[ikey]).GetMatrix4(), matrix);
      AiArraySetMtx(matrices, ikey, matrix);
   }

   AiNodeSetArray(procNode, "matrix", matrices);

   // LIGHT GROUP

   AtArray* light_group = GetRenderInstance()->LightMap().GetLightGroup(in_xsiObj);
   if (light_group)
   {
      CNodeSetter::SetBoolean(procNode, "use_light_group", true);
      if (AiArrayGetNumElements(light_group) > 0)
         AiNodeSetArray(procNode, "light_group", light_group);
   }

   CNodeSetter::SetByte(procNode, "visibility", GetVisibility(proceduralProperties, in_frame), true);

   uint8_t sidedness;
   if (GetSidedness(proceduralProperties, in_frame, sidedness))
      CNodeSetter::SetByte(procNode, "sidedness", sidedness, true);

   CNodeUtilities::SetMotionStartEnd(procNode);
   // Arnold-specific Parameters
   CustomProperty paramsProperty, userOptionsProperty;
   proceduralProperties.Find(L"arnold_parameters", paramsProperty);
   proceduralProperties.Find(L"arnold_user_options", userOptionsProperty); /// #680

   if (paramsProperty.IsValid())
      LoadArnoldParameters(procNode, paramsProperty.GetParameters(), in_frame);
   LoadUserOptions(procNode, userOptionsProperty, in_frame); /// #680
   LoadUserDataBlobs(procNode, in_xsiObj, in_frame); // #728

   if (!GetRenderOptions()->m_ignore_matte)
   {
      Property matteProperty;
      proceduralProperties.Find(L"arnold_matte", matteProperty);
      LoadMatte(procNode, matteProperty, in_frame);      
   }

   Material material(in_xsiObj.GetMaterial());
   CString matName = material.GetName();

   if (UseProceduralMaterial(matName))
   {
      // export the procedural's (unique) shader
      AtNode* shaderNode = LoadMaterial(material, LOAD_MATERIAL_SURFACE, in_frame, in_xsiObj.GetRef());
      if (shaderNode != NULL)
         AiNodeSetArray(procNode, "shader", AiArray(1, 1, AI_TYPE_NODE, shaderNode));

      // helge's patch for #1359
      if (in_xsiObj.GetType().IsEqualNoCase(L"polymsh"))
         ExportAlembicProceduralData(procNode, in_xsiObj, paramsProperty, proceduralProperties, in_frame);
   }

   // #1315 custom user data, initial patch by Steven Caron

   bool resolveTokens = resolveTokens = (bool)ParAcc_GetValue(proceduralInfo, L"resolveUserDataTokens", sFrame);

   if ((bool)ParAcc_GetValue(proceduralInfo, L"muteUserData", in_frame))
      return CStatus::OK;

   GridData userDataGrid = (GridData)proceduralInfo.GetParameterValue(L"userDataGrid");
   ExportUserDataGrid(procNode, userDataGrid, resolveTokens, sFrame);

   return CStatus::OK;
}


// Helge's patch for #1359. Exports shaders, displacement and displacement settings of the procedural object, 
// so they can retrieve them in the alambic procedural.
//
// @param in_procNode           The procedural node
// @param in_xsiObj             The Softimage procedural object
// @param in_arnoldParameters   The procedural's Arnold Parameters property
// @param in_proceduralProperties  The procedural's properties array
// @param in_frame              The frame time
//
// @return void
//
void ExportAlembicProceduralData(AtNode *in_procNode, const X3DObject &in_xsiObj, CustomProperty &in_arnoldParameters, CRefArray in_proceduralProperties, double in_frame)
{
   Property geoProperty = in_proceduralProperties.GetItem(L"Geometry Approximation");
   float adaptive_error = GetRenderOptions()->m_adaptive_error;
   uint8_t subdiv_iterations = (uint8_t)ParAcc_GetValue(geoProperty, L"gapproxmordrsl", in_frame);
   CString adaptive_metric(L"auto"), adaptive_space(L"raster");

   Primitive primitive = CObjectUtilities().GetPrimitiveAtFrame(in_xsiObj, in_frame);
   PolygonMesh polyMesh = CObjectUtilities().GetGeometryAtFrame(in_xsiObj, siConstructionModeSecondaryShape, in_frame);
   CGeometryAccessor geometryAccessor = polyMesh.GetGeometryAccessor(siConstructionModeSecondaryShape, siCatmullClark, 0, false);
   CRefArray materialsArray(geometryAccessor.GetMaterials());

   LONG nbMaterials = materialsArray.GetCount();					
   AtArray *shaders = AiArrayAllocate(nbMaterials, 1, AI_TYPE_NODE);

   AtArray *displacementShaders = AiArrayAllocate(nbMaterials, 1, AI_TYPE_NODE);
   bool dispOk(false);

   for (LONG i=0; i<nbMaterials; i++)
   {
      Material material(materialsArray[i]);
      AtNode *materialNode = LoadMaterial(material, LOAD_MATERIAL_SURFACE, in_frame, in_xsiObj.GetRef());
      AiArraySetPtr(shaders, i, materialNode);

      AtNode *dispMapNode = LoadMaterial(material, LOAD_MATERIAL_DISPLACEMENT, in_frame, in_xsiObj.GetRef());

      // the disp_map array must be exported only if at least one disp shader is valid. Else, Arnold crashes
      if (dispMapNode)
         dispOk=true;

      AiArraySetPtr(displacementShaders, i, dispMapNode);   
   }

   // export the procedural materials by a procedural_shader array attribute, so to preserve 
   // the procedural's shader array which we set already
   AiNodeDeclare (in_procNode, "procedural_shader", "constant ARRAY NODE");
   AiNodeSetArray(in_procNode, "procedural_shader", shaders);

   if (dispOk)
   {
      AiNodeDeclare (in_procNode, "disp_map", "constant ARRAY NODE");
      AiNodeSetArray(in_procNode, "disp_map", displacementShaders);

      if (in_arnoldParameters.IsValid())
      {
         AiNodeDeclare(in_procNode, "disp_zero_value", "constant FLOAT");
         CNodeSetter::SetFloat(in_procNode, "disp_zero_value", (float)ParAcc_GetValue(in_arnoldParameters, L"disp_zero_value", in_frame));

         AiNodeDeclare(in_procNode, "disp_height", "constant FLOAT");
         CNodeSetter::SetFloat(in_procNode, "disp_height", (float)ParAcc_GetValue(in_arnoldParameters, L"disp_height", in_frame));

         AiNodeDeclare(in_procNode, "disp_autobump", "constant BOOL");
         CNodeSetter::SetBoolean(in_procNode, "disp_autobump", (bool)ParAcc_GetValue(in_arnoldParameters, L"disp_autobump", in_frame));
         
         AiNodeDeclare(in_procNode, "disp_padding", "constant FLOAT");
         CNodeSetter::SetFloat(in_procNode, "disp_padding", (float)ParAcc_GetValue(in_arnoldParameters, L"disp_padding", in_frame));
      }
   }

   bool prop_adaptive_subdivision(false);
   bool subdiv_pixel_error_valid(false), subdiv_adaptive_error_valid(false);
   if (in_arnoldParameters.IsValid())
   {
      subdiv_pixel_error_valid    = in_arnoldParameters.GetParameter(L"subdiv_pixel_error").IsValid(); // < 3.9
      if (!subdiv_pixel_error_valid)
         subdiv_adaptive_error_valid = in_arnoldParameters.GetParameter(L"subdiv_adaptive_error").IsValid(); // >= 3.9

      if (subdiv_pixel_error_valid || subdiv_adaptive_error_valid) 
      {
         prop_adaptive_subdivision = (bool)ParAcc_GetValue(in_arnoldParameters, L"adaptive_subdivision", in_frame);
         if (prop_adaptive_subdivision)
         {
            if (subdiv_pixel_error_valid)
               adaptive_error = (float)ParAcc_GetValue(in_arnoldParameters, L"subdiv_pixel_error", in_frame);
            else
               adaptive_error = (float)ParAcc_GetValue(in_arnoldParameters, L"subdiv_adaptive_error", in_frame);
               adaptive_metric = ParAcc_GetValue(in_arnoldParameters, L"subdiv_adaptive_metric", in_frame).GetAsText();
               adaptive_space = ParAcc_GetValue(in_arnoldParameters, L"subdiv_adaptive_space", in_frame).GetAsText();
         }
         uint8_t prop_subdiv_iterations = (uint8_t)ParAcc_GetValue(in_arnoldParameters, L"subdiv_iterations", in_frame);
         subdiv_iterations += prop_subdiv_iterations;
      }
   }

   if (subdiv_iterations > 0)
   {
      Parameter subdrule = primitive.GetParameter(L"subdrule");
      if (subdrule.IsValid())
      {
         CValue subdruleType = subdrule.GetValue(in_frame);
         AiNodeDeclare(in_procNode, "subdiv_type", "constant STRING");
         if (subdruleType == 3)
            CNodeSetter::SetString(in_procNode, "subdiv_type", "linear");
         else
            CNodeSetter::SetString(in_procNode, "subdiv_type", "catclark");
      }

      AiNodeDeclare(in_procNode, "subdiv_iterations", "constant BYTE");
      CNodeSetter::SetByte(in_procNode, "subdiv_iterations", subdiv_iterations);

      if (subdiv_pixel_error_valid) // < 3.9
      {
         AiNodeDeclare(in_procNode, "subdiv_pixel_error", "constant FLOAT");
         CNodeSetter::SetFloat(in_procNode, "subdiv_pixel_error", adaptive_error);
      }
      else // >= 3.9
      {
         AiNodeDeclare(in_procNode, "subdiv_adaptive_error", "constant FLOAT");
         CNodeSetter::SetFloat(in_procNode, "subdiv_adaptive_error", adaptive_error);
      }

      AiNodeDeclare(in_procNode, "subdiv_adaptive_metric", "constant STRING");
      CNodeSetter::SetString(in_procNode, "subdiv_adaptive_metric", adaptive_metric.GetAsciiString());
      AiNodeDeclare(in_procNode, "subdiv_adaptive_space", "constant STRING");
      CNodeSetter::SetString(in_procNode, "subdiv_adaptive_space", adaptive_space.GetAsciiString());
   }
}


// Return whether the input string does NOT start with "procedural_material" or "scene_material" (case insensitive)
// If returning true, the procedural loader will load the material from the procedural object.
// Else, the materials loaded from the ass (or procedural) file will be preserved.
//
// @param in_materialName       The input string
//
// @return false if in_materialName starts with "procedural_material" or "scene_material" (case insensitive), else true
//
bool UseProceduralMaterial(const CString &in_materialName)
{
   ULONG length = in_materialName.Length();
   if (length > 13)
   {
      CString s = in_materialName.GetSubString(0, 14);
      if (s.IsEqualNoCase(L"scene_material"))
         return false;
      if (length > 15)
      {
         s = in_materialName.GetSubString(0, 16);
         if (s.IsEqualNoCase(L"procedural_material"))
            return false;
      }
   }
   return true;
}


 
