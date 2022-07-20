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

#include <map>
#include "ProxySession.h"
#include "Settings.h"
#include "sync.h"

namespace ProtocolFilters
{
	class PFEvents;

	class Proxy : public NFAPI_NS NF_EventHandler
	{
	public:
		Proxy(void);
		virtual ~Proxy(void);

		void free();

		void threadStart();
		void threadEnd();
		void tcpConnectRequest(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_TCP_CONN_INFO pConnInfo);
		void tcpConnected(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_TCP_CONN_INFO pConnInfo);
		void tcpClosed(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_TCP_CONN_INFO pConnInfo);
		void tcpReceive(NFAPI_NS ENDPOINT_ID id, const char * buf, int len);
		void tcpSend(NFAPI_NS ENDPOINT_ID id, const char * buf, int len);
		void tcpCanReceive(NFAPI_NS ENDPOINT_ID id);
		void tcpCanSend(NFAPI_NS ENDPOINT_ID id);
		void udpCreated(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_UDP_CONN_INFO pConnInfo);
		void udpConnectRequest(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_UDP_CONN_REQUEST pConnReq);
		void udpClosed(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_UDP_CONN_INFO pConnInfo);
		void udpReceive(NFAPI_NS ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, NFAPI_NS PNF_UDP_OPTIONS options);
		void udpSend(NFAPI_NS ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, NFAPI_NS PNF_UDP_OPTIONS options);
		void udpCanReceive(NFAPI_NS ENDPOINT_ID id);
		void udpCanSend(NFAPI_NS ENDPOINT_ID id);
		void setParsersEventHandler(PFEvents * pHandler);
		PFEvents * getParsersEventHandler();

		bool postObject(NFAPI_NS ENDPOINT_ID id, PFObject * pObject);
		bool tcpDisconnect(NFAPI_NS ENDPOINT_ID id, PF_ObjectType dt);

		bool addFilter(NFAPI_NS ENDPOINT_ID id,
						PF_FilterType type, 
						tPF_FilterFlags flags = FF_DEFAULT,
						PF_OpTarget target = OT_LAST, 
						PF_FilterType typeBase = FT_NONE);
			
		bool deleteFilter(NFAPI_NS ENDPOINT_ID id, PF_FilterType type);

		int getFilterCount(NFAPI_NS ENDPOINT_ID id);
		bool isFilterActive(NFAPI_NS ENDPOINT_ID id, PF_FilterType type);
		bool canDisableFiltering(NFAPI_NS ENDPOINT_ID id);

		ProxySession * findAssociatedSession(const std::string & sAddr, bool isRemote);

		void setSessionState(NFAPI_NS ENDPOINT_ID id, bool suspend);

	protected:
		ProxySession * createSession(NFAPI_NS ENDPOINT_ID id, int protocol, GEN_SESSION_INFO * pSessionInfo);
		bool deleteSession(NFAPI_NS ENDPOINT_ID id);
		bool deleteAllSessions();
		ProxySession * findSession(NFAPI_NS ENDPOINT_ID id);

	private:
		typedef std::map<NFAPI_NS ENDPOINT_ID, ProxySession*>	tSessions;
		tSessions m_sessions;
		PFEvents * m_pParsersEventHandler;
		AutoCriticalSection m_cs;
	};
}