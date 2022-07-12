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
#include "Proxy.h"

using namespace ProtocolFilters;
#ifndef _C_API
using namespace nfapi;
#endif

#if defined(WIN32) && (defined(_DEBUG) || defined(_RELEASE_LOG))
#define dbgPrintConnInfo _dbgPrintConnInfo 

#include <psapi.h>

#pragma comment(lib, "psapi.lib")

typedef BOOL (WINAPI *tQueryFullProcessImageNameA)(
			HANDLE hProcess,
			DWORD dwFlags,
			LPSTR lpExeName,
			PDWORD lpdwSize
);

tQueryFullProcessImageNameA _QueryFullProcessImageNameA =
			(tQueryFullProcessImageNameA)GetProcAddress(
							GetModuleHandle(_T("kernel32")), "QueryFullProcessImageNameA");


BOOL
getProcessName(DWORD processId, char * buf, DWORD len)
{
	BOOL res = FALSE;
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
	if (hProcess)
	{
		if (_QueryFullProcessImageNameA)
		{
			res = _QueryFullProcessImageNameA(hProcess, 0, buf, &len);
		} else
		{
			res = GetModuleFileNameExA(hProcess, NULL, buf, len);
		}

		CloseHandle(hProcess);
	}

	return res;
}


/**
* Print the connection information
**/
void _dbgPrintConnInfo(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	char localAddr[MAX_PATH] = "";
	char remoteAddr[MAX_PATH] = "";
	DWORD dwLen;
	sockaddr * pAddr;
	char processName[MAX_PATH] = "";
	
	pAddr = (sockaddr*)pConnInfo->localAddress;
	dwLen = sizeof(localAddr);

	WSAAddressToStringA((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				localAddr, 
				&dwLen);

	pAddr = (sockaddr*)pConnInfo->remoteAddress;
	dwLen = sizeof(remoteAddr);

	WSAAddressToStringA((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				remoteAddr, 
				&dwLen);
	
	if (!getProcessName(pConnInfo->processId, processName, sizeof(processName)/sizeof(processName[0])))
	{
		processName[0] = '\0';
	}

	DbgPrint("tcpConnected id=%llu flag=%d pid=%d direction=%s local=%s remote=%s\n\tProcess: %s\n",
		id,
		pConnInfo->filteringFlag,
		pConnInfo->processId, 
		(pConnInfo->direction == NF_D_IN)? "in" : ((pConnInfo->direction == NF_D_OUT)? "out" : "none"),
		localAddr, 
		remoteAddr,
		processName);
}
#else
#define dbgPrintConnInfo 
#endif


Proxy::Proxy(void)
{
	m_pParsersEventHandler = NULL;
}

Proxy::~Proxy(void)
{
}

void Proxy::free()
{
	deleteAllSessions();
}

void Proxy::threadStart()
{
	if (!m_pParsersEventHandler)
		return;

	m_pParsersEventHandler->threadStart();
}

void Proxy::threadEnd()
{
//	deleteAllSessions();
	
	if (!m_pParsersEventHandler)
		return;

	m_pParsersEventHandler->threadEnd();
}

void Proxy::tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	if (!m_pParsersEventHandler)
		return;

	DbgPrint("Proxy::tcpConnectRequest() id=%llu", id);

	m_pParsersEventHandler->tcpConnectRequest(id, pConnInfo);
}

void Proxy::tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	DbgPrint("Proxy::tcpConnected() id=%llu", id);

	if (!m_pParsersEventHandler)
		return;

#ifdef _DEMO
	static unsigned int counter = -1;
	if (counter == 0)
		return;
	counter--;
#endif

	dbgPrintConnInfo(id, pConnInfo);

	if (pConnInfo->filteringFlag & NF_FILTER)
	{
		createSession(id, IPPROTO_TCP, (GEN_SESSION_INFO*)pConnInfo);
	}

	m_pParsersEventHandler->tcpConnected(id, pConnInfo);
}

void Proxy::tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
{
	DbgPrint("Proxy::tcpClosed() id=%llu", id);

	if (!m_pParsersEventHandler)
		return;

	deleteSession(id);

	m_pParsersEventHandler->tcpClosed(id, pConnInfo);
}

void Proxy::tcpReceive(ENDPOINT_ID id, const char * buf, int len)
{
	DbgPrint("Proxy::tcpReceive() id=%llu len=%d", id, len);

#if defined(_DEBUG) || defined(_RELEASE_LOG)
	if (len > 0)
	{
		std::string s(buf, len);
		DbgPrint("Contents:\n%s", s.c_str());
	}
#endif

	ProxySession * pSession = findSession(id);
	if (pSession)
	{
		pSession->tcp_packet(NULL, DD_INPUT, PD_RECEIVE, buf, len);
		pSession->release();
	} else
	{
		if (!m_pParsersEventHandler)
			return;
		m_pParsersEventHandler->tcpPostReceive(id, buf, len);
	}
}

void Proxy::tcpSend(ENDPOINT_ID id, const char * buf, int len)
{
	DbgPrint("Proxy::tcpSend() id=%llu len=%d", id, len);

#if defined(_DEBUG) || defined(_RELEASE_LOG)
	if (len > 0)
	{
		std::string s(buf, len);
		DbgPrint("Contents:\n%s", s.c_str());
	}
#endif

	ProxySession * pSession = findSession(id);
	if (pSession)
	{
		pSession->tcp_packet(NULL, DD_INPUT, PD_SEND, buf, len);
		pSession->release();
	} else
	{
		if (!m_pParsersEventHandler)
			return;
		m_pParsersEventHandler->tcpPostSend(id, buf, len);
	}
}

void Proxy::tcpCanReceive(ENDPOINT_ID id)
{
	ProxySession * pSession = findSession(id);
	if (pSession)
	{
		DbgPrint("Proxy::tcpCanReceive() id=%llu", id);
		pSession->tcp_canPost(PD_RECEIVE);

		pSession->release();
	}
}

void Proxy::tcpCanSend(ENDPOINT_ID id)
{
	ProxySession * pSession = findSession(id);
	if (pSession)
	{
		DbgPrint("Proxy::tcpCanSend() id=%llu", id);
		pSession->tcp_canPost(PD_SEND);

		pSession->release();
	}
}

void Proxy::udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
	if (!m_pParsersEventHandler)
		return;
	m_pParsersEventHandler->udpCreated(id, pConnInfo);
}

void Proxy::udpConnectRequest(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_UDP_CONN_REQUEST pConnReq)
{
	if (!m_pParsersEventHandler)
		return;
	m_pParsersEventHandler->udpConnectRequest(id, pConnReq);
}

void Proxy::udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
{
	if (!m_pParsersEventHandler)
		return;
	m_pParsersEventHandler->udpClosed(id, pConnInfo);
}

void Proxy::udpReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, NFAPI_NS PNF_UDP_OPTIONS options)
{
	if (!m_pParsersEventHandler)
		return;
	m_pParsersEventHandler->udpPostReceive(id, remoteAddress, buf, len, options);
}

void Proxy::udpSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, NFAPI_NS PNF_UDP_OPTIONS options)
{
	if (!m_pParsersEventHandler)
		return;
	m_pParsersEventHandler->udpPostSend(id, remoteAddress, buf, len, options);
}

void Proxy::udpCanReceive(ENDPOINT_ID id)
{
}

void Proxy::udpCanSend(ENDPOINT_ID id)
{
}

void Proxy::setParsersEventHandler(PFEvents * pHandler)
{
	AutoLock lock(m_cs);
	m_pParsersEventHandler = pHandler;
}

PFEvents * Proxy::getParsersEventHandler()
{
	AutoLock lock(m_cs);
	return m_pParsersEventHandler;
}

ProxySession * Proxy::createSession(NFAPI_NS ENDPOINT_ID id, int protocol, GEN_SESSION_INFO * pSessionInfo)
{
	AutoLock lock(m_cs);

	ProxySession * pSession = new ProxySession(*this);
	int infoLen = (protocol == IPPROTO_TCP)? sizeof(NF_TCP_CONN_INFO) : sizeof(NF_UDP_CONN_INFO);

	if (!pSession)
		return NULL;

	if (!pSession->init(id, protocol, pSessionInfo, infoLen))
	{
		delete pSession;
		return NULL;
	}

	m_sessions[id] = pSession;

	return pSession;
}

bool Proxy::deleteSession(NFAPI_NS ENDPOINT_ID id)
{
	ProxySession * p = NULL;

	{
		AutoLock lock(m_cs);

		tSessions::iterator it = m_sessions.find(id);
		if (it == m_sessions.end())
		{
			return false;
		} else
		{
			p = it->second;
			m_sessions.erase(it);
		}
	}

	if (p)
	{
		p->release();
	}
	return true;
}

bool Proxy::deleteAllSessions()
{
	typedef std::vector<ProxySession*> tSessionList;
	tSessionList sessionList;

	{
		AutoLock lock(m_cs);

		tSessions::iterator it;
		for (it = m_sessions.begin(); it != m_sessions.end(); it++)
		{
			sessionList.push_back(it->second);
		}

		m_sessions.clear();
	}

	tSessionList::iterator it;
	for (it = sessionList.begin(); it != sessionList.end(); it++)
	{
		(*it)->release();
	}

	return true;
}



ProxySession * Proxy::findSession(NFAPI_NS ENDPOINT_ID id)
{
	AutoLock lock(m_cs);
	tSessions::iterator it = m_sessions.find(id);
	ProxySession * p = (it == m_sessions.end())? NULL : it->second;
	if (p)
	{
		p->addRef();
	}
	return p;
}

bool Proxy::postObject(NFAPI_NS ENDPOINT_ID id, PFObject * pObject)
{
	ProxySession * pSession = findSession(id);
	if (!pSession)
		return false;

	bool res = pSession->postObject(*(PFObjectImpl*)pObject);

	pSession->release();

	return res;
}

bool Proxy::tcpDisconnect(NFAPI_NS ENDPOINT_ID id, PF_ObjectType dt)
{
	ProxySession * pSession = findSession(id);
	if (!pSession)
		return false;

	pSession->tcpDisconnect(dt);

	pSession->release();

	return true;
}

bool Proxy::addFilter(NFAPI_NS ENDPOINT_ID id,
				PF_FilterType type, 
				tPF_FilterFlags flags,
				PF_OpTarget target, 
				PF_FilterType typeBase)
{
	ProxySession * pSession = findSession(id);
	if (!pSession)
	{
		DbgPrint("Proxy::addFilter() id=%llu session not found", id);
		return false;
	}

	bool res = pSession->addFilter(type, flags, target, typeBase);

	pSession->release();

	return res;
}
	
bool Proxy::deleteFilter(NFAPI_NS ENDPOINT_ID id, PF_FilterType type)
{
	ProxySession * pSession = findSession(id);
	if (!pSession)
	{
		DbgPrint("Proxy::deleteFilter() id=%llu session not found", id);
		return false;
	}

	bool res = pSession->deleteFilter(type);

	pSession->release();

	return res;
}

int Proxy::getFilterCount(NFAPI_NS ENDPOINT_ID id)
{
	ProxySession * pSession = findSession(id);
	if (!pSession)
	{
		DbgPrint("Proxy::getFilterCount() id=%llu session not found", id);
		return false;
	}

	int res = pSession->getFilterCount();

	pSession->release();

	return res;
}

bool Proxy::isFilterActive(NFAPI_NS ENDPOINT_ID id, PF_FilterType type)
{
	ProxySession * pSession = findSession(id);
	if (!pSession)
	{
		DbgPrint("Proxy::isFilterActive() id=%llu session not found", id);
		return false;
	}

	bool res = pSession->isFilterActive(type);

	pSession->release();

	return res;
}


ProxySession * Proxy::findAssociatedSession(const std::string & sAddr, bool isRemote)
{
	AutoLock lock(m_cs);

	std::string sPort, sPort1;
	size_t pos, pos1;

	DbgPrint("Proxy::findAssociatedSession() sAddr=%s, isRemote=%d", sAddr.c_str(), isRemote);

	if (!isRemote)
	{
		sPort = sAddr;
		if ((pos = sPort.rfind(":")) != std::string::npos)
		{
			sPort = sPort.substr(pos + 1);
		}
	}

	for (tSessions::iterator it = m_sessions.begin();
		it != m_sessions.end(); it++)
	{
		if (isRemote)
		{
			sPort1 = it->second->getAssociatedRemoteEndpointStr();
			if (sPort1.empty())
				continue;

			std::string sRemoteAddr;

			if (sPort1[0] == ':')
			{
				sRemoteAddr = it->second->getRemoteEndpointStr();
				if ((pos = sRemoteAddr.rfind(":")) != std::string::npos &&
					(pos1 = sPort1.rfind(":")) != std::string::npos)
				{
					sRemoteAddr.erase(pos+1);
					sRemoteAddr += sPort1.substr(pos1 + 1);
					DbgPrint("Proxy::findAssociatedSession() sRemoteAddr=%s", sRemoteAddr.c_str());
				} else
				{
					continue;
				}
			} else
			{
				sRemoteAddr = sPort1;
			}

			if (sRemoteAddr == sAddr)
			{
				it->second->addRef();
				return it->second;
			}
		} else
		{
			sPort1 = it->second->getAssociatedLocalEndpointStr();

			if ((pos = sPort1.rfind(":")) != std::string::npos)
			{
				sPort1 = sPort1.substr(pos + 1);
			}

			if (sPort == sPort1)
			{
				it->second->addRef();
				return it->second;
			}
		}
	}

	return NULL;
}

bool Proxy::canDisableFiltering(NFAPI_NS ENDPOINT_ID id)
{
	ProxySession * pSession = findSession(id);
	if (!pSession)
	{
		DbgPrint("Proxy::canDisableFiltering() id=%llu session not found", id);
		return false;
	}

	bool res = pSession->canDisableFiltering();

	pSession->release();

	return res;
}

void Proxy::setSessionState(NFAPI_NS ENDPOINT_ID id, bool suspend)
{
	ProxySession * pSession = findSession(id);
	if (!pSession)
	{
		DbgPrint("Proxy::setSessionState() id=%llu session not found", id);
		return;
	}

	pSession->setSessionState(suspend);

	pSession->release();
}
