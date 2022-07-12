//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _STDINC_H
#define _STDINC_H

#include <windows.h>
#include <stdlib.h>

#define malloc_np(size) malloc(size)
#define free_np(p) free(p)

typedef CRITICAL_SECTION __SPIN_LOCK;

#define sl_init(x) InitializeCriticalSection(x)
#define sl_lock(x) EnterCriticalSection(x)
#define sl_unlock(x) LeaveCriticalSection(x)
#define sl_free(x) DeleteCriticalSection(x)

#endif