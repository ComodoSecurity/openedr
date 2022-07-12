//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _HEADER_C
#define _HEADER_C

#include "PFCDefs.h"

#ifdef __cplusplus
extern "C" 
{
#endif

typedef void PFHeaderField_c, *PPFHeaderField_c;
typedef void PFHeader_c, *PPFHeader_c;

PF_API_C const char * PFHeaderField_getName(PPFHeaderField_c hf);
PF_API_C const char * PFHeaderField_getValue(PPFHeaderField_c hf);
PF_API_C const char * PFHeaderField_getUnfoldedValue(PPFHeaderField_c hf);

PF_API_C PPFHeader_c PFHeader_create();
PF_API_C PPFHeader_c PFHeader_parse(const char * buf, int len);
PF_API_C void PFHeader_free(PPFHeader_c ph);
PF_API_C void PFHeader_addField(PPFHeader_c ph, const char * name, const char * value, int toStart);
PF_API_C void PFHeader_removeFields(PPFHeader_c ph, const char * name, int exact);
PF_API_C void PFHeader_clear(PPFHeader_c ph);
PF_API_C PPFHeaderField_c PFHeader_findFirstField(PPFHeader_c ph, const char * name);
PF_API_C int PFHeader_size(PPFHeader_c ph);
PF_API_C PPFHeaderField_c PFHeader_getField(PPFHeader_c ph, int index);

#ifdef __cplusplus 
}
#endif

#endif 

