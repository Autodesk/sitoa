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
   h.SetPluginInfo(in_reg, "Arnold Scene Preferences");

   in_reg.RegisterEvent("SITOA_OnEndNewScene", siOnEndNewScene);
   in_reg.RegisterEvent("SITOA_OnStartup", siOnStartup);   
  
   in_reg.RegisterCommand("Arnold Render Channels Creator", "SITOA_CreateRenderChannels");  
   
   return true;
}

function SITOA_OnEndNewScene_OnEvent(in_ctxt)
{
   if (Application.Interactive)
      SetSceneForArnold();
 
   return false;
}


function SITOA_OnStartup_OnEvent(in_ctxt)
{
   if (Application.Interactive)
      SetSceneForArnold();
   
   var xsiPrefs = Application.Preferences;
   var renderPrefs = xsiPrefs.Categories("Arnold Render");
   // Custom preferences were not installed
   if (renderPrefs == null) 
   {
     var sceneRoot = Dictionary.GetObject("Scene_Root", false);
     var customProperty = sceneRoot.AddProperty("ArnoldRenderPreferences", false, "ArnoldRenderPreferences");
     InstallCustomPreferences(customProperty, "Arnold Render");  
   } 
   else
      RefreshCustomPreferences();

   return false;
}


function SetSceneForArnold()
{
   if (Application.Preferences.GetPreferenceValue("rendering.renderer") == "Arnold Render")
   {
      logMessage("[sitoa] Detected Arnold as default renderer. Establishing basic scene settings.");

      try
      {
         // Change default light shader
         var light = Dictionary.GetObject("light");

         // Modify the default scene light
         var currentShader = light.Shaders(0);
         if (currentShader.Name == "soft_light")
         {
            var shader = CreateShaderFromProgID("ArnoldLightShaders.arnold_distant_light.1.0", light.ActivePrimitive, null);
            SIConnectShaderToCnxPoint(shader, light.ActivePrimitive + ".LightShader", true);

            DisconnectAndDeleteOrUnnestShaders("light.light.soft_light", "light.light");
            SetValue(shader + ".intensity", 4.0, "");
            SetValue(shader + ".angle", 0.53, "");
         }
      
         // Modify Scene_Material to use standard_surface
         var sceneMaterial = Dictionary.GetObject("Sources.Materials.DefaultLib.Scene_Material");
         var currentShader = sceneMaterial.Surface.Source.Parent;
         if (currentShader.Name == "Phong")
         {
            var shader = CreateShaderFromProgID("Arnold.standard_surface.1.0", sceneMaterial, null);
            var closure = CreateShaderFromProgID("Arnold.closure.1.0", sceneMaterial, null);
            SIConnectShaderToCnxPoint(shader, closure.closure, false);
            SIConnectShaderToCnxPoint(closure, sceneMaterial.Surface, false);
            DisconnectAndDeleteOrUnnestShaders(sceneMaterial + ".Phong", sceneMaterial);
         }

         // Use pass render options as the view render options are not supported for the moment
         SetValue("Views.ViewA.RenderRegion.UsePassOptions,Views.ViewB.RenderRegion.UsePassOptions,"+
                  "Views.ViewC.RenderRegion.UsePassOptions,Views.ViewD.RenderRegion.UsePassOptions",
                  Array(true, true, true, true), null);

         // Adding additional Arnold AOV core layers as custom channels
         CreateRenderChannels();
         
         // Setting Arnold as the renderer 
         SetValue("Passes.RenderOptions.Renderer", "Arnold Render", null);
      }
      catch (exception)
      {
         logMessage("[sitoa] Error establishing basic scene settings for Arnold Renderer.", siErrorMsg); 
      }
   }
}


function ArnoldRenderChannelsCreator_Execute(in_arg)
{
   CreateRenderChannels();
}


function CreateRenderChannels()
{
   // Getting list of Available Channels for the Scene
   // Before creating them, we will check below if each Arnold Render Channel already exists
   var props = ActiveProject.ActiveScene.PassContainer.Properties;
   var channels = new ActiveXObject("XSI.Collection");
   for (var i=0; i<props.Count; i++)
   {
      if(props(i).FullName=="Passes.RenderOptions")
         var channels = props(i).RenderChannels;
   }
   
   var aov_array = [];

   aov_array.push({ name: "A",                     type: siRenderChannelGrayscaleType });
   aov_array.push({ name: "AA_inv_density",        type: siRenderChannelGrayscaleType });
   aov_array.push({ name: "albedo",                type: siRenderChannelColorType });
   aov_array.push({ name: "background",            type: siRenderChannelColorType });
   aov_array.push({ name: "coat",                  type: siRenderChannelColorType });
   aov_array.push({ name: "coat_albedo",           type: siRenderChannelColorType });
   aov_array.push({ name: "coat_direct",           type: siRenderChannelColorType });
   aov_array.push({ name: "coat_indirect",         type: siRenderChannelColorType });
   aov_array.push({ name: "cputime",               type: siRenderChannelGrayscaleType });
   aov_array.push({ name: "crypto_asset",          type: siRenderChannelColorType });  // cryptomatte
   aov_array.push({ name: "crypto_material",       type: siRenderChannelColorType });  // cryptomatte
   aov_array.push({ name: "crypto_object",         type: siRenderChannelColorType });  // cryptomatte
   aov_array.push({ name: "diffuse",               type: siRenderChannelColorType });
   aov_array.push({ name: "diffuse_albedo",        type: siRenderChannelColorType });
   aov_array.push({ name: "diffuse_direct",        type: siRenderChannelColorType });
   aov_array.push({ name: "diffuse_indirect",      type: siRenderChannelColorType });
   aov_array.push({ name: "direct",                type: siRenderChannelColorType });
   aov_array.push({ name: "emission",              type: siRenderChannelColorType });
   aov_array.push({ name: "highlight",             type: siRenderChannelColorType });  // toon shader
   aov_array.push({ name: "ID",                    type: siRenderChannelLabelType });
   aov_array.push({ name: "indirect",              type: siRenderChannelColorType });
   aov_array.push({ name: "motionvector",          type: siRenderChannelColorType });
   aov_array.push({ name: "N",                     type: siRenderChannelVectorType });
   aov_array.push({ name: "opacity",               type: siRenderChannelColorType });
   aov_array.push({ name: "P",                     type: siRenderChannelVectorType });
   aov_array.push({ name: "Pref",                  type: siRenderChannelVectorType });
   aov_array.push({ name: "raycount",              type: siRenderChannelGrayscaleType });
   aov_array.push({ name: "rim_light",             type: siRenderChannelColorType });  // toon shader
   aov_array.push({ name: "shadow",                type: siRenderChannelColorType });  // shadow_matte shader
   aov_array.push({ name: "shadow_diff",           type: siRenderChannelColorType });  // shadow_matte shader
   aov_array.push({ name: "shadow_mask",           type: siRenderChannelColorType });  // shadow_matte shader
   aov_array.push({ name: "shadow_matte",          type: siRenderChannelColorType });
   aov_array.push({ name: "sheen",                 type: siRenderChannelColorType });
   aov_array.push({ name: "sheen_direct",          type: siRenderChannelColorType });
   aov_array.push({ name: "sheen_indirect",        type: siRenderChannelColorType });
   aov_array.push({ name: "sheen_albedo",          type: siRenderChannelColorType });
   aov_array.push({ name: "specular",              type: siRenderChannelColorType });
   aov_array.push({ name: "specular_albedo",       type: siRenderChannelColorType });
   aov_array.push({ name: "specular_direct",       type: siRenderChannelColorType });
   aov_array.push({ name: "specular_indirect",     type: siRenderChannelColorType });
   aov_array.push({ name: "sss",                   type: siRenderChannelColorType });
   aov_array.push({ name: "sss_albedo",            type: siRenderChannelColorType });
   aov_array.push({ name: "sss_direct",            type: siRenderChannelColorType });
   aov_array.push({ name: "sss_indirect",          type: siRenderChannelColorType });
   aov_array.push({ name: "transmission",          type: siRenderChannelColorType });
   aov_array.push({ name: "transmission_albedo",   type: siRenderChannelColorType });
   aov_array.push({ name: "transmission_direct",   type: siRenderChannelColorType });
   aov_array.push({ name: "transmission_indirect", type: siRenderChannelColorType });
   aov_array.push({ name: "volume",                type: siRenderChannelColorType });
   aov_array.push({ name: "volume_albedo",         type: siRenderChannelColorType });
   aov_array.push({ name: "volume_direct",         type: siRenderChannelColorType });
   aov_array.push({ name: "volume_indirect",       type: siRenderChannelColorType });
   aov_array.push({ name: "volume_opacity",        type: siRenderChannelColorType });
   aov_array.push({ name: "volume_Z",              type: siRenderChannelGrayscaleType });
   aov_array.push({ name: "Z",                     type: siRenderChannelGrayscaleType });

   var aov_name, aov_type;
	for (var i = 0; i < aov_array.length; i++) 
	{
		aov_name = "Arnold_" + aov_array[i].name;
      aov_type = aov_array[i].type;
      if (channels(aov_name) == null)    
         CreateRenderChannel(aov_name, aov_type, null);
	}
}

////////////////////////////////
// doc: list of Arnold 5.0.0.2 AOVs:
////////////////////////////////
/*
N VECTOR
Z FLOAT
direct RGB
emission RGB
indirect RGB
Pref VECTOR
albedo RGB
background RGB
diffuse_direct RGB
sss_albedo RGB
specular_albedo RGB
diffuse RGB
cputime FLOAT
diffuse_indirect RGB
sss_indirect RGB
diffuse_albedo RGB
shadow_matte RGBA
specular RGB
specular_direct RGB
specular_indirect RGB
transmission RGB
transmission_direct RGB
transmission_indirect RGB
AA_offset VECTOR2
transmission_albedo RGB
sss RGB
sss_direct RGB
volume RGB
volume_direct RGB
volume_indirect RGB
volume_albedo RGB
A FLOAT
ZBack FLOAT
opacity RGB
volume_opacity RGB
raycount FLOAT
ID UINT
shader NODE
object NODE
P VECTOR
nb_aovs = 42
*/
