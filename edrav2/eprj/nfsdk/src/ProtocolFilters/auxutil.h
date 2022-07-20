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

#include <string>

namespace ProtocolFilters
{
	int aux_find(char c, const char * buf, int len);
	int aux_rfind(char c, const char * buf, int len);
	std::string aux_stringprintf(const char *fmt, ...);
	std::string aux_getAddressString(const sockaddr * pAddr);
}