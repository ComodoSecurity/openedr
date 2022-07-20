//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _CERTIMP_H
#define _CERTIMP_H
 
namespace ProtocolFilters
{
	/**
	*	Import certificate to Windows system storage.
	*/
	BOOL importCertificateToWindowsStorage(LPCSTR szCert, DWORD certLen, 
					  LPSTR szStore, BOOL bUser);

	void importCertificateToMozillaAndOpera(const std::wstring & cert, NFSyncEvent & hEvent);

	void importCertificateToPidgin(const std::wstring & cert, NFSyncEvent & hEvent);
}

#endif // _CERTIMP_H