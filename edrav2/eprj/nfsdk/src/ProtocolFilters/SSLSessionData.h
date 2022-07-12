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

#include "IOB.h"
#include "SSLDataProvider.h"
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace ProtocolFilters
{
	class SSLSessionData
	{
	public:
		SSLSessionData(void);
		~SSLSessionData(void);

		bool init(bool bTls, bool bServer = false, SSLKeys * pkeys = NULL, int ssl_version = 0, tPF_FilterFlags flags = 0);
		void free();

		int useCertificateBuffer(char * buf, int len, int type);
		int usePrivateKeyBuffer(char * buf, int len, int type);
		
		SSL_CTX *	m_pCtx;
		BIO *		m_pbioRead;
		BIO *		m_pbioWrite;
		SSL *		m_pSSL;
		IOB			m_buffer;
	};
}