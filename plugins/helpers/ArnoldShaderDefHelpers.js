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
   h.SetPluginInfo(in_reg, "ArnoldShaderDefHelpers");

   in_reg.RegisterCommand("SItoAShaderDefHelpers", "SItoAShaderDefHelpers");

   return true;
}

function XSIUnloadPlugin( in_reg )
{
   return true;
}

function SItoAShaderDefHelpers_Init(io_Context)
{
   var oCmd = io_Context.Source;
   oCmd.SetFlag(siNoLogging, true);
   oCmd.SetFlag(siSupportsKeyAssignment, false);      
   oCmd.ReturnValue = true;
   return true;   
}

function SItoAShaderDefHelpers_Execute()
{
   return new ShaderHelperObj();
}


function ShaderHelperObj()
{
   this.Type = "SItoAHelperObj";

   // constant guids for mapping some parameters to the ogl viewport
   // 
   // the guid used as port for the others
   this.UiMapperGuid     = "{8C80DEF3-1064-11d3-B0B7-00A0C982A112}";
   // constant color
   this.UiConstantGuid   = "{3515CC71-082C-11D0-91DE-00A024C78EE3}";
   // diffuse color
   this.UiDiffuseGuid    = "{3515CC72-082C-11D0-91DE-00A024C78EE3}";
   // specular color
   this.UiSpecularGuid   = "{3515CC73-082C-11D0-91DE-00A024C78EE3}";
   // light color
   this.UiLightColorGuid = "{07C5EDE4-52A4-11D0-8298-00A0243E366B}";

   this.SetCapability = function(in_paramOptions, in_animatable, in_texturable, in_inspectable)
   {
      if (in_animatable != null)
         in_paramOptions.SetAnimatable(in_animatable);

      if (in_texturable != null)
         in_paramOptions.SetTexturable(in_texturable);
      else // it's off by default, we want it on
         in_paramOptions.SetTexturable(true);

      if (in_inspectable != null)
         in_paramOptions.SetInspectable(in_inspectable);
   }


   this.SetMinMax = function(in_paramOptions, in_min, in_max, in_smin, in_smax)
   {
      if (in_min != null && in_max != null)
         in_paramOptions.SetHardLimit(in_min, in_max);
   
      if (in_smin != null && in_smax != null)
         in_paramOptions.SetSoftLimit(in_smin, in_smax);
   }

   this.AddScalar = function(in_params, in_name, in_default, in_min, in_max, in_smin, in_smax, in_animatable, in_texturable, in_inspectable)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();

      this.SetCapability(paramOptions, in_animatable, in_texturable, in_inspectable);
      this.SetMinMax(paramOptions, in_min, in_max, in_smin, in_smax);
      paramOptions.SetDefaultValue(in_default);
      
      in_params.AddParamDef(in_name, siShaderDataTypeScalar, paramOptions); 
   }

   this.AddInteger = function(in_params, in_name, in_default, in_min, in_max, in_smin, in_smax, in_animatable, in_texturable, in_inspectable)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();

      this.SetCapability(paramOptions, in_animatable, in_texturable, in_inspectable);
      this.SetMinMax(paramOptions, in_min, in_max, in_smin, in_smax);
      paramOptions.SetDefaultValue(in_default);
      
      in_params.AddParamDef(in_name, siShaderDataTypeInteger, paramOptions); 
   }

   this.AddBoolean = function(in_params, in_name, in_default, in_animatable, in_texturable, in_inspectable)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();

      this.SetCapability(paramOptions, in_animatable, in_texturable, in_inspectable);
      paramOptions.SetDefaultValue(in_default);
      
      in_params.AddParamDef(in_name, siShaderDataTypeBoolean, paramOptions); 
   }


   this.AddColor3 = function(in_params, in_name, in_default_r, in_default_g, in_default_b, in_animatable, in_texturable, in_inspectable, in_uiMapping)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();

      this.SetCapability(paramOptions, in_animatable, in_texturable, in_inspectable);
      
      if (in_uiMapping != null)
         paramOptions.SetAttribute(this.UiMapperGuid, in_uiMapping);
      
      var paramDef = in_params.AddParamDef(in_name, siShaderDataTypeColor3, paramOptions);
      var subParamDef = paramDef.SubParamDefs;
      subParamDef.GetParamDefByName("red").DefaultValue = in_default_r;
      subParamDef.GetParamDefByName("green").DefaultValue = in_default_g;
      subParamDef.GetParamDefByName("blue").DefaultValue = in_default_b;
   }
 

   this.AddColor4 = function(in_params, in_name, in_default_r, in_default_g, in_default_b, in_default_a, in_animatable, in_texturable, in_inspectable)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();

      this.SetCapability(paramOptions, in_animatable, in_texturable, in_inspectable);
      
      var paramDef = in_params.AddParamDef(in_name, siShaderDataTypeColor4, paramOptions);
      var subParamDef = paramDef.SubParamDefs;
      subParamDef.GetParamDefByName("red").DefaultValue = in_default_r;
      subParamDef.GetParamDefByName("green").DefaultValue = in_default_g;
      subParamDef.GetParamDefByName("blue").DefaultValue = in_default_b;
      subParamDef.GetParamDefByName("alpha").DefaultValue = in_default_a;
   }

   this.AddVector3 = function(in_params, in_name, in_default_x, in_default_y, in_default_z, in_min, in_max, in_smin, in_smax, in_animatable, in_texturable, in_inspectable)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();

      this.SetCapability(paramOptions, in_animatable, in_texturable, in_inspectable);
      this.SetMinMax(paramOptions, in_min, in_max, in_smin, in_smax);
      
      var paramDef = in_params.AddParamDef(in_name, siShaderDataTypeVector3, paramOptions);
      var subParamDef = paramDef.SubParamDefs;
      subParamDef.GetParamDefByName("x").DefaultValue = in_default_x;
      subParamDef.GetParamDefByName("y").DefaultValue = in_default_y;
      subParamDef.GetParamDefByName("z").DefaultValue = in_default_z;
   }

   this.AddVector2 = function(in_params, in_name, in_default_x, in_default_y, in_min, in_max, in_smin, in_smax, in_animatable, in_texturable, in_inspectable)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();

      this.SetCapability(paramOptions, in_animatable, in_texturable, in_inspectable);
      this.SetMinMax(paramOptions, in_min, in_max, in_smin, in_smax);
      
      var paramDef = in_params.AddParamDef(in_name, siShaderDataTypeVector2, paramOptions);
      var subParamDef = paramDef.SubParamDefs;
      subParamDef.GetParamDefByName("x").DefaultValue = in_default_x;
      subParamDef.GetParamDefByName("y").DefaultValue = in_default_y;
   }

   this.AddString = function(in_params, in_name, in_default)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      this.SetCapability(paramOptions, false, false, true);
      paramOptions.SetDefaultValue(in_default);
      var paramDef = in_params.AddParamDef(in_name, siShaderDataTypeString, paramOptions);
   }

   this.AddLightProfile = function(in_params, in_name)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      var paramDef = in_params.AddParamDef(in_name, siShaderDataTypeLightProfile, paramOptions);
   }

   this.AddShader = function(in_params, in_name)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      this.SetCapability(paramOptions, false, true, false); // for input shader ports
      paramOptions.SetAttribute(siReferenceFilterAttribute, siShaderReferenceFilter);
      var paramDef = in_params.AddParamDef(in_name, siShaderDataTypeReference, paramOptions);
   }

   this.AddImage = function(in_params, in_name)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      var paramDef = in_params.AddParamDef(in_name, siShaderDataTypeImage, paramOptions);      
   }

   this.AddNode = function(in_params, in_name)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      this.SetCapability(paramOptions, false, true, false);
      var paramDef = in_params.AddParamDef(in_name, siShaderDataTypeReference, paramOptions);
   }

   this.AddOutputColor3 = function(in_params)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      paramDef = in_params.AddParamDef("out", siShaderDataTypeColor3, paramOptions);
      paramDef.MainPort = false;       
   }
 
   this.AddOutputColor4 = function(in_params)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      paramDef = in_params.AddParamDef("out", siShaderDataTypeColor4, paramOptions);
      paramDef.MainPort = false;       
   }

   this.AddOutputScalar = function(in_params)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      paramDef = in_params.AddParamDef("out", siShaderDataTypeScalar, paramOptions);
      paramDef.MainPort = false;       
   }

   this.AddOutputInteger = function(in_params)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      paramDef = in_params.AddParamDef("out", siShaderDataTypeInteger, paramOptions);
      paramDef.MainPort = false;       
   }

   this.AddOutputShader = function(in_params)
   {
      paramOptions = XSIFactory.CreateShaderParamDefOptions();
      paramOptions.SetAttribute(siReferenceFilterAttribute, siShaderReferenceFilter);
      paramDef = in_params.AddParamDef("out", siShaderDataTypeReference, paramOptions);
      paramDef.MainPort = false;       
   }

   this.AddArnoldRendererDef = function(in_shaderDef)
   {
      var h = SItoAToolHelper();
      in_shaderDef.AddRendererDef(h.RendererName);
   }
}





