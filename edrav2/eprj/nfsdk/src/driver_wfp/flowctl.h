//
// 	NetFilterSDK 
// 	Copyright (C) 2017 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _FLOWCTL_H
#define _FLOWCTL_H

typedef int tFlowControlHandle;
typedef unsigned __int64 uint64_t;

BOOLEAN flowctl_init();
void flowctl_free();
tFlowControlHandle flowctl_add(uint64_t inLimit, uint64_t outLimit);
BOOLEAN flowctl_modifyLimits(tFlowControlHandle fcHandle, uint64_t inLimit, uint64_t outLimit);
BOOLEAN flowctl_delete(tFlowControlHandle fcHandle);
BOOLEAN flowctl_getStat(tFlowControlHandle fcHandle, PNF_FLOWCTL_STAT pStat);
BOOLEAN flowctl_updateStat(tFlowControlHandle fcHandle, PNF_FLOWCTL_STAT pStat);
BOOLEAN flowctl_mustSuspend(tFlowControlHandle fcHandle, BOOLEAN isOut, uint64_t lastIoTime);
void flowctl_update(tFlowControlHandle fcHandle, BOOLEAN isOut, uint64_t nBytes);
uint64_t flowctl_getTickCount();
BOOLEAN flowctl_exists(tFlowControlHandle fcHandle);


#endif