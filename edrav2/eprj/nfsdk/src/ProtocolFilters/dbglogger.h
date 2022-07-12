//
// 	NetFilterSDK 
// 	Copyright (C) Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_DBGLOGGER_H)
#define _DBGLOGGER_H

#include "stdafx.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <time.h>
#include <queue>
#include "sync.h"
#include "thread.h"
#include "utf8.h"

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
	
	static DBGLogger dbgLog;

	static DBGLogger& instance()
	{
		return dbgLog;
	}

	bool init(const std::string & sFileName)
	{
		ProtocolFilters::AutoLock cs(m_cs);

		m_sFileName = sFileName;

		if (!m_thread.create(_workerThread, (void*)this))
			return false;
	
		m_enabled = true;

		return true;
	}

	void free()
	{
		if (!m_enabled)
			return;

		m_enabled = false;

		m_threadEvent.fire(1);
		m_thread.wait();
	}

	bool isEnabled()
	{
		return m_enabled;
	}

	void writeInfo(const char*  sMessage, ...)
	{
		int nSize = BUFSIZ;
		string sBuffer;
		va_list args;

		if (!isEnabled())
			return;

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
			ProtocolFilters::AutoLock cs(m_cs);
			m_logQueue.push(sBuffer);
		}

		m_threadEvent.fire(0);
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

	ProtocolFilters::NFThread		m_thread;
	ProtocolFilters::NFSyncEvent	m_threadEvent;

	ProtocolFilters::AutoCriticalSection m_cs;

	DBGLogger() : m_enabled(false), m_pFile(NULL)
	{
	}

	static NFTHREAD_FUNC _workerThread(void* pData)
	{
		((DBGLogger*)pData)->workerThread();
		return NULL;
	}
	
	void workerThread()
	{
		FILE* pFile = NULL;
		int flags = 0;

		pFile = open();
		if (!pFile)
			return;

		for (;;)
		{
			m_threadEvent.wait(&flags);
            
			if (flags != 0)
				break;

			for (;;)
			{
				std::string sBuffer;
				
				{
					ProtocolFilters::AutoLock lock(m_cs);

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
			ProtocolFilters::AutoLock lock(m_cs);

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
