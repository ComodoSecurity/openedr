//
// 	NetFilterSDK 
// 	Copyright (C) 2014 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#pragma once

#include "sync.h"

namespace nfapi
{

typedef void (*tTimeoutProc)(ENDPOINT_ID id);

class Timers
{
public:
	Timers()
	{
		m_stopEvent.Attach(CreateEvent(NULL, TRUE, FALSE, NULL));
		m_hThread = INVALID_HANDLE_VALUE;
		m_timeoutProc = NULL;
	}
	
	~Timers()
	{
		free();
	}

	bool init(tTimeoutProc timeoutProc)
	{
		HANDLE hThread;
		unsigned threadId;

		if (!timeoutProc)
			return false;

		m_timeoutProc = timeoutProc;

		ResetEvent(m_stopEvent);

		hThread = (HANDLE)_beginthreadex(0, 0,
						 _threadProc,
						 (LPVOID)this,
						 0,
						 &threadId);
		if (hThread != 0 && hThread != INVALID_HANDLE_VALUE)
		{
			m_hThread = hThread;
			return true;
		}

		return false;
	}

	void free()
	{
		killAllTimers();

		SetEvent(m_stopEvent);

		if (m_hThread != INVALID_HANDLE_VALUE)
		{
			WaitForSingleObject(m_hThread, INFINITE);
			CloseHandle(m_hThread);
			m_hThread = INVALID_HANDLE_VALUE;
		}
	}

	void addTimer(ENDPOINT_ID id, DWORD timeout)
	{
		AutoLock lock(m_cs);
		m_timers[id] = timeout;
	}

	void killTimer(ENDPOINT_ID id)
	{
		AutoLock lock(m_cs);
		m_timers.erase(id);
	}

	void killAllTimers()
	{
		AutoLock lock(m_cs);
		m_timers.clear();
	}

protected:

	void threadProc()
	{
		for (;;)
		{
			DWORD res = WaitForSingleObject(m_stopEvent, 100);
			if (res == WAIT_OBJECT_0)
				break;

			AutoLock lock(m_cs);

			for (tTimers::iterator it = m_timers.begin(); it != m_timers.end(); )
			{
				if (it->second > 0)
				{
					it->second--;
					it++;
				}
				else
				{
					m_timeoutProc(it->first);
					it = m_timers.erase(it);
				}
			}
		}
	}

	static unsigned WINAPI _threadProc(void* pData)
	{
		(reinterpret_cast<Timers*>(pData))->threadProc();
		return 0;
	}

private:
	typedef std::map<ENDPOINT_ID, DWORD> tTimers;
	tTimers m_timers;

	HANDLE	m_hThread;
	nfapi::AutoHandle m_stopEvent;
	nfapi::AutoCriticalSection m_cs;
	tTimeoutProc m_timeoutProc;
};

}