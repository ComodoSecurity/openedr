//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_PROTOCOL_FILTERS_C)
#define _PROTOCOL_FILTERS_C

#include "PFCDefs.h"
#include "PFObject_c.h"
#include "PFEvents_c.h"
#include "PFFilterDefs.h"
#include "PFHeader_c.h"

#ifdef __cplusplus
extern "C" 
{
#endif

	///////////////////////////////////////////////////////////////////////
	// API functions
	///////////////////////////////////////////////////////////////////////

#ifdef WIN32
	PF_API_C BOOL pfc_init(PFEvents_c * pHandler, const wchar_t * dataFolder);
#else
	PF_API_C BOOL pfc_init(PFEvents_c * pHandler, const char * dataFolder);
#endif

	PF_API_C void pfc_free();

	PF_API_C NF_EventHandler * pfc_getNFEventHandler();

	PF_API_C BOOL pfc_postObject(ENDPOINT_ID id, PFObject_c * pObject);

	PF_API_C BOOL pfc_addFilter(ENDPOINT_ID id,
						PF_FilterType type, 
						tPF_FilterFlags flags,
						PF_OpTarget target, 
						PF_FilterType typeBase);
			
	PF_API_C BOOL pfc_deleteFilter(ENDPOINT_ID id, PF_FilterType type);

	PF_API_C int pfc_getFilterCount(ENDPOINT_ID id);

	PF_API_C BOOL pfc_isFilterActive(ENDPOINT_ID id, PF_FilterType type);

	PF_API_C void pfc_setRootSSLCertSubject(const char * rootSubject);
	PF_API_C void pfc_setRootSSLCertSubjectEx(const char * rootSubject, const char * x509, int x509Len, const char * pkey, int pkeyLen);
	
	PF_API_C void pfc_setRootSSLCertImportFlags(unsigned long flags);

	PF_API_C BOOL pfc_loadCAStore(const char * rootCAFileName);

#ifdef WIN32
	PF_API_C BOOL pfc_getRootSSLCertFileName(wchar_t * fileName, int fileNameLen);
#else
	PF_API_C BOOL pfc_getRootSSLCertFileName(char * fileName, int fileNameLen);
#endif

	PF_API_C BOOL pfc_canDisableFiltering(ENDPOINT_ID id);

	PF_API_C PFObject_c * pfc_createObject(tPF_ObjectType type, int nStreams);
	
	///////////////////////////////////////////////////////////////////////
	// Support functions
	///////////////////////////////////////////////////////////////////////

	PF_API_C BOOL pfc_getProcessOwnerA(unsigned long processId, char * buf, int len);
	PF_API_C BOOL pfc_getProcessOwnerW(unsigned long processId, wchar_t * buf, int len);

#ifdef _UNICODE
#define pfc_getProcessOwner pfc_getProcessOwnerW
#else
#define pfc_getProcessOwner pfc_getProcessOwnerA
#endif

	PF_API_C BOOL pfc_readHeader(PPFStream_c pStream, PPFHeader_c ph);
	PF_API_C BOOL pfc_writeHeader(PPFStream_c pStream, PPFHeader_c ph);

	PF_API_C BOOL pfc_unzipStream(PPFStream_c pStream);

	PF_API_C void pfc_waitForImportCompletion();

	PF_API_C BOOL pfc_startLog(const char * logFileName);
	PF_API_C void pfc_stopLog();

	PF_API_C void pfc_setExceptionsTimeout(eEXCEPTION_CLASS ec, unsigned __int64 timeout);

	PF_API_C void pfc_deleteExceptions(eEXCEPTION_CLASS ec);

#ifdef __cplusplus
}
#endif

#endif