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

#include "IFilterBase.h"
#include "Settings.h"
#include "SSLSessionData.h"

namespace ProtocolFilters
{
	class SSLFilter : public IFilterBase
	{
	public:
		SSLFilter(ProxySession * pSession, tPF_FilterFlags flags);
		~SSLFilter(void);

		eFilterResult tcp_packet(eDataDirection dd,
								ePacketDirection pd,
								const char * buf, int len);
		
		eFilterResult udp_packet(eDataDirection dd,
								ePacketDirection pd,
								const unsigned char * remoteAddress, 
								const char * buf, int len);

		PF_FilterType getType();

		PF_FilterCategory getCategory();

		bool postObject(PFObjectImpl & object);

		PF_FilterFlags getFlags()
		{
			return (PF_FilterFlags)m_flags;
		}

		bool addException(bool useCounter = true);
		void clearExceptions();
		void clearTlsExceptions(const char * type = NULL);
		void addTlsException(const char * type = NULL);
		void confirmTlsException(const char * type = NULL);

		void setRequestClientCert(bool request)
		{
			m_requestClientCert = request;
		}
		bool getRequestClientCert()
		{
			return m_requestClientCert;
		}

		X509 * getClientCert()
		{
			return m_clientCert;
		}

		EVP_PKEY * getClientPKey()
		{
			return m_clientPKey;
		}

		void freeClientKeys()
		{
			if (m_clientCert)
			{
				X509_free(m_clientCert);
				m_clientCert = NULL;
			}
			if (m_clientPKey)
			{
				EVP_PKEY_free(m_clientPKey);
				m_clientPKey = NULL;
			}
		}
		void resetClientKeys()
		{
			m_clientCert = NULL;
			m_clientPKey = NULL;
		}

		void setALPNProtos(const void * protos, int len)
		{
			m_alpnProtos.reset();
			m_alpnProtos.append((char*)protos, len);
		}

		IOB & getALPNProtosSelected()
		{
			return m_alpnProtosSelected;
		}

		PF_DATA_PART_CHECK_RESULT indicateClientCertRequest();

		SSL_SESSION * getRemoteSSLSession()
		{
			if (!m_sdRemote.m_pSSL)
				return NULL;

			return SSL_get1_session(m_sdRemote.m_pSSL);
		}

		void setRemoteSSLSession(SSL_SESSION * pSession)
		{
			if (m_assocSSLSession)
			{
				SSL_SESSION_free(m_assocSSLSession);
			}
			m_assocSSLSession = pSession;
		}

	protected:
		int SSLRead_server(SSLSessionData & sd);
		int SSLRead_client(SSLSessionData & sd);
		bool isException();
		bool isTlsException(const char * type = NULL);
		bool tryNextPackets();

	private:
		ProxySession * m_pSession;
		bool m_bTls;

		SSLSessionData m_sdLocal;
		SSLSessionData m_sdRemote;
		IOB  m_tempBufferLocal;
		IOB  m_tempBufferRemote;

		X509 * m_clientCert;
		EVP_PKEY * m_clientPKey;

		unsigned int m_bytesTransmitted;
			
		enum SSL_FILTER_STATE
		{
			SFS_NONE,
			SFS_CLIENT_HANDSHAKE,
			SFS_SERVER_HANDSHAKE,
			SFS_DATA_EXCHANGE,
			SFS_DISCONNECTED,
			SFS_SERVER_HANDSHAKE_REQUEST_CLIENT_CERT
		};
		
		SSL_FILTER_STATE m_state;
		bool m_serverHandshake;
		int m_remoteDisconnectState;
		int m_localDisconnectState;
		bool m_requestClientCert;
		bool m_clearExceptions;
		bool m_compatibility;
		IOB m_alpnProtos;
		IOB m_alpnProtosSelected;
		IOB	m_ocspEncodedResponse;
		SSL_SESSION * m_assocSSLSession;
		bool m_bFirstPacket;
		nf_time_t m_tlsEstablishedTs;

		tPF_FilterFlags m_flags;
		GEN_SESSION_INFO m_si;
		std::string m_tlsDomain;
	};
}