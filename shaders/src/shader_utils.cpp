/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <ai.h>
#include "shader_utils.h"


void transform_1D( float &coord,
                   const float repeats,
                   const float min,
                   const float max,
                   const float offset,
                   const bool torus,
                   const bool alt,
                   float &dx,
                   float &dy )
{
   if (torus)
      coord -= floor(coord);

   // repeats
   if (repeats && coord>=0 && coord<1)
   {
      coord *= repeats;
      dx *= repeats;
      dy *= repeats;

      // alt
      if (alt)
      {
         int i = (int)floor(coord);
         if (!(i & 1 ))
            coord = 2*i + 1 - coord;
      }
      coord -= floor(coord);
   }

   // remap
   if (min != max)
   {
      float delta = (max - min);
      coord = min + coord * delta;
      dx *= delta;
      dy *= delta;
   }

   // offset 
   coord += offset;
}


void compute_uv( float &u, float &v,
                 const AtVector& repeats, 
                 const AtVector& min, 
                 const AtVector& max, 
                 const bool torus_u, const bool torus_v,
                 const bool alt_u, const bool alt_v,
                 AtShaderGlobals *sg)
{
   transform_1D( u, repeats.x, min.x, max.x, 0, torus_u, alt_u, sg->dudx, sg->dudy );
   transform_1D( v, repeats.y, min.y, max.y, 0, torus_v, alt_v, sg->dvdx, sg->dvdy );
}


void compute_uv( float &u, float &v,
                 const AtVector& repeats, 
                 const AtVector& min, 
                 const AtVector& max, 
                 const bool torus_u, const bool torus_v,
                 const bool alt_u, const bool alt_v,
                 float &dudx, float &dudy, 
                 float &dvdx, float &dvdy )
{
   transform_1D( u, repeats.x, min.x, max.x, 0, torus_u, alt_u, dudx, dudy );
   transform_1D( v, repeats.y, min.y, max.y, 0, torus_v, alt_v, dvdx, dvdy );
}

void compute_uv( float &u, float &v,
                 const AtVector& repeats, 
                 const AtVector& min, 
                 const AtVector& max, 
                 const bool torus_u, const bool torus_v,
                 const bool alt_u, const bool alt_v)
{
   float dummy = 0.0f;
   transform_1D( u, repeats.x, min.x, max.x, 0, torus_u, alt_u, dummy, dummy );
   transform_1D( v, repeats.y, min.y, max.y, 0, torus_v, alt_v, dummy, dummy );
}

void compute_uvw( float &u, float &v, float &w,
                  const AtVector& repeats, 
                  const AtVector& min, 
                  const AtVector& max, 
                  const bool torus_u, const bool torus_v, const bool torus_w,
                  const bool alt_u, const bool alt_v, const bool alt_w,
                  const AtVector offset,
                  float &dudx, float &dudy,
                  float &dvdx, float &dvdy )
{
   float dummy = 0.0f;
   transform_1D( u, repeats.x, min.x, max.x, offset.x, torus_u, alt_u, dudx, dudy );
   transform_1D( v, repeats.y, min.y, max.y, offset.y, torus_v, alt_v, dvdx, dvdy );
   transform_1D( w, repeats.z, min.z, max.z, offset.z, torus_w, alt_w, dummy, dummy );
}

void compute_uvw( float &u, float &v, float &w,
                  const AtVector& repeats, 
                  const AtVector& min, 
                  const AtVector& max, 
                  const bool torus_u, const bool torus_v, const bool torus_w,
                  const bool alt_u, const bool alt_v, const bool alt_w,
                  const AtVector offset)
{
   float dummy = 0.0f;
   transform_1D( u, repeats.x, min.x, max.x, offset.x, torus_u, alt_u, dummy, dummy );
   transform_1D( v, repeats.y, min.y, max.y, offset.y, torus_v, alt_v, dummy, dummy );
   transform_1D( w, repeats.z, min.z, max.z, offset.z, torus_w, alt_w, dummy, dummy );
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
// Class for resolving the Mari <udim> and Mudbox <tile> token
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

static const AtString s_void_uv_set_name; // let void for main set

// initialize the class memebrs and check if this actually a valid <udim> or <tile> filename.
//
// @return true if this is a valid <udim> filename
//
bool CTokenFilename::Init(const char* in_filename)
{
   if (in_filename)
      m_filename = in_filename;

   const char* udim = strstr(m_filename, "<udim");
   if (!udim) udim = strstr(m_filename, "<UDIM");

   if (udim)
   {
      // Find the end of the udim specification
      const char* udim_end = strstr(udim, ">");
      if (udim_end)
      {
         m_mode = UDIM;
         m_tagStart = (short int)(udim - m_filename);
         m_tagEnd = (short int)((udim_end + 1) - m_filename); 
         // Are we using a nonstandard udim spec?  (E.g. <udim:100>)
         if (udim[5] == ':')
         {
            int dim = atoi(udim + 6);
            // atoi returns zero if it can't parse a number; use defaults if it fails
            if (dim > 0)
               m_dim = (short int)dim;
         }

         m_isValid = m_tagStart >= 0;
      }
   }
   else
   {
      const char *tile = strstr(m_filename, "<tile");
      if (tile)
      {
         const char* tile_end = strstr(tile, ">");
         if (tile_end)
         {
            m_mode = TILE;
            m_tagStart = (short int)(tile - m_filename);
            m_tagEnd = (short int)((tile_end + 1) - m_filename); 
            m_isValid = m_tagStart >= 0;
         }
      }
   }

   return m_isValid;
}


// if a UDIM lookup is near a tile's edge, use the triangle's centroid to
// decide on which side of the edge is the tile we should load
static inline void AdjustUDIMLookup(const AtShaderGlobals *sg,
                                    float &udim_u, float &udim_v,
                                    int &col, int &row,
                                    float eps,
                                    int udim_dim)
{
   if (udim_u < eps || 1 - udim_u < eps || udim_v < eps || 1 - udim_v < eps)
   {
      AtVector2 uvs[3];
      if (AiShaderGlobalsGetVertexUVs(sg, s_void_uv_set_name, uvs))
      {
         float centroid_u = (uvs[0].x + uvs[1].x + uvs[2].x) * (1 / 3.0f);
         float centroid_v = (uvs[0].y + uvs[1].y + uvs[2].y) * (1 / 3.0f);
         int row_centroid = static_cast <int> (ceilf(centroid_v) - 1 );
         int col_centroid = static_cast <int> (ceilf(centroid_u) - 1 );
         row_centroid = AiMax(row_centroid, 0);
         col_centroid = AiClamp(col_centroid, 0, udim_dim - 1);

         if (udim_u < eps && col_centroid < col)
         {
            col = col - 1;
            udim_u = 1;
         }
         else if (1 - udim_u < eps && col_centroid > col)
         {
            col = col + 1;
            udim_u = 0;
         }
         if (udim_v < eps && row_centroid < row)
         {
            row = row - 1;
            udim_v = 1;
         }
         else if (1 - udim_v < eps && row_centroid > row)
         {
            row = row + 1;
            udim_v = 0;
         }
      }
   }
}

// Same as above, but for TILE-ed images
static inline void AdjustTILELookup(const AtShaderGlobals *sg,
                                    float &udim_u, float &udim_v,
                                    int &col, int &row,
                                    float eps)
{
   if (udim_u < eps || 1 - udim_u < eps || udim_v < eps || 1 - udim_v < eps)
   {
      AtVector2 uvs[3];
      if (AiShaderGlobalsGetVertexUVs(sg, s_void_uv_set_name, uvs))
      {
         float centroid_u = (uvs[0].x + uvs[1].x + uvs[2].x) * (1 / 3.0f);
         float centroid_v = (uvs[0].y + uvs[1].y + uvs[2].y) * (1 / 3.0f);
         int row_centroid = static_cast <int> (ceilf(centroid_u));
         int col_centroid = static_cast <int> (ceilf(centroid_v));
         row_centroid = AiMax(row_centroid, 1);
         col_centroid = AiMax(col_centroid, 1);

         if (udim_u < eps && row_centroid < row)
         {
            row = row - 1;
            udim_u = 1;
         }
         else if (1 - udim_u < eps && row_centroid > row)
         {
            row = row + 1;
            udim_u = 0;
         }
         if (udim_v < eps && col_centroid < col)
         {
            col = col - 1;
            udim_v = 1;
         }
         else if (1 - udim_v < eps && col_centroid > col)
         {
            col = row + 1;
            udim_v = 0;
         }
      }
   }
}

// return the resolved <udim> or <tile> string depending on the input u,v, 
// and the new u,v to be used to look up the resolved texture.
//
// @param in_sg         the shader globals, or NULL if called at update time
// @param io_u          the input u coordinate
// @param io_v          the input v coordinate
//
// @return the resolved filename (must be freed by the caller) else NULL
//
const char* CTokenFilename::Resolve(const AtShaderGlobals *in_sg, float &io_u, float &io_v)
{
   if (!m_isValid)
      return NULL;

   uint32_t filenameLen = (uint32_t)strlen(m_filename);
   // alloc the result strings, make it a little bigger than just the filename length
   // For example a dummy<tile>.tx could expand to dummy_uXXXX_vYYYY.tx
   // If during eval, allocate by AiShaderGlobalsQuickAlloc, and the caller must NOT free the returned buffer
   char *out_name = in_sg ? (char*)AiShaderGlobalsQuickAlloc(in_sg, filenameLen + 10) : (char*)AiMalloc(filenameLen + 10);
    // head: result == "dummy"
   memcpy(out_name, m_filename, m_tagStart);

   int row, col;
   if (m_mode == UDIM)
   {
      row = static_cast <int> (ceilf(io_v) - 1);
      col = static_cast <int> (ceilf(io_u) - 1);
   }
   else
   {
      row = static_cast <int> (ceilf(io_u));
      col = static_cast <int> (ceilf(io_v));
   }

   io_u = fmodf(io_u, 1.0f);
   io_v = fmodf(io_v, 1.0f);

   if (m_mode == UDIM)
   {  
      if (col < 0)
      {
         col = 0;
         io_u = 0.0f;
      }
      else if (col >= m_dim)
      {
         col = m_dim - 1;
         io_u = 1.0f;
      }

      if (row < 0)
      {
         row = 0;
         io_v = 0.0f;
      }

      if (in_sg)
      {
         float eps = m_dim / 65536.0f;
         AdjustUDIMLookup(in_sg, io_u, io_v, col, row, eps, m_dim);
      }

      // tail: result = "dummy    .tx"
      memcpy(out_name + m_tagStart + 4, m_filename + m_tagEnd, filenameLen + 1 - m_tagEnd);

      unsigned int index = 1001 + col + (row * m_dim);
      // these are the only chars that need to be overwritten.
      // the head and tail of the resolved name is alread set at Init time
      out_name[m_tagStart + 3] = '0' + index % 10; index /= 10;
      out_name[m_tagStart + 2] = '0' + index % 10; index /= 10;
      out_name[m_tagStart + 1] = '0' + index % 10; index /= 10;
      out_name[m_tagStart + 0] = '0' + index % 10;
   }
   else // TILE
   {  
      if (col < 1)
      {
         col = 1;
         io_v = 0.0f;
      }

      if (row < 1)
      {
         row = 1;
         io_u = 0.0f;
      }

      if (in_sg)
      {
         float eps = 10.0f / 65536.0f;
         AdjustTILELookup(in_sg, io_u, io_v, col, row, eps);
      }

      // tail: result = "dummy      .tx"
      memcpy(out_name + m_tagStart + 6, m_filename + m_tagEnd, filenameLen + 1 - m_tagEnd);

      if (row < 10 && col < 10)
      {
         out_name[m_tagStart + 0] = '_';
         out_name[m_tagStart + 1] = 'u';
         out_name[m_tagStart + 2] = '0' + row;
         out_name[m_tagStart + 3] = '_';
         out_name[m_tagStart + 4] = 'v';
         out_name[m_tagStart + 5] = '0' + col;
      }
      else
      {
         // example: u>9 -> row==10. Our middle string will then be _u10_v1
         // We can't use the preallocated m_result string,
         // because it has only 6 chars available, and we need 7.
         // Let's first compute the length of the string (assuming we're not going over 9999)
         int length = 4; // _u + _v
         // add the number of digits
         if (row < 10)        length+=1;
         else if (row < 100)  length+=2;
         else if (row < 1000) length+=3;
         else                 length+=4;

         if (col < 10)        length+=1;
         else if (col < 100)  length+=2;
         else if (col < 1000) length+=3;
         else                 length+=4;

         char *s = (char*)alloca(length+1);
         sprintf(s, "_u%d_v%d", row, col);
         // only the head of m_result stays the same. Let' overwrite the middle
         memcpy(out_name + m_tagStart, s, length);
         // and the tail
         memcpy(out_name + m_tagStart + length, m_filename + m_tagEnd, filenameLen + 1 - m_tagEnd);
      }
   }

   return out_name;
}


// log the class members, for debugging purposes
void CTokenFilename::Log()
{
   fprintf(stderr, "----- CTokenFilename log: -----\n");

   fprintf(stderr, "m_isValid      = %s\n", m_isValid ? "True" : "False");
   fprintf(stderr, "m_filename     = %s\n", m_filename);
   fprintf(stderr, "m_tagStart     = %d\n", m_tagStart);
   fprintf(stderr, "m_tagEnd       = %d\n", m_tagEnd);
   fprintf(stderr, "m_dim          = %d\n", m_dim);
}


// Return the name of the owner of the shader.
// Usually, the node is sg->Op, but in case of a ginstance (which has its own sg->Op), 
// we want the name of the master node, which is the actual owner of the shader.
// In SItoA we name the ginstance with spaces, being the last token of the string the master node.
const char* GetShaderOwnerName(const AtShaderGlobals *in_sg)
{
   if (!in_sg->Op)
      return NULL;

   const char* name = AiNodeGetName(in_sg->Op);
   const char* p = name + strlen(name) - 1;
   while (p > name)
   {
      if (*p == ' ')
         return p + 1;
      p--;
   }

   return name;
}

