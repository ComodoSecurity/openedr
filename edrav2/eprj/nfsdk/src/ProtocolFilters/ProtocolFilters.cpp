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
#include "ProtocolFilters.h"
#include "Proxy.h"
#include "sync.h"
#include "PFHeader.h"
#include "UnzipStream.h"

#ifdef WIN32
#include "TSProcessInfoHelper.h"
static TSProcessInfoHelper g_PIHelper;
#endif

using namespace ProtocolFilters;

static Proxy g_proxy;

#if defined(_DEBUG) || defined(_RELEASE_LOG)
DBGLogger DBGLogger::dbgLog;
#endif

#ifdef _C_API
#undef PF_API
#define PF_API
#endif

Proxy & getProxy()
{
	return g_proxy;
}

#ifdef WIN32

PF_API BOOL ProtocolFilters::pf_init(PFEvents * pHandler, const wchar_t * dataFolder)
{
#ifdef WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return FALSE;
	}

	g_PIHelper.init();
#endif

	g_proxy.setParsersEventHandler(pHandler);
	
	if (!Settings::get().init(dataFolder))
		return FALSE;

#if defined(_DEBUG) || defined(_RELEASE_LOG)
	pf_startLog("ProtocolFiltersLog.txt");
	DbgPrint("pf_init %s", encodeUTF8(dataFolder).c_str());
#endif

	return TRUE;
}

#else

PF_API BOOL ProtocolFilters::pf_init(PFEvents * pHandler, const char * dataFolder)
{
	g_proxy.setParsersEventHandler(pHandler);
	
	if (!Settings::get().init(dataFolder))
		return FALSE;

#if defined(_DEBUG) || defined(_RELEASE_LOG)
	pf_startLog("ProtocolFiltersLog.txt");
	DbgPrint("pf_init %s", dataFolder);
#endif

	return TRUE;
}

#endif

PF_API void ProtocolFilters::pf_free()
{
	g_proxy.setParsersEventHandler(NULL);
	Settings::get().free();
	g_proxy.free();

	pf_stopLog();

#ifdef WIN32
	g_PIHelper.free();
	WSACleanup();
#endif
}

PF_API NFAPI_NS NF_EventHandler * ProtocolFilters::pf_getNFEventHandler()
{
	return &g_proxy;
}

PF_API BOOL ProtocolFilters::pf_postObject(NFAPI_NS ENDPOINT_ID id, PFObject * pObject)
{
	return g_proxy.postObject(id, pObject);
}

PF_API BOOL ProtocolFilters::pf_addFilter(NFAPI_NS ENDPOINT_ID id,
				PF_FilterType type, 
				tPF_FilterFlags flags,
				PF_OpTarget target, 
				PF_FilterType typeBase)
{
	return g_proxy.addFilter(id, type, flags, target, typeBase);
}
	
PF_API BOOL ProtocolFilters::pf_deleteFilter(NFAPI_NS ENDPOINT_ID id, PF_FilterType type)
{
	return g_proxy.deleteFilter(id, type);
}

PF_API int ProtocolFilters::pf_getFilterCount(NFAPI_NS ENDPOINT_ID id)
{
	return g_proxy.getFilterCount(id);
}

PF_API BOOL ProtocolFilters::pf_isFilterActive(NFAPI_NS ENDPOINT_ID id, PF_FilterType type)
{
	return g_proxy.isFilterActive(id, type)? TRUE : FALSE;
}

PF_API BOOL ProtocolFilters::pf_canDisableFiltering(NFAPI_NS ENDPOINT_ID id)
{
	return g_proxy.canDisableFiltering(id)? TRUE : FALSE;
}

PF_API void ProtocolFilters::pf_setRootSSLCertSubject(const char * rootSubject)
{
#ifndef PF_NO_SSL_FILTER
	Settings::get().getSSLDataProvider()->setRootName(rootSubject);
#endif
}

PF_API void ProtocolFilters::pf_setRootSSLCertSubjectEx(const char * rootSubject, const char * x509, int x509Len, const char * pkey, int pkeyLen)
{
#ifndef PF_NO_SSL_FILTER
	Settings::get().getSSLDataProvider()->setRootNameEx(rootSubject, x509, x509Len, pkey, pkeyLen);
#endif
}

PF_API void ProtocolFilters::pf_setRootSSLCertImportFlags(unsigned long flags)
{
#ifndef PF_NO_SSL_FILTER
	Settings::get().getSSLDataProvider()->setRootSSLCertImportFlags(flags);
#endif
}

PF_API BOOL ProtocolFilters::pf_loadCAStore(const char * rootCAFileName)
{
#ifndef PF_NO_SSL_FILTER
	return Settings::get().getSSLDataProvider()->loadCAStore(rootCAFileName);
#else
	return TRUE;
#endif
}

#ifdef WIN32
PF_API BOOL ProtocolFilters::pf_getRootSSLCertFileName(wchar_t * fileName, int fileNameLen)
{
#ifndef PF_NO_SSL_FILTER
	tPathString certFileName = Settings::get().getSSLDataProvider()->getRootSSLCertFileName();
	if (fileNameLen < (int)certFileName.length()+1)
	{
		return FALSE;
	}

	wcscpy(fileName, certFileName.c_str());
#endif

	return TRUE;
}
#else
PF_API BOOL ProtocolFilters::pf_getRootSSLCertFileName(char * fileName, int fileNameLen)
{
#ifndef PF_NO_SSL_FILTER
	tPathString certFileName = Settings::get().getSSLDataProvider()->getRootSSLCertFileName();
	if (fileNameLen < certFileName.length()+1)
	{
		return FALSE;
	}

	strcpy(fileName, certFileName.c_str());
#endif

	return TRUE;
}
#endif

PF_API BOOL ProtocolFilters::pf_unzipStream(PFStream * pStream)
{
	IOB buf, outBuf;
	tStreamSize len;
	UnzipStream uzStream;
	MFStream tempStream;

	if (!tempStream.open())
		return FALSE;

	if (!buf.append(NULL, 16384, false))
		return FALSE;

	pStream->seek(0, FILE_BEGIN);

	while ((len = pStream->read(buf.buffer(), buf.size())))
	{
		outBuf.reset();

		if (uzStream.uncompress(buf.buffer(), len, outBuf))
		{
			if (!tempStream.write(outBuf.buffer(), outBuf.size()))
				return FALSE;
		} else
		{
			return FALSE;
		}
	}

	tempStream.copyTo(pStream);

	return TRUE;
}

PF_API void ProtocolFilters::pf_waitForImportCompletion()
{
#ifndef PF_NO_SSL_FILTER
	Settings::get().getSSLDataProvider()->waitForImportCompletion();
#endif
}

PF_API BOOL ProtocolFilters::pf_startLog(const char * logFileName)
{
#if defined(_DEBUG) || defined(_RELEASE_LOG)
	if (DBGLogger::instance().isEnabled())
		return TRUE;

	DBGLogger::instance().init(logFileName);
#endif
	return TRUE;
}

PF_API void ProtocolFilters::pf_stopLog()
{
#if defined(_DEBUG) || defined(_RELEASE_LOG)
	if (DBGLogger::instance().isEnabled())
	{
		DBGLogger::instance().free();
	}
#endif
}

PF_API void ProtocolFilters::pf_setExceptionsTimeout(eEXCEPTION_CLASS ec, unsigned __int64 timeout)
{
#ifndef PF_NO_SSL_FILTER
	Settings::get().getSSLDataProvider()->setExceptionsTimeout(ec, timeout);
#endif
}

PF_API void ProtocolFilters::pf_deleteExceptions(eEXCEPTION_CLASS ec)
{
#ifndef PF_NO_SSL_FILTER
	Settings::get().getSSLDataProvider()->deleteExceptions(ec);
#endif
}


#ifdef WIN32
PF_API BOOL ProtocolFilters::pf_getProcessOwnerA(unsigned long processId, char * buf, int len)
{
	if (g_PIHelper.isTerminalServicesEnabled() && g_PIHelper.getProcessOwnerA(processId, buf, len))
	{
		return true;
	} else
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
			
		if (!hProcess)
		{
			return FALSE;
		}
		
		HANDLE hToken;
		char userName[250] = "";
		DWORD ccName = sizeof(userName) / sizeof(userName[0]); 
		char domainName[250] = "";
		DWORD ccDName = sizeof(domainName) / sizeof(domainName[0]);
		std::string user;
				
		if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken))
		{
			TOKEN_USER tu[8];
			DWORD len;

			if (GetTokenInformation(hToken, TokenUser, &tu[0], sizeof(tu), &len))
			{
				SID_NAME_USE snu;

				if (LookupAccountSidA(NULL, tu[0].User.Sid, userName, &ccName, 
					domainName, &ccDName, &snu))
				{
					user = domainName;
					user += "\\";
					user += userName;
				}
			}
			CloseHandle(hToken);
		}

		CloseHandle(hProcess);

		if (user.length() == 0 ||
			(int)user.length() >= len)
		{
			return FALSE;
		}

		strcpy(buf, user.c_str());
	}

	return TRUE;
}

PF_API BOOL ProtocolFilters::pf_getProcessOwnerW(unsigned long processId, wchar_t * buf, int len)
{
	if (g_PIHelper.isTerminalServicesEnabled() && g_PIHelper.getProcessOwnerW(processId, buf, len))
	{
		return true;
	} else
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
			
		if (!hProcess)
		{
			return FALSE;
		}
		
		HANDLE hToken;
		wchar_t userName[250] = L"";
		DWORD ccName = sizeof(userName) / sizeof(userName[0]); 
		wchar_t domainName[250] = L"";
		DWORD ccDName = sizeof(domainName) / sizeof(domainName[0]);
		std::wstring user;
				
		if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken))
		{
			TOKEN_USER tu[8];
			DWORD len;

			if (GetTokenInformation(hToken, TokenUser, &tu[0], sizeof(tu), &len))
			{
				SID_NAME_USE snu;

				if (LookupAccountSidW(NULL, tu[0].User.Sid, userName, &ccName, 
					domainName, &ccDName, &snu))
				{
					user = domainName;
					user += L"\\";
					user += userName;
				}
			}
			CloseHandle(hToken);
		} else
		{
		}

		CloseHandle(hProcess);

		if (user.length() == 0 ||
			(int)user.length() >= len)
		{
			return FALSE;
		}

		wcscpy(buf, user.c_str());
	}
	return TRUE;
}
#endif
