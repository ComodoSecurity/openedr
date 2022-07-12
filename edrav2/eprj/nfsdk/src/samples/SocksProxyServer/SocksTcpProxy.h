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
#include "nfapi.h"


namespace TcpProxyServer
{

#define TCP_PACKET_SIZE 8192
#define UDP_PACKET_SIZE 65536
#define IN_SOCKET true
#define OUT_SOCKET false

struct DATA_PACKET
{
	DATA_PACKET()
	{
		buffer.len = 0;
		buffer.buf = NULL;
	}
	DATA_PACKET(char * buf, int len)
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

typedef std::vector<DATA_PACKET> tPacketList;

enum OV_TYPE
{
	OVT_ACCEPT,
	OVT_CONNECT,
	OVT_CLOSE,
	OVT_SEND,
	OVT_RECEIVE,
	OVT_UDP_SEND,
	OVT_UDP_RECEIVE
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
	nfapi::ENDPOINT_ID id;
	OV_TYPE		type;
	tPacketList packetList;

	char		remoteAddress[NF_MAX_ADDRESS_LENGTH];
	int			remoteAddressLen;

	SOCKET	socket;
	DWORD	dwTransferred;
	int		error;
};

enum PROXY_STATE
{
	PS_NONE,
	PS_REQUEST,
	PS_CONNECT,
	PS_CONNECT_SOCKS4,
	PS_CONNECTED_UDP,
	PS_CONNECTED,
	PS_ERROR,
	PS_CLOSED
};

struct TCP_SOCKET_DATA
{
	TCP_SOCKET_DATA()
	{
		socket = INVALID_SOCKET;
		disconnected = false;
		receiveInProgress = false;
		sendInProgress = false;
		disconnect = false;
		closed = false;
	}
	~TCP_SOCKET_DATA()
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

struct UDP_SOCKET_DATA
{
	UDP_SOCKET_DATA()
	{
		socket = INVALID_SOCKET;
		receiveInProgress = false;
	}
	~UDP_SOCKET_DATA()
	{
		if (socket != INVALID_SOCKET)
		{
			closesocket(socket);
		}
	}

	SOCKET	socket;
	bool	receiveInProgress;
	char	remoteAddress[NF_MAX_ADDRESS_LENGTH];
	int		remoteAddressLen;
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
		memset(&udpConnInfo, 0, sizeof(udpConnInfo));
		refCount = 1;
	}
	~PROXY_DATA()
	{
	}

	nfapi::ENDPOINT_ID id;

	TCP_SOCKET_DATA		inSocket;
	TCP_SOCKET_DATA		outSocket;

	UDP_SOCKET_DATA		inUdpSocket;
	UDP_SOCKET_DATA		outUdpSocket;

	PROXY_STATE		proxyState;
	nfapi::NF_TCP_CONN_INFO connInfo;
	nfapi::NF_UDP_CONN_INFO udpConnInfo;

	bool	suspended;
	bool	offline;
	
	int		refCount;
	AutoCriticalSection lock;
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
	}

	~TCPProxy()
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

	bool init(unsigned short port, int threadCount = 0)
	{
		bool result = false;

		if (!initExtensions())
			return false;

		if (!m_service.init(this))
			return false;

		if (!m_pool.init(threadCount, this))
			return false;

		InitializeListHead(&m_ovDataList);
		m_ovDataCounter = 0;

		InitializeListHead(&m_eventList);

		m_port = port;

		m_connId = 0;

		for (;;)
		{
			sockaddr_in addr;
			memset(&addr, 0, sizeof(addr));
			addr.sin_family = AF_INET;
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

			result = true;
			break;
		}

		for (;;)
		{
			sockaddr_in6 addr;

			memset(&addr, 0, sizeof(addr));
			addr.sin6_family = AF_INET6;
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

			m_ipv6Available = true;

			break;
		}

		if (!result)
		{
			free();
		}

		return result;
	}

	void free()
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

	void setEventHandler(nfapi::NF_EventHandler * pEventHandler)
	{
		m_pPFEventHandler = pEventHandler;
	}

	bool tcpPostSend(nfapi::ENDPOINT_ID id, char * buf, int len)
	{
		DbgPrint("TCPProxy::tcpPostSend[%I64u], len=%d", id, len);

		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		if (pd->offline)
		{
			return true;
		}

		return startTcpSend(pd, OUT_SOCKET, buf, len);
	}

	bool tcpPostReceive(nfapi::ENDPOINT_ID id, char * buf, int len)
	{
		DbgPrint("TCPProxy::tcpPostReceive[%I64u], len=%d", id, len);
		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		return startTcpSend(pd, IN_SOCKET, buf, len);
	}

	bool tcpSetState(nfapi::ENDPOINT_ID id, bool suspended)
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
				startTcpReceive(pd, IN_SOCKET);
				
				if (!pd->offline)
				{
					startTcpReceive(pd, OUT_SOCKET);
				}
			}
		}

		return true;
	}

	bool tcpClose(nfapi::ENDPOINT_ID id)
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

	bool udpPostSend(nfapi::ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len)
	{
		DbgPrint("TCPProxy::udpPostSend[%I64u], len=%d", id, len);

		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		if (pd->offline)
		{
			return true;
		}

		tPacketList packetList;
		packetList.push_back(DATA_PACKET((char*)buf, len));

		startUdpSend(pd, OUT_SOCKET, 
			packetList,
			(char*)remoteAddress, 
			(((sockaddr*)remoteAddress)->sa_family == AF_INET)? sizeof(sockaddr_in) : sizeof(sockaddr_in6));

		startUdpReceive(pd, OUT_SOCKET);

		return true;
	}

	bool udpPostReceive(nfapi::ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len)
	{
		DbgPrint("TCPProxy::udpPostReceive[%I64u], len=%d", id, len);
		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		if (((sockaddr*)remoteAddress)->sa_family == AF_INET)
		{
			sockaddr_in * addr = (sockaddr_in*)remoteAddress;
			
			SOCKS5_UDP_REQUEST_IPv4 req;

			req.reserved = 0;
			req.frag = 0;
			req.address_type = SOCKS5_ADDR_IPV4;
			req.address = addr->sin_addr.S_un.S_addr;
			req.port = addr->sin_port;

			tPacketList packetList;

			packetList.push_back(DATA_PACKET((char*)&req, sizeof(req)));
			packetList.push_back(DATA_PACKET((char*)buf, len));
			
			startUdpSend(pd, IN_SOCKET, 
				packetList,
				pd->inUdpSocket.remoteAddress, 
				pd->inUdpSocket.remoteAddressLen);
		} else
		{
			sockaddr_in6 * addr = (sockaddr_in6*)remoteAddress;
			
			SOCKS5_UDP_REQUEST_IPv6 req;

			req.reserved = 0;
			req.frag = 0;
			req.address_type = SOCKS5_ADDR_IPV6;
			memcpy(req.address, &addr->sin6_addr, 16);
			req.port = addr->sin6_port;

			tPacketList packetList;

			packetList.push_back(DATA_PACKET((char*)&req, sizeof(req)));
			packetList.push_back(DATA_PACKET((char*)buf, len));
			
			startUdpSend(pd, IN_SOCKET, 
				packetList,
				pd->inUdpSocket.remoteAddress, 
				pd->inUdpSocket.remoteAddressLen);
		}

		startUdpReceive(pd, OUT_SOCKET);

		return true;
	}

	bool udpSetState(nfapi::ENDPOINT_ID id, bool suspended)
	{
		DbgPrint("TCPProxy::udpSetState[%I64u], suspended=%d", id, suspended);

		AutoProxyData pd(this, id);
		if (!pd)
			return false;

		pd->suspended = suspended;

		if (!suspended)
		{
			if (pd->proxyState == PS_CONNECTED_UDP)
			{
				startUdpReceive(pd, IN_SOCKET);
				
				if (!pd->offline)
				{
					startUdpReceive(pd, OUT_SOCKET);
				}
			}
		}

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
		AutoProxyData(TCPProxy * pThis, nfapi::ENDPOINT_ID id)
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

	PROXY_DATA * findProxyData(nfapi::ENDPOINT_ID id)
	{
		AutoLock lock(m_cs);

		tSocketMap::iterator it = m_socketMap.find(id);
		if (it == m_socketMap.end())
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
				if (pd->inUdpSocket.socket != INVALID_SOCKET)
				{
					m_pPFEventHandler->udpClosed(pd->id, &pd->udpConnInfo);
				} else
				{
					m_pPFEventHandler->tcpClosed(pd->id, &pd->connInfo);
				}
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
		pov->packetList.push_back(DATA_PACKET(NULL, 2 * (sizeof(sockaddr_in6) + 16)));

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


	bool startConnect(SOCKET socket, sockaddr * pAddr, int addrLen, nfapi::ENDPOINT_ID id)
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

	bool startTcpSend(PROXY_DATA * pd, bool isInSocket, char * buf, int len)
	{
		DbgPrint("TCPProxy::startTcpSend[%I64u] isInSocket=%d, len=%d", pd->id, isInSocket, len);

		TCP_SOCKET_DATA * psd;

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
			psd->packetList.push_back(DATA_PACKET(buf, len));
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
					startClose(pd->outSocket.socket, pd->id);
				} else
				{
					startClose(pd->inSocket.socket, pd->id);
				}
			}
			return true;
		}

		OV_DATA * pov;
		DWORD dwBytes;

		pov = newOV_DATA();

		pov->type = OVT_SEND;
		pov->id = pd->id;
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
				DbgPrint("TCPProxy::startTcpSend[%I64u] failed, err=%d", pd->id, err);
				
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

	bool startTcpReceive(PROXY_DATA	* pd, bool isInSocket)
	{
		DbgPrint("TCPProxy::startTcpReceive[%I64u] isInSocket=%d", pd->id, isInSocket);

		TCP_SOCKET_DATA * psd;

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
		pov->id = pd->id;
		pov->packetList.push_back(DATA_PACKET(NULL, TCP_PACKET_SIZE));

		dwFlags = 0;

		psd->receiveInProgress = true;

		if (WSARecv(psd->socket, &pov->packetList[0](), 1, &dwBytes, &dwFlags, &pov->ol, NULL) != 0)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				DbgPrint("TCPProxy::startTcpReceive[%I64u] failed, err=%d", pd->id, err);

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

	bool startUdpSend(PROXY_DATA * pd, bool isInSocket, 
		tPacketList & packetList, 
		char * remoteAddress, int remoteAddressLen)
	{
		DbgPrint("TCPProxy::startUdpSend[%I64u] isInSocket=%d", pd->id, isInSocket);

		UDP_SOCKET_DATA * psd;

		if (isInSocket)
		{
			psd = &pd->inUdpSocket;
		} else
		{
			psd = &pd->outUdpSocket;
		}
		if (packetList.size() == 0)
		{
			return false;
		}
		if (psd->socket == INVALID_SOCKET)
		{
			return false;
		}

		OV_DATA * pov;
		DWORD dwBytes;

		pov = newOV_DATA();

		pov->type = OVT_UDP_SEND;
		pov->id = pd->id;
		pov->packetList = packetList;

		packetList.clear();

		if (WSASendTo(psd->socket, 
				&pov->packetList[0](), (DWORD)pov->packetList.size(), 
				&dwBytes, 0, 
				(sockaddr*)remoteAddress, remoteAddressLen,
				&pov->ol, NULL) != 0)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				DbgPrint("TCPProxy::startUdpSend[%I64u] failed, err=%d", pd->id, err);
				deleteOV_DATA(pov);
				return false;
			}
		}

		return true;
	}

	bool startUdpReceive(PROXY_DATA * pd, bool isInSocket)
	{
		DbgPrint("TCPProxy::startUdpReceive[%I64u] isInSocket=%d", pd->id, isInSocket);

		UDP_SOCKET_DATA * psd;

		if (isInSocket)
		{
			psd = &pd->inUdpSocket;
		} else
		{
			psd = &pd->outUdpSocket;
		}

		if (psd->socket == INVALID_SOCKET)
		{
			return false;
		}

		if (psd->receiveInProgress)
		{
			return true;
		}

		if (pd->suspended && 
			pd->proxyState == PS_CONNECTED_UDP)
		{
			return true;
		}

		OV_DATA * pov;
		DWORD dwBytes, dwFlags;

		pov = newOV_DATA();
		pov->type = OVT_UDP_RECEIVE;
		pov->id = pd->id;
		pov->packetList.push_back(DATA_PACKET(NULL, UDP_PACKET_SIZE));

		dwFlags = 0;

		psd->receiveInProgress = true;

		pov->remoteAddressLen = sizeof(pov->remoteAddress);

		if (WSARecvFrom(psd->socket, &pov->packetList[0](), 1, 
			&dwBytes, &dwFlags, 
			(sockaddr*)pov->remoteAddress, &pov->remoteAddressLen,
			&pov->ol, NULL) != 0)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				DbgPrint("TCPProxy::startUdpReceive[%I64u] failed, err=%d", pd->id, err);
				deleteOV_DATA(pov);
				return false;
			}
		} 
		return true;
	}

	bool startClose(SOCKET socket, nfapi::ENDPOINT_ID id)
	{
		DbgPrint("TCPProxy::startClose[%I64u] socket %d", 
			id,
			socket);

		OV_DATA * pov = newOV_DATA();
		pov->type = OVT_CLOSE;
		pov->id = id;
		return m_service.postCompletion(socket, 0, &pov->ol);
	}

	void setKeepAliveVals(SOCKET s)
	{
		tcp_keepalive tk;
		DWORD dwRet;

		tk.onoff = 1;
		tk.keepalivetime = 5 * 60 * 1000;
		tk.keepaliveinterval = 1000;

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

		if (pd->proxyState == PS_CONNECT_SOCKS4)
		{
			SOCKS4_RESPONSE resp;
			resp.reserved = 0;
			resp.res_code = S4RC_SUCCESS;
			resp.ip = ((sockaddr_in*)pd->connInfo.remoteAddress)->sin_addr.S_un.S_addr;
			resp.port = ((sockaddr_in*)pd->connInfo.remoteAddress)->sin_port;

			startTcpSend(pd, IN_SOCKET, (char*)&resp, sizeof(resp));
		} else
		{
			if (((sockaddr*)pd->connInfo.remoteAddress)->sa_family == AF_INET)
			{
				SOCKS5_RESPONSE_IPv4 resp;
				resp.version = SOCKS_5;
				resp.res_code = 0;
				resp.reserved = 0;
				resp.address_type = SOCKS5_ADDR_IPV4;
				resp.address = ((sockaddr_in*)pd->connInfo.remoteAddress)->sin_addr.S_un.S_addr;
				resp.port = ((sockaddr_in*)pd->connInfo.remoteAddress)->sin_port;

				startTcpSend(pd, IN_SOCKET, (char*)&resp, sizeof(resp));
			} else
			{
				SOCKS5_RESPONSE_IPv6 resp;
				resp.version = SOCKS_5;
				resp.res_code = 0;
				resp.reserved = 0;
				resp.address_type = SOCKS5_ADDR_IPV6;
				memcpy(resp.address, &((sockaddr_in6*)pd->connInfo.remoteAddress)->sin6_addr, 16);
				resp.port = ((sockaddr_in6*)pd->connInfo.remoteAddress)->sin6_port;

				startTcpSend(pd, IN_SOCKET, (char*)&resp, sizeof(resp));
			}
		}

		pd->proxyState = PS_CONNECTED;

		startTcpReceive(pd, IN_SOCKET);
		startTcpReceive(pd, OUT_SOCKET);	

		if (m_pPFEventHandler)
		{
			pd->connInfo.filteringFlag |= nfapi::NF_FILTER;
			m_pPFEventHandler->tcpConnected(pov->id, &pd->connInfo);
		}
	}
	
	void onSendComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		DbgPrint("TCPProxy::onSendComplete[%I64u] socket=%d, bytes=%d", pov->id, socket, dwTransferred);

		AutoProxyData pd(this, pov->id);
		if (!pd)
			return;

		TCP_SOCKET_DATA * psd;
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
			startTcpSend(pd, isInSocket, NULL, -1);
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
				startTcpReceive(pd, !isInSocket);
			}
		}
	}

	void onReceiveComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		DbgPrint("TCPProxy::onReceiveComplete[%I64u] socket=%d, bytes=%d", pov->id, socket, dwTransferred);

		AutoProxyData pd(this, pov->id);
		if (!pd)
			return;

		TCP_SOCKET_DATA * psd;
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
				pd->connInfo.filteringFlag & nfapi::NF_FILTER)
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

		if (isInSocket == IN_SOCKET)
		{
			if (pd->proxyState == PS_CONNECTED)
			{
				if (m_pPFEventHandler && 
					pd->connInfo.filteringFlag & nfapi::NF_FILTER)
				{
					m_pPFEventHandler->tcpSend(pov->id, pov->packetList[0].buffer.buf, dwTransferred);
					
					startTcpReceive(pd, IN_SOCKET);
				} else
				{
					if (!startTcpSend(pd, OUT_SOCKET, pov->packetList[0].buffer.buf, dwTransferred))
					{
						DbgPrint("TCPProxy::onReceiveComplete startTcpSend failed");
					}
				}
			} else
			if (pd->proxyState == PS_CONNECTED_UDP)
			{
				startTcpReceive(pd, IN_SOCKET);
			} else
			{
				switch (pd->proxyState)
				{
				case PS_NONE:
					{
						if (dwTransferred < sizeof(SOCKS5_AUTH_REQUEST))
						{
							pd->proxyState = PS_ERROR;
							break;
						}

						SOCKS45_REQUEST * pr = (SOCKS45_REQUEST *)pov->packetList[0].buffer.buf;
						
						if (pr->socks5_auth_req.version != SOCKS_5)
						{
							if (pr->socks5_auth_req.version != SOCKS_4)
							{
								pd->proxyState = PS_ERROR;
							} else
							{
								if (dwTransferred < sizeof(SOCKS4_REQUEST))
								{
									pd->proxyState = PS_ERROR;
									break;
								}
								
								pd->proxyState = PS_ERROR;

								if (pr->socks4_req.command == S4C_CONNECT)
								{
									sockaddr_in * remoteAddress = (sockaddr_in*)pd->connInfo.remoteAddress;
									
									memset(remoteAddress, 0, sizeof(sockaddr_in));
									remoteAddress->sin_family = AF_INET;
									remoteAddress->sin_addr.S_un.S_addr = pr->socks4_req.ip;
									remoteAddress->sin_port = pr->socks4_req.port;

									if (m_pPFEventHandler)
									{
										pd->connInfo.filteringFlag |= nfapi::NF_FILTER;
																				
										m_pPFEventHandler->tcpConnectRequest(pd->id, &pd->connInfo);

										if (pd->connInfo.filteringFlag & nfapi::NF_BLOCK)
										{
											break;
										} else
										if (pd->connInfo.filteringFlag & nfapi::NF_OFFLINE)
										{
											pd->offline = true;
											pd->proxyState = PS_CONNECTED;
											break;
										}
									}

									for (;;)
									{
										SOCKET tcpSocket = WSASocket(((sockaddr*)pd->connInfo.remoteAddress)->sa_family, 
											SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);  
										if (tcpSocket == INVALID_SOCKET)
										{
											DbgPrint("TCPProxy::onReceiveComplete WSASocket failed");
											break;  
										}

										pd->outSocket.socket = tcpSocket;

										if (!m_service.registerSocket(tcpSocket))
										{
											DbgPrint("TCPProxy::onReceiveComplete registerSocket failed");
											break;
										}

										if (!startConnect(tcpSocket, 
												(sockaddr*)pd->connInfo.remoteAddress, 
												(((sockaddr*)pd->connInfo.remoteAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
												pd->id))
										{
											DbgPrint("TCPProxy::onReceiveComplete startConnect failed");
											break;
										}

										pd->proxyState = PS_CONNECT_SOCKS4;
										break;
									}										
								}
							}
							break;
						}

						pd->proxyState = PS_REQUEST;

						SOCK5_AUTH_RESPONSE authResp;
						authResp.version = SOCKS_5;
						authResp.method = S5AM_NONE;

						startTcpSend(pd, IN_SOCKET, (char*)&authResp, sizeof(authResp));
						startTcpReceive(pd, IN_SOCKET);
					}
					break;

				case PS_REQUEST:
					{
						if (dwTransferred < sizeof(SOCKS5_REQUEST))
						{
							pd->proxyState = PS_ERROR;
							break;
						}

						SOCKS5_REQUEST * pr = (SOCKS5_REQUEST*)pov->packetList[0].buffer.buf;
						
						if (pr->version != SOCKS_5)
						{
							pd->proxyState = PS_ERROR;
							break;
						}

						pd->proxyState = PS_ERROR;

						if (pr->address_type == SOCKS5_ADDR_IPV4)
						{
							if (dwTransferred < sizeof(SOCKS5_REQUEST_IPv4))
							{
								break;
							}

							SOCKS5_REQUEST_IPv4 * req = (SOCKS5_REQUEST_IPv4 *)pr;

							sockaddr_in * remoteAddress = (sockaddr_in*)pd->connInfo.remoteAddress;
							
							memset(remoteAddress, 0, sizeof(sockaddr_in));
							remoteAddress->sin_family = AF_INET;
							remoteAddress->sin_addr.S_un.S_addr = req->address;
							remoteAddress->sin_port = req->port;
						} else
						if (pr->address_type == SOCKS5_ADDR_IPV6)
						{
							if (dwTransferred < sizeof(SOCKS5_REQUEST_IPv6))
							{
								break;
							}
							
							SOCKS5_REQUEST_IPv6 * req = (SOCKS5_REQUEST_IPv6 *)pr;

							sockaddr_in6 * remoteAddress = (sockaddr_in6*)pd->connInfo.remoteAddress;
							
							memset(remoteAddress, 0, sizeof(sockaddr_in6));
							remoteAddress->sin6_family = AF_INET6;
							memcpy(&remoteAddress->sin6_addr, req->address, 16);
							remoteAddress->sin6_port = req->port;
						} else
						{
							DbgPrint("TCPProxy::onReceiveComplete unsupported address type %d", pr->address_type);
							break;
						}

						if (m_pPFEventHandler)
						{
							pd->connInfo.filteringFlag |= nfapi::NF_FILTER;
																	
							m_pPFEventHandler->tcpConnectRequest(pd->id, &pd->connInfo);

							if (pd->connInfo.filteringFlag & nfapi::NF_BLOCK)
							{
								break;
							} else
							if (pd->connInfo.filteringFlag & nfapi::NF_OFFLINE)
							{
								pd->offline = true;
								pd->proxyState = PS_CONNECTED;
								break;
							}
						}

						if (pr->command == S5C_CONNECT)
						{
							DbgPrint("TCPProxy::onReceiveComplete S5C_CONNECT");

							for (;;)
							{
								SOCKET tcpSocket = WSASocket(((sockaddr*)pd->connInfo.remoteAddress)->sa_family, 
									SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);  
								if (tcpSocket == INVALID_SOCKET)
								{
									DbgPrint("TCPProxy::onReceiveComplete WSASocket failed");
									break;  
								}

								pd->outSocket.socket = tcpSocket;

								if (!m_service.registerSocket(tcpSocket))
								{
									DbgPrint("TCPProxy::onReceiveComplete registerSocket failed");
									break;
								}

								if (!startConnect(tcpSocket, 
										(sockaddr*)pd->connInfo.remoteAddress, 
										(((sockaddr*)pd->connInfo.remoteAddress)->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
										pd->id))
								{
									DbgPrint("TCPProxy::onReceiveComplete startConnect failed");
									break;
								}

								pd->proxyState = PS_CONNECT;
								break;
							}
						} else
						if (pr->command == S5C_UDP_ASSOCIATE)
						{
							DbgPrint("TCPProxy::onReceiveComplete S5C_UDP_ASSOCIATE");

							for (;;)
							{
								SOCKET udpSocket = WSASocket((pr->address_type == SOCKS5_ADDR_IPV4)? AF_INET : AF_INET6, 
									SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);  
								if (udpSocket == INVALID_SOCKET)
								{
									DbgPrint("TCPProxy::onReceiveComplete S5C_UDP_ASSOCIATE WSASocket failed");
									break;  
								}

								pd->inUdpSocket.socket = udpSocket;

								if (!m_service.registerSocket(udpSocket))
								{
									DbgPrint("TCPProxy::onReceiveComplete S5C_UDP_ASSOCIATE registerSocket failed");
									break;
								}

								pd->outUdpSocket.socket = WSASocket((pr->address_type == SOCKS5_ADDR_IPV4)? AF_INET : AF_INET6, 
									SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);  
								if (pd->outUdpSocket.socket == INVALID_SOCKET)
								{
									DbgPrint("TCPProxy::onReceiveComplete S5C_UDP_ASSOCIATE WSASocket failed");
									break;  
								}

								if (!m_service.registerSocket(pd->outUdpSocket.socket))
								{
									DbgPrint("TCPProxy::onReceiveComplete S5C_UDP_ASSOCIATE registerSocket failed");
									break;
								}

								if (pr->address_type == SOCKS5_ADDR_IPV4)
								{
									sockaddr_in addr;
									memset(&addr, 0, sizeof(addr));
									addr.sin_family = AF_INET;

									if (bind(udpSocket, (SOCKADDR*)&addr, sizeof(addr)) != 0)
									{
										DbgPrint("TCPProxy::onReceiveComplete S5C_UDP_ASSOCIATE bind failed");
										break;
									}
								} else
								if (pr->address_type == SOCKS5_ADDR_IPV6)
								{
									sockaddr_in6 addr;
									memset(&addr, 0, sizeof(addr));
									addr.sin6_family = AF_INET6;

									if (bind(udpSocket, (SOCKADDR*)&addr, sizeof(addr)) != 0)
									{
										DbgPrint("TCPProxy::onReceiveComplete S5C_UDP_ASSOCIATE bind failed");
										break;
									}
								} else
								{
									break;
								}

								char localAddress[NF_MAX_ADDRESS_LENGTH];
								int localAddressLen = sizeof(localAddress);
								if (getsockname(udpSocket, (sockaddr*)&localAddress, &localAddressLen) != 0)
								{
									DbgPrint("TCPProxy::onReceiveComplete S5C_UDP_ASSOCIATE getsockname failed");
									break;
								}

								if (pr->address_type == SOCKS5_ADDR_IPV4)
								{
									SOCKS5_RESPONSE_IPv4 resp;
									resp.version = SOCKS_5;
									resp.res_code = 0;
									resp.reserved = 0;
									resp.address_type = SOCKS5_ADDR_IPV4;
									resp.address = ((sockaddr_in*)localAddress)->sin_addr.S_un.S_addr;
									resp.port = ((sockaddr_in*)localAddress)->sin_port;

									startTcpSend(pd, IN_SOCKET, (char*)&resp, sizeof(resp));
								} else
								if (pr->address_type == SOCKS5_ADDR_IPV6)
								{
									SOCKS5_RESPONSE_IPv6 resp;
									resp.version = SOCKS_5;
									resp.res_code = 0;
									resp.reserved = 0;
									resp.address_type = SOCKS5_ADDR_IPV6;
									memcpy(resp.address, &((sockaddr_in6*)localAddress)->sin6_addr, 16);
									resp.port = ((sockaddr_in6*)localAddress)->sin6_port;

									startTcpSend(pd, IN_SOCKET, (char*)&resp, sizeof(resp));
								} else
								{
									DbgPrint("TCPProxy::onReceiveComplete S5C_UDP_ASSOCIATE invalid address type");
									break;
								}

								pd->udpConnInfo.ip_family = (pr->address_type == SOCKS5_ADDR_IPV4)? AF_INET : AF_INET6;
								memcpy(pd->udpConnInfo.localAddress, pd->connInfo.localAddress, sizeof(pd->udpConnInfo.localAddress));

								if (m_pPFEventHandler)
								{
									m_pPFEventHandler->udpCreated(pov->id, &pd->udpConnInfo);
								}

								pd->proxyState = PS_CONNECTED_UDP;

								startUdpReceive(pd, IN_SOCKET);

								startTcpReceive(pd, IN_SOCKET);

								break;
							}
							break;
						}
					}
					break;
				}
				
				if (pd->proxyState == PS_ERROR)
				{
					startClose(socket, pov->id);
				}
			}
		} else
		// OUT_SOCKET
		{
			if (pd->proxyState == PS_CONNECTED)
			{
				if (m_pPFEventHandler &&
					pd->connInfo.filteringFlag & nfapi::NF_FILTER)
				{
					m_pPFEventHandler->tcpReceive(pov->id, pov->packetList[0].buffer.buf, dwTransferred);

					startTcpReceive(pd, OUT_SOCKET);
				} else
				{
					if (!startTcpSend(pd, IN_SOCKET, pov->packetList[0].buffer.buf, dwTransferred))
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

	void onUdpReceiveComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		DbgPrint("TCPProxy::onUdpReceiveComplete[%I64u] socket=%d, bytes=%d", pov->id, socket, dwTransferred);

		AutoProxyData pd(this, pov->id);
		if (!pd)
			return;

		UDP_SOCKET_DATA * psd;
		bool isInSocket;

		if (pd->inUdpSocket.socket == socket)
		{
			isInSocket = true;
			psd = &pd->inUdpSocket;
		} else
		{
			isInSocket = false;
			psd = &pd->outUdpSocket;
		}

		psd->receiveInProgress = false;

		if (dwTransferred == 0 || 
			pd->proxyState != PS_CONNECTED_UDP)
		{
			if (error == 0)
			{
				startUdpReceive(pd, isInSocket);
			} else
			{
				DbgPrint("TCPProxy::onUdpReceiveComplete[%I64u] socket=%d, error=%d", pov->id, socket, error);
			}
			return;
		}

		if (isInSocket == IN_SOCKET)
		{
			if (dwTransferred <= sizeof(SOCKS5_UDP_REQUEST_IPv4))
			{
				DbgPrint("TCPProxy::onUdpReceiveComplete[%I64u] socket=%d, invalid header", pov->id, socket);
				startUdpReceive(pd, IN_SOCKET);
				return;
			}

			memcpy(pd->inUdpSocket.remoteAddress, pov->remoteAddress, sizeof(pov->remoteAddress));
			pd->inUdpSocket.remoteAddressLen = pov->remoteAddressLen;

			SOCKS5_UDP_REQUEST * pReq = (SOCKS5_UDP_REQUEST*)pov->packetList[0].buffer.buf;

			if (pReq->address_type == SOCKS5_ADDR_IPV4)
			{
				SOCKS5_UDP_REQUEST_IPv4 * pReqIPv4 = (SOCKS5_UDP_REQUEST_IPv4*)pov->packetList[0].buffer.buf;

				sockaddr_in addr;
				memset(&addr, 0, sizeof(addr));
				addr.sin_family = AF_INET;
				addr.sin_addr.S_un.S_addr = pReqIPv4->address;
				addr.sin_port = pReqIPv4->port;

				if (m_pPFEventHandler)
				{
					m_pPFEventHandler->udpSend(pov->id, 
						(unsigned char*)&addr,
						pov->packetList[0].buffer.buf + sizeof(SOCKS5_UDP_REQUEST_IPv4), 
						dwTransferred - sizeof(SOCKS5_UDP_REQUEST_IPv4),
						NULL);
				} else
				{
					tPacketList packetList;
					
					packetList.push_back(DATA_PACKET(
						pov->packetList[0].buffer.buf + sizeof(SOCKS5_UDP_REQUEST_IPv4), 
						dwTransferred - sizeof(SOCKS5_UDP_REQUEST_IPv4)));

					startUdpSend(pd, OUT_SOCKET, 
						packetList,
						(char*)&addr, 
						sizeof(addr));
				}
			} else
			if (pReq->address_type == SOCKS5_ADDR_IPV6)
			{
				if (dwTransferred <= sizeof(SOCKS5_UDP_REQUEST_IPv6))
				{
					DbgPrint("TCPProxy::onUdpReceiveComplete[%I64u] socket=%d, invalid header", pov->id, socket);
					startUdpReceive(pd, IN_SOCKET);
					return;
				}

				SOCKS5_UDP_REQUEST_IPv6 * pReqIPv6 = (SOCKS5_UDP_REQUEST_IPv6*)pov->packetList[0].buffer.buf;
				
				sockaddr_in6 addr;
				memset(&addr, 0, sizeof(addr));
				addr.sin6_family = AF_INET6;
				memcpy(&addr.sin6_addr, pReqIPv6->address, 16);
				addr.sin6_port = pReqIPv6->port;

				if (m_pPFEventHandler)
				{
					m_pPFEventHandler->udpSend(pov->id, 
						(unsigned char*)&addr,
						pov->packetList[0].buffer.buf + sizeof(SOCKS5_UDP_REQUEST_IPv6), 
						dwTransferred - sizeof(SOCKS5_UDP_REQUEST_IPv6),
						NULL);
				} else
				{
					tPacketList packetList;
				
					packetList.push_back(DATA_PACKET(
						pov->packetList[0].buffer.buf + sizeof(SOCKS5_UDP_REQUEST_IPv6), 
						dwTransferred - sizeof(SOCKS5_UDP_REQUEST_IPv6)));

					startUdpSend(pd, OUT_SOCKET, 
						packetList,
						(char*)&addr, 
						sizeof(addr));
				}
			} else
			{
				DbgPrint("TCPProxy::onUdpReceiveComplete[%I64u] socket=%d, invalid address type %d", pov->id, socket, pReq->address_type);
			}

			startUdpReceive(pd, OUT_SOCKET);
			startUdpReceive(pd, IN_SOCKET);
		} else
		// OUT_SOCKET
		{
			if (((sockaddr*)pov->remoteAddress)->sa_family == AF_INET)
			{
				sockaddr_in * addr = (sockaddr_in*)pov->remoteAddress;
				
				SOCKS5_UDP_REQUEST_IPv4 req;

				req.reserved = 0;
				req.frag = 0;
				req.address_type = SOCKS5_ADDR_IPV4;
				req.address = addr->sin_addr.S_un.S_addr;
				req.port = addr->sin_port;

				if (m_pPFEventHandler)
				{
					m_pPFEventHandler->udpReceive(pov->id, 
						(unsigned char*)pov->remoteAddress,
						pov->packetList[0].buffer.buf, 
						dwTransferred,
						NULL);
				} else
				{
					tPacketList packetList;
					packetList.push_back(DATA_PACKET((char*)&req, sizeof(req)));
					packetList.push_back(DATA_PACKET(pov->packetList[0].buffer.buf, dwTransferred));
					
					startUdpSend(pd, IN_SOCKET, 
						packetList,
						pd->inUdpSocket.remoteAddress, 
						pd->inUdpSocket.remoteAddressLen);
				}
			} else
			{
				sockaddr_in6 * addr = (sockaddr_in6*)pov->remoteAddress;
				
				SOCKS5_UDP_REQUEST_IPv6 req;

				req.reserved = 0;
				req.frag = 0;
				req.address_type = SOCKS5_ADDR_IPV6;
				memcpy(req.address, &addr->sin6_addr, 16);
				req.port = addr->sin6_port;

				if (m_pPFEventHandler)
				{
					m_pPFEventHandler->udpReceive(pov->id, 
						(unsigned char*)pov->remoteAddress,
						pov->packetList[0].buffer.buf, 
						dwTransferred,
						NULL);
				} else
				{
					tPacketList packetList;

					packetList.push_back(DATA_PACKET((char*)&req, sizeof(req)));
					packetList.push_back(DATA_PACKET(pov->packetList[0].buffer.buf, dwTransferred));
					
					startUdpSend(pd, IN_SOCKET, 
						packetList,
						pd->inUdpSocket.remoteAddress, 
						pd->inUdpSocket.remoteAddressLen);
				}
			}

			startUdpReceive(pd, IN_SOCKET);
			startUdpReceive(pd, OUT_SOCKET);
		}
	}

	void onAcceptComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		sockaddr * pLocalAddr = NULL;
		sockaddr * pRemoteAddr = NULL;
		int localAddrLen, remoteAddrLen;
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
				socket,
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

			m_connId++;

			pd->id = m_connId;

			pd->inSocket.socket = acceptSocket;
			memcpy(pd->connInfo.localAddress, pRemoteAddr, remoteAddrLen);
			memcpy(pd->connInfo.remoteAddress, pLocalAddr, localAddrLen);
			pd->connInfo.ip_family = ipFamily;
			pd->connInfo.direction = nfapi::NF_D_OUT;
			pd->connInfo.processId = 0;
			
			m_socketMap[pd->id] = pd;

			startTcpReceive(pd, IN_SOCKET);
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

		TCP_SOCKET_DATA *psd, *psd2;
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
				startTcpReceive(pd, isInSocket2);			
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
			case OVT_UDP_RECEIVE:
				onUdpReceiveComplete(pov->socket, pov->dwTransferred, pov, pov->error);
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

	typedef std::map<nfapi::ENDPOINT_ID, PROXY_DATA*> tSocketMap;
	tSocketMap m_socketMap;

	LPFN_ACCEPTEX m_pAcceptEx;
	LPFN_CONNECTEX m_pConnectEx;
	LPFN_GETACCEPTEXSOCKADDRS m_pGetAcceptExSockaddrs;

	SOCKET m_listenSocket;
	SOCKET m_acceptSocket;

	SOCKET m_listenSocket_IPv6;
	SOCKET m_acceptSocket_IPv6;

	nfapi::ENDPOINT_ID m_connId;

	unsigned short m_port;

	bool	m_ipv4Available;
	bool	m_ipv6Available;

	nfapi::NF_EventHandler * m_pPFEventHandler;

	LIST_ENTRY	m_eventList;
	AutoCriticalSection m_csEventList;
	
	ThreadPool m_pool;

	AutoCriticalSection m_cs;
};

}