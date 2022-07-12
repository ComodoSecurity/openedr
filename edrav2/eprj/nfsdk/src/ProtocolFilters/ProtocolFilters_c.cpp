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

#ifdef _C_API

#include "ProtocolFilters_c.h"
#include "ProtocolFilters.h"
#include "Proxy.h"

class NF_EventHandlerWrapper : public NF_EventHandler
{
public:
	static ProtocolFilters::Proxy * m_pProxy;

	NF_EventHandlerWrapper()
	{
		NF_EventHandler eh = { 
			threadStart,
			threadEnd,
			tcpConnectRequest,
			tcpConnected,
			tcpClosed,
			tcpReceive,
			tcpSend,
			tcpCanReceive,
			tcpCanSend,
			udpCreated,
			udpConnectRequest,
			udpClosed,
			udpReceive,
			udpSend,
			udpCanReceive,
			udpCanSend
		};

		*(NF_EventHandler*)this = eh;
		m_pProxy = (ProtocolFilters::Proxy *)ProtocolFilters::pf_getNFEventHandler();
	}

	static void threadStart()
	{
		m_pProxy->threadStart();
	}

	static void threadEnd()
	{
		m_pProxy->threadEnd();
	}
		
	static void tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		m_pProxy->tcpConnectRequest(id, pConnInfo);
	}

	static void tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		m_pProxy->tcpConnected(id, pConnInfo);
	}

	static void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		m_pProxy->tcpClosed(id, pConnInfo);
	}

	static void tcpReceive(ENDPOINT_ID id, const char * buf, int len)
	{	
		m_pProxy->tcpReceive(id, buf, len);
	}

	static void tcpSend(ENDPOINT_ID id, const char * buf, int len)
	{
		m_pProxy->tcpSend(id, buf, len);
	}

	static void tcpCanReceive(ENDPOINT_ID id)
	{
		m_pProxy->tcpCanReceive(id);
	}

	static void tcpCanSend(ENDPOINT_ID id)
	{
		m_pProxy->tcpCanSend(id);
	}

	static void udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		m_pProxy->udpCreated(id, pConnInfo);
	}

	static void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
	{
		m_pProxy->udpConnectRequest(id, pConnReq);
	}

	static void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		m_pProxy->udpClosed(id, pConnInfo);
	}

	static void udpReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{	
		m_pProxy->udpReceive(id, remoteAddress, buf, len, options);
	}

	static void udpSend(ENDPOINT_ID id, const unsigned char * remoteAddress, const char * buf, int len, PNF_UDP_OPTIONS options)
	{
		m_pProxy->udpSend(id, remoteAddress, buf, len, options);
	}

	static void udpCanReceive(ENDPOINT_ID id)
	{
		m_pProxy->udpCanReceive(id);
	}

	static void udpCanSend(ENDPOINT_ID id)
	{
		m_pProxy->udpCanSend(id);
	}

};


class PFEventsWrapper : public ProtocolFilters::PFEvents
{
public:
	PFEvents_c m_events;

	void init(PFEvents_c * p) 
	{
		memcpy(&m_events, p, sizeof(m_events));
	}

	void threadStart()
	{
		m_events.threadStart();
	}

	void threadEnd()
	{
		m_events.threadEnd();
	}

	void tcpConnectRequest(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		m_events.tcpConnectRequest(id, pConnInfo);
	}

	void tcpConnected(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		m_events.tcpConnected(id, pConnInfo);
	}
	
	void tcpClosed(ENDPOINT_ID id, PNF_TCP_CONN_INFO pConnInfo)
	{
		m_events.tcpClosed(id, pConnInfo);
	}

	void udpCreated(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		m_events.udpCreated(id, pConnInfo);
	}
	
	void udpConnectRequest(ENDPOINT_ID id, PNF_UDP_CONN_REQUEST pConnReq)
	{
		m_events.udpConnectRequest(id, pConnReq);
	}

	void udpClosed(ENDPOINT_ID id, PNF_UDP_CONN_INFO pConnInfo)
	{
		m_events.udpClosed(id, pConnInfo);
	}

	void dataAvailable(ENDPOINT_ID id, ProtocolFilters::PFObject * pObject)
	{
		m_events.dataAvailable(id, pObject);
	}
	
	PF_DATA_PART_CHECK_RESULT dataPartAvailable(ENDPOINT_ID id, ProtocolFilters::PFObject * pObject)
	{
		return (PF_DATA_PART_CHECK_RESULT)m_events.dataPartAvailable(id, pObject);
	}

	NF_STATUS tcpPostSend(ENDPOINT_ID id, const char * buf, int len)
	{
		return m_events.tcpPostSend(id, buf, len);
	}

	NF_STATUS tcpPostReceive(ENDPOINT_ID id, const char * buf, int len)
	{
		return m_events.tcpPostReceive(id, buf, len);
	}

	NF_STATUS tcpSetConnectionState(ENDPOINT_ID id, int suspended)
	{
		return m_events.tcpSetConnectionState(id, suspended);
	}

	NF_STATUS udpPostSend(ENDPOINT_ID id, const unsigned char * remoteAddress, 
									const char * buf, int len, PNF_UDP_OPTIONS options)
	{
		return m_events.udpPostSend(id, remoteAddress, buf, len, options);
	}

	NF_STATUS udpPostReceive(ENDPOINT_ID id, const unsigned char * remoteAddress, 
									const char * buf, int len, PNF_UDP_OPTIONS options)
	{
		return m_events.udpPostReceive(id, remoteAddress, buf, len, options);
	}

	NF_STATUS udpSetConnectionState(ENDPOINT_ID id, int suspended)
	{
		return m_events.udpSetConnectionState(id, suspended);
	}

};

static PFEventsWrapper g_ew;
static NF_EventHandlerWrapper g_ehw;
ProtocolFilters::Proxy * NF_EventHandlerWrapper::m_pProxy = NULL;

#ifdef WIN32
PF_API_C int PFAPI_CC pfc_init(PFEvents_c * pHandler, const wchar_t * dataFolder)
#else
PF_API_C int PFAPI_CC pfc_init(PFEvents_c * pHandler, const char * dataFolder)
#endif
{
	g_ew.init(pHandler);

	return ProtocolFilters::pf_init(&g_ew, dataFolder);
}

PF_API_C void PFAPI_CC pfc_free()
{
	ProtocolFilters::pf_free();
}


PF_API_C NF_EventHandler * PFAPI_CC pfc_getNFEventHandler()
{
	return &g_ehw;
}

PF_API_C int PFAPI_CC pfc_postObject(NFAPI_NS ENDPOINT_ID id, PFObject_c * pObject)
{
	return ProtocolFilters::pf_postObject(id, (ProtocolFilters::PFObject*)pObject);
}

PF_API_C int PFAPI_CC pfc_addFilter(NFAPI_NS ENDPOINT_ID id,
				PF_FilterType type, 
				tPF_FilterFlags flags,
				PF_OpTarget target, 
				PF_FilterType typeBase)
{
	return ProtocolFilters::pf_addFilter(id, type, flags, target, typeBase);
}
	
PF_API_C int PFAPI_CC pfc_deleteFilter(NFAPI_NS ENDPOINT_ID id, PF_FilterType type)
{
	return ProtocolFilters::pf_deleteFilter(id, type);
}

PF_API_C int PFAPI_CC pfc_getFilterCount(NFAPI_NS ENDPOINT_ID id)
{
	return ProtocolFilters::pf_getFilterCount(id);
}

PF_API_C BOOL PFAPI_CC pfc_isFilterActive(NFAPI_NS ENDPOINT_ID id, PF_FilterType type)
{
	return ProtocolFilters::pf_isFilterActive(id, type);
}

PF_API_C BOOL PFAPI_CC pfc_canDisableFiltering(NFAPI_NS ENDPOINT_ID id)
{
	return ProtocolFilters::pf_canDisableFiltering(id);
}

PF_API_C void PFAPI_CC pfc_setRootSSLCertSubject(const char * rootSubject)
{
	ProtocolFilters::pf_setRootSSLCertSubject(rootSubject);
}

PF_API_C void PFAPI_CC pfc_setRootSSLCertSubjectEx(const char * rootSubject, const char * x509, int x509Len, const char * pkey, int pkeyLen)
{
	ProtocolFilters::pf_setRootSSLCertSubjectEx(rootSubject, x509, x509Len, pkey, pkeyLen);
}

PF_API_C void PFAPI_CC pfc_setRootSSLCertImportFlags(unsigned long flags)
{
	ProtocolFilters::pf_setRootSSLCertImportFlags(flags);
}

PF_API_C BOOL PFAPI_CC pfc_loadCAStore(const char * rootCAFileName)
{
	return ProtocolFilters::pf_loadCAStore(rootCAFileName);
}

#ifdef WIN32
PF_API_C BOOL PFAPI_CC pfc_getRootSSLCertFileName(wchar_t * fileName, int fileNameLen)
{
	return ProtocolFilters::pf_getRootSSLCertFileName(fileName, fileNameLen);
}
#else
PF_API_C BOOL PFAPI_CC pfc_getRootSSLCertFileName(char * fileName, int fileNameLen)
{
	return ProtocolFilters::pf_getRootSSLCertFileName(fileName, fileNameLen);
}
#endif

PF_API_C BOOL PFAPI_CC pfc_readHeader(PPFStream_c pStream, PPFHeader_c ph)
{
	ProtocolFilters::PFStream * pStreamCpp = (ProtocolFilters::PFStream *)pStream;
	ProtocolFilters::PFHeader * pHeaderCpp = (ProtocolFilters::PFHeader *)ph;
	int res = FALSE;

	if (!pHeaderCpp || !pStreamCpp || !pStreamCpp->size())
		return FALSE;

	pHeaderCpp->clear();

	char * buf = (char*)malloc((int)pStreamCpp->size() + 1);
	if (!buf)
		return res;

	pStreamCpp->seek(0, FILE_BEGIN);
	pStreamCpp->read(buf, (int)pStreamCpp->size());
	buf[pStreamCpp->size()] = '\0';

	res = pHeaderCpp->parseString(buf, (int)pStreamCpp->size());
	
	free(buf);

	return res;
}

PF_API_C BOOL PFAPI_CC pfc_writeHeader(PPFStream_c pStream, PPFHeader_c ph)
{
	ProtocolFilters::PFStream * pStreamCpp = (ProtocolFilters::PFStream *)pStream;
	ProtocolFilters::PFHeader * pHeaderCpp = (ProtocolFilters::PFHeader *)ph;
	int res = FALSE;

	if (!pHeaderCpp || !pStreamCpp)
		return res;

	std::string s = pHeaderCpp->toString() + "\r\n";

	pStreamCpp->reset();
	res = pStreamCpp->write(s.c_str(), (int)s.length()) == s.length();

	return res;
}

#ifdef WIN32
PF_API_C BOOL PFAPI_CC pfc_getProcessOwnerA(unsigned long processId, char * buf, int len)
{
	return ProtocolFilters::pf_getProcessOwnerA(processId, buf, len);
}

PF_API_C BOOL PFAPI_CC pfc_getProcessOwnerW(unsigned long processId, wchar_t * buf, int len)
{
	return ProtocolFilters::pf_getProcessOwnerW(processId, buf, len);
}
#endif

PF_API_C BOOL PFAPI_CC pfc_unzipStream(PPFStream_c pStream)
{
	return ProtocolFilters::pf_unzipStream((ProtocolFilters::PFStream *)pStream);
}

PF_API_C void PFAPI_CC pfc_waitForImportCompletion()
{
	return ProtocolFilters::pf_waitForImportCompletion();
}

PF_API_C BOOL PFAPI_CC pfc_startLog(const char * logFileName)
{
#if defined(_DEBUG) || defined(_RELEASE_LOG)
	return ProtocolFilters::pf_startLog(logFileName);
#else
	return TRUE;
#endif
}

PF_API_C void PFAPI_CC pfc_stopLog()
{
#if defined(_DEBUG) || defined(_RELEASE_LOG)
	ProtocolFilters::pf_stopLog();
#endif
}

PF_API_C void PFAPI_CC pfc_setExceptionsTimeout(eEXCEPTION_CLASS ec, unsigned __int64 timeout)
{
	ProtocolFilters::pf_setExceptionsTimeout(ec, timeout);
}

PF_API_C void PFAPI_CC pfc_deleteExceptions(eEXCEPTION_CLASS ec)
{
	ProtocolFilters::pf_deleteExceptions(ec);
}

#endif