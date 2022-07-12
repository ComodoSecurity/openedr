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
#include "Settings.h"

using namespace ProtocolFilters;

Settings & Settings::get()
{
	static Settings settings;
	return settings;
}


Settings::Settings(void)
{
}

Settings::~Settings(void)
{
}

#ifdef WIN32
bool Settings::init(const wchar_t * baseFolder)
{
	wchar_t path[_MAX_PATH];

	wcscpy(m_baseFolder, baseFolder);

	if (!ensureFolderExists(baseFolder))
		return false;

	wcscpy(path, baseFolder);
	wcscat(path, L"/SSL");

	if (!ensureFolderExists(path))
		return false;

#ifndef PF_NO_SSL_FILTER
	if (!m_sslDataProvider.init(path))
		return false;
#endif

	return true;
}

#else

bool Settings::init(const char * baseFolder)
{
	char path[_MAX_PATH];

	strcpy(m_baseFolder, baseFolder);

	if (!ensureFolderExists(baseFolder))
		return false;

	strcpy(path, baseFolder);
	strcat(path, "/SSL");

	if (!ensureFolderExists(path))
		return false;

#ifndef PF_NO_SSL_FILTER
	if (!m_sslDataProvider.init(path))
		return false;
#endif

	return true;
}
#endif

void Settings::free()
{
#ifndef PF_NO_SSL_FILTER
	m_sslDataProvider.free();
#endif
}

#ifndef PF_NO_SSL_FILTER
SSLDataProvider * Settings::getSSLDataProvider()
{
	return &m_sslDataProvider;
}
#endif

#ifdef WIN32

bool Settings::ensureFolderExists(const wchar_t * path)
{
	DWORD attrs = GetFileAttributesW(path);
	
	if ((attrs != INVALID_FILE_ATTRIBUTES) &&
		(attrs & FILE_ATTRIBUTE_DIRECTORY))
		return true;

	if (CreateDirectoryW(path, NULL))
		return true;

	return false;
}

#else

bool Settings::ensureFolderExists(const char * path)
{
	if (mkdir(path, 755) == -1)
	{
		if (errno != EEXIST)
			return false;
	}
	return true;
}

#endif

#ifdef WIN32

std::wstring Settings::getBaseFolder()
{
	return m_baseFolder;
}

#endif