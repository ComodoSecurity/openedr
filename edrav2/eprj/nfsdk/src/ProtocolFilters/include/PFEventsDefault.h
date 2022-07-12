//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_PFEVENTS_DEF)
#define _PFEVENTS_DEF

#ifdef WIN32
#include "nfapi.h"
#endif

#include "PFEvents.h"

namespace ProtocolFilters
{
	class PFEventsDefault : public PFEvents
	{
	public:
		virtual void threadStart()
		{
		}

		virtual void threadEnd()
		{
		}

		virtual void tcpConnectRequest(nfapi::ENDPOINT_ID id, nfapi::PNF_TCP_CONN_INFO pConnInfo)
		{
		}

		virtual void tcpConnected(nfapi::ENDPOINT_ID id, nfapi::PNF_TCP_CONN_INFO pConnInfo) = 0;
		
		virtual void tcpClosed(nfapi::ENDPOINT_ID id, nfapi::PNF_TCP_CONN_INFO pConnInfo)
		{
		}

		virtual void udpCreated(nfapi::ENDPOINT_ID id, nfapi::PNF_UDP_CONN_INFO pConnInfo)
		{
		}
		
		virtual void udpConnectRequest(nfapi::ENDPOINT_ID id, nfapi::PNF_UDP_CONN_REQUEST pConnReq)
		{
		}

		virtual void udpClosed(nfapi::ENDPOINT_ID id, nfapi::PNF_UDP_CONN_INFO pConnInfo)
		{
		}

		virtual void dataAvailable(nfapi::ENDPOINT_ID id, PFObject * pObject) = 0;
		
		virtual PF_DATA_PART_CHECK_RESULT dataPartAvailable(nfapi::ENDPOINT_ID id, PFObject * pObject)
		{
			return DPCR_FILTER;
		}

		virtual NF_STATUS tcpPostSend(nfapi::ENDPOINT_ID id, const char * buf, int len)
		{
			return nfapi::nf_tcpPostSend(id, buf, len);
		}

		virtual NF_STATUS tcpPostReceive(nfapi::ENDPOINT_ID id, const char * buf, int len)
		{
			return nfapi::nf_tcpPostReceive(id, buf, len);
		}

		virtual NF_STATUS tcpSetConnectionState(nfapi::ENDPOINT_ID id, int suspended)
		{
			return nfapi::nf_tcpSetConnectionState(id, suspended);
		}

		virtual NF_STATUS udpPostSend(nfapi::ENDPOINT_ID id, const unsigned char * remoteAddress, 
										const char * buf, int len, 
										nfapi::PNF_UDP_OPTIONS options)
		{
			return nfapi::nf_udpPostSend(id, remoteAddress, buf, len, options);
		}

		virtual NF_STATUS udpPostReceive(nfapi::ENDPOINT_ID id, const unsigned char * remoteAddress, 
				const char * buf, int len, nfapi::PNF_UDP_OPTIONS options)
		{
			return nfapi::nf_udpPostReceive(id, remoteAddress, buf, len, options);
		}

		virtual NF_STATUS udpSetConnectionState(nfapi::ENDPOINT_ID id, int suspended)
		{
			return nfapi::nf_udpSetConnectionState(id, suspended);
		}
	};

}
#endif