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
 
#ifndef PF_NO_SSL_FILTER

#if _MSC_VER < 1700
#define _InterlockedExchangeAdd InterlockedExchangeAdd
#endif

#include "SSLFilter.h"
#include "ProxySession.h"
#include "auxutil.h"
#include <openssl/pkcs12.h>
#include <openssl/ocsp.h>
#include "openssl/ssl/ssl_locl.h"

using namespace ProtocolFilters;

#define TLS_1_2_PROTOCOL "tls1_2"
#define TLS_1_0_PROTOCOL "tls1_0"
#define TLS_ALL_CIPHERS "tls_ac"
#define VERIFY_TIMEOUT (60 * 60 * 24)
#define INVALID_CERT_TIMEOUT (1)

typedef std::map<SSL*, SSLFilter*> tCtxMap;
static tCtxMap g_ctxMap;
static AutoCriticalSection g_ctxMapCS;

#ifdef WIN32

static int client_cert_cb(SSL *ssl, X509 **x509, EVP_PKEY **pkey)
{
	AutoLock lock(g_ctxMapCS);

	DbgPrint("Client certificate requested");
	
	tCtxMap::iterator it = g_ctxMap.find(ssl);
	if (it != g_ctxMap.end())
	{
		if (it->second->getFlags() & FF_SSL_INDICATE_CLIENT_CERT_REQUESTS)
		{
			PF_DATA_PART_CHECK_RESULT cr;
			cr = it->second->indicateClientCertRequest();
			if (cr == DPCR_BYPASS)
				return 0;
		}

		if (it->second->getFlags() & FF_SSL_SUPPORT_CLIENT_CERTIFICATES)
		{
			if (it->second->getClientCert() && it->second->getClientPKey())
			{
				DbgPrint("Client certificate specified");
				*x509 = it->second->getClientCert();
				*pkey = it->second->getClientPKey();
				it->second->resetClientKeys();
				return 1;
			} else
			{
				if (it->second->getRequestClientCert())
					return 0;

				it->second->setRequestClientCert(true);

				return -1;
			}
		} else
		{
			it->second->addException();
		}
	}

	return 0;
}

#endif

static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	return 1;
}

static int ocsp_resp_cb(SSL *s, void *arg)
{
    const unsigned char *p;
    int len;

	IOB * ocspEncodedResponse = (IOB*)arg;

	len = SSL_get_tlsext_status_ocsp_resp(s, &p);
    
	if (!p)
    {
        return 1;
    }

	ocspEncodedResponse->reset();
	ocspEncodedResponse->append((char*)p, len);

	return 1;
}


SSLFilter::SSLFilter(ProxySession * pSession, tPF_FilterFlags flags) :
	m_pSession(pSession),
	m_state(SFS_NONE),
	m_flags(flags)
{
	m_pSession->getSessionInfo(&m_si);
	m_bTls = (flags & (FF_SSL_TLS | FF_SSL_TLS_AUTO)) != 0;
	m_serverHandshake = false;
	m_remoteDisconnectState = 0;
	m_localDisconnectState = 0;
	m_bytesTransmitted = 0;
	setActive(false);
	m_requestClientCert = false;
	m_clientCert = NULL;
	m_clientPKey = NULL;
	m_clearExceptions = true;
	m_compatibility = false;
	m_assocSSLSession = NULL;
	m_bFirstPacket = true;
	m_tlsEstablishedTs = 0;
}

SSLFilter::~SSLFilter(void)
{
	AutoLock lock(g_ctxMapCS);
	g_ctxMap.erase(m_sdRemote.m_pSSL);
	freeClientKeys();

	if (m_assocSSLSession)
	{
		SSL_SESSION_free(m_assocSSLSession);
		m_assocSSLSession = NULL;
	}
}

int SSLFilter::SSLRead_server(SSLSessionData & sd)
{
	int n, err;

	if (!SSL_is_init_finished(sd.m_pSSL))
	{
		n = SSL_accept(sd.m_pSSL);
		if (n < 0)
		{
			err = SSL_get_error(sd.m_pSSL, n);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
			{
				return 0;
			}

			return -1;
		}

		return 0;
	}

	n = SSL_read(sd.m_pSSL, sd.m_buffer.buffer(), sd.m_buffer.size());
	if (n < 0)
	{
		err = SSL_get_error(sd.m_pSSL, n);

		if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
		{
			return 0;
		}

		return 0;
	}

	return n;
}

int SSLFilter::SSLRead_client(SSLSessionData & sd)
{
	int n, err;

	if (!SSL_is_init_finished(sd.m_pSSL))
	{
		n = SSL_connect(sd.m_pSSL);
		if (n < 0)
		{
			err = SSL_get_error(sd.m_pSSL, n);
			if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
			{
				return 0;
			}

			return -1;
		}

		return 0;
	}

	n = SSL_read(sd.m_pSSL, sd.m_buffer.buffer(), sd.m_buffer.size());
	if (n < 0)
	{
		err = SSL_get_error(sd.m_pSSL, n);

		if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE)
		{
			return 0;
		}

		char buf[256];

		ERR_error_string_n(ERR_peek_last_error(), buf, sizeof(buf));

		return 0;
	}

	return n;
}

PF_DATA_PART_CHECK_RESULT SSLFilter::indicateClientCertRequest()
{
	PF_DATA_PART_CHECK_RESULT cr = DPCR_FILTER;

	PFEvents * pHandler = m_pSession->getParsersEventHandler();
	if (pHandler)
	{
		PFStream * pStream;
		PFObjectImpl obj(OT_SSL_CLIENT_CERT_REQUEST, 1);

		pStream = obj.getStream(0);
		if (pStream)
		{
			if (!m_tlsDomain.empty())
			{
				pStream->write(m_tlsDomain.c_str(), (tStreamSize)m_tlsDomain.length()+1);
			} else
			{
				std::string hostname = m_pSession->getRemoteEndpointStr();
				size_t pos = hostname.find(":");
				if (pos > 0)
				{
					hostname = hostname.substr(0, pos);
				}
				pStream->write(hostname.c_str(), (tStreamSize)hostname.length()+1);
			}
			pStream->seek(0, FILE_BEGIN);
		}

		obj.setReadOnly(true);

		try {
			DbgPrint("SSLFilter::indicateClientCertRequest() dataPartAvailable, type=%d", obj.getType());
			cr = pHandler->dataPartAvailable(m_pSession->getId(), &obj);
		} catch (...)
		{
		}

		switch (cr)
		{
		case DPCR_BYPASS:
			{
				DbgPrint("SSLFilter::indicateClientCertRequest() dataPartAvailable returned DPCR_BYPASS");
				addException(false);
			}

		default:
			DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned %d (unknown)", cr);
			break;
		}
	}


	return cr;
}


bool SSLFilter::addException(bool useCounter)
{
	DbgPrint("SSLFilter::addException");

	if (m_pSession->isDeletion())
	{
		return false;
	}

	std::string eps = m_pSession->getRemoteEndpointStr();

	if ((eps.find("127.0.0.1") != std::string::npos) &&
		m_tlsDomain.empty())
		return false;

	eps += "_" + m_tlsDomain;

	if (m_flags & FF_SSL_COMPATIBILITY)
	{
		if (!isTlsException(TLS_ALL_CIPHERS))
		{
			addTlsException(TLS_ALL_CIPHERS);
			return false;
		}
	}

	if (m_flags & FF_SSL_INDICATE_EXCEPTIONS)
	{
		PFEvents * pHandler = m_pSession->getParsersEventHandler();
		if (pHandler)
		{
			PFStream * pStream;
			PFObjectImpl obj(OT_SSL_EXCEPTION, 1);

			pStream = obj.getStream(0);
			if (pStream)
			{
				if (!m_tlsDomain.empty())
				{
					pStream->write(m_tlsDomain.c_str(), (tStreamSize)m_tlsDomain.length()+1);
				} else
				{
					std::string hostname = m_pSession->getRemoteEndpointStr();
					size_t pos = hostname.find(":");
					if (pos > 0)
					{
						hostname = hostname.substr(0, pos);
					}
					pStream->write(hostname.c_str(), (tStreamSize)hostname.length()+1);
				}
				pStream->seek(0, FILE_BEGIN);
			}

			obj.setReadOnly(true);

			PF_DATA_PART_CHECK_RESULT cr = DPCR_FILTER;
	
			try {
				DbgPrint("SSLFilter::addException() dataPartAvailable, type=%d", obj.getType());
				cr = pHandler->dataPartAvailable(m_pSession->getId(), &obj);
			} catch (...)
			{
			}

			switch (cr)
			{
			case DPCR_BLOCK:
				{
					DbgPrint("SSLFilter::addException() dataPartAvailable returned STATE_BLOCK");
					return false;
				}

			default:
				DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned %d (unknown)", cr);
				break;
			}
		}
	}


	Settings::get().getSSLDataProvider()->addException(eps.c_str(), useCounter);

	return true;
}

bool SSLFilter::isException()
{
	std::string eps = m_pSession->getRemoteEndpointStr();
	eps += "_" + m_tlsDomain;
	
	return Settings::get().getSSLDataProvider()->isException(eps.c_str());
}

void SSLFilter::clearExceptions()
{
	std::string eps = m_pSession->getRemoteEndpointStr();
	eps += "_" + m_tlsDomain;

	Settings::get().getSSLDataProvider()->clearExceptions(eps.c_str());
}

void SSLFilter::clearTlsExceptions(const char * type)
{
	std::string eps = m_pSession->getRemoteEndpointStr();
	eps += "_" + m_tlsDomain;

	if (type)
	{
		eps += "_";
		eps += type;
	}

	Settings::get().getSSLDataProvider()->clearTlsExceptions(eps.c_str());
}

void SSLFilter::addTlsException(const char * type)
{
	DbgPrint("SSLFilter::addTlsException");

	if (m_pSession->isDeletion())
	{
		return;
	}

	std::string eps = m_pSession->getRemoteEndpointStr();

	if ((eps.find("127.0.0.1") != std::string::npos) &&
		m_tlsDomain.empty())
		return;

	eps += "_" + m_tlsDomain;

	if (type)
	{
		eps += "_";
		eps += type;
	}

	Settings::get().getSSLDataProvider()->addTlsException(eps.c_str());
}

bool SSLFilter::isTlsException(const char * type)
{
	std::string eps = m_pSession->getRemoteEndpointStr();

	eps += "_" + m_tlsDomain;
	
	if (type)
	{
		eps += "_";
		eps += type;
	}

	return Settings::get().getSSLDataProvider()->isTlsException(eps.c_str());
}

void SSLFilter::confirmTlsException(const char * type)
{
	std::string eps = m_pSession->getRemoteEndpointStr();

	eps += "_" + m_tlsDomain;
	
	if (type)
	{
		eps += "_";
		eps += type;
	}

	Settings::get().getSSLDataProvider()->confirmTlsException(eps.c_str());
}

bool SSLFilter::tryNextPackets()
{
	if (!(m_flags & FF_SSL_TLS_AUTO))
		return false;

	m_bTls = true;
	return m_bytesTransmitted < DEFAULT_BUFFER_SIZE;
}

#ifdef WIN32

class ImpersonateProcess
{
public:
	ImpersonateProcess(DWORD processId)
	{
		HANDLE hToken = NULL;
		HANDLE hProcess = NULL;

		m_error = false;

		hProcess = OpenProcess(MAXIMUM_ALLOWED, FALSE, processId);
		if (hProcess)
		{
			if (OpenProcessToken(hProcess, MAXIMUM_ALLOWED, &hToken) && hToken)
			{
				HANDLE hPrimaryToken = NULL;

				if (DuplicateTokenEx(hToken, 
							 MAXIMUM_ALLOWED,
							 0, 
							 SecurityIdentification, 
							 TokenPrimary, 
							 &hPrimaryToken) && hPrimaryToken)
				{
					if (!ImpersonateLoggedOnUser(hPrimaryToken))
					{
						m_error = true;
					}

					m_hToken.Attach(hPrimaryToken);
				} else
				{
					m_error = true;
				}

				CloseHandle(hToken);
			} else
			{
				m_error = true;
			}

			CloseHandle(hProcess);
		} else
		{
			m_error = true;
		}
	}
	~ImpersonateProcess()
	{
		RevertToSelf();
	}
	
	bool m_error;
	AutoHandle m_hToken;
};

int dump_certs_pkeys_bags (STACK_OF(PKCS12_SAFEBAG) *bags, char *pass, int passlen, X509 ** x, EVP_PKEY ** pkey);
int dump_certs_pkeys_bag (PKCS12_SAFEBAG *bag, char *pass, int passlen, X509 ** x, EVP_PKEY ** pkey);

static int dump_certs_keys_p12 (PKCS12 *p12, char *pass, int passlen, X509 ** x, EVP_PKEY ** pkey)
{
	STACK_OF(PKCS7) *asafes = NULL;
	STACK_OF(PKCS12_SAFEBAG) *bags;
	int i, bagnid;
	int ret = 0;
	PKCS7 *p7;

	if (!( asafes = PKCS12_unpack_authsafes(p12))) 
		return 0;
	
	for (i = 0; i < sk_PKCS7_num (asafes); i++) 
	{
		p7 = sk_PKCS7_value (asafes, i);
	
		bagnid = OBJ_obj2nid (p7->type);

		if (bagnid == NID_pkcs7_data) 
		{
			bags = PKCS12_unpack_p7data(p7);
		} else 
		if (bagnid == NID_pkcs7_encrypted) 
		{
			bags = PKCS12_unpack_p7encdata(p7, pass, passlen);
		} else 
			continue;
		
		if (!bags) 
			goto err;
	    	
		if (!dump_certs_pkeys_bags (bags, pass, passlen, x, pkey)) 
		{
			sk_PKCS12_SAFEBAG_pop_free (bags, PKCS12_SAFEBAG_free);
			goto err;
		}
		
		sk_PKCS12_SAFEBAG_pop_free (bags, PKCS12_SAFEBAG_free);
		
		bags = NULL;
	}
	
	ret = 1;

	err:

	if (asafes)
		sk_PKCS7_pop_free (asafes, PKCS7_free);
	
	return ret;
}

static int dump_certs_pkeys_bags (STACK_OF(PKCS12_SAFEBAG) *bags, char *pass, int passlen, X509 ** x, EVP_PKEY ** pkey)
{
	int i;
	for (i = 0; i < sk_PKCS12_SAFEBAG_num (bags); i++) 
	{
		if (!dump_certs_pkeys_bag(sk_PKCS12_SAFEBAG_value (bags, i), pass, passlen, x, pkey))
		    return 0;
	}
	return 1;
}

static int dump_certs_pkeys_bag (PKCS12_SAFEBAG *bag, char *pass, int passlen, X509 ** x, EVP_PKEY ** pkey)
{
	EVP_PKEY *_pkey;
	PKCS8_PRIV_KEY_INFO *p8;
	X509 *x509;
	
	switch (M_PKCS12_bag_type(bag))
	{
	case NID_keyBag:
		p8 = (PKCS8_PRIV_KEY_INFO *)PKCS12_SAFEBAG_get0_p8inf(bag);
		if (!(_pkey = EVP_PKCS82PKEY (p8))) 
			return 0;

		if (*pkey)
			EVP_PKEY_free(_pkey);
		else
			*pkey = _pkey;
	break;

	case NID_pkcs8ShroudedKeyBag:
		if (!(p8 = PKCS12_decrypt_skey(bag, pass, passlen)))
				return 0;
		if (!(_pkey = EVP_PKCS82PKEY (p8))) 
		{
			PKCS8_PRIV_KEY_INFO_free(p8);
			return 0;
		}
		
		PKCS8_PRIV_KEY_INFO_free(p8);

		if (*pkey)
			EVP_PKEY_free(_pkey);
		else
			*pkey = _pkey;
	break;

	case NID_certBag:
		if (M_PKCS12_cert_bag_type(bag) != NID_x509Certificate )
			return 1;
		
		if (!(x509 = PKCS12_certbag2x509(bag))) 
			return 0;
		
		if (*x)
			X509_free(x509);
		else
			*x = x509;
	break;

	case NID_safeContentsBag:
		return dump_certs_pkeys_bags ((STACK_OF(PKCS12_SAFEBAG) *)PKCS12_SAFEBAG_get0_safes(bag), pass, passlen, x, pkey);
					
	default:
		return 1;
	break;
	}
	return 1;
}


static int readPKS12Buffer(char * buf, int len, X509 ** x, EVP_PKEY ** pkey)
{
	BIO *	in = NULL;
	int		ret = 0;
    PKCS12 * p12 = NULL;

	in = BIO_new(BIO_s_mem());
	if (!in)
		goto end;

	BIO_write(in, buf, len);

    if (!(p12 = d2i_PKCS12_bio(in, NULL))) 
	{
		goto end;
	}

	ret = dump_certs_keys_p12(p12, "1", 1, x, pkey);

	if (!*x || !*pkey)
		ret = 0;

end:
	if (in != NULL) 
		BIO_free(in);

	if (p12) 
		PKCS12_free(p12);

	return (ret);
}



#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

static bool findCertificate(
	void*			szCert, 
	DWORD			certLen,
	X509 ** x,
	EVP_PKEY ** pkey)
{
    PCCERT_CONTEXT			 pClientCert = NULL;
    PCCERT_CONTEXT			 pCert = NULL;
	wchar_t					 szwStore[] = L"MY";
    bool					 result = false;
	HCERTSTORE				 hStore = NULL;
	HCERTSTORE				 hSysStore = NULL;
	BOOL					 bResult = FALSE;
	PCCERT_CONTEXT			 pTempCert = NULL;
	HCERTSTORE				 hTempStore = NULL;

	*x = NULL;
	*pkey = NULL;

     // Open Certificate Store
     hStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                      ENCODING,
                      0,
                      CERT_SYSTEM_STORE_CURRENT_USER,
                      (LPVOID)szwStore);
     if (!hStore)
     {
		 goto cleanup;
     }

	 hSysStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                      ENCODING,
                      0,
                      CERT_SYSTEM_STORE_CURRENT_USER,
                      (LPVOID)szwStore);
     if (!hSysStore)
     {
		 goto cleanup;
     }

	 // Add Certificate to store
     bResult = CertAddEncodedCertificateToStore(hStore,
                                     X509_ASN_ENCODING,
                                     (LPBYTE)szCert,
                                     certLen,
                                     CERT_STORE_ADD_REPLACE_EXISTING,
                                     &pClientCert);
     if (!bResult || !pClientCert)
     {
		 goto cleanup;
     }
    
	pCert = CertFindCertificateInStore(hSysStore,
		ENCODING,
		0,
		CERT_FIND_EXISTING,
		pClientCert,
		pCert);

	if (pCert)
	{
		 hTempStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
						  ENCODING,
						  0,
						  CERT_SYSTEM_STORE_CURRENT_USER,
						  (LPVOID)szwStore);
		 if (!hTempStore)
		 {
			 goto cleanup;
		 }

		 bResult = CertAddCertificateContextToStore(hTempStore,
										 pCert,
										 CERT_STORE_ADD_REPLACE_EXISTING,
										 &pTempCert);
		 if (!bResult)
		 {
			 goto cleanup;
		 }

   
		CRYPT_DATA_BLOB pfx;
		memset(&pfx, 0, sizeof(pfx));

		PFXExportCertStoreEx(hTempStore, &pfx, L"1", NULL, EXPORT_PRIVATE_KEYS);
		if (pfx.cbData > 0)
		{
			pfx.pbData = (BYTE*)malloc(pfx.cbData);
			if (!PFXExportCertStoreEx(hTempStore, &pfx, L"1", NULL, EXPORT_PRIVATE_KEYS))
			{
				free(pfx.pbData);
				pfx.pbData = NULL;
				pfx.cbData = 0;
			}
		}

		if (pfx.cbData && pfx.pbData)
		{
			if (readPKS12Buffer((char*)pfx.pbData, pfx.cbData, x, pkey))
			{
				result = true;
			} else
			{
				if (*x)
				{
					X509_free(*x);
					*x = NULL;
				}
				if (*pkey)
				{
					EVP_PKEY_free(*pkey);
					*pkey = NULL;
				}
			}
			free(pfx.pbData);
		}

	}

cleanup:

	if (pClientCert)
	{
		CertFreeCertificateContext(pClientCert);
	}
	if (pCert)
	{
		CertFreeCertificateContext(pCert);
	}

    if (hStore) 
	{
		CertCloseStore(hStore, 0);
	}

    if (hSysStore) 
	{
		CertCloseStore(hSysStore, 0);
	}
	if (pTempCert)
	{
		CertFreeCertificateContext(pTempCert);
	}
    if (hTempStore) 
	{
		CertCloseStore(hTempStore, 0);
	}

	return result;
}

#endif

static int alpn_cb(SSL *s, const unsigned char **out, unsigned char *outlen,
                   const unsigned char *in, unsigned int inlen, void *arg)
{
	SSLFilter * pFilter = (SSLFilter*)arg;

	if (pFilter)
	{
		pFilter->setALPNProtos(in, inlen);
#if defined(_DEBUG) || defined(_RELEASE_LOG)
		DbgPrint("ALPN protocols:");
        for (unsigned int i = 0; i < inlen;) 
		{
			std::string s((char*)&in[i + 1], in[i]);
			DbgPrint("proto: %s", s.c_str());
            i += in[i] + 1;
        }
#endif
	}

    if (SSL_select_next_proto
        ((unsigned char **)out, outlen, in, inlen, in,
         inlen) != OPENSSL_NPN_NEGOTIATED) 
	{
        return SSL_TLSEXT_ERR_NOACK;
    }

    return SSL_TLSEXT_ERR_OK;
}

static int alpn_server_cb(SSL *s, const unsigned char **out, unsigned char *outlen,
                   const unsigned char *in, unsigned int inlen, void *arg)
{
	SSLFilter * pFilter = (SSLFilter*)arg;

	if (pFilter && (pFilter->getALPNProtosSelected().size() > 0))
	{
		*out = (unsigned char*)pFilter->getALPNProtosSelected().buffer();
		*outlen = (unsigned char)pFilter->getALPNProtosSelected().size();
	}

    return SSL_TLSEXT_ERR_OK;
}

eFilterResult SSLFilter::tcp_packet(eDataDirection dd,
					ePacketDirection pd,
					const char * buf, int len)
{
	char tempBuf[DEFAULT_BUFFER_SIZE];
	int offset = 0;
	ePacketDirection pdReal = pd;
	ePacketDirection pdReverse = (pd == PD_SEND)? PD_RECEIVE : PD_SEND;
	int n;

	DbgPrint("SSLFilter::tcp_packet() id=%llu dd=%d pd=%d m_state=%d len=%d",
				m_pSession->getId(), dd, pd, m_state, len);

	if (m_si.tcpConnInfo.direction != NFAPI_NS NF_D_OUT)
	{
		if (m_state == SFS_NONE && pd == PD_RECEIVE)
			return tryNextPackets()? FR_PASSTHROUGH : FR_DELETE_ME;
	}

	if (dd == DD_OUTPUT)
	{
		if (m_state != SFS_DATA_EXCHANGE)
		{
			m_pSession->tcp_packet(this, DD_OUTPUT, pd, buf, len);
			return FR_DATA_EATEN;
		}

		if (len == 0)
		{
			bool messageSent = false;

			if (pd == PD_RECEIVE)
			{
				if (m_sdLocal.m_pSSL)
				{
					n = SSL_shutdown(m_sdLocal.m_pSSL);
					
					while (BIO_pending(m_sdLocal.m_pbioWrite))
					{
						n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
						if (n > 0)
						{
							m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, tempBuf, n);
							messageSent = true;
						}
					}
				}
			} else
			{
				if (m_sdRemote.m_pSSL)
				{
					n = SSL_shutdown(m_sdRemote.m_pSSL);

					while (BIO_pending(m_sdRemote.m_pbioWrite))
					{
						n = BIO_read(m_sdRemote.m_pbioWrite, tempBuf, sizeof(tempBuf));
						if (n > 0)
						{
							m_pSession->tcp_packet(this, DD_OUTPUT, PD_SEND, tempBuf, n);
							messageSent = true;
						}
					}
				}
			}

			if (!messageSent)
				m_pSession->tcp_packet(this, DD_OUTPUT, pdReal, NULL, 0);

			return FR_DATA_EATEN;
		}

		if (pd == PD_RECEIVE)
		{
			while (offset < len)
			{
				n = SSL_write(m_sdLocal.m_pSSL, buf + offset, len - offset);
				
				if (n <= 0)
					break;

				offset += n;

				while (BIO_pending(m_sdLocal.m_pbioWrite))
				{
					n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, tempBuf, n);
					}
				}
			}
		} else
		{
			if (m_flags & FF_SSL_DECODE_ONLY)
			{
				m_pSession->tcp_packet(this, DD_OUTPUT, pd, buf, len);
				return FR_DATA_EATEN;
			}

			while (offset < len)
			{
				n = SSL_write(m_sdRemote.m_pSSL, buf + offset, len - offset);
				if (n <= 0)
					break;

				offset += n;

				while (BIO_pending(m_sdRemote.m_pbioWrite))
				{
					n = BIO_read(m_sdRemote.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, PD_SEND, tempBuf, n);
					}
				}
			}
		}
		return FR_DATA_EATEN;
	}

	switch (m_state)
	{
	case SFS_NONE:
		{
			m_bytesTransmitted += len;

			if (len < 10)
			{
				return tryNextPackets()? FR_PASSTHROUGH : FR_DELETE_ME;
			}

			if (pd == PD_RECEIVE)
			{
				return tryNextPackets()? FR_PASSTHROUGH : FR_DELETE_ME;
			}

			SSLSessionData sdTemp;
			SSLKeys tempKeys;

			if (!Settings::get().getSSLDataProvider()->getSelfSignedCert("test", NULL, false, false, tempKeys, 0))
			{
				DbgPrint("SSLFilter::tcp_packet() getSelfSignedCert(test) failed");
				return FR_DELETE_ME;
			}

			if (!sdTemp.init(m_bTls, true, &tempKeys))
			{
				DbgPrint("SSLFilter::tcp_packet() sdTemp.init failed");
				return FR_DELETE_ME;
			}

			if (m_flags & FF_SSL_ENABLE_ALPN)
			{
				// Set ALPN callback
				SSL_CTX_set_alpn_select_cb(sdTemp.m_pCtx, alpn_cb, this);
			}

			BIO_write(sdTemp.m_pbioRead, buf, len);

			n = SSL_accept(sdTemp.m_pSSL);
			if (n < 0)
			{
				int err = SSL_get_error(sdTemp.m_pSSL, n);
				if ((err != SSL_ERROR_WANT_READ) || (len < 11))
				{
					DbgPrint("SSLFilter::tcp_packet() SSL_accept failed, err=%d", err);
					return tryNextPackets()? FR_PASSTHROUGH : FR_DELETE_ME;
				}
			}

			if (SSL_get_servername(sdTemp.m_pSSL, TLSEXT_NAMETYPE_host_name))
			{
				m_tlsDomain = SSL_get_servername(sdTemp.m_pSSL, TLSEXT_NAMETYPE_host_name);

				DbgPrint("SSLFilter::tcp_packet() tlsDomain = %s", m_tlsDomain.c_str());
			}

			// Check the server IP:port, bypass exceptions
			if (isException())
			{
				DbgPrint("SSLFilter::tcp_packet() bypass exception");
				return FR_DELETE_ME;
			}

			if (m_flags & FF_SSL_INDICATE_HANDSHAKE_REQUESTS)
			{
				// Indicate SNI if available, as read-only object
				PFEvents * pHandler = m_pSession->getParsersEventHandler();
				if (pHandler)
				{
					PFObjectImpl obj((pd == PD_SEND)? OT_SSL_HANDSHAKE_OUTGOING : OT_SSL_HANDSHAKE_INCOMING, 1);
					PFStream * pStream = obj.getStream(0);

					if (pStream)
					{
						if (!m_tlsDomain.empty())
						{
							pStream->write(m_tlsDomain.c_str(), (tStreamSize)m_tlsDomain.length()+1);
						} else
						{
							std::string hostname = m_pSession->getRemoteEndpointStr();
							size_t pos = hostname.find(":");
							if (pos > 0)
							{
								hostname = hostname.substr(0, pos);
							}
							pStream->write(hostname.c_str(), (tStreamSize)hostname.length()+1);
						}
						pStream->seek(0, FILE_BEGIN);
					}

					obj.setReadOnly(true);

					PF_DATA_PART_CHECK_RESULT cr = DPCR_FILTER;
			
					try {
						DbgPrint("SSLFilter::tcp_packet() dataPartAvailable, type=%d", obj.getType());
						cr = pHandler->dataPartAvailable(m_pSession->getId(), &obj);
					} catch (...)
					{
					}

					switch (cr)
					{
					case DPCR_FILTER:
						{
							DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_FILTER");
						}
						break;
					
					case DPCR_BYPASS:
						{
							DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_BYPASS");
							return FR_DELETE_ME;
						}
						break;

					case DPCR_BLOCK:
						{
							DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned STATE_BLOCK");
							return FR_ERROR;
						}
						break;

					case DPCR_FILTER_READ_ONLY:
						{
							m_flags |= FF_SSL_DECODE_ONLY;
						}
						break;

					default:
						DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned %d (unknown)", cr);
						break;
					}
				}
			}

			DbgPrint("SSL version %x", SSL_version(sdTemp.m_pSSL));

			if (m_flags & FF_SSL_DECODE_ONLY)
			{
				DbgPrint("SSLFilter::tcp_packet() decode only path");

				m_tempBufferLocal.reset();
				if (!m_tempBufferLocal.append(buf, len))
					return FR_DELETE_ME;

				setActive(true);

				std::string domainname;

				if (!m_tlsDomain.empty())
				{
					domainname = m_tlsDomain;
				} else
				{
					std::string hostname = m_pSession->getRemoteEndpointStr();
					size_t pos = hostname.find(":");
					if (pos > 0)
					{
						domainname = hostname.substr(0, pos);
					}
				}

				DbgPrint("SSLFilter::tcp_packet() host = %s", domainname.c_str());
	
				tempKeys.m_pkey.reset();
				tempKeys.m_x509.reset();

				if (!Settings::get().getSSLDataProvider()->getSignedCert(domainname.c_str(), NULL, tempKeys, m_flags))
				{
					return FR_ERROR;
				} 

				// Continue SSL handshake with local endpoint
				if (!m_sdLocal.init(m_bTls, true, &tempKeys))
					return FR_ERROR;

				n = BIO_write(m_sdLocal.m_pbioRead, m_tempBufferLocal.buffer(), m_tempBufferLocal.size());
				if (n < 0)
					return FR_ERROR;

				n = SSL_accept(m_sdLocal.m_pSSL);
				if (n < 0)
				{
					if (SSL_get_error(m_sdLocal.m_pSSL, n) != SSL_ERROR_WANT_READ)
					{
						return FR_ERROR;
					}
				}

				while (BIO_pending(m_sdLocal.m_pbioWrite))
				{
					n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReverse, tempBuf, n);
					}
				}

				// Continue SSL handshake with local endpoint
				m_state = SFS_SERVER_HANDSHAKE;

				return FR_DATA_EATEN;
			}

			int sslVersion = SSL_version(sdTemp.m_pSSL);

			if (sslVersion != 0x300 &&
				sslVersion != 0x200)
			{
				if (!isTlsException())
				{
					m_bTls = true;

					if (isTlsException(TLS_1_0_PROTOCOL))
					{
						sslVersion = 0x301;
					} else
					{
						sslVersion = 0x304;
					}
				}
			}

			if (!m_sdRemote.init(m_bTls, false, NULL, sslVersion, m_flags))
			{
				return FR_DELETE_ME;
			} 

			if (m_assocSSLSession)
			{
				SSL_set_session(m_sdRemote.m_pSSL, m_assocSSLSession);
			}

			if (m_alpnProtos.size() > 0)
			{
				SSL_set_alpn_protos(m_sdRemote.m_pSSL, (unsigned char*)m_alpnProtos.buffer(), m_alpnProtos.size());
			}

#ifdef WIN32
			{
				{
					AutoLock lock(g_ctxMapCS);
					g_ctxMap[m_sdRemote.m_pSSL] = this;
				}

				SSL_CTX_set_client_cert_cb(m_sdRemote.m_pCtx, client_cert_cb);

				if (m_flags & FF_SSL_VERIFY)
				{
					// Add OCSP stapling TLS extension
					SSL_set_tlsext_status_type(m_sdRemote.m_pSSL, TLSEXT_STATUSTYPE_ocsp);
					SSL_CTX_set_tlsext_status_cb(m_sdRemote.m_pCtx, ocsp_resp_cb);
					SSL_CTX_set_tlsext_status_arg(m_sdRemote.m_pCtx, &m_ocspEncodedResponse);
				}
			}
#endif

			if (!m_tlsDomain.empty())
			{
				SSL_set_tlsext_host_name(m_sdRemote.m_pSSL, m_tlsDomain.c_str());
			}

			std::string cipherList;

			if (SSL_get_session(sdTemp.m_pSSL) != NULL &&
				SSL_get_ciphers(sdTemp.m_pSSL) != NULL)
			{
				const char * cipher;
				SSL_CIPHER *c;
				STACK_OF(SSL_CIPHER) *sk;

				sk = SSL_get_ciphers(sdTemp.m_pSSL);

				for (int i=0; i < sk_SSL_CIPHER_num(sk); i++)
				{
					 c = (SSL_CIPHER *)sk_SSL_CIPHER_value(sk, i);
					 if (c == NULL)
						 break;
					 cipher = SSL_CIPHER_get_name(c);
					 if (!cipher)
						 break;
					 if (!cipherList.empty())
	 					 cipherList += ":";
					 cipherList += cipher;
				}
			}

			if ((m_flags & FF_SSL_COMPATIBILITY) &&
				!isTlsException(TLS_ALL_CIPHERS))
			{
				SSL_set_cipher_list(m_sdRemote.m_pSSL, "RC4");
				m_compatibility = true;
			} else
			if (!cipherList.empty())
			{
				SSL_set_cipher_list(m_sdRemote.m_pSSL, (cipherList+":AES:SEED:DES-CBC3-SHA:+ALL:!aNULL:!eNULL:!EXP-RC4-MD5").c_str());
			} else
			{
				SSL_set_cipher_list(m_sdRemote.m_pSSL, "AES:SEED:DES-CBC3-SHA:+ALL:!aNULL:!eNULL:!EXP-RC4-MD5");
			}

			m_tempBufferLocal.reset();
			if (!m_tempBufferLocal.append(buf, len))
				return FR_DELETE_ME;

			n = SSL_connect(m_sdRemote.m_pSSL);
			if (n < 0)
			{
				int err = SSL_get_error(m_sdRemote.m_pSSL, n);

				if (err != SSL_ERROR_WANT_READ)
				{
					return FR_DELETE_ME;
				}
			}

			while (BIO_pending(m_sdRemote.m_pbioWrite))
			{
				n = BIO_read(m_sdRemote.m_pbioWrite, tempBuf, sizeof(tempBuf));
				if (n > 0)
				{
					m_pSession->tcp_packet(this, DD_OUTPUT, pdReal, tempBuf, n);
				}
			}

			m_state = SFS_CLIENT_HANDSHAKE;

			setActive(true);

			return FR_DATA_EATEN;
		}
		break;
	
	case SFS_CLIENT_HANDSHAKE:
		{
			// Return error on disconnect 
			if (len == 0)
			{
				if (pd == PD_RECEIVE)
				{
					if (m_flags & FF_SSL_COMPATIBILITY)
					{
						if (!isTlsException(TLS_ALL_CIPHERS))
						{
							addTlsException(TLS_ALL_CIPHERS);
							return FR_ERROR;
						}
					}

					// Try using TLS 1.2 protocol in case if the server rejects TLS 1.0 request
					if (m_bTls && (SSL_version(m_sdRemote.m_pSSL) > 0x300))
					{
						if (isTlsException(TLS_1_0_PROTOCOL))
						{
//							clearTlsExceptions(TLS_1_0_PROTOCOL);
						} else
						if (!m_compatibility && !isTlsException(TLS_1_0_PROTOCOL))
						{
							if (m_flags & FF_SSL_DISABLE_TLS_1_0)
							{
								addException();
							} else
							{
								addTlsException(TLS_1_0_PROTOCOL);
							}
						}
					}
				}

				return FR_ERROR;
			}
			
			if (m_serverHandshake)
			{
				pdReverse = pd;
				pd = (pd == PD_SEND)? PD_RECEIVE : PD_SEND;
			}

			if (pd == PD_SEND)
			{
				m_tempBufferLocal.append(buf, len);
				return FR_DATA_EATEN;
			}

			// Connect to remote endpoint
			if (!SSL_is_init_finished(m_sdRemote.m_pSSL))
			{
				BIO_write(m_sdRemote.m_pbioRead, buf, len);
	
				n = SSL_connect(m_sdRemote.m_pSSL);
				if (n < 0)
				{
					int err = SSL_get_error(m_sdRemote.m_pSSL, n);
					
					if (err != SSL_ERROR_WANT_READ &&
						err != SSL_X509_LOOKUP)
					{
						if (ERR_GET_REASON(ERR_get_error()) == SSL_R_DH_KEY_TOO_SMALL)
						{
							DbgPrint("SSLFilter::tcp_packet() Weak DH prime, do not filter such connections [1]");
							addException(false);
							return FR_ERROR;
						}

						if (m_bTls)
						{
							if (m_flags & FF_SSL_COMPATIBILITY)
							{
								if (!isTlsException(TLS_ALL_CIPHERS))
								{
									addTlsException(TLS_ALL_CIPHERS);
									return FR_ERROR;
								}
							}

							if (SSL_version(m_sdRemote.m_pSSL) == 0x300 || SSL_version(m_sdRemote.m_pSSL) == 0x200)
							{
								addTlsException();
							} else
							{
								if (m_flags & FF_SSL_DISABLE_TLS_1_0)
								{
									addException();
								} else
								if (isTlsException(TLS_1_0_PROTOCOL))
								{
									addException(false);
								} else
								{
									addTlsException(TLS_1_0_PROTOCOL);
								}
							}
						}
						return FR_ERROR;
					}
				}

				if (n == 0)
				{
					// Controlled shutdown
					SSL_shutdown(m_sdRemote.m_pSSL);
					// Do not try to decrypt this connection anymore 
					addException();
				}

				if (m_flags & FF_SSL_VERIFY)
				{
					EVP_PKEY *key;
					if (SSL_get_server_tmp_key(m_sdRemote.m_pSSL, &key))
					{
						if (EVP_PKEY_id(key) == EVP_PKEY_DH) 
						{
							int nBits = EVP_PKEY_bits(key);
							if (nBits < 1024)
							{
								DbgPrint("SSLFilter::tcp_packet() Weak DH prime, do not filter such connections [2]");
								if (addException(false))
								{
									EVP_PKEY_free(key);
									return FR_ERROR;
								}
							}
						}

						EVP_PKEY_free(key);
					}
				}

				while (BIO_pending(m_sdRemote.m_pbioWrite))
				{
					n = BIO_read(m_sdRemote.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReverse, tempBuf, n);
					}
				}

				if (!SSL_is_init_finished(m_sdRemote.m_pSSL))
				{
#ifdef WIN32
					if (m_requestClientCert)
					{
						X509 *peer = NULL;

						if (peer = SSL_get_peer_certificate(m_sdRemote.m_pSSL))
						{
							if (!m_tlsDomain.empty())
							{
								_snprintf(tempBuf, sizeof(tempBuf), "%s", m_tlsDomain.c_str());
							} else
							{
								if (X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
										NID_commonName,	tempBuf, sizeof(tempBuf)) == -1)
								{
									std::string hostname = m_pSession->getRemoteEndpointStr();
									size_t pos = hostname.find(":");
									if (pos > 0)
									{
										hostname = hostname.substr(0, pos);
									}
									_snprintf(tempBuf, sizeof(tempBuf), "%s", hostname.c_str());
								}
							}

							{
								char subject[256] = {0};
								char issuer[256] = {0};
							
								if (X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
										NID_commonName,	subject, sizeof(subject)) != -1)
								{
									if (X509_NAME_get_text_by_NID(X509_get_issuer_name(peer),
											NID_commonName,	issuer, sizeof(issuer)) != -1)
									{
										if (stricmp(subject, issuer) == 0)
											m_flags |= FF_SSL_SELF_SIGNED_CERTIFICATE;
									}
								}

								if (m_flags & FF_SSL_INDICATE_SERVER_CERTIFICATES)
								{
									PFEvents * pHandler = m_pSession->getParsersEventHandler();
									if (pHandler)
									{
										PFStream * pStream;
										PFObjectImpl obj(OT_SSL_SERVER_CERTIFICATE, 3);

										pStream = obj.getStream(SSL_SCS_CERTIFICATE);
										if (pStream)
										{
											int n;
											IOB buf;

											n = i2d_X509(peer, NULL);
											buf.reset();
											if (buf.resize(n))
											{
												unsigned char * p = (unsigned char *)buf.buffer();
												i2d_X509(peer, &p);

												pStream->write(buf.buffer(), (tStreamSize)buf.size());
												pStream->seek(0, FILE_BEGIN);
											}
										}

										pStream = obj.getStream(SSL_SCS_SUBJECT);
										if (pStream && subject[0])
										{
											pStream->write(subject, (tStreamSize)strlen(subject)+1);
											pStream->seek(0, FILE_BEGIN);
										}
										pStream = obj.getStream(SSL_SCS_ISSUER);
										if (pStream && issuer[0])
										{
											pStream->write(issuer, (tStreamSize)strlen(issuer)+1);
											pStream->seek(0, FILE_BEGIN);
										}

										obj.setReadOnly(true);

										PF_DATA_PART_CHECK_RESULT cr = DPCR_FILTER;
								
										try {
											DbgPrint("SSLFilter::tcp_packet() dataPartAvailable, type=%d", obj.getType());
											cr = pHandler->dataPartAvailable(m_pSession->getId(), &obj);
										} catch (...)
										{
										}

										switch (cr)
										{
										case DPCR_FILTER:
											{
												DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_FILTER");
											}
											break;
										
										case DPCR_BYPASS:
											{
												DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_BYPASS");
												addException(false);
												X509_free(peer);
												return FR_ERROR;
											}
											break;

										case DPCR_BLOCK:
											{
												DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned STATE_BLOCK");
												X509_free(peer);
												return FR_ERROR;
											}
											break;

										default:
											DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned %d (unknown)", cr);
											break;
										}
									}
								}
							}

							SSLKeys tempKeys;

							if (m_flags & FF_SSL_VERIFY)
							{
								unsigned __int64 curTime;
								unsigned __int64 certTime;
								bool fastCheck = true;

#ifdef WIN32
								curTime = _time64(NULL);
#else
								curTime = time(NULL);
#endif
								if (Settings::get().getSSLDataProvider()->getCertLastVerifyTime(tempBuf, peer, certTime))
								{
									if ((curTime - certTime) > VERIFY_TIMEOUT)
									{
										fastCheck = false;
									}
								} else
								{
									fastCheck = false;
								}

								{
									STACK_OF(X509) * pChain = SSL_get_peer_cert_chain(m_sdRemote.m_pSSL);

									if (!fastCheck)
									{
										Settings::get().getSSLDataProvider()->setCertLastVerifyTime(tempBuf, peer, curTime);
									}

									if (pChain)
									{
										DWORD res = Settings::get().getSSLDataProvider()->verifyCertificate(m_pSession->getId(), tempBuf, m_ocspEncodedResponse, peer, pChain, fastCheck);
										if (res != SEC_E_OK)
										{
											DbgPrint("Certificate %s is not trusted (err=%x)", tempBuf, res);

											PFEvents * pHandler = m_pSession->getParsersEventHandler();
											if (pHandler)
											{
												PFObjectImpl obj(OT_SSL_INVALID_SERVER_CERTIFICATE, 1);
												PFStream * pStream = obj.getStream(0);

												char *peerName = X509_NAME_oneline(X509_get_subject_name(peer), NULL, 0);
												if (pStream && peerName)
												{
													pStream->write(peerName, (tStreamSize)strlen(peerName)+1);
													pStream->seek(0, FILE_BEGIN);
												}

												obj.setReadOnly(true);

												PF_DATA_PART_CHECK_RESULT cr = DPCR_FILTER;
										
												try {
													DbgPrint("SSLFilter::tcp_packet() dataPartAvailable, type=%d", obj.getType());
													cr = pHandler->dataPartAvailable(m_pSession->getId(), &obj);
												} catch (...)
												{
												}

												switch (cr)
												{
												case DPCR_FILTER:
													{
														DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_FILTER");
													}
													break;
												
												case DPCR_BYPASS:
													{
														DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_BYPASS");
														addException(false);
														X509_free(peer);
														return FR_ERROR;
													}
													break;

												case DPCR_BLOCK:
													{
														DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned STATE_BLOCK");
														X509_free(peer);
														return FR_ERROR;
													}
													break;

												default:
													DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned %d (unknown)", cr);
													break;
												}
											}
										}
									}
								}
							}

							if (m_flags & FF_SSL_SELF_SIGNED_CERTIFICATE)
							{
								// Generate self-signed certificate with the server name in properties
								if (!Settings::get().getSSLDataProvider()->getSelfSignedCert(tempBuf, peer, 
									(m_flags & FF_SSL_DONT_IMPORT_SELF_SIGNED)? false : true, 
									false, tempKeys, m_flags))
								{
									X509_free(peer);
									return FR_ERROR;
								}
							} else
							{
								if (!Settings::get().getSSLDataProvider()->getSignedCert(tempBuf, peer, tempKeys, m_flags))
								{
									X509_free(peer);
									return FR_ERROR;
								} 
							}

							X509_free(peer);
				
							// Continue SSL handshake with local endpoint
							if (!m_sdLocal.init(m_bTls, true, &tempKeys))
								return FR_ERROR;

							if (m_flags & FF_SSL_ENABLE_ALPN)
							{
								const unsigned char *proto;
								unsigned int proto_len = 0;
								
								SSL_get0_alpn_selected(m_sdRemote.m_pSSL, &proto, &proto_len);
								if (proto_len > 0)
								{
									m_alpnProtosSelected.reset();
									m_alpnProtosSelected.append((char*)proto, proto_len);

							#if defined(_DEBUG) || defined(_RELEASE_LOG)
									DbgPrint("SSLFilter::tcp_packet() id=%llu, selected ALPN protocol: %s", 
										m_pSession->getId(),
										m_alpnProtosSelected.buffer());
							#endif
								}

								if (m_alpnProtosSelected.size() > 0)
								{
									// Set ALPN callback
									SSL_CTX_set_alpn_select_cb(m_sdLocal.m_pCtx, alpn_server_cb, this);
								}
							}

							if ((m_flags & FF_SSL_COMPATIBILITY) && !isTlsException(TLS_ALL_CIPHERS))
							{
								SSL_set_cipher_list(m_sdLocal.m_pSSL, "RC4");
								m_compatibility = true;
							} else
							{
			//					SSL_set_cipher_list(m_sdLocal.m_pSSL, "DEFAULT");
							}

							n = BIO_write(m_sdLocal.m_pbioRead, m_tempBufferLocal.buffer(), m_tempBufferLocal.size());
							if (n < 0)
								return FR_ERROR;
	
							SSL_set_verify(m_sdLocal.m_pSSL, SSL_VERIFY_PEER, verify_callback);
	
							n = SSL_accept(m_sdLocal.m_pSSL);
							if (n < 0)
							{
								if (SSL_get_error(m_sdLocal.m_pSSL, n) != SSL_ERROR_WANT_READ)
								{
									return FR_ERROR;
								}
							}

							m_state = SFS_SERVER_HANDSHAKE_REQUEST_CLIENT_CERT;
	
							while (BIO_pending(m_sdLocal.m_pbioWrite))
							{
								n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
								if (n > 0)
								{
									m_pSession->tcp_packet(this, DD_OUTPUT, pdReal, tempBuf, n);
								}
							}

							return FR_DATA_EATEN;
						} else
						{
							return FR_ERROR;
						}
					}
#endif
					return FR_DATA_EATEN;
				}
			}

			// Get the server certificate

			X509 *peer = SSL_get_peer_certificate(m_sdRemote.m_pSSL);

			if (peer)
			{
				if (!m_tlsDomain.empty())
				{
					_snprintf(tempBuf, sizeof(tempBuf), "%s", m_tlsDomain.c_str());
				} else
				{
					if (X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
							NID_commonName,	tempBuf, sizeof(tempBuf)) == -1)
					{
						std::string hostname = m_pSession->getRemoteEndpointStr();
						size_t pos = hostname.find(":");
						if (pos > 0)
						{
							hostname = hostname.substr(0, pos);
						}
						_snprintf(tempBuf, sizeof(tempBuf), "%s", hostname.c_str());
					}
				}

				{
					char subject[256] = {0};
					char issuer[256] = {0};
				
					if (X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
							NID_commonName,	subject, sizeof(subject)) != -1)
					{
						if (X509_NAME_get_text_by_NID(X509_get_issuer_name(peer),
								NID_commonName,	issuer, sizeof(issuer)) != -1)
						{
							if (stricmp(subject, issuer) == 0)
							{
								m_flags |= FF_SSL_SELF_SIGNED_CERTIFICATE;
							}
						}
					}

					if (m_flags & FF_SSL_INDICATE_SERVER_CERTIFICATES)
					{
						PFEvents * pHandler = m_pSession->getParsersEventHandler();
						if (pHandler)
						{
							PFStream * pStream;
							PFObjectImpl obj(OT_SSL_SERVER_CERTIFICATE, 3);

							pStream = obj.getStream(SSL_SCS_CERTIFICATE);
							if (pStream)
							{
								int n;
								IOB buf;

								n = i2d_X509(peer, NULL);
								buf.reset();
								if (buf.resize(n))
								{
									unsigned char * p = (unsigned char *)buf.buffer();
									i2d_X509(peer, &p);

									pStream->write(buf.buffer(), (tStreamSize)buf.size());
									pStream->seek(0, FILE_BEGIN);
								}
							}

							pStream = obj.getStream(SSL_SCS_SUBJECT);
							if (pStream && subject[0])
							{
								pStream->write(subject, (tStreamSize)strlen(subject)+1);
								pStream->seek(0, FILE_BEGIN);
							}
							pStream = obj.getStream(SSL_SCS_ISSUER);
							if (pStream && issuer[0])
							{
								pStream->write(issuer, (tStreamSize)strlen(issuer)+1);
								pStream->seek(0, FILE_BEGIN);
							}

							obj.setReadOnly(true);

							PF_DATA_PART_CHECK_RESULT cr = DPCR_FILTER;
					
							try {
								DbgPrint("SSLFilter::tcp_packet() dataPartAvailable, type=%d", obj.getType());
								cr = pHandler->dataPartAvailable(m_pSession->getId(), &obj);
							} catch (...)
							{
							}

							switch (cr)
							{
							case DPCR_FILTER:
								{
									DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_FILTER");
								}
								break;
							
							case DPCR_BYPASS:
								{
									DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_BYPASS");
									addException(false);
									X509_free(peer);
									return FR_ERROR;
								}
								break;

							case DPCR_BLOCK:
								{
									DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned STATE_BLOCK");
									X509_free(peer);
									return FR_ERROR;
								}
								break;

							default:
								DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned %d (unknown)", cr);
								break;
							}
						}
					}
				}

				SSLKeys tempKeys;

				if (m_flags & FF_SSL_VERIFY)
				{
					unsigned __int64 curTime;
					unsigned __int64 certTime;
					bool fastCheck = true;

#ifdef WIN32
					curTime = _time64(NULL);
#else
					curTime = time(NULL);
#endif

					if (Settings::get().getSSLDataProvider()->getCertLastVerifyTime(tempBuf, peer, certTime))
					{
						if ((curTime - certTime) > VERIFY_TIMEOUT)
						{
							fastCheck = false;
						}
					} else
					{
						fastCheck = false;
					}

					{
						STACK_OF(X509) * pChain = SSL_get_peer_cert_chain(m_sdRemote.m_pSSL);

						if (!fastCheck)
						{
							Settings::get().getSSLDataProvider()->setCertLastVerifyTime(tempBuf, peer, curTime);
						}

						if (pChain)
						{
							DWORD res = Settings::get().getSSLDataProvider()->verifyCertificate(m_pSession->getId(), tempBuf, m_ocspEncodedResponse, peer, pChain, fastCheck);
							if (res != X509_V_OK)
							{
								DbgPrint("Certificate %s is not trusted (err=%x)", tempBuf, res);

								PFEvents * pHandler = m_pSession->getParsersEventHandler();
								if (pHandler)
								{
									PFObjectImpl obj(OT_SSL_INVALID_SERVER_CERTIFICATE, 1);
									PFStream * pStream = obj.getStream(0);

									char *peerName = X509_NAME_oneline(X509_get_subject_name(peer), NULL, 0);
									if (pStream && peerName)
									{
										pStream->write(peerName, (tStreamSize)strlen(peerName)+1);
										pStream->seek(0, FILE_BEGIN);
									}

									obj.setReadOnly(true);

									PF_DATA_PART_CHECK_RESULT cr = DPCR_FILTER;
							
									try {
										DbgPrint("SSLFilter::tcp_packet() dataPartAvailable, type=%d", obj.getType());
										cr = pHandler->dataPartAvailable(m_pSession->getId(), &obj);
									} catch (...)
									{
									}

									switch (cr)
									{
									case DPCR_FILTER:
										{
											DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_FILTER");
										}
										break;
									
									case DPCR_BYPASS:
										{
											DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned DPCR_BYPASS");
											addException(false);
											X509_free(peer);
											return FR_ERROR;
										}
										break;

									case DPCR_BLOCK:
										{
											DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned STATE_BLOCK");
											X509_free(peer);
											return FR_ERROR;
										}
										break;

									default:
										DbgPrint("SSLFilter::tcp_packet() dataPartAvailable returned %d (unknown)", cr);
										break;
									}
								}
							}
						}
					}
				}

				if (m_flags & FF_SSL_SELF_SIGNED_CERTIFICATE)
				{
					// Generate self-signed certificate with the server name in properties
					if (!Settings::get().getSSLDataProvider()->getSelfSignedCert(tempBuf, peer, 
						(m_flags & FF_SSL_DONT_IMPORT_SELF_SIGNED)? false : true, 
						false, tempKeys, m_flags))
					{
						X509_free(peer);
						return FR_ERROR;
					}
				} else
				{
					if (!Settings::get().getSSLDataProvider()->getSignedCert(tempBuf, peer, tempKeys, m_flags))
					{
						X509_free(peer);
						return FR_ERROR;
					} 
				}

				X509_free(peer);

				// Continue SSL handshake with local endpoint
				if (!m_sdLocal.init(m_bTls, true, &tempKeys))
					return FR_ERROR;

				if (m_flags & FF_SSL_ENABLE_ALPN)
				{
					const unsigned char *proto;
					unsigned int proto_len = 0;

					SSL_get0_alpn_selected(m_sdRemote.m_pSSL, &proto, &proto_len);
					if (proto_len > 0)
					{
						m_alpnProtosSelected.reset();
						m_alpnProtosSelected.append((char*)proto, proto_len);

				#if defined(_DEBUG) || defined(_RELEASE_LOG)
						DbgPrint("SSLFilter::tcp_packet() id=%llu, selected ALPN protocol: %s", 
							m_pSession->getId(),
							m_alpnProtosSelected.buffer());
				#endif
					}

					if (m_alpnProtosSelected.size() > 0)
					{
						// Set ALPN callback
						SSL_CTX_set_alpn_select_cb(m_sdLocal.m_pCtx, alpn_server_cb, this);
					}
				}

				// Don't change ciphers for local socket to make the browsers happy

				if ((m_flags & FF_SSL_COMPATIBILITY) && !isTlsException(TLS_ALL_CIPHERS))
				{
					SSL_set_cipher_list(m_sdLocal.m_pSSL, "RC4");
					m_compatibility = true;
				} else
				{
//					SSL_set_cipher_list(m_sdLocal.m_pSSL, "DEFAULT");
				}


				n = BIO_write(m_sdLocal.m_pbioRead, m_tempBufferLocal.buffer(), m_tempBufferLocal.size());
				if (n < 0)
					return FR_ERROR;

				n = SSL_accept(m_sdLocal.m_pSSL);
				if (n < 0)
				{
					if (SSL_get_error(m_sdLocal.m_pSSL, n) != SSL_ERROR_WANT_READ)
					{
						return FR_ERROR;
					}
				}

				while (BIO_pending(m_sdLocal.m_pbioWrite))
				{
					n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReal, tempBuf, n);
					}
				}

				// Read a packet from remote endpoint to temporary buffer
				for (;;)
				{
					n = SSLRead_client(m_sdRemote);
					if (n < 0)
						return FR_ERROR;
								
					if (n == 0)
						break;

					if (n > 0)
					{
						m_tempBufferRemote.append(m_sdRemote.m_buffer.buffer(), n);
					}
				}
			} else
			{
				return FR_ERROR;
			}

			// Continue SSL handshake with local endpoint
			m_state = SFS_SERVER_HANDSHAKE;

			return FR_DATA_EATEN;
		}
		break;

	case SFS_SERVER_HANDSHAKE:
		{
			if (len == 0)
			{
				if (pd == PD_RECEIVE)
				{
					m_remoteDisconnectState++;
				} else
				{
					addException();
					return FR_ERROR;
				}
			}

			if (m_serverHandshake)
			{
				pdReverse = pd;
				pd = (pd == PD_SEND)? PD_RECEIVE : PD_SEND;
			}

			if (len > 0 && pd == PD_RECEIVE)
			{
				if (!(m_flags & FF_SSL_DECODE_ONLY))
				{
					// Read a packet from remote endpoint to temporary buffer
					BIO_write(m_sdRemote.m_pbioRead, buf, len);

					for (;;)
					{
						n = SSLRead_client(m_sdRemote);
						if (n < 0)
							return FR_ERROR;
									
						if (n == 0)
							break;

						if (n > 0)
						{
							m_tempBufferRemote.append(m_sdRemote.m_buffer.buffer(), n);
						}
					} 

					if (SSL_get_shutdown(m_sdRemote.m_pSSL))
					{
						m_remoteDisconnectState++;
					}

					return FR_DATA_EATEN;
				}

				return FR_PASSTHROUGH;
			}

			// Continue SSL handshake
			n = BIO_write(m_sdLocal.m_pbioRead, buf, len);

			if (!SSL_is_init_finished(m_sdLocal.m_pSSL))
			{
				n = SSL_accept(m_sdLocal.m_pSSL);
				if (n < 0)
				{
					if (SSL_get_error(m_sdLocal.m_pSSL, n) != SSL_ERROR_WANT_READ)
					{
						return FR_DELETE_ME;
					}
				} 

				if (n == 0)
				{
					// Controlled shutdown
					SSL_shutdown(m_sdLocal.m_pSSL);
					// Do not try to decrypt this connection anymore 
					addException();
				}

				while (BIO_pending(m_sdLocal.m_pbioWrite))
				{
					n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReverse, tempBuf, n);
					}
				}						
	
				if (!SSL_is_init_finished(m_sdLocal.m_pSSL))
				{
					return FR_DATA_EATEN;
				}
			}
			
			// Start the data exchange
			m_state = SFS_DATA_EXCHANGE;
			
			m_tlsEstablishedTs = nf_time();

			if (!(m_flags & FF_SSL_DECODE_ONLY))
			{
				// Ok, handshake is finished. Now filter the received packets saved in temporary buffer.
				if (m_tempBufferRemote.size() > 0)
				{
					m_pSession->tcp_packet(this, DD_INPUT, pdReverse, 
						m_tempBufferRemote.buffer(), m_tempBufferRemote.size());
					
					m_tempBufferRemote.reset();
				}
			}

			m_tempBufferLocal.reset();

			// Also filter the packet from local endpoint
			for (;;)
			{
				n = SSLRead_server(m_sdLocal);
				if (n < 0)
					return FR_ERROR;

				if (n == 0)
					break;

				if ((n > 0) && !m_remoteDisconnectState)
				{
					m_tempBufferLocal.append(m_sdLocal.m_buffer.buffer(), n);
				}
			}
				
			if (m_tempBufferLocal.size() > 0)
			{
				m_pSession->tcp_packet(this, DD_INPUT, pdReal, m_tempBufferLocal.buffer(), m_tempBufferLocal.size());
				m_tempBufferLocal.reset();

				m_bFirstPacket = false;
			}

			for (int i=0; i<m_remoteDisconnectState; i++)
			{
				m_pSession->tcp_packet(this, DD_INPUT, pdReverse, NULL, 0);
				break;
			}

			return FR_DATA_EATEN;
		}
		break;
	
#ifdef WIN32
	case SFS_SERVER_HANDSHAKE_REQUEST_CLIENT_CERT:
		{
			if (len == 0)
			{
				if (pd == PD_RECEIVE)
				{
					m_remoteDisconnectState++;
				} else
				{
					addException();
					return FR_ERROR;
				}
			}

			if (m_serverHandshake)
			{
				pdReverse = pd;
				pd = (pd == PD_SEND)? PD_RECEIVE : PD_SEND;
			}

			if (len > 0 && pd == PD_RECEIVE)
			{
				// Read a packet from remote endpoint to temporary buffer
				BIO_write(m_sdRemote.m_pbioRead, buf, len);

				for (;;)
				{
					n = SSLRead_client(m_sdRemote);
					if (n < 0)
						return FR_ERROR;
								
					if (n == 0)
						break;

					if (n > 0)
					{
						m_tempBufferRemote.append(m_sdRemote.m_buffer.buffer(), n);
					}
				} 

				if (SSL_get_shutdown(m_sdRemote.m_pSSL))
				{
					m_remoteDisconnectState++;
				}

				return FR_DATA_EATEN;
			}

			// Continue SSL handshake
			n = BIO_write(m_sdLocal.m_pbioRead, buf, len);

			if (!SSL_is_init_finished(m_sdLocal.m_pSSL))
			{
				n = SSL_accept(m_sdLocal.m_pSSL);
				if (n < 0)
				{
					if (SSL_get_error(m_sdLocal.m_pSSL, n) != SSL_ERROR_WANT_READ)
					{
						return FR_DELETE_ME;
					}
				} 

				if (n == 0)
				{
					// Controlled shutdown
					SSL_shutdown(m_sdLocal.m_pSSL);
					// Do not try to decrypt this connection anymore 
					addException();
				}

				while (BIO_pending(m_sdLocal.m_pbioWrite))
				{
					n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReverse, tempBuf, n);
					}
				}						
	
				if (!SSL_is_init_finished(m_sdLocal.m_pSSL))
				{
					return FR_DATA_EATEN;
				}
			}
			
			if (m_requestClientCert)
			{
				X509 * peer;
				
				if (peer = SSL_get_peer_certificate(m_sdLocal.m_pSSL))
				{
					IOB buf;
					int n;
					unsigned char * p;

					n = i2d_X509(peer, NULL);
					if (!buf.resize(n))
					{
						X509_free(peer);
						return FR_ERROR;
					}

					p = (unsigned char *)buf.buffer();
					i2d_X509(peer, &p);
					X509_free(peer);

					X509 * x509 = NULL;
					EVP_PKEY * pkey = NULL;

					GEN_SESSION_INFO si;
					m_pSession->getSessionInfo(&si);

					ImpersonateProcess ip(si.tcpConnInfo.processId);

					if (findCertificate(buf.buffer(), buf.size(), &x509, &pkey))
					{
						m_clientCert = x509;
						m_clientPKey = pkey;
					} else
					{
						addException(false);
					}
				}

				n = SSL_connect(m_sdRemote.m_pSSL);
				if (n < 0)
				{
					int err = SSL_get_error(m_sdRemote.m_pSSL, n);
					
					if (err != SSL_ERROR_WANT_READ &&
						err != SSL_X509_LOOKUP)
					{
						return FR_ERROR;
					}
				}

				if (n == 0)
				{
					// Controlled shutdown
					SSL_shutdown(m_sdRemote.m_pSSL);
					// Do not try to decrypt this connection anymore 
					addException();
				}
					
				while (BIO_pending(m_sdRemote.m_pbioWrite))
				{
					n = BIO_read(m_sdRemote.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pd, tempBuf, n);
					}
				}
			}

			m_requestClientCert = false;
			m_state = SFS_DATA_EXCHANGE;
			m_tlsEstablishedTs = nf_time();

			m_tempBufferLocal.reset();

			return FR_DATA_EATEN;
		}
		break;
#endif

	case SFS_DATA_EXCHANGE:
//		if (m_sessionDirection == nfapi::NF_D_OUT)
		{
			if (isException()) 
			{
				DbgPrint("SSLFilter::tcp_packet() bypass exception");
				return FR_ERROR;
			}

			// Handle disconnect request
			if (len == 0)
			{
				if (m_bFirstPacket)
				{
					if (!m_tlsDomain.empty())
					{
						std::string host = m_tlsDomain;
						
						strlwr((char*)host.c_str());
						
						if (host.find("surfeasy.com") != std::string::npos ||
							host.find("opera-proxy.net") != std::string::npos ||
							host.find("google.com") != std::string::npos)
						{
							addException();
							m_clearExceptions = false;
						}
					}
				}

				m_pSession->tcp_packet(this, DD_INPUT, pd, NULL, 0);

				if (pd == PD_RECEIVE)
				{
					if (m_remoteDisconnectState == 0)
					{
						m_pSession->tcp_packet(this, DD_INPUT, pd, NULL, 0);
						m_remoteDisconnectState = 1;
					}
				} else
				{
					if (m_localDisconnectState == 0)
					{
						m_pSession->tcp_packet(this, DD_INPUT, pd, NULL, 0);
						m_localDisconnectState = 1;
					}
				}
				//m_state = SFS_DISCONNECTED;

				return FR_DATA_EATEN;
			}

			bool isFirstPacket = m_bFirstPacket;

			m_bFirstPacket = false;

			if (m_serverHandshake)
			{
				pdReverse = pd;
				pd = (pd == PD_SEND)? PD_RECEIVE : PD_SEND;
			}

			IOB tempBuffer;

			if (pd == PD_RECEIVE)
			{
				if (m_flags & FF_SSL_DECODE_ONLY)
				{
					m_pSession->tcp_packet(this, DD_INPUT, pdReal, buf, len);
					return FR_DATA_EATEN;
				}

				if (!SSL_is_init_finished(m_sdRemote.m_pSSL))
				{
					BIO_write(m_sdRemote.m_pbioRead, buf, len);
		
					n = SSL_connect(m_sdRemote.m_pSSL);
					if (n < 0)
					{
						int err = SSL_get_error(m_sdRemote.m_pSSL, n);
						
						if (err != SSL_ERROR_WANT_READ &&
							err != SSL_X509_LOOKUP)
						{
							return FR_ERROR;
						}
					}

					if (n == 0)
					{
						// Controlled shutdown
						SSL_shutdown(m_sdRemote.m_pSSL);
						// Do not try to decrypt this connection anymore 
						addException();
					}
						
					while (BIO_pending(m_sdRemote.m_pbioWrite))
					{
						n = BIO_read(m_sdRemote.m_pbioWrite, tempBuf, sizeof(tempBuf));
						if (n > 0)
						{
							m_pSession->tcp_packet(this, DD_OUTPUT, pdReverse, tempBuf, n);
						}
					}

					if (m_requestClientCert)
					{
						SSL_set_verify(m_sdLocal.m_pSSL, SSL_VERIFY_PEER, verify_callback);

						SSL_renegotiate(m_sdLocal.m_pSSL);

						n = SSL_do_handshake(m_sdLocal.m_pSSL);
						if (n < 0)
						{
							if (SSL_get_error(m_sdLocal.m_pSSL, n) != SSL_ERROR_WANT_READ)
							{
								return FR_ERROR;
							}
						}

						while (BIO_pending(m_sdLocal.m_pbioWrite))
						{
							n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
							if (n > 0)
							{
								m_pSession->tcp_packet(this, DD_OUTPUT, PD_RECEIVE, tempBuf, n);
							}
						}						

						m_state = SFS_SERVER_HANDSHAKE_REQUEST_CLIENT_CERT;

						return FR_DATA_EATEN;
					}

					if (!SSL_is_init_finished(m_sdRemote.m_pSSL))
						return FR_DATA_EATEN;

					if (m_tempBufferLocal.size())
					{
						m_pSession->tcp_packet(this, DD_INPUT, pdReverse, m_tempBufferLocal.buffer(), m_tempBufferLocal.size());
						m_tempBufferLocal.reset();
					}

					return FR_DATA_EATEN;
				}

				if (m_tempBufferRemote.size())
				{
					tempBuffer.append(m_tempBufferRemote.buffer(), m_tempBufferRemote.size());
					m_tempBufferRemote.reset();
				}

				// Filter the packet received from remote endpoint
				while (offset < len)
				{
					n = BIO_write(m_sdRemote.m_pbioRead, buf + offset, len - offset);

					if (n <= 0)
						return FR_ERROR;

					offset += n;

					DbgPrint("SSLFilter::tcp_packet() id=%llu dd=%d pd=%d m_state=%d len=%d written=%d",
								m_pSession->getId(), dd, pdReal, m_state, len, n);

					for (;;)
					{
						n = SSLRead_client(m_sdRemote);
						if (n < 0)
							return FR_ERROR;

						if (n == 0)
							break;
						
						if (n > 0)
						{
							tempBuffer.append(m_sdRemote.m_buffer.buffer(), n);
						}
					} 
				}

				if (SSL_renegotiate_pending(m_sdRemote.m_pSSL))
				{
					// First - continue renegotiate
                    n = SSL_connect(m_sdRemote.m_pSSL);
                    if (n < 0)
                    {
                        int err = SSL_get_error(m_sdRemote.m_pSSL, n);
                        
                        if (err != SSL_ERROR_WANT_READ &&
                            err != SSL_X509_LOOKUP)
                        {
                            return FR_ERROR;
                        }
                    }
                    
                    if (n == 0)
                    {
                        // Controlled shutdown
                        SSL_shutdown(m_sdRemote.m_pSSL);
                        // Do not try to decrypt this connection anymore
                        addException();
                    }
                    
                    while (BIO_pending(m_sdRemote.m_pbioWrite))
                    {
                        n = BIO_read(m_sdRemote.m_pbioWrite, tempBuf, sizeof(tempBuf));
                        if (n > 0)
                        {
                            m_pSession->tcp_packet(this, DD_OUTPUT, pdReverse, tempBuf, n);
                        }
                    }
                    
                    if (!SSL_is_init_finished(m_sdRemote.m_pSSL))
                        return FR_DATA_EATEN;
                    
                    if (m_tempBufferLocal.size())
                    {
                        m_pSession->tcp_packet(this, DD_INPUT, pdReverse, m_tempBufferLocal.buffer(), m_tempBufferLocal.size());
                        m_tempBufferLocal.reset();
                    }
                    
                    return FR_DATA_EATEN;
				}

				while (BIO_pending(m_sdRemote.m_pbioWrite))
				{
					n = BIO_read(m_sdRemote.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReverse, tempBuf, n);
					}
				}						

				if (tempBuffer.size() > 0)
				{
					if ((tempBuffer.size() < 256) && (m_sdRemote.m_pSSL->rlayer.packet_length > 0))
					{
						m_tempBufferRemote.append(tempBuffer.buffer(), tempBuffer.size());
					} else
					{
						m_pSession->tcp_packet(this, DD_INPUT, pdReal, tempBuffer.buffer(), tempBuffer.size());
					}
				}

				if (SSL_get_shutdown(m_sdRemote.m_pSSL))
				{
					// Send disconnect request to filtering chain
					m_pSession->tcp_packet(this, DD_INPUT, pdReal, NULL, 0);
					m_remoteDisconnectState = 1;
				}

				if (m_clearExceptions)
				{
					m_clearExceptions = false;

					confirmTlsException(TLS_ALL_CIPHERS);
					confirmTlsException(TLS_1_0_PROTOCOL);
					confirmTlsException();
					clearExceptions();
				}

				return FR_DATA_EATEN;
			} else
			{
				if (!SSL_is_init_finished(m_sdLocal.m_pSSL))
				{
					BIO_write(m_sdLocal.m_pbioRead, buf, len);

					n = SSL_accept(m_sdLocal.m_pSSL);
					if (n < 0)
					{
						if (SSL_get_error(m_sdLocal.m_pSSL, n) != SSL_ERROR_WANT_READ)
						{
							return FR_DELETE_ME;
						}
					} 

					if (n == 0)
					{
						// Controlled shutdown
						SSL_shutdown(m_sdLocal.m_pSSL);
						// Do not try to decrypt this connection anymore 
						addException();
					}

					while (BIO_pending(m_sdLocal.m_pbioWrite))
					{
						n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
						if (n > 0)
						{
							m_pSession->tcp_packet(this, DD_OUTPUT, pdReverse, tempBuf, n);
						}
					}						
		
					if (!SSL_is_init_finished(m_sdLocal.m_pSSL))
					{
						return FR_DATA_EATEN;
					}

					if (!(m_flags & FF_SSL_DECODE_ONLY))
					{
						if (m_tempBufferRemote.size())
						{
							m_pSession->tcp_packet(this, DD_INPUT, pdReverse, m_tempBufferRemote.buffer(), m_tempBufferRemote.size());
							m_tempBufferLocal.reset();
						}
					}

					return FR_DATA_EATEN;
				}

				if (m_tempBufferLocal.size())
				{
					tempBuffer.append(m_tempBufferLocal.buffer(), m_tempBufferLocal.size());
					m_tempBufferLocal.reset();
				}

				// Filter the packet sent from local endpoint
				while (offset < len)
				{
					n = BIO_write(m_sdLocal.m_pbioRead, buf + offset, len - offset);

					if (n <= 0)
						return FR_ERROR;

					offset += n;

					DbgPrint("SSLFilter::tcp_packet() id=%llu dd=%d pd=%d m_state=%d len=%d written=%d",
								m_pSession->getId(), dd, pdReal, m_state, len, n);

					for (;;)
					{
						n = SSLRead_server(m_sdLocal);
						if (n < 0)
							return FR_ERROR;

						if (n == 0)
							break;

						if (n > 0)
						{
							tempBuffer.append(m_sdLocal.m_buffer.buffer(), n);
						}
					}
				}

				while (BIO_pending(m_sdLocal.m_pbioWrite))
				{
					n = BIO_read(m_sdLocal.m_pbioWrite, tempBuf, sizeof(tempBuf));
					if (n > 0)
					{
						m_pSession->tcp_packet(this, DD_OUTPUT, pdReverse, tempBuf, n);
					}
				}						

				if (tempBuffer.size() > 0)
				{
					isFirstPacket = false;

					if (((tempBuffer.size() < 256) && (m_sdLocal.m_pSSL->rlayer.packet_length > 0)) ||
						(!(m_flags & FF_SSL_DECODE_ONLY) && !SSL_is_init_finished(m_sdRemote.m_pSSL)))
					{
						m_tempBufferLocal.append(tempBuffer.buffer(), tempBuffer.size());
					} else
					{
						m_pSession->tcp_packet(this, DD_INPUT, pdReal, tempBuffer.buffer(), tempBuffer.size());
					}
				}

				if (SSL_get_shutdown(m_sdLocal.m_pSSL))
				{
					// Add exception if the client uses graceful disconnect
					if (isFirstPacket &&
						((nf_time() - m_tlsEstablishedTs) < INVALID_CERT_TIMEOUT))
					{
						addException();
						m_clearExceptions = false;
					}

					// Send disconnect request to filtering chain
					m_pSession->tcp_packet(this, DD_INPUT, pdReal, NULL, 0);
					m_localDisconnectState = 1;
				}

				if (m_clearExceptions)
				{
					m_clearExceptions = false;

					confirmTlsException(TLS_ALL_CIPHERS);
					confirmTlsException(TLS_1_0_PROTOCOL);
					confirmTlsException();
					clearExceptions();
				}

				return FR_DATA_EATEN;
			}
		}
		break;

	case SFS_DISCONNECTED:
//		if (m_sessionDirection == nfapi::NF_D_OUT)
		{
			// Disconnect
			m_pSession->tcp_packet(this, DD_INPUT, pd, NULL, 0);
			return FR_DATA_EATEN;
		}
		break;
	default:
		break;
	}

	return FR_PASSTHROUGH;
}
		
eFilterResult SSLFilter::udp_packet(eDataDirection dd,
					ePacketDirection pd,
					const unsigned char * remoteAddress, 
					const char * buf, int len)
{
	return FR_DELETE_ME;
}

PF_FilterType SSLFilter::getType()
{
	return FT_SSL;
}

PF_FilterCategory SSLFilter::getCategory()
{
	return FC_PREPROCESSOR;
}

bool SSLFilter::postObject(PFObjectImpl & object)
{
	return false;
}

#endif