/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#pragma once

#include "ai_critsec.h"

#include <xsi_decl.h>
#include <xsi_string.h>

#include <vector>

using namespace std;
using namespace XSI;

class CMessage
{
private:

   CString        m_message;
   siSeverityType m_severity;

public:

   CMessage()
   {}
   ~CMessage()
   {}

   CMessage(const CMessage &in_arg)
   {
      m_message = in_arg.m_message;
      m_severity = in_arg.m_severity;
   }

   CMessage(CString &in_message, siSeverityType in_severity)
   {
      m_message  = in_message;
      m_severity = in_severity;
   }

   void Log();
};


class CMessageQueue
{
private:
   // LONG              m_mainThreadId;
   vector <CMessage> m_messages;
   AtMutex           m_cs;

public:
   CMessageQueue()
   {
   }

   ~CMessageQueue()
   {
   }

   // void PutMainThreadId(LONG in_id);
   // bool IsMainThreadId();
   void LogMsg(CString in_message, siSeverityType in_severity=siInfoMsg);
   void Log();
};


enum eSItoALogLevel
{
   eSItoALogLevel_Errors = 0,
   eSItoALogLevel_Warnings,
   eSItoALogLevel_Info,
   eSItoALogLevel_Debug  
};


class CRenderMessages
{
public: 
   static void SetLogLevel(unsigned int in_logLevel, bool in_console, bool in_file);
   static void LogCallback(int in_mask, int in_severity, const char* in_msg, int in_tab);

private:
   static AtMutex m_CriticalSection;
   // SItoA Log Level 
   static unsigned int m_LogLevel;
   static bool m_console, m_file;
};
