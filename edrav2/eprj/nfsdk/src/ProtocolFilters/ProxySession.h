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

#include <vector>
#include "IFilterBase.h"
#include "MFStream.h"

namespace ProtocolFilters
{
	class Proxy;

	class ProxySession
	{
	public:
		ProxySession(Proxy & proxy);
		virtual ~ProxySession(void);

		unsigned long addRef()
		{
			return InterlockedIncrement(&m_refCounter); 
		}

		unsigned long release()
		{
			long c = InterlockedDecrement(&m_refCounter);
			if (c <= 0)
			{
				delete this;
			}
			return c;
		}

		bool init(NFAPI_NS ENDPOINT_ID id, int protocol, GEN_SESSION_INFO * pSessionInfo, int infoLen);
		void free();

		NFAPI_NS ENDPOINT_ID getId();
		
		int getProtocol();

		PFEvents * getParsersEventHandler();

		void getSessionInfo(GEN_SESSION_INFO * pSessionInfo);

		// IProxySession
		eFilterResult tcp_packet(IFilterBase * pSourceFilter,
						eDataDirection dd,
						ePacketDirection pd,
						const char * buffer, int len);
		
		eFilterResult udp_packet(IFilterBase * pSourceFilter,
						eDataDirection dd,
						ePacketDirection pd,
						const unsigned char * remoteAddress, 
						const char * buffer, int len);

		void tcp_canPost(ePacketDirection pd);

		void udp_canPost(ePacketDirection pd);

		IFilterBase * getFilter(PF_FilterType type);

		bool addFilter(PF_FilterType type, 
						tPF_FilterFlags flags = FF_DEFAULT,
						PF_OpTarget target = OT_LAST, 
						PF_FilterType typeBase = FT_NONE);
			
		bool deleteFilter(PF_FilterType type);

		int getFilterCount();

		bool canDisableFiltering();

		bool isFilterActive(PF_FilterType type);

		bool postObject(PFObjectImpl & object);

		void tcpDisconnect(PF_ObjectType dt);

		bool tcpPostObjectStreams(IFilterBase * pSourceFilter, ePacketDirection pd, PFObjectImpl & object);
		bool tcpPostStream(IFilterBase * pSourceFilter, ePacketDirection pd, PFStream * pStream);

		std::string getRemoteEndpointStr();
		void setRemoteEndpointStr(const std::string & s);

		std::string getLocalEndpointStr();
		void setLocalEndpointStr(const std::string & s);

		std::string getAssociatedRemoteEndpointStr();
		void setAssociatedRemoteEndpointStr(const std::string & s);

		std::string getAssociatedLocalEndpointStr();
		void setAssociatedLocalEndpointStr(const std::string & s);

		PF_FilterType getAssociatedFilterType();
		void setAssociatedFilterType(PF_FilterType t);

		std::string getAssociatedSessionInfo();
		void setAssociatedSessionInfo(const std::string & s);

		PF_FilterFlags getAssociatedFilterFlags();
		void setAssociatedFilterFlags(PF_FilterFlags f);

		bool isFirstPacket();
		void setFirstPacket(bool v);

		bool deleteProtocolFilters(IFilterBase * exceptThisOne);

		bool isDeletion();

		bool isSuspended();
		void setSessionState(bool suspend);

	protected:
		bool tcp_postData(ePacketDirection pd, const char * buf, int len);
		IFilterBase * createFilter(PF_FilterType type, tPF_FilterFlags flags);
		int findFilter(PF_FilterType type);
		int findFilter(IFilterBase * pFilter);
		bool findAssociatedSession();

	private:
		NFAPI_NS ENDPOINT_ID	m_id;
		GEN_SESSION_INFO	m_sessionInfo;
		int					m_protocol;
		std::string			m_localEndpointStr;
		std::string			m_remoteEndpointStr;
		std::string			m_associatedLocalEndpointStr;
		std::string			m_associatedRemoteEndpointStr;
		PF_FilterType		m_associatedFilterType;
		std::string			m_associatedSessionInfo;
		PF_FilterFlags		m_flags;
		bool				m_firstPacket;

		typedef std::vector<IFilterBase*> tFilters;
		tFilters		m_filters;	

		MFStream		m_tcpSendsStream;
		MFStream		m_tcpReceivesStream;

		int				m_sendPosted;
		int				m_receivePosted;

		bool			m_disconnectLocal;
		bool			m_disconnectRemote;
		bool			m_disconnectRemotePending;

		bool			m_deletion;
		bool			m_suspended;

		Proxy & m_proxy;

		volatile long m_refCounter;
		AutoCriticalSection m_cs;
	};

}