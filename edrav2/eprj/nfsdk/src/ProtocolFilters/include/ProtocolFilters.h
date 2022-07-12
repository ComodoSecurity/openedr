// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_PROTOCOL_FILTERS)
#define _PROTOCOL_FILTERS

#include "PFCDefs.h"
#include "PFObject.h"
#include "PFEvents.h"
#include "PFFilterDefs.h"
#include "PFHeader.h"

namespace ProtocolFilters
{
	///////////////////////////////////////////////////////////////////////
	// API functions
	///////////////////////////////////////////////////////////////////////

    /**
	 * Initialize the library
	 * @param pHandler Event handler
	 * @param dataFolder A path to configuration folder, where the library
			  stores SSL certificates and temporary files.
     */
#ifdef WIN32
	PF_API BOOL pf_init(PFEvents * pHandler, const wchar_t * dataFolder);
#else
	PF_API BOOL pf_init(PFEvents * pHandler, const char * dataFolder);
#endif
	/** 
	 * Free the library 
	 */
	PF_API void pf_free();

	/** 
	 * Returns a pointer to event handler class for passing to nf_init() 
	 */
	PF_API NFAPI_NS NF_EventHandler * pf_getNFEventHandler();

	/**
	 *	Post an object to specified endpoint
	 *	@param id Endpoint id
	 *	@param pObject Object with some content
	 */
	PF_API BOOL pf_postObject(NFAPI_NS ENDPOINT_ID id, PFObject * pObject);

	/**
	 *	Adds a new filter to session filtering chain.
	 *	@param id Endpoint id
	 *	@param type Type of the filter to add
	 *	@param flags Filter specific flags
	 *	@param target Position where to add new filter (OT_NEXT, OT_NEXT - relative to typeBase)
	 *	@param typeBase Type of origin filter
	 */
	PF_API BOOL pf_addFilter(NFAPI_NS ENDPOINT_ID id,
						PF_FilterType type, 
						tPF_FilterFlags flags = FF_DEFAULT,
						PF_OpTarget target = OT_LAST, 
						PF_FilterType typeBase = FT_NONE);
			
	/**
	 *	Removes the specified filter from chain.
	 *	@param id Endpoint id
	 *	@param type Type of the filter to remove
	 */
	PF_API BOOL pf_deleteFilter(NFAPI_NS ENDPOINT_ID id, PF_FilterType type);

	/**
	 *	Returns the number of active filters for the specified connection.
	 *	@param id Endpoint id
	 */
	PF_API int pf_getFilterCount(NFAPI_NS ENDPOINT_ID id);

	/**
	 *	Returns TRUE if there is a filter of the specified type in filtering chain
	 *	@param id Endpoint id
	 *	@param type Filter type
	 */
	PF_API BOOL pf_isFilterActive(NFAPI_NS ENDPOINT_ID id, PF_FilterType type);

	/**
	 *	Returns true when it is safe to disable filtering for the connection 
	 *  with specified id (there are no filters in chain and internal buffers are empty).
	 */
	PF_API BOOL pf_canDisableFiltering(NFAPI_NS ENDPOINT_ID id);

	/**
	 *	Specifies a subject of root certificate, used for generating other SSL certificates.
	 *	This name appears in "Issued by" field of certificates assigned to filtered SSL
	 *	connections. Default value - "NetFilterSDK".
	 */
	PF_API void pf_setRootSSLCertSubject(const char * rootSubject);
	PF_API void pf_setRootSSLCertSubjectEx(const char * rootSubject, const char * x509, int x509Len, const char * pkey, int pkeyLen);
	
	/**
	 *	Specifies import flags from ePF_RootSSLImportFlag enumeration, allowing to control
	 *	Importing root SSL certificate in pf_setRootSSLCertSubject to supported storages.
	 *	Default value - RSIF_IMPORT_EVERYWHERE.
	 */
	PF_API void pf_setRootSSLCertImportFlags(unsigned long flags);

	PF_API BOOL pf_loadCAStore(const char * rootCAFileName);

#ifdef WIN32
	PF_API BOOL pf_getRootSSLCertFileName(wchar_t * fileName, int fileNameLen);
#else
	PF_API BOOL pf_getRootSSLCertFileName(char * fileName, int fileNameLen);
#endif

	///////////////////////////////////////////////////////////////////////
	// Support functions
	///////////////////////////////////////////////////////////////////////
	
#ifdef WIN32
	/**
	 *	Returns an owner of the specified process formatted as <domain>\<user name>.
	 *	@param processId Process identifier
	 *	@param buf Buffer
	 *	@param len Number of elements in buf
	 */
	PF_API BOOL pf_getProcessOwnerA(unsigned long processId, char * buf, int len);
	PF_API BOOL pf_getProcessOwnerW(unsigned long processId, wchar_t * buf, int len);

#ifdef _UNICODE
#define pf_getProcessOwner pf_getProcessOwnerW
#else
#define pf_getProcessOwner pf_getProcessOwnerA
#endif

#endif

	/**
	 *	Load header from stream
	 */
	inline BOOL pf_readHeader(PFStream * pStream, PFHeader * ph)
	{
		if (!ph || !pStream || !pStream->size())
			return false;

		ph->clear();

		char * buf = (char*)malloc((int)pStream->size() + 1);
		if (!buf)
			return false;

		pStream->seek(0, FILE_BEGIN);
		pStream->read(buf, (int)pStream->size());
		buf[pStream->size()] = '\0';

		BOOL res = ph->parseString(buf, (int)pStream->size());
		
		free(buf);

		return res;
	}

	/**
	 *	Save header to stream
	 */
	inline BOOL pf_writeHeader(PFStream * pStream, PFHeader * ph)
	{
		if (!ph || !pStream)
			return false;

		std::string s = ph->toString() + "\r\n";

		pStream->reset();

		return pStream->write(s.c_str(), (int)s.length()) == s.length();
	}

	/**
	 *	Uncompress stream contents
	 */
	PF_API BOOL pf_unzipStream(PFStream * pStream);

	/**
	 *	Return after root certificate import thread is completed
	 */
	PF_API void pf_waitForImportCompletion();

	PF_API BOOL pf_startLog(const char * logFileName);
	PF_API void pf_stopLog();

	/**
	 *	Set timeout in seconds for SSL exceptions of the specified class
	 */
	PF_API void pf_setExceptionsTimeout(eEXCEPTION_CLASS ec, unsigned __int64 timeout);

    /**
	 * Delete SSL exceptions of the specified class
	 */
	PF_API void pf_deleteExceptions(eEXCEPTION_CLASS ec);
}


#endif