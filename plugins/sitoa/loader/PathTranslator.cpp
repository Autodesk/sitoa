/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "common/Tools.h"
#include "loader/PathTranslator.h"
#include "renderer/Renderer.h"

#include <xsi_project.h>
#include <xsi_scene.h>

#include <ctype.h>
#include <string.h>
#include <sstream>
#include <sys/stat.h>


// Resolve the xsi token (for instance [Frame]) at in_frame and returns the result
CPathString CPathString::ResolveTokens(double in_frame, CString in_extraToken)
{
   if (!in_extraToken.IsEmpty())
   {
      if (in_extraToken == L"[Pass]")
      {
         Pass pass(Application().GetActiveProject().GetActiveScene().GetActivePass());
         *this = CStringUtilities().ReplaceString(in_extraToken, pass.GetName(), *this);
      }
   }
   return CUtils::ResolveTokenString(*this, CTime(in_frame), false);
}

// Resolve (in place) the xsi token (for instance [Frame]) at in_frame
void CPathString::ResolveTokensInPlace(double in_frame, CString in_extraToken)
{
   *this = ResolveTokens(in_frame, in_extraToken);
}

// Resolve the path, in case we are migrating a scene between windows and linux
CPathString CPathString::ResolvePath()
{
   return CUtils::ResolvePath(*this);
}

// Resolve (in place) the path, in case we are migrating a scene between windows and linux
void CPathString::ResolvePathInPlace()
{
   *this = ResolvePath();
}

// Return true is the path is a sequence, ie containing the [Frame] token
bool CPathString::IsSequence()
{
   return this->FindString(L"[Frame") > 0;
}


// return true if the path ends by .ass or by .ass.gz
bool CPathString::IsAss()
{
   ULONG index = this->ReverseFindString(L".ass", UINT_MAX);
   if (index == this->Length() - 4)
      return true;
   index = this->ReverseFindString(L".ass.gz", UINT_MAX);
   if (index == this->Length() - 7)
      return true;
   return false;
}


// return true if the path ends by .obj or by .obj.gz
bool CPathString::IsObj()
{
   ULONG index = this->ReverseFindString(L".obj", UINT_MAX);
   if (index == this->Length() - 4)
      return true;
   index = this->ReverseFindString(L".obj.gz", UINT_MAX);
   if (index == this->Length() - 7)
      return true;
   return false;
}


// return true if the path ends by .ply or by .ply.gz
bool CPathString::IsPly()
{
   ULONG index = this->ReverseFindString(L".ply", UINT_MAX);
   if (index == this->Length() - 4)
      return true;
   return false;
}


// return true if the path ends by .dll (win) or by .so (linux)
bool CPathString::IsSo()
{
   if (CUtils::IsWindowsOS())
   {
      ULONG index = this->ReverseFindString(L".dll", UINT_MAX);
      if (index == this->Length() - 4)
         return true;
   }
   else
   {
      ULONG index = this->ReverseFindString(L".so", UINT_MAX);
      if (index == this->Length() - 3)
         return true;
   }
   return false;
}


// return true if the path has a valid extension for precedurals
bool CPathString::IsProcedural()
{
   return IsAss() || IsObj() || IsPly() || IsSo();
}


// substitute .ass (or .ass.gz) with .asstoc
CPathString CPathString::GetAssToc()
{
   CString s;
   CPathString result(L"");

   ULONG index = this->ReverseFindString(L".ass", UINT_MAX);
   if (index == this->Length() - 4)
   {
      s = this->GetSubString(0, index);
      result = s + L".asstoc";
   }
   else
   {
      index = this->ReverseFindString(L".ass.gz", UINT_MAX);
      if (index == this->Length() - 7)
      {
         s = this->GetSubString(0, index);
         result = s + L".asstoc";
      }
   }
   return result; 
}


// return true if this is an empty string, else false
bool CPathString::IsVoid()
{
   ULONG length = this->Length();
   if (length == 0)
      return true;

   // also return true if the string is all made of spaces
   for (ULONG i=0; i<length; i++)
   {
      if (this->GetAt(i) != ' ')
         return false;
   }
   return true;
}


// Compute the number of chars for the root of this path.
// "C:\..." -> 3, "\\disk..." -> 2, "/usr..." -> 1, or 0 in case of other (invalid) patterns
//
// @param in_windowsSlash   if on, manage '\', else '/'
//
int CPathString::GetNbStartingChars(bool in_windowsStart, bool in_windowsSlash)
{
   // int startNbChar=0;
   char c0, c1, c2;
   
   int length = (int)this->Length();

   int minLength = in_windowsStart ? 2 : 1; // win can start with "\\" or "c:\"

   if (length < minLength)
      return 0;

   c0 = this->GetAt(0);

   if (!in_windowsStart)
   {
      if (c0 == '/')
         return 1; // '/'
      else
         return 0; // error
   }

   // in_windowsStart true, 

   c1 = this->GetAt(1);

   if (in_windowsSlash)
   {
      if (c0 == '\\' && c1 == '\\') //UNC PATH
         return 2;
      else if (length > 2)
      {
         c2 = this->GetAt(2);
         if (c1 == ':' && c2 == '\\')
            return 3; // for instance "C:\"
      }
   }
   else if (length > 2) // "C:/...
   {
      c2 = this->GetAt(2);
      if (c1 == ':' && c2 == '/')
         return 3; // for instance "C:\"
   }

   // if getting here, we failed

   return 0;
}


#define MAX_FILENAME_LEN 1024
// Added for #1162
// from http://www.codeguru.com/cpp/misc/misc/fileanddirectorynaming/article.php/c263
// Given the in_directory directory, returns the relative path (representing the class the absolute one).
// For example, if in_directory is C:\foo\bar and this is C:\foo\whee\text.txt is given,
// GetRelativeFilename returns ..\whee\text.txt.
// In Arnold, apparently we always use '/' as separator, so in_windowsSlash is always false,
// however we are ready to manage other cases
//
// @param in_directory     the directory
// @param in_windowsSlash  if on, use '\' as folder separator, else use '/'. That's always passed false now
//
// @return the relative path
// 
CPathString CPathString::GetRelativeFilename(CPathString in_directory, bool in_windowsSlash)
{
   bool isWindows = !(this->GetAt(0) == '/');

   // get rid of all the trailing \ or / at the end of the directory path
   const char theSlash = in_windowsSlash ? '\\' : '/';

   // if the path to be converted ends by / or \, return an error, we expect a file (not a dir) path
   if (this->GetAt(this->Length()-1) == theSlash)
   {
      // GetMessageQueue()->LogMsg(L"GetRelativeFilename: Wrong filename for " + *this, siErrorMsg);
      return CPathString(L"");
   }

   while (in_directory.GetAt(in_directory.Length()-1) == theSlash)
      in_directory = in_directory.GetSubString(0, in_directory.Length()-1);

   int relDirStartNbChar = in_directory.GetNbStartingChars(isWindows, in_windowsSlash);
   if (relDirStartNbChar == 0)
   {
      // GetMessageQueue()->LogMsg(L"GetRelativeFilename: Wrong format for " + in_directory, siErrorMsg);
      return CPathString(L"");
   }

   int fullPathStartNbChar = this->GetNbStartingChars(isWindows, in_windowsSlash);
   if (fullPathStartNbChar == 0)
   {
      // GetMessageQueue()->LogMsg(L"GetRelativeFilename: Wrong format for " + *this, siErrorMsg);
      return CPathString(L"");
   }

   if (relDirStartNbChar != fullPathStartNbChar)
   {
      // GetMessageQueue()->LogMsg(L"GetRelativeFilename: Mismatch in the number of starting chars", siErrorMsg);
      return CPathString(L"");
   }

   // in any case, the first char of the path must be the same,
   // since I don't know what to do to translate cases like C:\... to D:\...
   if (in_directory.GetAt(0) != this->GetAt(0))
   {
      // GetMessageQueue()->LogMsg(L"GetRelativeFilename: Different drive letter", siErrorMsg);
      return CPathString(L"");
   }

   // path are ok: same starting chars (for instance "C:\")
   // let's actually do the conversion

   int afMarker = 0, rfMarker = 0;
   int i = 0;
   int levels = 0;
   static char relativeFilename[MAX_FILENAME_LEN+1];
   CPathString result;

   const char *absoluteFilename = this->GetAsciiString();
   const char *currentDirectory = in_directory.GetAsciiString();

   CString slashS = in_windowsSlash ? L"\\" : L"/";

   char slashC = slashS.GetAt(0);

   int cdLen = (int)strlen(currentDirectory);
   int afLen = (int)strlen(absoluteFilename);

   // make sure the names are not too long or too short
   if (cdLen > MAX_FILENAME_LEN || cdLen < relDirStartNbChar+1 || 
       afLen > MAX_FILENAME_LEN || afLen < relDirStartNbChar+1)
      return CPathString(L"");

   // Handle DOS names that are on different drives:
   if (currentDirectory[0] != absoluteFilename[0])
   {
      // not on the same drive, so only absolute filename will do
      strcpy(relativeFilename, absoluteFilename);
      result.PutAsciiString(relativeFilename);
      return result;
   }

   // they are on the same drive, find out how much of the current directory
   // is in the absolute filename
   i = relDirStartNbChar;
   while (i < afLen && i < cdLen && currentDirectory[i] == absoluteFilename[i])
      i++;

   if (i == cdLen && (absoluteFilename[i] == slashC || absoluteFilename[i-1] == slashC))
   {
      // the whole current directory name is in the file name,
      // so we just trim off the current directory name to get the
      // current file name.
      if (absoluteFilename[i] == slashC)
         // a directory name might have a trailing slash but a relative
         // file name should not have a leading one...
         i++;

      strcpy(relativeFilename, &absoluteFilename[i]);
      result.PutAsciiString(relativeFilename);
      return result;
   }

   // The file is not in a child directory of the current directory, so we
   // need to step back the appropriate number of parent directories by
   // using "..\"s.  First find out how many levels deeper we are than the
   // common directory
   afMarker = i;
   levels = 1;

   // count the number of directory levels we have to go up to get to the
   // common directory
   while (i < cdLen)
   {
      i++;
      if (currentDirectory[i] == slashC)
      {
         // make sure it's not a trailing slash
         i++;
         if (currentDirectory[i] != '\0')
            levels++;
      }
   }

   // move the absolute filename marker back to the start of the directory name
   // that it has stopped in.
   while (afMarker > 0 && absoluteFilename[afMarker-1] != slashC)
      afMarker--;

   // check that the result will not be too long
   if (levels * 3 + afLen - afMarker > MAX_FILENAME_LEN)
      return CPathString(L"");

   // add the appropriate number of "..\"s.
   rfMarker = 0;
   for (i = 0; i < levels; i++)
   {
      relativeFilename[rfMarker++] = '.';
      relativeFilename[rfMarker++] = '.';
      relativeFilename[rfMarker++] = slashC;
   }

   // copy the rest of the filename into the result string
   strcpy(&relativeFilename[rfMarker], &absoluteFilename[afMarker]);
   result.PutAsciiString(relativeFilename);
   return result;
}


// If ending by in_slash, remove it
void CPathString::RemoveTrailingSlashInPlace(const char in_slash)
{
   if (this->GetAt(this->Length()-1) == in_slash)
      *this = this->GetSubString(0, this->Length()-1);
}


// Resolve and env variable at the start of the string, for instance [doh]mystring.
// If failing, for whatever reason, return the same input string
CPathString CPathString::ResolveStartingEnvVar()
{
   CPathString errorResult(*this);

   if (this->GetAt(0) != '[')
      return errorResult; // fail
   ULONG closingIndex = this->FindString(L"]");
   if (closingIndex == UINT_MAX)
      return errorResult; // fail
   
   CString envVar = this->GetSubString(1, closingIndex-1);
   // GetMessageQueue()->LogMsg(L"envVar = " + envVar);

   char *envVarValue = getenv(envVar.GetAsciiString());
   if (!envVarValue)
   {
      GetMessageQueue()->LogMsg(L"[sitoa] Cannot resolve the environment variable " + envVar, siWarningMsg);
      return errorResult; // fail
   }

   CPathString result(envVarValue);
   result+= this->GetSubString(closingIndex+1, this->Length());
   // GetMessageQueue()->LogMsg(L"ResolveStartingEnvVar: returning  " + result);
   return result;
}



/////////////////////////////////
// Class used to manage the searchpath parameters
/////////////////////////////////

// initialize and build the paths vector (if the string is made of directories separated by ; (win) or : (linux)
//
// @param in_s               the string to parse
// @param in_checkExistence  if true, check for the directory existence before accepting it as a valid path
// 
void CSearchPath::Put(const CString &in_s, bool in_checkExistence)
{
   *this = in_s;

   const char slash = CUtils::Slash().GetAt(0);
   // #1289: use ";" also in linux
   CStringArray sArray = in_s.Split(L";");
   LONG nbPaths = sArray.GetCount();

   CPathString path, envResolvedPath;

   for (LONG i=0; i<nbPaths; i++)
   {
      path = sArray[i];

      // #1228: get the env resolved path. For instance, if
      // path == [here]/there and "here" is an env var set to "C:\temp" then
      // envResolvedPath <- "C:\temp\there"
      // If ResolveStartingEnvVar fails for whatever reason, for instance because there is no
      // [..], or the env var does not exist, envResolvedPath is set equal to path
      // So, if path == "C:\temp\there", then also envResolvedPath <- "C:\temp\there"
      envResolvedPath = path.ResolveStartingEnvVar();

      path.RemoveTrailingSlashInPlace(slash);
      envResolvedPath.RemoveTrailingSlashInPlace(slash);

      // If already in vector, skip.
      // We used sets instead of vectors, so to avoid this "slower" search, but
      // the order of items would not be preserved (#1276).
      bool found = false;
      for (vector <CPathString>::iterator it=m_paths.begin(); it!=m_paths.end(); it++)
      {
         if (*it == path)
         {
            found = true;
            break;
         }
      }
      if (found)
         continue;

      if (in_checkExistence) // we must check the env resolved path
      {
         bool pathExists = CPathUtilities().PathExists(envResolvedPath.GetAsciiString());

         if (pathExists)
         {
            m_paths.push_back(path); // push the path as it is
            m_envResolvedPaths.push_back(envResolvedPath); // push the env resolved path
         }
         else
            GetMessageQueue()->LogMsg(L"[sitoa] Cannot find on disk the search path " + envResolvedPath, siWarningMsg);
      }
      else
      {
         // push them as they are
         m_paths.push_back(path);
         m_envResolvedPaths.push_back(envResolvedPath);
      }
   }
}


// return the size of m_paths
int CSearchPath::GetCount()
{
   return (int)m_paths.size();
}


// is this a multiple directory path?
bool CSearchPath::IsMultiple()
{
   return GetCount() > 1;
}


// return the env-resolved searchpaths
bool CSearchPath::GetPaths(vector <CPathString> &out_paths)
{
   if (GetCount() < 1)
      return false;

   out_paths = m_envResolvedPaths;
   return true;
}


// translate (w2l or l2w) all the paths
CPathString CSearchPath::Translate()
{
   CString separator;
   // first, decide the separator. 
   // If not using linktab, use ; for windows and : for linux, as needed from Arnold
   if (!CPathTranslator::IsInitialized())
      separator = CUtils::IsWindowsOS() ? L";" : L":";
   else
   {
      unsigned int pathTranslatorMode = CPathTranslator::GetTranslationMode();
      separator = pathTranslatorMode == TRANSLATOR_WIN_TO_LINUX ? L":" : L";";
   }

   CPathString result(L"");

   for (vector <CPathString>::iterator it=m_paths.begin(); it!=m_paths.end(); it++)
   {
      if (it != m_paths.begin())
         result+= separator;
      result+= (CString)CPathTranslator::TranslatePath(it->GetAsciiString(), false);
   }

   return result;
}


// load the arnold plugins from each entry of m_envResolvedPaths
void CSearchPath::LoadPlugins()
{
   for (vector <CPathString>::iterator it=m_envResolvedPaths.begin(); it!=m_envResolvedPaths.end(); it++)
   {
      // GetMessageQueue()->LogMsg("---------> Loading plugin from path " + *it);
      AiLoadPlugins(it->GetAsciiString());
   }
}


void CSearchPath::Clear()
{
   m_paths.clear();
   m_envResolvedPaths.clear();
}


// log the paths
void CSearchPath::Log()
{
   GetMessageQueue()->LogMsg(L"CSearchPath::Log");
   GetMessageQueue()->LogMsg(L" Path = " + *this + L" has " + CString((int)m_paths.size()) + L" paths");

   vector <CPathString>::iterator it;
   for (it=m_paths.begin(); it!=m_paths.end(); it++)
      GetMessageQueue()->LogMsg(L"  path = " + *it);
   for (it=m_envResolvedPaths.begin(); it!=m_envResolvedPaths.end(); it++)
      GetMessageQueue()->LogMsg(L"  env resolved path = " + *it);
}


// log the paths by AiMsgDebug
void CSearchPath::LogDebug(const CString in_searchPathName)
{
   CString s;

   if (m_paths.size() == 0)
   {
      s = L"[sitoa] No valid path for " + in_searchPathName;
      AiMsgDebug(s.GetAsciiString());
      return;
   }

   s = L"[sitoa] Paths for " + in_searchPathName + L":";
   AiMsgDebug(s.GetAsciiString());

   vector <CPathString>::iterator it;
   for (it=m_paths.begin(); it!=m_paths.end(); it++)
   {
      s = L" [sitoa] " + *it;
      AiMsgDebug(s.GetAsciiString());
   }

   s = L"[sitoa] Resolved paths for " + in_searchPathName + L":";
   AiMsgDebug(s.GetAsciiString());

   for (it=m_envResolvedPaths.begin(); it!=m_envResolvedPaths.end(); it++)
   {
      s = L" [sitoa] " + *it;
      AiMsgDebug(s.GetAsciiString());
   }
}


/////////////////////////////////
// Simple class for managing picture sequences strings, eg "seq.[1..10;3].png"
/////////////////////////////////

// construct by CString, and set all members
CImgSequencePathString::CImgSequencePathString(CString in_s)
{
   m_path = in_s;

   m_start = m_end = m_padding = 0;

   // ex: in_s = "seq.[1..10;3].png"
   ULONG dotIndex = m_path.ReverseFindString(L".", UINT_MAX);
   if (dotIndex == UINT_MAX)
   {
      m_isValid = false;
      return;
   }

   ULONG closingIndex = m_path.ReverseFindString(L"]", UINT_MAX);
   if (closingIndex == UINT_MAX)
   {
      m_isValid = false;
      return;
   }

   ULONG openingIndex = m_path.ReverseFindString(L"[", UINT_MAX);
   if (openingIndex == UINT_MAX)
   {
      m_isValid = false;
      return;
   }

   // the '.' is right after the ']'?
   m_isValid = dotIndex == closingIndex+1;

   if (!m_isValid)
      return;

   // m_basePath = in_s = "seq."
   m_basePath = m_path.GetSubString(0, openingIndex);

   m_endPath = m_path.GetSubString(dotIndex, m_path.Length());

   // framesS = in_s = "1..10;3"
   CString framesS = m_path.GetSubString(openingIndex+1, closingIndex-openingIndex-1);

   CString startEndS, paddindS;

   if (framesS.FindString(L";") == UINT_MAX) // no ';' ?
   {
      startEndS = framesS;
      paddindS = L"1";
   }
   else
   {
      CStringArray semiColonArray = framesS.Split(L";");
      startEndS = semiColonArray[0];
      paddindS = semiColonArray[1];
   }

   CStringArray startEndArray = startEndS.Split(L"..");
   if (startEndArray.GetCount() != 2)
   {
      m_isValid = false;
      return;
   }

   CString startS = startEndArray[0];
   CString endS   = startEndArray[1];

   m_start   = atoi(startS.GetAsciiString());
   m_end     = atoi(endS.GetAsciiString());
   m_padding = atoi(paddindS.GetAsciiString());
}


// return if this is a valid sequence
bool CImgSequencePathString::IsValid()
{
   return m_isValid;
}


// resolve the sequence path at frame in_frame
CString CImgSequencePathString::ResolveAtFrame(int in_frame)
{
   if (!m_isValid)
      return L"";
   if (in_frame < m_start || in_frame > m_end)
      return L"";
   // put in_frame into resultS
   ostringstream ossF;
   ossF << in_frame;
   CString resultS(ossF.str().c_str());

   // add the initial zeroes accoding the the padding
   switch (m_padding)
   {
      case 2:
         if (in_frame < 10)
            resultS = L"0" + resultS;
         break;
      case 3:
         if (in_frame < 10)
            resultS = L"00" + resultS;
         else if (in_frame < 100)
            resultS = L"0" + resultS;
         break;
      case 4:
         if (in_frame < 10)
            resultS = L"000" + resultS;
         else if (in_frame < 100)
            resultS = L"00" + resultS;
         else if (in_frame < 1000)
            resultS = L"0" + resultS;
         break;
      case 5:
         if (in_frame < 10)
            resultS = L"0000" + resultS;
         else if (in_frame < 100)
            resultS = L"000" + resultS;
         else if (in_frame < 1000)
            resultS = L"00" + resultS;
         else if (in_frame < 10000)
            resultS = L"0" + resultS;
         break;
      default: // should we support padding > 5 ?
         break;
   }

   resultS = m_basePath + resultS;
   resultS = resultS + m_endPath;

   return resultS;
}


// resolve the sequence path at the start frame
CString CImgSequencePathString::ResolveAtStartFrame()
{
   return ResolveAtFrame(m_start);
}


// resolve the sequence path at the end frame
CString CImgSequencePathString::ResolveAtEndFrame()
{
   return ResolveAtFrame(m_end);
}


// log the members (debug)
void CImgSequencePathString::Log()
{
   GetMessageQueue()->LogMsg(L"String            = " + m_path);
   GetMessageQueue()->LogMsg(L"Base Path         = " + m_basePath);
   GetMessageQueue()->LogMsg(L"End Path          = " + m_endPath);
   GetMessageQueue()->LogMsg(L"Start/End/Padding = " + (CString)m_start + L" " + (CString)m_end + L" " + (CString)m_padding);
}


////////////////////////////////////////////////////
////////////////////////////////////////////////////

pathMap CPathTranslator::m_PathMap; 
bool CPathTranslator::m_Initialized = false; 
unsigned int CPathTranslator::m_TranslationMode = TRANSLATOR_WIN_TO_LINUX; //win will write paths with the "/"

// Read the linktab file and store the path pairs.
//
// @param in_linktabFile   the linktab file path
// @param in_mode          win to linux or viceversa
//
// @return true if the file was found and its syntax correct, else false
//
bool CPathTranslator::Initialize(const char* in_linktabFile, unsigned int in_mode)
{
   if (!in_linktabFile)
      return false;

   if (!m_Initialized)
   {
      // Opening file to read the paths
      std::ifstream file(in_linktabFile);
      if (file.is_open())
      {
         string line;

         // Reading line by line
         unsigned int nline = 0;
         while (std::getline(file, line))
         {
            nline++;

            char *str = new char[line.length() + 1];
            strcpy(str,line.c_str());

            // Tokens (win & linux paths must be separated by a tab, like XSI specifies)
            char* winpattern = strtok(str, "\t");   //Tab
            char* linuxpattern = strtok(NULL, "");
           
            // If the "conversion" line is ok
            if (winpattern!=NULL && linuxpattern!=NULL)
            {
               // Converting to lower case win paths (case insensitive)
               CStringUtilities().ToLower(winpattern);

               if (in_mode == TRANSLATOR_WIN_TO_LINUX)
                  m_PathMap.insert(pairMap(string(winpattern), string(linuxpattern)));
               else
                  m_PathMap.insert(pairMap(string(linuxpattern), string(winpattern)));
            }
            else
            {
               CString filename;
               filename.PutAsciiString(in_linktabFile);
               GetMessageQueue()->LogMsg(L"[sitoa] Can't parse " + filename + L" at line " + CValue((LONG)nline).GetAsText() + L". Wrong file format", siErrorMsg);
            }
            
            delete[] str;
         }

         file.close();
         m_TranslationMode = in_mode;
         m_Initialized = true;
      }
      // Can't open file, returning false
      else
         return false;
   }
      
   // All ok, returning true
   return true;
}


const char* CPathTranslator::TranslatePath(const char* in_path, bool in_replaceExtensionWithTX)
{
   // As string
   string str(in_path);
   string strComparison(in_path);

   if (in_replaceExtensionWithTX)
   {
      // There a bit a mix of strings and CStrings after fixing #1212...

      size_t lastDotPos = str.find_last_of('.');

      bool doReplace = false;
      if (lastDotPos > 0)
      {
         string txPath;
         ULONG dotIndex;

         CImgSequencePathString sequencePath((CString)(str.c_str()));
         if (sequencePath.IsValid()) // we have a sequence, for instance "seq.[1..10;3].png"
         {
            // we check for the existance of the tx files for the first and last frame
            // as proposed for #1212
            doReplace = true;
            // start frame:
            CString startFramePath = sequencePath.ResolveAtStartFrame();
            if (startFramePath == L"")
               doReplace = false;
            else
            {
               dotIndex = startFramePath.ReverseFindString(L".", UINT_MAX);
               startFramePath = startFramePath.GetSubString(0, dotIndex);
               startFramePath = startFramePath + L".tx";
               // GetMessageQueue()->LogMsg(L"startFramePath  = " + startFramePath);
               if (!CPathUtilities().PathExists(startFramePath.GetAsciiString()))
                  doReplace = false;
            }
            if (doReplace) // survived the start frame existence
            {
               // end frame:
               CString endFramePath = sequencePath.ResolveAtEndFrame();
               if (endFramePath == L"")
                  doReplace = false;
               else
               {
                  dotIndex = endFramePath.ReverseFindString(L".", UINT_MAX);
                  endFramePath = endFramePath.GetSubString(0, dotIndex);
                  endFramePath = endFramePath + L".tx";
                  // GetMessageQueue()->LogMsg(L"End  = " + endFramePath);
                  if (!CPathUtilities().PathExists(endFramePath.GetAsciiString()))
                     doReplace = false;
               }
            }

            if (doReplace) // ok, both frames exist (note that others may miss!)
            {
               // "seq.[1..10;3].png" -> "seq.[1..10;3].tx"
               txPath = str.substr(0, lastDotPos);
               txPath.append(".tx");
               // GetMessageQueue()->LogMsg((CString)str.c_str() + " replaced by " + (CString)txPath.c_str());
               str = txPath;
            }
         }
         else // regular (single) path
         {
            // check if this is a <udim> or <tile> path, #1521
            size_t tokenPos(string::npos);
            bool isUdim(false), isTile(false);

            tokenPos = str.find("<udim>");
            isUdim = tokenPos != string::npos;
            if (!isUdim)
            {
               tokenPos = str.find("<tile>");
               isTile = tokenPos != string::npos;
            }

            if (isUdim || isTile) // if so, we'll check the existence of the path used in the 0..1 UV range
            {
               string expandedTokenStr = str;
               if (isUdim)
                  expandedTokenStr.replace(tokenPos, 6, "1001");
               else
                  expandedTokenStr.replace(tokenPos, 6, "_u1_v1");

               size_t expandedTokenlastDotPos = expandedTokenStr.find_last_of('.');
               txPath = expandedTokenStr.substr(0, expandedTokenlastDotPos);
               txPath.append(".tx");
               if (CPathUtilities().PathExists(txPath.c_str())) // the _u1_v1 or 1001 tx file exists, allow .tx for the original tokened path
               {
                  txPath = str.substr(0, lastDotPos);
                  txPath.append(".tx");
                  str = txPath;
               }
            }
            else
            {
               // replace the extension with "tx"
               txPath = str.substr(0, lastDotPos);
               txPath.append(".tx");

               // Attempt to get the file attributes
               if (CPathUtilities().PathExists(txPath.c_str()))
               {  // the tx file exists, use this path
                  // "seq.7.png" -> "seq.7.tx"
                  str = txPath;
               }
            }
         }
      }
   }

   // If it is initializated we will look for a translated path
   // Other code will be executed always to do a slash conversion
   // to avoid issues (ticket:366)
   if (m_Initialized)
   {
      // If mode is Win to Linux we will convert to lower case
      // for comparison (case insensitive)
      if (m_TranslationMode == TRANSLATOR_WIN_TO_LINUX)
      {
         unsigned int length = (unsigned int)strComparison.length();
         for (unsigned int i=0; i<length; ++i)
            strComparison[i] = (char)tolower(strComparison[i]);
      }
         
      for (pathMap::const_iterator it=m_PathMap.begin(); it!=m_PathMap.end(); it++)
      {
         if ((int)strComparison.find(it->first) == 0)
         {
            // We will maintain the letter cases from the original path, so we only 
            // are changing the "pattern" instead all path
            str = it->second + str.substr(it->first.length());
            break;
         }
      }
   }

   // Slashs
   char slash_orig, slash_dest;
   
   if (m_TranslationMode == TRANSLATOR_WIN_TO_LINUX)
   {
      slash_orig = '\\';
      slash_dest = '/';
   }
   else
   {
      slash_orig = '/';
      slash_dest = '\\';
   }

   unsigned int len = (unsigned int)str.length();
   char* newpath = new char[len+1];
   strcpy(newpath, str.c_str());
      
   for (unsigned int i=0; i<len; i++)
   {
      if (newpath[i]==slash_orig)
         newpath[i] = slash_dest;
   }
      
   newpath[len] = '\0';
   return newpath;
}


void CPathTranslator::Destroy()
{
   m_PathMap.clear();
   m_Initialized = false;
   // fixing #1235
   m_TranslationMode = TRANSLATOR_WIN_TO_LINUX;
}


// return true if the pt was initialized (so a linktab file was found etc etc
bool CPathTranslator::IsInitialized()
{
   return m_Initialized;
}


// return the translation mode (win->linux by default, since we write slashed pathes from windows
unsigned int CPathTranslator::GetTranslationMode()
{
   return m_TranslationMode;
}


/////////////////////////////////
// Simple class for managing the missing shaders error messages
/////////////////////////////////

// Return whether in_shader is in the set already
//
// @param in_shader          the shader name
// @return true if found, else false
bool CMissingShaderMap::Find(const CString in_shader)
{
   return m_shaders.find(in_shader) != m_shaders.end();
}


// Add in_shader to the set
//
// @param in_shader          the shader name
// @return true if in_shader was successfully entered, else false if the entry was in set already
bool CMissingShaderMap::Add(const CString in_shader)
{
   if (Find(in_shader))
      return false; // already there
   m_shaders.insert(in_shader);
   return true; // new entry
}


// Clear the set
void CMissingShaderMap::Clear()
{
   m_shaders.clear();
}

