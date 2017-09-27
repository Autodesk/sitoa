/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "sitoa.h"

#include "loader/Procedurals.h"
#include "renderer/RenderInstance.h"
#include "renderer/Renderer.h"

#include <xsi_graphicsequencer.h>
#include <xsi_graphicsequencercontext.h>
#include <xsi_model.h>
#include <xsi_ppgeventcontext.h>
#include <xsi_ppglayout.h>
#include <xsi_vector3.h>
#include <xsi_x3dobject.h>

#include <ai.h>


#ifdef _WINDOWS
#include <windows.h>
#else
#define LPVOID void*
#endif

#include <math.h>
#include <GL/gl.h>

#include <vector>
#include <string>
#include <algorithm>

using namespace std;
using namespace XSI; 
using namespace XSI::MATH;


enum eDrawMode
{
   eDrawMode_Box = 0,
   eDrawMode_Points,
   eDrawMode_Wireframe
};


enum eNodeType
{
   eNodeType_Polymesh,
   eNodeType_Curves,
   eNodeType_Points,
   eNodeType_Sphere,
   eNodeType_Box
};


// The sequencer object class
class CGSObject
{
private:
   eNodeType             m_nodeType; 
   vector <AtVector>     m_vertices;
   vector <AtVector>     m_shuffledVertices;
   AtVector              m_bbMin, m_bbMax;
   vector <unsigned int> m_lineIndices; // indices of the lines to be drawn in wireframe mode

public:
   X3DObject             m_placeholder; // the Softimage procedural object

   CGSObject()
   {
      m_bbMin.x = m_bbMin.y = m_bbMin.z = 1000000.0f;
      m_bbMax.x = m_bbMax.y = m_bbMax.z = -1000000.0f;
   }

   CGSObject(const CGSObject &in_arg) : 
      m_nodeType(in_arg.m_nodeType), m_vertices(in_arg.m_vertices), 
      m_shuffledVertices(in_arg.m_shuffledVertices), m_lineIndices(in_arg.m_lineIndices), 
      m_placeholder(in_arg.m_placeholder)
   {
      m_bbMin = in_arg.m_bbMin;
      m_bbMax = in_arg.m_bbMax;
   }

   ~CGSObject()
   {
      m_vertices.clear();
      m_shuffledVertices.clear();
      m_lineIndices.clear();
   }

   // shuffle the vertices into m_shuffledVertices
   void ShuffleVertices();
   // Copy a polymesh node arrays into the sequencer object
   bool SetGeometry(AtArray* in_vlist, AtArray* in_vidxs, AtArray* in_nsides, AtMatrix in_matrix, eNodeType in_nodeType);
   // Copy a curves/points/sphere node arrays into the sequencer object
   bool SetGeometry(AtArray* in_points, AtArray* in_numPoints, AtMatrix in_matrix, eNodeType in_nodeType);
   // Copy a box node into the sequencer object
   bool SetGeometry(AtVector in_min, AtVector in_max);
   // Set the Softimage placeholder obj out of its id
   void SetPlaceholder(int in_id);
   // Update the sequencer object's bbox by an input point
   void UpdateBoundingBox(AtVector *in_p);
   // Draw the bounding box in ogl
   void DrawBox(AtRGB in_color, float in_size, double *in_placeholderMatrix);
   // Draw the object's points
   void DrawPoints(AtRGB in_color, float in_size, float in_pointsDisplayPcg, double *in_placeholderMatrix);
   // Draw the object in wire frame mode
   void DrawWireFrame(AtRGB in_color, float in_size, float in_pointsDisplayPcg, double *in_placeholderMatrix);
};


// The sequencer user data class
class CGSUserData
{
private:
   vector <CGSObject> m_objects; // the objects to draw
public:
   double m_frame;
   bool   m_useAsstoc;

   CGSUserData()
   {
      m_useAsstoc = false;
   }

   ~CGSUserData()
   {
      m_objects.clear();
   }

   // Check and return the SITOA_ViewerProperty, if found under the scene root
   bool   ViewerPropertyExists(Property &out_property);
   // Push an object into the objects vector to be drawned
   void   PushObject(CGSObject &in_object);
   // Draw all the objects stored in the objects vector
   void   Draw(int in_mode, bool in_randomColors, int in_seed, AtRGB in_color, float in_size, float in_pointsDisplayPcg, bool in_usePerProceduralParameters);
   // Initializes the sequencer data
   bool   Initialize();
   // Render the view
   void   Render(Property in_prop, siViewMode &out_viewMode);
};




