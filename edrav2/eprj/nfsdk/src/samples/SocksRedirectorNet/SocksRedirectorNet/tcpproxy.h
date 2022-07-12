
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
#include <vector>
#include <mswsock.h>
#include <mstcpip.h>
#include "sync.h"
#include "linkedlist.h"
#include "socksdefs.h"
#include "iocp.h"
#include "threadpool.h"
#include "nfevents.h"
#include "DbgLogger.h"

namespace TcpProxy
{

#define TCP_PACKET_SIZE 8192
#define IN_SOCKET true
#define OUT_SOCKET false

struct TCP_PACKET
{
	TCP_PACKET()
	{
		buffer.len = 0;
		buffer.buf = NULL;
	}
	TCP_PACKET(const char * buf, int len)
	{
		if (len > 0)
		{
			buffer.buf = new char[len];
			buffer.len = len;

			if (buf)
			{
				memcpy(buffer.buf, buf, len);
			}
		} else
		{
			buffer.buf = NULL;
			buffer.len = 0;
		}	
	}
	
	WSABUF & operator ()()
	{
		return buffer;
	}

	void free()
	{
		if (buffer.buf)
		{
			delete[] buffer.buf;
		}
	}

	WSABUF	buffer;
};

typedef std::vector<TCP_PACKET> tPacketList;

enum OV_TYPE
{
	OVT_ACCEPT,
	OVT_CONNECT,
	OVT_CLOSE,
	OVT_SEND,
	OVT_RECEIVE
};

struct OV_DATA
{
	OV_DATA()
	{
		memset(&ol, 0, sizeof(ol));
	}
	~OV_DATA()
	{
		for (tPacketList::iterator it = packetList.begin(); it != packetList.end(); it++)
		{
			it->free();
		}
 	}

	OVERLAPPED	ol;
	LIST_ENTRY	entry;
	LIST_ENTRY	entryEventList;
	NFAPI_NS ENDPOINT_ID id;
	OV_TYPE		type;
	tPacketList packetList;

	SOCKET	socket;
	DWORD	dwTransferred;
	int		error;
};

enum PROXY_STATE
{
	PS_NONE,
	PS_AUTH,
	PS_AUTH_NEGOTIATION,
	PS_CONNECT,
	PS_CONNECTED,
	PS_ERROR,
	PS_CLOSED
};

enum PROXY_TYPE
{
	PROXY_NONE,
	PROXY_SOCKS5
};

struct SOCKET_DATA
{
	SOCKET_DATA()
	{
		socket = INVALID_SOCKET;
		disconnected = false;
		receiveInProgress = false;
		sendInProgress = false;
		disconnect = false;
		closed = false;
	}
	~SOCKET_DATA()
	{
		if (socket != INVALID_SOCKET)
		{
			closesocket(socket);
		}
		for (tPacketList::iterator it = packetList.begin(); it != packetList.end(); it++)
		{
			it->free();
		}
	}

	SOCKET	socket;
	bool	disconnected;
	bool	receiveInProgress;
	bool	sendInProgress;
	bool	disconnect;
	bool	closed;
	tPacketList packetList;
};

struct LOCAL_PROXY_CONN_INFO
{
	LOCAL_PROXY_CONN_INFO()
	{
	}
	LOCAL_PROXY_CONN_INFO(const LOCAL_PROXY_CONN_INFO & v)
	{
		*this = v;
	}
	LOCAL_PROXY_CONN_INFO & operator = (const LOCAL_PROXY_CONN_INFO & v)
	{
		connInfo = v.connInfo;
		proxyType = v.proxyType;
		memcpy(proxyAddress, v.proxyAddress, sizeof(proxyAddress));
		proxyAddressLen = v.proxyAddressLen;
		userName = v.userName;
		userPassword = v.userPassword;
		return *this;
	}

	NFAPI_NS NF_TCP_CONN_INFO connInfo;

	PROXY_TYPE		proxyType;
	char			proxyAddress[NF_MAX_ADDRESS_LENGTH];
	int				proxyAddressLen;

	std::string		userName;
	std::string		userPassword;
};

struct PROXY_DATA
{
	PROXY_DATA()
	{
		id = 0;
		proxyState = PS_NONE;
		suspended = false;
		offline = false;
		memset(&connInfo, 0, sizeof(connInfo));
		refCount = 1;
		proxyType = PROXY_NONE;
		proxyAddressLen = 0;
	}
	~PROXY_DATA()
	{
	}

	NFAPI_NS ENDPOINT_ID id;

	SOCKET_DATA		inSocket;
	SOCKET_DATA		outSocket;

	PROXY_STATE		proxyState;
	NFAPI_NS NF_TCP_CONN_INFO connInfo;

	PROXY_TYPE		proxyType;
	char			proxyAddress[NF_MAX_ADDRESS_LENGTH];
	int				proxyAddressLen;

	std::string		userName;
	std::string		userPassword;

	bool	suspended;
	bool	offline;
	
	int		refCount;
	AutoCriticalSection lock;

private:
	PROXY_DATA & operator = (const PROXY_DATA & v)
	{
		return *this;
	}

};

class TCPProxy : public IOCPHandler, public ThreadJobSource
{
public:
	TCPProxy()
	{
		m_listenSocket = INVALID_SOCKET;
		m_acceptSocket = INVALID_SOCKET;
		m_listenSocket_IPv6 = INVALID_SOCKET;
		m_acceptSocket_IPv6 = INVALID_SOCKET;
		m_port = 0;
		m_pPFEventHandler = NULL;
		m_ipv4Available = false;
		m_ipv6Available = false;
		m_proxyType = PROXY_NONE;
		m_proxyAddressLen = 0;
	}

	virtual ~TCPProxy()
	{
	}

	unsigned short getPort()
	{
		return m_port;
	}

	bool isIPFamilyAvailable(int ipFamily)
	{
		switch (ipFamily)
		{
		case AF_INET:
			return m_ipv4Available;
		case AF_INET6:
			return m_ipv6Available;
		}
		return false;
	}

	bool init(unsigned short port, bool bindToLocalhost = true, int threadCount = 0)
	{
		bool result = false;

		DbgPrint("TCPProxy::init port=%d", htons(port));

		m_timeout = 5 * 60 * 1000;

		InitializeListHead(&m_ovDataList);
		m_ovDataCounter = 0;

		InitializeListHead(&m_eventList);

		m_port = port;

		m_connId = 0;

		if (!initExtensions())
		{
			DbgPrint("TCPProxy::init initExtensions() failed");
			return false;
		}

		for (;;)
		{
			if (!m_service.init(this))
			{			
				DbgPrint("TCPProxy::init m_service.init() failed");
				break;
			}

			if (!m_pool.init(threadCount, this))
			{
				DbgPrint("TCPProxy::init m_pool.init() failed");
				break;
			}

			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
			if (bindToLocalhost)
			{
				addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
			}
			addr.sin_port = m_port;

			m_listenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);  
			if (m_listenSocket == INVALID_SOCKET)
				break;  

			if (bind(m_listenSocket, (SOCKADDR*)&addr, sizeof(addr)) != 0)
				break;

			if (listen(m_listenSocket, SOMAXCONN) != 0)
				break;

			m_service.registerSocket(m_listenSocket);

			if (!startAccept(AF_INET))
				break;

			m_ipv4Available = true;

			DbgPrint("TCPProxy::init IPv4 listen socket initialized");

			result = true;
			break;
		}

		if (result)
		for (;;)
		{
			sockaddr_in6 addr;

			memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
			if (bindToLocalhost)
			{
				addr.sin6_addr.u.Byte[15] = 1;
			}
			addr.sin6_port = m_port;

			m_listenSocket_IPv6 = WSASocket(AF_INET6, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);  
			if (m_listenSocket_IPv6 == INVALID_SOCKET)
				break;  

			if (bind(m_listenSocket_IPv6, (SOCKADDR*)&addr, sizeof(addr)) != 0)
				break;

			if (listen(m_listenSocket_IPv6, SOMAXCONN) != 0)
				break;

			m_service.registerSocket(m_listenSocket_IPv6);

			if (!startAccept(AF_INET6))
				break;

			DbgPrint("TCPProxy::init IPv6 listen socket initialized");

			m_ipv6Available = true;

			break;
		}

		if (!result)
		{
			free();
		}

		return result;
	}

	virtual void free()
	{
		m_service.free();
		m_pool.free();

		InitializeListHead(&m_eventList);

		PLIST_ENTRY p;
		OV_DATA * pov;
		while (!IsListEmpty(&m_ovDataList))
		{
			p = RemoveHeadList(&m_ovDataList);
			pov = CONTAINING_RECORD(p, OV_DATA, entry);
			delete pov;
		}

		while (!m_socketMap.empty())
		{
			tSocketMap::iterator it = m_socketMap.begin();
			delete it->second;
			m_socketMap.erase(it);
		}

		if (m_listenSocket != INVALID_SOCKET)
		{
			closesocket(m_listenSocket);
			m_listenSocket = INVALID_SOCKET;
		}
		if (m_acceptSocket != INVALID_SOCKET)
		{
			closesocket(m_acceptSocket);
			m_acceptSocket = INVALID_SOCKET;
		}
		if (m_listenSocket_IPv6 != INVALID_SOCKET)
		{
			closesocket(m_listenSocket_IPv6);
			m_listenSocket_IPv6 = INVALID_SOCKET;
		}
		if (m_acceptSocket_IPv6 != INVALID_SOCKET)
		{
			closesocket(m_acceptSocket_IPv6);
			m_acceptSocket_IPv6 = INVALID_SOCKET;
		}

		m_port = 0;
		m_pPFEventHandler = NULL;
		m_ipv4Available = false;
		m_ipv6Available = false;
	}

	bool setProxy(NFAPI_NS ENDPOINT_ID id, PROXY_TYPE proxyType, 
		const char * proxyAddress, int proxyAddressLen,
		const char * userName = NULL, const char * userPassword = NULL)
	{
		DbgPrint("TCPProxy::setProxy[%I64u], type=%d", id, proxyType);

		if (id == 0)
		{
			if (proxyAddress)
			{
				memcpy(m_proxyAddress, proxyAddress, 
					(proxyAddressLen < sizeof(m_proxyAddress))? proxyAddressLen : sizeof(m_proxyAddress));
				m_proxyAddressLen = proxyAddressLen;
				m_proxyType = proxyType;
				
				if (userName)
				{
					m_userName = userName;
				}
				if (userPassword)
				{
					m_userPassword = userPassword;
				}
			} else
			{
				m_proxyType = PROXY_NONE;
				m_proxyAddressLen = 0;
				m_userName = "";
				m_userPassword = "";
			}

			return true;
		}

		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		if (proxyAddress)
		{
			memcpy(pd->proxyAddress, proxyAddress, 
				(proxyAddressLen < sizeof(pd->proxyAddress))? proxyAddressLen : sizeof(pd->proxyAddress));
			pd->proxyAddressLen = proxyAddressLen;
			pd->proxyType = proxyType;

			if (userName)
			{
				pd->userName = userName;
			}
			if (userPassword)
			{
				pd->userPassword = userPassword;
			}
		} else
		{
			pd->proxyType = PROXY_NONE;
			pd->proxyAddressLen = 0;
			pd->userName = "";
			pd->userPassword = "";
		}

		return true;
	}

	void setEventHandler(NFAPI_NS NF_EventHandler * pEventHandler)
	{
		m_pPFEventHandler = pEventHandler;
	}

	void setTimeout(DWORD value)
	{
		AutoLock lock(m_cs);

		if (value < 5)
			value = 5;

		m_timeout = value * 1000;
	}

	virtual bool getRemoteAddress(sockaddr * pRemoteAddr, LOCAL_PROXY_CONN_INFO * pConnInfo) = 0;

	bool tcpPostSend(NFAPI_NS ENDPOINT_ID id, const char * buf, int len)
	{
		DbgPrint("TCPProxy::tcpPostSend[%I64u], len=%d", id, len);

		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		if (pd->offline)
		{
			return true;
		}

		return startTcpSend(pd, OUT_SOCKET, buf, len, id);
	}

	bool tcpPostReceive(NFAPI_NS ENDPOINT_ID id, const char * buf, int len)
	{
		DbgPrint("TCPProxy::tcpPostReceive[%I64u], len=%d", id, len);
		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		return startTcpSend(pd, IN_SOCKET, buf, len, id);
	}

	bool tcpSetState(NFAPI_NS ENDPOINT_ID id, bool suspended)
	{
		DbgPrint("TCPProxy::tcpSetState[%I64u], suspended=%d", id, suspended);

		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		pd->suspended = suspended;

		if (!suspended)
		{
			if (pd->proxyState == PS_CONNECTED)
			{
				startTcpReceive(pd, IN_SOCKET, id);
				
				if (!pd->offline)
				{
					startTcpReceive(pd, OUT_SOCKET, id);
				}
			}
		}

		return true;
	}

	bool tcpClose(NFAPI_NS ENDPOINT_ID id)
	{
		DbgPrint("TCPProxy::tcpClose[%I64u]", id);

		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		if (pd->proxyState != PS_CLOSED)
		{
			pd->proxyState = PS_CLOSED;
			releaseProxyData(pd);
		}
		return true;
	}

	bool getTCPConnInfo(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_TCP_CONN_INFO pConnInfo)
	{
		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		*pConnInfo = pd->connInfo;
		return true;
	}

protected:

	OV_DATA * newOV_DATA()
	{
		OV_DATA * pov = new OV_DATA();
		AutoLock lock(m_cs);
		InsertTailList(&m_ovDataList, &pov->entry);
		m_ovDataCounter++;
		return pov;
	}

	void deleteOV_DATA(OV_DATA * pov)
	{
		AutoLock lock(m_cs);
		RemoveEntryList(&pov->entry);
		InitializeListHead(&pov->entry);
		delete pov;
		m_ovDataCounter--;
		DbgPrint("TCPProxy::deleteOV_DATA ov set size=%d", m_ovDataCounter);
	}

	class AutoProxyData
	{
	public:
		AutoProxyData(TCPProxy * pThis, NFAPI_NS ENDPOINT_ID id)
		{
			thisClass = pThis;
			ptr = pThis->findProxyData(id);
			if (ptr)
			{
				ptr->lock.Lock();
			}
		}
		~AutoProxyData()
		{
			if (ptr)
			{
				ptr->lock.Unlock();
				thisClass->releaseProxyData(ptr);
			}
		}
		PROXY_DATA * operator ->()
		{
			return ptr;
		}
		operator PROXY_DATA*()
		{
			return ptr;
		}
		TCPProxy * thisClass;
		PROXY_DATA * ptr;
	};

	PROXY_DATA * findProxyData(NFAPI_NS ENDPOINT_ID id)
	{
		AutoLock lock(m_cs);

		tSocketMap::iterator it = m_socketMap.find(id);
		if (it == m_socketMap.end())
			return NULL;

		if (it->second->proxyState == PS_CLOSED)
			return NULL;

		it->second->refCount++;

		return it->second;
	}

	int releaseProxyData(PROXY_DATA * pd)
	{
		AutoLock lock(m_cs);
		
		if (pd->refCount > 0)
		{
			pd->refCount--;
		}
		
		if (pd->refCount == 0)
		{
			DbgPrint("TCPProxy::releaseProxyData[%I64u] delete", pd->id);

			if (m_pPFEventHandler)
			{
				m_pPFEventHandler->tcpClosed(pd->id, &pd->connInfo);
			}

			m_socketMap.erase(pd->id);
			delete pd;

			DbgPrint("TCPProxy::releaseProxyData socket map size=%d", m_socketMap.size());

			return 0;
		}
		return pd->refCount;
	}

	void * getExtensionFunction(SOCKET s, const GUID *which_fn)
	{
		void *ptr = NULL;
		DWORD bytes=0;
		WSAIoctl(s, 
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			(GUID*)which_fn, sizeof(*which_fn),
			&ptr, sizeof(ptr),
			&bytes, 
			NULL, 
			NULL);
		return ptr;
	}

	bool initExtensions()
	{
		const GUID acceptex = WSAID_ACCEPTEX;
		const GUID connectex = WSAID_CONNECTEX;
		const GUID getacceptexsockaddrs = WSAID_GETACCEPTEXSOCKADDRS;

		SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET)
			return false;

		m_pAcceptEx = (LPFN_ACCEPTEX)getExtensionFunction(s, &acceptex);
		m_pConnectEx = (LPFN_CONNECTEX)getExtensionFunction(s, &connectex);
		m_pGetAcceptExSockaddrs = (LPFN_GETACCEPTEXSOCKADDRS)getExtensionFunction(s, &getacceptexsockaddrs);
		
		closesocket(s);	

		return m_pAcceptEx!= NULL && m_pConnectEx != NULL && m_pGetAcceptExSockaddrs != NULL;
	}

	bool startAccept(int ipFamily)
	{
		SOCKET s = WSASocket(ipFamily, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (s == INVALID_SOCKET) 
		{
			return false;
		}

		OV_DATA * pov = newOV_DATA();
		DWORD dwBytes;

		pov->type = OVT_ACCEPT;
		pov->packetList.push_back(TCP_PACKET(NULL, 2 * (sizeof(sockaddr_in6) + 16)));

		if (ipFamily == AF_INET)
		{
			m_acceptSocket = s;
		} else
		{
			m_acceptSocket_IPv6 = s;
		}

		if (!m_pAcceptEx((ipFamily == AF_INET)? m_listenSocket : m_listenSocket_IPv6, 
					 s, 
					 pov->packetList[0].buffer.buf,
					 0,
					 sizeof(sockaddr_in6) + 16, 
					 sizeof(sockaddr_in6) + 16, 
					 &dwBytes, 
					 &pov->ol))
		{
			if (WSAGetLastError() != ERROR_IO_PENDING)
			{
				closesocket(s);
				deleteOV_DATA(pov);
				return false;
			}
		} 

		return true;
	}


	bool startConnect(SOCKET socket, sockaddr * pAddr, int addrLen, NFAPI_NS ENDPOINT_ID id)
	{
		OV_DATA * pov = newOV_DATA();
		pov->type = OVT_CONNECT;
		pov->id = id;

		DbgPrint("TCPProxy::startConnect[%I64u], socket=%d", id, socket);

		if (pAddr->sa_family == AF_INET)
		{
			struct sockaddr_in addr;
			ZeroMemory(&addr, sizeof(addr));
			addr.sin_family = AF_INET;
			
			bind(socket, (SOCKADDR*) &addr, sizeof(addr));
		} else
		{
			struct sockaddr_in6 addr;
			ZeroMemory(&addr, sizeof(addr));
			addr.sin6_family = AF_INET6;
			
			bind(socket, (SOCKADDR*) &addr, sizeof(addr));
		}

		if (!m_pConnectEx(socket, pAddr, addrLen, NULL, 0, NULL, &pov->ol))
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				DbgPrint("TCPProxy::startConnect[%I64u] failed, err=%d", id, err);
				deleteOV_DATA(pov);
				return false;
			}
		} 

		return true;
	}

	bool startTcpSend(PROXY_DATA * pd, bool isInSocket, const char * buf, int len, NFAPI_NS ENDPOINT_ID id)
	{
		DbgPrint("TCPProxy::startTcpSend[%I64u] isInSocket=%d, len=%d", id, isInSocket, len);

		SOCKET_DATA * psd;

		if (isInSocket)
		{
			psd = &pd->inSocket;
		} else
		{
			psd = &pd->outSocket;
		}

		if (len == 0)
		{
			psd->disconnect = true;
		} else
		if (len != -1)
		{
			psd->packetList.push_back(TCP_PACKET(buf, len));
		}

		if (psd->sendInProgress)
		{
			return true;
		}

		if (psd->packetList.empty())
		{
			if (psd->disconnect)
			{
				if (isInSocket)
				{
					startClose(pd->outSocket.socket, id);
				} else
				{
					startClose(pd->inSocket.socket, id);
				}
			}
			return true;
		}

		OV_DATA * pov;
		DWORD dwBytes;

		pov = newOV_DATA();

		pov->type = OVT_SEND;
		pov->id = id;
		pov->packetList = psd->packetList;

		psd->packetList.clear();

		if (psd->closed ||
			psd->socket == INVALID_SOCKET)
		{
			pov->type = OVT_RECEIVE;
			if (!m_service.postCompletion(psd->socket, 0, &pov->ol))
			{
				deleteOV_DATA(pov);
			}
			return false;
		}

		psd->sendInProgress = true;

		if (WSASend(psd->socket, 
				&pov->packetList[0](), (DWORD)pov->packetList.size(), 
				&dwBytes, 0, &pov->ol, NULL) != 0)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				DbgPrint("TCPProxy::startTcpSend[%I64u] failed, err=%d", id, err);
				
				psd->closed = true;
				psd->sendInProgress = false;

				pov->type = OVT_RECEIVE;
				if (!m_service.postCompletion(psd->socket, 0, &pov->ol))
				{
					deleteOV_DATA(pov);
				}
				
				return false;
			}
		}

		return true;
	}

	bool startTcpReceive(PROXY_DATA	* pd, bool isInSocket, NFAPI_NS ENDPOINT_ID id)
	{
		DbgPrint("TCPProxy::startTcpReceive[%I64u] isInSocket=%d", id, isInSocket);

		SOCKET_DATA * psd;

		if (isInSocket)
		{
			psd = &pd->inSocket;
		} else
		{
			psd = &pd->outSocket;
		}

		if (psd->closed || 
			psd->disconnected || 
			psd->socket == INVALID_SOCKET)
		{
			return false;
		}

		if (psd->receiveInProgress)
		{
			return true;
		}

		if (pd->suspended && 
			pd->proxyState == PS_CONNECTED &&
			!psd->disconnect)
		{
			return true;
		}

		OV_DATA * pov;
		DWORD dwBytes, dwFlags;

		pov = newOV_DATA();
		pov->type = OVT_RECEIVE;
		pov->id = id;
		pov->packetList.push_back(TCP_PACKET(NULL, TCP_PACKET_SIZE));

		dwFlags = 0;

		psd->receiveInProgress = true;

		if (WSARecv(psd->socket, &pov->packetList[0](), 1, &dwBytes, &dwFlags, &pov->ol, NULL) != 0)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				DbgPrint("TCPProxy::startTcpReceive[%I64u] failed, err=%d", id, err);

				psd->closed = true;
				psd->receiveInProgress = false;

				if (!m_service.postCompletion(psd->socket, 0, &pov->ol))
				{
					deleteOV_DATA(pov);
				}

				return false;
			}
		} 
	
		return true;
	}

	bool startClose(SOCKET socket, NFAPI_NS ENDPOINT_ID id)
	{
		DbgPrint("TCPProxy::startClose[%I64u] socket %d", 
			id,
			socket);

		OV_DATA * pov = newOV_DATA();
		pov->type = OVT_CLOSE;
		pov->id = id;
		return m_service.postCompletion(socket, 0, &pov->ol);
	}

	void onConnectComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		if (error != 0)
		{
			DbgPrint("TCPProxy::onConnectComplete[%I64u] failed, err=%d", pov->id, error);
			startClose(socket, pov->id);
			return;
		}

		DbgPrint("TCPProxy::onConnectComplete[%I64u] err=%d", pov->id, error);

		AutoProxyData pd(this, pov->id);
		if (!pd)
			return;

		BOOL val = 1;
		setsockopt(socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
		setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&val, sizeof(val));
		setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(val));

		setKeepAliveVals(socket);

		if (pd->proxyType == PROXY_SOCKS5)
		{
			SOCKS5_AUTH_REQUEST authReq;

			authReq.version = SOCKS_5;
			authReq.nmethods = 1;

			if (!pd->userName.empty())
			{
				authReq.methods[0] = S5AM_UNPW;
			} else
			{
				authReq.methods[0] = S5AM_NONE;
			}

			if (startTcpSend(pd, OUT_SOCKET, (char*)&authReq, sizeof(authReq), pov->id))
			{
				pd->proxyState = PS_AUTH;
			}
			
			startTcpReceive(pd, OUT_SOCKET, pov->id);			
		} else
		{
			pd->proxyState = PS_CONNECTED;

			startTcpReceive(pd, IN_SOCKET, pov->id);
			startTcpReceive(pd, OUT_SOCKET, pov->id);	

			if (m_pPFEventHandler)
			{
				m_pPFEventHandler->tcpConnected(pov->id, &pd->connInfo);
			}
		}
	}
	
	void onSendComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		DbgPrint("TCPProxy::onSendComplete[%I64u] socket=%d, bytes=%d", pov->id, socket, dwTransferred);

		AutoProxyData pd(this, pov->id);
		if (!pd)
			return;

		SOCKET_DATA * psd;
		bool isInSocket;

		if (pd->inSocket.socket == socket)
		{
			isInSocket = true;
			psd = &pd->inSocket;
		} else
		{
			isInSocket = false;
			psd = &pd->outSocket;
		}

		psd->sendInProgress = false;

		if (dwTransferred == 0)
		{
			DbgPrint("TCPProxy::onSendComplete[%I64u] error=%d", pov->id, error);

			psd->closed = true;
			
			pov = newOV_DATA();
			pov->type = OVT_RECEIVE;
			if (!m_service.postCompletion(socket, 0, &pov->ol))
			{
				deleteOV_DATA(pov);
			}
			return;
		}

		if (psd->disconnect || !psd->packetList.empty())
		{
			startTcpSend(pd, isInSocket, NULL, -1, pov->id);
		} else
		if (pd->proxyState == PS_CONNECTED)
		{
			if (m_pPFEventHandler)
			{
				if (isInSocket)
				{
					m_pPFEventHandler->tcpCanReceive(pov->id);
				} else
				{
					m_pPFEventHandler->tcpCanSend(pov->id);
				}
			}

			if (!pd->offline)
			{
				startTcpReceive(pd, !isInSocket, pov->id);
			}
		}
	}

	void onReceiveComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		DbgPrint("TCPProxy::onReceiveComplete[%I64u] socket=%d, bytes=%d", pov->id, socket, dwTransferred);

		AutoProxyData pd(this, pov->id);
		if (!pd)
			return;

		SOCKET_DATA * psd;
		bool isInSocket;

		if (pd->inSocket.socket == socket)
		{
			isInSocket = true;
			psd = &pd->inSocket;
		} else
		{
			isInSocket = false;
			psd = &pd->outSocket;
		}

		psd->receiveInProgress = false;

		if (dwTransferred == 0)
		{
			if (error != 0)
			{
				DbgPrint("TCPProxy::onReceiveComplete[%I64u] error=%d", pov->id, error);
				psd->closed = true;
			}

			if (pd->proxyState == PS_CONNECTED &&
				m_pPFEventHandler && 
				pd->connInfo.filteringFlag & NFAPI_NS NF_FILTER)
			{
				if (pd->outSocket.socket == socket)
				{
					pd->outSocket.disconnected = true;
					m_pPFEventHandler->tcpReceive(pov->id, NULL, 0);
				} else
				{
					pd->inSocket.disconnected = true;
					if (pd->offline)
					{
						m_pPFEventHandler->tcpReceive(pov->id, NULL, 0);
					}
					m_pPFEventHandler->tcpSend(pov->id, NULL, 0);
				}
			} else
			{
				startClose(socket, pov->id);
			}

			return;
		}

		if (isInSocket == OUT_SOCKET)
		{
			if (pd->proxyState == PS_CONNECTED)
			{
				if (m_pPFEventHandler && 
					pd->connInfo.filteringFlag & NFAPI_NS NF_FILTER)
				{
					m_pPFEventHandler->tcpReceive(pov->id, pov->packetList[0].buffer.buf, dwTransferred);
					
					startTcpReceive(pd, OUT_SOCKET, pov->id);
				} else
				{
					if (!startTcpSend(pd, IN_SOCKET, pov->packetList[0].buffer.buf, dwTransferred, pov->id))
					{
						DbgPrint("TCPProxy::onReceiveComplete startTcpSend failed");
					}
				}
			} else
			{
				switch (pd->proxyState)
				{
				case PS_NONE:
					break;

				case PS_AUTH:
					{
						if (dwTransferred < sizeof(SOCK5_AUTH_RESPONSE))
							break;

						SOCK5_AUTH_RESPONSE * pr = (SOCK5_AUTH_RESPONSE *)pov->packetList[0].buffer.buf;
						
						if (pr->version != SOCKS_5)
						{
							break;
						}

						if (pr->method == S5AM_UNPW && !pd->userName.empty())
						{
							std::vector<char> authReq;

							authReq.push_back(1);
							authReq.push_back((char)pd->userName.length());
							authReq.insert(authReq.end(), pd->userName.begin(), pd->userName.end());
							authReq.push_back((char)pd->userPassword.length());
							
							if (!pd->userPassword.empty())
								authReq.insert(authReq.end(), pd->userPassword.begin(), pd->userPassword.end());

							if (startTcpSend(pd, OUT_SOCKET, (char*)&authReq[0], (int)authReq.size(), pov->id))
							{
								pd->proxyState = PS_AUTH_NEGOTIATION;
							}

							break;
						}

						char * realRemoteAddress = (char*)pd->connInfo.remoteAddress;
						int realRemoteAddressLen = (((sockaddr*)realRemoteAddress)->sa_family == AF_INET)? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

						int ipFamily = ((sockaddr*)realRemoteAddress)->sa_family;

						if (ipFamily == AF_INET)
						{
							SOCKS5_REQUEST_IPv4 req;

							req.version = SOCKS_5;
							req.command = S5C_CONNECT;
							req.reserved = 0;
							req.address_type = SOCKS5_ADDR_IPV4;
							req.address = ((sockaddr_in*)realRemoteAddress)->sin_addr.S_un.S_addr;
							req.port = ((sockaddr_in*)realRemoteAddress)->sin_port;

							if (startTcpSend(pd, OUT_SOCKET, (char*)&req, sizeof(req), pov->id))
							{
								pd->proxyState = PS_CONNECT;
							}		
						} else
						{
							SOCKS5_REQUEST_IPv6 req;

							req.version = SOCKS_5;
							req.command = S5C_CONNECT;
							req.reserved = 0;
							req.address_type = SOCKS5_ADDR_IPV6;
							memcpy(&req.address, &((sockaddr_in6*)realRemoteAddress)->sin6_addr, 16);
							req.port = ((sockaddr_in6*)realRemoteAddress)->sin6_port;

							if (startTcpSend(pd, OUT_SOCKET, (char*)&req, sizeof(req), pov->id))
							{
								pd->proxyState = PS_CONNECT;
							}
						}
					}
					break;

				case PS_AUTH_NEGOTIATION:
					{
						if (dwTransferred < sizeof(SOCK5_AUTH_RESPONSE))
							break;

						SOCK5_AUTH_RESPONSE * pr = (SOCK5_AUTH_RESPONSE *)pov->packetList[0].buffer.buf;
						
						if (pr->version != 0x01 || pr->method != 0x00)
						{
							break;
						}

						char * realRemoteAddress = (char*)pd->connInfo.remoteAddress;
						int realRemoteAddressLen = (((sockaddr*)realRemoteAddress)->sa_family == AF_INET)? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

						int ipFamily = ((sockaddr*)realRemoteAddress)->sa_family;

						if (ipFamily == AF_INET)
						{
							SOCKS5_REQUEST_IPv4 req;

							req.version = SOCKS_5;
							req.command = S5C_CONNECT;
							req.reserved = 0;
							req.address_type = SOCKS5_ADDR_IPV4;
							req.address = ((sockaddr_in*)realRemoteAddress)->sin_addr.S_un.S_addr;
							req.port = ((sockaddr_in*)realRemoteAddress)->sin_port;

							if (startTcpSend(pd, OUT_SOCKET, (char*)&req, sizeof(req), pov->id))
							{
								pd->proxyState = PS_CONNECT;
							}		
						} else
						{
							SOCKS5_REQUEST_IPv6 req;

							req.version = SOCKS_5;
							req.command = S5C_CONNECT;
							req.reserved = 0;
							req.address_type = SOCKS5_ADDR_IPV6;
							memcpy(&req.address, &((sockaddr_in6*)realRemoteAddress)->sin6_addr, 16);
							req.port = ((sockaddr_in6*)realRemoteAddress)->sin6_port;

							if (startTcpSend(pd, OUT_SOCKET, (char*)&req, sizeof(req), pov->id))
							{
								pd->proxyState = PS_CONNECT;
							}
						}
					}
					break;

				case PS_CONNECT:
					{
						if (dwTransferred < sizeof(SOCKS5_RESPONSE))
							break;

						SOCKS5_RESPONSE * pr = (SOCKS5_RESPONSE *)pov->packetList[0].buffer.buf;
						
						if (pr->version != SOCKS_5 || pr->res_code != 0)
							break;
						
						DWORD responseLen = (pr->address_type == SOCKS5_ADDR_IPV4)? 
							sizeof(SOCKS5_RESPONSE_IPv4) : sizeof(SOCKS5_RESPONSE_IPv6);

						pd->proxyState = PS_CONNECTED;
						
						if (m_pPFEventHandler)
						{
							m_pPFEventHandler->tcpConnected(pov->id, &pd->connInfo);
						}

						if (dwTransferred > responseLen)
						{
							if (m_pPFEventHandler && 
								pd->connInfo.filteringFlag & NFAPI_NS NF_FILTER)
							{
								m_pPFEventHandler->tcpReceive(pov->id, 
									pov->packetList[0].buffer.buf + responseLen, 
									dwTransferred - responseLen);
							} else
							{
								if (!startTcpSend(pd, IN_SOCKET, 
									pov->packetList[0].buffer.buf + responseLen, 
									dwTransferred - responseLen, pov->id))
								{
									DbgPrint("TCPProxy::onReceiveComplete startTcpSend failed");
								}
							}
						}

						startTcpReceive(pd, IN_SOCKET, pov->id);
						startTcpReceive(pd, OUT_SOCKET, pov->id);
					}
					break;
				}

				startTcpReceive(pd, OUT_SOCKET, pov->id);
			}

		} else
		// IN_SOCKET
		{
			if (pd->proxyState == PS_CONNECTED)
			{
				if (m_pPFEventHandler &&
					pd->connInfo.filteringFlag & NFAPI_NS NF_FILTER)
				{
					m_pPFEventHandler->tcpSend(pov->id, pov->packetList[0].buffer.buf, dwTransferred);

					startTcpReceive(pd, IN_SOCKET, pov->id);
				} else
				{
					if (!startTcpSend(pd, OUT_SOCKET, pov->packetList[0].buffer.buf, dwTransferred, pov->id))
					{
						DbgPrint("TCPProxy::onReceiveComplete startTcpSend failed");
					}
				}
			} else
			{
				startClose(socket, pov->id);
			}
		}
	}

	void setKeepAliveVals(SOCKET s)
	{
		tcp_keepalive tk;
		DWORD dwRet;

		{
			AutoLock lock(m_cs);

			tk.onoff = 1;
			tk.keepalivetime = m_timeout;
			tk.keepaliveinterval = 1000;
		}

		int err = WSAIoctl(s, SIO_KEEPALIVE_VALS,
		  (LPVOID) &tk,    
		  (DWORD) sizeof(tk), 
		  NULL,         
		  0,       
		  (LPDWORD) &dwRet,
		  NULL,
		  NULL);	
		if (err != 0)
		{
			DbgPrint("TCPProxy::setKeepAliveVals WSAIoctl err=%d", WSAGetLastError());
		}
	}

	void onAcceptComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		sockaddr * pLocalAddr = NULL;
		sockaddr * pRemoteAddr = NULL;
		int localAddrLen, remoteAddrLen;
		char	realRemoteAddress[NF_MAX_ADDRESS_LENGTH];
		SOCKET acceptSocket;
		int ipFamily;
		bool result = false;

		if (socket == m_listenSocket)
		{
			acceptSocket = m_acceptSocket;
			ipFamily = AF_INET;
		} else
		{
			acceptSocket = m_acceptSocket_IPv6;
			ipFamily = AF_INET6;
		}

		if (error != 0)
		{
			DbgPrint("TCPProxy::onAcceptComplete() failed, err=%d", error);

			closesocket(acceptSocket);

			if (!startAccept(ipFamily))
			{
				DbgPrint("TCPProxy::startAccept() failed");
			}
			return;
		}

		m_pGetAcceptExSockaddrs(pov->packetList[0].buffer.buf,
			0,
			sizeof(sockaddr_in6) + 16,
			sizeof(sockaddr_in6) + 16,
			&pLocalAddr,
			&localAddrLen,
			&pRemoteAddr,
			&remoteAddrLen);

		{
#if defined(_DEBUG) || defined(_RELEASE_LOG)
			char localAddr[MAX_PATH] = "";
			char remoteAddr[MAX_PATH] = "";
			DWORD dwLen;
			sockaddr * pAddr;
	
			pAddr = pLocalAddr;
			dwLen = sizeof(localAddr);

			WSAAddressToString((LPSOCKADDR)pAddr, 
						(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
						NULL, 
						localAddr, 
						&dwLen);

			pAddr = pRemoteAddr;
			dwLen = sizeof(remoteAddr);

			WSAAddressToString((LPSOCKADDR)pAddr, 
						(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
						NULL, 
						remoteAddr, 
						&dwLen);

			DbgPrint("TCPProxy::onAcceptComplete() accepted socket %d, %s <- %s", 
				acceptSocket,
				localAddr,
				remoteAddr);
#endif
		}

		m_service.registerSocket(acceptSocket);

		setsockopt(acceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&m_listenSocket, sizeof(m_listenSocket));

		BOOL val = 1;
		setsockopt(acceptSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&val, sizeof(val));
		setsockopt(acceptSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(val));

		setKeepAliveVals(acceptSocket);

		{
			AutoLock lock(m_cs);

			PROXY_DATA * pd = new PROXY_DATA();

			pd->inSocket.socket = acceptSocket;

			unsigned short	remotePort;
			if (((sockaddr*)pLocalAddr)->sa_family == AF_INET)
			{
				remotePort = ((sockaddr_in*)pRemoteAddr)->sin_port;
			} else
			{
				remotePort = ((sockaddr_in6*)pRemoteAddr)->sin6_port;
			}
			
			m_connId++;

			pd->id = m_connId;
			
			m_socketMap[pd->id] = pd;

			pd->connInfo.ip_family = ((sockaddr*)pLocalAddr)->sa_family;
			pd->connInfo.direction = NFAPI_NS NF_D_OUT;
			memcpy(pd->connInfo.localAddress, pRemoteAddr, remoteAddrLen);

			bool addressFound = false;

			LOCAL_PROXY_CONN_INFO connInfo;

			if (getRemoteAddress(pRemoteAddr, &connInfo))
			{
				pd->connInfo = connInfo.connInfo;
				pd->proxyType = connInfo.proxyType;
				memcpy(pd->proxyAddress, connInfo.proxyAddress, sizeof(pd->proxyAddress));
				pd->proxyAddressLen = connInfo.proxyAddressLen;
				pd->userName = connInfo.userName;
				pd->userPassword = connInfo.userPassword;

				addressFound = true;

				if (m_pPFEventHandler)
				{
					pd->connInfo.filteringFlag |= NFAPI_NS NF_FILTER;
					
					m_pPFEventHandler->tcpConnectRequest(pd->id, &pd->connInfo);

					if (pd->connInfo.filteringFlag & NFAPI_NS NF_BLOCK)
					{
						addressFound = false;
					} else
					if (pd->connInfo.filteringFlag & NFAPI_NS NF_OFFLINE)
					{
						pd->offline = true;
						result = true;
					}
				}

				if (m_proxyType != PROXY_NONE)
				{
					pd->proxyType = m_proxyType;
					memcpy(pd->proxyAddress, m_proxyAddress, sizeof(m_proxyAddress));
					pd->proxyAddressLen = m_proxyAddressLen;
					pd->userName = m_userName;
					pd->userPassword = m_userPassword;
				}

				if (pd->proxyType == PROXY_SOCKS5)
				{
#if defined(_DEBUG) || defined(_RELEASE_LOG)
					char remoteAddr[MAX_PATH] = "";
					DWORD dwLen;
					sockaddr * pAddr;

					pAddr = (sockaddr*)pd->proxyAddress;
					dwLen = sizeof(remoteAddr);

					WSAAddressToString((LPSOCKADDR)pAddr, 
								(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
								NULL, 
								remoteAddr, 
								&dwLen);

					DbgPrint("TCPProxy::onAcceptComplete() redirect to SOCKS proxy %s", remoteAddr);
#endif

					memcpy(realRemoteAddress, pd->proxyAddress, sizeof(realRemoteAddress));
				} else
				{
					memcpy(realRemoteAddress, pd->connInfo.remoteAddress, sizeof(realRemoteAddress));
				}
			} else
			{
				DbgPrint("TCPProxy::onAcceptComplete real remote address not found");
			}

			if (addressFound)
			{
				if (pd->offline)
				{
					pd->proxyState = PS_CONNECTED;
					pd->outSocket.disconnected = true;

					startTcpReceive(pd, IN_SOCKET, pd->id);

					if (m_pPFEventHandler)
					{
						m_pPFEventHandler->tcpConnected(pd->id, &pd->connInfo);
					}
				} else
				{

					for (;;)
					{
						SOCKET tcpSocket = WSASocket(((sockaddr*)realRemoteAddress)->sa_family, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);  
						if (tcpSocket == INVALID_SOCKET)
						{
							DbgPrint("TCPProxy::onAcceptComplete WSASocket failed");
							break;  
						}

						pd->outSocket.socket = tcpSocket;

						if (!m_service.registerSocket(tcpSocket))
						{
							DbgPrint("TCPProxy::onAcceptComplete registerSocket failed");
							break;
						}

						if (!startConnect(tcpSocket, 
								(sockaddr*)realRemoteAddress, 
								(((sockaddr*)realRemoteAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
								pd->id))
						{
							DbgPrint("TCPProxy::onAcceptComplete startConnect failed");
							break;
						}

						result = true;

						break;
					}
				}
			}

			if (!result)
			{
				m_socketMap.erase(pd->id);
				delete pd;
			}
		}

		if (!startAccept(ipFamily))
		{
			DbgPrint("TCPProxy::startAccept() failed");
		}
	}

	void onClose(SOCKET socket, OV_DATA * pov)
	{
		bool close = false;

		DbgPrint("TCPProxy::onClose[%I64u] socket=%d", pov->id, socket);
		
		AutoProxyData pd(this, pov->id);
		if (!pd)
			return;

		SOCKET_DATA *psd, *psd2;
		bool isInSocket2;

		if (pd->inSocket.socket == socket)
		{
			psd = &pd->inSocket;
			psd2 = &pd->outSocket;
			isInSocket2 = false;
		} else
		{
			psd = &pd->outSocket;
			psd2 = &pd->inSocket;
			isInSocket2 = true;
		}

		psd->disconnected = true;

		if (!psd2->disconnected && 
			!psd2->closed && 
			psd2->socket != INVALID_SOCKET)
		{
			if (shutdown(psd2->socket, (psd->closed)? SD_BOTH : SD_SEND) == 0)
			{
				startTcpReceive(pd, isInSocket2, pov->id);			
			} else
			{
				DbgPrint("TCPProxy::onClose[%I64u] shutdown error=%d, closed", pov->id, WSAGetLastError());
				close = true;
			}
		} else
		{
			close = true;
		}

		if (close)
		{
			DbgPrint("TCPProxy::onClose[%I64u] closed", pov->id);

			if (pd->proxyState != PS_CLOSED)
			{
				pd->proxyState = PS_CLOSED;
				releaseProxyData(pd);
			}
		}
	}

	virtual void onComplete(SOCKET socket, DWORD dwTransferred, OVERLAPPED * pOverlapped, int error)
	{
		OV_DATA * pov = (OV_DATA*)pOverlapped;

		pov->socket = socket;
		pov->dwTransferred = dwTransferred;
		pov->error = error;

		{
			AutoLock lock(m_csEventList);
			InsertTailList(&m_eventList, &pov->entryEventList);
		}

		m_pool.jobAvailable();
	}

	virtual void execute()
	{
		OV_DATA * pov;

		{
			AutoLock lock(m_csEventList);
			
			if (IsListEmpty(&m_eventList))
				return;

			pov = CONTAINING_RECORD(m_eventList.Flink, OV_DATA, entryEventList);

			RemoveEntryList(&pov->entryEventList);
			InitializeListHead(&pov->entryEventList);
		}

		if (pov)
		{
			switch (pov->type)
			{
			case OVT_ACCEPT:
				onAcceptComplete(pov->socket, pov->dwTransferred, pov, pov->error);
				break;
			case OVT_CONNECT:
				onConnectComplete(pov->socket, pov->dwTransferred, pov, pov->error);
				break;
			case OVT_SEND:
				onSendComplete(pov->socket, pov->dwTransferred, pov, pov->error);
				break;
			case OVT_RECEIVE:
				onReceiveComplete(pov->socket, pov->dwTransferred, pov, pov->error);
				break;
			case OVT_CLOSE:
				onClose(pov->socket, pov);
				break;
			}

			deleteOV_DATA(pov);
		}

		{
			AutoLock lock(m_csEventList);
			if (!IsListEmpty(&m_eventList))
			{
				m_pool.jobAvailable();
			}
		}
	}

	virtual void threadStarted()
	{
		if (m_pPFEventHandler)
		{
			m_pPFEventHandler->threadStart();
		}
	}

	virtual void threadStopped()
	{
		if (m_pPFEventHandler)
		{
			m_pPFEventHandler->threadEnd();
		}
	}

private:
	IOCPService m_service;
	
	LIST_ENTRY	m_ovDataList;
	int m_ovDataCounter;

	typedef std::map<NFAPI_NS ENDPOINT_ID, PROXY_DATA*> tSocketMap;
	tSocketMap m_socketMap;

	LPFN_ACCEPTEX m_pAcceptEx;
	LPFN_CONNECTEX m_pConnectEx;
	LPFN_GETACCEPTEXSOCKADDRS m_pGetAcceptExSockaddrs;

	SOCKET m_listenSocket;
	SOCKET m_acceptSocket;

	SOCKET m_listenSocket_IPv6;
	SOCKET m_acceptSocket_IPv6;

	NFAPI_NS ENDPOINT_ID m_connId;

	unsigned short m_port;

	bool	m_ipv4Available;
	bool	m_ipv6Available;

	NFAPI_NS NF_EventHandler * m_pPFEventHandler;

	LIST_ENTRY	m_eventList;
	AutoCriticalSection m_csEventList;
	
	ThreadPool m_pool;

	DWORD m_timeout;

	PROXY_TYPE m_proxyType;
	char	m_proxyAddress[NF_MAX_ADDRESS_LENGTH];
	int		m_proxyAddressLen;

	std::string m_userName;
	std::string m_userPassword;

	AutoCriticalSection m_cs;
};

class LocalTCPProxy : public TCPProxy
{
public:
	void free()
	{
		TCPProxy::free();

		AutoLock lock(m_cs);
		m_connInfoMap.clear();
	}

	void setConnInfo(LOCAL_PROXY_CONN_INFO * pConnInfo)
	{
		AutoLock lock(m_cs);
		m_connInfoMap[((sockaddr_in*)pConnInfo->connInfo.localAddress)->sin_port] = *pConnInfo;
	}

	virtual bool getRemoteAddress(sockaddr * pRemoteAddr, LOCAL_PROXY_CONN_INFO * pConnInfo)
	{
		AutoLock lock(m_cs);

		unsigned short remotePort = 0;

		if (pRemoteAddr->sa_family == AF_INET)
		{
			remotePort = ((sockaddr_in*)pRemoteAddr)->sin_port;
		} else
		{
			remotePort = ((sockaddr_in6*)pRemoteAddr)->sin6_port;
		}

		tConnInfoMap::iterator it = m_connInfoMap.find(remotePort);
		if (it != m_connInfoMap.end())
		{
			*pConnInfo = it->second;
			m_connInfoMap.erase(it);
			return true;
		}
		return false;
	}

private:
	
	typedef std::map<unsigned short, LOCAL_PROXY_CONN_INFO> tConnInfoMap;
	tConnInfoMap m_connInfoMap;

	AutoCriticalSection m_cs;
};


}