/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

// Utility functions shared by different shaders
#include <sstream>
#include <cstdio>
#include <cstring>

using namespace std;

#define BACKUP_SG_UVS \
	float _orig_u(sg->u), _orig_v(sg->v);	\
	float _orig_dudx(sg->dudx), _orig_dudy(sg->dudy), _orig_dvdx(sg->dvdx), _orig_dvdy(sg->dvdy);

#define RESTORE_SG_UVS \
   sg->u = _orig_u; \
   sg->v = _orig_v; \
   sg->dudx = _orig_dudx; \
   sg->dudy = _orig_dudy; \
   sg->dvdx = _orig_dvdx; \
   sg->dvdy = _orig_dvdy;

/** Computes UV texture coordinates, taking into account wrapping, alternate
 *  and repeat modes
 *
 * \param[in,out] u        Original u coordinate to be modified
 * \param[in,out] v        Original v coordinate to be modified
 * \param         repeats  Tiling Scale of the texture 
 * \param         min      The uv min value will be remaped to min
 * \param         max      The uv max value will be remaped to max
 * \param         torus_u  Use wrapping in u
 * \param         torus_v  Use wrapping in v
 * \param         alt_u    Mirrors the tiling in x
 * \param         alt_v    Mirrors the tiling in y
 * \param         sg       Shader globals for derivatives access (sg->dudx, ...)
 */
void compute_uv( float &u, float &v,
                 const AtVector& repeats, 
                 const AtVector& min, 
                 const AtVector& max, 
                 const bool torus_u, const bool torus_v,
                 const bool alt_u, const bool alt_v,
                  AtShaderGlobals *sg);


/** Computes UV texture coordinates, taking into account wrapping, alternate
 *  and repeat modes
 *
 * \param[in,out] u        Original u coordinate to be modified
 * \param[in,out] v        Original v coordinate to be modified
 * \param         repeats  Tiling Scale of the texture 
 * \param         min      The uv min value will be remaped to min
 * \param         max      The uv max value will be remaped to max
 * \param         torus_u  Use wrapping in u
 * \param         torus_v  Use wrapping in v
 * \param         alt_u    Mirrors the tiling in x
 * \param         alt_v    Mirrors the tiling in y
 * \param         dudx     U derivative wrt screen X-axis (not normalized)
 * \param         dudy     U derivative wrt screen Y-axis (not normalized)
 * \param         dvdx     V derivative wrt screen X-axis (not normalized)
 * \param         dvdy     V derivative wrt screen Y-axis (not normalized)
 */
void compute_uv( float &u, float &v,
                 const AtVector& repeats, 
                 const AtVector& min, 
                 const AtVector& max, 
                 const bool torus_u, const bool torus_v,
                 const bool alt_u, const bool alt_v,
                 float &dudx, float &dudy, 
                 float &dvdx, float &dvdy );

void compute_uv( float &u, float &v,
                 const AtVector& repeats, 
                 const AtVector& min, 
                 const AtVector& max, 
                 const bool torus_u, const bool torus_v,
                 const bool alt_u, const bool alt_v);


/** Computes UVW texture coordinates, taking into account wrapping, alternate
 *   repeat and offset modes
 *
 * \param[in,out] u        Original u coordinate to be modified
 * \param[in,out] v        Original v coordinate to be modified
 * \param[in,out] w        Original w coordinate to be modified
 * \param         repeats  Tiling Scale of the texture 
 * \param         min      The uvw min value will be remaped to min
 * \param         max      The uvw max value will be remaped to max
 * \param         torus_u  Use wrapping in u
 * \param         torus_v  Use wrapping in v
 * \param         torus_z  Use wrapping in w
 * \param         alt_u    Mirrors the tiling in x
 * \param         alt_v    Mirrors the tiling in y
 * \param         alt_z    Mirrors the tiling in z
 * \param         dudx     U derivative wrt screen X-axis (not normalized)
 * \param         dudy     U derivative wrt screen Y-axis (not normalized)
 * \param         dvdx     V derivative wrt screen X-axis (not normalized)
 * \param         dvdy     V derivative wrt screen Y-axis (not normalized)
 */
void compute_uvw( float &u, float &v, float &w,
                  const AtVector& repeats, 
                  const AtVector& min, 
                  const AtVector& max, 
                  const bool torus_u, const bool torus_v, const bool torus_z,
                  const bool alt_u, const bool alt_v, const bool alt_z,
                  const AtVector offset,
                  float &dudx, float &dudy, 
                  float &dvdx, float &dvdy );

void compute_uvw( float &u, float &v, float &w,
                  const AtVector& repeats, 
                  const AtVector& min, 
                  const AtVector& max, 
                  const bool torus_u, const bool torus_v, const bool torus_z,
                  const bool alt_u, const bool alt_v, const bool alt_z,
                  const AtVector offset);

// Class for resolving the Mari <udim> and Mudbox <tile> token

class CTokenFilename
{
   enum eTokenModes
   {
      NONE = 0,
      UDIM,
      TILE
   };

private:
   const char* m_filename; // the original filename
   short int   m_tagStart; // start offset of the <udim> token in the filename string (-1 for no udim)
   short int   m_tagEnd;   // end index of the <udim> token
   short int   m_dim;      // the number following the udim in cases such as <udim:100>. Default is 10
   uint8_t     m_mode;     // udim or tile ?
   bool        m_isValid;  // true if m_filename is a valid filename, else false

public:
   CTokenFilename() :
      m_tagStart(0), m_tagEnd(0), m_dim(10), m_mode(NONE), m_isValid(false)
   {}

   CTokenFilename(const char* in_filename) : 
      m_filename(in_filename), m_tagStart(0), m_tagEnd(0), m_dim(10), m_mode(NONE), m_isValid(false)
   {}

   CTokenFilename(const CTokenFilename &in_arg) : 
      m_filename(in_arg.m_filename), m_tagStart(in_arg.m_tagStart), m_tagEnd(in_arg.m_tagEnd),
      m_dim(in_arg.m_dim), m_mode(in_arg.m_mode), m_isValid(in_arg.m_isValid)
   {}

   ~CTokenFilename() {}

   // return true if m_filename is a valid <udim> or <tile> filename, else false
   bool IsValid() { return m_isValid; }
   // initialize the class memebrs and check if this actually a valid <udim> or <tile> filename.
   bool Init(const char* in_filename = NULL);
   // return the resolved <udim> or <tile> string depending on the input u,v, 
   // and the new u,v to be used to look up the resolved texture.
   const char* Resolve(const AtShaderGlobals *in_sg, float &io_u, float &io_v);
   // log the class members, for debugging purposes
   void Log();
};

inline float MAP01F(float a)
{
   return (a + 1.0f) * 0.5f;
}

const char* GetShaderOwnerName(const AtShaderGlobals *in_sg);


