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
   h.SetPluginInfo(in_reg, "Arnold Lights");
   
   // Light commands
   in_reg.RegisterCommand("AddPointLight",       "SITOA_AddPointLight");
   in_reg.RegisterCommand("AddDistantLight",     "SITOA_AddDistantLight");
   in_reg.RegisterCommand("AddSpotLight",        "SITOA_AddSpotLight");
   in_reg.RegisterCommand("AddSkydomeLight",     "SITOA_AddSkydomeLight");
   in_reg.RegisterCommand("AddDiskLight",        "SITOA_AddDiskLight");
   in_reg.RegisterCommand("AddQuadLight",        "SITOA_AddQuadLight");
   in_reg.RegisterCommand("AddCylinderLight",    "SITOA_AddCylinderLight");
   in_reg.RegisterCommand("AddMeshLight",        "SITOA_AddMeshLight");
   in_reg.RegisterCommand("AddPhotometricLight", "SITOA_AddPhotometricLight");

   // Light filters
   in_reg.RegisterCommand("AddBarndoorFilter",     "SITOA_AddBarndoorFilter");
   in_reg.RegisterCommand("AddGoboFilter",         "SITOA_AddGoboFilter");
   in_reg.RegisterCommand("AddLightBlockerFilter", "SITOA_AddLightBlockerFilter"); 
   in_reg.RegisterCommand("AddLightDecayFilter",   "SITOA_AddLightDecayFilter");
   in_reg.RegisterCommand("AddPassthroughFilter",  "SITOA_AddPassthroughFilter");

   return true;
}


/////////////////////////////////
//   LIGHTS xsiMenu
/////////////////////////////////
// unused, but let's keep it here, in case we need to get the soft version in this js
function GetVersion()
{
	var v = Application.Version();
	var s = v.split(".");
	return s[0];
}

// COMMANDS 

function CommonAddLightArguments(in_ctxt)
{
   var oCmd = in_ctxt.Source;
   var oArgs = oCmd.Arguments;
   oArgs.Add("in_name", siArgumentInput);
   return true;
}


function AddPointLight_Init(in_ctxt)
{
   CommonAddLightArguments(in_ctxt);
}

function AddPointLight_Execute(in_name)
{
   var name = in_name == null ? "Point" : in_name;
   var light = GetPrimLight("Point.Preset", name, "", null, null, null);
   
   ApplyLightShader(light, "arnold_point_light");
   light.Parameters("LightArea").value  = true;
   light.Parameters("LightAreaGeom").value  = 3;   
   // Mapping Arnold Light Parameter to XSI Light Parameter..   
   var spotRadius = light.Parameters("LightAreaXformSX");
   spotRadius.AddExpression(light.FullName+".light.point_light.radius");
   return light; // return the light
}



function AddDistantLight_Init(in_ctxt)
{
   CommonAddLightArguments(in_ctxt);
}

function AddDistantLight_Execute(in_name)
{
   var name = in_name == null ? "Infinite" : in_name;
   var light = GetPrimLight("Infinite.Preset", name, "", null, null, null);
   
   ApplyLightShader(light, "arnold_distant_light");
   return light;
}


function AddSpotLight_Init(in_ctxt)
{
   CommonAddLightArguments(in_ctxt);
}

function AddSpotLight_Execute(in_name)
{
   var name = in_name == null ? "Spot" : in_name;
   var light = GetPrimLight("Spot.Preset", name, "", null, null, null);
   var lightPrim = light.Light;
   
   ApplyLightShader(lightPrim, "arnold_spot_light");
   // Mapping Arnold Light Parameters to XSI Light Parameters..
   var coneAngle = lightPrim.Parameters("LightCone");
   coneAngle.AddExpression(lightPrim.FullName+".light.spot_light.cone_angle");
   var spotRadius = lightPrim.Parameters("LightAreaXformSX");
   spotRadius.AddExpression(lightPrim.FullName+".light.spot_light.radius");
   lightPrim.LightArea = true;   
   lightPrim.LightAreaGeom = 2;
   return lightPrim;
}


function AddSkydomeLight_Init(in_ctxt)
{
   CommonAddLightArguments(in_ctxt);
}

function AddSkydomeLight_Execute(in_name)
{
   var name = in_name == null ? "Skydome" : in_name;
   var light = GetPrimLight("LightSun.Preset", name, "", null, null, null);
   
   ApplyLightShader(light, "arnold_skydome_light"); 
   return light;
}


function AddQuadLight_Init(in_ctxt)
{
   CommonAddLightArguments(in_ctxt);
}

function AddQuadLight_Execute(in_name)
{
   var name = in_name == null ? "Quad" : in_name;
   var light = GetPrimLight("Light_Box.Preset", name, "", null, null, null);
   var lightPrim = light.Light;
   
   ApplyLightShader(lightPrim, "arnold_quad_light"); 
   return lightPrim;   
}


function AddDiskLight_Init(in_ctxt)
{
   CommonAddLightArguments(in_ctxt);
}

function AddDiskLight_Execute(in_name)
{
   var name = in_name == null ? "Disk" : in_name;
   var light = GetPrimLight("Spot.Preset", name, "", null, null, null);
   var lightPrim = light.Light;   
   var lightType = lightPrim.Parameters("Type");
   lightType.Value = 1; // Infinite light gizmo
   lightPrim.LightArea = true;
   lightPrim.LightAreaGeom = 2 // 2 means Disk shape

   ApplyLightShader(lightPrim, "arnold_disk_light"); 
   return lightPrim;
}


function AddCylinderLight_Init(in_ctxt)
{
   CommonAddLightArguments(in_ctxt);
}

function AddCylinderLight_Execute(in_name)
{
   var name = in_name == null ? "Cylinder" : in_name;
   var light = GetPrimLight("Point.Preset", name, "", null, null, null);
   light.LightArea = true;
   light.LightAreaGeom = 4 // 4 means Cylinder shape
   
   ApplyLightShader(light, "arnold_cylinder_light");
   return light;
}


function AddMeshLight_Init(in_ctxt)
{
   var oCmd = in_ctxt.Source;
   var oArgs = oCmd.Arguments;
   oArgs.Add("in_name", siArgumentInput);
   oArgs.AddObjectArgument("in_obj");
   return true;
}

function AddMeshLight_Execute(in_name, in_obj)
{
   var name = in_name == null ? "Mesh" : in_name;
   var refObj;
   
   if (in_obj == null)
   {
      // first, let ask to pick a mesh object
      var siRMB = 0;
      Application.StatusBar ="Pick mesh object";
      var rtn = PickObject( "Pick mesh object", "");
      refObj = rtn.Value("PickedElement");
      var button = rtn.Value("ButtonPressed");
      var modifier = rtn.Value("ModifierPressed");

      if (button == siRMB)
         return;
         
      if (refObj.type != "polymsh")
      {
         logMessage("Please pick a mesh object", siWarningMsg);
         return null;
      }
   }
   else
   {
      if (in_obj.type != "polymsh")
      {
         logMessage("Input argument not a mesh object", siWarningMsg);
         return null;
      }
      refObj = in_obj;
   }
   
   // Create Light Rig
   var light = GetPrimLight("Point.Preset", name, "", null, null, null);
   light.LightArea = true;
   light.LightAreaGeom = 5 // 5 means Object shape, Softimage >= 2011 only
   
   SetReference2(light + ".LightAreaObject", refObj, null);

   // apply shader
   ApplyLightShader(light, "arnold_mesh_light");
   // constrain the light TO the mesh
   ApplyCns("Pose", light, refObj, null);
   return light;
}


function AddPhotometricLight_Init(in_ctxt)
{
   CommonAddLightArguments(in_ctxt);
}

function AddPhotometricLight_Execute(in_name)
{
   var name = in_name == null ? "Spot" : in_name;
   var light = GetPrimLight("Spot.Preset", name, "", null, null, null);
   var lightPrim = light.Light;
   
   ApplyLightShader(lightPrim, "arnold_photometric_light");
   return lightPrim;
}


///////////////////////////////////////
///////////////////////////////////////

function ApplyLightShader(primitive, shaderName)
{
   var shader = CreateShaderFromProgID("ArnoldLightShaders." + shaderName + ".1.0", primitive, null);
   SIConnectShaderToCnxPoint(shader, primitive + ".LightShader", true);
}


///////////////////////////////////////
// Light filters commands
///////////////////////////////////////

// Commands for applying the filters

function CommonAddFilterArguments(in_ctxt)
{
   var oCmd = in_ctxt.Source;
   var oArgs = oCmd.Arguments;
   oArgs.AddWithHandler("in_collection", "Collection");
   oArgs.Add("in_inspect", siArgumentInput);
   return true;
}


function AddBarndoorFilter_Init(in_ctxt)
{
   return CommonAddFilterArguments(in_ctxt);
}

function AddBarndoorFilter_Execute(in_collection, in_inspect)
{
   if (in_collection.count == 0)
      return null;
   logMessage("[sitoa] Adding Barndoor Filter");
   var inspect = in_inspect == null ? true : in_inspect;
   var out_collection = AssignFilter(in_collection, "barndoor", inspect);
   return out_collection;
}


function AddGoboFilter_Init(in_ctxt)
{
   return CommonAddFilterArguments(in_ctxt);
}

function AddGoboFilter_Execute(in_collection, in_inspect)
{
   if (in_collection.count == 0)
      return null;
   logMessage("[sitoa] Adding Gobo Filter");
   var inspect = in_inspect == null ? true : in_inspect;
   var out_collection = AssignFilter(in_collection, "gobo", inspect);
   return out_collection;
}


function AddLightBlockerFilter_Init(in_ctxt)
{
   return CommonAddFilterArguments(in_ctxt);
}

function AddLightBlockerFilter_Execute(in_collection, in_inspect)
{
   if (in_collection.count == 0)
      return null;
   logMessage("[sitoa] Adding Light Blocker Filter");
   var inspect = in_inspect == null ? true : in_inspect;
   var out_collection = AssignFilter(in_collection, "light_blocker", inspect);
   return out_collection;
}


function AddLightDecayFilter_Init(in_ctxt)
{
   return CommonAddFilterArguments(in_ctxt);
}

function AddLightDecayFilter_Execute(in_collection, in_inspect)
{
   if (in_collection.count == 0)
      return null;
   logMessage("[sitoa] Adding Light Decay Filter");
   var inspect = in_inspect == null ? true : in_inspect;
   var out_collection = AssignFilter(in_collection, "light_decay", inspect);
   return out_collection;
}


function AddPassthroughFilter_Init(in_ctxt)
{
   return CommonAddFilterArguments(in_ctxt);
}

function AddPassthroughFilter_Execute(in_collection, in_inspect)
{
   if (in_collection.count == 0)
      return null;
   logMessage("[sitoa] Adding Passthrough Filter");
   var inspect = in_inspect == null ? true : in_inspect;
   var out_collection = AssignFilter(in_collection, "passthrough_filter", inspect);
   return out_collection;
}

///////////////////////////////////////
// New light filters functions
///////////////////////////////////////

var POINT_LIGHT       = 0;
var DISTANT_LIGHT     = 1;
var SPOT_LIGHT        = 2;
var AREA_LIGHT        = 3;
var SKYDOME_LIGHT     = 4;
var PHOTOMETRIC_LIGHT = 5;
var UNSUPPORTED_LIGHT = 6;

var FILTER_DECAY        = 0;
var FILTER_GOBO         = 1;
var FILTER_BARNDOOR     = 2;
var FILTER_BLOCKER      = 3;
var FILTER_PASSTHROUGH  = 4;
var FILTER_CUSTOM       = 5;

function GetLightShaderType(in_name)
{
   if (in_name.indexOf("arnold_point_light") != -1)
      return POINT_LIGHT;
   if (in_name.indexOf("arnold_distant_light") != -1)
      return DISTANT_LIGHT;
   if (in_name.indexOf("arnold_spot_light") != -1)
      return SPOT_LIGHT;
   if (in_name.indexOf("arnold_disk_light") != -1)
      return AREA_LIGHT;
   if (in_name.indexOf("arnold_quad_light") != -1)
      return AREA_LIGHT;
   if (in_name.indexOf("arnold_cylinder_light") != -1)
      return AREA_LIGHT;
   if (in_name.indexOf("arnold_mesh_light") != -1)
      return AREA_LIGHT;
   if (in_name.indexOf("skydome_light") != -1)
      return SKYDOME_LIGHT;
   if (in_name.indexOf("arnold_photometric_light") != -1)
      return PHOTOMETRIC_LIGHT;
      
   return UNSUPPORTED_LIGHT;  
}


function GetFilterShaderType(in_name)
{
   if (in_name.indexOf(".barndoor.") != -1)
      return FILTER_BARNDOOR;
   if (in_name.indexOf(".light_decay.") != -1)
      return FILTER_DECAY;
   if (in_name.indexOf(".light_blocker.") != -1)
      return FILTER_BLOCKER;
   if (in_name.indexOf(".gobo.") != -1)
      return FILTER_GOBO;
   if (in_name.indexOf(".passthrough_filter.") != -1)
      return FILTER_PASSTHROUGH;

   return FILTER_CUSTOM;
}


function IsFilterCompatibleWithLight(in_lightType, in_filterType)
{
   // blockers and passthrough ok for all types of light
   if (in_filterType == "light_blocker" || in_filterType == "passthrough_filter") 
      return true;
   else if (in_filterType == "light_decay")
      return !(in_lightType == DISTANT_LIGHT || in_lightType == SKYDOME_LIGHT);
   else if (in_filterType == "gobo")
      return in_lightType == SPOT_LIGHT;
   else if (in_filterType == "barndoor")
      return in_lightType == SPOT_LIGHT;

   return false;
}


function AddFilterShader(in_filterName, in_lightShader, in_light, in_inspect)
{

   var shader = shader = CreateShaderFromProgID("ArnoldLightShaders." + in_filterName + ".1.0", in_light, null);

   var port;
   var portFound = false;
   var nbGobo=0;
   var nbBarndoor=0;

   for (var i=1; i<=5; i++)
   {
      var portName = "filter" + i;
      var currentFilter = in_lightShader.Parameters(portName).source;
      
      if (!portFound)
      if (currentFilter == null)
      {
         // first available free port is used to connect the new filter
         port = in_lightShader.Parameters(portName);
         portFound = true;
      }
      
      if (currentFilter != null)
      {
         currentFilterType = GetFilterShaderType(currentFilter.parent.ProgId);
         if (currentFilterType == FILTER_GOBO)
            nbGobo++;
         else if (currentFilterType == FILTER_BARNDOOR)
            nbBarndoor++;
      }
   }
   
   if (!portFound)
   {
      logMessage("[sitoa] Failed to connect " + in_filterName + " to " + in_lightShader, siErrorMsg);
      return null;
   }

   if (in_filterName == "gobo" && nbGobo > 0)
   {
      logMessage("[sitoa] Can't connect more than 1 gobo filter to " + in_lightShader, siErrorMsg);
      return null;
   }
   if (in_filterName == "barndoor" && nbBarndoor > 0)
   {
      logMessage("[sitoa] Can't connect more than 1 barndoor filter to " + in_lightShader, siErrorMsg);
      return null;
   }

   SIConnectShaderToCnxPoint(shader, port, false);

   if (in_inspect)
      InspectObj(shader);
      
   return shader;
}


function AssignFilterToLight(in_light, in_filter, in_inspect)
{
   if (in_light.type != "light")
   {
      logMessage("[sitoa] Can't assign a light filter to " + in_light.FullName, siErrorMsg);
      return null;
   }
   
   var lightShader = in_light.Parameters("LightShader").source.parent;
   var shaderType = GetLightShaderType(lightShader.ProgId);
   
   if (shaderType == UNSUPPORTED_LIGHT)
   {
      logMessage("[sitoa] Can't assign a light filter to " + in_light.FullName, siErrorMsg);
      return null;
   }
   
   if (!IsFilterCompatibleWithLight(shaderType, in_filter))
   {
      logMessage("[sitoa] Can't assign a " + in_filter + " light filter to " + in_light.FullName, siErrorMsg);
      return null;
   }
   
   return AddFilterShader(in_filter, lightShader, in_light, in_inspect);
}


function AssignFilter(in_collection, in_filter, in_inspect)
{
   var out_collection = new ActiveXObject("XSI.Collection");

   var filter;
   for (var i=0; i<in_collection.count; i++)
   {
      filter = AssignFilterToLight(in_collection.Item(i), in_filter, in_inspect);
      if (filter != null)
         out_collection.Add(filter);
   }
   
   if (out_collection.Count > 0)
      return out_collection;
   else
      return null;
}

