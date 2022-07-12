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

#include "SSLSessionData.h"
 
using namespace ProtocolFilters;

SSLSessionData::SSLSessionData(void)
{
	m_pCtx = NULL;
	m_pSSL = NULL;
	m_pbioRead = NULL;
	m_pbioWrite = NULL;
}

SSLSessionData::~SSLSessionData(void)
{
	free();
}

int SSLSessionData::useCertificateBuffer(char * buf, int len, int type)
{
	BIO *	in;
	int		ret = 0;
	X509 *	x = NULL;

	BIO_METHOD * bm;
	
	bm = (BIO_METHOD*)BIO_f_buffer();

	in = BIO_new(bm);

	BIO_set_buffer_read_data(in, buf, len);

	if (in == NULL)
	{
		goto end;
	}

	if (type == SSL_FILETYPE_ASN1)
	{
		x=d2i_X509(&x, (const unsigned char**)&buf, len);
	}
	else if (type == SSL_FILETYPE_PEM)
	{
		x=PEM_read_bio_X509(in,NULL,  SSL_CTX_get_default_passwd_cb(m_pCtx), SSL_CTX_get_default_passwd_cb_userdata(m_pCtx));
	}
	else
	{
		goto end;
	}

	if (x == NULL)
	{
		goto end;
	}

	ret = SSL_CTX_use_certificate(m_pCtx,x);

end:

	if (x != NULL) 
		X509_free(x);
	
	if (in != NULL) 
		BIO_free(in);
	
	return (ret);
}



int SSLSessionData::usePrivateKeyBuffer(char * buf, int len, int type)
{
	int ret=0;
	BIO *in;
	EVP_PKEY *pkey=NULL;

	BIO_METHOD * bm;
	
	bm = (BIO_METHOD*)BIO_f_buffer();

	in = BIO_new(bm);

	BIO_set_buffer_read_data(in, buf, len);

	if (in == NULL)
	{
		goto end;
	}

	if (type == SSL_FILETYPE_ASN1)
	{
		pkey=d2i_AutoPrivateKey(&pkey,(const unsigned char**)&buf,len);
	} else
	if (type == SSL_FILETYPE_PEM)
	{
		pkey=PEM_read_bio_PrivateKey(in,NULL,
			 SSL_CTX_get_default_passwd_cb(m_pCtx), SSL_CTX_get_default_passwd_cb_userdata(m_pCtx));
	} else
	{
		goto end;
	}

	if (pkey == NULL)
	{
		goto end;
	}
	
	ret = SSL_CTX_use_PrivateKey(m_pCtx,pkey);

	EVP_PKEY_free(pkey);

end:
	if (in != NULL) 
		BIO_free(in);
	
	return(ret);
}


bool SSLSessionData::init(bool bTls, bool bServer, SSLKeys * pkeys, int ssl_version, tPF_FilterFlags flags)
{
	int n;

	if (bServer)
	{
		if (!pkeys)
			return false;

		m_pCtx = SSL_CTX_new(TLS_server_method());
		if (!m_pCtx)
			return false;

		n = useCertificateBuffer(
						pkeys->m_x509.buffer(), 
						pkeys->m_x509.size(), 
						SSL_FILETYPE_ASN1);
		if (n <= 0)
			goto error;

		n = usePrivateKeyBuffer(
						pkeys->m_pkey.buffer(), 
						pkeys->m_pkey.size(), 
						SSL_FILETYPE_ASN1);
		if (n <= 0)
			goto error;

//		SSL_CTX_set_ecdh_auto(m_pCtx, 1);

		SSL_CTX_set_options(m_pCtx, SSL_OP_NO_COMPRESSION);
		SSL_CTX_set_options(m_pCtx, SSL_OP_ALL);

		m_pSSL = SSL_new(m_pCtx);
		if (!m_pSSL)
			goto error; 

		m_pbioRead = BIO_new(BIO_s_mem());
		if (!m_pbioRead)
			goto error;

		m_pbioWrite = BIO_new(BIO_s_mem());
		if (!m_pbioWrite)
			goto error;

		SSL_set_bio(m_pSSL, m_pbioRead, m_pbioWrite);

		SSL_set_accept_state(m_pSSL);
		
		m_buffer.reset();
		m_buffer.append(NULL, DEFAULT_BUFFER_SIZE, false);

		return true;
	} else
	{
		if (bTls)
		{
			m_pCtx = SSL_CTX_new(TLS_client_method());
			if (!m_pCtx)
				return false;

			SSL_CTX_set_options(m_pCtx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

			if (flags & FF_SSL_DISABLE_TLS_1_0)
			{
				SSL_CTX_set_options(m_pCtx, SSL_OP_NO_TLSv1);
			}

			if (flags & FF_SSL_DISABLE_TLS_1_1)
			{
				SSL_CTX_set_options(m_pCtx, SSL_OP_NO_TLSv1_1);
			}

			if (ssl_version < 0x304)
			{
				SSL_CTX_set_options(m_pCtx, SSL_OP_NO_TLSv1_3);
			}
			if (ssl_version < 0x303)
			{
				SSL_CTX_set_options(m_pCtx, SSL_OP_NO_TLSv1_2);
			}
			if (ssl_version < 0x302)
			{
				SSL_CTX_set_options(m_pCtx, SSL_OP_NO_TLSv1_1);
			}
		} else
		{
			m_pCtx = SSL_CTX_new(TLS_client_method());
			if (!m_pCtx)
				return false;
		}

		SSL_CTX_set_options(m_pCtx, SSL_OP_ALL);
		SSL_CTX_set_options(m_pCtx, SSL_OP_NO_COMPRESSION);

		m_pSSL = SSL_new(m_pCtx);
		if (!m_pSSL)
			goto error;

		m_pbioRead = BIO_new(BIO_s_mem());
		if (!m_pbioRead)
			goto error;

		m_pbioWrite = BIO_new(BIO_s_mem());
		if (!m_pbioWrite)
			goto error;

		SSL_set_bio(m_pSSL, m_pbioRead, m_pbioWrite);

		SSL_set_connect_state(m_pSSL);

		m_buffer.reset();
		m_buffer.append(NULL, DEFAULT_BUFFER_SIZE, false);

		return true;
	}

error:
	free();
	
	return false;
}

void SSLSessionData::free()
{
	if (m_pSSL)
	{
		SSL_free(m_pSSL);
		m_pSSL = NULL;
	}

	if (m_pCtx)
	{
		SSL_CTX_free(m_pCtx);
		m_pCtx = NULL;
	}
}

#endif