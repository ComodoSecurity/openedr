//
// 	NetFilterSDK 
// 	Copyright (C) Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _CERTIMP_NIX_H
#define _CERTIMP_NIX_H

#include "sync.h"

namespace ProtocolFilters
{
	bool importCertificateToSystemStorage(const char * certName, const char * certPath, void * pCert, int certLen);

	void importCertificateToMozilla(const std::string & cert, NFSyncEvent & hEvent);
}

#endif // _CERTIMP_H