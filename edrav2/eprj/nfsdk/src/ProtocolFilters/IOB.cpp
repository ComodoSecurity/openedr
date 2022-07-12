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
#include "IOB.h"

using namespace ProtocolFilters;

IOB::IOB() : 
				m_pBuffer(NULL), 
				m_uOffset(0),
				m_uSize(0), 
				m_uLimit(100*1024*1024),
				m_strategy(AS_RESERVE)
{

}

IOB::~IOB()
{
	reset();
}

IOB::IOB(const IOB & copyFrom)
{
	m_pBuffer = NULL;
	*this = copyFrom;
}
		
IOB & IOB::operator = (const IOB & copyFrom)
{
	reset();
	
	m_uLimit = copyFrom.m_uLimit;

	m_strategy = copyFrom.m_strategy;

	append(copyFrom.buffer(), copyFrom.size());
	
	return *this;
}


bool IOB::append(const char *_pBuf, unsigned long _uLen, bool copy)
{
	unsigned long	uRequired;
	char *		p;

	if ((!_pBuf && copy) || _uLen == 0)
		return false;

	uRequired = m_uOffset + _uLen + 1;

	if (uRequired > m_uLimit)
	{
		return false;
	}

	if (uRequired > m_uSize)
	{
		unsigned long uWant = m_uSize ? m_uSize : 10;

		if (m_strategy == AS_RESERVE)
		{
			while (uWant <= uRequired) 
			{
				uWant *= 2;
			}
		} else
		{
			uWant = uRequired;
		}

		if (uWant >= m_uLimit)
		{
			uWant = uRequired;
		}
		
		if (m_pBuffer)
		{
			if (NULL == (p = (char *)realloc(m_pBuffer, uWant)))
			{
				return false;
			}
		} else
		{
			if (NULL == (p = (char *)malloc(uWant)))
			{
				return false;
			}
		}
		m_uSize = uWant;

		m_pBuffer = p;
   }

	if (copy)
	{
		memcpy(m_pBuffer + m_uOffset, _pBuf, (size_t)_uLen);
	}

	m_uOffset += _uLen;

	m_pBuffer[m_uOffset] = '\0';

	return true;
}

bool IOB::append(char c)
{
	return append(&c, 1);
}


void IOB::setLimit(unsigned long _uLimit)
{
	m_uLimit = _uLimit;
}

unsigned long IOB::getLimit() const
{
	return m_uLimit;
}

void IOB::reset()
{
	m_uSize = 0;
	m_uOffset = 0;
	if (m_pBuffer)
	{
		::free(m_pBuffer);
		m_pBuffer = NULL;
	}
}

char * IOB::buffer() const
{
	return m_pBuffer;
}

unsigned long IOB::capacity() const
{
	return m_uSize;
}

unsigned long IOB::size() const
{
	return m_uOffset;
}

bool IOB::reserve(unsigned long len)
{
	if (len <= m_uSize)
		return true;

	return append(NULL, len - m_uSize, false);
}

bool IOB::resize(unsigned long len)
{
	if (len < m_uSize)
	{
		m_uOffset = len;
		m_pBuffer[m_uOffset] = '\0';
		return true;
	}

	return append(NULL, len - m_uSize, false);
}

