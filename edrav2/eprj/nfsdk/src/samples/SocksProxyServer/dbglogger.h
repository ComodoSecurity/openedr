//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_DBGLOGGER_H)
#define _DBGLOGGER_H

#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>
#include <string>
#include <time.h>
#include <queue>
#include <process.h>
#include "sync.h"

#pragma warning (disable:4996)

using std::string;

#if !defined(_DEBUG) && !defined(_RELEASE_LOG)
#define DbgPrint
#else
#define DbgPrint DBGLogger::instance().writeInfo
#endif

class DBGLogger
{
public:

	virtual ~DBGLogger() 
	{
	}
	
	void free()
	{
		{
			AutoLock lock(m_cs);
			if (!m_enabled)
				return;
			m_enabled = false;
		}
		if (m_hThread)
		{
			SetEvent(m_threadStop);

            WaitForSingleObject(m_hThread, INFINITE);

            CloseHandle(m_hThread);
            m_hThread = NULL;
		}
	}

	static DBGLogger dbgLog;

	static DBGLogger& instance()
	{
		return dbgLog;
	}

	void init(const std::string & sFileName)
	{
		AutoLock cs(m_cs);

		m_sFileName = sFileName;
		m_enabled = true;

		ResetEvent(m_threadStop);
		m_hThread = (HANDLE)_beginthreadex(0, 0,
                     _workerThread,
                     (LPVOID)this,
                     0,
                     NULL);
		if (m_hThread == (HANDLE)(-1L))
		{
			m_hThread = NULL;
		}
	}

	void writeInfo(const char*  sMessage, ...)
	{
		int nSize = BUFSIZ;
		string sBuffer;
		va_list args;

		va_start(args, sMessage);
		{
			do
			{
				sBuffer.resize(nSize);
				memset(&sBuffer[0], 0, sBuffer.length());

				int nResult = _vsnprintf(const_cast<char*>(sBuffer.data()),
					sBuffer.length(), sMessage, args);

				if (nResult < 0)
				{
					nSize *= 2;
				}
				else
				{
					sBuffer.resize(nResult);

					sBuffer = getTime() + string(": ") + sBuffer + string("\r\n");

					break;
				}
			}
			while (nSize);
		}
		va_end(args);

		{
			AutoLock cs(m_cs);
			if (!m_enabled)
			{
				return;
			}
			m_logQueue.push(sBuffer);
		}

		SetEvent(m_threadWake);
	}


	string generateFileName(string sName) const
	{
		size_t nSize = MAX_PATH;
		string sPath;

		do
		{
			sPath.resize(nSize);

			nSize = GetModuleFileName(NULL, 
				const_cast<char*>(sPath.data()),
				(DWORD)(sPath.length() - 1));
		
			if (nSize >= sPath.length() - 2)
			{
				nSize *= 2;
			}
			else
			{
				sPath.resize(nSize);

				size_t nIndex = sPath.rfind("\\");
				if (nIndex != string::npos && nIndex > 0)
				{
					sPath.resize(nIndex);
				}
				else
				{
					sPath = "";
				}

				break;
			}
		} 
		while (nSize);

		SYSTEMTIME systemTime;

		GetSystemTime(&systemTime);

		char sValue[BUFSIZ];

		string sFileName = sName;
		
		if (systemTime.wMonth < 10)
		{
			sFileName += "0";
		}

		_ltoa(systemTime.wMonth, sValue, 10);
		sFileName += sValue;

		if (systemTime.wDay < 10)
		{
			sFileName += "0";
		}
		_ltoa(systemTime.wDay, sValue, 10);
		sFileName += sValue;

		_ltoa(systemTime.wYear, sValue, 10);
		sFileName += sValue;

		sFileName += "-";
		
		for (size_t i = 0;;i++)
		{
			WIN32_FIND_DATA findFileData;
			HANDLE hFind;
			char sValue[BUFSIZ];
			string sTempName = sFileName;

			_ltoa((long)i, sValue, 10);

			sTempName += sValue;
			sTempName += ".lst";

			sTempName = sPath + string("\\") + sTempName;

			hFind = FindFirstFile(sTempName.c_str(), &findFileData);
			if (hFind == INVALID_HANDLE_VALUE) 
			{
				sFileName = sTempName;

				break;
			} 
			else
			{
				FindClose(hFind);
			}
		}

		return sFileName;
	}

	string getTime() const
	{
		string sTime;
		time_t tNow = time(NULL);
		struct tm* ptm = localtime(&tNow);

		if (ptm != NULL)
		{
			sTime = asctime(ptm);

			size_t nIndex = sTime.find_first_of("\r\n");
			if (nIndex != string::npos)
			{
				sTime.resize(nIndex);
			}
		}

		return sTime;
	}

private:
	bool m_enabled;
	FILE * m_pFile;
	string m_sFileName;
	typedef std::queue<std::string> tLogQueue;
	tLogQueue m_logQueue;
	HANDLE m_hThread;
	AutoEventHandle m_threadStop;
	AutoEventHandle m_threadWake;
	AutoCriticalSection m_cs;

	DBGLogger() : m_enabled(false), m_pFile(NULL), m_hThread(NULL)
	{
	}

	static unsigned WINAPI _workerThread(void* pData)
	{
		((DBGLogger*)pData)->workerThread();
		return 0;
	}
	
	void workerThread()
	{
		HANDLE events[] = { m_threadStop, m_threadWake };
		
		FILE* pFile = NULL;

		pFile = open();
		if (!pFile)
			return;

        bool fExitLoop = false;

		for (;;)
		{
            if (fExitLoop) 
			{ 
				break; 
			}

			DWORD res = WaitForMultipleObjects(2, events, FALSE, 5000);
            if (res == WAIT_OBJECT_0) 
			{ 
				fExitLoop = true; 
			}

			for (;;)
			{
				std::string sBuffer;
				
				{
					AutoLock lock(m_cs);

					if (m_logQueue.empty())
						break;

					sBuffer = m_logQueue.front();
					m_logQueue.pop();
				}

				if (pFile)
				{
					fwrite(sBuffer.c_str(), sizeof(char), sBuffer.length(), pFile);
					fflush(pFile);
				}
			}
		}

		{
			AutoLock lock(m_cs);

			while( !m_logQueue.empty() )
			{
				m_logQueue.pop();
			}
		}

		close();
	}


	FILE* open() 
	{
		if (m_pFile)
			return m_pFile;

		m_pFile = fopen(m_sFileName.c_str(), "ab");
		if (m_pFile == NULL)
		{
			m_pFile = fopen(m_sFileName.c_str(), "wb");
		}

		return m_pFile;
	}

	void close()
	{
		if (m_pFile != NULL)
		{
			fclose(m_pFile);
			m_pFile = NULL;
		}
	}
};


#endif // !defined(_DBGLOGGER_H)
