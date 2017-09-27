/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <set>

#include <xsi_string.h>

#define TRANSLATOR_WIN_TO_LINUX     0
#define TRANSLATOR_LINUX_TO_WIN     1

using namespace std;
using namespace XSI;

// Simple class for managing path strings
class CPathString : public CString
{
public:
   CPathString() {}
   CPathString(const CString &in_s)     : CString(in_s) {}
   CPathString(const CPathString &in_s) : CString(in_s) {}
   CPathString(const wchar_t *in_s)     : CString(in_s) {}

   ~CPathString() {}

   // Resolve the xsi token (for instance [Frame]) at in_frame and returns the result
   CPathString ResolveTokens(double in_frame, CString in_extraToken=L"");
   // Resolve (in place) the xsi token (for instance [Frame]) at in_frame
   void ResolveTokensInPlace(double in_frame, CString in_extraToken=L"");
   // Resolve the path, in case we are migrating a scene between windows and linux
   CPathString ResolvePath();
   // Resolve (in place) the path, in case we are migrating a scene between windows and linux
   void ResolvePathInPlace();
   // return true is the path is a sequence, ie containing the [Frame] token
   bool IsSequence();
   // return true if the path ends by .ass or by .ass.gz
   bool IsAss();
   // return true if the path ends by .obj or by .obj.gz
   bool IsObj();
   // return true if the path ends by .ply or by .ply.gz
   bool IsPly();
   // return true if the path ends by .dll (win) or by .so (linux)
   bool IsSo();
   // return true if the path has a valid extension for precedurals
   bool IsProcedural();
   // substitute .ass (or .ass.gz) with .asstoc
   CPathString GetAssToc();
   // return true if this is an empty string, else false
   bool IsVoid();
   // Compute the number of chars for the root of this path.
   int GetNbStartingChars(bool in_windowsStart, bool in_windowsSlash);
   // Given the in_directory directory, returns the relative path
   CPathString GetRelativeFilename(CPathString in_directory, bool in_windowsSlash);
   // If ending by in_slash, remove it
   void RemoveTrailingSlashInPlace(const char in_slash);
   // Resolve and env variable at the start of the string, for instance [doh]myass,
   CPathString ResolveStartingEnvVar();
};


// Class used to manage the searchpath parameters
class CSearchPath : public CString
{
private:
   // the paths. For instance if initialized by "C:\temp;C:\dev" -> m_paths[0]=="C:\temp", m_paths[1]=="C:\dev"
   vector <CPathString> m_paths;
   // a copy of m_paths, with the starting [env] token (if any) resolved
   vector <CPathString> m_envResolvedPaths;

public:
   CSearchPath() { }
   CSearchPath(const CString &in_s) : CString(in_s) {}

   ~CSearchPath() 
   {
      m_paths.clear();
      m_envResolvedPaths.clear();
   }

   // initialize and build the paths vector (if the string is made of directories separated by ; (win) or : (linux)
   void        Put(const CString &in_s, bool in_checkExistence);
   // return the size of m_paths
   int         GetCount();
   // is this a multiple directory path?
   bool        IsMultiple();
   // return the env-resolved searchpaths
   bool        GetPaths(vector <CPathString> &out_paths);
   // translate (w2l or l2w) all the paths
   CPathString Translate();
   // load the arnold plugins from each entry of m_envResolvedPaths
   void        LoadPlugins();
   // clears the vectors
   void        Clear();
   // log the paths by AiMsgDebug
   void        Log(); 
   // log the paths by AiMsgDebug
   void        LogDebug(const CString in_searchPathName);
};


// Simple class for managing picture sequences strings, eg "seq.[1..10;3].png"
class CImgSequencePathString
{
private:
   bool m_isValid;  // valid syntax for the constructor?
   int  m_start;    // start frame ("seq.[1..10;3].png" -> 1)
   int  m_end;      // end frame ("seq.[1..10;3].png" -> 10)
   int  m_padding;  // padding ("seq.[1..10;3].png" -> 3)
                    // m_padding is 1 if the sequence string is simply "seq.[1..10].png"
   CString m_path;      // the path ("seq.[1..10;3].png")
   CString m_basePath;  // the path until the first [ ("seq.[1..10;3].png" -> "seq.")
   CString m_endPath;   // the path after the last ] ("seq.[1..10;3].png" -> ".png")

public:
   CImgSequencePathString() 
   { 
      m_isValid = false;
      m_start = m_end = m_padding = 0;
   }
   // construct by CString
   CImgSequencePathString(CString in_s);

   ~CImgSequencePathString() {}
   // return if this is a valid sequence
   bool IsValid();
   // resolve the sequence path at frame in_frame
   CString ResolveAtFrame(int in_frame);
   // resolve the sequence path at the start frame
   CString ResolveAtStartFrame();
   // resolve the sequence path at the end frame
   CString ResolveAtEndFrame();
   // log the members (debug)
   void Log();
};


typedef pair<string, string> pairMap;   
typedef map<string, string> pathMap;

// Class for Path Translations
class CPathTranslator
{
public: 
   
   // Initializes the Path Translator mapping table
   static bool Initialize(const char* linktabFile, unsigned int mode);
   // Destroy Path Translator mapping table
   static void Destroy();
   // Initializes the Path Translator mapping tables
   static const char* TranslatePath(const char* in_path, bool in_replaceExtensionWithTX);
   // return true if the pt was initialized (so a linktab file was found etc etc)
   static bool IsInitialized();
   // return the translation mode (win->linux by default, since we write slashed pathes from windows)
   static unsigned int GetTranslationMode();

private:

   // Custom implementation of strlwr 
   static char* strlwr(char* s);
   // Map to allocate the pairs
   static pathMap m_PathMap;
   // If it is Initialized
   static bool m_Initialized;
   // Mode of Translation
   static unsigned int m_TranslationMode;
};


// Simple class for managing the missing shaders error messages
class CMissingShaderMap
{
public:
   CMissingShaderMap() { }
   ~CMissingShaderMap() 
   {
      m_shaders.clear();
   }
   // Return whether in_shader is in the set already
   bool Find(const CString in_shader);
   // Add in_shader to the set
   bool Add(const CString in_shader);
   // Clear the set
   void Clear();

private:
   set <CString> m_shaders;
};



