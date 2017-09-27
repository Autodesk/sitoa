/************************************************************************************************************************************
Copyright 2017 Autodesk, Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. 
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, 
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
See the License for the specific language governing permissions and limitations under the License.
************************************************************************************************************************************/

#include "renderer/Renderer.h"
#include "renderer/RenderMessages.h"

using namespace XSI;

// Logs the message. This is the only Application().LogMessage allowed, apart for the one in XSIUnloadPlugin 
//
void CMessage::Log()
{
   Application().LogMessage(m_message, m_severity);
}

#if 0
void CMessageQueue::PutMainThreadId(LONG in_id)
{
   m_mainThreadId = in_id;
}


bool CMessageQueue::IsMainThreadId()
{
   LONG id = CThreadUtilities::GetThreadId();
   return m_mainThreadId == id;
}
#endif

// Add a message into the queue, or log it right away if in xsibatch mode
//
// @param in_message          the message
// @param in_severity         the severity
//
void CMessageQueue::LogMsg(CString in_message, siSeverityType in_severity)
{
   AiCritSecEnter(&m_cs);

   CMessage message(in_message, in_severity);
   // #1787: xsibatch does not trigger the timer event, although properly registered. 
   // So, let use the message queue only in interactive mode. Else, simply print out the message
   if (Application().IsInteractive())
      m_messages.push_back(message);
   else
      message.Log();

   AiCritSecLeave(&m_cs);
}


// Log to console the message queue, then delete the queue. Called by the timer event only
//
void CMessageQueue::Log()
{
   AiCritSecEnter(&m_cs);
   vector <CMessage>::iterator it;
   for (it = m_messages.begin(); it != m_messages.end(); it++)
      it->Log();
   m_messages.clear();
   AiCritSecLeave(&m_cs);
}


AtCritSec CRenderMessages::m_CriticalSection;
bool CRenderMessages::m_Initialized = false;
unsigned int CRenderMessages::m_LogLevel = eSItoALogLevel_Warnings;
bool CRenderMessages::m_console = false;
bool CRenderMessages::m_file = false;


void CRenderMessages::Initialize()
{
   if (!m_Initialized)
      AiCritSecInit(&m_CriticalSection);

   m_Initialized = true;
}


void CRenderMessages::Destroy()
{
   if (m_Initialized)
      AiCritSecClose(&m_CriticalSection);

   m_Initialized = false;
}


// Initialize the render messages flags
//
// @param in_logLevel        the log level
// @param in_console         true is logging to console is required
// @param in_file            true is logging into file is required
//
void CRenderMessages::SetLogLevel(unsigned int in_logLevel, bool in_console, bool in_file)
{
   m_LogLevel = in_logLevel;
   m_console = in_console;
   m_file = in_file;
}


// The callback used to log all the Arnold messages
//
void CRenderMessages::LogCallback(int in_mask, int in_severity, const char* in_msg, int in_tab)
{
   if (!m_Initialized)
      return;

   // Critical section, multiple render threads can call this method
   AiCritSecEnter(&m_CriticalSection);
   CString message(in_msg);
   CString file_message(in_msg);

   // Due to a bug in LogMessage, we need to replace % with %%
   // for the messages being printed in the script editor.
   CStringArray curMsgParts = message.Split(L"%");
   message.Clear();
   for (LONG i=0; i<curMsgParts.GetCount(); i++)
   {
      message+= curMsgParts[i];
      if (i < curMsgParts.GetCount() - 1)
         message += L"%%";
   }
   // The bug work around is over - remove when the bug is gone

   if (m_LogLevel > eSItoALogLevel_Warnings)
   {
      char aux[32];
      CString usedMemory;

      CString elapsedTime = CTimeUtilities().FormatTime(AiMsgUtilGetElapsedTime(), 0, true, false);
      sprintf(aux, "%4dMB", (int)(AiMsgUtilGetUsedMemory()/AiSqr(1024)));
      usedMemory.PutAsciiString(aux);

      message = elapsedTime + L" " + usedMemory + L"   | " + message;
      if (m_file) // do the same for the string going to file (without the double %) for #1925
         file_message = elapsedTime + L" " + usedMemory + L"   | " + file_message;
   }

   if (m_console) // log to console
   {
      siSeverityType severity(siInfoMsg);
      if (in_severity == AI_SEVERITY_WARNING)
         severity = siWarningMsg;
      else if (in_severity == AI_SEVERITY_ERROR)
         severity = siErrorMsg;

      GetMessageQueue()->LogMsg(L"[arnold] " + message, severity);
   }
   if (m_file && GetRenderInstance()->m_logFile.is_open()) // log into file
      GetRenderInstance()->m_logFile << file_message.GetAsciiString() << "\n";

   AiCritSecLeave(&m_CriticalSection);
}

