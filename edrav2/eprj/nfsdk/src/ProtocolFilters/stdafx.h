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

typedef unsigned long long nf_time_t;

#if defined (WIN32)

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_SECURE_NO_WARNINGS 1
//#define _SECURE_SCL 0

#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <stdio.h>
#include <process.h>
#include <io.h>

inline nf_time_t nf_time()
{
  long long freq = 0;
  long long counter = 0;
  QueryPerformanceFrequency((LARGE_INTEGER*)&freq); // always succeeded
  QueryPerformanceCounter((LARGE_INTEGER*)&counter);  // always succeeded
  return counter / freq;
}

#define CERT_CHAIN_PARA_HAS_EXTRA_FIELDS 1

typedef unsigned __int64 uint64_t;

#else

#if defined(__APPLE__) && defined(__MACH__)
#define _MACOS 1
#endif

#ifdef DEBUG 
#define _DEBUG
#endif

//#define __LARGE64_FILES

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <glob.h>
#include <syslog.h>
#include <pwd.h> 
#include <grp.h> 
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <netdb.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>

#define _MAX_PATH 260
#define MAX_PATH 260

#define _snprintf snprintf
#define _vsnprintf vsnprintf

#define BOOL int
#define TRUE 1
#define FALSE 0

#define strnicmp strncasecmp
#define stricmp strcasecmp
#define _time64 time
#define _atoi64 atoll
#define _chsize ftruncate
#define _fileno fileno
#define _fseeki64 fseek
#define _ftelli64 ftell

#define __int64 long long

#define UNALIGNED

#define DWORD unsigned long

#define InterlockedIncrement(x) __sync_fetch_and_add(x, 1)
#define InterlockedDecrement(x) __sync_fetch_and_sub(x, 1)

inline char *strlwr(char *str)
{
  unsigned char *p = (unsigned char *)str;

  while (*p) 
  {
     *p = tolower((unsigned char)*p);
      p++;
  }

  return str;
}

#ifdef _MACOS

#include <mach/mach_time.h>

inline nf_time_t nf_time()
{
	/* Get the timebase info */
	mach_timebase_info_data_t info;
	mach_timebase_info(&info);

	uint64_t t = mach_absolute_time();

	/* Convert to seconds */
	t *= info.numer;
	t /= info.denom;
	t /= 1000 * 1000 * 1000;

	return t;
}

#else
inline nf_time_t nf_time()
{
	struct timespec ts;
	if (clock_gettime(CLOCK_MONOTONIC,&ts) != 0) 
	{
	 	//error
		return time(NULL);
	}
	return ts.tv_sec;
}

#endif

#endif

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <queue>

// EDR_FIX: Replace usage auto_ptr with unique_ptr
// auto_ptr is removed in VS2019 by default
namespace std {
template <typename T>
using auto_ptr = unique_ptr<T>;
}

#include "dbglogger.h"

#define DEFAULT_BUFFER_SIZE 8192

