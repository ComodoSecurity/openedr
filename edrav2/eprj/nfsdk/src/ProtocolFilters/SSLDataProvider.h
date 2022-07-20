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

#include <stdlib.h>
#include <map>
#include <set>
#include <queue>
#include <string>
#include <openssl/x509.h>
#ifdef WIN32
#include <io.h>
#endif
#include "IOB.h"
#include "sync.h"
#include "PFFilterDefs.h"
#include "thread.h"

namespace ProtocolFilters
{

#ifdef WIN32
	typedef std::wstring tPathString;
#else
	typedef std::string tPathString;
#endif


	class SSLKeys
	{
	public:
		SSLKeys() {}

		SSLKeys(const SSLKeys & k)
		{
			*this = k;
		}

		SSLKeys & operator = (const SSLKeys & k)
		{
			m_x509 = k.m_x509;
			m_pkey = k.m_pkey;
			return *this;
		}

		IOB		m_x509;
		IOB		m_pkey;
	};

	#define CACHE_TIMEOUT (10 * 60)
	#define CACHE_TIMEOUT_CHECK_PERIOD (10 * 60)

	typedef std::vector<char> tBytes;
	typedef std::vector<tBytes> tCertificateList;

	struct VERIFY_INFO
	{
		VERIFY_INFO()
		{
		}
		VERIFY_INFO(const VERIFY_INFO & v)
		{
			*this = v;
		}
		VERIFY_INFO & operator = (const VERIFY_INFO & v)
		{
			flagFileName = v.flagFileName;
			certList = v.certList;
			id = v.id;
			return *this;
		}

		std::string		flagFileName;
		tCertificateList	certList;
		unsigned __int64 id;
	};

	int crypt_buffer(unsigned char * buf, int len, int encrypt);

	class CertStorage
	{
	public:
		CertStorage()
		{
			m_hFile = NULL;
		}

		~CertStorage()
		{
			free();
		}

		bool init(const tPathString & fileName)
		{
			AutoLock lock(m_cs);

			free();

#ifdef WIN32
			m_hFile = _wfopen(fileName.c_str(), L"r+b");
			if (!m_hFile)
			{
				m_hFile = _wfopen(fileName.c_str(), L"w+b");
				if (!m_hFile)
					return false;
			}
#else
			m_hFile = fopen(fileName.c_str(), "r+b");
			if (!m_hFile)
			{
				m_hFile = fopen(fileName.c_str(), "w+b");
				if (!m_hFile)
					return false;
			}
#endif		
			m_lastCacheCheckTime = 0;

			loadMap();

			return true;
		}

		void free()
		{
			AutoLock lock(m_cs);

			if (m_hFile)
			{
				fclose(m_hFile);
				m_hFile = NULL;
			}

			for (tCertStorageMap::iterator it = m_certStorageMap.begin();
				it != m_certStorageMap.end(); it++)
			{
				if (it->second.pKeys)
					delete it->second.pKeys;
			}
			m_certStorageMap.clear();
		}

		inline std::string strToLower(const std::string& str)
		{
			std::string result;
			result.reserve(str.size()); // optimisation

			for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
			{
				result.append(1, (char) ::tolower(*it));
			}

			return result;
		}

		bool getCert(const std::string & name, IOB & cert, IOB & pkey)
		{
			AutoLock lock(m_cs);

			CERT_ITEM_INFO cii;
			IOB buf;
			
			deleteUnusedKeys();

			tCertStorageMap::iterator it = m_certStorageMap.find(strToLower(name));
			if (it == m_certStorageMap.end())
				return false;

			if (it->second.pKeys)
			{
				it->second.lastUseTime = nf_time();
				cert = it->second.pKeys->m_x509;
				pkey = it->second.pKeys->m_pkey;
				return true;
			}

			fseek(m_hFile, it->second.fileOffset, SEEK_SET);

			if (!read(&cii, sizeof(cii)))
				return false;

			if (!buf.resize(cii.certLength + cii.pkeyLength))
				return false;

			fseek(m_hFile, cii.nameLength, SEEK_CUR);

			if (!read(buf.buffer(), buf.size()))
				return false;

			crypt_buffer((unsigned char*)buf.buffer(), buf.size(), 0);

			if (!cert.append(buf.buffer(), cii.certLength))
				return false;

			if (!pkey.append(buf.buffer() + cii.certLength, cii.pkeyLength))
				return false;

			it->second.lastUseTime = nf_time();

			it->second.pKeys = new SSLKeys();
			it->second.pKeys->m_x509 = cert;
			it->second.pKeys->m_pkey = pkey;

			return true;
		}

		bool addCert(const std::string & name, IOB & cert, IOB & pkey)
		{
			AutoLock lock(m_cs);

			CERT_ITEM_INFO cii;
			int align = 0;
			unsigned int offset = 0;
			IOB buf;
			std::string lcName = strToLower(name);

			if (name.empty() || 
				cert.size() == 0 ||
				pkey.size() == 0)
			{
				return false;
			}

			deleteCert(name);

			fseek(m_hFile, 0, SEEK_END);
			offset = ftell(m_hFile);
			
			if (!buf.append(cert.buffer(), cert.size()))
				return false;

			if (!buf.append(pkey.buffer(), pkey.size()))
				return false;

			if (buf.size() % 16)
			{
				align = 16 - (buf.size() % 16);

				char zero[16] = {0};
				if (!buf.append(zero, align))
					return false;
			}

			crypt_buffer((unsigned char*)buf.buffer(), buf.size(), 1);

			cii.nameLength = (unsigned short)name.length();
			cii.certLength = (unsigned short)cert.size();
			cii.pkeyLength = (unsigned short)pkey.size();
			cii.totalLength = (unsigned short)(sizeof(cii) + cii.nameLength + buf.size());
			cii.lastVerifyTime = 0;

			if (!write(&cii, sizeof(cii)))
				return false;

			if (!write(&lcName[0], lcName.length()))
				return false;

			if (!write(buf.buffer(), buf.size()))
				return false;

			CERT_KEYS ck;

			ck.fileOffset = offset;
			ck.lastVerifyTime = 0;
			ck.lastUseTime = nf_time();
			ck.pKeys = new SSLKeys();
			ck.pKeys->m_x509 = cert;
			ck.pKeys->m_pkey = pkey;

			m_certStorageMap[lcName] = ck;

			fflush(m_hFile);

			return true;
		}

		bool deleteCert(const std::string & name)
		{
			AutoLock lock(m_cs);

			CERT_ITEM_INFO cii;
			std::string lcName = strToLower(name);

			tCertStorageMap::iterator it = m_certStorageMap.find(lcName);
			if (it == m_certStorageMap.end())
				return false;

			fseek(m_hFile, it->second.fileOffset, SEEK_SET);

			if (!read(&cii, sizeof(cii)))
				return false;

			fseek(m_hFile, it->second.fileOffset, SEEK_SET);

			cii.nameLength = 0;

			if (!write(&cii, sizeof(cii)))
				return false;

			if (it->second.pKeys)
				delete it->second.pKeys;

			m_certStorageMap.erase(it);

			fflush(m_hFile);

			return true;
		}

		bool isCertExists(const std::string & name)
		{
			AutoLock lock(m_cs);

			std::string lcName = strToLower(name);

			return m_certStorageMap.find(lcName) != m_certStorageMap.end();
		}

		bool getCertLastVerifyTime(const std::string & name, unsigned __int64 & ts)
		{
			AutoLock lock(m_cs);

			tCertStorageMap::iterator it = m_certStorageMap.find(strToLower(name));
			if (it == m_certStorageMap.end())
				return false;

			ts = it->second.lastVerifyTime;

			return true;
		}

		bool setCertLastVerifyTime(const std::string & name, unsigned __int64 ts)
		{
			AutoLock lock(m_cs);

			CERT_ITEM_INFO cii;
			IOB buf;
			
			tCertStorageMap::iterator it = m_certStorageMap.find(strToLower(name));
			if (it == m_certStorageMap.end())
				return false;

			it->second.lastVerifyTime = ts;

			fseek(m_hFile, it->second.fileOffset, SEEK_SET);

			if (!read(&cii, sizeof(cii)))
				return false;

			cii.lastVerifyTime = ts;

			fseek(m_hFile, it->second.fileOffset, SEEK_SET);

			if (!write(&cii, sizeof(cii)))
				return false;

			fflush(m_hFile);

			return true;
		}

	protected:

		void deleteUnusedKeys()
		{
			nf_time_t curTime = nf_time();

			if ((curTime - m_lastCacheCheckTime) < CACHE_TIMEOUT_CHECK_PERIOD)
				return;

			m_lastCacheCheckTime = curTime;

			for (tCertStorageMap::iterator it = m_certStorageMap.begin();
				it != m_certStorageMap.end(); it++)
			{
				if (it->second.pKeys && 
					(curTime - it->second.lastUseTime) > CACHE_TIMEOUT)
				{
					delete it->second.pKeys;
					it->second.pKeys = NULL;
				}
			}
		}

		bool loadMap()
		{
			CERT_ITEM_INFO cii;
			std::string name;
			unsigned int offset = 0;

			fseek(m_hFile, 0, SEEK_SET);

			while (read(&cii, sizeof(cii)))
			{
				if (cii.nameLength == 0)
				{
					if (cii.totalLength == 0)
					{
						_chsize(_fileno(m_hFile), offset);
						break;
					}

					offset += cii.totalLength;
					fseek(m_hFile, offset, SEEK_SET);
					continue;
				}

				name.resize(cii.nameLength);
				
				if (!read(&name[0], cii.nameLength))
					break;

				CERT_KEYS ck;
				ck.fileOffset = offset;
				ck.lastUseTime = 0;
				ck.lastVerifyTime = cii.lastVerifyTime;
				ck.pKeys = NULL;

				m_certStorageMap[name] = ck;
				
				offset += cii.totalLength;
				
				fseek(m_hFile, offset, SEEK_SET);
			}

			return true;
		}

		bool read(void * buf, size_t len)
		{
			if (fread(buf, 1, len, m_hFile) != len)
				return false;

			return true;
		}

		bool write(const void * buf, size_t len)
		{
			if (fwrite(buf, 1, len, m_hFile) != len)
				return false;

			return true;
		}

	#pragma pack(push, 1)
		typedef struct _CERT_ITEM_INFO
		{
			unsigned short totalLength;
			unsigned short nameLength;
			unsigned short certLength;
			unsigned short pkeyLength;
			unsigned __int64 lastVerifyTime;
		} CERT_ITEM_INFO, *PCERT_ITEM_INFO;
	#pragma pack(pop)

		typedef struct _CERT_KEYS
		{
			unsigned int		fileOffset;
			nf_time_t			lastUseTime;
			unsigned __int64	lastVerifyTime;
			SSLKeys	* pKeys;
		} CERT_KEYS, *PCERT_KEYS;

		typedef std::map<std::string, CERT_KEYS> tCertStorageMap;
		tCertStorageMap m_certStorageMap;
		nf_time_t m_lastCacheCheckTime;

		FILE * m_hFile;
		AutoCriticalSection m_cs;
	};

	typedef unsigned __int64 tExceptionTimeout;

	class ExceptionStorage
	{
	public:
		ExceptionStorage()
		{
		}

		~ExceptionStorage()
		{
		}

		bool init(const tPathString & fileName)
		{
			AutoLock lock(m_cs);

			m_fileName = fileName;

			m_storage.clear();

			if (!load())
			{
				m_storage.clear();
			}

			return true;
		}

		void free()
		{
			AutoLock lock(m_cs);

			save();
			m_storage.clear();
		}

		void clear()
        {
            AutoLock lock(m_cs);
            m_storage.clear();
            save();
        }

		inline std::string strToLower(const std::string& str)
		{
			std::string result;
			result.reserve(str.size()); 

			for (std::string::const_iterator it = str.begin(); it != str.end(); it++)
			{
				result.append(1, (char) ::tolower(*it));
			}

			return result;
		}

		bool addItem(const std::string & name)
		{
			AutoLock lock(m_cs);

			if (name.empty())
			{
				return false;
			}

			std::string lcName = strToLower(name);

			tStorage::iterator it = m_storage.find(lcName);
			if (it != m_storage.end())
			{
				it->second.ts = time(NULL);
				return true;
			}

			EXCEPTION_INFO ei;
			ei.ts = time(NULL);

			m_storage[lcName]  = ei;
			return true;
		}

		bool itemExists(const std::string & name, tExceptionTimeout timeout)
		{
			std::string lcName = strToLower(name);

			AutoLock lock(m_cs);

			tStorage::iterator it = m_storage.find(lcName);
			if (it == m_storage.end())
				return false;

			if (timeout > 0)
			{
				time_t ts = time(NULL);
				if ((ts - it->second.ts) > timeout)
				{
					m_storage.erase(it);
					return false;
				}
			}

			return true;
		}

	protected:

		bool load()
		{
			std::string name;
			EXCEPTION_INFO ei;
			unsigned int len;
			bool res = true;

#ifdef WIN32
			FILE * hFile = _wfopen(m_fileName.c_str(), L"r+b");
#else
			FILE * hFile = fopen(m_fileName.c_str(), "r+b");
#endif
			if (!hFile)
			{
				return false;
			}

			while (fread(&len, 1, sizeof(len), hFile) == sizeof(len))
			{
				if (len > 2000)
				{
					res = false;
					break;
				}

				name.resize(len);
				
				if (fread(&ei, 1, sizeof(ei), hFile) != sizeof(ei))
				{
					res = false;
					break;
				}

				if (fread(&name[0], 1, len, hFile) != len)
				{
					res = false;
					break;
				}

				m_storage[name] = ei;
			}

			fclose(hFile);

			return res;
		}

		bool save()
		{
			unsigned int len;
			bool res = true;

#ifdef WIN32
			FILE * hFile = _wfopen(m_fileName.c_str(), L"wb");
#else
			FILE * hFile = fopen(m_fileName.c_str(), "wb");
#endif
			if (!hFile)
			{
				return false;
			}

			tStorage::iterator it;
			for (it = m_storage.begin(); it != m_storage.end(); it++)
			{
				len = (unsigned int)it->first.length();
				if (fwrite(&len, 1, sizeof(len), hFile) != sizeof(len))
				{
					res = false;
					break;
				}

				if (fwrite(&it->second, 1, sizeof(EXCEPTION_INFO), hFile) != sizeof(EXCEPTION_INFO))
				{
					res = false;
					break;
				}

				if (fwrite(it->first.c_str(), 1, it->first.length(), hFile) != it->first.length())
				{
					res = false;
					break;
				}
			}

			fclose(hFile);

			return res;
		}

		typedef struct _EXCEPTION_INFO
		{
			tExceptionTimeout ts;
		} EXCEPTION_INFO, *PEXCEPTION_INFO;

		typedef std::map<std::string, EXCEPTION_INFO> tStorage;
		tStorage m_storage;

		tPathString m_fileName;

		AutoCriticalSection m_cs;
	};



	class SSLDataProvider 
	{
	public:
		SSLDataProvider(void);
		~SSLDataProvider(void);

		bool init(const tPathString & baseFolder);
		void free();

		bool getSelfSignedCert(const char * serverName, void * pTemplate, bool addToTrusted, bool ca, SSLKeys & k, tPF_FilterFlags flags);
		bool getSignedCert(const char * serverName, void * pTemplate, SSLKeys & k, tPF_FilterFlags flags);
	
		void addException(const char * endpoint, bool useCounter = true);
		bool isException(const char * endpoint);
		void clearExceptions(const char * endpoint);
		
		void clearTlsExceptions(const char * endpoint);

		void addTlsException(const char * endpoint);
		bool isTlsException(const char * endpoint);
		void confirmTlsException(const char * endpoint);

		void setRootName(const char * rootName);
		void setRootNameEx(const char * rootName, const char * x509, int x509Len, const char * pkey, int pkeyLen);
		void setRootSSLCertImportFlags(unsigned long flags);

		bool isCertificateExists(const char * serverName);

		bool getCertLastVerifyTime(const char * serverName, void * pTemplate, unsigned __int64 & ts);
		bool setCertLastVerifyTime(const char * serverName, void * pTemplate, unsigned __int64 ts);

#ifdef WIN32
		DWORD verifyCertificate(const tCertificateList & list, DWORD flags, IOB & ocsp);
#endif
		DWORD verifyCertificate(unsigned __int64 id, const char * serverName, IOB & ocsp, void * x509, STACK_OF(X509) *certs, bool fastCheck);
		void verifyCertificate(const VERIFY_INFO & verifyInfo);

		bool loadCAStore(const char * fileName);

		int verifyCertificate_OpenSSL(const tCertificateList & list, IOB & ocsp);

		void waitForImportCompletion();

		tExceptionTimeout getExceptionsTimeout(eEXCEPTION_CLASS ec);
		void setExceptionsTimeout(eEXCEPTION_CLASS ec, tExceptionTimeout ts);
		void deleteExceptions(eEXCEPTION_CLASS ec);

		tPathString getRootSSLCertFileName();

	protected:
		bool loadKeys(const char * serverName, SSLKeys & k);

		bool saveKeys(const char * serverName, SSLKeys & k);
		bool saveFile(const tPathString & fileName, IOB & iob);

		void fixFileName(tPathString & fileName);

		void startImportThread();
		void stopImportThread();
		void importCertificateThread();
		static NFTHREAD_FUNC importCertificateThread(void * pThis);
		void startVerifyThread();
		void stopVerifyThread();
		void verifyThread();
		static NFTHREAD_FUNC verifyThread(void * pThis);

		bool verifyOcspResponse(void *ocsp, STACK_OF(X509) * certs);

#ifdef WIN32
		bool loadSystemCaStore();
#endif
		int _add_ext(X509 *cert, int nid, const char *value);
		int _delete_ext(X509 *cert, int nid);
		int rand_serial(BIGNUM *b, ASN1_INTEGER *ai);
		int copy_extensions(X509 *dst, X509 *src);
		bool hostMatches(const char *hostname, const char *certHostname);
		bool checkSubjectName(const char *szName, X509 * x);

		int _mkcert(const char *szName, 
			X509 * pTemplate, 
			X509 **x509p, EVP_PKEY **pkeyp, 
			int bits, int days, bool ca, bool ownPrivKey, tPF_FilterFlags flags);

		int _mkcert_child(const char *szName, 
				  X509 * pTemplate, 
				  X509 **x509p, EVP_PKEY **pkeyp, 
				  X509 *x509Parent, EVP_PKEY * pkeyParent, 
				  int bits, int days, tPF_FilterFlags flags);

		bool createKeys(const char * szName, X509 * pTemplate, bool ca, bool ownPrivKey, SSLKeys & k, tPF_FilterFlags flags);
		bool createChildKeys(const char * szName, X509 * pTemplate, const SSLKeys & parent, SSLKeys & k, tPF_FilterFlags flags);

	private:
		tPathString m_baseFolder;
		bool	m_initialized;
		std::string	m_rootSSLCertName;
		
		tPathString m_rootSSLCertFile;

		NFThread	m_importThread;
		NFSyncEvent	m_importThreadEvent;

		NFThread	m_verifyThread;
		NFSyncEvent	m_verifyThreadEvent;
		
		typedef std::queue<VERIFY_INFO*> tVerifyQueue;
		tVerifyQueue m_verifyQueue;
		AutoCriticalSection m_csVerify;

		struct DELAYED_EXCEPTION
		{
			nf_time_t		ts;
			unsigned long	counter;
		};

		typedef std::map<std::string,DELAYED_EXCEPTION> tDelayedExceptionCounters;
		tDelayedExceptionCounters m_ecMap;

		typedef std::map<std::string,int> tExceptionCounters;
		tExceptionCounters m_ecTlsMap;

		unsigned long m_flags;
		CertStorage m_certStorage;
		ExceptionStorage m_xStorage;
		ExceptionStorage m_xtlsStorage;
		ExceptionStorage m_xvStorage;

		X509_STORE* m_rootCaStore;
		STACK_OF(X509)* m_rootCaList;

		tExceptionTimeout m_exceptionsTimeout[EXC_MAX];

		AutoCriticalSection m_cs;
	};
}