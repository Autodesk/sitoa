/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "renderer/Renderer.h"


CRenderInstance g_RenderRegionPass;

CRenderInstance* g_Render;
CRenderInstance* GetRenderInstance() { return g_Render; }

CRenderOptions* GetRenderOptions() { return &g_Render->m_renderOptions; } 

CMessageQueue g_messageQueue;
CMessageQueue *GetMessageQueue() { return &g_messageQueue; }

// Initialize the reference counter of the Lock Scene Data mechanism
int LockSceneData::m_referenceCount = 0;

SITOA_CALLBACK ArnoldRender_Init(CRef &in_ctxt)
{
   Context ctxt(in_ctxt);
   Renderer renderer = ctxt.GetSource();

   // Tell the render manager what render processes we support.
   CLongArray process;
   process.Add(siRenderProcessRender);
   process.Add(siRenderProcessExportObjectArchive);
   renderer.PutProcessTypes(process);

   // Specify the custom property to use for the renderer options
   renderer.AddProperty(siRenderPropertyOptions, L"Arnold Render Options");

   // Specifing Available output formats
   renderer.AddOutputImageFormat(L"Arnold TIFF", L"tif");
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGBA", siImageBitDepthInteger8);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGBA", siImageBitDepthInteger16);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGBA", siImageBitDepthInteger32);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGBA", siImageBitDepthFloat32);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGB",  siImageBitDepthInteger8);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGB",  siImageBitDepthInteger16);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGB",  siImageBitDepthInteger32);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGB",  siImageBitDepthFloat32);

   renderer.AddOutputImageFormat(L"Arnold JPEG", L"jpg");
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGB", siImageBitDepthInteger8);

   renderer.AddOutputImageFormat(L"Arnold PNG", L"png");
   // png's alpha not written anymore by Arnold 4.1, so we only allow for RGB
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGB",  siImageBitDepthInteger8);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType, L"RGB",  siImageBitDepthInteger16);

   renderer.AddOutputImageFormat(L"Arnold OpenEXR", L"exr");
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType,        L"RGBA",   siImageBitDepthFloat16);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType,        L"RGBA",   siImageBitDepthFloat32);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType,        L"RGB",    siImageBitDepthFloat16);
   renderer.AddOutputImageFormatSubType(siRenderChannelColorType,        L"RGB",    siImageBitDepthFloat32);
   renderer.AddOutputImageFormatSubType(siRenderChannelGrayscaleType,    L"FLOAT",  siImageBitDepthFloat16);
   renderer.AddOutputImageFormatSubType(siRenderChannelGrayscaleType,    L"FLOAT",  siImageBitDepthFloat32);
   renderer.AddOutputImageFormatSubType(siRenderChannelDepthType,        L"FLOAT",  siImageBitDepthFloat16);
   renderer.AddOutputImageFormatSubType(siRenderChannelDepthType,        L"FLOAT",  siImageBitDepthFloat32);
   renderer.AddOutputImageFormatSubType(siRenderChannelVectorType,       L"VECTOR", siImageBitDepthFloat16);
   renderer.AddOutputImageFormatSubType(siRenderChannelVectorType,       L"VECTOR", siImageBitDepthFloat32);
   renderer.AddOutputImageFormatSubType(siRenderChannelLabelType,        L"INT",    siImageBitDepthInteger32);
   renderer.AddOutputImageFormatSubType(siRenderChannelNormalVectorType, L"VECTOR", siImageBitDepthFloat16);
   renderer.AddOutputImageFormatSubType(siRenderChannelNormalVectorType, L"VECTOR", siImageBitDepthFloat32);

   // Do not delete, else we don't get the .ass option when Export->Selected Objects
   // The price to pay is that .ass will also show up as a valid filter when browsing from the standin primitive
   renderer.PutObjectArchiveFormat(L"Arnold Scene Source", L"ass", false, false);

   // Assigning actual RenderInstance with RenderRegionPass
   g_Render = &g_RenderRegionPass;

   return CStatus::OK;
}


SITOA_CALLBACK ArnoldRender_Term(CRef &in_ctxt)
{
   g_Render->DestroyScene(true);
   return CStatus::OK;
}


SITOA_CALLBACK ArnoldRender_Cleanup(CRef &in_ctxt)
{     
   Context ctxt(in_ctxt);
   
   if (ctxt.GetAttribute(L"RenderID").GetAsText().IsEmpty())
      g_Render->DestroyScene(false);
  
   return CStatus::OK;
}


SITOA_CALLBACK ArnoldRender_Abort(CRef &in_ctxt)
{
   if (!g_Render->InterruptRenderSignal())
      g_Render->InterruptRender();

   return CStatus::OK;
}


SITOA_CALLBACK ArnoldRender_Quality(CRef &in_ctxt)
{
   return CStatus::OK;
}


SITOA_CALLBACK ArnoldRender_Process(CRef &in_ctxt)
{
   CStatus status = CStatus::OK;
  
   RendererContext rendererContext = RendererContext(in_ctxt);
   // read the rendering options to get m_rebuild_mode
   Renderer renderer = rendererContext.GetSource();
   Property renderProperty = rendererContext.GetRendererProperty(rendererContext.GetTime());
   GetRenderOptions()->Read(renderProperty);
   
   if (GetRenderOptions()->m_ipr_rebuild_mode == eIprRebuildMode_Always)
      g_Render->DestroyScene(false);
   // frame change ? Destroy if not in flythrough mode
   else if (GetRenderOptions()->m_ipr_rebuild_mode != eIprRebuildMode_Flythrough)
      if (g_Render->GetFrame() != rendererContext.GetTime())
         g_Render->DestroyScene(false);

   g_Render->SetInterruptRenderSignal(false);     

   g_Render->InitializeRender(in_ctxt);

   siRenderProcessType process = (siRenderProcessType)((int)rendererContext.GetAttribute(L"Process"));

   if (process == siRenderProcessRender)
      status = g_Render->Process();
   else if (process == siRenderProcessExportObjectArchive)
      status = g_Render->Export();

   return status;
}


SITOA_CALLBACK ArnoldRender_Query(CRef &in_ctxt) 
{
   Context ctxt(in_ctxt);
   int type = ctxt.GetAttribute(L"QueryType");

   switch(type)
   {
      case siRenderQueryArchiveIsValid:
      {
         CString filename = ctxt.GetAttribute(L"Filename");
         if (!filename.IsEmpty())
         {
            CStringArray fileParts = filename.Split(L".");
            CString extension = fileParts[fileParts.GetCount()-1];

            ctxt.PutAttribute(L"Valid", false);
            ctxt.PutAttribute(L"MultiFrame", false);

            if (extension == L"ass" || (extension == L"gz" && fileParts[fileParts.GetCount()-2] == L"ass"))
            {
               FILE* pFile;
               pFile = fopen(filename.GetAsciiString(),"r");
               if (pFile!=NULL)
               {
                  ctxt.PutAttribute(L"Valid", true);
                  fclose(pFile);
               }
            }
         }
         break;
      }
      case siRenderQueryWantDirtyList:
      {
         ctxt.PutAttribute(L"WantDirtyList", true); // #1565 for Softimage 2014
         break;
      }
      case siRenderQueryDisplayBitDepths:
      {
         CLongArray bitDepths;
         bitDepths.Add(siImageBitDepthInteger8);
         bitDepths.Add(siImageBitDepthInteger16);
         
         CString softimageVersionString = Application().GetVersion();
         CStringArray softimageVersion = softimageVersionString.Split(L".");
         if (atoi(softimageVersion[0].GetAsciiString()) >= 10)
            bitDepths.Add(siImageBitDepthFloat32);
         
         ctxt.PutAttribute(L"BitDepths", bitDepths);
         break;
      }
#if XSISDK_VERSION > 11000   // #1934, #1845
      case siRenderQueryHasPreMulAlphaOutput:
      {
         ctxt.PutAttribute(L"HasPreMulAlphaOutput", false);
         break;
      }
#endif      
   }

   return CStatus::OK;
}

LockSceneData::LockSceneData()
: m_renderer((Renderer) GetRenderInstance()->GetRendererRef())
{
   if (GetRenderInstance()->GetRenderType() != L"Export")
   {
      ++m_referenceCount;
      m_status = m_renderer.LockSceneData();
   }
}


LockSceneData::~LockSceneData()
{
   if (m_status == CStatus::OK && GetRenderInstance()->GetRenderType() != L"Export")
   {
      --m_referenceCount;
      m_status = m_renderer.UnlockSceneData();   
   }
}
