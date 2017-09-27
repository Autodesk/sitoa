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
   h.SetPluginInfo(in_reg, "Arnold Properties");

   in_reg.RegisterCommand("AddVisibilityProperties",     "SITOA_AddVisibilityProperties");
   in_reg.RegisterCommand("AddMatteProperties",          "SITOA_AddMatteProperties");
   in_reg.RegisterCommand("AddSidednessProperties",      "SITOA_AddSidednessProperties");
   in_reg.RegisterCommand("AddParametersProperties",     "SITOA_AddParametersProperties");
   in_reg.RegisterCommand("AddProceduralProperties",     "SITOA_AddProceduralProperties");
   in_reg.RegisterCommand("AddUserOptionsProperties",    "SITOA_AddUserOptionsProperties");
   in_reg.RegisterCommand("AddTextureOptionsProperties", "SITOA_AddTextureOptionsProperties");
   in_reg.RegisterCommand("AddCameraOptionsProperties",  "SITOA_AddCameraOptionsProperties");
   in_reg.RegisterCommand("AddVolumeProperties",         "SITOA_AddVolumeProperties");

   in_reg.RegisterProperty("arnold_visibility");
   in_reg.RegisterProperty("arnold_matte");
   in_reg.RegisterProperty("arnold_sidedness");
   in_reg.RegisterProperty("arnold_parameters");
   in_reg.RegisterProperty("arnold_procedural");
   in_reg.RegisterProperty("arnold_user_options");
   in_reg.RegisterProperty("arnold_texture_options");
   in_reg.RegisterProperty("arnold_camera_options");
   in_reg.RegisterProperty("arnold_volume");

   return true;
}


/////////////////////////////////
//   COMMANDS
/////////////////////////////////

function CommonAddProperties(in_collection, in_type, in_inspect)
{
   if (in_collection.count == 0)
      return null;
      
   var inspect = in_inspect == null ? true : in_inspect; 

   var prop;
   var out_collection = new ActiveXObject("XSI.Collection");
   var volume_collection = new ActiveXObject("XSI.Collection");

   logMessage("[sitoa] Adding Arnold " + in_type + " Properties");

   for (var i=0; i<in_collection.count; i++)
   {
      prop = null;
      switch (in_type)
      {
         case "Visibility":
            prop = AddVisibilityProperty(in_collection.item(i));
            break;
         case "Matte":
            prop = AddMatteProperty(in_collection.item(i));
            break;
         case "Sidedness":
            prop = AddSidednessProperty(in_collection.item(i));
            break;
         case "Parameters":
            prop = AddParametersProperty(in_collection.item(i));
            break;
         case "Procedural":
            prop = AddProceduralProperty(in_collection.item(i));
            break;
         case "User Options":
            prop = AddUserOptionsProperty(in_collection.item(i));
            break;
         case "Texture Options":
            prop = AddTextureOptionsProperty(in_collection.item(i));
            break;            
         case "Camera Options":
            prop = AddCameraOptionsProperty(in_collection.item(i));
            break;            
         case "Volume":
            prop = AddVolumeProperty(in_collection.item(i));
            if (prop != null)
               volume_collection.Add(in_collection.item(i))
            break;
         default:
            break;
      }

      if (prop != null)
      {
         out_collection.Add(prop);
         if (inspect && in_type != "Volume")
            InspectObj(prop);
      }
   }

   // apply standard_volume to the volume objects
   if (in_type == "Volume")
   {
      SITOA_AddShader("Arnold.standard_volume.1.0", "surface", null, false);
      InspectObj(prop);
   }

   return out_collection;
}


// COMMANDS 

function CommonAddArguments(in_ctxt)
{
   var oCmd = in_ctxt.Source;
   var oArgs = oCmd.Arguments;
   oArgs.AddWithHandler("in_collection", "Collection");
   oArgs.Add("in_inspect");
   return true;
}


function AddVisibilityProperties_Init(in_ctxt)
{
   return CommonAddArguments(in_ctxt);
}

function AddVisibilityProperties_Execute(in_collection, in_inspect)
{
   return CommonAddProperties(in_collection, "Visibility", in_inspect);
}


function AddMatteProperties_Init(in_ctxt)
{
   return CommonAddArguments(in_ctxt);
}

function AddMatteProperties_Execute(in_collection, in_inspect)
{
   return CommonAddProperties(in_collection, "Matte", in_inspect);
}


function AddSidednessProperties_Init(in_ctxt)
{
   return CommonAddArguments(in_ctxt);
}

function AddSidednessProperties_Execute(in_collection, in_inspect)
{
   return CommonAddProperties(in_collection, "Sidedness", in_inspect);
}


function AddParametersProperties_Init(in_ctxt)
{
   return CommonAddArguments(in_ctxt);
}

function AddParametersProperties_Execute(in_collection, in_inspect)
{
   return CommonAddProperties(in_collection, "Parameters", in_inspect);
}


function AddProceduralProperties_Init(in_ctxt)
{
   return CommonAddArguments(in_ctxt);
}

function AddProceduralProperties_Execute(in_collection, in_inspect)
{
   return CommonAddProperties(in_collection, "Procedural", in_inspect);
}


function AddUserOptionsProperties_Init(in_ctxt)
{
   return CommonAddArguments(in_ctxt);
}

function AddUserOptionsProperties_Execute(in_collection, in_inspect)
{
   return CommonAddProperties(in_collection, "User Options", in_inspect);
}

function AddTextureOptionsProperties_Init(in_ctxt)
{
   return CommonAddArguments(in_ctxt);
}

function AddTextureOptionsProperties_Execute(in_collection, in_inspect)
{
   return CommonAddProperties(in_collection, "Texture Options", in_inspect);
}


function AddCameraOptionsProperties_Init(in_ctxt)
{
   return CommonAddArguments(in_ctxt);
}

function AddCameraOptionsProperties_Execute(in_collection, in_inspect)
{
   return CommonAddProperties(in_collection, "Camera Options", in_inspect);
}


function AddVolumeProperties_Init(in_ctxt)
{
   return CommonAddArguments(in_ctxt);
}

function AddVolumeProperties_Execute(in_collection, in_inspect)
{
   return CommonAddProperties(in_collection, "Volume", in_inspect);
}

/////////////////////////////////
// PROPERTY ADDERS
/////////////////////////////////


function CommonHasProperty(in_obj, in_name)
{
   var propCollection = in_obj.Properties;
   var prop = propCollection.find(in_name);
   return classname(prop) != "";
}


function AddVisibilityProperty(in_xsiObj)
{
   var prop = null;
    // is in_xsiObj capable of hosting properties ?
   var classOk = in_xsiObj.IsClassOf(siSceneItemID);
  
   var type = in_xsiObj.type;
   if (classOk && (type == "polymsh" || type == "#Group" || type == "pointcloud" || type == "hair" || type == "Partition"))
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_visibility"))
         prop = in_xsiObj.AddProperty("arnold_visibility", false, "Arnold Visibility");
   }
   else
      logmessage("[sitoa] Cannot add Visibility Settings to " + in_xsiObj.FullName + ". It can only be assigned to PolyMeshes, Groups, PointClouds, Hair or Partitions", siErrorMsg);
      
   return prop;      
}


function AddMatteProperty(in_xsiObj)
{
   var prop = null;
    // is in_xsiObj capable of hosting properties ?
   var classOk = in_xsiObj.IsClassOf(siSceneItemID);
  
   var type = in_xsiObj.type;
   if (classOk && (type == "polymsh" || type == "#Group" || type == "pointcloud" || type == "hair" || type == "Partition"))
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_matte"))
         prop = in_xsiObj.AddProperty("arnold_matte", false, "Arnold Matte");
   }
   else
      logmessage("[sitoa] Cannot add Matte Settings to " + in_xsiObj.FullName + ". It can only be assigned to PolyMeshes, Groups, PointClouds, Hair or Partitions", siErrorMsg);
      
   return prop;      
}


function AddSidednessProperty(in_xsiObj)
{
   var prop = null;
   // is in_xsiObj capable of hosting properties ?
   var classOk = in_xsiObj.IsClassOf(siSceneItemID);

   var type = in_xsiObj.type;
   if (classOk && (type == "polymsh" || type == "#Group" || type == "pointcloud" || type == "hair" || type == "Partition"))
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_sidedness"))
         prop = in_xsiObj.AddProperty("arnold_sidedness", false, "Arnold Sidedness");
   }
   else
      logmessage("[sitoa] Cannot add Sidedness Settings to " + in_xsiObj.FullName + ". It can only be assigned to PolyMeshes, Groups, PointClouds, Hair or Partitions", siErrorMsg);

   return prop;      
}


function AddParametersProperty(in_xsiObj)
{
   var prop = null;
   // is in_xsiObj capable of hosting properties ?
   var classOk = in_xsiObj.IsClassOf(siSceneItemID);
   
   var type = in_xsiObj.type;
   if (classOk && (type == "polymsh" || type == "#Group" || type == "Partition"))
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_parameters"))
      {
         prop = in_xsiObj.AddProperty("arnold_parameters", false, "Arnold Parameters");
         AddParamsShape(prop);
         AddParamsSubdivision(prop);
      }
   }
   else if (classOk && (type == "hair" || type == "pointcloud"))
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_parameters"))
      {
         prop = in_xsiObj.AddProperty("arnold_parameters", false, "Arnold Parameters");
         AddParamsShape(prop);
         AddParamsCurve(prop, type=="pointcloud");
      }
   }
   else
      logmessage("[sitoa] " + in_xsiObj.FullName + " is not a valid Object to add Arnold Parameters to", siErrorMsg);

   return prop;      
}


function AddProceduralProperty(in_xsiObj)
{
   var prop = null;
   // is in_xsiObj capable of hosting properties ?
   var classOk = in_xsiObj.IsClassOf(siSceneItemID);
   
   var type = in_xsiObj.type;
   if (classOk && (type == "polymsh" || type == "hair" || type == "pointcloud"))
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_procedural"))
         prop = in_xsiObj.AddProperty("arnold_procedural", false, "Arnold Procedural");
   }
   else
      logmessage("[sitoa] Cannot add Procedural property to " + in_xsiObj.FullName + ". It can only be assigned to PolyMeshes, Hair and Pointclouds", siErrorMsg);

   return prop;      
}


function AddUserOptionsProperty(in_xsiObj)
{
   var prop = null;
   // is in_xsiObj capable of hosting properties ?
   var classOk = in_xsiObj.IsClassOf(siSceneItemID);

   var type = in_xsiObj.type;
   if (classOk && (type == "polymsh" || type == "hair" || type == "pointcloud" || type == "light" || type == "camera" || type == "#Group" || type == "Partition"))
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_user_options"))
         prop = in_xsiObj.AddProperty("arnold_user_options", false, "Arnold User Options");
   }
   else
      logMessage("[sitoa] Cannot add User Options property to " + in_xsiObj.FullName + ". It can only be assigned to PolyMeshes, Hair, Lights, Cameras and Pointclouds", siErrorMsg);

   return prop;      
}


function AddTextureOptionsProperty(in_xsiObj)
{
   var prop = null;
   var cName = ClassName(in_xsiObj);
   if (cName == "ImageClip")
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_texture_options"))
         prop = in_xsiObj.AddProperty("arnold_texture_options", false, "Arnold Texture Options");	  
   }
   else
      logMessage("[sitoa] Cannot add Texture Options property to " + in_xsiObj.FullName + ". It can only be assigned to image clips", siErrorMsg);

   return prop;      
}


function AddCameraOptionsProperty(in_xsiObj)
{
   var prop = null;
   var cName = ClassName(in_xsiObj);
   if (cName == "Camera")
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_camera_options"))
         prop = in_xsiObj.AddProperty("arnold_camera_options", false, "Arnold Camera Options");    
   }
   else
      logMessage("[sitoa] Cannot add Camera Options property to " + in_xsiObj.FullName + ". It can only be assigned to cameras", siErrorMsg);

   return prop;      
}


function AddVolumeProperty(in_xsiObj)
{
   var prop = null;
   // is in_xsiObj capable of hosting properties ?
   var classOk = in_xsiObj.IsClassOf(siSceneItemID);
   
   var type = in_xsiObj.type;
   if (classOk && (type == "polymsh" || type == "hair" || type == "pointcloud"))
   {
      if (!CommonHasProperty(in_xsiObj, "arnold_volume"))
         prop = in_xsiObj.AddProperty("arnold_volume", false, "Arnold Volume");
   }
   else
      logmessage("[sitoa] Cannot add Volume property to " + in_xsiObj.FullName + ". It can only be assigned to PolyMeshes, Hair and Pointclouds", siErrorMsg);

   return prop;      
}

/////////////////////////////////
/////////////////////////////////
/////////////////////////////////


// custom user parameters.
// these arrays try to mimic Arnold API constants/values

var ArnoldUserDataStructure = new Array
(
    "SINGLE", 0,
    "ARRAY", 1
);

var ArnoldUserDataType = new Array
(
    "INT",    0,
    "BOOL",   1,
    "FLOAT",  2,
    "RGB",    3,
    "RGBA",   4,
    "VECTOR", 5,
    "VECTOR2", 6,
    "STRING", 7,
    "MATRIX", 8
);


function AddUserData(in_prop, in_addMuteAndResolveTokens)
{
   var p;
   if (in_addMuteAndResolveTokens)
   {
      p = in_prop.AddParameter2("muteUserData", siBool, 0, 0, 1, 0, 1, 0, siPersistable | siAnimatable);
      p = in_prop.AddParameter2("resolveUserDataTokens", siBool, 0, 0, 1, 0, 1, 0, siPersistable | siAnimatable);
   }

   var gridItem = in_prop.AddGridParameter("userDataGrid");
   var grid = gridItem.Value

   grid.RowCount = 0;
   grid.ColumnCount = 4;
   
   grid.SetColumnLabel(0, "Name");
   grid.SetColumnLabel(1, "Structure");
   grid.SetColumnLabel(2, "Type");
   grid.SetColumnLabel(3, "Value");
   
   grid.SetColumnType(1, siColumnCombo);
   grid.SetColumnType(2, siColumnCombo);
   
   grid.SetColumnComboItems(1, ArnoldUserDataStructure);
   grid.SetColumnComboItems(2, ArnoldUserDataType);   
}


function AddParamsShape(in_prop)
{
   in_prop.AddParameter2("opaque",             siBool, 1, 0, 1, 0, 5, 0, siPersistable|siAnimatable);
   in_prop.AddParameter2("invert_normals",     siBool, 0, 0, 1, 0, 5, 0, siPersistable|siAnimatable);
   in_prop.AddParameter2("self_shadows",       siBool, 1, 0, 1, 0, 5, 0, siPersistable|siAnimatable);
   in_prop.AddParameter2("receive_shadows",    siBool, 1, 0, 1, 0, 5, 0, siPersistable|siAnimatable);
   in_prop.AddParameter2("export_pref",               siBool, false, null, null, null, null);
   in_prop.AddParameter2("subdiv_smooth_derivs",      siBool, false, null, null, null, null);
   in_prop.AddParameter2("sss_setname", siString, "", null, null, null, null, 0, siPersistable|siAnimatable);
   in_prop.AddParameter2("trace_sets",  siString, "", null, null, null, null, 0, siPersistable|siAnimatable);
    //new per-object motion blur 
   in_prop.AddParameter2("motion_transform",           siBool,     true, null, null, null, null);
   in_prop.AddParameter2("motion_deform",              siBool,     true, null, null, null, null);
   in_prop.AddParameter2("override_motion_step",       siBool,     false, null, null, null, null);
   in_prop.AddParameter2("motion_step_transform",      siInt4,     2, 2, 200, 2, 15);
   in_prop.AddParameter2("motion_step_deform",         siInt4,     2, 2, 200, 2, 15);
   in_prop.AddParameter2("use_pointvelocity",          siBool,     false, null, null, null, null);
   // volume step size
   in_prop.AddParameter2("step_size",        siFloat, 0.0, 0, 1000, 0, 1, 0, siPersistable | siAnimatable);
   in_prop.AddParameter2("volume_padding",   siFloat, 0.0, 0, 1000, 0, 1, 0, siPersistable | siAnimatable);
}


function AddParamsCurve(in_prop, strands)
{
   in_prop.AddParameter2("mode", siString, "ribbon", null, null, null, null, 0, siPersistable|siAnimatable);
   in_prop.AddParameter2("min_pixel_width", siFloat, 0.25, 0, 2, 0, 2, 0, siPersistable|siAnimatable);
}


function AddParamsSubdivision(in_prop, strands)
{
   in_prop.AddParameter2("disp_zero_value",        siFloat,  0.0,      -100000, 100000, 0.0,  1,    0, siPersistable|siAnimatable);
   in_prop.AddParameter2("disp_height",            siFloat,  1.0,      -100000, 100000, 0.0,  10.0, 0, siPersistable|siAnimatable);
   in_prop.AddParameter2("disp_padding",           siFloat,  0.0,      -100000, 100000, 0.0,  10.0, 0, siPersistable|siAnimatable);
   in_prop.AddParameter2("subdiv_iterations",      siInt4,   0,        0,       255,    0,    10,   0, siPersistable|siAnimatable);
   in_prop.AddParameter2("subdiv_adaptive_error",  siFloat,  2.0,      0.0,     100.0,  0.0,  10.0, 0, siPersistable|siAnimatable);
   in_prop.AddParameter2("disp_autobump",          siBool,   1,        0,       1,      0,    1,    0, siPersistable|siAnimatable);
   in_prop.AddParameter2("adaptive_subdivision",   siBool,   0,        0,       1,      0,    1,    0, siPersistable|siAnimatable);
   in_prop.AddParameter2("subdiv_adaptive_metric", siString, "auto",   null,    null,   null, null, 0, siPersistable|siAnimatable);   
   in_prop.AddParameter2("subdiv_adaptive_space",  siString, "raster", null,    null,   null, null, 0, siPersistable|siAnimatable);   
}


function UserDataLayout(in_layout, in_addMuteAndResolveTokens)
{
   in_layout.AddGroup("User Data");

   if (in_addMuteAndResolveTokens)
   {
      in_layout.AddRow();
         xsiItem = in_layout.AddItem("muteUserData", "Mute");
         xsiItem = in_layout.AddItem("resolveUserDataTokens", "Resolve Tokens");
      in_layout.EndRow();
   }  

      in_layout.AddSpacer();

      gridItem = in_layout.AddItem("userDataGrid", "", siControlGrid);
      gridItem.SetAttribute(siUINoLabel, true);
      gridItem.SetAttribute(siUIGridSelectionMode, siSelectionCell)
      gridItem.SetAttribute(siUIGridColumnWidths, "20:100:80:60:250");

      in_layout.AddRow();
         xsiItem = in_layout.AddButton("NewData",    "New Data");
         xsiItem = in_layout.AddButton("DeleteData", "Delete Data");
      in_layout.EndRow();

   in_layout.EndGroup();     
}


/////////////////////////////////
//   VISIBILITY
/////////////////////////////////

function arnold_visibility_Define(io_Context)
{
   var prop = io_Context.Source;	

   prop.AddParameter2("camera",                siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("cast_shadow",           siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("diffuse_reflection",    siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("specular_reflection",   siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("diffuse_transmission",  siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("specular_transmission", siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("volume",                siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
}

function arnold_visibility_DefineLayout(io_Context)
{
   var layout = io_Context.Source;
   layout.Clear();

   layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Visibility");    

   layout.AddItem("camera",                "Camera");
   layout.AddItem("cast_shadow",           "Shadow");
   layout.AddItem("diffuse_reflection",    "Diffuse Reflection");
   layout.AddItem("specular_reflection",   "Specular Reflection");
   layout.AddItem("diffuse_transmission",  "Diffuse Transmission");
   layout.AddItem("specular_transmission", "Specular Transmission");
   layout.AddItem("volume",                "Volume");
}


/////////////////////////////////
//   Matte
/////////////////////////////////
function arnold_matte_Define(io_Context)
{
   var prop = io_Context.Source;	
   prop.AddParameter2("on", siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
}

function arnold_matte_DefineLayout(io_Context)
{
   var layout = io_Context.Source;
   layout.Clear();

   layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Matte");    
   item = layout.AddItem("on", "On");
}

/////////////////////////////////
//   SIDEDNESS
/////////////////////////////////

function arnold_sidedness_Define(io_Context)
{
   var prop = io_Context.Source;	

   prop.AddParameter2("camera",                siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("cast_shadow",           siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("diffuse_reflection",    siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("specular_reflection",   siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("diffuse_transmission",  siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("specular_transmission", siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("volume",                siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
}

function arnold_sidedness_DefineLayout(io_Context)
{
   var layout = io_Context.Source;
   layout.Clear();

   layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Sidedness");       
   
   layout.AddItem("camera",                "Camera");
   layout.AddItem("cast_shadow",           "Shadow");
   layout.AddItem("diffuse_reflection",    "Diffuse Reflection");
   layout.AddItem("specular_reflection",   "Specular Reflection");
   layout.AddItem("diffuse_transmission",  "Diffuse Transmission");
   layout.AddItem("specular_transmission", "Specular Transmission");
   layout.AddItem("volume",                "Volume");
}


/////////////////////////////////
//   ARNOLD PARAMETERS LAYOUT
/////////////////////////////////

function arnold_parameters_DefineLayout(io_Context)
{
   var xsiLayout = io_Context.Source;
   xsiLayout.Clear();

   xsiLayout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Arnold+Parameters");

   xsiLayout.AddGroup("Shape", true, 50);
      xsiLayout.AddItem("opaque",               "Opaque");
      xsiLayout.AddItem("invert_normals",       "Invert Normals");
      xsiLayout.AddItem("self_shadows",         "Self Shadows");
      xsiLayout.AddItem("receive_shadows",      "Receive Shadows");
      xsiLayout.AddItem("export_pref",          "Export Bind Pose (Pref)");
      xsiLayout.AddItem("subdiv_smooth_derivs", "Smooth Subdiv Tangents");
   xsiLayout.EndGroup();

   xsiLayout.AddGroup("SSS Set Name", true, 50);
      item = xsiLayout.AddItem("sss_setname", "");
      item.SetAttribute(siUINoLabel, true);
   xsiLayout.EndGroup();

   xsiLayout.AddGroup("Trace Sets", true, 50);
      item = xsiLayout.AddItem("trace_sets", "");
      item.SetAttribute(siUINoLabel, true);
   xsiLayout.EndGroup();

   xsiLayout.AddGroup("Motion Blur", true);
      xsiLayout.AddItem("override_motion_step", "Override Keys");

      xsiLayout.AddRow();
         xsiLayout.AddItem("motion_transform", "Transformation");
         item = xsiLayout.AddItem("motion_step_transform", "Keys");
         item.SetAttribute(siUINoSlider,true);
      xsiLayout.EndRow();

      xsiLayout.AddRow();
         xsiLayout.AddItem("motion_deform", "Deformation");
         item = xsiLayout.AddItem("motion_step_deform", "Keys");
         item.SetAttribute(siUINoSlider,true);
      xsiLayout.EndRow();

      xsiLayout.AddItem("use_pointvelocity", "Use ICE PointVelocity");
   xsiLayout.EndGroup();

   // new (2.2) displacement parameters at object's level
   xsiLayout.AddGroup("Displacement", true);
      item = xsiLayout.AddItem("disp_height", "Height");
      item.setAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = xsiLayout.AddItem("disp_padding", "Bounds Padding");
      item.setAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = xsiLayout.AddItem("disp_zero_value", "Zero Value");
      item.setAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = xsiLayout.AddItem("disp_autobump", "AutoBump");
   xsiLayout.EndGroup();
   xsiLayout.AddGroup("Subdivision", true);
      item = xsiLayout.AddItem("subdiv_iterations", "Additional Iterations");
      item.setAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = xsiLayout.AddItem("adaptive_subdivision", "Adaptive Subdivision");
      item = xsiLayout.AddItem("subdiv_adaptive_error", "Adaptive Error");
      item.setAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = xsiLayout.AddEnumControl( "subdiv_adaptive_metric", Array("Auto", "auto", 
                                                                "Edge Length", "edge_length", 
                                                                "Flatness", "flatness"), 
                                                                "Adaptive Metric", siControlCombo);
      item.setAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = xsiLayout.AddEnumControl( "subdiv_adaptive_space", Array("Raster", "raster", 
                                                                "Object", "object"), 
                                                                "Adaptive Space", siControlCombo);
      item.setAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
   xsiLayout.EndGroup();

   xsiLayout.AddGroup("Volume", true);
      item = xsiLayout.AddItem("step_size", "Step Size");
      item.setAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = xsiLayout.AddItem("volume_padding", "Padding");
      item.setAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
   xsiLayout.EndGroup();

   // Perhaps these parameters dont exist in the property
   try 
   {
      xsiLayout.AddGroup("Arnold Curves Parameters", true, 50);
         xsiLayout.AddItem("min_pixel_width", "Min. Pixel Width");
         xsiLayout.AddEnumControl( "mode", Array( "Ribbon", "ribbon", "Thick", "thick", "Oriented Ribbon (ICE Strands)", "oriented"), "Mode", siControlCombo);
      xsiLayout.EndGroup();
   }
   catch(exception)
   {
   }
}


/////////////////////////////////
//   TEXTURE OPTIONS LAYOUT
/////////////////////////////////
function arnold_texture_options_Define(io_Context)
{
   var prop = io_Context.Source;
   prop.AddParameter2("filter",      siInt4, 3, 0, 3, 0, 3, 0, siPersistable|siAnimatable);
   prop.AddParameter2("mipmap_bias", siInt4, 0, -100, 100, -3, 10, 0, siPersistable|siAnimatable);
   prop.AddParameter2("swap_uv",     siBool, 0, 0, 1, 0, 5, 0, siPersistable|siAnimatable);	
   prop.AddParameter2("u_wrap",      siInt4, 0, 0, 5, 0, 5, 0, siPersistable|siAnimatable);
   prop.AddParameter2("v_wrap",      siInt4, 0, 0, 5, 0, 5, 0, siPersistable|siAnimatable);
}

function arnold_texture_options_DefineLayout(io_Context)
{
   var xsiLayout = io_Context.Source;
   xsiLayout.Clear();

   xsiLayout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Texture+Options");

   xsiLayout.AddGroup("Texture Options", true, 50);
      xsiLayout.AddEnumControl("filter", Array("Closest", 0, "Bilinear" , 1, "Bicubic", 2, "Smart Bicubic", 3), "Filter", siControlCombo);
      xsiLayout.AddItem("mipmap_bias", "Mipmap Bias");
      xsiLayout.AddItem("swap_uv", "Swap UV");
      xsiLayout.AddEnumControl("u_wrap", Array("Don't Override", 0, "Periodic", 1, "Black" , 2, "Clamp", 3, "Mirror", 4, "File", 5), "U Wrap", siControlCombo);
      xsiLayout.AddEnumControl("v_wrap", Array("Don't Override", 0, "Periodic", 1, "Black" , 2, "Clamp", 3, "Mirror", 4, "File", 5), "V Wrap", siControlCombo);
   xsiLayout.EndGroup();
}


/////////////////////////////////
//   CAMERA OPTIONS
/////////////////////////////////
function arnold_camera_options_Define(io_Context)
{
   var prop = io_Context.Source;

   prop.AddParameter2("camera_type", siString, "persp_camera", siPersistable|siAnimatable);

   prop.AddParameter2("exposure", siFloat, 0, -10000, 10000, -5, 5, 0, siPersistable|siAnimatable);

   // cylindrical
   prop.AddParameter2("cyl_horizontal_fov", siFloat, 60, 0.001, 360, 0.001, 360, 0, siPersistable|siAnimatable);
   prop.AddParameter2("cyl_vertical_fov", siFloat, 90, 0.001, 180, 0.001, 180, 0, siPersistable|siAnimatable);
   prop.AddParameter2("cyl_projective", siInt4, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);  
   // fisheye
   prop.AddParameter2("fisheye_autocrop", siBool, 0, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   // vr
   prop.AddParameter2("vr_mode", siString, "side_by_side", siPersistable|siAnimatable);
   prop.AddParameter2("vr_projection", siString, "latlong", siPersistable|siAnimatable);
   prop.AddParameter2("vr_eye_separation", siFloat, 0.65, 0, 1000000, 0, 10, 0, siPersistable|siAnimatable);
   prop.AddParameter2("vr_eye_to_neck", siFloat, 0.0, 0, 1000000, 0, 10, 0, siPersistable|siAnimatable);
   prop.AddParameter2("vr_top_merge_mode", siString, "cosine", siPersistable|siAnimatable);
   prop.AddParameter2("vr_top_merge_angle", siFloat, 90.0, 0, 180, 0, 180, 0, siPersistable|siAnimatable);
   prop.AddParameter2("vr_bottom_merge_mode", siString, "cosine", siPersistable|siAnimatable);
   prop.AddParameter2("vr_bottom_merge_angle", siFloat, 90.0, 0, 180, 0, 180, 0, siPersistable|siAnimatable);
   // not exposing merge_shader, as we can't connect a shader to the property
   // prop.AddParameter2("vr_merge_shader", siFloat, 0.0, 0, 1, 0, 1, 0, siPersistable|siAnimatable);

   prop.AddParameter2("override_camera_shutter", siBool, 0, 0, 1, 0, 1, 0, siPersistable|siAnimatable);  
   prop.AddParameter2("shutter_start", siFloat, -0.25, -1000, 1000, -0.5, 0.5, 0, siPersistable|siAnimatable);
   prop.AddParameter2("shutter_end",   siFloat, 0.25, -1000, 1000, -0.5, 0.5, 0, siPersistable|siAnimatable);
   prop.AddParameter2("shutter_type", siString, "box", siPersistable|siAnimatable);

   var curve = prop.AddFCurveParameter("shutter_curve");
   var value = curve.Value;
   value.SetKeys(new Array(0, 0, 0.5, 1, 1, 0));
   value.Interpolation = siLinearInterpolation;

   prop.AddParameter2("rolling_shutter", siString, "off", siPersistable|siAnimatable);
   prop.AddParameter2("rolling_shutter_duration", siFloat, 0, 0, 1, 0, 1, 0, siPersistable|siAnimatable);

   prop.AddParameter2("enable_depth_of_field", siBool, 0, 0, 1, 0, 1, 0, siPersistable|siAnimatable);  
   prop.AddParameter2("use_interest_distance", siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);  
   prop.AddParameter2("focus_distance", siFloat, 100, 0.001, 1000000, 0.001, 1000, 0, siPersistable|siAnimatable);

   prop.AddParameter2("aperture_size", siFloat, 0.1 ,0, 999999, 0, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("aperture_aspect_ratio", siFloat, 1, 0, 999999, 0, 50, 0, siPersistable|siAnimatable);
   prop.AddParameter2("use_polygonal_aperture", siBool, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);  

   prop.AddParameter2("aperture_blades", siInt4, 5 ,3, 100, 3, 20, 0, siPersistable|siAnimatable);
   prop.AddParameter2("aperture_blade_curvature", siFloat, 0, -100, 100, -5, 1, 0, siPersistable|siAnimatable);
   prop.AddParameter2("aperture_rotation", siFloat, 0 ,0, 100, 0, 50, 0, siPersistable|siAnimatable);

   prop.AddParameter2("enable_filtermap", siBool, 0, 0, 1, 0, 1, 0, siPersistable|siAnimatable);  
   prop.AddParameter2("filtermap", siString, "", siPersistable|siAnimatable);

   prop.AddParameter2("enable_uv_remap", siBool, 0, 0, 1, 0, 1, 0, siPersistable|siAnimatable);  
   prop.AddParameter2("uv_remap", siString, "", siPersistable|siAnimatable);
}

function arnold_camera_options_DefineLayout(io_Context)
{
   var layout = io_Context.Source;
   layout.Clear();
   var item;

   layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Camera+Options");

   layout.AddTab("Camera Type");

      item = layout.AddEnumControl("camera_type", Array("Perspective", "persp_camera", 
                                                        "Spherical" ,  "spherical_camera", 
                                                        "Cylindrical", "cyl_camera", 
                                                        "Fisheye",     "fisheye_camera",
                                                        "VR",          "vr_camera",
                                                        "Custom (lens shader)", "custom_camera"), "Camera Type", siControlCombo);
      item.SetAttribute(siUINoLabel, true);

   layout.AddGroup("Exposure");
      item = layout.AddItem("exposure", "")
      item.SetAttribute(siUINoLabel, true);
   layout.EndGroup();

   layout.AddGroup("Cylindrical");
      item = layout.AddItem("cyl_horizontal_fov", "Horizontal FOV");
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = layout.AddItem("cyl_vertical_fov",   "Vertical FOV");
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = layout.AddEnumControl("cyl_projective", Array("Linear (Lat Long)",     0, 
                                                           "Standard (Projective)", 1), "Projective", siControlCombo);
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
   layout.EndGroup();

   layout.AddGroup("Fisheye");
      item = layout.AddItem("fisheye_autocrop", "Autocrop");
   layout.EndGroup();

   layout.AddGroup("VR");
      item = layout.AddEnumControl("vr_mode", Array("Side by Side", "side_by_side", 
                                                    "Over Under", "over_under",
                                                    "Left Eye", "left_eye",
                                                    "Right Eye", "right_eye"), "Mode", siControlCombo);
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = layout.AddEnumControl("vr_projection", Array("LatLong", "latlong", 
                                                          "Cubemap 6x1", "cubemap_6x1",
                                                          "Cubemap 3x2", "cubemap_3x2"), "Projection", siControlCombo);
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = layout.AddItem("vr_eye_separation", "Eye Separation");
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = layout.AddItem("vr_eye_to_neck", "Eye to Neck");
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = layout.AddEnumControl("vr_top_merge_mode", Array("None", "none", 
                                                              "Cosine", "cosine"), "Top Merge Mode", siControlCombo);
                                                              // "Shader", "shader"), "Top Merge Mode", siControlCombo);
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = layout.AddItem("vr_top_merge_angle", "Top Merge Angle");
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = layout.AddEnumControl("vr_bottom_merge_mode", Array("None", "none", 
                                                                 "Cosine", "cosine"), "Bottom Merge Mode", siControlCombo);
                                                                 // "Shader", "shader"), "Bottom Merge Mode", siControlCombo);
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      item = layout.AddItem("vr_bottom_merge_angle", "Bottom Merge Angle");
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);
      // item = layout.AddItem("vr_merge_shader", "Merge Shader");
      // item.SetAttribute(siUILabelMinPixels, 130);
      // item.SetAttribute(siUILabelPercentage, 50);
   layout.EndGroup();


   layout.AddGroup("Depth of Field");
      item = layout.AddItem("enable_depth_of_field", "Enable");

      layout.AddGroup("Focus Distance");
         item = layout.AddItem("use_interest_distance", "Focus At Interest Point");
         item = layout.AddItem("focus_distance", "Distance");
         item.SetAttribute(siUILabelMinPixels, 130);
         item.SetAttribute(siUILabelPercentage, 50);
      layout.EndGroup();

      layout.AddGroup("Aperture");
         item = layout.AddItem("aperture_size", "Size");
         item.SetAttribute(siUILabelMinPixels, 130);
         item.SetAttribute(siUILabelPercentage, 50);
         item = layout.AddItem("aperture_aspect_ratio", "Aspect Ratio");
         item.SetAttribute(siUILabelMinPixels, 130);
         item.SetAttribute(siUILabelPercentage, 50);
         item = layout.AddItem("use_polygonal_aperture", "Polygonal Aperture");
         item = layout.AddItem("aperture_blades", "Blades");
         item.SetAttribute(siUILabelMinPixels, 130);
         item.SetAttribute(siUILabelPercentage, 50);
         item = layout.AddItem("aperture_blade_curvature", "Blade Curvature");
         item.SetAttribute(siUILabelMinPixels, 130);
         item.SetAttribute(siUILabelPercentage, 50);
         item = layout.AddItem("aperture_rotation", "Rotation");
         item.SetAttribute(siUILabelMinPixels, 130);
         item.SetAttribute(siUILabelPercentage, 50);
      layout.EndGroup();
   layout.EndGroup();

   layout.AddTab("Motion Blur");
      layout.AddGroup("Shutter Start/End");
         item = layout.AddItem("override_camera_shutter", "Override Geometry Shutter");
         item = layout.AddItem("shutter_start", "Shutter Start");
         item.SetAttribute(siUILabelMinPixels, 130);
         item.SetAttribute(siUILabelPercentage, 50);
         item = layout.AddItem("shutter_end", "Shutter End");
         item.SetAttribute(siUILabelMinPixels, 130);
         item.SetAttribute(siUILabelPercentage, 50);
      layout.EndGroup();

      item = layout.AddEnumControl("shutter_type", Array("Box",      "box", 
                                                         "Triangle", "triangle", 
                                                         "Curve",    "curve"), "Shutter Type", siControlCombo);
      item.SetAttribute(siUILabelMinPixels, 130);
      item.SetAttribute(siUILabelPercentage, 50);

      layout.AddGroup("Shutter Curve");
         item = layout.AddItem("shutter_curve", "");
         item.SetAttribute(siUINoLabel, true);
      layout.EndGroup();

      layout.AddGroup("Rolling Shutter");
         item = layout.AddEnumControl("rolling_shutter", Array("Off", "off", "Top" , "top", "Bottom", "bottom", "Left", "left", "Right", "right"), "Rolling Shutter", siControlCombo);
         item.SetAttribute(siUINoLabel, true);

         item = layout.AddItem("rolling_shutter_duration", "Duration");
      layout.EndGroup();

   layout.AddTab("Maps");
      layout.AddGroup("Filter Map");
         item = layout.AddItem("enable_filtermap", "Enable");
         item = layout.AddItem("filtermap", "", siControlImageClip ) ;
         item.SetAttribute(siUINoLabel, true);

         item.SetAttribute(siUIImageFile, true);
         item.SetAttribute(siUIOpenFile, true);
      layout.EndGroup();

      layout.AddGroup("Perspective UV Remap");
         item = layout.AddItem("enable_uv_remap", "Enable");
         item = layout.AddItem("uv_remap", "", siControlImageClip ) ;
         item.SetAttribute(siUINoLabel, true);

         item.SetAttribute(siUIImageFile, true);
         item.SetAttribute(siUIOpenFile, true);
      layout.EndGroup();
}

function arnold_camera_options_OnInit() 
{
   arnold_camera_options_enable_filtermap_OnChanged();
   arnold_camera_options_shutter_type_OnChanged();
   arnold_camera_options_camera_type_OnChanged();
   arnold_camera_options_override_camera_shutter_OnChanged();
   arnold_camera_options_rolling_shutter_OnChanged();
   dof_logic(PPG);
}

function arnold_camera_options_camera_type_OnChanged()
{
   var cameraType = PPG.camera_type.Value;

   PPG.cyl_horizontal_fov.Enable(cameraType == "cyl_camera");
   PPG.cyl_vertical_fov.Enable(cameraType == "cyl_camera");
   PPG.cyl_projective.Enable(cameraType == "cyl_camera");

   PPG.fisheye_autocrop.Enable(cameraType == "fisheye_camera");

   PPG.vr_mode.Enable(cameraType == "vr_camera");
   PPG.vr_projection.Enable(cameraType == "vr_camera");
   PPG.vr_eye_separation.Enable(cameraType == "vr_camera");
   PPG.vr_eye_to_neck.Enable(cameraType == "vr_camera");
   PPG.vr_top_merge_mode.Enable(cameraType == "vr_camera");
   PPG.vr_top_merge_angle.Enable(cameraType == "vr_camera");
   PPG.vr_bottom_merge_mode.Enable(cameraType == "vr_camera");
   PPG.vr_bottom_merge_angle.Enable(cameraType == "vr_camera");
   // PPG.vr_merge_shader.Enable(cameraType == "vr_camera");

   PPG.enable_uv_remap.Enable(cameraType == "persp_camera");
   arnold_camera_options_enable_uv_remap_OnChanged();

   arnold_camera_options_vr_top_merge_mode_OnChanged();
   arnold_camera_options_vr_bottom_merge_mode_OnChanged();

   dof_logic();
}

function arnold_camera_options_shutter_type_OnChanged() 
{
   PPG.shutter_curve.Enable(PPG.shutter_type.Value == "curve");
}

function arnold_camera_options_override_camera_shutter_OnChanged()
{
   PPG.shutter_start.Enable(PPG.override_camera_shutter.Value)
   PPG.shutter_end.Enable(PPG.override_camera_shutter.Value)
}

function arnold_camera_options_rolling_shutter_OnChanged() 
{
   PPG.rolling_shutter_duration.Enable(PPG.rolling_shutter.Value != "off");
}

function dof_logic()
{
   var cameraType = PPG.camera_type.Value;
   var hasDOF = cameraType == "persp_camera" || cameraType == "fisheye_camera";

   var enableDOF = PPG.enable_depth_of_field.Value;
   var focusAtDistance = PPG.use_interest_distance.Value;
   var polygonalAperture = PPG.use_polygonal_aperture.Value;

   PPG.enable_depth_of_field.Enable(hasDOF);

   enableDOF = enableDOF && hasDOF; 

   PPG.use_interest_distance.Enable(enableDOF);
   PPG.focus_distance.Enable(enableDOF && !focusAtDistance);
   PPG.aperture_size.Enable(enableDOF);
   PPG.aperture_aspect_ratio.Enable(enableDOF);
   PPG.use_polygonal_aperture.Enable(enableDOF);
   PPG.aperture_blades.Enable(enableDOF && polygonalAperture);
   PPG.aperture_blade_curvature.Enable(enableDOF && polygonalAperture);
   PPG.aperture_rotation.Enable(enableDOF && polygonalAperture);
}

function arnold_camera_options_enable_depth_of_field_OnChanged()
{
   dof_logic();
}

function arnold_camera_options_use_interest_distance_OnChanged()
{
   dof_logic();
}

function arnold_camera_options_use_polygonal_aperture_OnChanged()
{
   dof_logic();
}

function arnold_camera_options_enable_filtermap_OnChanged() 
{
   PPG.filtermap.Enable(PPG.enable_filtermap.Value);
}

function arnold_camera_options_enable_uv_remap_OnChanged() 
{
   PPG.uv_remap.Enable(PPG.enable_uv_remap.Value);
}

function arnold_camera_options_vr_top_merge_mode_OnChanged()
{
   var camera_type = PPG.camera_type.Value;
   var top_merge_mode    = PPG.vr_top_merge_mode.Value;
   var bottom_merge_mode = PPG.vr_bottom_merge_mode.Value;
   var enable_angle = camera_type == "vr_camera" && top_merge_mode == "cosine";
   PPG.vr_top_merge_angle.Enable(enable_angle);

   // var enable_merge_shader = camera_type == "vr_camera" && (top_merge_mode == "shader" || bottom_merge_mode == "shader");
   // PPG.vr_merge_shader.Enable(enable_merge_shader);
}

function arnold_camera_options_vr_bottom_merge_mode_OnChanged()
{
   var camera_type = PPG.camera_type.Value;
   var top_merge_mode    = PPG.vr_top_merge_mode.Value;
   var bottom_merge_mode = PPG.vr_bottom_merge_mode.Value;
   var enable_angle = camera_type == "vr_camera" && bottom_merge_mode == "cosine";
   PPG.vr_bottom_merge_angle.Enable(enable_angle);

   // var enable_merge_shader = camera_type == "vr_camera" && (top_merge_mode == "shader" || bottom_merge_mode == "shader");
   // PPG.vr_merge_shader.Enable(enable_merge_shader);
}

/////////////////////////////////
//   EVENTS
/////////////////////////////////

function arnold_parameters_OnInit() 
{
   // is this a scene saved with a previous version of the property?
   var oCustomProperty = PPG.Inspected.Item(0);
   if (oCustomProperty.Parameters("override_motion_step") != null)
      arnold_parameters_override_motion_step_OnChanged();
   
   if (oCustomProperty.Parameters("adaptive_subdivision") != null)
      arnold_parameters_adaptive_subdivision_OnChanged();

   // since we added OnInit, let's also manage the case when the property has the hair section or not
   if (oCustomProperty.Parameters("min_pixel_width") != null)
      arnold_parameters_mode_OnChanged();

   if (oCustomProperty.Parameters("step_size") != null)
      arnold_parameters_step_size_OnChanged();
}


function arnold_parameters_override_motion_step_OnChanged() 
{
   var cp = PPG.Inspected.Item(0);

   var overrideSteps = PPG.override_motion_step.Value;

   var transform = true;
   if (cp.Parameters("motion_transform") != null)
      transform = PPG.motion_transform.Value

   var deform = true;
   if (cp.Parameters("motion_deform") != null)
      deform = PPG.motion_deform.Value
   
   PPG.motion_step_transform.Enable(overrideSteps && transform);
   PPG.motion_step_deform.Enable(overrideSteps && deform);
}


function arnold_parameters_motion_transform_OnChanged() 
{
   arnold_parameters_override_motion_step_OnChanged();
}


function arnold_parameters_motion_deform_OnChanged() 
{
   arnold_parameters_override_motion_step_OnChanged();
}


function arnold_parameters_adaptive_subdivision_OnChanged() 
{
	var oCustomProperty = PPG.Inspected.Item(0);
   var adaptive_on = PPG.adaptive_subdivision.Value;
   if (oCustomProperty.Parameters("subdiv_adaptive_metric") != null)
		PPG.subdiv_adaptive_metric.Enable(adaptive_on);
   PPG.subdiv_adaptive_error.Enable(adaptive_on);
   if (oCustomProperty.Parameters("subdiv_adaptive_space") != null)
      PPG.subdiv_adaptive_space.Enable(adaptive_on);
}


function arnold_parameters_mode_OnChanged() 
{
   if (PPG.mode.Value=="thick")
      PPG.min_pixel_width.Value = 0;
      
   PPG.min_pixel_width.Enable(PPG.mode.Value=="ribbon" || PPG.mode.Value=="oriented");
}


function arnold_parameters_step_size_OnChanged() 
{
   var oCustomProperty = PPG.Inspected.Item(0);
   if (oCustomProperty.Parameters("volume_padding") != null)
      PPG.volume_padding.Enable(PPG.step_size.Value > 0);
}


/////////////////////////////////
//   Procedural 
/////////////////////////////////


function arnold_procedural_Define(io_Context)
{
   var customProperty = io_Context.Source;	
   var p = customProperty.AddParameter2("filename", siString, "","","","","", 0, siPersistable|siAnimatable);
   p = customProperty.AddParameter2("overrideFrame", siBool, 0, 0, 1, 0, 5, 0, siPersistable|siAnimatable);	
   p = customProperty.AddParameter2("frame", siInt4, 1, -1000000, 1000000, 1, 100, siClassifAppearance, siPersistable|siAnimatable);
   p.enable(false);

   // new (2.2) viewer parameters at object's level
   p = customProperty.AddParameter2("mode", siInt4, 0, 0, 2, 0, 2, siClassifAppearance, siPersistable|siAnimatable);
   p = customProperty.AddParameter2("randomColors", siBool, 0, 0, 1, 0, 1, 0, siPersistable|siAnimatable);	
   p = customProperty.AddParameter2("seed", siInt4, 50, 0, 100, 0, 100, siClassifAppearance, siPersistable|siAnimatable);
   p.enable(false);
   p = customProperty.AddParameter2("colorR", siDouble, 1.0, 0, 1, 0, 1,siClassifUnknown,siPersistable | siKeyable);
   p = customProperty.AddParameter2("colorG", siDouble, 0.0, 0, 1, 0, 1,siClassifUnknown,siPersistable | siKeyable);
   p = customProperty.AddParameter2("colorB", siDouble, 0.0, 0, 1, 0, 1,siClassifUnknown,siPersistable | siKeyable);
   p = customProperty.AddParameter2("size", siInt4, 1, 0, 100, 0, 5, siClassifAppearance, siPersistable|siAnimatable);
   p = customProperty.AddParameter2("pointsDisplayPcg", siFloat, 1, 0, 1, 0, 1, 0, siPersistable|siAnimatable);
   p.enable(false); // because mode default is box

   // #1315 custom user parameters on procedurals
   AddUserData(customProperty, true);
}


function arnold_procedural_DefineLayout(io_Context)
{
   var xsiLayout = io_Context.Source;
   xsiLayout.Clear();

   xsiLayout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Stand-ins");          
   
   var xsiItem = xsiLayout.AddItem("filename", "Filename", siControlFilePath);
	
   var filter = "Arnold Scene Source (*.ass *.ass.gz)|*.ass:*.ass.gz";
   filter+= "|Wavefront Obj (*.obj *.obj.gz)|*.obj:*.obj.gz";
   filter+= "|Stanford Geometry (*.ply)|*.ply";
   filter += "|All Files (*.*)|*.*||";

   xsiItem.setAttribute(siUIFileFilter, filter);
   xsiItem.setAttribute(siUIFileMustExist, true);
   xsiItem.setAttribute(siUIOpenFile, true);

   xsiLayout.AddRow()
      xsiItem = xsiLayout.AddItem("overrideFrame", "Override Frame");
      xsiItem.WidthPercentage = 40;
      xsiItem = xsiLayout.AddItem("frame", "Frame");
      xsiItem.WidthPercentage = 60;
   xsiLayout.EndRow()

   UserDataLayout(xsiLayout, true);

   xsiLayout.AddTab("SITOA_Viewer");
      xsiLayout.AddEnumControl("mode", Array("Box", 0, "Points" , 1, "Wireframe", 2), "Mode", siControlCombo);
      xsiLayout.AddGroup("Colors", true, 0);
         xsiLayout.AddRow();
            xsiItem = xsiLayout.AddItem("randomColors", "Random Colors");
            xsiItem = xsiLayout.AddItem("seed", "Seed");
         xsiLayout.EndRow();
         xsiItem = xsiLayout.AddColor("colorR", "Color", false);
      xsiLayout.EndGroup();
      xsiLayout.AddGroup("Options", true, 0);
         xsiItem = xsiLayout.AddItem("size", "Line/Point Size");
         xsiItem.setAttribute(siUILabelMinPixels, 130);
         xsiItem.SetAttribute(siUILabelPercentage, 50);
         xsiItem = xsiLayout.AddItem("pointsDisplayPcg", "Points Display %");
         xsiItem.setAttribute(siUILabelMinPixels, 130);
         xsiItem.SetAttribute(siUILabelPercentage, 50);
      xsiLayout.EndGroup();
}


function arnold_procedural_OnInit(io_Context)
{
   var oCustomProperty = PPG.Inspected.Item(0);
   arnold_procedural_muteUserData_OnChanged();
}


// Replace a browsed path with a [Frame]d one
// For instance, turn
// "C:\\temp\\torus_Archive.[1..100;3].ass.gz";
// into
// "C:\\temp\\torus_Archive.[Frame #3].ass.gz";
function ReplacePathWithToken(in_path)
{
   var index = in_path.lastIndexOf(".ass");
   var extension = in_path.slice(index);
   // logMessage("extension = " + extension);
   var closingBracketIndex = index-1;
   var c = in_path.charAt(closingBracketIndex);
   if (c != ']')
      return in_path;

   var openingBracketIndex = in_path.lastIndexOf("[");
   if (openingBracketIndex == -1)
      return in_path;
   openingBracketIndex++;
   
   framesString = in_path.substring(openingBracketIndex, closingBracketIndex);
   // framesString is whatever stands between the [..]
   // logMessage(framesString);
	
   // "Frame" already there, quit
   if (framesString.indexOf("Frame") != -1)
      return in_path;
   // "Time" there, quit
   if (framesString.indexOf("Time") != -1)
      return in_path;

   var padding = 1;
   // a possible case is framesString = "1..10;3"
   // check if there is a ';'
   c = framesString.charAt(framesString.length-2);
   if (c == ';')
   {
      // get as padding the integer after ';'
      c = framesString.charAt(framesString.length-1);
      padding = parseInt(c);
      if (padding < 1 || padding > 5)
         padding = 1;
   }
   else
   {
      // another is framesString = "001..010"
      // count the number of 0s for the first number
      for (var i=0; i<framesString.length; i++)
      {
         c = framesString.charAt(i);
         if (c !== '0')
            break;
         padding++;
      }
   }
	
   // logMessage("padding = " + padding);

   var result = in_path.substring(0, openingBracketIndex);
   result+= "Frame";

   if (padding > 1)
      result+= " #" + padding;
	
   result+= "]" + extension;
   // logMessage("result = " + result);
   return result;
}

function GetFileExtension(in_path)
{
   var index = in_path.lastIndexOf(".") + 1;
   var extension = in_path.slice(index);
   return extension;
}


function arnold_procedural_path_OnChanged() 
{
   var s = ReplacePathWithToken(PPG.path.Value);
   PPG.path.Value = s;
}


function arnold_procedural_overrideFrame_OnChanged() 
{
   PPG.frame.Enable(PPG.overrideFrame.Value);
}


function arnold_procedural_mode_OnChanged() 
{
   PPG.pointsDisplayPcg.Enable(PPG.mode.Value == 1);
}


function arnold_procedural_randomColors_OnChanged() 
{
   PPG.seed.Enable(PPG.randomColors.Value);
   PPG.colorR.Enable(!PPG.randomColors.Value);
}

// #1315 custom user parameters on procedurals
function newUserData(PPG)
{
    var customProperty = PPG.Inspected.Item(0);    
    var grid = customProperty.userDataGrid.Value;
    
    grid.BeginEdit();
    grid.RowCount = grid.RowCount + 1;
    // default cell values
    grid.SetCell(1, grid.RowCount - 1, 0); // single structure
    grid.SetCell(2, grid.RowCount - 1, 0); // integer
    grid.EndEdit();
}


function deleteUserData(PPG)
{
   var grid = PPG.userDataGrid.Value;
   grid.BeginEdit();
   var widget = grid.GridWidget;
    
   // shift the rows upwards to overwrite the selected rows
   var writePos = 0;
   for (var readPos = 0; readPos<grid.RowCount; readPos++)
   {
      if (!widget.IsRowSelected(readPos))
      {
         if (readPos != writePos)
            grid.SetRowValues(writePos, grid.GetRowValues(readPos));

         writePos++;
      }
   }
   // remove the last row
   grid.RowCount = writePos;
   grid.EndEdit();   
}


function arnold_procedural_NewData_OnClicked()
{
   newUserData(PPG);
   arnold_procedural_muteUserData_OnChanged()
}


function arnold_procedural_DeleteData_OnClicked()
{
   deleteUserData(PPG);
   PPG.Refresh();
}


// Turn the grid green/red (enable/disable)
function arnold_procedural_muteUserData_OnChanged()
{
   var enbl = !PPG.muteUserData.value;
   PPG.resolveUserDataTokens.Enable(enbl);

   var grid = PPG.userDataGrid.Value;
   if (grid.RowCount < 1)
      return;

   var color = grid.GetRowBackgroundColor(0);
   
	if (enbl)
	{
      color.Red   = 220;
      color.Green = 255;
	}
   else
	{
      color.Red   = 255;
      color.Green = 220;
	}
   color.Blue  = 220;

	for (var readPos = 0; readPos < grid.RowCount; readPos++)
		grid.SetRowBackgroundColor(readPos, color);

   PPG.Refresh();
}


function arnold_user_options_Define(io_Context)
{
   var customProperty = io_Context.Source;	

   var p = customProperty.AddParameter2("user_options", siString, "","","","","", 0, siPersistable|siAnimatable);
   p = customProperty.AddParameter2("mute", siBool, 0, 0, 1, 0, 5, 0, siPersistable|siAnimatable);	
   p = customProperty.AddParameter2("resolve_tokens", siBool, 0, 0, 1, 0, 5, 0, siPersistable|siAnimatable);	

   AddUserData(customProperty, false);
}


function arnold_user_options_DefineLayout(io_Context)
{
   var xsiLayout = io_Context.Source;
   xsiLayout.Clear();

   xsiLayout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/User+Options");          

   xsiLayout.AddRow();
      item = xsiLayout.AddItem("mute", "Mute");
      item = xsiLayout.AddItem("resolve_tokens", "Resolve Tokens");
   xsiLayout.EndRow();

   item = xsiLayout.AddItem("user_options", "Options");
   item.setAttribute(siUILabelMinPixels, 80);
   item.SetAttribute(siUILabelPercentage, 30);

   UserDataLayout(xsiLayout, false);
}


function arnold_user_options_mute_OnChanged()
{
   var oCustomProperty = PPG.Inspected.Item(0);
   var enbl = !PPG.mute.value;

   PPG.user_options.Enable(enbl);

   if (oCustomProperty.Parameters("resolve_tokens") != null)
      PPG.resolve_tokens.Enable(enbl);

   var grid = PPG.userDataGrid.Value;
   if (grid.RowCount < 1)
      return;

   var color = grid.GetRowBackgroundColor(0);
   
   if (enbl)
   {
      color.Red   = 220;
      color.Green = 255;
   }
   else
   {
      color.Red   = 255;
      color.Green = 220;
   }
   color.Blue  = 220;

   for (var readPos = 0; readPos < grid.RowCount; readPos++)
      grid.SetRowBackgroundColor(readPos, color);

   PPG.Refresh();
}


function arnold_user_options_NewData_OnClicked()
{
   newUserData(PPG);
   arnold_user_options_mute_OnChanged()
}


function arnold_user_options_DeleteData_OnClicked()
{
   deleteUserData(PPG);
   PPG.Refresh();
}


/////////////////////////////////
//   VOLUME 
/////////////////////////////////


function arnold_volume_Define(io_Context)
{
   var customProperty = io_Context.Source;   
   var p;
   p = customProperty.AddParameter2("filename",       siString, "", "", "", "", "", siClassifUnknown, siPersistable);
   p = customProperty.AddParameter2("grids",          siString, "density", "", "", "", "", siClassifUnknown, siPersistable);
   p = customProperty.AddParameter2("velocity_grids", siString, "velocity", "", "", "", "", siClassifUnknown, siPersistable);

   p = customProperty.AddParameter2("step_size", siFloat, 0, 0.0, 1000.0, 0.0, 1.0, siClassifUnknown, siPersistable | siAnimatable);
   p = customProperty.AddParameter2("step_scale", siFloat, 1.0, 0.0, 1000.0, 0.0, 2.0, siClassifUnknown, siPersistable | siAnimatable);
   p = customProperty.AddParameter2("volume_padding", siFloat, 0.0, 0.0, 100.0, 0.0, 1.0, siClassifUnknown, siPersistable | siAnimatable);

   var fps = Application.ActiveProject.Properties("Play Control").Rate.Value;
   p = customProperty.AddParameter2("velocity_fps", siFloat, fps, 1, 1000, 1, 60, siClassifUnknown, siPersistable | siAnimatable);
   p = customProperty.AddParameter2("velocity_outlier_threshold", siFloat, 0.001, 0, 1, 0, 0.1, siClassifUnknown, siPersistable | siAnimatable);
   p = customProperty.AddParameter2("velocity_scale", siFloat, 1, 0, 1000000, 0, 10, siClassifUnknown, siPersistable | siAnimatable);
   p = customProperty.AddParameter2("compress", siBool,    1, 0, 1, 0, 1, siClassifUnknown, siPersistable);

   // p = customProperty.AddParameter2("motion_start", siFloat, 0, 0, 1, 0, 1, siClassifUnknown, siPersistable | siAnimatable);
   // p = customProperty.AddParameter2("motion_end", siFloat, 1, 0, 1, 0, 1, siClassifUnknown, siPersistable | siAnimatable);

   p = customProperty.AddParameter2("grid_names", siString, "", "", "", "", "", siClassifUnknown, siPersistable | siReadOnly);
}


function arnold_volume_DefineLayout(io_Context)
{
   var layout = io_Context.Source;
   layout.Clear();

   layout.SetAttribute(siUIHelpFile, "https://support.solidangle.com/display/A5SItoAUG/Volume");          
   
   item = layout.AddItem("filename", "Filename", siControlFilePath);
   filter = "VDB file (*.vdb)|*.vdb";
   filter += "|All Files (*.*)|*.*||";

   item.setAttribute(siUIFileFilter, filter);
   item.setAttribute(siUIFileMustExist, true);
   item.setAttribute(siUIOpenFile, true);

   item = layout.AddItem("grids", "Grids");
   item = layout.AddItem("velocity_grids", "Velocity Grids");

   item = layout.AddItem("step_size", "Step Size");
   item = layout.AddItem("step_scale", "Step Scale");
   item = layout.AddItem("volume_padding", "Volume Padding");

   item = layout.AddItem("velocity_fps", "Velocity FPS");
   item = layout.AddItem("velocity_outlier_threshold", "Velocity Threshold");
   item = layout.AddItem("velocity_scale", "Velocity Scale");

   item = layout.AddItem("compress", "Compress");

   // item = layout.AddItem("motion_start", "Motion Start");
   // item = layout.AddItem("motion_end", "Motion End");

   layout.AddGroup("Available Grids");
      item = layout.AddItem("grid_names", "Available Grid");
      item.SetAttribute(siUINoLabel, true);
   layout.EndGroup();
}

function arnold_volume_OnInit(io_Context)
{
}

function arnold_volume_filename_OnChanged()
{
   var cp = PPG.Inspected.Item(0);

   var names = "";
   var path = PPG.filename.Value;
   if (path != "")
   {
      var frame = GetValue("PlayControl.Current");
      path = XSIUtils.ResolveTokenString(path, frame, false);
      names = SITOA_OpenVdbGrids(path)
   }
   if (cp.Parameters("grid_names") != null)
      PPG.grid_names.Value = names;
   if (names != "")
      LogMessage("[sitoa] Available VDB grids: " + names);
} 
