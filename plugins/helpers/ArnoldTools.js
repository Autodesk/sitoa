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
   var h = new ToolHelperObj();
   h.SetPluginInfo(in_reg, "Arnold Tools");

   in_reg.RegisterCommand("RegenerateArnoldRenderOptions", "SITOA_RegenerateRenderOptions");
   // expose the helper object to other plugins
   in_reg.RegisterCommand("SItoAToolHelper", "SItoAToolHelper");
   
   return true;
}

/////////////////////////////////
//   TOOLS
/////////////////////////////////

function SItoAToolHelper_Init(io_Context)
{
   var oCmd = io_Context.Source;
   oCmd.SetFlag(siNoLogging, true);
   oCmd.SetFlag(siSupportsKeyAssignment, false);      
   oCmd.ReturnValue = true;
   return true;   
}

function SItoAToolHelper_Execute()
{
   return new ToolHelperObj();
}

// small helper to share between all the js plugins
function ToolHelperObj()
{
   this.Type = "SItoAToolHelperObj";
   this.RendererName = "Arnold Render";

   // Get the sitoa.dll/so plugin to copy the major/minor version
   this.PreLoadRenderPlugin = function(in_thisFileOrigin, in_verbose)
   {
      // if not loaded yet, load the dll/so
      var arnoldPlugin = Application.plugins("Arnold Render");
      if (arnoldPlugin == null)
      {
         var arnoldPluginPath;
         if (XSIUtils.IsWindowsOS())
            arnoldPluginPath = XSIUtils.BuildPath(in_thisFileOrigin, "bin", "nt-x86-64", "sitoa.dll")
         else
            arnoldPluginPath = XSIUtils.BuildPath(in_thisFileOrigin, "bin", "linux", "x64", "sitoa.so")

         if (in_verbose)
            logMessage("PreLoading " + arnoldPluginPath);

         arnoldPlugin = LoadPlugin(arnoldPluginPath);
      }

      return arnoldPlugin;
   }

   // set the minor and major version, taking them from the cpp plugin
   this.SetMinorMajor = function(in_reg)
   {
      var sitoaPlugin = this.PreLoadRenderPlugin(in_reg.OriginPath, false);

      if (sitoaPlugin != null)
      {
         in_reg.Major = sitoaPlugin.Major;
         in_reg.Minor = sitoaPlugin.Minor;
      }
      else // in case the render plugin was not found, let set a dummy version couple
      {
         in_reg.Major = 10;
         in_reg.Minor = 10;
      }      
   }

   // set all the basic info (minor, major, author, name)
   this.SetPluginInfo = function(in_reg, in_name)
   {
      in_reg.Author = "SolidAngle";
      in_reg.Name = in_name;
      this.SetMinorMajor(in_reg);
   }
}


function RegenerateArnoldRenderOptions_Execute(in_arg)
{
   var plugins = Application.Plugins;
   var nplugins = plugins.count;
   var rendererFileName = "";
   
   var views = Dictionary.GetObject("Application.Views");
   var viewRegionParamNames = new Array();
   var viewRegionParamValues = new Array();
   var viewRegionNames = new Array();
      
   for (var i=nplugins-1; i>=0; i--)
   {
      var plugin = plugins(i);
      if (plugin.name == "Arnold Render")
      {
         // first, collect all the current region options
         CollectRegionParameters(views, viewRegionParamNames, viewRegionParamValues, viewRegionNames);

         // then delete them.
         logMessage("[sitoa] Deleting all region options");
         DeleteViewRegionOptions(views);

         logMessage("[sitoa] Unloading " + plugin.name);
         rendererFileName = plugin.fileName;
         Application.UnloadPlugin(rendererFileName);
         break
      }
   }

   // LogArray(viewRegionParamNames[0]);
         
   if (rendererFileName == "")
   {
      logMessage("[sitoa] Error unloading renderer, plugin not loaded!",siError);
      return false;
   }
   
   var container = ActiveProject.ActiveScene.PassContainer
   var properties = container.Properties;
   var nprops = properties.count;
   var options = [];
   var toDelete = [];
   for (var i=0; i<nprops; i++)
   {
      var prop = properties(i);
      if (prop.owners(0).fullname != container.fullname)
         continue;
      if (prop.name == "Arnold Render Options")
      {
         options[options.length] = ["Scene"];
         toDelete[toDelete.length] = prop;
         break;
      }
   }
   
   var passes = ActiveProject.ActiveScene.Passes;
   var npasses = passes.count;
   for (var i=0;i<npasses;i++)
   {
      var pass = passes(i);
      if (pass.type != "Pass")
         continue;
      var properties = pass.LocalProperties;
      var nprops = properties.count;
      for (var j=0; j<nprops; j++)
      {
         var prop = properties(j);
         if (prop.owners(0).fullname != pass.fullname)
            continue;
         if (prop.name == "Arnold Render Options")
         {
            options[options.length] = [ pass.name ];
            toDelete[toDelete.length] = prop;
            break;
         }
      }
   }
   
   // collect all of the parameter values!
   for (var i=0;i<options.length;i++)
   {
      logMessage("[sitoa] Storing values for options on " + options[0]+"...");
      var params = [];
      for (var j=0;j<toDelete[i].Parameters.count;j++)
      {
         var param = toDelete[i].Parameters(j);
         if (param.scriptname == "Name")
            continue;
         params[params.length] = [param.scriptname, param.value];
      }
      options[i][1] = params;
   }
   
   logMessage("[sitoa] Deleting all render options");
   DeleteObj(toDelete);
   
   logMessage("[sitoa] Reloading " + rendererFileName);
   Application.LoadPlugin(rendererFileName);
   SetValue("Passes.RenderOptions.Renderer", "Arnold Render");
   
   // reload scene
   SaveScene();
   var sceneFile = ActiveProject.ActiveScene.fileName.value;
   OpenScene(sceneFile,false);
   
   var container = ActiveProject.ActiveScene.PassContainer;
   var properties = container.Properties;
   var nprops = properties.count;
   for (var i=0; i<nprops; i++)
   {
      var prop = properties(i);
      if (prop.name == "Arnold Render Options")
      {
         var params = false;
         for (var j=0;j<options.length;j++)
         {
            if (options[j][0] == "Scene")
            {
               params = options[j][1];
               break;
            }
         }
         
         if (!params)
            break;
            
         logMessage("[sitoa] Restoring parameters on Scene");
         for (var j=0;j<params.length;j++)
         {
            var param = prop.parameters(params[j][0]);
            if (!param)
               continue;
            param.value = params[j][1];
         }
      }
   }   

   var passes = ActiveProject.ActiveScene.Passes;
   var npasses = passes.count;
   for (var i=0;i<npasses;i++)
   {
      var pass = passes(i);
      if (pass.type != "Pass")
         continue;
      var params = false;
      for (var j=0;j<options.length;j++)
      {
         if (options[j][0] == pass.name)
         {
            params = options[j][1];
            break;
         }
      }
      
      if (!params)
         continue;
         
      logMessage("[sitoa] Regenerating property for pass "+pass.name);
      var prop = MakeLocal(pass+".Arnold_Render_Options", siDefaultPropagation)(0);
      for (var j=0;j<params.length;j++)
      {
         var param = prop.parameters(params[j][0]);
         if (!param)
            continue;
         param.value = params[j][1];
      }
   }
   
   // finally, restore all the region options
   logMessage("[sitoa] Restoring region options");
   StoreRegionParameters(views, viewRegionParamNames, viewRegionParamValues, viewRegionNames);
}


/////////////////////////////////////////
/////////////////////////////////////////
// functions for saving/restoring/deleting the region options
/////////////////////////////////////////
/////////////////////////////////////////

function DeleteRegionOptions(view)
{
   var renderRegion = Dictionary.GetObject(view + ".RenderRegion", false);
   if (renderRegion == null)
      return;

   // copy the UsePassOptions value
   var usePassOptions = renderRegion.Parameters("UsePassOptions").Value;
   // set it to false, to have access to the options
   renderRegion.Parameters("UsePassOptions").Value = false;

   var arnoldRegion = Dictionary.GetObject(renderRegion + ".Arnold_Render_Options", false);
   if (arnoldRegion != null)
   {
      // logMessage("Deleting " + arnoldRegion);
      DeleteObj(arnoldRegion);
   }
   // restore the UsePassOptions value
   renderRegion.Parameters("UsePassOptions").Value = usePassOptions;
}


function DeleteViewRegionOptions(parent)
{
   var nested = parent.NestedObjects;
   var view;
   for (var i=0; i<nested.Count; i++)
   {
      view = nested.Item(i);
      if (view.type == "View")
         DeleteRegionOptions(view);
      else if (view.type == "List")
         DeleteViewRegionOptions(view);
   }
}


// loop the views and collect their region parameters
function CollectRegionParameters(parent, paramNames, paramValues, viewNames)
{
   var nested = parent.NestedObjects;
   for (var i=0; i<nested.Count; i++)
   {
      var view = nested.Item(i);
      if (view.type == "View")
      {
         var renderRegion = Dictionary.GetObject(view + ".RenderRegion", false);
         if (renderRegion == null)
            continue;
         // copy the UsePassOptions value            		
         var usePassOptions = renderRegion.Parameters("UsePassOptions").Value;
         // set it to false, to have access to the options
         renderRegion.Parameters("UsePassOptions").Value = false;			
         var arnoldRegion = Dictionary.GetObject(renderRegion + ".Arnold_Render_Options", false);
         if (arnoldRegion != null)
         {
            var parameters = arnoldRegion.Parameters;
            paramNames[paramNames.length] = new Array();
            paramValues[paramValues.length] = new Array();
            viewNames[viewNames.length] = view.name;
            for (var j=0; j<parameters.count; j++)
            {
               paramNames[paramNames.length-1][j] = parameters.Item(j).name;
               paramValues[paramNames.length-1][j] = parameters.Item(j).value;
            }
         }
         // restore the UsePassOptions value
         renderRegion.Parameters("UsePassOptions").Value = usePassOptions;
      }
      else if (view.type == "List")
         CollectRegionParameters(view, paramNames, paramValues, viewNames);
   }
}


function FindInArray(a, item)
{
   for (var i=0; i<a.length; i++)
      if (a[i] == item)
         return i;
   return -1;
}

// for debug
function LogArray(a)
{
   for (var i=0; i<a.length; i++)
      logMessage("name = " + a[i])
}


// loop the views and restore their region parameters
function StoreRegionParameters(parent, paramNames, paramValues, viewNames)
{
   var nested = parent.NestedObjects;
   for (var i=0; i<nested.Count; i++)
   {
      var view = nested.Item(i);
      if (view.type == "View")
      {
         var renderRegion = Dictionary.GetObject(view + ".RenderRegion", false);
         if (renderRegion == null)
            continue;
         // copy the UsePassOptions value
         var usePassOptions = renderRegion.Parameters("UsePassOptions").Value;
         // set it to false, to have access to the options
         renderRegion.Parameters("UsePassOptions").Value = false;			
         var arnoldRegion = Dictionary.GetObject(renderRegion + ".Arnold_Render_Options", false);
         if (arnoldRegion != null)
         {
            var viewIndex = FindInArray(viewNames, view.name);
            if (viewIndex == -1)
               continue;
				
            // logMessage("Storing View " + view.name);
            var parameters = arnoldRegion.Parameters;
            for (var j=0; j<parameters.count; j++)
            {
               // check if this param was saved. New parameters will return -1 and be skipped
               var paramIndex = FindInArray(paramNames[viewIndex], parameters.Item(j).name);
               if (paramIndex != -1)
               {
                  //logMessage("Storing " + parameters.Item(j).name + " = " + paramValues[viewIndex][paramIndex]);
                  // set it to the stored value
                  parameters.Item(j).value = paramValues[viewIndex][paramIndex];
               }
            }
         }	
         // restore the UsePassOptions value
         renderRegion.Parameters("UsePassOptions").Value = usePassOptions;
      }
      else if (view.type == "List")
         StoreRegionParameters(view, paramNames, paramValues, viewNames);
   }
}
