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

#ifdef _C_API

#include "PFObject_c.h"
#include "PFObject.h"

using namespace ProtocolFilters;

tPF_ObjectType PFObject_getType(PPFObject_c pObject)
{
	if (!pObject)
		return OT_NULL;
	PFObject * p = (PFObject*)pObject;
	return p->getType();
}

void PFObject_setType(PPFObject_c pObject, tPF_ObjectType type)
{
	if (!pObject)
		return;
	PFObject * p = (PFObject*)pObject;
	p->setType(type);
}

BOOL PFObject_isReadOnly(PPFObject_c pObject)
{
	if (!pObject)
		return FALSE;
	PFObject * p = (PFObject*)pObject;
	return p->isReadOnly();
}

void PFObject_setReadOnly(PPFObject_c pObject, int value)
{
	if (!pObject)
		return;
	PFObject * p = (PFObject*)pObject;
	p->setReadOnly(value != 0);
}

int PFObject_getStreamCount(PPFObject_c pObject)
{
	if (!pObject)
		return 0;
	PFObject * p = (PFObject*)pObject;
	return p->getStreamCount();
}

PFStream_c * PFObject_getStream(PPFObject_c pObject, int index)
{
	if (!pObject)
		return NULL;
	PFObject * p = (PFObject*)pObject;
	return p->getStream(index);
}

void PFObject_clear(PPFObject_c pObject)
{
	if (!pObject)
		return;
	PFObject * p = (PFObject*)pObject;
	p->clear();
}

PFObject_c * PFObject_clone(PPFObject_c pObject)
{
	if (!pObject)
		return NULL;
	PFObject * p = (PFObject*)pObject;
	return p->clone();
}

PFObject_c * PFObject_detach(PPFObject_c pObject)
{
	if (!pObject)
		return NULL;
	PFObject * p = (PFObject*)pObject;
	return p->detach();
}

void PFObject_free(PPFObject_c pObject)
{
	if (!pObject)
		return;
	PFObject * p = (PFObject*)pObject;
	p->free();
}

PPFObject_c PFObject_create(tPF_ObjectType type, int nStreams)
{
	return ProtocolFilters::PFObject_create(type, nStreams);
}


#endif