//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _IFILTER_BASE
#define _IFILTER_BASE

#include "PFFilterDefs.h"
#include "nfevents.h"
#include "PFEvents.h"
#include "PFObjectImpl.h"

namespace ProtocolFilters
{
	#define FILTER_UNKNOWN -1

	enum eDataDirection
	{
		DD_INPUT,
		DD_OUTPUT
	};

	enum ePacketDirection
	{
		PD_RECEIVE,
		PD_SEND
	};

	enum eFilterResult
	{
		FR_DELETE_ME,
		FR_DELETE_OTHERS,
		FR_PASSTHROUGH,
		FR_DATA_EATEN,
		FR_ERROR
	};

	struct GEN_SESSION_INFO
	{
		union
		{
			NFAPI_NS NF_TCP_CONN_INFO tcpConnInfo;
			NFAPI_NS NF_UDP_CONN_INFO udpConnInfo;
		};
	};

	/**
	 *	The interface for filter objects
	 */
	class IFilterBase
	{
	public:
		IFilterBase() : m_deleted(false), m_active(true)
		{
		}
		virtual ~IFilterBase() 
		{
		}
		
		virtual eFilterResult tcp_packet(eDataDirection dd,
								ePacketDirection pd,
								const char * buf, int len) = 0;
		
		virtual eFilterResult udp_packet(eDataDirection dd,
								ePacketDirection pd,
								const unsigned char * remoteAddress, 
								const char * buf, int len) = 0;
		
		virtual void tcp_canPost(ePacketDirection pd)
		{
		}

		virtual void udp_canPost(ePacketDirection pd)
		{
		}

		virtual PF_FilterType getType() = 0;

		virtual PF_FilterCategory getCategory() = 0;

		virtual bool postObject(PFObjectImpl & object) = 0;

		virtual PF_FilterFlags getFlags() = 0;

		bool isDeleted()
		{
			return m_deleted;
		}

		void setDeleted()
		{
			m_deleted = true;
			m_active = false;
		}
		
		bool isActive()
		{
			return m_active;
		}

		void setActive(bool value)
		{
			m_active = value;
		}

	private:
		bool m_deleted;
		bool m_active;
	};

	class ProxySession;

}
#endif // _IFILTER_BASE