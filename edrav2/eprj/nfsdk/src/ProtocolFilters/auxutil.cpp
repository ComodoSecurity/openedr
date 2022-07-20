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
#include "auxutil.h"

using namespace ProtocolFilters;

int ProtocolFilters::aux_find(char c, const char * buf, int len)
{
	for (int i=0; i<len; i++)
	{
		if (buf[i] == c)
			return i;
	}
	return -1;
}

int ProtocolFilters::aux_rfind(char c, const char * buf, int len)
{
	for (int i=len-1; i>=0; i--)
	{
		if (buf[i] == c)
			return i;
	}
	return -1;
}

#define BUFFER_SIZE 1024

std::string ProtocolFilters::aux_stringprintf(const char *fmt, ...)
{
	char buffer[BUFFER_SIZE];
	va_list argp;

	memset(buffer, 0, BUFFER_SIZE);
	
	va_start(argp, fmt);
	vsnprintf(buffer, BUFFER_SIZE - 1, fmt, argp);
	va_end(argp);

	return (std::string)(buffer);
}


std::string ProtocolFilters::aux_getAddressString(const sockaddr * pAddr)
{
#ifdef WIN32

	char addrStr[MAX_PATH] = "";
	DWORD dwLen;
	
	dwLen = sizeof(addrStr);

	if (WSAAddressToStringA((LPSOCKADDR)pAddr, 
				(pAddr->sa_family == AF_INET6)? sizeof(sockaddr_in6) : sizeof(sockaddr_in), 
				NULL, 
				addrStr, 
				&dwLen) == 0)
	{
		return addrStr;
	}

	return "";

#else

	struct sockaddr_in myname_in;
	struct sockaddr_un myname_un;
	std::string result;
	
	memset(&myname_in, 0, sizeof(struct sockaddr_in));
	memset(&myname_un, 0, sizeof(struct sockaddr_un));
	
	if (pAddr->sa_family == AF_INET)
	{
		memcpy(&myname_in, pAddr, sizeof(struct sockaddr_in));
		
		result = aux_stringprintf("%s:%d", inet_ntoa(myname_in.sin_addr), ntohs(myname_in.sin_port));
	}
	else
	{
		memcpy(&myname_un, pAddr, sizeof(struct sockaddr_un));
		
		result = myname_un.sun_path;
	}
	
	return result;

#endif

}

