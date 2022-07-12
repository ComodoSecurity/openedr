#pragma once

#include <set>
#include <map>
#include <list>
#include <mswsock.h>
#include "sync.h"
#include "socksdefs.h"
#include "iocp.h"

namespace UdpProxy
{

#define PACKET_SIZE 65536

typedef std::vector<char> tBuffer;

enum OV_TYPE
{
	OVT_CONNECT,
	OVT_TCP_SEND,
	OVT_TCP_RECEIVE,
	OVT_UDP_SEND,
	OVT_UDP_RECEIVE
};

struct OV_DATA
{
	OV_DATA()
	{
		memset(&ol, 0, sizeof(ol));
	}

	OVERLAPPED	ol;
	unsigned __int64 id;
	OV_TYPE		type;
	char		remoteAddress[NF_MAX_ADDRESS_LENGTH];
	int			remoteAddressLen;
	tBuffer		buffer;
};

typedef std::set<OV_DATA*> tOvDataSet;

enum PROXY_STATE
{
	PS_NONE,
	PS_AUTH,
	PS_AUTH_NEGOTIATION,
	PS_UDP_ASSOC,
	PS_CONNECTED,
	PS_CLOSED
};

struct UDP_PACKET
{
	char		remoteAddress[NF_MAX_ADDRESS_LENGTH];
	int			remoteAddressLen;
	tBuffer		buffer;
};

typedef std::list<UDP_PACKET*> tPacketList;

struct PROXY_DATA
{
	PROXY_DATA()
	{
		tcpSocket = INVALID_SOCKET;
		udpSocket = INVALID_SOCKET;
		proxyState = PS_NONE;
		memset(remoteAddress, 0, sizeof(remoteAddress));
		remoteAddressLen = 0;
		udpRecvStarted = false;
	}
	~PROXY_DATA()
	{
		if (tcpSocket != INVALID_SOCKET)
		{
			shutdown(tcpSocket, SD_BOTH);
			closesocket(tcpSocket);
		}
		if (udpSocket != INVALID_SOCKET)
		{
			closesocket(udpSocket);
		}
		while (!udpSendPackets.empty())
		{
			tPacketList::iterator it = udpSendPackets.begin();
			delete (*it);
			udpSendPackets.erase(it);
		}
	}

	SOCKET	tcpSocket;
	SOCKET	udpSocket;
	
	PROXY_STATE	proxyState;

	char		remoteAddress[NF_MAX_ADDRESS_LENGTH];
	int			remoteAddressLen;

	std::string		userName;
	std::string		userPassword;

	tPacketList udpSendPackets;

	bool	udpRecvStarted;
};

typedef std::map<unsigned __int64, PROXY_DATA*> tSocketMap;

class UDPProxyHandler
{
public:
	virtual void onUdpReceiveComplete(unsigned __int64 id, char * buf, int len, char * remoteAddress, int remoteAddressLen) = 0;
};


class UDPProxy : public IOCPHandler
{
public:
	UDPProxy() : m_pProxyHandler(NULL)
	{
	}

	~UDPProxy()
	{
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
		const GUID connectex = WSAID_CONNECTEX;

		SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == INVALID_SOCKET)
			return false;

		m_pConnectEx = (LPFN_CONNECTEX)getExtensionFunction(s, &connectex);
		
		closesocket(s);	

		return m_pConnectEx != NULL;
	}

	bool init(UDPProxyHandler * pProxyHandler, 
		char * proxyAddress, int proxyAddressLen,
		const char * userName = NULL, const char * userPassword = NULL)
	{
		if (!initExtensions())
			return false;

		if (!m_service.init(this))
			return false;

		m_pProxyHandler = pProxyHandler;

		memcpy(m_proxyAddress, proxyAddress, proxyAddressLen);
		m_proxyAddressLen = proxyAddressLen;

		if (userName)
		{
			m_userName = userName;
		}
		if (userPassword)
		{
			m_userPassword = userPassword;
		}

		return true;
	}

	void free()
	{
		m_service.free();

		while (!m_ovDataSet.empty())
		{
			tOvDataSet::iterator it = m_ovDataSet.begin();
			delete (*it);
			m_ovDataSet.erase(it);
		}
		while (!m_socketMap.empty())
		{
			tSocketMap::iterator it = m_socketMap.begin();
			delete it->second;
			m_socketMap.erase(it);
		}
	}

	bool createProxyConnection(unsigned __int64 id)
	{
		bool result = false;

		DbgPrint("UDPProxy::createProxyConnection %I64u", id);

		for (;;)
		{
			SOCKET tcpSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);  
			if (tcpSocket == INVALID_SOCKET)
				return false;  

			SOCKET udpSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);  
			if (udpSocket == INVALID_SOCKET)
			{
				closesocket(tcpSocket);
				return false;  
			}

			{
				AutoLock lock(m_cs);

				PROXY_DATA * pd = new PROXY_DATA();
				pd->tcpSocket = tcpSocket;
				pd->udpSocket = udpSocket;
				
				pd->userName = m_userName;
				pd->userPassword = m_userPassword;

				m_socketMap[id] = pd;
			}

			if (!m_service.registerSocket(tcpSocket))
				break;

			if (!m_service.registerSocket(udpSocket))
				break;

			if (!startConnect(tcpSocket, (sockaddr*)m_proxyAddress, m_proxyAddressLen, id))
				break;

			result = true;

			break;
		}

		if (!result)
		{
			{
				AutoLock lock(m_cs);

				tSocketMap::iterator it = m_socketMap.find(id);
				if (it != m_socketMap.end())
				{
					delete it->second;
					m_socketMap.erase(it);
				}
			}
		}

		return result;
	}

	void deleteProxyConnection(unsigned __int64 id)
	{
		DbgPrint("UDPProxy::deleteProxyConnection %I64u", id);

		AutoLock lock(m_cs);
		tSocketMap::iterator it = m_socketMap.find(id);
		if (it != m_socketMap.end())
		{
			delete it->second;
			m_socketMap.erase(it);
		}
	}

	bool udpSend(unsigned __int64 id, char * buf, int len, char * remoteAddress, int remoteAddressLen)
	{
		SOCKET s;

//		DbgPrint("udpSend %I64u", id);

		{
			AutoLock lock(m_cs);
			
			tSocketMap::iterator it = m_socketMap.find(id);
			if (it == m_socketMap.end())
			{
				return false;
			}

			if (it->second->proxyState != PS_CONNECTED)
			{
				if (len > 0)
				{
					UDP_PACKET * p = new UDP_PACKET();

					memcpy(p->remoteAddress, remoteAddress, remoteAddressLen);
					p->remoteAddressLen = remoteAddressLen;
					p->buffer.resize(len);
					memcpy(&p->buffer[0], buf, len);

					it->second->udpSendPackets.push_back(p);

//					DbgPrint("udpSend %I64u packet pended", id);
				}
				return true;
			}

			s = it->second->udpSocket;

			OV_DATA * pov = newOV_DATA();
			DWORD dwBytes;

			pov->type = OVT_UDP_SEND;
			pov->id = id;

			if (len > 0)
			{
				if (((sockaddr*)remoteAddress)->sa_family == AF_INET)
				{
					pov->buffer.resize(len + sizeof(SOCKS5_UDP_REQUEST_IPv4));
					
					SOCKS5_UDP_REQUEST_IPv4 * pReq = (SOCKS5_UDP_REQUEST_IPv4*)&pov->buffer[0];
					pReq->reserved = 0;
					pReq->frag = 0;
					pReq->address_type = SOCKS5_ADDR_IPV4;
					pReq->address = ((sockaddr_in*)remoteAddress)->sin_addr.S_un.S_addr;
					pReq->port = ((sockaddr_in*)remoteAddress)->sin_port;

					memcpy(&pov->buffer[sizeof(SOCKS5_UDP_REQUEST_IPv4)], buf, len);

				} else
				{
					pov->buffer.resize(len + sizeof(SOCKS5_UDP_REQUEST_IPv6));

					SOCKS5_UDP_REQUEST_IPv6 * pReq = (SOCKS5_UDP_REQUEST_IPv6*)&pov->buffer[0];
					pReq->reserved = 0;
					pReq->frag = 0;
					pReq->address_type = SOCKS5_ADDR_IPV6;
					memcpy(pReq->address, &((sockaddr_in6*)remoteAddress)->sin6_addr, 16);
					pReq->port = ((sockaddr_in6*)remoteAddress)->sin6_port;

					memcpy(&pov->buffer[sizeof(SOCKS5_UDP_REQUEST_IPv6)], buf, len);
				}

			}

			WSABUF bufs;

			bufs.buf = &pov->buffer[0];
			bufs.len = (u_long)pov->buffer.size();

			if (WSASendTo(s, &bufs, 1, &dwBytes, 0, 
				(sockaddr*)it->second->remoteAddress, it->second->remoteAddressLen, 
				&pov->ol, NULL) != 0)
			{
				int err = WSAGetLastError();
				if (err != ERROR_IO_PENDING)
				{
					pov->type = OVT_UDP_RECEIVE;
					pov->buffer.clear();
					if (!m_service.postCompletion(s, 0, &pov->ol))
					{
						deleteOV_DATA(pov);
					}
					return false;
				}
			}

			if (!it->second->udpRecvStarted)
			{
				it->second->udpRecvStarted = true;
				startUdpReceive(s, id, NULL);
			}
		}

		return true;
	}

	OV_DATA * newOV_DATA()
	{
		OV_DATA * pov = new OV_DATA();
		AutoLock lock(m_cs);
		m_ovDataSet.insert(pov);
		return pov;
	}

	void deleteOV_DATA(OV_DATA * pov)
	{
		AutoLock lock(m_cs);
		tOvDataSet::iterator it;
		it = m_ovDataSet.find(pov);
		if (it == m_ovDataSet.end())
			return;
		m_ovDataSet.erase(it);
		delete pov;
	}

	bool startConnect(SOCKET socket, sockaddr * pAddr, int addrLen, unsigned __int64 id)
	{
		OV_DATA * pov = newOV_DATA();
		pov->type = OVT_CONNECT;
		pov->id = id;

		DbgPrint("UDPProxy::startConnect %I64u, socket=%d", id, socket);

		{
			struct sockaddr_in addr;
			ZeroMemory(&addr, sizeof(addr));
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			addr.sin_port = 0;
			
			bind(socket, (SOCKADDR*) &addr, sizeof(addr));
		}		

		if (!m_pConnectEx(socket, pAddr, addrLen, NULL, 0, NULL, &pov->ol))
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				DbgPrint("UDPProxy::startConnect %I64u failed, err=%d", id, err);
				deleteOV_DATA(pov);
				return false;
			}
		} 

		return true;
	}


	bool startUdpReceive(SOCKET socket, unsigned __int64 id, OV_DATA * pov)
	{
		DWORD dwBytes, dwFlags;
		WSABUF bufs;

		if (pov == NULL)
		{
			pov = newOV_DATA();
			pov->type = OVT_UDP_RECEIVE;
			pov->id = id;
			pov->buffer.resize(PACKET_SIZE);
		}

		bufs.buf = &pov->buffer[0];
		bufs.len = (u_long)pov->buffer.size();

		dwFlags = 0;

		pov->remoteAddressLen = sizeof(pov->remoteAddress);

		if (WSARecvFrom(socket, &bufs, 1, &dwBytes, &dwFlags, (sockaddr*)pov->remoteAddress, &pov->remoteAddressLen, &pov->ol, NULL) != 0)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				if (!m_service.postCompletion(socket, 0, &pov->ol))
				{
					deleteOV_DATA(pov);
				}
				return true;
			}
		} 
	
		return true;
	}

	bool startTcpReceive(SOCKET socket, unsigned __int64 id, OV_DATA * pov)
	{
		DWORD dwBytes, dwFlags;
		WSABUF bufs;

		if (pov == NULL)
		{
			pov = newOV_DATA();
			pov->type = OVT_TCP_RECEIVE;
			pov->id = id;
			pov->buffer.resize(PACKET_SIZE);
		}

		bufs.buf = &pov->buffer[0];
		bufs.len = (u_long)pov->buffer.size();

		dwFlags = 0;

		if (WSARecv(socket, &bufs, 1, &dwBytes, &dwFlags, &pov->ol, NULL) != 0)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				if (!m_service.postCompletion(socket, 0, &pov->ol))
				{
					deleteOV_DATA(pov);
				}
				return true;
			}
		} 
	
		return true;
	}

	bool startTcpSend(SOCKET socket, char * buf, int len, unsigned __int64 id)
	{
		OV_DATA * pov = newOV_DATA();
		DWORD dwBytes;

		DbgPrint("UDPProxy::startTcpSend %I64u bytes=%d", id, len);

		pov->id = id;
		pov->type = OVT_TCP_SEND;

		if (len > 0)
		{
			pov->buffer.resize(len);
			memcpy(&pov->buffer[0], buf, len);
		}

		WSABUF bufs;

		bufs.buf = &pov->buffer[0];
		bufs.len = (u_long)pov->buffer.size();

		if (WSASend(socket, &bufs, 1, &dwBytes, 0, 
			&pov->ol, NULL) != 0)
		{
			int err = WSAGetLastError();
			if (err != ERROR_IO_PENDING)
			{
				DbgPrint("UDPProxy::startTcpSend %I64u failed, err=%d", id, err);
				pov->type = OVT_TCP_RECEIVE;
				pov->buffer.clear();
				if (!m_service.postCompletion(socket, 0, &pov->ol))
				{
					deleteOV_DATA(pov);
				}
				return false;
			}
		} 
	
		return true;
	}

	void onUdpSendComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
//		DbgPrint("UDPProxy::onUdpSendComplete %I64u bytes=%d, err=%d", pov->id, dwTransferred, error);
		deleteOV_DATA(pov);
	}

	void onUdpReceiveComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
//		DbgPrint("UDPProxy::onUdpReceiveComplete %I64u bytes=%d, err=%d", pov->id, dwTransferred, error);

		if (dwTransferred == 0)
		{
			deleteOV_DATA(pov);
			return;
		}

		if (dwTransferred > sizeof(SOCKS5_UDP_REQUEST))
		{
			SOCKS5_UDP_REQUEST * pReq = (SOCKS5_UDP_REQUEST*)&pov->buffer[0];
			if (pReq->address_type == SOCKS5_ADDR_IPV4)
			{
				SOCKS5_UDP_REQUEST_IPv4 * pReqIPv4 = (SOCKS5_UDP_REQUEST_IPv4*)&pov->buffer[0];
				
				sockaddr_in addr;
				memset(&addr, 0, sizeof(addr));
				addr.sin_family = AF_INET;
				addr.sin_addr.S_un.S_addr = pReqIPv4->address;
				addr.sin_port = pReqIPv4->port;

				m_pProxyHandler->onUdpReceiveComplete(pov->id, 
					&pov->buffer[sizeof(SOCKS5_UDP_REQUEST_IPv4)], 
					dwTransferred - sizeof(SOCKS5_UDP_REQUEST_IPv4), 
					(char*)&addr, 
					sizeof(addr));
			} else
			if (pReq->address_type == SOCKS5_ADDR_IPV6)
			{
				SOCKS5_UDP_REQUEST_IPv6 * pReqIPv6 = (SOCKS5_UDP_REQUEST_IPv6*)&pov->buffer[0];
				
				sockaddr_in6 addr;
				memset(&addr, 0, sizeof(addr));
				addr.sin6_family = AF_INET6;
				memcpy(&addr.sin6_addr, pReqIPv6->address, 16);
				addr.sin6_port = pReqIPv6->port;

				m_pProxyHandler->onUdpReceiveComplete(pov->id, 
					&pov->buffer[sizeof(SOCKS5_UDP_REQUEST_IPv6)], 
					dwTransferred - sizeof(SOCKS5_UDP_REQUEST_IPv6), 
					(char*)&addr, 
					sizeof(addr));
			}
		}

		memset(&pov->ol, 0, sizeof(pov->ol));
		
		startUdpReceive(socket, pov->id, pov);
	}

	void onConnectComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		if (error != 0)
		{
			DbgPrint("UDPProxy::onConnectComplete %I64u failed, err=%d", pov->id, error);
			deleteProxyConnection(pov->id);
			deleteOV_DATA(pov);
			return;
		}

		{
			AutoLock lock(m_cs);
			
			tSocketMap::iterator it = m_socketMap.find(pov->id);
			if (it != m_socketMap.end())
			{
				BOOL val = 1;
				setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char*)&val, sizeof(val));
				setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&val, sizeof(val));

				SOCKS5_AUTH_REQUEST authReq;

				authReq.version = SOCKS_5;
				authReq.nmethods = 1;

				if (!it->second->userName.empty())
				{
					authReq.methods[0] = S5AM_UNPW;
				} else
				{
					authReq.methods[0] = S5AM_NONE;
				}

				if (startTcpSend(it->second->tcpSocket, (char*)&authReq, sizeof(authReq), pov->id))
				{
					it->second->proxyState = PS_AUTH;
				}
				
				startTcpReceive(it->second->tcpSocket, pov->id, NULL);			
			}
		}

		deleteOV_DATA(pov);
	}
	
	void onTcpSendComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
//		DbgPrint("UDPProxy::onTcpSendComplete %I64u bytes=%d, err=%d", pov->id, dwTransferred, error);
		deleteOV_DATA(pov);
	}

	void onTcpReceiveComplete(SOCKET socket, DWORD dwTransferred, OV_DATA * pov, int error)
	{
		DbgPrint("UDPProxy::onTcpReceiveComplete %I64u bytes=%d, err=%d", pov->id, dwTransferred, error);

		if (dwTransferred == 0)
		{
			deleteOV_DATA(pov);
			return;
		}

		{
			AutoLock lock(m_cs);
			
			tSocketMap::iterator it = m_socketMap.find(pov->id);
			if (it != m_socketMap.end())
			{
				switch (it->second->proxyState)
				{
				case PS_NONE:
					break;

				case PS_AUTH:
					{
						if (dwTransferred < sizeof(SOCK5_AUTH_RESPONSE))
							break;

						SOCK5_AUTH_RESPONSE * pr = (SOCK5_AUTH_RESPONSE *)&pov->buffer[0];
						
						if (pr->version != SOCKS_5)
						{
							break;
						}

						if (pr->method == S5AM_UNPW && !it->second->userName.empty())
						{
							std::vector<char> authReq;

							authReq.push_back(1);
							authReq.push_back((char)it->second->userName.length());
							authReq.insert(authReq.end(), it->second->userName.begin(), it->second->userName.end());
							authReq.push_back((char)it->second->userPassword.length());
							
							if (!it->second->userPassword.empty())
								authReq.insert(authReq.end(), it->second->userPassword.begin(), it->second->userPassword.end());

							if (startTcpSend(it->second->tcpSocket, (char*)&authReq[0], (int)authReq.size(), pov->id))
							{
								it->second->proxyState = PS_AUTH_NEGOTIATION;
							}

							break;
						}

						SOCKS5_REQUEST_IPv4 req;

						req.version = SOCKS_5;
						req.command = S5C_UDP_ASSOCIATE;
						req.reserved = 0;
						req.address_type = SOCKS5_ADDR_IPV4;
						req.address = 0;
						req.port = 0;

						if (startTcpSend(it->second->tcpSocket, (char*)&req, sizeof(req), pov->id))
						{
							it->second->proxyState = PS_UDP_ASSOC;
						}		

					}
					break;

				case PS_AUTH_NEGOTIATION:
					{
						if (dwTransferred < sizeof(SOCK5_AUTH_RESPONSE))
							break;

						SOCK5_AUTH_RESPONSE * pr = (SOCK5_AUTH_RESPONSE *)&pov->buffer[0];
						
						if (pr->version != 0x01 || pr->method != 0x00)
						{
							break;
						}

						SOCKS5_REQUEST_IPv4 req;

						req.version = SOCKS_5;
						req.command = S5C_UDP_ASSOCIATE;
						req.reserved = 0;
						req.address_type = SOCKS5_ADDR_IPV4;
						req.address = 0;
						req.port = 0;

						if (startTcpSend(it->second->tcpSocket, (char*)&req, sizeof(req), pov->id))
						{
							it->second->proxyState = PS_UDP_ASSOC;
						}		

					}
					break;

				case PS_UDP_ASSOC:
					{
						if (dwTransferred < sizeof(SOCKS5_RESPONSE))
							break;

						SOCKS5_RESPONSE * pr = (SOCKS5_RESPONSE *)&pov->buffer[0];
						
						if (pr->version != SOCKS_5 || pr->res_code != 0)
							break;
						
						if (pr->address_type == SOCKS5_ADDR_IPV4)
						{
							SOCKS5_RESPONSE_IPv4 * prIPv4 = (SOCKS5_RESPONSE_IPv4 *)&pov->buffer[0];
							sockaddr_in addr;
							memset(&addr, 0, sizeof(addr));
							addr.sin_family = AF_INET;
							addr.sin_addr.S_un.S_addr = ((sockaddr_in*)m_proxyAddress)->sin_addr.S_un.S_addr;
							addr.sin_port = prIPv4->port;

							memcpy(it->second->remoteAddress, &addr, sizeof(addr));
							it->second->remoteAddressLen = sizeof(addr);
						} else
						if (pr->address_type == SOCKS5_ADDR_IPV6)
						{
							SOCKS5_RESPONSE_IPv6 * prIPv6 = (SOCKS5_RESPONSE_IPv6 *)&pov->buffer[0];
							sockaddr_in6 addr;
							memset(&addr, 0, sizeof(addr));
							addr.sin6_family = AF_INET6;
							memcpy(&addr.sin6_addr, &((sockaddr_in6*)m_proxyAddress)->sin6_addr, 16);
							addr.sin6_port = prIPv6->port;

							memcpy(it->second->remoteAddress, &addr, sizeof(addr));
							it->second->remoteAddressLen = sizeof(addr);
						} else
						{
							break;
						}

						it->second->proxyState = PS_CONNECTED;

						while (!it->second->udpSendPackets.empty())
						{
							tPacketList::iterator itp = it->second->udpSendPackets.begin();
							
							udpSend(pov->id, &(*itp)->buffer[0], (int)(*itp)->buffer.size(), (*itp)->remoteAddress, (*itp)->remoteAddressLen);
							
							delete (*itp);
							it->second->udpSendPackets.erase(itp);
						}
					}
					break;
				}

			}
		}

		memset(&pov->ol, 0, sizeof(pov->ol));
		
		startTcpReceive(socket, pov->id, pov);
	}


	virtual void onComplete(SOCKET socket, DWORD dwTransferred, OVERLAPPED * pOverlapped, int error)
	{
		OV_DATA * pov = (OV_DATA*)pOverlapped;

		switch (pov->type)
		{
		case OVT_UDP_SEND:
			onUdpSendComplete(socket, dwTransferred, pov, error);
			break;
		case OVT_UDP_RECEIVE:
			onUdpReceiveComplete(socket, dwTransferred, pov, error);
			break;
		case OVT_CONNECT:
			onConnectComplete(socket, dwTransferred, pov, error);
			break;
		case OVT_TCP_SEND:
			onTcpSendComplete(socket, dwTransferred, pov, error);
			break;
		case OVT_TCP_RECEIVE:
			onTcpReceiveComplete(socket, dwTransferred, pov, error);
			break;
		}
	}

private:
	IOCPService m_service;
	UDPProxyHandler * m_pProxyHandler;

	tOvDataSet m_ovDataSet;
	tSocketMap m_socketMap;

	LPFN_CONNECTEX m_pConnectEx;

	char		m_proxyAddress[NF_MAX_ADDRESS_LENGTH];
	int			m_proxyAddressLen;

	std::string m_userName;
	std::string m_userPassword;

	AutoCriticalSection m_cs;
};

}