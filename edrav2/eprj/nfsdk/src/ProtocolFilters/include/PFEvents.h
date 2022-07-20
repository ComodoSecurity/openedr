//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_PFEVENTS)
#define _PFEVENTS

#include "nfevents.h"
#include "PFObject.h"

namespace ProtocolFilters
{
	class PFEvents
	{
	public:
        // The events from NF_EventHandler

		virtual void threadStart() = 0;

		virtual void threadEnd() = 0;

		virtual void tcpConnectRequest(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_TCP_CONN_INFO pConnInfo) = 0;

		virtual void tcpConnected(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_TCP_CONN_INFO pConnInfo) = 0;
		
		virtual void tcpClosed(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_TCP_CONN_INFO pConnInfo) = 0;

		virtual void udpCreated(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_UDP_CONN_INFO pConnInfo) = 0;
		
		virtual void udpConnectRequest(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_UDP_CONN_REQUEST pConnReq) = 0;

		virtual void udpClosed(NFAPI_NS ENDPOINT_ID id, NFAPI_NS PNF_UDP_CONN_INFO pConnInfo) = 0;

        // New object is ready for filtering
		virtual void dataAvailable(NFAPI_NS ENDPOINT_ID id, PFObject * pObject) = 0;
		
        // A part of content is available for examining.
		virtual PF_DATA_PART_CHECK_RESULT dataPartAvailable(NFAPI_NS ENDPOINT_ID id, PFObject * pObject) = 0;

        // The library calls this functions to post the filtered data buffers
        // to destination, and to control the state of filtered connections.

		virtual NF_STATUS tcpPostSend(NFAPI_NS ENDPOINT_ID id, const char * buf, int len) = 0;

		virtual NF_STATUS tcpPostReceive(NFAPI_NS ENDPOINT_ID id, const char * buf, int len) = 0;

		virtual NF_STATUS tcpSetConnectionState(NFAPI_NS ENDPOINT_ID id, int suspended) = 0;

		virtual NF_STATUS udpPostSend(NFAPI_NS ENDPOINT_ID id, const unsigned char * remoteAddress, 
										const char * buf, int len, NFAPI_NS PNF_UDP_OPTIONS options) = 0;

		virtual NF_STATUS udpPostReceive(NFAPI_NS ENDPOINT_ID id, const unsigned char * remoteAddress, 
										const char * buf, int len, NFAPI_NS PNF_UDP_OPTIONS options) = 0;

		virtual NF_STATUS udpSetConnectionState(NFAPI_NS ENDPOINT_ID id, int suspended) = 0;
	};
	
}
#endif