//
// 	NetFilterSDK 
// 	Copyright (C) Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#pragma once

#include <set>
#include <map>
#include <list>
#include <mswsock.h>
#include "sync.h"
#include "socksdefs.h"

class IOCPHandler
{
public:
	virtual void onComplete(SOCKET socket, DWORD dwTransferred, OVERLAPPED * pOverlapped, int error) = 0;
};

class IOCPService
{
public:
	IOCPService() : m_hIOCP(INVALID_HANDLE_VALUE), m_pHandler(NULL)
	{
	}
	~IOCPService()
	{
	}

	bool init(IOCPHandler * pHandler)
	{
		m_pHandler = pHandler;

		if (m_hIOCP != INVALID_HANDLE_VALUE)
			return false;

		m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);
		if (m_hIOCP == INVALID_HANDLE_VALUE)
			return false;

		ResetEvent(m_stopEvent);

		HANDLE hThread = (HANDLE)_beginthreadex(0, 0,
						 _workerThread,
						 (LPVOID)this,
						 0,
						 NULL);
	
		if (hThread != 0)
		{
			m_workerThread.Attach(hThread);
		}

		return true;
	}

	void free()
	{
		if (m_workerThread == INVALID_HANDLE_VALUE)
			return;

		SetEvent(m_stopEvent);
		WaitForSingleObject(m_workerThread, INFINITE);
		m_workerThread.Close();

		if (m_hIOCP != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hIOCP);
			m_hIOCP = INVALID_HANDLE_VALUE;
		}
	}

	bool registerSocket(SOCKET s)
	{
		if (!CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)s, 1))
			return false;

		return true;
	}

	bool postCompletion(SOCKET s, DWORD dwTransferred, LPOVERLAPPED pol)
	{
		return PostQueuedCompletionStatus(m_hIOCP, dwTransferred, (ULONG_PTR)s, pol)? true : false;
	}

protected:

	void workerThread()
	{
		DWORD dwTransferred;
		ULONG_PTR cKey;
		OVERLAPPED * pOverlapped;

		for (;;)
		{
			if (GetQueuedCompletionStatus(m_hIOCP, &dwTransferred, &cKey, &pOverlapped, 500))
			{
				m_pHandler->onComplete((SOCKET)cKey, dwTransferred, pOverlapped, 0);
			} else
			{
				DWORD err = GetLastError();
				if (err != WAIT_TIMEOUT)
				{
					m_pHandler->onComplete((SOCKET)cKey, dwTransferred, pOverlapped, err);
				}
			}

			if (WaitForSingleObject(m_stopEvent, 0) == WAIT_OBJECT_0)
				break;
		}

	}

	static unsigned int WINAPI _workerThread(void * pThis)
	{
		((IOCPService*)pThis)->workerThread();
		return 0;
	}

private:

	HANDLE m_hIOCP;
	AutoEventHandle m_stopEvent;
	AutoHandle		m_workerThread;
	IOCPHandler	*	m_pHandler;
};

