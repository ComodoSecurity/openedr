//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//

#include "stdafx.h"

#ifdef WIN32
#include "TSProcessInfoHelper.h"

TSProcessInfoHelper::TSProcessInfoHelper(void)
{
}

TSProcessInfoHelper::~TSProcessInfoHelper(void)
{
}

bool TSProcessInfoHelper::init()
{
	m_hWTSAPI = LoadLibraryA("Wtsapi32.dll");

	if (m_hWTSAPI)
	{
		m_pWTSQuerySessionInformationA = (pWTSQuerySessionInformationA)GetProcAddress(m_hWTSAPI, "WTSQuerySessionInformationA");
		if (!m_pWTSQuerySessionInformationA)
		{
			FreeLibrary(m_hWTSAPI);
			m_hWTSAPI = NULL;
		}

		m_pWTSQuerySessionInformationW = (pWTSQuerySessionInformationW)GetProcAddress(m_hWTSAPI, "WTSQuerySessionInformationW");
		if (!m_pWTSQuerySessionInformationW)
		{
			FreeLibrary(m_hWTSAPI);
			m_hWTSAPI = NULL;
		}

		m_pWTSFreeMemory = (pWTSFreeMemory)GetProcAddress(m_hWTSAPI, "WTSFreeMemory");
		if (!m_pWTSFreeMemory)
		{
			FreeLibrary(m_hWTSAPI);
			m_hWTSAPI = NULL;
		}
	}
	return m_hWTSAPI != NULL;
}

void TSProcessInfoHelper::free()
{
	if (m_hWTSAPI)
	{
		FreeLibrary(m_hWTSAPI);
		m_hWTSAPI = NULL;
	}
}


BOOL TSProcessInfoHelper::isTerminalServicesEnabled()
{
	return m_hWTSAPI != NULL;
}

BOOL TSProcessInfoHelper::getProcessOwnerW(int processId, wchar_t * buf, int len)
{
	if (!isTerminalServicesEnabled())
		return FALSE;

	DWORD sessionId;

	if (!ProcessIdToSessionId(processId, &sessionId))
		return FALSE;

	wchar_t * pBuffer = NULL;
	std::wstring user;
	DWORD tempLen;
	
	if (m_pWTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSDomainName, &pBuffer, &tempLen))
	{
		user = pBuffer;
		user += L"\\";
		m_pWTSFreeMemory(pBuffer);
	}

	if (m_pWTSQuerySessionInformationW(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSUserName, &pBuffer, &tempLen))
	{
		user += pBuffer;
		m_pWTSFreeMemory(pBuffer);
	}

	if ((int)user.length() >= len)
		return FALSE;

	wcscpy(buf, user.c_str());

	return TRUE;
}

BOOL TSProcessInfoHelper::getProcessOwnerA(int processId, char * buf, int len)
{
	if (!isTerminalServicesEnabled())
		return FALSE;

	DWORD sessionId;

	if (!ProcessIdToSessionId(processId, &sessionId))
		return FALSE;

	char * pBuffer = NULL;
	std::string user;
	DWORD tempLen;
	
	if (m_pWTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSDomainName, &pBuffer, &tempLen))
	{
		user = pBuffer;
		user += "\\";
		m_pWTSFreeMemory(pBuffer);
	}

	if (m_pWTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, sessionId, WTSUserName, &pBuffer, &tempLen))
	{
		user += pBuffer;
		m_pWTSFreeMemory(pBuffer);
	}

	if ((int)user.length() >= len)
		return FALSE;

	strcpy(buf, user.c_str());

	return TRUE;
}


#endif
