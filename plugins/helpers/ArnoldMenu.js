/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

function XSILoadPlugin(in_reg)
{
   if (Application.plugins("Arnold Tools") == null)
      LoadPlugin(XSIUtils.BuildPath(in_reg.OriginPath, "ArnoldTools.js"));
   
   var h = SItoAToolHelper();
   h.SetPluginInfo(in_reg, "Arnold Menu");
   // exposing the shader appliers as commands, so to make them public
   in_reg.RegisterCommand("AddShader",       "SITOA_AddShader");
   in_reg.RegisterCommand("AddShaderStack",  "SITOA_AddShaderStack");
   in_reg.RegisterCommand("AddCamera",       "SITOA_AddCamera");
   // This main menu is dynamic (re-populated every time), because the so/dll sub-menu 
   // may change if the user changes the shaders search path
   in_reg.RegisterMenu(siMenuAnchorPoints.siMenuMainTopLevelID, "Arnold", true, true);
   // properties sub-menu
   in_reg.RegisterMenu(siMenuAnchorPoints.siMenuTbGetPropertyID, "Arnold Properties", true, false);
   // material sub-menu
   in_reg.RegisterMenu(siMenuAnchorPoints.siMenuTbGetMaterialID, "Arnold Shaders", true, false);
   // pass sub-menu
   in_reg.RegisterMenu(siMenuAnchorPoints.siMenuTbRenderPassEditID, "Arnold Pass Shaders", true, false);
   // lights menu  
   in_reg.RegisterMenu(siMenuAnchorPoints.siMenuTbGetLightID, "Arnold Lights", false, false);
   // camera menu
   in_reg.RegisterMenu(siMenuAnchorPoints.siMenuTbGetCameraID, "Arnold Cameras", false, false);

   return true;
}


// commands

function AddShader_Init(in_ctxt)
{
   var oCmd = in_ctxt.Source;
   var oArgs = oCmd.Arguments;
   oArgs.Add("in_shaderName");
   oArgs.Add("in_connectionPoint");
   oArgs.Add("in_collection");
   oArgs.Add("in_inspect");
   return true;
}


function AddShader_Execute(in_shaderName, in_connectionPoint, in_collection, in_inspect)
{
   var collection = new ActiveXObject("XSI.Collection");
   collection = in_collection == null ? selection : in_collection;

   var inspect = in_inspect == null ? true : in_inspect;

   if (collection.count == 0)
   {
      logMessage("[sitoa] Please select a valid target for " + in_shaderName, siErrorMsg);
      return;
   }
   
   var shader = null;

   // texture shaders, assign it to the current mat without destroying it
   if (in_connectionPoint != "surface")
   {
      for (var i=0; i<collection.count; i++)
      {
         var xsiObj = collection.Item(i);

         if (xsiObj.Type == "camera")
            shader = CreateShaderFromProgID(in_shaderName);
         else
         {
            if (xsiObj.Type == "material")
               xsiObj = xsiObj.Parent;
              
            if (xsiObj.material == null)
            {
               logMessage("[sitoa] Can't assign " + in_shaderName + ". " + xsiObj + " doesn't have a material", siErrorMsg);
               return;
            }

            shader = CreateShaderFromProgID(in_shaderName, xsiObj.material, null);
           
            if (in_connectionPoint != null && in_connectionPoint != "")
               SIConnectShaderToCnxPoint(shader, xsiObj.material + "." + in_connectionPoint, false);
         }
      }
   }
   else
   {
      // since we want to use ApplyShader to create a void material, but we don't have presets anymore, 
      // we apply a basic preset (Constant), then delete the shader, and then connect the desired surface shader 
      var factoryBase = Application.InstallationPath(siFactoryPath); 
      var path = XSIUtils.BuildPath(factoryBase, "Data", "DSPresets", "Shaders", "Material", "Constant.Preset");
      ApplyShader(path, "", null, "", siLetLocalMaterialsOverlap);
      for (var i=0; i<collection.count; i++)
      {
         var xsiObj = collection.Item(i);
         if (xsiObj.material == null)
            continue;

         shader = xsiObj.material.surface.source.parent;
         if (shader != null)
         {
            DeleteObj(shader);      
            shader = CreateShaderFromProgID(in_shaderName, xsiObj.material, null);
            // we only want to add a closure shader when it's needed
            // closures have 20 as OutputType value so we test for that
            if (shader.OutputType == 20)
            {
               var closure = CreateShaderFromProgID("Arnold.closure.1.0", xsiObj.material, null);
               SIConnectShaderToCnxPoint(shader, closure + ".closure", false);
               SIConnectShaderToCnxPoint(closure, xsiObj.material + ".surface", false);
            }
            else
            {
               SIConnectShaderToCnxPoint(shader, xsiObj.material + ".surface", false);
            }
         }
      }
   }
   
   if (inspect && shader != null)
      InspectObj(shader);
}


function AddShaderStack_Init(in_ctxt)
{
   var oCmd = in_ctxt.Source;
   var oArgs = oCmd.Arguments;
   oArgs.Add("in_shaderName");
   oArgs.Add("in_connectionPoint");
   return true;
}

function AddShaderStack_Execute(in_shaderName, in_connectionPoint)
{
   var pass = ActiveProject.ActiveScene.ActivePass;
   var shader = CreateShaderFromProgID(in_shaderName, pass, null);
   // we only want to add a closure shader when it's needed
   // closures have 20 as OutputType value so we test for that 
   if (shader.OutputType == 20)
   {
      var closure = CreateShaderFromProgID("Arnold.closure.1.0", pass, null);
      SIConnectShaderToCnxPoint(shader, closure + ".closure", false);
      SIConnectShaderToCnxPoint(closure, pass + "." + in_connectionPoint, false);
   }
   else
   {
      SIConnectShaderToCnxPoint(shader, pass + "." + in_connectionPoint, false);
   }
   InspectObj(shader);
}


function AddCamera_Init(in_ctxt)
{
   var oCmd = in_ctxt.Source;
   var oArgs = oCmd.Arguments;
   oArgs.Add("in_cameraType");
   return true;
}

function AddCamera_Execute(in_cameraType)
{
   if (!(in_cameraType == "persp_camera" || in_cameraType == "spherical_camera" ||  in_cameraType == "cyl_camera" || 
         in_cameraType == "vr_camera" || in_cameraType == "fisheye_camera"))
   {
      logMessage("[sitoa] SITOA_AddCamera: Invalid camera type " + in_cameraType, siErrorMsg);
      return;
   }
   var cameraRoot = GetPrimCamera("Camera", "Arnold Camera", "");
   var children = cameraRoot.FindChildren("", siCameraPrimType);
   var camera = children(0);
   var prop = SITOA_AddCameraOptionsProperties(camera);
   prop(0).camera_type.value = in_cameraType;
   return prop(0);
}


// the Arnold top menu
function Arnold_Init(io_Context)
{
   var xsiMenu = io_Context.Source;
  
   var properties = xsiMenu.AddItem("Properties", siMenuItemSubmenu);
   AddPropertiesSubMenu(properties);
 
   var lights = xsiMenu.AddItem("Lights", siMenuItemSubmenu);
   AddLightsSubMenu(lights, false);
   
   var filters = xsiMenu.AddItem("Light Filters", siMenuItemSubmenu);
   AddFiltersSubMenu(filters);
   
   var shaders = xsiMenu.AddItem("Shaders", siMenuItemSubmenu);
   AddShadersSubMenu(shaders);

   var cameras = xsiMenu.AddItem("Cameras", siMenuItemSubmenu);
   AddCamerasSubMenu(cameras);

   var menuTitle = XSIUtils.IsWindowsOS() ? "DLL Shaders" : "DSO Shaders";
   var shaderDef = xsiMenu.AddItem(menuTitle, siMenuItemSubmenu);
  
   var s = SITOA_GetShaderDef(); // this is exposed by cpp, and returns the list of the dll/so shaders 
   var split = s.split(";");
   for (var i=0; i<split.length; i++)
   {
      if (split[i] == "separator")
         shaderDef.AddSeparatorItem();
      else
         shaderDef.AddCallbackItem(split[i], "OnShaderDef");
   }   

   var commands = xsiMenu.AddItem("Commands", siMenuItemSubmenu);
   AddCommandsSubMenu(commands)
   
   var help = xsiMenu.AddItem("Help", siMenuItemSubmenu);
   AddHelpSubMenu(help)
}


// properties sub-menu
function ArnoldProperties_Init(io_Context)
{
   var xsiMenu = io_Context.Source;
   AddPropertiesSubMenu(xsiMenu);
}


function AddPropertiesSubMenu(in_menu)
{
   in_menu.AddCommandItem("Visibility",      "AddVisibilityProperties");
   in_menu.AddCommandItem("Matte",           "AddMatteProperties");
   in_menu.AddCommandItem("Parameters",      "AddParametersProperties");
   in_menu.AddCommandItem("Procedural",      "AddProceduralProperties");
   in_menu.AddCommandItem("Volume",          "AddVolumeProperties");
   in_menu.AddCommandItem("User Options",    "AddUserOptionsProperties");
   in_menu.AddCommandItem("Texture Options", "AddTextureOptionsProperties");
   in_menu.AddCommandItem("Camera Options",  "AddCameraOptionsProperties");
   in_menu.AddCommandItem("Sidedness",       "AddSidednessProperties");
   in_menu.AddCommandItem("Denoiser",        "AddDenoiserProperties");
}


// materials sub-menu
function ArnoldShaders_Init(io_Context)
{
   var xsiMenu = io_Context.Source;
   AddShadersSubMenu(xsiMenu);
}


function AddShadersSubMenu(in_menu)
{
   in_menu.AddCallbackItem("Car Paint",         "OnShadersMenu");
   in_menu.AddCallbackItem("Standard Surface",  "OnShadersMenu");
   in_menu.AddCallbackItem("Standard Hair",     "OnShadersMenu");
   in_menu.AddCallbackItem("Toon",              "OnShadersMenu");
   in_menu.AddCallbackItem("Utility",           "OnShadersMenu");
   in_menu.AddSeparatorItem();
   in_menu.AddCallbackItem("Camera Projection", "OnShadersMenu");
   in_menu.AddCallbackItem("Complex IOR",       "OnShadersMenu");
   in_menu.AddCallbackItem("Curvature",         "OnShadersMenu");
   in_menu.AddCallbackItem("Flakes",            "OnShadersMenu");
   in_menu.AddCallbackItem("Image",             "OnShadersMenu");
   in_menu.AddCallbackItem("Noise",             "OnShadersMenu");
   in_menu.AddCallbackItem("Thin Film",         "OnShadersMenu");
   in_menu.AddCallbackItem("Triplanar",         "OnShadersMenu");
   in_menu.AddSeparatorItem();
   in_menu.AddCallbackItem("Standard Volume",   "OnShadersMenu");
   in_menu.AddSeparatorItem();
   in_menu.AddCallbackItem("Physical Sky",      "OnShadersMenu");   
}


// pass sub-menu
function ArnoldPassShaders_Init(io_Context)
{
   var xsiMenu = io_Context.Source;
   xsiMenu.AddCallbackItem("Atmosphere Volume", "OnShadersMenu");
   xsiMenu.AddCallbackItem("Fog",               "OnShadersMenu");
   xsiMenu.AddSeparatorItem();
   xsiMenu.AddCallbackItem("Cryptomatte",       "OnShadersMenu");
}

// lights sub-menu
function ArnoldLights_Init(io_Context)
{
   var xsiMenu = io_Context.Source;
   AddLightsSubMenu(xsiMenu, true);
}


function AddLightsSubMenu(in_menu, in_longName)
{
   var name = in_longName ? "Arnold Point Light" : "Point";
   in_menu.AddCommandItem(name, "AddPointLight");
   name = in_longName ? "Arnold Distant Light" : "Distant";
   in_menu.AddCommandItem(name, "AddDistantLight");
   name = in_longName ? "Arnold Spot Light" : "Spot";
   in_menu.AddCommandItem(name, "AddSpotLight");
   name = in_longName ? "Arnold Skydome Light" : "Skydome";
   in_menu.AddCommandItem(name, "AddSkydomeLight");
   name = in_longName ? "Arnold Disk Light" : "Disk";
   in_menu.AddCommandItem(name, "AddDiskLight");
   name = in_longName ? "Arnold Quad Light" : "Quad";
   in_menu.AddCommandItem(name, "AddQuadLight");
   name = in_longName ? "Arnold Cylinder Light" : "Cylinder";
   in_menu.AddCommandItem(name, "AddCylinderLight");
   name = in_longName ? "Arnold Mesh Light" : "Mesh";
   in_menu.AddCommandItem(name, "AddMeshLight");
   name = in_longName ? "Arnold Photometric Light" : "Photometric";
   in_menu.AddCommandItem(name, "AddPhotometricLight");
}


function AddFiltersSubMenu(in_menu)
{
   in_menu.AddCommandItem("Barndoor",    "AddBarndoorFilter");
   in_menu.AddCommandItem("Blocker",     "AddLightBlockerFilter");
   in_menu.AddCommandItem("Decay",       "AddLightDecayFilter");
   in_menu.AddCommandItem("Gobo",        "AddGoboFilter");
   in_menu.AddCommandItem("Passthrough", "AddPassthroughFilter");
}


// cameras sub-menu
function ArnoldCameras_Init(io_Context)
{
   var xsiMenu = io_Context.Source;
   AddCamerasSubMenu(xsiMenu, true);
}

function AddCamerasSubMenu(in_menu, in_longName)
{
   var name = in_longName ? "Arnold Perspective Camera" : "Perspective";
   in_menu.AddCallbackItem(name, "OnCamerasMenu");
   name = in_longName ? "Arnold Spherical Camera" : "Spherical";
   in_menu.AddCallbackItem(name, "OnCamerasMenu");
   name = in_longName ? "Arnold Cylindrical Camera" : "Cylindrical";
   in_menu.AddCallbackItem(name, "OnCamerasMenu");
   name = in_longName ? "Arnold Fisheye Camera" : "Fisheye";
   in_menu.AddCallbackItem(name, "OnCamerasMenu");
   name = in_longName ? "Arnold VR Camera" : "VR";
   in_menu.AddCallbackItem(name, "OnCamerasMenu");
}


function AddCommandsSubMenu(in_menu)
{
   in_menu.AddCallbackItem("Destroy Scene",                "OnCommandsMenu");
   in_menu.AddCallbackItem("Flush Textures",               "OnCommandsMenu");
   in_menu.AddCallbackItem("Regenerate Rendering Options", "OnCommandsMenu");
   in_menu.AddCallbackItem("Create Render Channels",       "OnCommandsMenu");
   in_menu.AddCallbackItem("Register CLM Product Information", "OnCommandsMenu");
}


function AddHelpSubMenu(in_menu)
{
   item = in_menu.AddCallbackItem("User Guide",      "OnHelpMenu");
   item = in_menu.AddSeparatorItem();
   item = in_menu.AddCallbackItem("Solid Angle",     "OnHelpMenu");
   item = in_menu.AddCallbackItem("Mailing Lists",   "OnHelpMenu");
   item = in_menu.AddCallbackItem("Knowledge Base",  "OnHelpMenu");
   item = in_menu.AddCallbackItem("Support",         "OnHelpMenu");
   item = in_menu.AddCallbackItem("Support Blog",    "OnHelpMenu");
   item = in_menu.AddSeparatorItem();
   item = in_menu.AddCallbackItem("Licensing Home",  "OnHelpMenu");
   item = in_menu.AddCallbackItem("Licensing",       "OnHelpMenu");
   item = in_menu.AddSeparatorItem();
   item = in_menu.AddCallbackItem("Legal Notice",    "OnHelpMenu");
   item = in_menu.AddCallbackItem("About SItoA",     "OnHelpMenu");
}

function OnHelpMenu(in_ctxt)
{
   var item = in_ctxt.Source;
   var url;
   switch (item.Name)
   {
      case "User Guide":
         url = "https://support.solidangle.com/display/A5SItoAUG/Arnold+for+Softimage+User+Guide";
         break;
      case "Solid Angle":
         url = "https://www.solidangle.com";
         break;
      case "Mailing Lists":
         url = "https://subscribe.solidangle.com";
         break;
      case "Knowledge Base":
         url = "https://ask.solidangle.com";
         break;
      case "Support":
         url = "https://support.solidangle.com";
         break;
      case "Support Blog":
         url = "https://support.solidangle.com/blog/arnsupp";
         break;
      case "Licensing Home":
         url = "https://www.solidangle.com/support/licensing";
         break;
      case "Licensing":
         Licensing();
         return;
      case "Legal Notice":
         LegalNotice();
         return;
      case "About SItoA":
         About();
         return;
   }

   if (XSIUtils.IsWindowsOS())
   {
      var objShell = new ActiveXObject("shell.application");
      objShell.ShellExecute(url, "", "", "open", 1);
   }
   else
   {
      var cmd = "xdg-open " + url;
      System(cmd);
   }
}

// the menus' callbacks

function OnCommandsMenu(in_ctxt)
{
   var item = in_ctxt.Source;
   switch (item.Name)
   {
      case "Destroy Scene":
         SITOA_DestroyScene();
         break;
      case "Flush Textures":
         SITOA_FlushTextures();
         break;
      case "Regenerate Rendering Options":
         SITOA_RegenerateRenderOptions();
         break;
      case "Create Render Channels":
         SITOA_CreateRenderChannels();
         break;
      case "Register CLM Product Information":
         SITOA_PitReg();
         break;
   }
}

function OnShadersMenu(in_ctxt)
{
   var item = in_ctxt.Source;
   switch (item.Name)
   {
      case "Car Paint":
         SITOA_AddShader("Arnold.car_paint.1.0", "surface");
         break;
      case "Standard Surface":
         SITOA_AddShader("Arnold.standard_surface.1.0", "surface");
         break;
      case "Standard Hair":
         SITOA_AddShader("Arnold.standard_hair.1.0", "surface");
         break;
      case "Toon":
         SITOA_AddShader("Arnold.toon.1.0", "surface");
         break;
      case "Utility":
         SITOA_AddShader("Arnold.utility.1.0", null);
         break;
      case "Camera Projection":
         SITOA_AddShader("Arnold.camera_projection.1.0", null);
         break;
      case "Complex IOR":
         SITOA_AddShader("Arnold.complex_ior.1.0", null);
         break;
      case "Curvature":
         SITOA_AddShader("Arnold.curvature.1.0", null);
         break;
      case "Flakes":
         SITOA_AddShader("Arnold.flakes.1.0", null);
         break;
      case "Image":
         SITOA_AddShader("Arnold.image.1.0", null);
         break;
      case "Noise":
         SITOA_AddShader("Arnold.noise.1.0", null);
         break;
      case "Thin Film":
         SITOA_AddShader("Arnold.thin_film.1.0", null);
         break;
      case "Triplanar":
         SITOA_AddShader("Arnold.triplanar.1.0", null);
         break;
      case "Standard Volume":
         SITOA_AddShader("Arnold.standard_volume.1.0", "surface");
         break;
      case "Atmosphere Volume":
         SITOA_AddShaderStack("Arnold.atmosphere_volume.1.0", "VolumeShaderStack");
         break;
      case "Fog":
         SITOA_AddShaderStack("Arnold.fog.1.0", "VolumeShaderStack");
         break;
      case  "Physical Sky":
         SITOA_AddShaderStack("Arnold.physical_sky.1.0", null);
         break;
      case  "Cryptomatte":
         SITOA_AddShaderStack("Arnold.cryptomatte.1.0", "OutputShaderStack");
         break;
    }
}


function OnCamerasMenu(in_ctxt)
{
   var item = in_ctxt.Source;
   switch (item.Name)
   {
      case "Arnold Perspective Camera":
      case "Perspective":
         SITOA_AddCamera("persp_camera");
         break;
      case "Arnold Spherical Camera":
      case "Spherical":
         SITOA_AddCamera("spherical_camera");
         break;
      case "Arnold Cylindrical Camera":
      case "Cylindrical":
         SITOA_AddCamera("cyl_camera");
         break;
      case "Arnold Fisheye Camera":
      case "Fisheye":
         SITOA_AddCamera("fisheye_camera");
         break;
      case "Arnold VR Camera":
      case "VR":
         SITOA_AddCamera("vr_camera");
         break;
    }
}


function OnShaderDef(in_ctxt)
{
  // use the progid for the dll/so shaders
   var item = in_ctxt.Source;
   SITOA_AddShader("Arnold." + item.Name + ".1.0");
}


// credits
function About()
{
   var s;
   var prop = ActiveSceneRoot.AddCustomProperty("About", false) ;

   sitoaGuys = new Array(
                         "Luis Armengol", 
                         "Borja Morales", 
                         "Stefano Jannuzzo"
                         );
                         
   thanksGuys = new Array(
                          "Andreas Bystrom",
                          "Steven Caron",
                          "Julien Dubuisson",
                          "Steffen Dunner",
                          "Michael Heberlein",
                          "Paul Hudson",
                          "Halfdan Ingvarsson",
                          "Vladimir Jankijevic",
                          "Alan Jones",
                          "Guillaume Laforge",
                          "Thomas Mansencal",
                          "Helge Mathee",
                          "Eric Mootz",
                          "Holger Schoenberger",
                          "Frederic Servant",
                          "Jules Stevenson",
                          "Jens Lindgren"
                          );
                         
   var layout = prop.PPGLayout;
   // dummy param for the logo bitmap
   prop.AddParameter("dummy", siBool);
   
   // logo, title and versions
   layout.AddRow();
   
      layout.AddGroup("", false, 15);
      var item = layout.AddItem("dummy", "", siControlBitmap);
      item.SetAttribute(siUINoLabel, true);
      
      var arnoldPlugin = Application.plugins("Arnold Menu");
      if (arnoldPlugin != null)
      {
         var logoPath = XSIUtils.BuildPath(arnoldPlugin.OriginPath, "Pictures", "SItoA_Logo.bmp")
         item.SetAttribute(siUIFilePath, logoPath);
      }
      layout.EndGroup();

      layout.AddGroup("", false, 85);
      var versions = SITOA_ShowVersion(false);
      splitS = versions.split(";");
      var sitoaVersion = splitS[0];
      var arnoldVersion = splitS[1];

      s = "SItoA " + sitoaVersion + "\r\n";
      s+= "Arnold Core " + arnoldVersion;
      layout.AddStaticText(s);
   
      // copyright
      s = "Copyright (c) 2001-2009 Marcos Fajardo and Copyright (c) 2009-2017 Solid Angle S.L.\n\
All rights reserved"      
      layout.AddStaticText(s);

      // devs
      s = "Developed by : ";
      for (var i=0; i<sitoaGuys.length; i++)
      {
         s+= sitoaGuys[i];
         if (i < sitoaGuys.length-1)
            s+= ", ";
      }
      layout.AddStaticText(s);
   
      // contributors (just add items to the thanksGuys array if we have more)
      s = "Acknowledgements : ";
      for (var i=0; i<thanksGuys.length; i++)
      {
         s+= thanksGuys[i];
         if (i < thanksGuys.length-1)
            s+= ", ";
      }
      layout.AddStaticText(s, 500, 60);

   s = "\n\All use of this Software is subject to the terms and conditions of the software license agreement accepted upon installation of this Software and/or packaged with the Software.\n\
\n\
Third-Party Software Credits and Attributions\n\
\n\
Portions related to OpenImageIO Copyright 2008-2015 Larry Gritz et al. All Rights Reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  * Neither the name of the software's owners nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to IlmBase Copyright (c) 2002-2011, Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd. All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  Neither the name of Industrial Light & Magic nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to libjpeg (C) 1991-1998, Thomas G. Lane. All Rights Reserved except as specified below. The authors make NO WARRANTY or representation, either express or implied, with respect to this software, its quality, accuracy, merchantability, or fitness for a particular purpose. This software is provided \"AS IS\", and you, its user, assume the entire risk as to its quality and accuracy. Permission is hereby granted to use, copy, modify, and distribute this software (or portions thereof) for any purpose, without fee, subject to these conditions:  (1) If any part of the source code for this software is distributed, then this README file must be included, with this copyright and no-warranty notice unaltered; and any additions, deletions, or changes to the original files must be clearly indicated in accompanying documentation. (2) If only executable code is distributed, then the accompanying documentation must state that \"this software is based in part on the work of the Independent JPEG Group\". (3) Permission for use of this software is granted only if the user accepts full responsibility for any undesirable consequences; the authors accept NO LIABILITY for damages of any kind. These conditions apply to any software derived from or based on the IJG code, not just to the unmodified library. If you use our work, you ought to acknowledge us. Permission is NOT granted for the use of any IJG author's name or company name in advertising or publicity relating to this software or products derived from it. This software may be referred to only as \"the Independent JPEG Group's software\". We specifically permit and encourage the use of this software as the basis of commercial products, provided that all warranty or liability claims are assumed by the product vendor. ansi2knr.c is included in this distribution by permission of L. Peter Deutsch, sole proprietor of its copyright holder, Aladdin Enterprises of Menlo Park, CA. ansi2knr.c is NOT covered by the above copyright and conditions, but instead by the usual distribution terms of the Free Software Foundation; principally, that you must include source code if you redistribute it. (See the file ansi2knr.c for full details). However, since ansi2knr.c is not needed as part of any program generated from the IJG code, this does not limit you more than the foregoing paragraphs do. We are required to state that \"The Graphics Interchange Format(c) is the Copyright property of CompuServe Incorporated. GIF(sm) is a Service Mark property of CompuServe Incorporated.\"\n\
\n\
Portions related to libtiff Copyright (c) 1988-1997 Sam Leffler.  Copyright (c) 1991-1997 Silicon Graphics, Inc.  Permission to use, copy, modify, distribute, and sell this software and its documentation for any purpose is hereby granted without fee, provided that (i) the above copyright notices and this permission notice appear in all copies of the software and related documentation, and (ii) the names of Sam Leffler and Silicon Graphics may not be used in any advertising or publicity relating to the software without the specific, prior written permission of Sam Leffler and Silicon Graphics.  THE SOFTWARE IS PROVIDED \"AS-IS\" AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\n\
\n\
Portions related to OpenEXR Copyright (c) 2002-2011, Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd. All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  Neither the name of Industrial Light & Magic nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to zlib Copyright (C) 1995-2013 Jean-loup Gailly and Mark Adler. This software is provided 'as-is', without any express or implied warranty.  In no event will the authors be held liable for any damages arising from the use of this software.  Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:  1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.  2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software. 3. This notice may not be removed or altered from any source distribution.  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to OpenSubdiv Copyright (C) 2013 Pixar.  Licensed under the Apache License, Version 2.0 (the \"Apache License\") with the following modification; you may not use this file except in compliance with the Apache License and the following modification to it: Section 6. Trademarks. is deleted and replaced with: 6. Trademarks. This License does not grant permission to use the trade names, trademarks, service marks, or product names of the Licensor and its affiliates, except as required to comply with Section 4(c) of the License and to reproduce the content of the NOTICE file.  You may obtain a copy of the Apache License at http://www.apache.org/licenses/LICENSE-2.0.  Unless required by applicable law or agreed to in writing, software distributed under the Apache License with the above modification is distributed on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the Apache License for the specific language governing permissions and limitations under the Apache License.  \n\
\n\
Portions related to gperftools Copyright (C) 2005 Google Inc.  All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  * Neither the name of Google Inc. nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to OpenVDB Copyright (c) 2012-2013 DreamWorks Animation LLC.  All rights reserved. This software is distributed under the Mozilla Public License 2.0 (http://www.mozilla.org/MPL/2.0/).  Redistributions of source code must retain the above copyright and license notice and the following restrictions and disclaimer.  Neither the name of DreamWorks Animation nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00. A text copy of this license and the source code for OpenVDB (and modifications made by Solid Angle, if any) can be found at the Autodesk website www.autodesk.com/lgplsource.\n\
\n\
Portions related to Boost Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and accompanying documentation covered by this license (the \"Software\") to use, reproduce, display, distribute, execute, and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the Software is furnished to do so, all subject to the following:  The copyright notices in the Software and this entire statement, including the above license grant, this restriction and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all derivative works of the Software, unless such copies or derivative works are solely in the form of machine-executable object code generated by a source language processor.  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to pystring Copyright (c) 2008-2010, Sony Pictures Imageworks Inc.  All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  Neither the name of the organization Sony Pictures Imageworks nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to dirent Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.  This file contains Original Code and/or Modifications of Original Code as defined in and that are subject to the Apple Public Source License Version 2.0 (the 'License'). You may not use this file except in compliance with the License. Please obtain a copy of the License at http://www.opensource.apple.com/apsl/ and read it before using this file.  The Original Code and all software distributed under the License are distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  Please see the License for the specific language governing rights and limitations under the License.\n\
\n\
Portions related to dirent Copyright (c) 1989, 1993 The Regents of the University of California.  All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 3. All advertising materials mentioning features or use of this software must display the following acknowledgement:  This product includes software developed by the University of California, Berkeley and its contributors. 4. Neither the name of the University nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION).  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to OpenColorIO Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al. All Rights Reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of Sony Pictures Imageworks nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to Pystring is part of OpenColorIO Copyright (c) 2008-2010, Sony Pictures Imageworks Inc All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of the organization Sony Pictures Imageworks nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to yaml-cpp that is part of OpenColorIO Copyright (c) 2008 Jesse Beder.  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to PTEX software that is part of OpenColorIO 2009 Disney Enterprises, Inc. All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  The names \"Disney\", \"Walt Disney Pictures\", \"Walt Disney Animation Studios\" or the names of its contributors may NOT be used to endorse or promote products derived from this software without specific prior written permission from Walt Disney Pictures. Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED. IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.\n\
\n\
Portions related to Little CMS that is part of OpenColorIO Copyright (c) 1998-2010 Marti Maria Saguer.  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to argparse that is part of OpenColorIO Copyright 2008 Larry Gritz and the other authors and contributors. All Rights Reserved. Based on BSD-licensed software Copyright 2004 NVIDIA Corp.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. * Neither the name of the software's owners nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to complex_ior and thin_film shader Copyright (c) 2016 Chaos Software. Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to complex_ior shader Copyright (c) 2014, Ole Gulbrandsen All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of the <organization> nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to thin_film shader GameDev.net Open License TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION \n\
\n\
1. Definitions. \"Article\" shall refer to any body of text written by Author which describes and documents the use and/or operation of Source. It specifically does not refer to any accompanying Source either embedded within the body of text or attached to the article as a file. \"Author\" means the individual or entity that offers the Work under the terms of this License. \"License\" shall mean the terms and conditions for use, reproduction, and distribution as defined by Sections 1 through 9 of this document. \"Licensor\" shall mean the copyright owner or entity authorized by the copyright owner that is granting the License. \"You\" (or \"Your\") shall mean an individual or entity exercising permissions granted by this License. \"Source\" shall include all software text source code and configuration files used to create executable software \"Object\" shall mean any Source which has been converted into a machine executable software \"Work\" consists of both the Article and Source \"Publisher\" refers to GameDev.net LLC This agreement is between You and Author, the owner and creator of the Work located at Gamedev.net. \n\
\n\
2. Fair Dealing Rights. Nothing in this License is intended to reduce, limit, or restrict any uses free from copyright or rights arising from limitations or exceptions that are provided for in connection with the copyright protection under copyright law or other applicable laws. \n\
\n\
3. Grant of Copyright License. Subject to the terms and conditions of this License, the Author hereby grants to You a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable copyright license to the Work under the following stated terms: You may not reproduce the Article on any other website outside of Gamedev.net without express written permission from the Author. You may use, copy, link, modify and distribute under Your own terms, binary Object code versions based on the Work in your own software. You may reproduce, prepare derivative Works of, publicly display, publicly perform, sublicense, and distribute the Source and such derivative Source in Source form only as part of a larger software distribution and provided that attribution to the original Author is granted. The origin of this Work must not be misrepresented; you must not claim that you wrote the original Source. If you use this Source in a product, an acknowledgment of the Author name would be appreciated but is not required. \n\
\n\
4. Restrictions. The license granted in Section 3 above is expressly made subject to and limited by the following restrictions: Altered Source versions must be plainly marked as such, and must not be misrepresented as being the original software. This License must be visibly linked to from any online distribution of the Article by URI and using the descriptive text \"Licensed under the GameDev.net Open License\". Neither the name of the Author of this Work, nor any of their trademarks or service marks, may be used to endorse or promote products derived from this Work without express prior permission of the Author. Except as expressly stated herein, nothing in this License grants any license to Author's trademarks, copyrights, patents, trade secrets or any other intellectual property. No license is granted to the trademarks of Author even if such marks are included in the Work. Nothing in this License shall be interpreted to prohibit Author from licensing under terms different from this License any Work that Author otherwise would have a right to license. \n\
\n\
5. Grant of Patent License. Subject to the terms and conditions of this License, each Contributor hereby grants to You a perpetual, worldwide, non-exclusive, no-charge, royalty-free, irrevocable (except as stated in this section) patent license to make, have made, use, offer to sell, sell, import, and otherwise transfer the Work, where such license applies only to those patent claims licensable by such Contributor that are necessarily infringed by their Contribution(s) alone or by combination of their Contribution(s) with the Work to which such Contribution(s) was submitted. If You institute patent litigation against any entity (including a cross-claim or counterclaim in a lawsuit) alleging that the Work or Source incorporated within the Work constitutes direct or contributory patent infringement, then any patent licenses granted to You under this License for that Work shall terminate as of the date such litigation is filed. \n\
\n\
6. Limitation of Liability. In no event and under no legal theory, whether in tort (including negligence), contract, or otherwise, unless required by applicable law (such as deliberate and grossly negligent acts) or agreed to in writing, shall any Author or Publisher be liable to You for damages, including any direct, indirect, special, incidental, or consequential damages of any character arising as a result of this License or out of the use or inability to use the Work (including but not limited to damages for loss of goodwill, work stoppage, computer failure or malfunction, or any and all other commercial damages or losses), even if such Author has been advised of the possibility of such damages. \n\
\n\
7. DISCLAIMER OF WARRANTY. THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. \n\
\n\
8. Publisher. The parties hereby confirm that the Publisher shall not, under any circumstances, be responsible for and shall not have any liability in respect of the subject matter of this License. The Publisher makes no warranty whatsoever in connection with the Work and shall not be liable to You or any party on any legal theory for any damages whatsoever, including without limitation any general, special, incidental or consequential damages arising in connection to this license. The Publisher reserves the right to cease making the Work available to You at any time without notice. \n\
\n\
9. Termination. This License and the rights granted hereunder will terminate automatically upon any breach by You of the terms of this License. Individuals or entities who have received Deriviative Works from You under this License, however, will not have their licenses terminated provided such individuals or entities remain in full compliance with those licenses. Sections 1, 2, 6, 7, 8 and 9 will survive any termination of this License. Subject to the above terms and conditions, the license granted here is perpetual (for the duration of the applicable copyright in the Work). Notwithstanding the above, Licensor reserves the right to release the Work under different license terms or to stop distributing the Work at any time; provided, however that any such election will not serve to withdraw this License (or any other license that has been, or is required to be, granted under the terms of this License), and this License will continue in full force and effect unless terminated as stated above."


      layout.AddStaticText(s, 500, 8000);

      layout.EndGroup();

   layout.EndRow();

   InspectObj(prop, "", "About SItoA", siModal, false);
   
   DeleteObj(prop);
}

function LegalNotice()
{
   var prop = ActiveSceneRoot.AddCustomProperty("Legal Notice", false) ;
                         
   var layout = prop.PPGLayout;

var s = "(c) 2017 Autodesk, Inc.  All rights reserved.\n\
\n\
All use of this Software is subject to the terms and conditions of the software license agreement accepted upon installation of this Software and/or packaged with the Software.\n\
\n\
Third-Party Software Credits and Attributions\n\
\n\
Portions relate to The OpenGL Extension Wrangler Library (GLEW) v. 1.10.0:\n\
\n\
Copyright (C) 2008-2016, Nigel Stewart <nigels[]users sourceforge net>.  Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>.  Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>.  Copyright (C) 2002, Lev Povalahev.  All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. * The name of the author may be sed to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Certain portions Copyright (c) Autodesk, Inc. 2016. All rights reserved.\n\
Portions related to OpenImageIO 1.7.7 Copyright 2008-2015 Larry Gritz et al. All Rights Reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  * Neither the name of the software's owners nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to IlmBase 2.2 Copyright (c) 2002-2011, Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd. All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  Neither the name of Industrial Light & Magic nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to libjpeg (C) 1991-1998, Thomas G. Lane. All Rights Reserved except as specified below. The authors make NO WARRANTY or representation, either express or implied, with respect to this software, its quality, accuracy, merchantability, or fitness for a particular purpose. This software is provided “AS IS”, and you, its user, assume the entire risk as to its quality and accuracy. Permission is hereby granted to use, copy, modify, and distribute this software (or portions thereof) for any purpose, without fee, subject to these conditions:  (1) If any part of the source code for this software is distributed, then this README file must be included, with this copyright and no-warranty notice unaltered; and any additions, deletions, or changes to the original files must be clearly indicated in accompanying documentation. (2) If only executable code is distributed, then the accompanying documentation must state that \"this software is based in part on the work of the Independent JPEG Group\". (3) Permission for use of this software is granted only if the user accepts full responsibility for any undesirable consequences; the authors accept NO LIABILITY for damages of any kind. These conditions apply to any software derived from or based on the IJG code, not just to the unmodified library. If you use our work, you ought to acknowledge us. Permission is NOT granted for the use of any IJG author's name or company name in advertising or publicity relating to this software or products derived from it. This software may be referred to only as \"the Independent JPEG Group's software\". We specifically permit and encourage the use of this software as the basis of commercial products, provided that all warranty or liability claims are assumed by the product vendor. ansi2knr.c is included in this distribution by permission of L. Peter Deutsch, sole proprietor of its copyright holder, Aladdin Enterprises of Menlo Park, CA. ansi2knr.c is NOT covered by the above copyright and conditions, but instead by the usual distribution terms of the Free Software Foundation; principally, that you must include source code if you redistribute it. (See the file ansi2knr.c for full details). However, since ansi2knr.c is not needed as part of any program generated from the IJG code, this does not limit you more than the foregoing paragraphs do. We are required to state that \"The Graphics Interchange Format(c) is the Copyright property of CompuServe Incorporated. GIF(sm) is a Service Mark property of CompuServe Incorporated.\"\n\
\n\
Portions related to libtiff Copyright (c) 1988-1997 Sam Leffler.  Copyright (c) 1991-1997 Silicon Graphics, Inc.  Permission to use, copy, modify, distribute, and sell this software and its documentation for any purpose is hereby granted without fee, provided that (i) the above copyright notices and this permission notice appear in all copies of the software and related documentation, and (ii) the names of Sam Leffler and Silicon Graphics may not be used in any advertising or publicity relating to the software without the specific, prior written permission of Sam Leffler and Silicon Graphics.  THE SOFTWARE IS PROVIDED \"AS-IS\" AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\n\
\n\
Portions related to OpenEXR Copyright (c) 2002-2011, Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd. All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  Neither the name of Industrial Light & Magic nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to OpenSubdiv 3.1 Copyright (C) 2013 Pixar.  Licensed under the Apache License, Version 2.0 (the \"Apache License\") with the following modification; you may not use this file except in compliance with the Apache License and the following modification to it: Section 6. Trademarks. is deleted and replaced with: 6. Trademarks. This License does not grant permission to use the trade names, trademarks, service marks, or product names of the Licensor and its affiliates, except as required to comply with Section 4(c) of the License and to reproduce the content of the NOTICE file.  You may obtain a copy of the Apache License at http://www.apache.org/licenses/LICENSE-2.0.  Unless required by applicable law or agreed to in writing, software distributed under the Apache License with the above modification is distributed on an \"AS IS\" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the Apache License for the specific language governing permissions and limitations under the Apache License.  \n\
\n\
Portions related to gperftools 2.5 Copyright (C) 2005 Google Inc.  All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  * Neither the name of Google Inc. nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to OpenVDB 3.2.0 Copyright (c) 2012-2013 DreamWorks Animation LLC.  All rights reserved. OpenVDB is licensed under the Mozilla Public License v.2.0, which can be found at http://www.mozilla.org/MPL/2.0/.  A text copy of this license and the source code for OpenVDB (and modifications made by Autodesk, if any) can be found at the Autodesk website www.autodesk.com/lgplsource.\n\
\n\
Portions related to SCons Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013 The SCons Foundation.  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to Boost 1.59 Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and accompanying documentation covered by this license (the \"Software\") to use, reproduce, display, distribute, execute, and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the Software is furnished to do so, all subject to the following:  The copyright notices in the Software and this entire statement, including the above license grant, this restriction and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all derivative works of the Software, unless such copies or derivative works are solely in the form of machine-executable object code generated by a source language processor.  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to pystring 1.13 Copyright (c) 2008-2010, Sony Pictures Imageworks Inc.  All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  Neither the name of the organization Sony Pictures Imageworks nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to dirent Copyright (c) 1999-2003 Apple Computer, Inc.  All Rights Reserved.  This file contains Original Code and/or Modifications of Original Code as defined in and that are subject to the Apple Public Source License Version 2.0 (the 'License'). You may not use this file except in compliance with the License. Please obtain a copy of the License at http://www.opensource.apple.com/apsl/ and read it before using this file.  The Original Code and all software distributed under the License are distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.  Please see the License for the specific language governing rights and limitations under the License.\n\
\n\
Portions related to dirent Copyright (c) 1989, 1993 The Regents of the University of California.  All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. 3. All advertising materials mentioning features or use of this software must display the following acknowledgement:  This product includes software developed by the University of California, Berkeley and its contributors. 4. Neither the name of the University nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.  THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION).  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to OpenColorIO 1.0.9 Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al. All Rights Reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of Sony Pictures Imageworks nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to Pystring is part of OpenColorIO 1.0.9 Copyright (c) 2008-2010, Sony Pictures Imageworks Inc All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. Neither the name of the organization Sony Pictures Imageworks nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to yaml-cpp that is part of OpenColorIO 1.0.9 Copyright (c) 2008 Jesse Beder.  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to PTEX software that is part of OpenColorIO 1.0.9 2009 Disney Enterprises, Inc. All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  The names “Disney”, “Walt Disney Pictures”, “Walt Disney Animation Studios” or the names of its contributors may NOT be used to endorse or promote products derived from this software without specific prior written permission from Walt Disney Pictures. Disclaimer: THIS SOFTWARE IS PROVIDED BY WALT DISNEY PICTURES AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT AND TITLE ARE DISCLAIMED. IN NO EVENT SHALL WALT DISNEY PICTURES, THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.\n\
\n\
Portions related to Little CMS that is part of OpenColorIO 1.0.9 Copyright (c) 1998-2010 Marti Maria Saguer.  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to argparse that is part of OpenColorIO 1.0.9 Copyright 2008 Larry Gritz and the other authors and contributors. All Rights Reserved. Based on BSD-licensed software Copyright 2004 NVIDIA Corp.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. * Neither the name of the software’s owners nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to Python v. 2.5, 2.6, 2.7 Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006 Python Software Foundation; All Rights Reserved.\n\
\n\
Portions related to JsonCpp 1.8.0 Copyright (c) 2007-2010 Baptiste Lepilleur.  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to psutil 5.1.1 Copyright (c) 2009, Jay Loden, Dave Daeschler, Giampaolo Rodola'.  All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer. * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. * Neither the name of the psutil authors nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n\
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions Copyright (C) 2005-2015 Intel Corporation. Intel Threading Building Blocks v. 4.4 is distributed with this Autodesk software as a separate work. Intel Threading Building Blocks is licensed under the GNU General Public License v.2 with the Runtime Exception, which can be found at http://www.threadingbuildingblocks.org/. A text copy of this license and the source code for Intel Threading Building Blocks v. 4.4 (and modifications made by Autodesk, if any) can be found at the Autodesk website www.autodesk.com/lgplsource.\n\
\n\
Portions Copyright (c) 2007-2014 iMatix Corporation and Contributors. This Autodesk software contains ZeroMQ 3.2.2.  ZeroMQ is licensed under the GNU Lesser General Public License v.2.1, which can be found at http://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt.  A text copy of this license and the source code for ZeroMQ v.3.2.2 can be found at the Autodesk website www.autodesk.com/lgplsource or by sending a written request to:\n\
\n\
Autodesk, Inc.\n\
Attention:  General Counsel\n\
Legal Department\n\
111 McInnis Parkway\n\
San Rafael, CA 94903\n\
\n\
Your written request must:\n\
\n\
1. Contain a self-addressed CD/DVD mailer (or envelope sufficiently large to hold a DVD) with postage sufficient to cover the amount of the current U.S. Post Office First Class postage rate for CD/DVD mailers (or the envelope you have chosen) weighing  5 ounces from San Rafael, California USA to your indicated address; and\n\
\n\
\n\
2. Identify:\n\
   a. This Autodesk software name and release number;\n\
   b. That you are requesting the source code for ZeroMQ v. 3.2.2; and\n\
   c. The above URL (www.autodesk.com/lgplsource)\n\
\n\
so that Autodesk may properly respond to your request.  The offer to receive this ZeroMQ source code via the above URL (www.autodesk.com/lgplsource) or by written request to Autodesk is valid for a period of three (3) years from the date you purchased your license to this Autodesk software.\n\
\n\
You may modify, debug and relink ZeroMQ to this Autodesk software as provided under the terms of the GNU Lesser General Public License v.2.1.\n\
\n\
Portions Copyright (C) 1992-2006 Trolltech ASA. All rights reserved.  This Autodesk software contains Qt v.4.2.  Qt is licensed under the GNU Lesser General Public License v.2.1, which can be found at http://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt.  A text copy of this license is included on the media provided by Autodesk or with the download of this Autodesk software.  A text copy of this license and the source code for Qt v.4.2 can be found at the Autodesk website www.autodesk.com/lgplsource or by sending a written request to:\n\
\n\
Autodesk, Inc.\n\
Attention:  General Counsel\n\
Legal Department\n\
111 McInnis Parkway\n\
San Rafael, CA 94903\n\
\n\
Your written request must:\n\
\n\
1. Contain a self-addressed CD/DVD mailer (or envelope sufficiently large to hold a DVD) with postage sufficient to cover the amount of the current U.S. Post Office First Class postage rate for CD/DVD mailers (or the envelope you have chosen) weighing  5 ounces from San Rafael, California USA to your indicated address; and\n\
\n\
\n\
2. Identify:\n\
   a. This Autodesk software name and release number;\n\
   b. That you are requesting the source code for Qt v. 4.2; and\n\
   c. The above URL (www.autodesk.com/lgplsource)\n\
\n\
so that Autodesk may properly respond to your request.  The offer to receive this Qt source code via the above URL (www.autodesk.com/lgplsource) or by written request to Autodesk is valid for a period of three (3) years from the date you purchased your license to this Autodesk software.\n\
\n\
You may modify, debug and relink Qt to this Autodesk software as provided under the terms of the GNU Lesser General Public License v.2.1.\n\
\n\
\n\
Autodesk Color Management  \n\
\n\
Portions related to GLEE v. 5.4.0 Copyright (c) 2009 Ben Woodhouse.  All rights reserved. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer as the first lines of this file unmodified. 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution. THIS SOFTWARE IS PROVIDED BY BEN WOODHOUSE “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL BEN WOODHOUSE BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to OpenColorIO v 1.0.9 Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.  All Rights Reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  * Neither the name of Sony Pictures Imageworks nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n\
\n\
Portions related to SampleICC v 1.6.6 Copyright (c) 2003-2010 The International Color Consortium. All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  3. In the absence of prior written permission, the names \"ICC\" and \"The International Color Consortium\" must not be used to imply that the ICC organization endorses or promotes products derived from this software.  THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE INTERNATIONAL COLOR CONSORTIUM OR ITS CONTRIBUTING MEMBERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  This software consists of voluntary contributions made by many individuals on behalf of the The International Color Consortium.  Membership in the ICC is encouraged when this software is used for commercial purposes.  For more information on The International Color Consortium, please see <http://www.color.org/>.\n\
\n\
Portions related to SampleICC v 1.6.6 Copyright 2001-2004 Unicode, Inc.  \n\
\n\
Disclaimer\n\
This source code is provided as is by Unicode, Inc. No claims are made as to fitness for any particular purpose. No warranties of any kind are expressed or implied. The recipient agrees to determine applicability of information provided. If this file has been purchased on magnetic or optical media from Unicode, Inc., the sole remedy for any claim will be exchange of defective media within 90 days of receipt.\n\
\n\
Limitations on Rights to Redistribute This Code\n\
Unicode, Inc. hereby grants the right to freely use the information supplied in this file in the creation of products supporting the Unicode Standard, and to make copies of this file in any form for internal or external distribution as long as this notice remains attached.\n\
\n\
Portions related to SampleICC v 1.6.6 Copyright (c) 1994 SunSoft, Inc.  Rights Reserved.  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify,  merge, publish distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES \n\
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL SUNSOFT, INC. OR ITS PARENT COMPANY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  Except as contained in this notice, the name of SunSoft, Inc. shall not be used in advertising or otherwise to promote the sale, use or other dealings in this Software without written authorization from SunSoft Inc.   \n\
\n\
Portions related to eXpat v 1.2 Copyright (c) 1998, 1999 Thai Open Source Software Center Ltd.  Copyright (c) 1998, 1999, 2000 Thai Open Source Software Center Ltd.  Copyright 2000, Clark Cooper.  Copyright (c) 1998, 1999, 2000 Thai Open Source Software Center Ltd and Clark Cooper .  Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006 Expat maintainers. Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the \"Software\"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.  THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to Boost Version 1.47 Copyright (c) - August 17th, 2003.  Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and accompanying documentation covered by this license (the \"Software\") to use, reproduce, display, distribute, execute, and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the Software is furnished to do so, all subject to the following:  The copyright notices in the Software and this entire statement, including the above license grant, this restriction and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all derivative works of the Software, unless such copies or derivative works are solely in the form of machine-executable object code generated by a source language processor. THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n\
\n\
Portions related to TurbulenceFD API Copyright (c) 2015 Jascha Wetzel. All rights reserved.  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
   
   layout.AddStaticText(s, 500, 10000);

   InspectObj(prop, "", "Legal Notice", siModal, false);
   
   DeleteObj(prop);
}


function Licensing()
{
   var propCollection = ActiveSceneRoot.Properties;

   for (var i = 0; i < propCollection.Count; i++)
   {
      var prop = propCollection(i);
      if (prop.Name == "Arnold_License_Information")
      {
         InspectObj(prop);
         return;
      }
   }

   var showMac = SITOA_ShowMac();
   if (showMac == "")
   {
      LogMessage("[sitoa] Aborting the licensing property", siErrorMsg);
      return;
   }

   var p, item;
   var prop = ActiveSceneRoot.AddCustomProperty("Arnold License Information", false);
 
   var layout = prop.PPGLayout;

   var parts = showMac.split("separator");
   var macPart     = parts[0]; // rlmutil rlmhostid -q ether
   var licenseCheckPart = parts[1]; // kick -licensecheck
   var debugPart   = parts[2]; // rlmutil debug arnold
   
   var rlmhostid = macPart;
   
   var mac = "";
   // HtoA #724, skip hostid's likely to belong to virtual adapters or machines
   var rlmhostidSplit = rlmhostid.split(" ");
   for (var i = 0; i < rlmhostidSplit.length; i++)
   {
      var id = rlmhostidSplit[i];
      var ss = id.substring(0, 4);
      if (ss == "00ff" || ss == "00FF") // skip
         continue;
      var c2 = id.substring(1, 2); // second char
      if (c2 == "2" || c2 == "6" || c2 == "a" || c2 == "A" || c2 == "e" || c2 == "E")  // skip
         continue;

      mac = id; // first good mac
      break;
   }

   if (mac == "")
      LogMessage("[sitoa] No valid host id detected", siWarningMsg);

   layout.AddGroup(" Host ID (MAC Address) ");

   p = prop.AddParameter3("macs", siString, mac); 
   item = layout.AddString("macs", "Host ID (MAC Address)", false);
   item.SetAttribute(siUINoLabel, true);

   layout.EndGroup();
   
   var details = "kick -licensecheck:\r\n";
   details+= "========================\r\n";
   licenseCheckSplit = licenseCheckPart.split(";")
   for (var i = 0; i < licenseCheckSplit.length; i++)
   {
      line = licenseCheckSplit[i];
      details+= line;
      details+= "\r\n";
   }

   details+= "\r\n";
   details+= "rlmutil rlmdebug arnold:\r\n"
   details+= "========================\r\n";
   debugSplit = debugPart.split(";")
   for (var i = 0; i < debugSplit.length; i++)
   {
      line = debugSplit[i];
      details+= line;
      details+= "\r\n";
   }

   details+= "\r\n";
   details+= "rlmutil rlmhostid -q ether:\r\n";
   details+= "========================\r\n";
   details+= rlmhostid;

   layout.AddGroup(" Details ");

   layout.AddRow();
   p = prop.AddParameter3("showDetails", siBool, 0, 0, 1, false);
   item = layout.AddItem("showDetails", "Show");
   layout.EndRow();

   p = prop.AddParameter3("info", siString, details); // set to the kick -licensecheck result
   item = layout.AddString("info", "", true, 400); 
   item.SetAttribute(siUINoLabel, true);
   p.Show(false); // hidden on startup
   
   layout.EndGroup();

   layout.AddRow();
   item = layout.AddButton("save",    "Save Information... ");
   item = layout.AddButton("install", "Install License...  ");
   item = layout.AddButton("ok",      "         Ok         ");
   layout.EndRow();

   layout.Logic = showDetails_OnChanged.toString() + save_OnClicked.toString() + install_OnClicked.toString() + ok_OnClicked.toString();
   layout.Language = "JScript";

   layout.SetViewSize(500, 400);

   InspectObj(prop, "", "Arnold License Information", siRecycle, false);
}


function showDetails_OnChanged()
{
   var showDetails = PPG.showDetails.value;
   var layout = PPG.Inspected(0).PPGLayout;
   PPG.info.Show(showDetails);
   PPG.Refresh();
}


function save_OnClicked()
{
   var info = PPG.info.Value;
   var tk = new ActiveXObject("XSI.UIToolKit");
   var fb = tk.FileBrowser ;
   fb.DialogTitle = "Select a file" ;
   var filter = "Text file (*.txt)|*.txt";
   filter += "|All Files (*.*)|*.*||";
   fb.Filter = filter;
   fb.ShowSave();
   if (fb.FilePathName == "")
   {
      LogMessage("[sitoa] Please select a valid text file", siWarningMsg);
      return;   
   }

   var fso = new ActiveXObject("Scripting.FileSystemObject");
   var f = fso.OpenTextFile(fb.FilePathName, 2, true, 0);
   f.WriteLine(info);
   f.Close();
   var msg = "Information saved to " + fb.FilePathName;
   XSIUIToolkit.Msgbox(msg, siMsgOkOnly | siMsgInformation, "SItoA");
}


function install_OnClicked()
{
   var tk = new ActiveXObject("XSI.UIToolKit");
   var fb = tk.FileBrowser ;
   fb.DialogTitle = "Select a file" ;
   var filter = "Text file (*.lic)|*.lic";
   filter += "|All Files (*.*)|*.*||";
   fb.Filter = filter;
   fb.ShowOpen(); 
   var license = fb.FilePathName;
   if (license == "")
   {
      XSIUIToolkit.Msgbox("Please select a valid license file", siMsgOkOnly | siMsgExclamation, "SItoA");
      return;
   }

   var renderPlugin = Application.plugins("Arnold Render");
   var path = renderPlugin.OriginPath;
   var fso = new ActiveXObject("Scripting.FileSystemObject");
   try
   {
      fso.CopyFile(license, path, 1); // 1 for overwrite
      var msg = "License file " + license + " copied to " + path;
      XSIUIToolkit.Msgbox(msg, siMsgOkOnly | siMsgInformation, "SItoA");
   }
   catch(e)
   {
      LogMessage("[sitoa] Error copying license file " + license + " to " + path + ". " + e.description, siErrorMsg);
   }
}


function ok_OnClicked()
{
   DeleteObj(PPG.Inspected(0));
}