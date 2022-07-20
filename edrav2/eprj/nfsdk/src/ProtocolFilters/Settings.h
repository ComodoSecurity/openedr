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
#ifndef PF_NO_SSL_FILTER
#include "SSLDataProvider.h"
#endif

namespace ProtocolFilters
{
	class Settings
	{
	protected:
		Settings(void);

	public:
		~Settings(void);
	
		static Settings & get();
#ifdef WIN32
		bool init(const wchar_t * baseFolder);
#else
		bool init(const char * baseFolder);
#endif
		void free();
#ifndef PF_NO_SSL_FILTER
		SSLDataProvider * getSSLDataProvider();
#endif

#ifdef WIN32
		std::wstring getBaseFolder();
#endif

	protected:
#ifdef WIN32
		bool ensureFolderExists(const wchar_t * path);
#else
		bool ensureFolderExists(const char * path);
#endif

	private:
#ifdef WIN32
		wchar_t m_baseFolder[_MAX_PATH];
#else
		char m_baseFolder[_MAX_PATH];
#endif

#ifndef PF_NO_SSL_FILTER
		SSLDataProvider m_sslDataProvider;
#endif
	};
}