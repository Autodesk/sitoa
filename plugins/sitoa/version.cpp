/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include <version.h>

#include <xsi_utils.h>

#define SITOA_MAJOR_VERSION_NUM    5
#define SITOA_MINOR_VERSION_NUM    0
#define SITOA_FIX_VERSION          L"0-alpha"


CString GetSItoAVersion(bool in_addPlatform)
{
   CString version(CValue(SITOA_MAJOR_VERSION_NUM).GetAsText() + L"." + 
                   CValue(SITOA_MINOR_VERSION_NUM).GetAsText() + L"." + 
                   CValue(SITOA_FIX_VERSION).GetAsText());

   if (in_addPlatform)
   {
      CString platform = ((CUtils::IsWindowsOS() ? L"win" : L"linux"));
      version+= L" " + platform;
   }

   return version;
}


unsigned int GetMajorVersion()
{
   return SITOA_MAJOR_VERSION_NUM;
}


unsigned int GetMinorVersion()
{
   return SITOA_MINOR_VERSION_NUM;
}


CString GetFixVersion()
{
   return CString(SITOA_FIX_VERSION);
}
