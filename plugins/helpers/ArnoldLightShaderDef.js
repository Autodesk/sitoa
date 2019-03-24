/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

function XSILoadPlugin( in_reg )
{
   if (Application.plugins("Arnold Tools") == null)
      LoadPlugin(XSIUtils.BuildPath(in_reg.OriginPath, "ArnoldTools.js"));
   
   var h = SItoAToolHelper();
   h.SetPluginInfo(in_reg, "ArnoldLightShaders");

   // lights
   in_reg.RegisterShader("arnold_cylinder_light",    1, 0);
   in_reg.RegisterShader("arnold_disk_light",        1, 0);
   in_reg.RegisterShader("arnold_distant_light",     1, 0);
   in_reg.RegisterShader("arnold_mesh_light",        1, 0);
   in_reg.RegisterShader("arnold_photometric_light", 1, 0);
   in_reg.RegisterShader("arnold_point_light",       1, 0);
   in_reg.RegisterShader("arnold_quad_light",        1, 0);
   in_reg.RegisterShader("arnold_skydome_light",     1, 0);
   in_reg.RegisterShader("arnold_spot_light",        1, 0);

   // light filters
   in_reg.RegisterShader("barndoor",               1, 0);
   in_reg.RegisterShader("gobo",                   1, 0);
   in_reg.RegisterShader("light_blocker",          1, 0);
   in_reg.RegisterShader("light_decay",            1, 0);
   in_reg.RegisterShader("passthrough_filter", 1, 0);

   return true;
}

function XSIUnloadPlugin(in_reg)
{
   return true;
}


// parameters common to all the lights

 var lightLabelMinPixels = 100;
 var lightLabelPcg = 25;

function LightCommonParams(in_params, in_normalize, in_exposeColor, in_has_visibility, in_visible)
{
   var h = SItoAShaderDefHelpers(); // helper object
   h.AddColor3 (in_params, "color",          1, 1, 1,                 true, in_exposeColor, true, h.UiLightColorGuid);
   h.AddScalar (in_params, "intensity",      1, 0, 1000000, 0, 1,     true, false, true);
   h.AddScalar (in_params, "exposure",       0, null, null, -5, 5,    true, false, true);
   if (in_normalize)
      h.AddBoolean(in_params, "normalize",   true,                    true, false, true);
   h.AddInteger(in_params, "samples",        2, 0, 100, 0, 10,        true, false, true)
  
   h.AddBoolean(in_params, "cast_shadows",   true,                    true, false, true);
   h.AddBoolean(in_params, "cast_volumetric_shadows", true,           true, false, true);
   h.AddColor3 (in_params, "shadow_color",   0, 0, 0,                 true, false, true);
   h.AddScalar (in_params, "shadow_density", 1, 0, 1000000, 0, 1,     true, false, true);
 
   if (in_has_visibility)
   {
      var default_visibility = 0;
      if (in_visible)
         default_visibility = 1;
      h.AddScalar (in_params, "camera",       default_visibility, 0, 1, 0, 1, true, false, true);
      h.AddScalar (in_params, "transmission", default_visibility, 0, 1, 0, 1, true, false, true);
   } 
   h.AddScalar (in_params, "diffuse",        1, 0, 1, 0, 1,           true, false, true);
   h.AddScalar (in_params, "specular",       1, 0, 1, 0, 1,           true, false, true);
   h.AddScalar (in_params, "sss",            1, 0, 1, 0, 1,           true, false, true);
   h.AddScalar (in_params, "indirect",       1, 0, 1, 0, 1,           true, false, true);
   h.AddScalar (in_params, "volume",         1, 0, 1, 0, 1,           true, false, true);
   h.AddInteger(in_params, "max_bounces",    999, 0, 1000000, 0, 999, true, false, true)

   h.AddInteger(in_params, "volume_samples",     2, 1, 100, 1, 10,    true, false, true)

   h.AddString(in_params, "aov", "");

   for (var i=1; i<6; i++)
      h.AddShader(in_params, "filter"+i);
}

// common layout sections

function LightCommonLayoutColor(in_layout)
{
   in_layout.AddGroup("Color");
      item = in_layout.AddItem("color",     "Color");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("intensity", "Intensity");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("exposure",  "Exposure (f-stop)");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
   in_layout.EndGroup();
}

function LightCommonLayoutContribution(in_layout, in_has_visibility)
{
   in_layout.AddGroup("Contribution");
      if (in_has_visibility)
      {
         item = in_layout.AddItem("camera", "Camera");
         SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);              
         item = in_layout.AddItem("transmission", "Transmission");
         SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);              
      }
      item = in_layout.AddItem("diffuse",     "Diffuse");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("specular",    "Specular");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("sss",         "SSS");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("indirect",    "Indirect");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("volume",      "Volume");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("max_bounces", "Max Bounces");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
   in_layout.EndGroup();
}

function LightCommonLayoutShadows(in_layout)
{
   in_layout.AddGroup("Shadows");
      item = in_layout.AddItem("cast_shadows",   "Enabled");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("shadow_color",   "Color");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("shadow_density", "Density");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
   in_layout.EndGroup();
}

function LightCommonLayoutVolume(in_layout, in_is_skydome)
{
   in_layout.AddGroup("Volume");
      item = in_layout.AddItem("cast_volumetric_shadows", "Cast Volumetric Shadows");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("volume_samples",          "Samples");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
   in_layout.EndGroup();
   in_layout.AddGroup("AOV Light Group");
      item = in_layout.AddItem("aov", "");
      item.SetAttribute(siUINoLabel, true);
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      if (in_is_skydome)
      {
         item = in_layout.AddItem("aov_indirect", "Indirect Lighting");
         SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      }

   in_layout.EndGroup();
}

function LightCommonLayoutArea(in_layout)
{
   item = in_layout.AddItem("samples",   "Samples");
   SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
   item = in_layout.AddItem("normalize", "Normalize");
   SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
}

// common rendertree layout
function LightCommonRtLayout(in_layout, in_exposeColor)
{
   if (in_exposeColor)
      in_layout.AddItem("color");
   in_layout.AddGroup("Filters");
      for (var i=1; i<6; i++)
         in_layout.AddItem("filter"+i);
   in_layout.EndGroup();
}

// common logic
function lights_logic_OnInit()
{
   if (PPG.portal != null)
      lights_logic_portal_OnChanged();
   else
   {
      lights_logic_cast_shadows_OnChanged();
      lights_logic_indirect_OnChanged();
   }

   if (PPG.aov_indirect != null)
      lights_logic_aov_indirect_OnChanged();
}

function lights_logic_cast_shadows_OnChanged()
{
   PPG.shadow_color.Enable(PPG.cast_shadows.Value);
   PPG.shadow_density.Enable(PPG.cast_shadows.Value);
   PPG.cast_volumetric_shadows.Enable(PPG.cast_shadows.Value);
}

function lights_logic_indirect_OnChanged()
{
   PPG.max_bounces.Enable(PPG.indirect.Value > 0)   
}

function lights_logic_portal_OnChanged()
{
   if (PPG.portal == null)
      return;

   // here only for the quad light
   var enable = ! PPG.portal.Value;

   PPG.color.Enable(enable);
   PPG.intensity.Enable(enable);
   PPG.exposure.Enable(enable);
   PPG.normalize.Enable(enable);
   PPG.samples.Enable(enable);
   PPG.cast_shadows.Enable(enable);
   PPG.cast_volumetric_shadows.Enable(enable);
   PPG.shadow_color.Enable(enable);
   PPG.shadow_density.Enable(enable);
   PPG.diffuse.Enable(enable);
   PPG.specular.Enable(enable);
   PPG.sss.Enable(enable);
   PPG.indirect.Enable(enable);
   PPG.volume.Enable(enable);
   PPG.max_bounces.Enable(enable);
   PPG.volume_samples.Enable(enable);
   PPG.aov.Enable(enable);
   PPG.resolution.Enable(enable);
   PPG.spread.Enable(enable);

   if (enable)
   {
      lights_logic_cast_shadows_OnChanged();
      lights_logic_indirect_OnChanged();
   }
}

function lights_logic_aov_indirect_OnChanged()
{
   if (PPG.aov_indirect == null)
      return;

   // here only for the skydome light
   PPG.aov.Enable(! PPG.aov_indirect.Value);
}

////////////////// cylinder_light //////////////////
function ArnoldLightShaders_arnold_cylinder_light_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_arnold_cylinder_light_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   LightCommonParams(params, true, false, true, false);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   arnold_cylinder_light_Layout(shaderDef.PPGLayout);
   LightCommonRtLayout(shaderDef.RenderTreeLayout, false);

   return true;
}

function arnold_cylinder_light_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Cylinder+Light");

   LightCommonLayoutColor(in_layout);
   LightCommonLayoutContribution(in_layout, true);
   in_layout.AddGroup("Area");
      LightCommonLayoutArea(in_layout);
   in_layout.EndGroup();
   LightCommonLayoutShadows(in_layout);
   LightCommonLayoutVolume(in_layout, false);

   in_layout.SetAttribute(siUILogicPrefix, "lights_logic_");
}


////////////////// disk_light //////////////////
function ArnoldLightShaders_arnold_disk_light_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_arnold_disk_light_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   LightCommonParams(params, true, false, true, false);
   h.AddScalar(params, "spread", 1, 0, 1, 0, 1, true, false, true);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   arnold_disk_light_Layout(shaderDef.PPGLayout);
   LightCommonRtLayout(shaderDef.RenderTreeLayout, false);

   return true;
}

function arnold_disk_light_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Disk+Light");
   LightCommonLayoutColor(in_layout);
   LightCommonLayoutContribution(in_layout, true);
   in_layout.AddGroup("Area");
      LightCommonLayoutArea(in_layout);
      item = in_layout.AddItem("spread", "Spread");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);  
   in_layout.EndGroup();

   LightCommonLayoutShadows(in_layout);
   LightCommonLayoutVolume(in_layout, false);

   in_layout.SetAttribute(siUILogicPrefix, "lights_logic_");
}


////////////////// distant_light //////////////////
function ArnoldLightShaders_arnold_distant_light_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_arnold_distant_light_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   LightCommonParams(params, true, false, false, false);
   h.AddScalar(params, "angle", 0, 0, 180, 0, 10, true, false, true);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   arnold_distant_light_Layout(shaderDef.PPGLayout);
   LightCommonRtLayout(shaderDef.RenderTreeLayout, false);

   return true;
}

function arnold_distant_light_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Distant+Light");
   LightCommonLayoutColor(in_layout);
   LightCommonLayoutContribution(in_layout, false);
   in_layout.AddGroup("Area");
      item = in_layout.AddItem("angle", "Angle");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);  
      LightCommonLayoutArea(in_layout);
   in_layout.EndGroup();
   LightCommonLayoutShadows(in_layout);
   LightCommonLayoutVolume(in_layout, false);

   in_layout.SetAttribute(siUILogicPrefix, "lights_logic_");
}


////////////////// mesh_light //////////////////
function ArnoldLightShaders_arnold_mesh_light_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_arnold_mesh_light_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   LightCommonParams(params, true, true, false, false);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   arnold_mesh_light_Layout(shaderDef.PPGLayout);
   LightCommonRtLayout(shaderDef.RenderTreeLayout, true);

   return true;
}

function arnold_mesh_light_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Mesh+Light");

   LightCommonLayoutColor(in_layout);
   LightCommonLayoutContribution(in_layout, false);
   in_layout.AddGroup("Area");
      LightCommonLayoutArea(in_layout);
   in_layout.EndGroup();
   LightCommonLayoutShadows(in_layout);
   LightCommonLayoutVolume(in_layout, false);

   in_layout.SetAttribute(siUILogicPrefix, "lights_logic_");
}


////////////////// photometric_light //////////////////
function ArnoldLightShaders_arnold_photometric_light_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_arnold_photometric_light_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   LightCommonParams(params, true, false, false, false);
   h.AddLightProfile(params, "filename");
   h.AddScalar(params, "radius", 0, 0, 1000000, 0, 2, true, false, true);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   arnold_photometric_light_Layout(shaderDef.PPGLayout);
   LightCommonRtLayout(shaderDef.RenderTreeLayout, false);

   return true;
}

function arnold_photometric_light_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Photometric+Light");

   in_layout.AddGroup("Photometry File");
      item = in_layout.AddItem("filename", "Filename");
      item.SetAttribute(siUINoLabel, true);
   in_layout.EndGroup();
   LightCommonLayoutColor(in_layout);
   LightCommonLayoutContribution(in_layout, false);
   in_layout.AddGroup("Area");
      LightCommonLayoutArea(in_layout);
      item = in_layout.AddItem("radius", "Radius");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);  
   in_layout.EndGroup();

   LightCommonLayoutShadows(in_layout);
   LightCommonLayoutVolume(in_layout, false);

   in_layout.SetAttribute(siUILogicPrefix, "lights_logic_");
}


////////////////// point_light //////////////////
function ArnoldLightShaders_arnold_point_light_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_arnold_point_light_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   LightCommonParams(params, true, false, true, false);
   h.AddScalar (params, "radius", 0, 0, 1000000, 0, 10, true, false, true);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   arnold_point_light_Layout(shaderDef.PPGLayout);
   LightCommonRtLayout(shaderDef.RenderTreeLayout, false);

   return true;
}

function arnold_point_light_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Point+Light");

   LightCommonLayoutColor(in_layout);
   LightCommonLayoutContribution(in_layout, true);
   in_layout.AddGroup("Area");
      item = in_layout.AddItem("radius", "Radius");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);  
      LightCommonLayoutArea(in_layout);
   in_layout.EndGroup();
   LightCommonLayoutShadows(in_layout);
   LightCommonLayoutVolume(in_layout, false);

   in_layout.SetAttribute(siUILogicPrefix, "lights_logic_");
}


////////////////// quad_light //////////////////
function ArnoldLightShaders_arnold_quad_light_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_arnold_quad_light_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   LightCommonParams(params, true, true, true, false);
   h.AddInteger(params, "resolution", 512, 1, 1000000, 1, 4096, true, false, true);
   h.AddScalar(params, "spread", 1, 0, 1, 0, 1, true, false, true);
   h.AddBoolean(params, "portal", false, true, false, true);
   h.AddScalar(params, "roundness", 0, 0, 1, 0, 1, true, false, true);
   h.AddScalar(params, "soft_edge", 0, 0, 1, 0, 1, true, false, true);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   arnold_quad_light_Layout(shaderDef.PPGLayout);
   LightCommonRtLayout(shaderDef.RenderTreeLayout, true);

   return true;
}

function arnold_quad_light_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Quad+Light");

   LightCommonLayoutColor(in_layout);
   LightCommonLayoutContribution(in_layout, true);
   in_layout.AddGroup("Area");
      item = in_layout.AddItem("portal", "Portal");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      LightCommonLayoutArea(in_layout);
      item = in_layout.AddItem("resolution", "Resolution");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      item = in_layout.AddItem("spread", "Spread");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);  
      item = in_layout.AddItem("roundness", "Roundness");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);  
      item = in_layout.AddItem("soft_edge", "Soft Edge");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);  
   in_layout.EndGroup();

   LightCommonLayoutShadows(in_layout);
   LightCommonLayoutVolume(in_layout, false);

   in_layout.SetAttribute(siUILogicPrefix, "lights_logic_");
}


////////////////// skydome_light //////////////////
function ArnoldLightShaders_arnold_skydome_light_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_arnold_skydome_light_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   LightCommonParams(params, false, true, true, true); // no "normalize" param, yes texturable color, yes camera and transmission, yes camera and transmission on by default
   h.AddInteger(params, "resolution", 1000, 1, 1000000, 1, 4096, true, false, true);
   h.AddInteger(params, "format",     1, 0, 2, 0, 2, true, false, true);
   h.AddInteger(params, "portal_mode", 1, 0, 2, 0, 2, true, false, true);
   h.AddBoolean(params, "aov_indirect", false, true, false, true);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   arnold_skydome_light_Layout(shaderDef.PPGLayout);
   LightCommonRtLayout(shaderDef.RenderTreeLayout, true);

   return true;
}

function arnold_skydome_light_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Skydome+Light");

   LightCommonLayoutColor(in_layout);
   LightCommonLayoutContribution(in_layout, true);
   in_layout.AddGroup("Area");
      item = in_layout.AddItem("samples",   "Samples");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);     
      item = in_layout.AddItem("resolution", "Resolution");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      item = in_layout.AddEnumControl("format", Array(
         "Mirrored Ball", 0,
         "Angular Map", 1,
         "Lat-Long", 2), "Format", siControlCombo);
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      item = in_layout.AddEnumControl("portal_mode", Array(
         "Off", 0,
         "Interior Only", 1,
         "Interior Exterior", 2), "Portal Mode", siControlCombo);
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
   in_layout.EndGroup();
   LightCommonLayoutShadows(in_layout);
   LightCommonLayoutVolume(in_layout, true);

   in_layout.SetAttribute(siUILogicPrefix, "lights_logic_");
}


////////////////// spot_light //////////////////
function ArnoldLightShaders_arnold_spot_light_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_arnold_spot_light_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   LightCommonParams(params, true, false, false, false);
   h.AddScalar(params, "radius",          0, 0, 1000000, 0, 10, true, false, true);
   h.AddScalar(params, "lens_radius",     0,  0, 1000000, 0, 10);
   h.AddScalar(params, "cone_angle",      65, 0, 1000000, 0, 100);
   h.AddScalar(params, "penumbra_angle",  0, 0, 1000000, 0, 100);
   h.AddScalar(params, "aspect_ratio",    1, 0, 1000000, 0, 10);
   h.AddScalar(params, "roundness",       1, 0, 1, 0, 1);

   // OUTPUT
   h.AddOutputColor4(shaderDef.OutputParamDefs);

   arnold_spot_light_Layout(shaderDef.PPGLayout);
   LightCommonRtLayout(shaderDef.RenderTreeLayout, false);

   return true;
}

function arnold_spot_light_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Spot+Light");

   LightCommonLayoutColor(in_layout);
   LightCommonLayoutContribution(in_layout, false);
   in_layout.AddGroup("Area");
      item = in_layout.AddItem("radius", "Radius");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      LightCommonLayoutArea(in_layout);
   in_layout.EndGroup();
   LightCommonLayoutShadows(in_layout);
   in_layout.AddGroup("Geometry");
      item = in_layout.AddItem("aspect_ratio", "Aspect Ratio");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      item = in_layout.AddItem("roundness", "Roundness");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      item = in_layout.AddItem("lens_radius", "Lens Radius");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      item = in_layout.AddItem("cone_angle", "Cone Angle");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
      item = in_layout.AddItem("penumbra_angle", "Penumbra Angle");
      SetLabelPixelsAndPcg(item, lightLabelMinPixels, lightLabelPcg);
   in_layout.EndGroup();
   LightCommonLayoutVolume(in_layout, false);

   in_layout.SetAttribute(siUILogicPrefix, "lights_logic_");
}


//////////////////////////////////////////////////////////////
//////////              LIGHT FILTERS               //////////
//////////////////////////////////////////////////////////////

////////// barndoor //////////
function ArnoldLightShaders_barndoor_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_barndoor_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;
   h.AddScalar (params, "barndoor_top_left",     0.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_top_right",    0.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_top_edge",     0.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_right_top",    1.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_right_bottom", 1.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_right_edge",   0.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_bottom_left",  1.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_bottom_right", 1.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_bottom_edge",  0.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_left_top",     0.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_left_bottom",  0.0, 0.0, 1000000, 0.0, 10.0, true, false, true);
   h.AddScalar (params, "barndoor_left_edge",    0.0, 0.0, 1000000, 0.0, 10.0, true, false, true);

   // OUTPUT
   h.AddOutputShader(shaderDef.OutputParamDefs);

   // Renderer definition
   h.AddArnoldRendererDef(shaderDef);
   
   barndoor_Layout(shaderDef.PPGLayout);

   return true;
}

function barndoor_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Barndoor");

   var labelMinPixels = 100;
   var labelPcg = 25;
  
   in_layout.AddGroup("Top");
      item = in_layout.AddItem("barndoor_top_left",     "Top Left");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
      item = in_layout.AddItem("barndoor_top_right",    "Top Right");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
      item = in_layout.AddItem("barndoor_top_edge",     "Top Edge");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
   in_layout.EndGroup();
   in_layout.AddGroup("Right");
      item = in_layout.AddItem("barndoor_right_top",    "Right Top");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
      item = in_layout.AddItem("barndoor_right_bottom", "Right Bottom");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
      item = in_layout.AddItem("barndoor_right_edge",   "Right Edge");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
   in_layout.EndGroup();
   in_layout.AddGroup("Bottom");
      item = in_layout.AddItem("barndoor_bottom_left",  "Bottom Left");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
      item = in_layout.AddItem("barndoor_bottom_right", "Bottom Right");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
      item = in_layout.AddItem("barndoor_bottom_edge",  "Bottom Edge");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
   in_layout.EndGroup();
   in_layout.AddGroup("Left");
      item = in_layout.AddItem("barndoor_left_top",     "Left Top");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
      item = in_layout.AddItem("barndoor_left_bottom",  "Left Bottom");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
      item = in_layout.AddItem("barndoor_left_edge",    "Left Edge");
      SetLabelPixelsAndPcg(item, labelMinPixels, labelPcg);     
   in_layout.EndGroup();
}


////////// gobo //////////
function ArnoldLightShaders_gobo_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_gobo_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;

   h.AddImage (params, "slidemap");
   h.AddScalar(params, "rotate",      0.0, 0.0, 1000000, 0.0, 360.0);
   h.AddScalar(params, "sscale",      1.0, 0.001, 1000000, 0.001, 10.0);
   h.AddScalar(params, "tscale",      1.0, 0.001, 1000000, 0.001, 10.0);
   h.AddString(params, "swrap",       "periodic");
   h.AddString(params, "twrap",       "periodic");
   h.AddScalar(params, "density",     0.0, 0.0, 1.0, 0.0, 1.0);
   h.AddString(params, "filter_mode", "blend");
   h.AddScalar(params, "offset_x",    0.0, -1000000.0, 1000000.0, -1.0, 1.0);
   h.AddScalar(params, "offset_y",    0.0, -1000000.0, 1000000.0, -1.0, 1.0);

   // OUTPUT
   h.AddOutputShader(shaderDef.OutputParamDefs);

   // Renderer definition
   h.AddArnoldRendererDef(shaderDef);
   
   gobo_Layout(shaderDef.PPGLayout);

   return true;
}

function gobo_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Gobo");

   var labelMinPixels = 100;
   var labelPcg = 25;
  
   item = in_layout.AddItem("slidemap", "");
   item.SetAttribute(siUINoLabel, true);
   item = in_layout.AddItem("rotate",   "Rotation");
   item = in_layout.AddItem("sscale",   "Repeat S");
   item = in_layout.AddItem("tscale",   "Repeat T");

   var ar = Array("Periodic", "periodic",
                  "Black",    "black",
                  "Clamp",    "clamp",
                  "Mirror",   "mirror",
                  "File",     "file");

   item = in_layout.AddEnumControl("swrap", ar, "Wrap S", siControlCombo);
   item = in_layout.AddEnumControl("twrap", ar, "Wrap T", siControlCombo);
     
   item = in_layout.AddItem("density",    "Density");

   item = in_layout.AddEnumControl("filter_mode", Array(
      "Blend",   "blend",
      "Replace", "replace",
      "Add",     "add",
      "Sub",     "sub",
      "Mix",     "mix"), "Filter Mode", siControlCombo);

   in_layout.AddGroup("Offset");
      item = in_layout.AddItem("offset_x",   "X");
      item = in_layout.AddItem("offset_y",   "Y");
   in_layout.EndGroup();
}


////////// light_blocker //////////
function ArnoldLightShaders_light_blocker_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_light_blocker_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;

   h.AddString (params, "geometry_type",  "box");
   h.AddVector3(params, "scale",       1, 1, 1, null, null, 0, 10);
   h.AddVector3(params, "rotation",    0, 0, 0, null, null, -180, 180);
   h.AddVector3(params, "translation", 0, 0, 0, null, null, -50, 50);
   h.AddScalar (params, "density",     1, 0, 1, 0, 1);
   h.AddScalar (params, "width_edge",  0, 0, 1000000, 0, 3);
   h.AddScalar (params, "height_edge", 0, 0, 1000000, 0, 3);
   h.AddScalar (params, "ramp",        0, 0, 1, 0, 1);
   h.AddString (params, "axis",        "x");
   // h.AddColor3 (params, "shader",      0, 0, 0);   

   // OUTPUT
   h.AddOutputShader(shaderDef.OutputParamDefs);

   // Renderer definition
   h.AddArnoldRendererDef(shaderDef);
   
   light_blocker_Layout(shaderDef.PPGLayout);

   return true;
}

function light_blocker_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Light+Blocker");

   in_layout.AddGroup("Type");
      item = in_layout.AddEnumControl("geometry_type", Array(
         "Box",      "box",
         "Sphere",   "sphere",
         "Cylinder", "cylinder"), "Type", siControlCombo);
      item.SetAttribute(siUINoLabel, true);
   in_layout.EndGroup();

   in_layout.AddGroup("Transformation");
      item = in_layout.AddItem("scale",       "Scale");
      item = in_layout.AddItem("rotation",    "Rotation");
      item = in_layout.AddItem("translation", "Translation");
   in_layout.EndGroup();

   in_layout.AddRow();
      item = in_layout.AddButton("SetExpression",    "Set Expression");
      item = in_layout.AddButton("RemoveExpression", "Remove Expression");
   in_layout.EndRow();

   in_layout.AddGroup("Shape");
      item = in_layout.AddItem("density",     "Density");
      item = in_layout.AddItem("width_edge",  "Width Edge");
      item = in_layout.AddItem("height_edge", "Height Edge");
   in_layout.EndGroup();

   in_layout.AddGroup("Ramp");
      item = in_layout.AddItem("ramp",        "Ramp");
      item = in_layout.AddEnumControl("axis", Array(
         "X", "x",
         "Y", "y",
         "Z", "z"), "Axis", siControlCombo);
   in_layout.EndGroup();

   in_layout.SetAttribute(siUILogicPrefix, "light_blocker_");
}

function light_blocker_SetExpression_OnClicked() 
{
   var aRtn = PickObject("Pick Object", "Pick Object");
   var button = aRtn.Value("ButtonPressed");
   if (button == 1) // LMB
   {
      var obj  = aRtn.Value("PickedElement");

      var sx = obj.fullname + ".kine.global.sclx";
      var sy = obj.fullname + ".kine.global.scly";
      var sz = obj.fullname + ".kine.global.sclz";
      var rx = obj.fullname + ".kine.global.rotx";
      var ry = obj.fullname + ".kine.global.roty";
      var rz = obj.fullname + ".kine.global.rotz";
      var tx = obj.fullname + ".kine.global.posx";
      var ty = obj.fullname + ".kine.global.posy";
      var tz = obj.fullname + ".kine.global.posz";

      var pset = PPG.Inspected.Item(0);

      SetExpr(pset + ".scale.x", sx);
      SetExpr(pset + ".scale.y", sy);
      SetExpr(pset + ".scale.z", sy);
      SetExpr(pset + ".rotation.x", rx);
      SetExpr(pset + ".rotation.y", ry);
      SetExpr(pset + ".rotation.z", rz);
      SetExpr(pset + ".translation.x", tx);
      SetExpr(pset + ".translation.y", ty);
      SetExpr(pset + ".translation.z", tz);
   }
}

function light_blocker_RemoveExpression_OnClicked() 
{
   var pset = PPG.Inspected.Item(0);
   RemoveAnimation(pset + ".scale.x", null, null, null, null, null);
   RemoveAnimation(pset + ".scale.y", null, null, null, null, null);
   RemoveAnimation(pset + ".scale.z", null, null, null, null, null);
   RemoveAnimation(pset + ".rotation.x", null, null, null, null, null);
   RemoveAnimation(pset + ".rotation.y", null, null, null, null, null);
   RemoveAnimation(pset + ".rotation.z", null, null, null, null, null);
   RemoveAnimation(pset + ".translation.x", null, null, null, null, null);
   RemoveAnimation(pset + ".translation.y", null, null, null, null, null);
   RemoveAnimation(pset + ".translation.z", null, null, null, null, null);
}


////////// light_decay //////////
function ArnoldLightShaders_light_decay_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_light_decay_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;

   h.AddBoolean(params, "use_near_atten", false, true, false, true);
   h.AddScalar (params, "near_start",     0, 0, 1000000, 0, 500, true, false, true);
   h.AddScalar (params, "near_end",       0, 0, 1000000, 0, 500, true, false, true);
   h.AddBoolean(params, "use_far_atten",  false, true, false, true);
   h.AddScalar (params, "far_start",      0, 0, 1000000, 0, 500, true, false, true);
   h.AddScalar (params, "far_end",        0, 0, 1000000, 0, 500, true, false, true);

   // OUTPUT
   h.AddOutputShader(shaderDef.OutputParamDefs);

   // Renderer definition
   h.AddArnoldRendererDef(shaderDef);
   
   light_decay_Layout(shaderDef.PPGLayout);

   return true;
}

function light_decay_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Light+Decay");

   in_layout.AddGroup("Near Attenuation");
      item = in_layout.AddItem("use_near_atten", "Enable");
      item = in_layout.AddItem("near_start",     "Start");
      item = in_layout.AddItem("near_end",       "End");
   in_layout.EndGroup();

   in_layout.AddGroup("Far Attenuation");
      item = in_layout.AddItem("use_far_atten", "Enable");
      item = in_layout.AddItem("far_start",     "Start");
      item = in_layout.AddItem("far_end",       "End");
   in_layout.EndGroup();

   in_layout.SetAttribute(siUILogicPrefix, "light_decay_");
}

function light_decay_OnInit()
{
   light_decay_use_near_atten_OnChanged();
   light_decay_use_far_atten_OnChanged();
}

function light_decay_use_near_atten_OnChanged()
{
   PPG.near_start.Enable(PPG.use_near_atten.Value);
   PPG.near_end.Enable(PPG.use_near_atten.Value);
}

function light_decay_use_far_atten_OnChanged()
{
   PPG.far_start.Enable(PPG.use_far_atten.Value);
   PPG.far_end.Enable(PPG.use_far_atten.Value);
}


////////// passthrough_filter //////////
function ArnoldLightShaders_passthrough_filter_1_0_DefineInfo(in_ctxt) 
{
   return true;
}

function ArnoldLightShaders_passthrough_filter_1_0_Define(in_ctxt) 
{
   var h = SItoAShaderDefHelpers(); // helper object

   var shaderDef = in_ctxt.GetAttribute("Definition");
   shaderDef.AddShaderFamily(siShaderFamilyLight);
 
   // INPUT      
   params = shaderDef.InputParamDefs;

   h.AddInteger(params, "mode",  1, 0, 2, 0, 2, true, false, true);
   h.AddColor3 (params, "color", 1, 1, 1);

   // OUTPUT
   h.AddOutputShader(shaderDef.OutputParamDefs);

   // Renderer definition
   h.AddArnoldRendererDef(shaderDef);
   
   passthrough_filter_Layout(shaderDef.PPGLayout);

   return true;
}

function passthrough_filter_Layout(in_layout) 
{
   in_layout.Clear();
   in_layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/passthrough_filter");

   item = in_layout.AddEnumControl("mode", Array(
      "Mute",     0,
      "Set",      1,
      "Multiply", 2), "Mode", siControlCombo);

   item = in_layout.AddItem("color", "Color");

   in_layout.SetAttribute(siUILogicPrefix, "passthrough_filter_");
}


function passthrough_filter_OnInit()
{
   passthrough_filter_mode_OnChanged();
}

function passthrough_filter_mode_OnChanged()
{
   PPG.color.Enable(PPG.mode.value > 0);
}


//////////////////////////////////////////////////////////////
//////////                UTILITIES                 //////////
//////////////////////////////////////////////////////////////


function SetLabelPixelsAndPcg(in_item, in_labelMinPixels, in_labelPcg)
{
   in_item.LabelMinPixels  = in_labelMinPixels;
   in_item.LabelPercentage = in_labelPcg;
}

