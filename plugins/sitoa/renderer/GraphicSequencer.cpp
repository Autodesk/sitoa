/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "renderer/GraphicSequencer.h"

/*
void LogMatrix(AtMatrix in_m)
{
   GetMessageQueue()->LogMessage(L"---------");
   for (int i=0; i<4; i++)
      GetMessageQueue()->LogMessage((CString)in_m[i][0] + L" " + (CString)in_m[i][1] + L" " + (CString)in_m[i][2] + L" " + (CString)in_m[i][3]);
}
*/

/////////////////////////////////////////////////////
// The sequencer object class
/////////////////////////////////////////////////////

typedef struct 
{
   int m_index, m_rand;
} Index;

bool IndexSort(const Index& in_d0, const Index& in_d1)
{
  return in_d0.m_rand < in_d1.m_rand;
}

// Shuffle the vertices into m_shuffledVertices
// m_shuffledVertices is used when drawing points. Since it contains all
// the vertices in random order, when the percentage to draw is < 1 we just
// need to draw the first nbVertices*percentuage points.
//
void CGSObject::ShuffleVertices()
{
   srand(2014);

   unsigned int vlistCount = (unsigned int)m_vertices.size();
   m_shuffledVertices.resize(vlistCount);

   vector <Index> indexVector;
   indexVector.resize(vlistCount);
   // give a random number to each index
   for (unsigned int i=0; i<vlistCount; i++)
   {
      indexVector[i].m_index = i;
      indexVector[i].m_rand = rand();
   }
   
   // sort by the random number
   std::sort(indexVector.begin(), indexVector.end(), IndexSort);
   
   // the sorted indices are used to re-index the points into m_shuffledVectors
   for (unsigned int i=0; i<vlistCount; i++)
      m_shuffledVertices[i] = m_vertices[indexVector[i].m_index];

   indexVector.clear();
}


// Copy a polymesh node arrays into the sequencer object
// 
// @param in_vlist   The polymesh vlist array
// @param in_vidxs   The polymesh vlist array
// @param in_nsides  The polymesh nsides array
// @param in_matrix  The polymesh matrix
//
// @return true if all went well, else false 
//
bool CGSObject::SetGeometry(AtArray* in_vlist, AtArray* in_vidxs, AtArray* in_nsides, AtMatrix in_matrix, eNodeType in_nodeType)
{
   if (AiArrayGetType(in_vlist) != AI_TYPE_VECTOR)
      return false;
   if (AiArrayGetType(in_vidxs) != AI_TYPE_UINT)
      return false;
   if (AiArrayGetType(in_nsides) != AI_TYPE_UINT)
      return false;

   // get only the first key list
   int vlistCount = AiArrayGetNumElements(in_vlist);
   m_vertices.resize(vlistCount);

   AtVector p, lp;
   for (int i=0; i < vlistCount; i++)
   {
      lp = AiArrayGetVec(in_vlist, i);
      p = AiM4PointByMatrixMult(in_matrix, lp);
      m_vertices[i] = p;
      UpdateBoundingBox(&p);
   }

   ShuffleVertices(); // shuffle the vertices randomly into m_shuffledVertices

   unsigned int nsidesCount = AiArrayGetNumElements(in_nsides);
   if (nsidesCount == 0)
      nsidesCount = AiArrayGetNumElements(in_vidxs)/3; // for #1356. If in_nsides is void, then defaults to constant 3 (triangulated mesh)

   // in this section, we convert the mesh into a set of lines, to be drawn later by
   // glDrawElements(GL_LINES, m_lineIndices.size(), GL_UNSIGNED_INT, &m_lineIndices[0]);
   // We're using lines, because elements drawn by glDrawElements must have the same size, so we
   // can't draw loops or stripes with varying (per polygon) count.
   // So, if we have a cube, 24 singular edges -> 48 lines
   unsigned int nbEdges(0);
   unsigned int vidxsIndex(0);
   unsigned int nIndex, nsides;

   for (nIndex=0; nIndex<nsidesCount; nIndex++)
   {
      unsigned int nsides = AiArrayGetNumElements(in_nsides) > 0 ? AiArrayGetUInt(in_nsides, nIndex) : 3;
      nbEdges+=nsides;
   }

   m_lineIndices.resize(nbEdges*2); // the indices array

   // if the first quad of the cube has indices 3,5,7,0, then the indices for it will be
   // currentIndex = 0
   // 3, 5, 5, 7, 7, 0, 0, 3
   // currentIndex = 8
   // ...
   unsigned int currentIndex(0);
   for (nIndex=0; nIndex<nsidesCount; nIndex++)
   {
      nsides = AiArrayGetNumElements(in_nsides) > 0 ? AiArrayGetUInt(in_nsides, nIndex) : 3;
      for (unsigned int i=0; i<nsides; i++)
      {
         // the even indices are the idxs themselves, so
         // m_lineIndices[0] = 3, m_lineIndices[2] = 5, m_lineIndices[4] = 7, m_lineIndices[6] = 0
         m_lineIndices[currentIndex + i*2] = AiArrayGetUInt(in_vidxs, vidxsIndex);
         vidxsIndex++;
      }

      // the odd indices are equal to the next on the right, except the last one (7) that takes the first (0),
      // so ..., m_lineIndices[7] = m_lineIndices[0]
      for (unsigned int i=0; i<nsides; i++)
         m_lineIndices[currentIndex + i*2 + 1] = m_lineIndices[currentIndex + (i*2 + 2) % (nsides *2)];

      // base for the next polygon
      currentIndex+= nsides * 2;
   }

   m_nodeType = in_nodeType;
   return true;
}


// Copy a curves/points/sphere node arrays into the sequencer object
// 
// @param in_points      The curves points array
// @param in_numPoints   The curves numPoints array (for curves only)
// @param in_matrix      The curves matrix
//
// @return true if all went well, else false 
//
bool CGSObject::SetGeometry(AtArray* in_points, AtArray* in_numPoints, AtMatrix in_matrix, eNodeType in_nodeType)
{
   if (AiArrayGetType(in_points) != AI_TYPE_VECTOR)
      return false;
   if (in_numPoints && AiArrayGetType(in_numPoints) != AI_TYPE_UINT)
      return false;

   unsigned int i,j;

   // get only the first key list
   unsigned int vlistCount = AiArrayGetNumElements(in_points);
   m_vertices.resize(vlistCount);

   AtVector p, lp;
   for (i=0; i < vlistCount; i++)
   {
      lp = AiArrayGetVec(in_points, i);
      p = AiM4PointByMatrixMult(in_matrix, lp);
      m_vertices[i] = p;
      UpdateBoundingBox(&p);
   }

   ShuffleVertices(); // shuffle the vertices randomly into m_shuffledVertices

   unsigned int nbPointsPerCurve;
   if (in_numPoints) // curves case
   {
      unsigned int nbTotalEdges(0);
      unsigned int nbCurves, nbEdgesPerCurve;

      if (AiArrayGetNumElements(in_numPoints) == 1) // constant number of points per strand
      {
         nbPointsPerCurve = AiArrayGetUInt(in_numPoints, 0);
         nbCurves = vlistCount / nbPointsPerCurve;
         nbTotalEdges = nbCurves * (nbPointsPerCurve - 1);
      }
      else 
      {
         nbCurves = AiArrayGetNumElements(in_numPoints);
         for (unsigned i=0; i<nbCurves; i++)
         {
            // get the number of points of each curve
            nbPointsPerCurve = AiArrayGetUInt(in_numPoints, i);
            nbTotalEdges+= nbPointsPerCurve - 1;
         }
      }

      m_lineIndices.resize(nbTotalEdges*2);
      
      // say nbPointsPerCurve = 4. The lines must go through
      // 0 1 2 3, 4 5 6 7, ...
      // so the lines must connect
      // 0 1, 1 2, 2 3, 4 5, 5 6, 6 7, ...... 

      unsigned int currentIndex(0), curveBaseIndex(0);

      for (i=0; i<nbCurves; i++)
      {
         nbPointsPerCurve = AiArrayGetNumElements(in_numPoints) == 1 ? AiArrayGetUInt(in_numPoints, 0) : 
                                                                       AiArrayGetUInt(in_numPoints, i);
         nbEdgesPerCurve = nbPointsPerCurve - 1;

         // the even indices are the idxs themselves, so
         // m_lineIndices[0] = 0, m_lineIndices[2] = 1, m_lineIndices[4] = 2
         for (j=0; j<nbEdgesPerCurve; j++)
         {
            m_lineIndices[curveBaseIndex + j*2] = currentIndex;
            currentIndex++;
         }
         
         // the odd indices are equal to the next on the right
         for (unsigned int j=0; j<nbEdgesPerCurve-1; j++)
            m_lineIndices[curveBaseIndex + j*2 + 1] = m_lineIndices[curveBaseIndex + j*2 + 2];
         // except the last one, which is the next idx (m_lineIndices[5] = 3
         m_lineIndices[curveBaseIndex + (nbEdgesPerCurve-1)*2 + 1] = currentIndex;
         currentIndex++;

         curveBaseIndex+= nbEdgesPerCurve*2;
      }
   }

   m_nodeType = in_nodeType;
   return true;
}


// Copy a box node into the sequencer object
// 
// @param in_min         The box min
// @param in_max         The box max
//
// @return true 
//
bool CGSObject::SetGeometry(AtVector in_min, AtVector in_max)
{
   m_bbMin = in_min;
   m_bbMax = in_max;
   m_nodeType = eNodeType_Box;
   return true;
}


// Set the Softimage placeholder obj out of its id
// 
// @param in_id      The objet's id
//
void CGSObject::SetPlaceholder(int in_id)
{
   m_placeholder = (X3DObject)Application().GetObjectFromID((ULONG)in_id);
}


// Update the sequencer object's bbox by an input point
// 
// @param in_p      The point to update the bbox with
//
void CGSObject::UpdateBoundingBox(AtVector *in_p)
{
   if (in_p->x < m_bbMin.x)
      m_bbMin.x = in_p->x;
   if (in_p->y < m_bbMin.y)
      m_bbMin.y = in_p->y;
   if (in_p->z < m_bbMin.z)
      m_bbMin.z = in_p->z;

   if (in_p->x > m_bbMax.x)
      m_bbMax.x = in_p->x;
   if (in_p->y > m_bbMax.y)
      m_bbMax.y = in_p->y;
   if (in_p->z > m_bbMax.z)
      m_bbMax.z = in_p->z;
}


// Draw the bounding box in ogl
// 
// @param in_color              The color to use
// @param in_size               The line width to use
// @param in_placeholderMatrix  The ogl placeholder matrix
//
void CGSObject::DrawBox(AtRGB in_color, float in_size, double *in_placeholderMatrix)
{
   if (in_size <= 0.0f)
      return;
   if (m_nodeType != eNodeType_Box)
      if (m_vertices.size() < 1)
         return;

   glColor3f(in_color.r, in_color.g, in_color.b);

   glPushMatrix();
	glMultMatrixd(in_placeholderMatrix);

   AtVector p;
   
   // bottom floor
   glBegin(GL_LINE_LOOP);
      p = m_bbMin;
      glVertex3f(p.x, p.y, p.z);
      p.z = m_bbMax.z;
      glVertex3f(p.x, p.y, p.z);
      p.x = m_bbMax.x;
      glVertex3f(p.x, p.y, p.z);
      p.z = m_bbMin.z;
      glVertex3f(p.x, p.y, p.z);
   glEnd();

   // top floor
   glBegin(GL_LINE_LOOP);
      p = m_bbMax;
      glVertex3f(p.x, p.y, p.z);
      p.z = m_bbMin.z;
      glVertex3f(p.x, p.y, p.z);
      p.x = m_bbMin.x;
      glVertex3f(p.x, p.y, p.z);
      p.z = m_bbMax.z;
      glVertex3f(p.x, p.y, p.z);
   glEnd();

   // walls   
   glBegin(GL_LINES);
      p = m_bbMin;
      glVertex3f(p.x, p.y, p.z);
      p.y = m_bbMax.y;
      glVertex3f(p.x, p.y, p.z);

      p = m_bbMin;
      p.z = m_bbMax.z;
      glVertex3f(p.x, p.y, p.z);
      p.y = m_bbMax.y;
      glVertex3f(p.x, p.y, p.z);

      p = m_bbMin;
      p.x = m_bbMax.x;
      p.z = m_bbMax.z;
      glVertex3f(p.x, p.y, p.z);
      p.y = m_bbMax.y;
      glVertex3f(p.x, p.y, p.z);

      p = m_bbMin;
      p.x = m_bbMax.x;
      glVertex3f(p.x, p.y, p.z);
      p.y = m_bbMax.y;
      glVertex3f(p.x, p.y, p.z);
   glEnd();

   glPopMatrix();
}


// Draw the object's points
// 
// @param in_color              The color to use
// @param in_size               The point size to use
// @param in_pointsDisplayPcg   The pcg (0..1) of point to draw
// @param in_placeholderMatrix  The ogl placeholder matrix
//
void CGSObject::DrawPoints(AtRGB in_color, float in_size, float in_pointsDisplayPcg, double *in_placeholderMatrix)
{
   if (m_nodeType == eNodeType_Box)
   {
      DrawBox(in_color, in_size, in_placeholderMatrix);
      return;
   }

   if (in_size <= 0.0f)
      return;
   if (m_vertices.size() < 1)
      return;

   glPushMatrix();
	glMultMatrixd(in_placeholderMatrix);

   glEnableClientState(GL_VERTEX_ARRAY);
   glColor3f(in_color.r, in_color.g, in_color.b);

   int drawingSize = (int)((float)m_vertices.size() * in_pointsDisplayPcg);
   glVertexPointer(3, GL_FLOAT, 0, &m_shuffledVertices[0]);
   glDrawArrays(GL_POINTS, 0, (GLsizei)drawingSize);

   glDisableClientState(GL_VERTEX_ARRAY);

   glPopMatrix();
}


// Draw the object in wire frame mode
// 
// @param in_color              The color to use
// @param in_size               The line width to use
// @param in_pointsDisplayPcg   The pcg (0..1) of point to draw
// @param in_placeholderMatrix  The ogl placeholder matrix
//
void CGSObject::DrawWireFrame(AtRGB in_color, float in_size, float in_pointsDisplayPcg, double *in_placeholderMatrix)
{
   if (m_nodeType == eNodeType_Box)
   {
      DrawBox(in_color, in_size, in_placeholderMatrix);
      return;
   }
   if (m_nodeType == eNodeType_Points || m_nodeType == eNodeType_Sphere)
   {
      DrawPoints(in_color, in_size, in_pointsDisplayPcg, in_placeholderMatrix);
      return;
   }

  if (in_size <= 0.0f)
      return;
   if (m_vertices.size() < 1)
      return;

   glPushMatrix();
	glMultMatrixd(in_placeholderMatrix);

   glColor3f(in_color.r, in_color.g, in_color.b);

   // drawing the set of lines that were stored in m_lineIndices at loading time, in both mesh or curves case
   glEnableClientState(GL_VERTEX_ARRAY);
   glVertexPointer(3, GL_FLOAT, 0, &m_vertices[0]);
   glDrawElements(GL_LINES, (GLsizei)m_lineIndices.size(), GL_UNSIGNED_INT, &m_lineIndices[0]);
   glDisableClientState(GL_VERTEX_ARRAY);

   glPopMatrix();
}


/////////////////////////////////////////////////////
// The sequencer user data class
/////////////////////////////////////////////////////

// Check and return the SITOA_ViewerProperty, if found under the scene root
//
// @param out_property  The returned property (if found)
//
// @return true if the property exists, else false 
//
bool CGSUserData::ViewerPropertyExists(Property &out_property)
{
   CRefArray propArray = Application().GetActiveSceneRoot().GetProperties();

   for (int i=0; i<propArray.GetCount(); i++)
   {
      Property prop = propArray[i];
      if (prop.GetType() == L"SITOA_ViewerProperty")
      {
         out_property = prop;
         return true;
      }
   }

   return false;
}


// Push an object into the objects vector to be drawned
//
// @param in_object  The new object to push
//
void CGSUserData::PushObject(CGSObject &in_object)
{
   m_objects.push_back(in_object);
}


// Draw all the objects stored in the objects vector
// 
// @param in_mode              0==bbox, 1==points, 2==wireframe
// @param in_randomColors      Whether or not to generate a random color for each object
// @param in_seed              The random seed
// @param in_color             The color to use
// @param in_size              The line width to use
// @param in_pointsDisplayPcg  The pcg (0..1) of point to draw
//
void CGSUserData::Draw(int in_mode, bool in_randomColors, int in_seed, AtRGB in_color, float in_size, float in_pointsDisplayPcg, bool in_usePerProceduralParameters)
{
   if (in_randomColors)
      srand(in_seed);

   GLboolean xLighting;
   float xPointSize, xLineWidth, xColor[4];
   // save the current viewing options
   glGetFloatv(GL_POINT_SIZE, &xPointSize);
   glGetFloatv(GL_LINE_WIDTH, &xLineWidth);
   glGetFloatv(GL_CURRENT_COLOR, xColor);
   glGetBooleanv(GL_LIGHTING, &xLighting);

   glDisable(GL_LIGHTING);
   int count=0;

   for (vector <CGSObject>::iterator it=m_objects.begin(); it!=m_objects.end(); it++, count++)
   {
      int     mode = in_mode;
      bool    randomColors = in_randomColors;
      int     seed = in_seed;
      AtRGB   color = in_color;
      float   size = in_size;
      float   pointsDisplayPcg = in_pointsDisplayPcg;

      if (in_usePerProceduralParameters && it->m_placeholder.IsValid())
      {
         Property proceduralProperty = it->m_placeholder.GetProperties().GetItem(L"arnold_procedural");
         // the IsValid below is a check against scenes saved with a previous version of the procedural property
         if (proceduralProperty.IsValid() && proceduralProperty.GetParameter(L"mode").IsValid())
         {
            mode         = (int)ParAcc_GetValue(proceduralProperty, L"mode", DBL_MAX);
            randomColors = (bool)ParAcc_GetValue(proceduralProperty, L"randomColors", DBL_MAX);
            if (randomColors)
            {
               seed = (int)ParAcc_GetValue(proceduralProperty, L"seed", DBL_MAX);
               srand(seed + count);
            }
            else
            {
               color.r = (float)ParAcc_GetValue(proceduralProperty, L"colorR", DBL_MAX);
               color.g = (float)ParAcc_GetValue(proceduralProperty, L"colorG", DBL_MAX);
               color.b = (float)ParAcc_GetValue(proceduralProperty, L"colorB", DBL_MAX);
            }
            size             = (float)ParAcc_GetValue(proceduralProperty, L"size", DBL_MAX);
            pointsDisplayPcg = (float)ParAcc_GetValue(proceduralProperty, L"pointsDisplayPcg", DBL_MAX);
         }
      }

      glPointSize(size);
      glLineWidth(size);

      if (randomColors)
      {
         rand(); // trash the first rand after a srand, it's always ~= 0
         color.r = rand()/(float)RAND_MAX;
         color.g = rand()/(float)RAND_MAX;
         color.b = rand()/(float)RAND_MAX;
      }

      double phM[16]; // the placeholder matrix in ogl format
      if (it->m_placeholder.IsValid())
      {
         CMatrix4 m = it->m_placeholder.GetKinematics().GetGlobal().GetTransform().GetMatrix4();
         m.Get(phM[0], phM[1], phM[2], phM[3], phM[4], phM[5], phM[6], phM[7],
               phM[8], phM[9], phM[10], phM[11], phM[12], phM[13], phM[14], phM[15]);
      }
      else // identity
      {
         phM[1] = phM[2] = phM[3] = phM[4] = phM[6] = phM[7] = phM[8] = phM[9] = phM[11] = phM[12] = phM[13] = phM[14] = 0.0;
         phM[0] = phM[5] = phM[10] = phM[15] = 1.0;
      }

      switch (mode)
      {
         case eDrawMode_Box:
            it->DrawBox(color, size, phM);
            break;
         case eDrawMode_Points:
            it->DrawPoints(color, size, pointsDisplayPcg, phM);
            break;
         case eDrawMode_Wireframe:
            it->DrawWireFrame(color, size, pointsDisplayPcg, phM);
            break;
      }
   }

   // restore the viewing options
   glColor4f(xColor[0], xColor[1], xColor[2], xColor[3]);
   glPointSize(xPointSize);
   glLineWidth(xLineWidth);
   if (xLighting)
      glEnable(GL_LIGHTING);
}


// Initializes the sequencer user data, by:
// 1. Create an Arnold universe, and pushing all the procedural nodes associated with all the procedural properties
// 2. Write the universe to a temp ass with -resaveop, to flatten down all the geo created by the procedurals, and 
//    propagate to all the node an attribute representing the Softimage object's id 
// 3. Load the temp ass
// 4. Iterate all the shapes, pushing all the polymesh, curves, points, sphere nodes into the vector of the objects
//    that will then be drawn at Execute time
// 
// @return true if all went well, else false 
// 
bool CGSUserData::Initialize()
{
   // Until we don't have multiple universes, we have to destroy the current one (if any) and create a new one.
   // So the ipr scene will be destroyed after you open the GS
   if (AiArnoldIsActive())
      GetRenderInstance()->DestroyScene(false);
   if (AiArnoldIsActive())
      return false;

   Property prop;
   m_useAsstoc = false;
   if (ViewerPropertyExists(prop))
      // protect against scenes saved with a previous version of the property
      if (prop.GetParameter(L"use_asstoc").IsValid())
         m_useAsstoc = (bool)ParAcc_GetValue(prop, L"use_asstoc", DBL_MAX);

   AiBegin(GetSessionMode());
   AtNode *options = AiUniverseGetOptions();
   CNodeSetter::SetBoolean(options, "skip_license_check", true);
   CNodeSetter::SetBoolean(options, "enable_procedural_cache", false); // for #1660
   
   CRefArray properties;
   Property  proceduralProperty;
   // get all the candidate owners of the procedural property
   CStringArray families(3);
   families.Add(siMeshFamily);
   families.Add(siPointCloudFamily);
   families.Add(siGeometryFamily);
   CRefArray objects = Application().GetActiveSceneRoot().FindChildren(L"", L"", families, true);

   m_frame = CTimeUtilities().GetCurrentFrame();

   for (LONG i = 0; i < objects.GetCount(); i++)
   {
      X3DObject obj(objects[i]);
      if (!obj.IsValid())
         continue;
      properties = obj.GetProperties();

      proceduralProperty = properties.GetItem(L"arnold_procedural");
      if (!proceduralProperty.IsValid())
         continue;

      // If the placeholder is invisible, skip it
      Property vizProp = properties.GetItem(L"Visibility");
      if (!(bool)ParAcc_GetValue(vizProp, L"rendvis", DBL_MAX))
         continue;

      CPathString filename = ParAcc_GetValue(proceduralProperty, L"filename", DBL_MAX).GetAsText();

      // skip the procedural with the reserved (for ice) ArnoldProcedural prefix
      if (filename.IsEqualNoCase(g_ArnoldProceduralAttributePrefix))
         continue;

      double sFrame = m_frame;
      if ((bool)ParAcc_GetValue(proceduralProperty, L"overrideFrame", DBL_MAX))
         sFrame = (double)ParAcc_GetValue(proceduralProperty, L"frame", DBL_MAX);

      filename.ResolveTokensInPlace(sFrame); // resolve the tokens
      if (filename.IsEmpty())
         continue;
      if (!filename.IsProcedural())
         continue;

      AtNode* node = NULL;

      bool asstocFound(false);
      if (m_useAsstoc && filename.IsAss())
      {
         CVector3f bbMin, bbMax;
         CPathString asstocFilename = filename.GetAssToc();
         if (GetBoundingBoxFromScnToc(asstocFilename, bbMin, bbMax))
         {
            asstocFound = true;
            node = AiNode("box");
            CNodeSetter::SetVector(node, "min", bbMin.GetX(), bbMin.GetY(), bbMin.GetZ());
            CNodeSetter::SetVector(node, "max", bbMax.GetX(), bbMax.GetY(), bbMax.GetZ());
         }
      }

      if (!asstocFound)
      {
         node = AiNode("procedural");
         if (!node)
            continue;
         CNodeSetter::SetString(node,  "filename", filename.GetAsciiString());
      }

      CNodeUtilities().SetName(node, obj.GetFullName());
      // attach to the procedural node the Softimage object's id
      if (AiNodeDeclare(node, "SoftimageObjId", "constant INT"))
         CNodeSetter::SetInt(node, "SoftimageObjId", (int)CObjectUtilities().GetId(obj));
   }

   // to have the procedural matrices flattened down to the shapes,
   // we have to AiASSWrite. So, let's get a valid temporary filename.
   // We'll be able to avoid this costly step if Arnold exposes for a shape AtNode the 
   // pointer to the procedural node that originated a shape node, so that we'll be able
   // to concatenate the matrices ourselves.

   // CString tempPath = L"C:\\temp";
   CString tempPath = CUtils::ResolvePath(L"$TEMP"); // the Softimage temp dir, deleted on exit
   CString assPath = CUtils::BuildPath(tempPath, L"SITOA_Viewer.ass");
   AiASSWrite(assPath.GetAsciiString(), AI_NODE_ALL, true);
   // ok, done
   AiEnd();

   // now read back the resavep-ed universe
   AiBegin(GetSessionMode());
   options = AiUniverseGetOptions();
   CNodeSetter::SetBoolean(options, "preserve_scene_data", true);
   CNodeSetter::SetBoolean(options, "skip_license_check", true);

   AiASSLoad(assPath.GetAsciiString());

   AtMatrix matrix;

   AtNodeIterator *iter = AiUniverseGetNodeIterator(AI_NODE_SHAPE);
   while (!AiNodeIteratorFinished(iter))
   {
      AtNode *node = AiNodeIteratorGetNext(iter);
      if (!node)
         break;

      CGSObject cgsObj;

      // get the shape matrix
      AtArray* matrices = AiNodeGetArray(node, "matrix");
      if (matrices && AiArrayGetNumElements(matrices) > 0)
         matrix = AiArrayGetMtx(matrices, 0);
      else // #1356. nodes with no explicit matrix declared
         matrix = AiM4Identity();

      // if this is ginstance, get the master shape
      if (AiNodeIs(node, ATSTRING::ginstance))
         node = (AtNode*)AiNodeGetPtr(node, "node");

      bool ok(false);

      // get the appropriate data for each node type, and store them into the gs object
      if (AiNodeIs(node, ATSTRING::polymesh))
      {
         AtArray* vlist = AiNodeGetArray(node,  "vlist");
         AtArray* vidxs = AiNodeGetArray(node,  "vidxs");
         AtArray* nsides = AiNodeGetArray(node, "nsides");
         cgsObj.SetGeometry(vlist, vidxs, nsides, matrix, eNodeType_Polymesh);
         ok = true;
      }
      else if (AiNodeIs(node, ATSTRING::curves))
      {
         AtArray* points = AiNodeGetArray(node, "points");
         AtArray* num_points = AiNodeGetArray(node, "num_points");
         cgsObj.SetGeometry(points, num_points, matrix, eNodeType_Curves);
         ok = true;
      }
      else if (AiNodeIs(node, ATSTRING::points))
      {
         AtArray* points = AiNodeGetArray(node, "points");
         cgsObj.SetGeometry(points, NULL, matrix, eNodeType_Points);
         ok = true;
      }
      else if (AiNodeIs(node, ATSTRING::sphere))
      {
         AtArray* center = AiNodeGetArray(node, "center");
         cgsObj.SetGeometry(center, NULL, matrix, eNodeType_Sphere);
         ok = true;
      }
      else if (AiNodeIs(node, ATSTRING::box)) // asstoc mode ?
      {
         AtVector min = AiNodeGetVec(node, "min");
         AtVector max = AiNodeGetVec(node, "max");
         cgsObj.SetGeometry(min, max);
         ok = true;
      }

      if (ok)
      {
         // retrieve the placeholder id and give it to the gs object
         // so that at render time we'll be able to get the transformation matrix
         int id = AiNodeGetInt(node, "SoftimageObjId");
         cgsObj.SetPlaceholder(id);
         // finally, push the object in the user data
         PushObject(cgsObj);
      }
   }

   AiNodeIteratorDestroy(iter);
   AiEnd();
   return true;
}


// Render the view
// 
// @param in_prop        The SITOA_Viewer property, if any
// @param out_viewMode   The mode to use to draw all the rest of the Softimage scene, except our ogl stuff
//
void CGSUserData::Render(Property in_prop, siViewMode &out_viewMode)
{
   int    renderType(siWireframe);
   int    mode(0), seed(50);
   int    size(1);
   bool   randomColors(false), usePerProceduralParameters(false);
   double pointsDisplayPcg(1.0);
   AtRGB  color;
   color.r = 1.0f; color.g = 0.0f; color.b = 0.0f;

   // Read params from the property
   if (in_prop.IsValid())
   {
      renderType = (int)ParAcc_GetValue(in_prop, L"scene_view_type", DBL_MAX);
      mode = (int)ParAcc_GetValue(in_prop, L"mode", DBL_MAX);

      size = (int)ParAcc_GetValue(in_prop, L"size", DBL_MAX);
      randomColors = (bool)ParAcc_GetValue(in_prop, L"random_colors", DBL_MAX);
      if (randomColors)
         seed = (int)ParAcc_GetValue(in_prop, L"seed", DBL_MAX);
      else
      {
         color.r = (float)ParAcc_GetValue(in_prop, L"colorR", DBL_MAX);
         color.g = (float)ParAcc_GetValue(in_prop, L"colorG", DBL_MAX);
         color.b = (float)ParAcc_GetValue(in_prop, L"colorB", DBL_MAX);
      }

      if (renderType != 0)
         out_viewMode = (siViewMode)renderType;

      pointsDisplayPcg = (float)ParAcc_GetValue(in_prop, L"points_display_pcg", DBL_MAX);

      usePerProceduralParameters = (bool)ParAcc_GetValue(in_prop, L"use_per_procedural_parameters", DBL_MAX);
   }

   Draw(mode, randomColors, seed, color, (float)size, (float)pointsDisplayPcg, usePerProceduralParameters);
}

/////////////////////////////////////////////////////
// The Graphic sequencer callbacks.
// I had to #ifdef by the os, since I was not able to unify the callbacks type for both
/////////////////////////////////////////////////////

// Called just once
#ifdef _WINDOWS
XSIPLUGINCALLBACK	void SITOA_Viewer_Init(CRef in_sequencerContext, LPVOID *in_userData)
#else
SITOA_CALLBACK SITOA_Viewer_Init(CRef in_sequencerContext, LPVOID *in_userData)
#endif
{
   GraphicSequencerContext graphicSequencerContext = in_sequencerContext;
   CGraphicSequencer sequencer = graphicSequencerContext.GetGraphicSequencer();
   sequencer.RegisterDisplayCallback(L"SITOA_Viewer", 0, siPass, siCustom, L"SITOA_Viewer");

#ifdef _WINDOWS
#else
   return CStatus::OK;
#endif
}


// This one is called on every refresh of the view
#ifdef _WINDOWS
XSIPLUGINCALLBACK	void SITOA_Viewer_Execute(CRef in_sequencerContext, LPVOID *in_userData)
#else
SITOA_CALLBACK SITOA_Viewer_Execute(CRef in_sequencerContext, LPVOID *in_userData)
#endif
{
   GraphicSequencerContext graphicSequencerContext = in_sequencerContext;
   CGraphicSequencer sequencer = graphicSequencerContext.GetGraphicSequencer();
   siViewMode viewMode = siWireframe;

   if (in_userData != NULL)
   {
      CGSUserData *uData = (CGSUserData*)*in_userData;
      Property prop;

      bool refresh(false), useAsstoc(false);
      if (uData->ViewerPropertyExists(prop))
      {
         refresh = (bool)ParAcc_GetValue(prop, L"refresh_on_frame_change", DBL_MAX);
         if (prop.GetParameter(L"use_asstoc").IsValid())
            useAsstoc = (bool)ParAcc_GetValue(prop, L"use_asstoc", DBL_MAX);
      }

      bool destroyHimMyRobots = refresh && (CTimeUtilities().GetCurrentFrame() != uData->m_frame);
      destroyHimMyRobots = destroyHimMyRobots || useAsstoc != uData->m_useAsstoc;

      if (destroyHimMyRobots)
      {
         delete uData;
         uData = new CGSUserData;
         *in_userData = (LPVOID*)uData;
         uData->Initialize();
      }

      uData->Render(prop, viewMode);
   }

   sequencer.RenderSceneUsingMode(viewMode, siRenderDefault);

#ifdef _WINDOWS
#else
   return CStatus::OK;
#endif
}


// Called on exit
#ifdef _WINDOWS
XSIPLUGINCALLBACK	void SITOA_Viewer_Term(CRef in_sequencerContext, LPVOID *in_userData)
#else
SITOA_CALLBACK SITOA_Viewer_Term(CRef in_sequencerContext, LPVOID *in_userData)
#endif
{
   if (AiArnoldIsActive())
      GetRenderInstance()->DestroyScene(false);

#ifdef _WINDOWS
#else
   return CStatus::OK;
#endif
}


// This one is called when the user selectes the viewer from the drop down menu in the Softimage view.
// If more than window has SITOA_Viewer, it gets called for each window. It would be better to have a unique
// user data, instead of one per window, but I could not find the way
#ifdef _WINDOWS
XSIPLUGINCALLBACK	void  SITOA_Viewer_InitInstance(CRef in_sequencerContext, LPVOID *in_userData)
#else
SITOA_CALLBACK SITOA_Viewer_InitInstance(CRef in_sequencerContext, LPVOID *in_userData)
#endif
{
   GraphicSequencerContext graphicSequencerContext = in_sequencerContext;
   CGraphicSequencer sequencer = graphicSequencerContext.GetGraphicSequencer();

   CGSUserData *uData = new CGSUserData;
   *in_userData = (LPVOID*)uData;

   // apply the property on the root, if not there yet
   Property prop;
   if (!uData->ViewerPropertyExists(prop))
   {
      prop = Application().GetActiveSceneRoot().AddProperty(L"SITOA_ViewerProperty", false, L"SITOA_Viewer");
      //Show the property
      CValueArray args(5) ;
      args[0] = prop.GetFullName();
      CValue retval = false ;
      Application().ExecuteCommand(L"InspectObj", args, retval);
   }

   uData->Initialize();

#ifdef _WINDOWS
#else
   return CStatus::OK;
#endif
}


// This one is called when the user changes the viewing mode from the drop down menu in the Softimage view
#ifdef _WINDOWS
XSIPLUGINCALLBACK	void  SITOA_Viewer_TermInstance(CRef in_sequencerContext, LPVOID *in_userData)
#else
SITOA_CALLBACK SITOA_Viewer_TermInstance(CRef in_sequencerContext, LPVOID *in_userData)
#endif
{
   if (*in_userData)
   {
      CGSUserData *uData = (CGSUserData*)*in_userData;
      delete uData;
      *in_userData = NULL;
   }
#ifdef _WINDOWS
#else
   return CStatus::OK;
#endif
}



/////////////////////////////////////////////////////////////
// The SITOA_Viewer property
/////////////////////////////////////////////////////////////

SITOA_CALLBACK SITOA_ViewerProperty_Define( CRef& in_ctxt )
{
   Context ctxt( in_ctxt );
   Parameter oParam;
   CustomProperty cpset = ctxt.GetSource();
   cpset.AddParameter(L"scene_view_type", CValue::siInt4,    siPersistable | siKeyable, L"", L"", siWireframe, 0, 100, 0, 100, oParam);
   cpset.AddParameter(L"mode",            CValue::siInt4,    siPersistable | siKeyable, L"", L"", 0, 0, 2, 0, 2, oParam);
   cpset.AddParameter(L"size",            CValue::siInt4,    siPersistable | siAnimatable | siKeyable,  L"", L"", 1, 0, 10, 0, 5, oParam);
   cpset.AddParameter(L"random_colors",   CValue::siBool,    siPersistable | siKeyable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), oParam);
   cpset.AddParameter(L"seed",            CValue::siInt4,    siPersistable | siKeyable,L"", L"", 50, 0, 100, 0, 100, oParam);
   oParam.PutCapabilityFlag(siReadOnly, true);

   cpset.AddParameter(L"colorR",          CValue::siDouble,  siPersistable,  L"",L"",    1.0, 0.0, 1.0, 0.0, 1.0, oParam);
   cpset.AddParameter(L"colorG",          CValue::siDouble,  siPersistable,  L"",L"",    0.0, 0.0, 1.0, 0.0, 1.0, oParam);
   cpset.AddParameter(L"colorB",          CValue::siDouble,  siPersistable,  L"",L"",    0.0, 0.0, 1.0, 0.0, 1.0, oParam);

   cpset.AddParameter(L"refresh_on_frame_change", CValue::siBool,    siPersistable | siAnimatable | siKeyable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), oParam);
   cpset.AddParameter(L"points_display_pcg",      CValue::siDouble,  siPersistable | siAnimatable | siKeyable,  L"", L"", 1.0, 0.0, 1.0, 0.0, 1.0, oParam);
   oParam.PutCapabilityFlag(siReadOnly, true); // because the default mode is box

   cpset.AddParameter(L"use_asstoc",                 CValue::siBool,    siPersistable | siAnimatable | siKeyable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), oParam);
   cpset.AddParameter(L"use_per_procedural_parameters", CValue::siBool,    siPersistable | siAnimatable | siKeyable, L"", L"", false, CValue(), CValue(), CValue(), CValue(), oParam);

   return CStatus::OK;
}


SITOA_CALLBACK SITOA_ViewerProperty_DefineLayout( CRef& in_ctxt )
{
   Context ctxt(in_ctxt);
   PPGLayout oLayout;
   PPGItem oItem;
   oLayout = ctxt.GetSource();
   oLayout.Clear();

   oLayout.PutAttribute(siUIHelpFile, L"https://support.solidangle.com/display/A5SItoAUG/The+SItoA+Viewer");

   oLayout.AddGroup(L"Global Options");

   CValueArray sceneViewItems(18);
   sceneViewItems[0] =  L"Wireframe";           sceneViewItems[1] = siWireframe;
   sceneViewItems[2] =  L"Depth Cue";           sceneViewItems[3] = siDepthCue;
   sceneViewItems[4] =  L"Hidden Line Removal"; sceneViewItems[5] = siHiddenLineRemoval;
   sceneViewItems[6] =  L"Constant";            sceneViewItems[7] = siConstant;
   sceneViewItems[8] =  L"Shaded";              sceneViewItems[9] = siShaded;
   sceneViewItems[10] = L"Textured";            sceneViewItems[11] = siTextured;
   sceneViewItems[12] = L"Textured Decal";      sceneViewItems[13] = siTexturedDecal;
   sceneViewItems[14] = L"Realtime";            sceneViewItems[15] = siRealtimePortMaterial;
   sceneViewItems[16] = L"Hide";                sceneViewItems[17] = 0;
   oLayout.AddEnumControl(L"scene_view_type", sceneViewItems, L"Scene View", L"Combo");

   oLayout.AddItem (L"refresh_on_frame_change", L"Refresh On Frame Change");

   oLayout.EndGroup();

   oLayout.AddGroup(L"Procedurals");
      oLayout.AddItem (L"use_asstoc", L"Use .asstoc (if available)");   
      oLayout.AddItem (L"use_per_procedural_parameters", L"Use Per-Procedural Parameters");   
      CValueArray modeItems(6);
      modeItems[0] =  L"Box";       modeItems[1] = 0;
      modeItems[2] =  L"Points";    modeItems[3] = 1;
      modeItems[4] =  L"Wireframe"; modeItems[5] = 2;
      oLayout.AddEnumControl(L"mode", modeItems, L"Mode", L"Combo");

      oLayout.AddGroup(L"Colors");
      oLayout.AddRow();
      oLayout.AddItem (L"random_colors", L"Random Colors");
      oLayout.AddItem (L"seed", L"Seed");
      oLayout.EndRow();
      oLayout.AddColor(L"colorR",       L"Color", false);
      oLayout.EndGroup();

      oLayout.AddGroup(L"Options");
      oLayout.AddItem (L"size", L"Line/Point Size");
      oLayout.AddItem (L"points_display_pcg", L"Points Display %");
      oLayout.EndGroup();
   oLayout.EndGroup();

   return CStatus::OK;
}


// Layout event handler for random_colors
//
// @param in_prop   The SITOA_Viewer property
//
void RandomColorsOnChanged(CustomProperty in_prop)
{
   bool randomColors = (bool)ParAcc_GetValue(in_prop, L"random_colors", DBL_MAX);
   in_prop.GetParameter(L"seed").PutCapabilityFlag(siReadOnly,  !randomColors); 
   in_prop.GetParameter(L"colorR").PutCapabilityFlag(siReadOnly, randomColors); 
}


// Layout event handler for mode
//
// @param in_prop   The SITOA_Viewer property
//
void ModeOnChanged(CustomProperty in_prop)
{
   int mode = (int)ParAcc_GetValue(in_prop, L"mode", DBL_MAX);
   in_prop.GetParameter(L"points_display_pcg").PutCapabilityFlag(siReadOnly, mode != eDrawMode_Points);
}


// Layout event handler for use_per_procedural_parameters
//
// @param in_prop   The SITOA_Viewer property
//
void UsePerProceduralParametersOnChanged(CustomProperty in_prop)
{
   bool usePerProceduralParameters = (bool)ParAcc_GetValue(in_prop, L"use_per_procedural_parameters", DBL_MAX);

   in_prop.GetParameter(L"mode").PutCapabilityFlag(siReadOnly, usePerProceduralParameters); 
   in_prop.GetParameter(L"size").PutCapabilityFlag(siReadOnly, usePerProceduralParameters); 
   in_prop.GetParameter(L"random_colors").PutCapabilityFlag(siReadOnly, usePerProceduralParameters); 

   if (usePerProceduralParameters)
   {
      in_prop.GetParameter(L"points_display_pcg").PutCapabilityFlag(siReadOnly,  true); 
      in_prop.GetParameter(L"seed").PutCapabilityFlag(siReadOnly,  true); 
      in_prop.GetParameter(L"colorR").PutCapabilityFlag(siReadOnly, true); 
   }
   else
   {
      ModeOnChanged(in_prop);
      RandomColorsOnChanged(in_prop);
   }
}


SITOA_CALLBACK SITOA_ViewerProperty_PPGEvent(const CRef& in_ctxt)
{
   PPGEventContext ctxt(in_ctxt);
   PPGEventContext::PPGEvent eventID = ctxt.GetEventID();
   if (eventID == PPGEventContext::siParameterChange)
   {
      Parameter paramChanged = ctxt.GetSource();
      CString paramName = paramChanged.GetScriptName(); 
      CustomProperty prop = paramChanged.GetParent();

      if (paramName == L"random_colors")
         RandomColorsOnChanged(prop);
      else if (paramName == L"use_per_procedural_parameters")
         UsePerProceduralParametersOnChanged(prop);
      else if (paramName == L"mode")
         ModeOnChanged(prop);
   }

   return CStatus::OK;
}


