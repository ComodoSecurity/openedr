//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_PFOBJECT_C)
#define _PFOBJECT_C

#include "PFCDefs.h"
#include "PFStream_c.h"
#include "PFFilterDefs.h"

#ifdef __cplusplus
extern "C" 
{
#endif

typedef void PFObject_c, *PPFObject_c;

PF_API_C PPFObject_c PFObject_create(tPF_ObjectType type, int nStreams);

PF_API_C void PFObject_free(PPFObject_c pObject);

PF_API_C tPF_ObjectType PFObject_getType(PPFObject_c pObject);

PF_API_C void PFObject_setType(PPFObject_c pObject, tPF_ObjectType type);

PF_API_C BOOL PFObject_isReadOnly(PPFObject_c pObject);

PF_API_C void PFObject_setReadOnly(PPFObject_c pObject, int value);

PF_API_C int PFObject_getStreamCount(PPFObject_c pObject);

PF_API_C PFStream_c * PFObject_getStream(PPFObject_c pObject, int index);

PF_API_C void PFObject_clear(PPFObject_c pObject);

PF_API_C PFObject_c * PFObject_clone(PPFObject_c pObject);

PF_API_C PFObject_c * PFObject_detach(PPFObject_c pObject);

#ifdef __cplusplus
}
#endif

#endif 
