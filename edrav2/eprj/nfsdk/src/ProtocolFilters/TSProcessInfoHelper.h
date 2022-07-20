//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#pragma once

#include <list>
#include <string>

#ifndef _INC_WTSAPI

#define WTS_CURRENT_SERVER_HANDLE  ((HANDLE)NULL)

typedef enum _WTS_INFO_CLASS {
    WTSInitialProgram,
    WTSApplicationName,
    WTSWorkingDirectory,
    WTSOEMId,
    WTSSessionId,
    WTSUserName,
    WTSWinStationName,
    WTSDomainName,
    WTSConnectState,
    WTSClientBuildNumber,
    WTSClientName,
    WTSClientDirectory,
    WTSClientProductId,
    WTSClientHardwareId,
    WTSClientAddress,
    WTSClientDisplay,
    WTSClientProtocolType,
} WTS_INFO_CLASS;

typedef enum _WTS_CONNECTSTATE_CLASS {
    WTSActive,              // User logged on to WinStation
    WTSConnected,           // WinStation connected to client
    WTSConnectQuery,        // In the process of connecting to client
    WTSShadow,              // Shadowing another WinStation
    WTSDisconnected,        // WinStation logged on without client
    WTSIdle,                // Waiting for client to connect
    WTSListen,              // WinStation is listening for connection
    WTSReset,               // WinStation is being reset
    WTSDown,                // WinStation is down due to error
    WTSInit,                // WinStation in initialization
} WTS_CONNECTSTATE_CLASS;

typedef struct _WTS_SESSION_INFOA {
    DWORD SessionId;             // session id
    LPSTR pWinStationName;       // name of WinStation this session is connected to
    WTS_CONNECTSTATE_CLASS State; // connection state (see enum)
} WTS_SESSION_INFO, * PWTS_SESSION_INFO;

#endif

typedef VOID (WINAPI *pWTSFreeMemory)(IN PVOID pMemory);

typedef BOOL (WINAPI *pWTSQuerySessionInformationA)(
  HANDLE hServer,
  DWORD SessionId,
  WTS_INFO_CLASS WTSInfoClass,
  char** ppBuffer,
  DWORD* pBytesReturned
);

typedef BOOL (WINAPI *pWTSQuerySessionInformationW)(
  HANDLE hServer,
  DWORD SessionId,
  WTS_INFO_CLASS WTSInfoClass,
  wchar_t** ppBuffer,
  DWORD* pBytesReturned
);

typedef BOOL (WINAPI *pWTSEnumerateSessions)(
  HANDLE hServer,
  DWORD Reserved,
  DWORD Version,
  PWTS_SESSION_INFO* ppSessionInfo,
  DWORD* pCount
);

typedef std::list<std::string> tUsersList;

class TSProcessInfoHelper
{
public:
	TSProcessInfoHelper(void);
	~TSProcessInfoHelper(void);

	bool init();
	void free();

	/**
	 * Function name   : isTerminalServicesEnabled
	 * Description     : Returns true if Wtsapi32.dll was loaded successfully 
	 * @return	bool 
	 **/
	BOOL isTerminalServicesEnabled();

	BOOL getProcessOwnerA(int processId, char * buf, int len);
	BOOL getProcessOwnerW(int processId, wchar_t * buf, int len);

private:
	HMODULE m_hWTSAPI;
	pWTSQuerySessionInformationA m_pWTSQuerySessionInformationA;
	pWTSQuerySessionInformationW m_pWTSQuerySessionInformationW;
	pWTSFreeMemory m_pWTSFreeMemory;
};
