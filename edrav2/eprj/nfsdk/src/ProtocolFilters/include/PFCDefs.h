// 
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _PFCDEFS
#define _PFCDEFS

#ifdef WIN32

#ifndef _C_API

#define PF_API_C

#ifdef _PF_STATIC_LIB
	#define PF_API
#else
	#ifdef PROTOCOLFILTERS_EXPORTS
	#define PF_API __declspec(dllexport) 
	#else
	#define PF_API __declspec(dllimport) 
	#endif
#endif

#else

#define PF_API

#ifdef _PF_STATIC_LIB
	#define PF_API_C
#else
	#ifdef PROTOCOLFILTERS_EXPORTS
	#define PF_API_C __declspec(dllexport) 
	#else
	#define PF_API_C __declspec(dllimport) 
	#endif
#endif

#endif

#else
	#define PF_API
	#define PF_API_C
#endif


#ifdef WIN32
#define PFAPI_CC __cdecl
#else
#define PFAPI_CC 
#endif

#ifndef BOOL
typedef int BOOL;
#endif

#endif