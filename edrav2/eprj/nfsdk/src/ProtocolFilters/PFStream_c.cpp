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

#include "PFStream_c.h"
#include "PFStream.h"

tStreamPos PFStream_seek(PPFStream_c s, tStreamPos offset, int origin)
{
	if (!s)
		return 0;
	ProtocolFilters::PFStream * p = (ProtocolFilters::PFStream*)s;
	return p->seek(offset, origin);
}

tStreamSize PFStream_read(PPFStream_c s, void * buffer, tStreamSize size)
{
	if (!s)
		return 0;
	ProtocolFilters::PFStream * p = (ProtocolFilters::PFStream*)s;
	return p->read(buffer, size);
}

tStreamSize PFStream_write(PPFStream_c s, const void * buffer, tStreamSize size)
{
	if (!s)
		return 0;
	ProtocolFilters::PFStream * p = (ProtocolFilters::PFStream*)s;
	return p->write(buffer, size);
}

tStreamPos PFStream_size(PPFStream_c s)
{
	if (!s)
		return 0;
	ProtocolFilters::PFStream * p = (ProtocolFilters::PFStream*)s;
	return p->size();
}

tStreamPos PFStream_setEndOfStream(PPFStream_c s)
{
	if (!s)
		return 0;
	ProtocolFilters::PFStream * p = (ProtocolFilters::PFStream*)s;
	return p->setEndOfStream();
}

void PFStream_reset(PPFStream_c s)
{
	if (!s)
		return;
	ProtocolFilters::PFStream * p = (ProtocolFilters::PFStream*)s;
	p->reset();
}

tStreamPos PFStream_copyTo(PPFStream_c s, PPFStream_c d)
{
	if (!s || !d)
		return 0;
	ProtocolFilters::PFStream * ps = (ProtocolFilters::PFStream*)s;
	ProtocolFilters::PFStream * pd = (ProtocolFilters::PFStream*)s;
	return ps->copyTo(pd);
}

#endif