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

#ifdef WIN32
#include <windows.h>
#include <wincrypt.h>
#include <rpc.h>
#include <stdio.h>
#include "certimp.h"
#include <vector>
#include <queue>
#include "IOB.h"

#ifndef PF_NO_SSL_FILTER

#pragma comment(lib, "Crypt32.lib")

#include <openssl/rsa.h>
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>

using namespace ProtocolFilters;

#define ENCODING (X509_ASN_ENCODING | PKCS_7_ASN_ENCODING)

typedef std::queue<std::wstring> tPathsQueue;
#define	MAX_KEY_LEN	256


//
// ImportCertificate installs the crypto provider and certificate
// on the machine.
//
BOOL ProtocolFilters::importCertificateToWindowsStorage(LPCSTR szCert, DWORD certLen, 
                  LPSTR szStore, BOOL bUser)
{
   HCERTSTORE hStore = NULL;
   PCCERT_CONTEXT pCertContext = NULL;
   BOOL fReturn = FALSE;
   BOOL bResult;
   WCHAR szwStore[20];
   DWORD dwAcquireFlags = 0;
   DWORD dwCertOpenFlags = CERT_SYSTEM_STORE_CURRENT_USER;
   
   __try
   {
     if (!bUser)
     {
        dwAcquireFlags = CRYPT_MACHINE_KEYSET;
        dwCertOpenFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;
     }

     // Convert multibyte store name to wide char string
     if (mbstowcs(szwStore, szStore, strlen(szStore)+1) == (size_t)-1)
     {
       __leave;
     }

     // Open Certificate Store
     hStore = CertOpenStore(CERT_STORE_PROV_SYSTEM,
                      ENCODING,
                      0,
                      dwCertOpenFlags,
                      (LPVOID)szwStore);
     if (!hStore)
     {
       __leave;
     }

     // Add Certificate to store
     bResult = CertAddEncodedCertificateToStore(hStore,
                                     X509_ASN_ENCODING,
                                     (LPBYTE)szCert,
                                     certLen,
                                     CERT_STORE_ADD_REPLACE_EXISTING,
                                     &pCertContext);
     if (!bResult)
     {
       __leave;
     }
     fReturn = TRUE;
   }
   _finally
   {
     // Clean up
     if (pCertContext) CertFreeCertificateContext(pCertContext);
     if (hStore) CertCloseStore(hStore, 0);
   }

   return fReturn;
}

static void removeQuotes(std::wstring & s)
{
	for (std::wstring::iterator it = s.begin();
		it != s.end(); it++)
	{
		if (*it == '"')
		{
			it = s.erase(it);
			if (it == s.end()) 
				break;
		}
	}
}

static BOOL runProcess(wchar_t *szCmd, wchar_t * szDir)
{
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    
	ZeroMemory(&si,sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = FALSE;

	ZeroMemory(&pi, sizeof(pi));

    BOOL result = CreateProcessW(NULL, 
					szCmd, 
					NULL, 
					NULL, 
					FALSE, 
					DETACHED_PROCESS, 
					NULL, 
					szDir, 
                    &si, 
					&pi);

	if (result)
	{
		WaitForSingleObject( pi.hProcess, INFINITE );
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}

    return result;
}

static void importCertificate(const std::wstring & dir, const std::wstring & cert, int dbVersion)
{
	std::wstring certName = cert;
	size_t pos;
	pos = certName.rfind(L"\\");
	if (pos != std::string::npos)
	{
		certName = certName.substr(pos+1);
	}
	pos = certName.rfind(L".");
	if (pos != std::string::npos)
	{
		certName = certName.substr(0, pos);
	}
	removeQuotes(certName);

	std::wstring cmd, sShortDir, scShortDir;
	wchar_t shortDir[MAX_PATH];
	
	if (GetShortPathNameW(dir.c_str(), shortDir, sizeof(shortDir)/2))
	{
		sShortDir = shortDir;
	} else
	{
		sShortDir = dir;
	}

	if (GetShortPathNameW(cert.c_str(), shortDir, sizeof(shortDir)/2))
	{
		scShortDir = shortDir;
	} else
	{
		scShortDir = cert;
	}

	if (dbVersion >= 9)
	{
		cmd = L"nss\\certutil -A -t \"TCu\" -i \"" + scShortDir + L"\" -n \"" + certName + L"\" -d sql:\"" + sShortDir + L"\"" + L" -f pwfile";
	} else
	{
		cmd = L"nss\\certutil -A -t \"TCu\" -i \"" + scShortDir + L"\" -n \"" + certName + L"\" -d \"" + sShortDir + L"\"";
	}

	DbgPrint("importCertificate() dbVersion=%d, dbPath=%s, certPath=%s", 
		dbVersion, encodeUTF8(dir).c_str(), encodeUTF8(cert).c_str());

	if (!runProcess((wchar_t*)cmd.c_str(), NULL))
	{
		DbgPrint("importCertificate() runProcess failed");
	}
}

static const char pref[] = "user_pref(\"security.enterprise_roots.enabled\", true);";

void updateMozillaPrefs(const std::wstring & path)
{
	FILE * f;
	std::string lines;
	char tmpBuf[1000];
	size_t len;

	f = _wfopen(path.c_str(), L"rb");
	if (!f)
		return;

	while (len = fread(tmpBuf, 1, sizeof(tmpBuf)-1, f))
	{
		tmpBuf[len] = '\0';
		lines += tmpBuf;
	}

	fclose(f);

	size_t pos = lines.find(pref);
	if (pos != std::string::npos)
		return;

	f = _wfopen(path.c_str(), L"ab");
	if (!f)
		return;

	fwrite(pref, 1, sizeof(pref), f);

	fclose(f);
}

template <class _T>
static void push_v(std::vector<char> & data, _T v)
{
	for (int i=sizeof(_T)-1; i>=0; i--)
	{
		data.push_back(((char*)&v)[i]);
	}
}

static bool isOperaDbUpdated(const std::wstring & dbFile, const std::wstring & certName)
{
	FILE * fdb = _wfopen(dbFile.c_str(), L"rb");
	if (fdb)
	{
		std::vector<char> data;
		char c;
		size_t i, j;

		while (fread(&c, 1, 1, fdb))
		{
			data.push_back(c);
		}
		fclose(fdb);

		if (data.size() <= certName.length())
			return false;

		for (i=0; i <= (data.size() - certName.length()); i++)
		{
			for (j=0; j < certName.length(); j++)
			{
				if (data[i + j] != certName[j])
					break;
			}
			if (j == certName.length())
				return true;
		}
	}
	return false;
}

static void updateOperaDb(const std::wstring & dbFile, const std::wstring & cert)
{
	std::wstring certName = cert;
	size_t pos;
	pos = certName.rfind(L"\\");
	if (pos != std::string::npos)
	{
		certName = certName.substr(pos+1);
	}
	pos = certName.rfind(L".");
	if (pos != std::string::npos)
	{
		certName = certName.substr(0, pos);
	}
	removeQuotes(certName);

	if (certName.length() == 0)
		return;

	if (isOperaDbUpdated(dbFile, certName))
		return;

	std::vector<char> data;
	size_t i;
	char c;

	data.push_back(0x2);
	push_v(data, (int)(0x344 + (certName.length() - 12) * 4));
	data.push_back(0x20);
	push_v(data, (int)0x4);
	push_v(data, (int)0x1);
	push_v(data, (int)0x21000000);
	push_v(data, (char)certName.length());
	for (i=0; i<certName.length(); i++)
	{
		data.push_back(((char*)&certName[i])[0]);
	}
	data.push_back(0x22);
	push_v(data, (int)(0x26 + (certName.length() - 12)));
	push_v(data, (short)(0x3024 + (certName.length() - 12)));
	push_v(data, (int)0x310b3009);
	push_v(data, (int)0x06035504);
	push_v(data, (int)0x06130245);
	data.push_back(0x4e);
	push_v(data, (short)(0x3115 + (certName.length() - 12)));
	push_v(data, (short)(0x3013 + (certName.length() - 12)));
	push_v(data, (int)0x06035504);
	push_v(data, (short)0x0313);
	push_v(data, (char)certName.length());
	for (i=0; i<certName.length(); i++)
	{
		data.push_back(((char*)&certName[i])[0]);
	}
	data.push_back(0x23);
	
	unsigned int offset = (unsigned int)data.size();

	FILE * f = _wfopen(cert.c_str(), L"rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		int len = ftell(f);
		fseek(f, 0, SEEK_SET);

		push_v(data, len);

		while (fread(&c, 1, 1, f))
		{
			data.push_back(c);
		}

		fclose(f);

		const char cn[] = { 0x06, 0x03, 0x55, 0x04, 0x03 };
		for (unsigned int i = offset; i < (data.size()-sizeof(cn)); i++)
		{
			if (memcmp(&data[i], cn, sizeof(cn)) == 0)
			{
				char b = data[i+sizeof(cn)];
				data[59 + (certName.length() - 11)] = b;
				break;
			}
		}
	}

	unsigned int v = (unsigned int)data.size() - 5;

    v = ((v << 8) & 0x00ff0000) | 
         (v << 24)               | 
         ((v >> 8) & 0x0000ff00) | 
         (v >> 24);                

	*(DWORD*)&data[1] = v;

	FILE * fdb = _wfopen(dbFile.c_str(), L"ab");
	if (fdb)
	{
		for (i=0; i<data.size(); i++)
		{
			fwrite(&data[i], 1, 1, fdb);
		}
		fclose(fdb);
	}
}

void enumCommonFolders(const wchar_t * usersDir, tPathsQueue & pathsQueue)
{
	DWORD dwIndex = 0;
	LONG lRet;
	wchar_t wszSubKeyName[MAX_KEY_LEN];
	DWORD cbName;
	size_t usersDirLen;

	usersDirLen = wcslen(usersDir);

	cbName = (DWORD)ARRAYSIZE(wszSubKeyName);
	
	DbgPrint("enumCommonFolders");

	while ( (lRet = ::RegEnumKeyExW(
		HKEY_USERS, dwIndex, wszSubKeyName, &cbName, NULL,
		NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS )
	{
		if ( lRet == ERROR_SUCCESS )
		{
			std::wstring keyName;
			HKEY hItem = NULL;

			keyName = wszSubKeyName;
			keyName += L"\\Volatile Environment";

			if ( ::RegOpenKeyExW(HKEY_USERS, keyName.c_str(), 0, KEY_READ, &hItem) == ERROR_SUCCESS )
			{
				wchar_t wszPath[MAX_KEY_LEN] = L"";
				DWORD dwSize;
				DWORD dwType;

				dwSize = (DWORD)ARRAYSIZE(wszPath);

				if ( ERROR_SUCCESS == ::RegQueryValueExW(hItem, L"APPDATA", NULL, &dwType, (LPBYTE)&wszPath, &dwSize) )
				{
//					DbgPrint("enumCommonFolders APPDATA=%S", wszPath);

					if (wcsnicmp(usersDir, wszPath, usersDirLen) != 0)
					{
						pathsQueue.push(wszPath);
					}
				} else
				{
					DbgPrint("enumCommonFolders no APPDATA value");
				}

				dwSize = (DWORD)ARRAYSIZE(wszPath);

				if ( ERROR_SUCCESS == ::RegQueryValueExW(hItem, L"LOCALAPPDATA", NULL, &dwType, (LPBYTE)&wszPath, &dwSize) )
				{
//					DbgPrint("enumCommonFolders LOCALAPPDATA=%S", wszPath);

					if (wcsnicmp(usersDir, wszPath, usersDirLen) != 0)
					{
						pathsQueue.push(wszPath);
					}
				} else
				{
					DbgPrint("enumCommonFolders no LOCALAPPDATA value");
				}
				
				RegCloseKey(hItem);
			}
		}

		dwIndex++;
		cbName = (DWORD)ARRAYSIZE(wszSubKeyName);
	}
}


void ProtocolFilters::importCertificateToMozillaAndOpera(const std::wstring & cert, NFSyncEvent & hEvent)
{
	wchar_t usersDir[_MAX_PATH] = L"";
	wchar_t * p;
	WIN32_FIND_DATAW wfd;
	tPathsQueue pathsQueue;
	std::wstring curFolder, fileMask;
	
	if (!ExpandEnvironmentStringsW(L"%PUBLIC%", usersDir, _MAX_PATH) || (*usersDir == L'%'))
	{
		if (!ExpandEnvironmentStringsW(L"%ALLUSERSPROFILE%", usersDir, _MAX_PATH) || (*usersDir == L'%'))
			return;
	}

	if (wcslen(usersDir) < 4)
		return;

	for (p = usersDir + 3; *p; p++)
	{
		if (*p == L'\\' || *p == L'/')
		{
			*p = 0;
			break;
		}
	}

	pathsQueue.push(usersDir);

	enumCommonFolders(usersDir, pathsQueue);

	while (!pathsQueue.empty())
	{
		curFolder = pathsQueue.front();
		pathsQueue.pop();

		HANDLE hF = FindFirstFileW((curFolder + L"\\*").c_str(), &wfd);
		if (hF == INVALID_HANDLE_VALUE)
		{
			continue;
		} else
		{
			do {
				if (_wcsicmp(wfd.cFileName, L"cert8.db") == 0)
				{
					importCertificate(curFolder, cert, 8);
					updateMozillaPrefs(curFolder + L"\\prefs.js");
				} else
				if (_wcsicmp(wfd.cFileName, L"cert9.db") == 0)
				{
					importCertificate(curFolder, cert, 9);
					updateMozillaPrefs(curFolder + L"\\prefs.js");
				} else
				if (_wcsicmp(wfd.cFileName, L"opcacrt6.dat") == 0)
				{
					updateOperaDb(curFolder + L"\\" + wfd.cFileName, cert);
					break;
				}

				if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
					(wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
					continue;
				
				if (wcscmp(wfd.cFileName, L".") == 0 ||
					wcscmp(wfd.cFileName, L"..") == 0)
					continue;

				if (wcsicmp(wfd.cFileName, L"temp") == 0 ||
					wcsicmp(wfd.cFileName, L"microsoft") == 0 ||
					wcsicmp(wfd.cFileName, L"packages") == 0)
					continue;

				pathsQueue.push(curFolder + L"\\" + wfd.cFileName);

				if (hEvent.isFlagFired(1))
					break;

			} while( FindNextFileW(hF, &wfd) );

			FindClose(hF);
		}

		if (hEvent.isFlagFired(1))
			break;
	}
}


static bool loadFile(const wchar_t * fileName, IOB & iob)
{
	FILE * f;
	int pos;

	iob.reset();

	f = _wfopen(fileName, L"rb");
	if (!f)
		return false;

	fseek(f, 0, SEEK_END);
	pos = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (pos == 0)
	{
		fclose(f);
		return true;
	}

	if (!iob.resize((unsigned long)pos))
	{
		fclose(f);
		return false;
	}

	if (fread(iob.buffer(), 1, pos, f) != pos)
	{
		fclose(f);
		return false;
	}

	fclose(f);

	return true;
}

static bool convertDER2PEM(IOB & derCert, IOB & pemCert)
{
	X509 *	x = NULL;
	bool res = false;
	char * buf = derCert.buffer();
	int len = (int)derCert.size();

	pemCert.reset();

	x = d2i_X509(&x, (const unsigned char**)&buf, len);

	if (x)
	{
		BIO * pbio = BIO_new(BIO_s_mem());
		if (pbio)
		{
			if (PEM_write_bio_X509(pbio, x))
			{
				char c;
				int n;

				while (BIO_pending(pbio))
				{
					n = BIO_read(pbio, &c, 1);
					if (n > 0)
					{
						pemCert.append(c);
					}
				}
				
				res = true;
			}
			BIO_free(pbio);
		}

		X509_free(x);
	}

	return res;
}

void ProtocolFilters::importCertificateToPidgin(const std::wstring & cert, NFSyncEvent & hEvent)
{
	wchar_t usersDir[_MAX_PATH] = L"";
	wchar_t * p;
	WIN32_FIND_DATAW wfd;
	typedef std::queue<std::wstring> tPathsQueue;
	tPathsQueue pathsQueue;
	std::wstring curFolder, fileMask;
	
	if (ExpandEnvironmentStringsW(L"%PROGRAMFILES%", usersDir, _MAX_PATH) && (*usersDir != L'%'))
	{
		for (p = usersDir + 3; *p; p++)
		{
			if (*p == L'\\' || *p == L'/')
			{
				*p = 0;
				break;
			}
		}

		pathsQueue.push(usersDir);
	}

	if (ExpandEnvironmentStringsW(L"%PROGRAMFILES(x86)%", usersDir, _MAX_PATH) && (*usersDir != L'%'))
	{
		for (p = usersDir + 3; *p; p++)
		{
			if (*p == L'\\' || *p == L'/')
			{
				*p = 0;
				break;
			}
		}

		pathsQueue.push(usersDir);
	}

	IOB x509Cert;
	IOB x509PemCert;

	if (!loadFile(cert.c_str(), x509Cert))
	{
		return;
	}

	if (!convertDER2PEM(x509Cert, x509PemCert))
	{
		return;
	}

	std::wstring certName = cert;
	size_t pos;
	pos = certName.rfind(L"\\");
	if (pos != std::string::npos)
	{
		certName = certName.substr(pos+1);
	}
	pos = certName.rfind(L".");
	if (pos != std::string::npos)
	{
		certName = certName.substr(0, pos);
	}
	removeQuotes(certName);


	while (!pathsQueue.empty())
	{
		curFolder = pathsQueue.front();
		pathsQueue.pop();

		HANDLE hF = FindFirstFileW((curFolder + L"\\*").c_str(), &wfd);
		if (hF == INVALID_HANDLE_VALUE)
		{
			continue;
		} else
		{
			do {
				if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
					(wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
					continue;
				
				if (wcscmp(wfd.cFileName, L".") == 0 ||
					wcscmp(wfd.cFileName, L"..") == 0)
					continue;

				if (wcsicmp(wfd.cFileName, L"temp") == 0 ||
					wcsicmp(wfd.cFileName, L"microsoft") == 0 ||
					wcsicmp(wfd.cFileName, L"packages") == 0)
					continue;

				if (wcscmp(wfd.cFileName, L"ca-certs") == 0)
				{
					std::wstring destCertName = curFolder + L"\\" + wfd.cFileName + L"\\" + certName + L".pem";
					FILE * fout = _wfopen(destCertName.c_str(), L"wb");
					if (fout)
					{
						fwrite(x509PemCert.buffer(), 1, x509PemCert.size(), fout);
						fclose(fout);
					}
				} else
				{
					pathsQueue.push(curFolder + L"\\" + wfd.cFileName);
				}

				if (hEvent.isFlagFired(1))
					break;

			} while( FindNextFileW(hF, &wfd) );

			FindClose(hF);
		}

		if (hEvent.isFlagFired(1))
			break;
	}
}


#endif

#endif