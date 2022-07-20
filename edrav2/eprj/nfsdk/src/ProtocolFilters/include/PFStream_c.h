//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _PFSTREAM_C
#define _PFSTREAM_C

#include "PFCDefs.h"

#ifdef __cplusplus
extern "C" 
{
#endif

typedef unsigned __int64 tStreamPos;
typedef unsigned long tStreamSize;

typedef void PFStream_c, *PPFStream_c;

PF_API_C tStreamPos PFStream_seek(PPFStream_c s, tStreamPos offset, int origin);

PF_API_C tStreamSize PFStream_read(PPFStream_c s, void * buffer, tStreamSize size);

PF_API_C tStreamSize PFStream_write(PPFStream_c s, const void * buffer, tStreamSize size);

PF_API_C tStreamPos PFStream_size(PPFStream_c s);

PF_API_C tStreamPos PFStream_setEndOfStream(PPFStream_c s);

PF_API_C void PFStream_reset(PPFStream_c s);

PF_API_C tStreamPos PFStream_copyTo(PPFStream_c s, PPFStream_c d);

#ifdef __cplusplus
}
#endif


#endif 