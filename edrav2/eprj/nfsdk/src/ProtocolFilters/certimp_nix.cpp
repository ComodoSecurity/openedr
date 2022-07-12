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

#ifndef WIN32
#include "certimp_nix.h"

#ifndef PF_NO_SSL_FILTER

using namespace ProtocolFilters;

static int do_exec(char *const args[])
{
	pid_t pid;
	int status;
	int eo;

	pid = fork();
	if (pid == -1) 
	{
		DbgPrint("fork failed: %s\n", strerror(errno));
		return -1;
	}
	if (!pid) 
	{
		int fd = open("/dev/null", O_WRONLY);
	        if (fd > 0)
		{
			if (dup2(fd, 1)  != 1 )
			{
		               //handle error 
			}
			if (dup2(fd, 2)  != 1 )
			{
		               //handle error 
			}
        	}

		/* In child process, execute external command. */
		(void)execvp(args[0], args);
		/* We only get here if the execv() failed. */
		eo = errno;
		DbgPrint("execvp %s failed: %s\n", args[0], strerror(eo));
		_exit(eo);
	}

	// In parent process, wait for exernal command to finish. 
	if (wait4(pid, (int*)&status, 0, NULL) != pid) 
	{
		DbgPrint("BUG executing %s command.\n", args[0]);
		return -1;
	}
	if (!WIFEXITED(status)) 
	{
		DbgPrint("%s command aborted by signal %d.\n",
				args[0], WTERMSIG(status));
		return -1;
	}
	eo = WEXITSTATUS(status);
	if (eo) 
	{
		DbgPrint("%s command failed: %s\n", 
				args[0], strerror(eo));
		return -1;
	}

	return 0;
}


bool ProtocolFilters::importCertificateToSystemStorage(const char * certName, const char * certPath, void * pCert, int certLen)
{
	DbgPrint("exec security %s", certPath);

	const char * const rmargs[] = { 
			"security",
			"delete-certificate",
			"-c",
			certName,
			NULL };

	do_exec((char*const*)rmargs);

	const char * const args[] = { 
			"security",
			"add-trusted-cert",
			"-d",
			"-r", "trustRoot",
			"-k", "/Library/Keychains/System.keychain",
			certPath,
			NULL };

	if (do_exec((char*const*)args) != 0)
		return false;

	return true;
}

static void removeQuotes(std::string & s)
{
	for (std::string::iterator it = s.begin();
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

static void importCertificate(const std::string & dir, const std::string & cert, int dbVersion)
{
	std::string certName = cert;
	size_t pos;
	pos = certName.rfind("/");
	if (pos != std::string::npos)
	{
		certName = certName.substr(pos+1);
	}
	pos = certName.rfind(".");
	if (pos != std::string::npos)
	{
		certName = certName.substr(0, pos);
	}
	removeQuotes(certName);

	DbgPrint("exec certutil in %s", dir.c_str());

	std::string fDir;

	if (dbVersion >=9 )
	{
		fDir = "sql:" + dir;
	} else
	{
		fDir = dir;
	}

#ifdef _MACOS
	const char * const args[] = { 
			"nss/certutil",
			"-A",
			"-t", "TC",
			"-i", cert.c_str(),
			"-n", certName.c_str(),
			"-d", fDir.c_str(),
			(dbVersion >= 9)? "-f pwfile" : NULL,
			NULL };
#else
	const char * const args[] = { 
			"certutil",
			"-A",
			"-t", "TC",
			"-i", cert.c_str(),
			"-n", certName.c_str(),
			"-d", fDir.c_str(),
			(dbVersion >= 9)? "-f pwfile" : NULL,
			NULL };

	DbgPrint("certutil cert=%s", cert.c_str());
	DbgPrint("certutil certName=%s", certName.c_str());
	DbgPrint("certutil dir=%s", fDir.c_str());
#endif

	do_exec((char*const*)args);
}

static const char pref[] = "user_pref(\"security.enterprise_roots.enabled\", true);";

void updateMozillaPrefs(const std::string & path)
{
	FILE * f;
	std::string lines;
	char tmpBuf[1000];
	size_t len;

	f = fopen(path.c_str(), "rb");
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

	f = fopen(path.c_str(), "ab");
	if (!f)
		return;

	fwrite(pref, 1, sizeof(pref), f);

	fclose(f);
}

void ProtocolFilters::importCertificateToMozilla(const std::string & cert, NFSyncEvent & hEvent)
{
	typedef std::queue<std::string> tPathsQueue;
	tPathsQueue pathsQueue;
	std::string curFolder, fileMask;
	DIR *dp;
	struct dirent *ep;

	DbgPrint("importCertificateToMozilla()");

#ifdef _MACOS
	dp = opendir("/Users");
	if (dp != NULL)
	{
		while (ep = readdir(dp))
		{
			if (strcmp(ep->d_name, ".") == 0 ||
				strcmp(ep->d_name, "..") == 0)
			{
				continue;
			}
			
			curFolder = "/Users/";
			curFolder += ep->d_name;
			curFolder += "/Library/Application Support";

			DbgPrint("importCertificateToMozilla() %s", curFolder.c_str());

			pathsQueue.push(curFolder);
		}
		
		closedir (dp);
	}
#else
	dp = opendir("/home");
	if (dp != NULL)
	{
		while ((ep = readdir(dp)))
		{
			if (strcmp(ep->d_name, ".") == 0 ||
				strcmp(ep->d_name, "..") == 0)
			{
				continue;
			}
			
			curFolder = "/home/";
			curFolder += ep->d_name;
//			curFolder += "/.mozilla";

			DbgPrint("importCertificateToMozilla() %s", curFolder.c_str());

			pathsQueue.push(curFolder);
		}
		
		closedir (dp);
	}

#endif

	while (!pathsQueue.empty())
	{
		curFolder = pathsQueue.front();
		pathsQueue.pop();

		dp = opendir(curFolder.c_str());
		if (dp != NULL)
		{
//				DbgPrint("importCertificateToMozilla() enumerate %s", curFolder.c_str());

			while ((ep = readdir(dp)))
			{
//					DbgPrint("importCertificateToMozilla() enumerate %s : %s (%d)", curFolder.c_str(), ep->d_name, ep->d_type);

				if (strcmp(ep->d_name, ".") == 0 ||
					strcmp(ep->d_name, "..") == 0)
				{
					continue;
				}

#ifdef _MACOS
				if (ep->d_type == DT_DIR)
#else
				struct stat st;
				if (lstat((curFolder + "/" + ep->d_name).c_str(), &st) == 0 && S_ISDIR(st.st_mode))
#endif
				{
					pathsQueue.push(curFolder + "/" + ep->d_name);
//						DbgPrint("importCertificateToMozilla() folder %s", (curFolder + "/" + ep->d_name).c_str());
					continue;
				}

				if (stricmp(ep->d_name, "cert8.db") == 0)
				{
					importCertificate(curFolder, cert, 8);
#ifdef _MACOS
					updateMozillaPrefs(curFolder + "/prefs.js");
#endif
					continue;
				}

				if (stricmp(ep->d_name, "cert9.db") == 0)
				{
					importCertificate(curFolder, cert, 9);
#ifdef _MACOS
					updateMozillaPrefs(curFolder + "/prefs.js");
#endif
					continue;
				}
			}
			
			closedir (dp);
		} else
		{
			DbgPrint("importCertificateToMozilla() unable to open %s", curFolder.c_str());
		}

		if (hEvent.isFlagFired(1))
			break;
	}
}

#endif

#endif
