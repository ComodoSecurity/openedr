//
// 	NetFilterSDK 
// 	Copyright (C) 2013 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _CALLOUTS
#define _CALLOUTS

NTSTATUS callouts_init(PDEVICE_OBJECT deviceObject);
void callouts_free();
UINT16 callouts_getSublayerWeight();


#endif
