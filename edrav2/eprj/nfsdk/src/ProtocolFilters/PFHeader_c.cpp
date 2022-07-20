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

#include "PFHeader_c.h"
#include "PFHeader.h"

const char * PFHeaderField_getName(PPFHeaderField_c hf)
{
	if (!hf)
		return NULL;
	ProtocolFilters::PFHeaderField * pField = (ProtocolFilters::PFHeaderField *)hf;
	return pField->name().c_str();
}

const char * PFHeaderField_getValue(PPFHeaderField_c hf)
{
	if (!hf)
		return NULL;
	ProtocolFilters::PFHeaderField * pField = (ProtocolFilters::PFHeaderField *)hf;
	return pField->value().c_str();
}

const char * PFHeaderField_getUnfoldedValue(PPFHeaderField_c hf)
{
	if (!hf)
		return NULL;
	ProtocolFilters::PFHeaderField * pField = (ProtocolFilters::PFHeaderField *)hf;
	return pField->unfoldedValue().c_str();
}

PPFHeader_c PFHeader_create()
{
	return (PPFHeader_c) new ProtocolFilters::PFHeader();
}

PPFHeader_c PFHeader_parse(const char * buf, int len)
{
	ProtocolFilters::PFHeader * pHeader = new ProtocolFilters::PFHeader();
	if (!pHeader)
		return NULL;

	if (!buf || (len == 0))
		return (PPFHeader_c)pHeader;

	if (!pHeader->parseString(buf, len))
	{
		delete pHeader;
		return NULL;
	}

	return (PPFHeader_c) pHeader;
}

void PFHeader_free(PPFHeader_c ph)
{
	if (!ph)
		return;
	ProtocolFilters::PFHeader * pHeader = (ProtocolFilters::PFHeader *)ph;
	if (pHeader)
	{
		delete pHeader;
	}
}

void PFHeader_addField(PPFHeader_c ph, const char * name, const char * value, int toStart)
{
	if (!ph)
		return;
	ProtocolFilters::PFHeader * pHeader = (ProtocolFilters::PFHeader *)ph;

	pHeader->addField(name, value, toStart != 0);
}

void PFHeader_removeFields(PPFHeader_c ph, const char * name, int exact)
{
	if (!ph)
		return;
	ProtocolFilters::PFHeader * pHeader = (ProtocolFilters::PFHeader *)ph;

	pHeader->removeFields(name, exact != 0);
}

void PFHeader_clear(PPFHeader_c ph)
{
	if (!ph)
		return;
	ProtocolFilters::PFHeader * pHeader = (ProtocolFilters::PFHeader *)ph;

	pHeader->clear();
}

PPFHeaderField_c PFHeader_findFirstField(PPFHeader_c ph, const char * name)
{
	if (!ph)
		return NULL;
	ProtocolFilters::PFHeader * pHeader = (ProtocolFilters::PFHeader *)ph;

	return (PPFHeaderField_c)pHeader->findFirstField(name);
}

int PFHeader_size(PPFHeader_c ph)
{
	if (!ph)
		return 0;
	ProtocolFilters::PFHeader * pHeader = (ProtocolFilters::PFHeader *)ph;

	return (int)pHeader->size();
}
 
PPFHeaderField_c PFHeader_getField(PPFHeader_c ph, int index)
{
	if (!ph)
		return NULL;
	ProtocolFilters::PFHeader * pHeader = (ProtocolFilters::PFHeader *)ph;

	return (PPFHeaderField_c)pHeader->getField(index);
}

#endif 

