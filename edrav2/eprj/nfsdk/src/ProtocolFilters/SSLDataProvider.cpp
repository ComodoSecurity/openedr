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

#include "SSLDataProvider.h"
#include "FileStream.h"
#include "PFFilterDefs.h"
#include "Proxy.h"

#ifdef WIN32
#include <windows.h>
#include <process.h>
#endif

#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/aes.h>
#include <openssl/ocsp.h>

#ifdef WIN32

	#undef __WINCRYPT_H__
	#undef WINTRUST_H
	#include "certimp.h"
	#pragma comment(lib, "libssl.lib")
	#pragma comment(lib, "libcrypto.lib")

	#define CERTDB_NAME L"/cert.db"
	#define XDB_NAME L"/x2.db"
	#define XTLSDB_NAME L"/xtls2.db"
	#define XVDB_NAME L"/xv2.db"

#else

	#include "certimp_nix.h"

	#define CERTDB_NAME "/cert.db"
	#define XDB_NAME "/x2.db"
	#define XTLSDB_NAME "/xtls2.db"
	#define XVDB_NAME "/xv2.db"
#endif

#include "gpkey.h"

using namespace ProtocolFilters;

Proxy & getProxy();

#define EXCEPTION_TIMEOUT 10
#define EXCEPTION_LONG_TIMEOUT 5 * 60

#define _LOAD_CA_STORE 1

#ifdef WIN32

#ifndef CERT_CHAIN_OPT_IN_WEAK_SIGNATURE
	#define CERT_CHAIN_OPT_IN_WEAK_SIGNATURE 0x00010000
	#define CERT_STRONG_SIGN_SERIALIZED_INFO_CHOICE     1
#endif

#ifndef CERT_OCSP_RESPONSE_PROP_ID
#define CERT_OCSP_RESPONSE_PROP_ID          70
#endif

	typedef struct _CERT_STRONG_SIGN_SERIALIZED_INFO_EX {
		DWORD                   dwFlags;
		LPWSTR                  pwszCNGSignHashAlgids;
		LPWSTR                  pwszCNGPubKeyMinBitLengths; // Optional
	} CERT_STRONG_SIGN_SERIALIZED_INFO_EX, *PCERT_STRONG_SIGN_SERIALIZED_INFO_EX;

	#define CERT_STRONG_SIGN_ECDSA_ALGORITHM          L"ECDSA"

	typedef struct _CERT_STRONG_SIGN_PARA_EX {
		DWORD                   cbSize;

		DWORD                   dwInfoChoice;
		union  {
			void                                *pvInfo;

			// CERT_STRONG_SIGN_SERIALIZED_INFO_CHOICE
			PCERT_STRONG_SIGN_SERIALIZED_INFO_EX   pSerializedInfo;

			// CERT_STRONG_SIGN_OID_INFO_CHOICE
			LPSTR                               pszOID;
	        
		} DUMMYUNIONNAME;
	} CERT_STRONG_SIGN_PARA_EX, *PCERT_STRONG_SIGN_PARA_EX;

	typedef const CERT_STRONG_SIGN_PARA_EX *PCCERT_STRONG_SIGN_PARA_EX;


	typedef struct _CERT_CHAIN_PARA_EX {

		DWORD            cbSize;
		CERT_USAGE_MATCH RequestedUsage;

	#ifdef CERT_CHAIN_PARA_HAS_EXTRA_FIELDS

		// Note, if you #define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS, then, you
		// must zero all unused fields in this data structure.
		// More fields could be added in a future release.

		CERT_USAGE_MATCH RequestedIssuancePolicy;
		DWORD            dwUrlRetrievalTimeout;     // milliseconds
		BOOL             fCheckRevocationFreshnessTime;
		DWORD            dwRevocationFreshnessTime; // seconds

		// If nonNULL, any cached information before this time is considered
		// time invalid and forces a wire retrieval. When set overrides
		// the registry configuration CacheResync time.
		LPFILETIME                  pftCacheResync;

		//
		// The following is set to check for Strong Signatures
		//
		PCCERT_STRONG_SIGN_PARA_EX pStrongSignPara;

		//
		// By default the public key in the end certificate is checked.
		// CERT_CHAIN_STRONG_SIGN_DISABLE_END_CHECK_FLAG can be
		// set in the following flags to not check if the end certificate's public
		// key length is strong.
		//
		DWORD                   dwStrongSignFlags;

	#endif

	} CERT_CHAIN_PARA_EX, *PCERT_CHAIN_PARA_EX;

#endif

struct aes_k
{
	const unsigned char key[16];
	const unsigned char iv[32];
};

static struct aes_k const ssldp_pk = 
{ 
	{	
		0x11, 0x1b, 0x17, 0x09, 0xe6, 0xd2, 0x31, 0xc3,
		0x9d, 0x17, 0xda, 0xe2, 0x65, 0x35, 0x4a, 0x35
	}, /* key */
	{	
		0x19, 0xa6, 0x12, 0xd6, 0x66, 0xc1, 0x70, 0xfa, 
		0xe9, 0x58, 0x07, 0xe1, 0x87, 0x7d, 0x5f, 0xdb,
		0x81, 0x6c, 0x82, 0x48, 0xbb, 0xc7, 0xbd, 0xd0, 
		0x87, 0x36, 0x07, 0x17, 0x06, 0x0a, 0x33, 0xdc
	} /* iv */
};

int ProtocolFilters::crypt_buffer(unsigned char * buf, int len, int encrypt)
{
	AES_KEY			key;
	unsigned char	iv[AES_BLOCK_SIZE*2]; 
	int				aLen = len;

	if (!buf || (len <= AES_BLOCK_SIZE))
		return 0;

	// Align length to block size
	if (len % AES_BLOCK_SIZE)
	{
		aLen -= len % AES_BLOCK_SIZE;
	}

	memcpy(iv, ssldp_pk.iv, sizeof(iv));

	if (encrypt)
	{
		AES_set_encrypt_key(ssldp_pk.key, 8*sizeof(ssldp_pk.key), &key);
		AES_cbc_encrypt(buf, buf, aLen, &key, iv, AES_ENCRYPT);
	} else 
	{
		AES_set_decrypt_key(ssldp_pk.key, 8*sizeof(ssldp_pk.key), &key);
		AES_cbc_encrypt(buf, buf, aLen, &key, iv, AES_DECRYPT);
	}

	return len;
}

#ifdef WIN32
#define MUTEX_TYPE       HANDLE
#define MUTEX_SETUP(x)   x = CreateMutex(NULL,FALSE,NULL)
#define MUTEX_CLEANUP(x) CloseHandle(x)
#define MUTEX_LOCK(x)    WaitForSingleObject(x,INFINITE)
#define MUTEX_UNLOCK(x)  ReleaseMutex(x)
#define THREAD_ID        GetCurrentThreadId()
#else
#define MUTEX_TYPE       pthread_mutex_t
#define MUTEX_SETUP(x)   pthread_mutex_init(&(x), NULL)
#define MUTEX_CLEANUP(x) pthread_mutex_destroy(&(x))
#define MUTEX_LOCK(x)    pthread_mutex_lock(&(x))
#define MUTEX_UNLOCK(x)  pthread_mutex_unlock(&(x))
#define THREAD_ID        pthread_self()
#endif

static MUTEX_TYPE *g_lock_cs = NULL;

static void locking_callback(int mode, int type, const char *file, int line)
{
	if (mode & CRYPTO_LOCK)
	{
		MUTEX_LOCK(g_lock_cs[type]);
	} else
	{
		MUTEX_UNLOCK(g_lock_cs[type]);
	}
}

static unsigned long id_function(void)
{
  return ((unsigned long)THREAD_ID);
}

static void thread_setup(void)
{
	int i;

	g_lock_cs = (MUTEX_TYPE*)OPENSSL_malloc(CRYPTO_num_locks() * sizeof(MUTEX_TYPE));
	if (!g_lock_cs)
		return;

	for (i=0; i<CRYPTO_num_locks(); i++)
	{
		MUTEX_SETUP(g_lock_cs[i]);
	}

	CRYPTO_set_id_callback(id_function);
	CRYPTO_set_locking_callback((void (*)(int,int, const char *,int))locking_callback);
}

static void thread_cleanup(void)
{
	int i;

	if (!g_lock_cs)
		return;

	CRYPTO_set_id_callback(NULL);
	CRYPTO_set_locking_callback(NULL);
	
	for (i=0; i<CRYPTO_num_locks(); i++)
		MUTEX_CLEANUP(g_lock_cs[i]);
	
	OPENSSL_free(g_lock_cs);
	g_lock_cs = NULL;
}

void openssl_init()
{
	SSL_library_init();
	OpenSSL_add_ssl_algorithms();
	SSL_load_error_strings();
	ERR_load_crypto_strings();

	thread_setup();
}

void openssl_free()
{
	thread_cleanup();

	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
//	ERR_remove_state(0);
	EVP_cleanup();
}

SSLDataProvider::SSLDataProvider(void)
{
	m_initialized = false;
	m_flags = RSIF_IMPORT_EVERYWHERE | RSIF_GENERATE_ROOT_PRIVATE_KEY;
	m_rootCaStore = NULL;
	m_rootCaList = NULL;
}

SSLDataProvider::~SSLDataProvider(void)
{
	free();
}


bool SSLDataProvider::init(const tPathString & baseFolder)
{
	AutoLock lock(m_cs);

	if (m_initialized)
		return true;

	openssl_init();

	m_baseFolder = baseFolder;
	m_rootSSLCertName = "NetFilterSDK";

	memset(&m_exceptionsTimeout, 0, sizeof(m_exceptionsTimeout));

	for (;;)
	{
		if (!m_certStorage.init(m_baseFolder + CERTDB_NAME))
			break;

		if (!m_xStorage.init(m_baseFolder + XDB_NAME))
			break;

		if (!m_xtlsStorage.init(m_baseFolder + XTLSDB_NAME))
			break;

		if (!m_xvStorage.init(m_baseFolder + XVDB_NAME))
			break;

		startVerifyThread();

		m_initialized = true;
		break;
	}

	if (!m_initialized)
	{
		openssl_free();
		return false;
	}

	return true;
}

void SSLDataProvider::free()
{
	AutoLock lock(m_cs);

	if (m_initialized)
	{
		m_initialized = false;
		
		stopImportThread();

		stopVerifyThread();

		m_ecMap.clear();
		m_ecTlsMap.clear();
		m_certStorage.free();
		m_xStorage.free();
		m_xtlsStorage.free();
		m_xvStorage.free();

		if (m_rootCaStore)
		{
			X509_STORE_free(m_rootCaStore);
			m_rootCaStore = NULL;
		}

		if (m_rootCaList)
		{
			sk_X509_free(m_rootCaList);
			m_rootCaList = NULL;
		}

		openssl_free();
	}
}

int SSLDataProvider::_add_ext(X509 *cert, int nid, const char *value)
{
	X509_EXTENSION *ex;
	X509V3_CTX ctx;
	X509V3_set_ctx_nodb(&ctx);
	X509V3_set_ctx(&ctx, cert, cert, NULL, NULL, 0);
	ex = X509V3_EXT_conf_nid(NULL, &ctx, nid, (char*)value);
	if (!ex)
		return 0;

	X509_add_ext(cert,ex,-1);
	X509_EXTENSION_free(ex);
	return 1;
}

int SSLDataProvider::_delete_ext(X509 *cert, int nid)
{
	int loc = X509_get_ext_by_NID(cert, nid, -1);
	if (loc >= 0)
	{
		X509_delete_ext(cert, loc);
		return 1;
	} 
	return 0;
}

#define SERIAL_RAND_BITS	128

int SSLDataProvider::rand_serial(BIGNUM *b, ASN1_INTEGER *ai)
{
	BIGNUM *btmp;
	int ret = 0;
	if (b)
		btmp = b;
	else
		btmp = BN_new();

	if (!btmp)
		return 0;

	if (!BN_pseudo_rand(btmp, SERIAL_RAND_BITS, 0, 0))
		goto error;

	if (ai && !BN_to_ASN1_INTEGER(btmp, ai))
		goto error;

	ret = 1;
	
	error:

	if (!b)
		BN_free(btmp);
	
	return ret;
}

int SSLDataProvider::copy_extensions(X509 *dst, X509 *src)
{
	X509_EXTENSION *ext;
	int i;
	
	if (!dst || !src)
		return 1;

	i = X509_get_ext_by_NID(src, NID_subject_alt_name, -1);
	if (i != -1)
	{
		ext = X509_get_ext(src, i);
		X509_add_ext(dst, ext, -1);
	}

	i = X509_get_ext_by_NID(src, NID_basic_constraints, -1);
	if (i != -1)
	{
		ext = X509_get_ext(src, i);
		X509_add_ext(dst, ext, -1);
	}
/*
	i = X509_get_ext_by_NID(src, NID_certificate_policies, -1);
	if (i != -1)
	{
		ext = X509_get_ext(src, i);
		X509_add_ext(dst, ext, -1);
	}
*/
	i = X509_get_ext_by_NID(src, NID_policy_constraints, -1);
	if (i != -1)
	{
		ext = X509_get_ext(src, i);
		X509_add_ext(dst, ext, -1);
	}

	i = X509_get_ext_by_NID(src, NID_ext_key_usage, -1);
	if (i != -1)
	{
		ext = X509_get_ext(src, i);
		X509_add_ext(dst, ext, -1);
	}


/*
	i = X509_get_ext_by_NID(src, NID_info_access, -1);
	if (i != -1)
	{
		ext = X509_get_ext(src, i);
		X509_add_ext(dst, ext, -1);
	}
*/
/*
	for (int k=0; k<200; k++)
	{
		ext = X509_get_ext(src, k);
		if (ext)
			X509_add_ext(dst, ext, -1);
	}
*/
	return 1;
}

bool SSLDataProvider::hostMatches(const char *hostname, const char *certHostname)
{
	if (!stricmp(hostname, certHostname)) 
	{ 
		return true;
	} else 
	if (certHostname[0] == '*' && certHostname[1] == '.' && certHostname[2] != 0) 
	{ 
		const char * tail = strchr(hostname, '.');
		if (tail && !stricmp(tail + 1, certHostname + 2)) 
		{
			return true;
		}
	}
	return false;
}

bool SSLDataProvider::checkSubjectName(const char *szName, X509 * x)
{
	X509_name_st *name=NULL;
	int pos;

	name = X509_get_subject_name(x);
	if (name)
	{
		pos = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
		if (pos != -1)
		{
			X509_NAME_ENTRY * pEntry = X509_NAME_get_entry(name, pos);
			if (pEntry)
			{
				ASN1_STRING * entryStr = X509_NAME_ENTRY_get_data(pEntry);
				if (entryStr)
				{
					char * buf = NULL;
					int    len;

					len = ASN1_STRING_to_UTF8((unsigned char**)&buf, entryStr);
	
					if (buf && (len > 0))
					{
						std::string cn = std::string(buf, len);
						OPENSSL_free(buf);
						
						if (hostMatches(szName, cn.c_str()))
							return true;
					}
				}
			}
		}
	}

	GENERAL_NAMES *subjectAltNames;

	subjectAltNames = (GENERAL_NAMES*) X509_get_ext_d2i(x, NID_subject_alt_name, NULL, NULL);
	if (subjectAltNames)
	{
		int numalts = sk_GENERAL_NAME_num(subjectAltNames);

		for (int i=0; i<numalts; i++)
		{
			const GENERAL_NAME * gn = sk_GENERAL_NAME_value(subjectAltNames, i);
			if (gn && (gn->type == GEN_DNS) && gn->d.ia5)
			{
				char * buf = NULL;
				int    len;

				len = ASN1_STRING_to_UTF8((unsigned char**)&buf, gn->d.ia5);
				if (buf && (len > 0))
				{
					std::string cn = std::string(buf, len);
					OPENSSL_free(buf);

					if (hostMatches(szName, cn.c_str()))
						return true;
				}
			}
		}
	}
	
	return false;
}

int SSLDataProvider::_mkcert(const char *szName, 
			X509 * pTemplate, 
			X509 **x509p, EVP_PKEY **pkeyp, 
			int bits, int days, bool ca, bool ownPrivKey, tPF_FilterFlags flags)
{
	X509 *x;
	EVP_PKEY *pk = NULL;
	RSA *rsa = NULL;
	X509_name_st *name=NULL;

	if ((x509p == NULL) || (*x509p == NULL))
	{
		if ((x=X509_new()) == NULL)
			goto err;
	}
	else
		x= *x509p;

	if (ca && !ownPrivKey)
	{
		unsigned char * p = gpk;

		d2i_AutoPrivateKey(&pk, (const unsigned char**)&p, sizeof(gpk));
	} else
	{
		if ((pkeyp == NULL) || (*pkeyp == NULL))
		{
			if ((pk=EVP_PKEY_new()) == NULL)
			{
				return(0);
			}
		} else
			pk = *pkeyp;

		rsa=RSA_generate_key(bits,RSA_F4,NULL,NULL);
		if (!EVP_PKEY_assign_RSA(pk,rsa))
		{
			goto err;
		}
		rsa=NULL;
	}

	X509_set_version(x,2);
	//ASN1_INTEGER_set(X509_get_serialNumber(x),serial);

	X509_set_pubkey(x,pk);

	if (pTemplate != NULL &&
		(flags & FF_SSL_KEEP_SERIAL_NUMBERS))
	{
		X509_set_serialNumber(x, X509_get_serialNumber(pTemplate));
	} else
	{
		if (!rand_serial(NULL, X509_get_serialNumber(x)))
			goto err;
	}

	if (pTemplate)
	{
		name=X509_get_subject_name(pTemplate);

		if (!checkSubjectName(szName, pTemplate))
		{
			int pos = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
			if (pos != -1)
			{
				X509_NAME_delete_entry(name, pos);

				X509_NAME_add_entry_by_txt(name,"CN",
					MBSTRING_ASC, (unsigned char*)szName, -1, -1, 0);
			}
		}

		X509_set_subject_name(x, name);

//		X509_set_serialNumber(x, X509_get_serialNumber(pTemplate));

		copy_extensions(x, pTemplate);

		X509_set_notBefore(x, X509_get_notBefore(pTemplate));
		X509_set_notAfter(x, X509_get_notAfter(pTemplate));
	} else
	{
		name=X509_get_subject_name(x);

		X509_NAME_add_entry_by_txt(name,"C",
					MBSTRING_ASC, (unsigned char*)"EN", -1, -1, 0);
		X509_NAME_add_entry_by_txt(name,"CN",
					MBSTRING_ASC, (unsigned char*)szName, -1, -1, 0);

		X509_gmtime_adj(X509_get_notBefore(x), (long)-60*60*24*days);
		X509_gmtime_adj(X509_get_notAfter(x),(long)2*60*60*24*days);
	}

	X509_set_issuer_name(x,name);

	if (ca)
	{
		_add_ext(x, NID_basic_constraints, "critical,CA:TRUE");
		_add_ext(x, NID_key_usage, "critical,keyCertSign,cRLSign");
	}

	if (!X509_sign(x,pk,EVP_sha256()))
		goto err;

	*x509p=x;
	*pkeyp=pk;
	return(1);
err:
	if (x && (x != pTemplate))
	{
		X509_free(x);
	}

	if (pk)
	{
		EVP_PKEY_free(pk);
	}
	return(0);
}


int SSLDataProvider::_mkcert_child(const char *szName, 
				  X509 * pTemplate, 
				  X509 **x509p, EVP_PKEY **pkeyp, 
				  X509 *x509Parent, EVP_PKEY * pkeyParent, 
				  int bits, int days, tPF_FilterFlags flags)
{
	X509 *x;
	EVP_PKEY *pk;
	RSA *rsa;
	X509_name_st *name=NULL;
	const EVP_MD *md = EVP_sha256();

	if ((pkeyp == NULL) || (*pkeyp == NULL))
	{
		if ((pk=EVP_PKEY_new()) == NULL)
		{
			return(0);
		}
	} else
		pk= *pkeyp;

	if ((x509p == NULL) || (*x509p == NULL))
	{
		if ((x=X509_new()) == NULL)
			goto err;
	} else
		x= *x509p;

	rsa=RSA_generate_key(bits,RSA_F4,NULL,NULL);
	if (!EVP_PKEY_assign_RSA(pk,rsa))
	{
		goto err;
	}
	rsa=NULL;

	X509_set_version(x,2);

	if (pTemplate != NULL &&
		(flags & FF_SSL_KEEP_SERIAL_NUMBERS))
	{
		X509_set_serialNumber(x, X509_get_serialNumber(pTemplate));
	} else
	{
		if (!rand_serial(NULL, X509_get_serialNumber(x)))
			goto err;
	}

	if (!pTemplate)
	{
		name=X509_get_subject_name(x);

		X509_NAME_add_entry_by_txt(name,"C",
					MBSTRING_ASC, (unsigned char*)"EN", -1, -1, 0);
		X509_NAME_add_entry_by_txt(name,"CN",
					MBSTRING_ASC, (unsigned char*)szName, -1, -1, 0);

		X509_gmtime_adj(X509_get_notBefore(x),0);
		X509_gmtime_adj(X509_get_notAfter(x),(long)60*60*24*days);

		// Add also subject_alt_name to avoid issues with Chrome
		if (szName) 
		{
            char subAltName[1024];

			_snprintf(subAltName, sizeof(subAltName), "DNS:%s", szName);

			X509_EXTENSION *extension = X509V3_EXT_conf_nid (NULL, NULL, NID_subject_alt_name, subAltName);

			if (extension) 
			{
				X509_add_ext (x, extension, -1);
				X509_EXTENSION_free (extension);
			}
		}
	} else
	{
		name=X509_get_subject_name(pTemplate);

		if (!checkSubjectName(szName, pTemplate))
		{
			int pos = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
			if (pos != -1)
			{
				X509_NAME_delete_entry(name, pos);

				X509_NAME_add_entry_by_txt(name,"CN",
					MBSTRING_ASC, (unsigned char*)szName, -1, -1, 0);
			}
		}

		X509_set_subject_name(x, name);

		name=X509_get_subject_name(x509Parent);
		X509_set_issuer_name(x, name);

		copy_extensions(x, pTemplate);

		X509_set_notBefore(x, X509_get_notBefore(pTemplate));
		X509_set_notAfter(x, X509_get_notAfter(pTemplate));

		int sigNid = X509_get_signature_nid(pTemplate);
        const EVP_MD *pMd = EVP_get_digestbynid(sigNid);
        if (pMd) 
		{
            md = pMd;
        }
	}

	X509_set_pubkey(x,pk);

	X509_set_issuer_name(x, X509_get_subject_name(x509Parent));

	if (!X509_sign(x, pkeyParent, md))
		goto err;

	*x509p=x;
	*pkeyp=pk;
	return(1);

err:
	if (x && (x != pTemplate))
	{
		X509_free(x);
	}

	if (pk)
	{
		EVP_PKEY_free(pk);
	}

	return(0);
}

bool SSLDataProvider::createKeys(const char * szName, X509 * pTemplate, bool ca, bool ownPrivKey, SSLKeys & k, tPF_FilterFlags flags)
{
	X509 *x509=NULL;
	EVP_PKEY *pkey=NULL;
	int n;
	unsigned char * p;
	bool res = true;

	if (!_mkcert(szName, pTemplate, &x509, &pkey, 2048, 20*365, ca, ownPrivKey, flags))
		return false;
	
	n = i2d_X509(x509, NULL);
	k.m_x509.reset();
	if (!k.m_x509.resize(n))
	{
		res = false;
		goto fin;
	}
	
	p = (unsigned char *)k.m_x509.buffer();
	i2d_X509(x509, &p);

	n = i2d_PrivateKey(pkey, NULL);
	k.m_pkey.reset();
	if (!k.m_pkey.resize(n))
	{
		res = false;
		goto fin;
	}
	p = (unsigned char *)k.m_pkey.buffer();
	i2d_PrivateKey(pkey, &p);

fin:
	if (x509 && (x509 != pTemplate))
	{
		X509_free(x509);
	}
	if (pkey)
	{
		EVP_PKEY_free(pkey);
	}

	return res;
}

bool SSLDataProvider::createChildKeys(const char * szName, X509 * pTemplate, const SSLKeys & parent, SSLKeys & k, tPF_FilterFlags flags)
{
	X509 *x509p=NULL;
	EVP_PKEY *pkeyp=NULL;
	X509 *x509=NULL;
	EVP_PKEY *pkey=NULL;
	int n;
	unsigned char * p;
	bool res = true;
	int nBits = 1024;

	p = (unsigned char *)parent.m_x509.buffer();
	d2i_X509(&x509p, (const unsigned char **)&p, parent.m_x509.size());
	if (!x509p)
	{
		res = false;
		goto fin;
	}
	
	p = (unsigned char *)parent.m_pkey.buffer();
	d2i_AutoPrivateKey(&pkeyp, (const unsigned char **)&p, parent.m_pkey.size());
	if (!pkeyp)
	{
		res = false;
		goto fin;
	}

	if (!_mkcert_child(szName, pTemplate, 
		&x509, &pkey, 
		x509p, pkeyp, 
		nBits, 20*365, flags))
	{
		res = false;
		goto fin;
	}
	
	n = i2d_X509(x509, NULL);
	k.m_x509.reset();
	if (!k.m_x509.resize(n))
	{ 
		res = false;
		goto fin;
	}
	
	p = (unsigned char *)k.m_x509.buffer();
	i2d_X509(x509, &p);

	n = i2d_PrivateKey(pkey, NULL);
	k.m_pkey.reset();
	if (!k.m_pkey.resize(n))
	{
		res = false;
		goto fin;
	}
	p = (unsigned char *)k.m_pkey.buffer();
	i2d_PrivateKey(pkey, &p);

fin:
	if (x509p)
	{
		X509_free(x509p);
	}
	if (pkeyp)
	{
		EVP_PKEY_free(pkeyp);
	}
	if (x509 && (x509 != pTemplate))
	{
		X509_free(x509);
	}
	if (pkey)
	{
		EVP_PKEY_free(pkey);
	}

	return res;
}


bool SSLDataProvider::loadKeys(const char * serverName, SSLKeys & k)
{
	return m_certStorage.getCert(serverName, k.m_x509, k.m_pkey);
}


bool SSLDataProvider::saveFile(const tPathString & fileName, IOB & iob)
{
#ifdef WIN32
	FILE * hFile = _wfopen(fileName.c_str(), L"wb");
#else
	FILE * hFile = fopen(fileName.c_str(), "wb");
#endif
	if (!hFile)
		return false;

	fseek(hFile, 0, FILE_BEGIN);

	if (fwrite(iob.buffer(), 1, (tStreamSize)iob.size(), hFile) != (tStreamSize)iob.size())
	{
		fclose(hFile);
		return false;
	}

	fclose(hFile);
	return true;
}
	
bool SSLDataProvider::saveKeys(const char * serverName, SSLKeys & k)
{
	return m_certStorage.addCert(serverName, k.m_x509, k.m_pkey);
}

bool getSerialText(ASN1_INTEGER * serial, char * buf, int len)
{
	char c;
	int i, n;

	BIO * pbio = BIO_new(BIO_s_mem());
	if (!pbio)
		return false;

	i2a_ASN1_INTEGER(pbio, serial);

	i = 0;

	while (BIO_pending(pbio))
	{
		n = BIO_read(pbio, &c, 1);
		if ((n > 0) && (i < (len-1)))
		{
			buf[i] = c;
			i++;
		}
	}

	buf[i] = '\0';

	BIO_free(pbio);

	return true;
}

std::string getDigest(const X509 * x)
{
	char digest[SHA_DIGEST_LENGTH];
	unsigned int len = sizeof(digest);

	X509_digest(x, EVP_sha1(), (unsigned char*)digest, &len);
	
	std::string s;
	char num[3];

	for (int i=0; i<(int)sizeof(digest); i++)
	{
		_snprintf(num, sizeof(num), "%.2x", (unsigned char)digest[i]);
		s += num;
	}

	return s;
}


bool SSLDataProvider::getSelfSignedCert(const char * serverName, void * pTemplate, 
										bool addToTrusted, bool ca, SSLKeys & k, tPF_FilterFlags flags)
{
	AutoLock lock(m_cs);
	char	certName[_MAX_PATH];

	if (pTemplate)
	{
		std::string hash = getDigest((X509*)pTemplate);
		_snprintf(certName, sizeof(certName), "%s-%s#ss", serverName, hash.c_str());
	} else
	{
		_snprintf(certName, sizeof(certName), "%s", serverName);
	}

	if (loadKeys(certName, k))
	{
		if (addToTrusted)
		{
#ifdef WIN32
			importCertificateToWindowsStorage(k.m_x509.buffer(), k.m_x509.size(), "Root", FALSE);
#endif
		}
		return true;
	}

	if (!createKeys(serverName, (X509*)pTemplate, ca, 
			(m_flags & RSIF_GENERATE_ROOT_PRIVATE_KEY) != 0, 
			k, flags))
		return false;

	if (saveKeys(certName, k))
	{
		if (addToTrusted)
		{
#ifdef WIN32
			importCertificateToWindowsStorage(k.m_x509.buffer(), k.m_x509.size(), "Root", FALSE);
#endif
		}
	}

	return true;
}

bool SSLDataProvider::getSignedCert(const char * serverName, void * pTemplate, SSLKeys & k, tPF_FilterFlags flags)
{
	AutoLock lock(m_cs);
	SSLKeys parent;
	char	certName[_MAX_PATH];
	std::string hash;

	if (pTemplate)
	{
		hash = getDigest((X509*)pTemplate);
	} else
	{
		hash = "0";
	}

	_snprintf(certName, sizeof(certName), "%s-%s-%s#child", m_rootSSLCertName.c_str(), serverName, hash.c_str());

	if (loadKeys(certName, k)) 
	{
		return true;
	}

	if (!loadKeys(m_rootSSLCertName.c_str(), parent))
	{
		if (!getSelfSignedCert(m_rootSSLCertName.c_str(), NULL, true, true, parent, 0))
			return false;
	}

	if (!createChildKeys(serverName, (X509*)pTemplate, parent, k, flags))
		return false;

	if (!saveKeys(certName, k))
	{
		return false;
	}

	return true;
}

void SSLDataProvider::fixFileName(tPathString & fileName)
{
	for (tPathString::iterator it = fileName.begin(); it != fileName.end(); it++)
	{
#ifdef WIN32
		if (*it == L'*' || *it == L':')
			*it = L'_';
#else
		if (*it == L'*' || *it == L':')
			*it = L'_';
#endif
	}
}

void SSLDataProvider::setRootName(const char * rootName)
{
	AutoLock lock(m_cs);

	m_rootSSLCertName = rootName;
	m_rootSSLCertName += " 2";

	SSLKeys tempKeys;
	
	// Generate a self-signed CA certificate if it doesn't exist yet
	if (!getSelfSignedCert(m_rootSSLCertName.c_str(), NULL, true, true, tempKeys, 0))
		return;

	stopImportThread();

#ifdef WIN32
	std::wstring wRootName = decodeUTF8(m_rootSSLCertName);
	fixFileName(wRootName);
	m_rootSSLCertFile = m_baseFolder + L"/" + wRootName + L".cer";
#else
	tPathString rootNameStr = m_rootSSLCertName;
	fixFileName(rootNameStr);
	m_rootSSLCertFile = m_baseFolder + "/" + rootNameStr + ".cer";
#endif

	IOB cert, pkey;
	if (m_certStorage.getCert(m_rootSSLCertName.c_str(), cert, pkey))
	{
		saveFile(m_rootSSLCertFile, cert);

#ifdef _MACOS
		importCertificateToSystemStorage(rootName, m_rootSSLCertFile.c_str(), cert.buffer(), cert.size());
#endif
	}

	startImportThread();
}

void SSLDataProvider::setRootNameEx(const char * rootName, const char * x509, int x509Len, const char * pkey, int pkeyLen)
{
	AutoLock lock(m_cs);

	m_rootSSLCertName = rootName;
	m_rootSSLCertName += " 2";

	SSLKeys tempKeys;

	tempKeys.m_x509.append(x509, x509Len);
	tempKeys.m_pkey.append(pkey, pkeyLen);
	
	stopImportThread();

#ifdef WIN32
	std::wstring wRootName = decodeUTF8(m_rootSSLCertName);
	fixFileName(wRootName);
	m_rootSSLCertFile = m_baseFolder + L"/" + wRootName + L".cer";
#else
	tPathString rootNameStr = m_rootSSLCertName;
	fixFileName(rootNameStr);
	m_rootSSLCertFile = m_baseFolder + "/" + rootNameStr + ".cer";
#endif

	if (m_certStorage.addCert(m_rootSSLCertName.c_str(), tempKeys.m_x509, tempKeys.m_pkey))
	{
		saveFile(m_rootSSLCertFile, tempKeys.m_x509);

#ifdef _MACOS
		importCertificateToSystemStorage(rootName, m_rootSSLCertFile.c_str(), tempKeys.m_x509.buffer(), tempKeys.m_x509.size());
#endif
	}

	startImportThread();
}

void SSLDataProvider::setRootSSLCertImportFlags(unsigned long flags)
{
	AutoLock lock(m_cs);
	m_flags = flags;
}

tPathString SSLDataProvider::getRootSSLCertFileName()
{
	AutoLock lock(m_cs);
	return m_rootSSLCertFile;
}


void SSLDataProvider::addException(const char * endpoint, bool useCounter)
{
	AutoLock lock(m_cs);

	if (!endpoint)
	{
		return;
	}

	DbgPrint("SSLDataProvider::addException %s", endpoint);

	// Add exception only if a certificate from remote endpoint 
	// was refused twice 

	if (useCounter)
	{
		tDelayedExceptionCounters::iterator it = m_ecMap.find(endpoint);
		if (it == m_ecMap.end())
		{
			DELAYED_EXCEPTION de;

			de.ts = nf_time();
			de.counter = 1;

			m_ecMap[endpoint] = de;
			return;
		} else
		{
			nf_time_t curTime = nf_time();

			if ((curTime - it->second.ts) > EXCEPTION_TIMEOUT)
			{
				if ((curTime - it->second.ts) < EXCEPTION_LONG_TIMEOUT)
				{
					it->second.ts = curTime;
					it->second.counter++;

					if (it->second.counter <= 2)
					{
						return;
					}
				} else
				{
					it->second.ts = curTime;
					it->second.counter = 1;
					return;
				}
			} else
			{
				return;
			}
		}
	}

	DbgPrint("SSLDataProvider::addException added %s", endpoint);

	m_xStorage.addItem(endpoint);
}

void SSLDataProvider::clearExceptions(const char * endpoint)
{
	AutoLock lock(m_cs);

	if (!endpoint)
	{
		return;
	}

	DbgPrint("SSLDataProvider::clearExceptions %s", endpoint);

	m_ecMap.erase(endpoint);
}

void SSLDataProvider::clearTlsExceptions(const char * endpoint)
{
	AutoLock lock(m_cs);

	if (!endpoint)
	{
		return;
	}

	DbgPrint("SSLDataProvider::clearTlsExceptions %s", endpoint);

	m_ecTlsMap.erase(endpoint);
}

bool SSLDataProvider::isException(const char * endpoint)
{
	if (!endpoint)
	{
		return false;
	}

	return m_xStorage.itemExists(endpoint, getExceptionsTimeout(EXC_GENERIC));
}

void SSLDataProvider::addTlsException(const char * endpoint)
{
	AutoLock lock(m_cs);

	if (!endpoint)
	{
		return;
	}

	DbgPrint("SSLDataProvider::addTlsException %s", endpoint);

	tExceptionCounters::iterator it = m_ecTlsMap.find(endpoint);
	if (it == m_ecTlsMap.end())
	{
		m_ecTlsMap[endpoint] = 1;
	}
}

bool SSLDataProvider::isTlsException(const char * endpoint)
{
	AutoLock lock(m_cs);

	if (!endpoint)
	{
		return false;
	}

	tExceptionCounters::iterator it = m_ecTlsMap.find(endpoint);
	if (it != m_ecTlsMap.end())
	{
		return true;
	}

	return m_xtlsStorage.itemExists(endpoint, getExceptionsTimeout(EXC_TLS));
}

void SSLDataProvider::confirmTlsException(const char * endpoint)
{
	AutoLock lock(m_cs);

	if (!endpoint)
	{
		return;
	}

	tExceptionCounters::iterator it = m_ecTlsMap.find(endpoint);
	if (it == m_ecTlsMap.end())
	{
		return;
	}

	m_xtlsStorage.addItem(endpoint);
}


void SSLDataProvider::startImportThread()
{
	stopImportThread();

	m_importThread.create(importCertificateThread, (void*)this);
}

void SSLDataProvider::stopImportThread()
{
	if (!m_importThread.isCreated())
		return;

	m_importThreadEvent.fire(1);
	m_importThread.wait();
}

void SSLDataProvider::waitForImportCompletion()
{
	if (!m_importThread.isCreated())
		return;

	m_importThread.wait();
}


void SSLDataProvider::importCertificateThread()
{
	if (m_flags & RSIF_IMPORT_TO_MOZILLA_AND_OPERA)
	{
#ifdef WIN32
		importCertificateToMozillaAndOpera(m_rootSSLCertFile, m_importThreadEvent);
#else
		importCertificateToMozilla(m_rootSSLCertFile, m_importThreadEvent);
#endif
	}

#ifdef WIN32
	if (m_flags & RSIF_IMPORT_TO_PIDGIN)
	{
		importCertificateToPidgin(m_rootSSLCertFile, m_importThreadEvent);
	}
#endif
}

NFTHREAD_FUNC SSLDataProvider::importCertificateThread(void * pThis)
{
	if (!pThis)
		return 0;
	((SSLDataProvider*)pThis)->importCertificateThread();
	return 0;
}

bool SSLDataProvider::isCertificateExists(const char * serverName)
{
	return m_certStorage.isCertExists(serverName);
}

bool SSLDataProvider::getCertLastVerifyTime(const char * serverName, void * pTemplate, unsigned __int64 & ts)
{
	char	certName[_MAX_PATH];
	std::string hash;

	if (pTemplate)
	{
		hash = getDigest((X509*)pTemplate);
	} else
	{
		hash = "0";
	}

	_snprintf(certName, sizeof(certName), "%s-%s-%s#child", m_rootSSLCertName.c_str(), serverName, hash.c_str());

	return m_certStorage.getCertLastVerifyTime(certName, ts);
}

bool SSLDataProvider::setCertLastVerifyTime(const char * serverName, void * pTemplate, unsigned __int64 ts)
{
	char	certName[_MAX_PATH];
	std::string hash;

	if (pTemplate)
	{
		hash = getDigest((X509*)pTemplate);
	} else
	{
		hash = "0";
	}

	_snprintf(certName, sizeof(certName), "%s-%s-%s#child", m_rootSSLCertName.c_str(), serverName, hash.c_str());

	return m_certStorage.setCertLastVerifyTime(certName, ts);
}

#ifdef WIN32

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

DWORD SSLDataProvider::verifyCertificate(const tCertificateList & list, DWORD flags, IOB & ocsp)
{
    PCCERT_CONTEXT			 pServerCert;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA_EX          ChainPara;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
	wchar_t					 szwStore[] = L"MY";
    LPSTR rgszUsages[] = {  szOID_PKIX_KP_SERVER_AUTH,
                            szOID_SERVER_GATED_CRYPTO,
                            szOID_SGC_NETSCAPE };
    DWORD cUsages = sizeof(rgszUsages) / sizeof(LPSTR);
    DWORD   Status = SEC_E_OK;

	HCERTSTORE hStore = NULL;
	BOOL bResult;

	if (list.size() < 1)
		return SEC_E_CERT_UNKNOWN;

     // Open Certificate Store
     hStore = CertOpenStore(CERT_STORE_PROV_MEMORY,
                      ENCODING,
                      0,
                      CERT_SYSTEM_STORE_CURRENT_USER,
                      (LPVOID)szwStore);
     if (!hStore)
     {
	     Status = SEC_E_INTERNAL_ERROR;
		 goto cleanup;
     }

	 bResult = CertAddEncodedCertificateToStore(hStore,
									 X509_ASN_ENCODING,
									 (LPBYTE)&(list[0][0]),
									 (DWORD)list[0].size(),
									 CERT_STORE_ADD_REPLACE_EXISTING,
									 &pServerCert);
	 if (!bResult)
	 {
	     Status = SEC_E_INTERNAL_ERROR;
		 goto cleanup;
	 }

	 for (int i=1; i<(int)list.size(); i++)
	 {
	    PCCERT_CONTEXT pCert;

		 // Add Certificate to store
		 bResult = CertAddEncodedCertificateToStore(hStore,
										 X509_ASN_ENCODING,
										 (LPBYTE)&(list[i][0]),
										 (DWORD)list[i].size(),
										 CERT_STORE_ADD_REPLACE_EXISTING,
										 &pCert);
		 if (!bResult)
		 {
		     Status = SEC_E_INTERNAL_ERROR;
			 goto cleanup;
		 }

		if (pCert)
		{
			CertFreeCertificateContext(pCert);
		}
	 }

	
	if(pServerCert == NULL)
    {
        Status = SEC_E_WRONG_PRINCIPAL;
        goto cleanup;
    }

    //
    // Build certificate chain.
    //

    ZeroMemory(&ChainPara, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
    ChainPara.RequestedUsage.Usage.cUsageIdentifier     = cUsages;
    ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;
	ChainPara.dwUrlRetrievalTimeout = 5000;

	CERT_STRONG_SIGN_SERIALIZED_INFO_EX strong_signed_info;
	memset(&strong_signed_info, 0, sizeof(strong_signed_info));
	strong_signed_info.dwFlags = 0;  // Don't check OCSP or CRL signatures.
	strong_signed_info.pwszCNGSignHashAlgids =
	    L"RSA/SHA256;RSA/SHA384;RSA/SHA512;"
	    L"ECDSA/SHA256;ECDSA/SHA384;ECDSA/SHA512";
	strong_signed_info.pwszCNGPubKeyMinBitLengths = L"RSA/1024;ECDSA/256";
	
	CERT_STRONG_SIGN_PARA_EX strong_sign_params;
	memset(&strong_sign_params, 0, sizeof(strong_sign_params));
	strong_sign_params.cbSize = sizeof(strong_sign_params);
	strong_sign_params.dwInfoChoice = CERT_STRONG_SIGN_SERIALIZED_INFO_CHOICE;
	strong_sign_params.pSerializedInfo = &strong_signed_info;
	
	ChainPara.dwStrongSignFlags = 0;
	ChainPara.pStrongSignPara = &strong_sign_params;

	if (ocsp.size() > 0) 
	{
		CRYPT_DATA_BLOB ocsp_response_blob;
		ocsp_response_blob.cbData = ocsp.size();
		ocsp_response_blob.pbData =	reinterpret_cast<BYTE*>(ocsp.buffer());
    
		CertSetCertificateContextProperty(
			pServerCert, CERT_OCSP_RESPONSE_PROP_ID,
			CERT_SET_PROPERTY_IGNORE_PERSIST_ERROR_FLAG, &ocsp_response_blob);

		DbgPrint("Used OCSP response");
	}

    if (!CertGetCertificateChain(
                            NULL,
                            pServerCert,
                            NULL,
                            pServerCert->hCertStore,
                            (CERT_CHAIN_PARA*)&ChainPara,
                            flags,
                            NULL,
                            &pChainContext))
    {
        Status = GetLastError();
        goto cleanup;
    }

	if (pChainContext->TrustStatus.dwErrorStatus & 
		(CERT_TRUST_IS_OFFLINE_REVOCATION | CERT_TRUST_REVOCATION_STATUS_UNKNOWN))  
	{
		if (pChainContext)
		{
			CertFreeCertificateChain(pChainContext);
		}

		if (!CertGetCertificateChain(
								NULL,
								pServerCert,
								NULL,
								pServerCert->hCertStore,
								(CERT_CHAIN_PARA*)&ChainPara,
								CERT_CHAIN_CACHE_END_CERT | CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY,
								NULL,
								&pChainContext))
		{
			Status = GetLastError();
			goto cleanup;
		}
	}
	
	if (pChainContext->TrustStatus.dwErrorStatus)
    {
        Status = pChainContext->TrustStatus.dwErrorStatus;
        goto cleanup;
    }

    //
    // Validate certificate chain.
    // 

	SSL_EXTRA_CERT_CHAIN_POLICY_PARA sslPara;

	memset(&sslPara, 0, sizeof(sslPara));
	sslPara.cbSize = sizeof(sslPara);
	sslPara.dwAuthType = AUTHTYPE_SERVER;
	sslPara.fdwChecks = 0;

	memset(&PolicyPara, 0, sizeof(PolicyPara));
    PolicyPara.cbSize            = sizeof(PolicyPara);
	PolicyPara.dwFlags = 0x1000;
	PolicyPara.pvExtraPolicyPara = &sslPara;


    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);

    if(!CertVerifyCertificateChainPolicy(
                            CERT_CHAIN_POLICY_SSL,
                            pChainContext,
                            &PolicyPara,
                            &PolicyStatus))
    {
        Status = GetLastError();
        goto cleanup;
    }

    if(PolicyStatus.dwError)
    {
        Status = PolicyStatus.dwError;
        goto cleanup;
    }


    Status = SEC_E_OK;

cleanup:

    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }

	if (pServerCert)
	{
		CertFreeCertificateContext(pServerCert);
	}

    if (hStore) 
	{
		CertCloseStore(hStore, 0);
	}

	return Status;
}

#endif

int SSLDataProvider::verifyCertificate_OpenSSL(const tCertificateList & list, IOB & ocsp)
{
	int status = X509_V_OK;

	if (list.size() < 1)
	{
		return X509_V_ERR_UNSPECIFIED;
	}

	if (m_rootCaStore == NULL)
	{
		return X509_V_ERR_UNSPECIFIED;
	}

    X509_STORE_CTX * rootCaStoreCtx = NULL;
    STACK_OF(X509) * chain = NULL;
	X509 * pCert = NULL, * pTmpCert = NULL;
	const char * p;
    int err;

	for (;;)
	{
		rootCaStoreCtx = X509_STORE_CTX_new();
		if (rootCaStoreCtx == NULL) 
		{
			status = X509_V_ERR_UNSPECIFIED;
			break;
		}
		
		if ((chain = sk_X509_new_null()) == NULL) 
		{
			status = X509_V_ERR_UNSPECIFIED;
			break;
		}

		p = &((*list.begin())[0]);
		pCert = d2i_X509(NULL, (const unsigned char**) &p, (long)list.begin()->size());

		for (tCertificateList::const_iterator it = list.begin(); it != list.end(); it++)
		{
			p = &(*it)[0];
			pTmpCert = d2i_X509(NULL, (const unsigned char**) &p, (long)it->size());
			if (pTmpCert)
			{
				sk_X509_push(chain, pTmpCert); 
			}
		}

		{
			AutoLock lock(m_cs);
			if (!X509_STORE_CTX_init(rootCaStoreCtx, m_rootCaStore, pCert, chain)) 
			{
				status = X509_V_ERR_UNSPECIFIED;
				break;
			}
		}

		err = X509_verify_cert(rootCaStoreCtx);
		if (err > 0 && X509_STORE_CTX_get_error(rootCaStoreCtx) == X509_V_OK) 
		{
			DbgPrint("SSLDataProvider::verifyCertificate_OpenSSL OK\n");
		} else 
		{
			err = X509_STORE_CTX_get_error(rootCaStoreCtx);
			
			DbgPrint("SSLDataProvider::verifyCertificate_OpenSSL error '%s'[%d]: verification failed\n", 
				X509_verify_cert_error_string(err), err);
			
			status = err;
		}

		break;
	}	

	if (pCert)
	{
		X509_free(pCert);
	}

	if (chain)
	{
		sk_X509_free(chain);
	}

	if (rootCaStoreCtx)
	{
		X509_STORE_CTX_free(rootCaStoreCtx);
	}

	return status;
}

bool SSLDataProvider::loadCAStore(const char * fileName)
{
	AutoLock lock(m_cs);

	X509_STORE* rootCaStore = X509_STORE_new();
	int err;

	if (m_rootCaStore)
	{
		X509_STORE_free(m_rootCaStore);
	}

	err = X509_STORE_load_locations(rootCaStore, fileName, NULL);
	if (!err)
	{
		X509_STORE_free(m_rootCaStore);
		return false;
	}

	m_rootCaStore = rootCaStore;

	return true;
}

#ifdef WIN32
// 
// This function load the certificate from the windows certicate store
// modification of a part of the original source from:
// http://ftp.netbsd.org/pub/NetBSD/NetBSD-current/src/external/bsd/wpa/dist/src/crypto/tls_openssl.c
//
bool SSLDataProvider::loadSystemCaStore()
{
    PCCERT_CONTEXT ctx = NULL;
	X509 *cert;
	char buf[128];
	X509_STORE* rootCaStore = X509_STORE_new();

    STACK_OF(X509) *rootCaList = NULL;
    
	if ((rootCaList = sk_X509_new_null()) == NULL) 
	{
		DbgPrint("OpenSSL: Cannot initialize certificates stack");
        return false;
    }

    HCERTSTORE cs = CertOpenSystemStore(0, "Root");
    if (cs == NULL) 
	{
        DbgPrint("CryptoAPI: failed to open system Root cert store. Error=%d", (int) GetLastError());
        return false;
    }

    while((ctx = CertEnumCertificatesInStore(cs, ctx))) 
    {
        cert = d2i_X509(NULL, (const unsigned char**) &ctx->pbCertEncoded, ctx->cbCertEncoded);
        if (cert == NULL) 
        {
            DbgPrint("CryptoAPI: Could not process X509 DER encoding for Root cert");
            continue;
        }

        X509_NAME_oneline(X509_get_subject_name(cert), buf, sizeof(buf));

		if (X509_cmp_time(X509_get_notBefore(cert), NULL) >= 0 ||
			X509_cmp_time(X509_get_notAfter(cert), NULL) <= 0) 
		{
			DbgPrint("OpenSSL: Expired Root '%s'", buf);
            continue;
		}

		DbgPrint("OpenSSL: Loaded Root certificate from system store. Subject='%s'", buf);

		if (!X509_STORE_add_cert(rootCaStore, cert)) 
		{
            DbgPrint("Failed to add ca_cert to OpenSSL certificate store");
		}
		else 
		{
			sk_X509_push(rootCaList, cert);
		}
	}

	if (!CertCloseStore(cs, 0)) 
	{
        DbgPrint("Failed to close system cert store: error=%d", (int) GetLastError());
	}

	m_rootCaStore = rootCaStore;
	m_rootCaList = rootCaList;
    return true;
}

#endif

#define OCSP_SKEW       5
#define OCSP_MAXAGE     -1

//
// Method to verify OCSP response got from server (OCSP stapling)
//
bool SSLDataProvider::verifyOcspResponse(void *ocsp, STACK_OF(X509) * certs) 
{
    // check that peer cert exits
    if (!(certs && sk_X509_num(certs))) 
	{
        return false;
    }

    X509 *peer = sk_X509_value(certs, 0);
    //-----

    OCSP_RESPONSE *rsp = (OCSP_RESPONSE *)ocsp;
    OCSP_BASICRESP *br = NULL;
//    OCSP_RESPBYTES *rb = rsp->responseBytes;
    long l;

    l = OCSP_response_status(rsp);
    if (l != OCSP_RESPONSE_STATUS_SUCCESSFUL) 
	{
        return false;
    }

	if (!(br = OCSP_response_get1_basic(rsp)))
		return false;

    bool ret = true;
    do {
        // Workaround for ocsp response with no certs for verify
        STACK_OF(X509) *emptyStack = NULL;
		if (!OCSP_resp_get0_certs(br)) 
		{
			DbgPrint("Empty certs in the OCSP basic response");			
			OCSP_basic_add1_cert(br, sk_X509_value(certs, 0));
        }

        // Check that OCSP response is valid
		int result = OCSP_basic_verify(br, certs, m_rootCaStore, 0);

        if (emptyStack) 
		{
            sk_X509_free(emptyStack);
        }

        if (result <= 0) 
		{
			DbgPrint("OCSP basic verify failed");
            ret = false;
            break;
        }

        // Check status of the certificates in response        
        X509 *issuer = NULL;
        issuer = X509_find_by_subject(certs, X509_get_issuer_name(peer));
        if (!issuer) 
		{
            issuer = X509_find_by_subject(m_rootCaList, X509_get_issuer_name(peer));
        }
        
        if (!issuer) 
		{
			DbgPrint("Cannot verify certificate issuer");
            ret = false;
            break;
        }
        
        OCSP_CERTID *id = OCSP_cert_to_id(0, peer, issuer);
        if (!id) 
		{
			DbgPrint("Cannot get OCSP cert id");
            ret = false;
            break;
        }
        
        int reason, status;
        ASN1_GENERALIZEDTIME  *producedAt, *thisUpdate, *nextUpdate;

        if (!OCSP_resp_find_status(br, id, &status, &reason, &producedAt,
                                   &thisUpdate, &nextUpdate))
		{
			DbgPrint("Cannot get OCSP response status");
            ret = false;
            break;
        }

        if (!OCSP_check_validity(thisUpdate, nextUpdate, OCSP_SKEW, OCSP_MAXAGE))
		{
			DbgPrint("OCSP response status is not valid");
            ret = false;
            break;
        }
        
        // Check OCSP response status
		if (status != V_OCSP_CERTSTATUS_GOOD) 
		{
			DbgPrint("OCSP response status is not good");
            ret = false;
		}
    } while (0);

    OCSP_BASICRESP_free(br);
    return ret;
}


DWORD SSLDataProvider::verifyCertificate(unsigned __int64 id, const char * serverName, IOB & ocsp, void * x509, STACK_OF(X509) *certs, bool fastCheck)
{
	std::string	 flagFileName;
	std::string hash;
	DWORD res = 0;

	if (x509)
	{
		hash = getDigest((X509*)x509);
	} else
	{
		hash = "0";
	}

	flagFileName = m_rootSSLCertName + "-" + serverName + "-" + hash;

	if (m_xvStorage.itemExists(flagFileName, getExceptionsTimeout(EXC_CERT_REVOKED)))
	{
		return X509_V_ERR_UNSPECIFIED;
	}

	if (fastCheck)
	{
		return X509_V_OK;
	}

	tCertificateList list;
	if (certs)
	{
		int n;
		unsigned char * p;

		for (int i=0; i<sk_X509_num(certs); i++)
		{
			tBytes bytes;

			n = i2d_X509(sk_X509_value(certs,i), NULL);
			if (n > 0)
			{
				bytes.resize(n);
				p = (unsigned char *)&bytes[0];
				i2d_X509(sk_X509_value(certs,i), &p);
				
				list.push_back(bytes);
			}
		}
	}
/*
	if (ocsp) 
 	{
		DbgPrint("Verifying OCSP response for %s", flagFileName.c_str());
        res = verifyOcspResponse(ocsp, certs) ? SEC_E_OK : SEC_E_WRONG_PRINCIPAL;
		DbgPrint("OCSP verification result is %x", res);
    }
	if (!ocsp || res != SEC_E_OK)
*/	
	{
#ifdef WIN32
		// No OCSP stapling, now do it long way 
		DbgPrint("Start fast verifying certificate for %s", flagFileName.c_str());
		res = verifyCertificate(list, 
			CERT_CHAIN_OPT_IN_WEAK_SIGNATURE | CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY | CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT | CERT_CHAIN_CACHE_END_CERT,
			ocsp);
#else
		res = verifyCertificate_OpenSSL(list, ocsp);
#endif
		DbgPrint("Fast verification result is %x", res);	
 	}
 
	if (res != 0)
	{
		m_xvStorage.addItem(flagFileName);
	} else 
	{
		Proxy & proxy = getProxy();	
		proxy.setSessionState(id, true);

		// Schedule async slow OCSP response check
		VERIFY_INFO * pInfo = new VERIFY_INFO();
		pInfo->flagFileName = flagFileName;
		pInfo->certList = list;
		pInfo->id = id;
 
		{
			AutoLock lock(m_csVerify);
			m_verifyQueue.push(pInfo);
		}

		m_verifyThreadEvent.fire(0);
	}

 	return res;
 }

void SSLDataProvider::verifyCertificate(const VERIFY_INFO & verifyInfo)
{
	DWORD res;
	IOB ocsp;
	Proxy & proxy = getProxy();	

#ifdef WIN32
	res = verifyCertificate(verifyInfo.certList, 
		CERT_CHAIN_OPT_IN_WEAK_SIGNATURE | CERT_CHAIN_CACHE_END_CERT | CERT_CHAIN_REVOCATION_CHECK_CHAIN | CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT,
		ocsp);
#else
	res = verifyCertificate_OpenSSL(verifyInfo.certList, ocsp);
#endif

	if (res != 0)
	{
		m_xvStorage.addItem(verifyInfo.flagFileName);
		proxy.tcpDisconnect(verifyInfo.id, OT_TCP_DISCONNECT_LOCAL);
	}

	proxy.setSessionState(verifyInfo.id, false);
}

void SSLDataProvider::startVerifyThread()
{
	stopVerifyThread();

	m_verifyThread.create(verifyThread, (void*)this);
}

void SSLDataProvider::stopVerifyThread()
{
	if (!m_verifyThread.isCreated())
		return;

	m_verifyThreadEvent.fire(1);
	m_verifyThread.wait();

	while (!m_verifyQueue.empty())
	{
		VERIFY_INFO * p = m_verifyQueue.front();
		m_verifyQueue.pop();
		delete p;
	}
}

void SSLDataProvider::verifyThread()
{
	VERIFY_INFO * pVerifyInfo;
	int flags;

	for (;;)
	{
		m_verifyThreadEvent.wait(&flags);
		if (flags != 0)
			break;

		for (;;)
		{
			{		
				AutoLock lock(m_csVerify);

				if (m_verifyQueue.empty())
					break;

				pVerifyInfo = m_verifyQueue.front();
				m_verifyQueue.pop();
			}

			verifyCertificate(*pVerifyInfo);
			
			delete pVerifyInfo;
		}
	}

}

NFTHREAD_FUNC SSLDataProvider::verifyThread(void * pThis)
{
	if (!pThis)
		return 0;
	((SSLDataProvider*)pThis)->verifyThread();
	return 0;
}

tExceptionTimeout SSLDataProvider::getExceptionsTimeout(eEXCEPTION_CLASS ec)
{
	AutoLock lock(m_cs);

	if (ec > EXC_MAX)
		return 0;

	return m_exceptionsTimeout[ec];
}

void SSLDataProvider::setExceptionsTimeout(eEXCEPTION_CLASS ec, unsigned __int64 ts)
{
	AutoLock lock(m_cs);

	if (ec > EXC_MAX)
		return;

	m_exceptionsTimeout[ec] = ts;
}

void SSLDataProvider::deleteExceptions(eEXCEPTION_CLASS ec)
{
	switch (ec)
	{
	case EXC_GENERIC:
		m_xStorage.clear();
		break;
	case EXC_TLS:
		m_xtlsStorage.clear();
		break;
	case EXC_CERT_REVOKED:
		m_xvStorage.clear();
		break;
	}
}

#endif