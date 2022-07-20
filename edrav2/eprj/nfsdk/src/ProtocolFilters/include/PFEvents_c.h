//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_PFEVENTS_C)
#define _PFEVENTS_C

#include "nfevents.h"
#include "PFCDefs.h"
#include "PFObject_c.h"

#ifdef __cplusplus
extern "C" 
{
#endif

#pragma pack(push, 1)
	typedef struct _PFEvents_c
	{
        // The events from NF_EventHandler
		void (PFAPI_CC *threadStart)();

		void (PFAPI_CC *threadEnd)();

		void (PFAPI_CC *tcpConnectRequest)(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);

		void (PFAPI_CC *tcpConnected)(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);
		
		void (PFAPI_CC *tcpClosed)(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo);

		void (PFAPI_CC *udpCreated)(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo);
		
		void (PFAPI_CC *udpConnectRequest)(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq);

		void (PFAPI_CC *udpClosed)(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo);

        // New object is ready for filtering
		void (PFAPI_CC *dataAvailable)(ENDPOINT_ID id, PFObject_c * pObject);
		
        // A part of content is available for examining.
		PF_DATA_PART_CHECK_RESULT (PFAPI_CC *dataPartAvailable)(ENDPOINT_ID id, PFObject_c * pObject);

        // The library calls this functions to post the filtered data buffers
        // to destination, and to control the state of filtered connections.

		NF_STATUS (PFAPI_CC *tcpPostSend)(ENDPOINT_ID id, const char * buf, int len);

		NF_STATUS (PFAPI_CC *tcpPostReceive)(ENDPOINT_ID id, const char * buf, int len);

		NF_STATUS (PFAPI_CC *tcpSetConnectionState)(ENDPOINT_ID id, int suspended);

		NF_STATUS (PFAPI_CC *udpPostSend)(ENDPOINT_ID id, const unsigned char * remoteAddress, 
										const char * buf, int len,
										PNF_UDP_OPTIONS options);

		NF_STATUS (PFAPI_CC *udpPostReceive)(ENDPOINT_ID id, const unsigned char * remoteAddress, 
										const char * buf, int len,
										PNF_UDP_OPTIONS options);

		NF_STATUS (PFAPI_CC *udpSetConnectionState)(ENDPOINT_ID id, int suspended);

	} PFEvents_c, *PPFEvents_c;

#pragma pack(pop)
	
#ifdef __cplusplus
}
#endif

#endif