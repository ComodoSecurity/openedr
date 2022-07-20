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

#include <ntifs.h>
#include <ntstrsafe.h>
#include <tdikrnl.h>
#include <stdlib.h>

#include "gdefs.h"

#define MEM_TAG		'2TLF'

#define malloc_np(size)	ExAllocatePoolWithTag(NonPagedPool, (size), MEM_TAG)
#define free_np(p) ExFreePool(p);

typedef KSPIN_LOCK __SPIN_LOCK;

#define sl_init(x) KeInitializeSpinLock(x)
#define sl_lock(x, irql) KeAcquireSpinLock(x, irql)
#define sl_unlock(x, irql) KeReleaseSpinLock(x, irql)
#define sl_free(x) 

#define DPREFIX "[NF] "

//
// Software Tracing Definitions 
//

#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(CtlGuid,(a7f09d73, 5ac6, 4b8b, 8a33, e7b8c87e4606),  \
        WPP_DEFINE_BIT(FLAG_INFO))

#define _NF_INTERNALS

#endif