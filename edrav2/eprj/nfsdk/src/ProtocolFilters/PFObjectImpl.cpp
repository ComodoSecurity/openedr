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
#include "PFObjectImpl.h"
#include "MFStream.h"

using namespace ProtocolFilters;

PFObjectImpl::PFObjectImpl(tPF_ObjectType type, int nStreams, bool isStatic) :
	m_objectType(type),
	m_nStreams(nStreams),
	m_static(isStatic)
{
	if (nStreams > 0)
	{
		m_ppStreams = (PFStream **)malloc(sizeof(PFStream *) * nStreams);
		if (m_ppStreams)
		{
			memset(m_ppStreams, 0, sizeof(PFStream *) * nStreams);
		}
	} else
	{
		m_ppStreams = NULL;
	}

	m_readOnly = false;
}

PFObjectImpl::~PFObjectImpl()
{
	freeStreams();
}

PFObjectImpl::PFObjectImpl(const PFObjectImpl & object)
{
	*this = object;
}

void PFObjectImpl::free()
{
	if (!m_static)
	{
		delete this;
	}
}

void PFObjectImpl::freeStreams()
{
	if (m_ppStreams)
	{
		for (int i=0; i<m_nStreams; i++)
		{
			if (m_ppStreams[i])
			{
				m_ppStreams[i]->free();
				m_ppStreams[i] = NULL;
			}
		}

		::free(m_ppStreams);
		m_ppStreams = NULL;
	}
}

PFObjectImpl & PFObjectImpl::operator = (const PFObjectImpl & object)
{
	if (&object == this)
		return *this;

	if (m_ppStreams)
	{
		freeStreams();
	}

	m_objectType = object.m_objectType;
	m_nStreams = object.m_nStreams;

	if (m_nStreams > 0)
	{
		m_ppStreams = (PFStream **)malloc(sizeof(PFStream *) * m_nStreams);
		
		if (m_ppStreams)
		{
			memset(m_ppStreams, 0, sizeof(PFStream *) * m_nStreams);

			for (int i=0; i<m_nStreams; i++)
			{
				PFStream * pStream = object.m_ppStreams[i];

				if (pStream)
				{
					m_ppStreams[i] = createByteStream();
					
					if (m_ppStreams[i])
					{
						pStream->copyTo(m_ppStreams[i]);
					}
				}
			}
		}
	}

	return *this;
}

tPF_ObjectType PFObjectImpl::getType() 
{
	return m_objectType;
}

void PFObjectImpl::setType(tPF_ObjectType type)
{
	m_objectType = type;
}

bool PFObjectImpl::isReadOnly()
{
	return m_readOnly;
}

void PFObjectImpl::setReadOnly(bool value)
{
	m_readOnly = value;
}

int PFObjectImpl::getStreamCount()
{
	return m_nStreams;
}

PFStream * PFObjectImpl::getStream(int index) 
{
	if (index < 0 || index >= m_nStreams)
		return NULL;

	if (!m_ppStreams[index])
	{
		m_ppStreams[index] = createByteStream();
	}

	return m_ppStreams[index];
}

void PFObjectImpl::clear()
{
	if (m_ppStreams)
	{
		for (int i=0; i<m_nStreams; i++)
		{
			if (m_ppStreams[i])
			{
				m_ppStreams[i]->reset();
			}
		}
	}
}

PFObjectImpl * PFObjectImpl::clone()
{
	PFObjectImpl * pNewObject = new PFObjectImpl(m_objectType, m_nStreams, false);
	if (!pNewObject)
		return NULL;

	*pNewObject = *this;

	return pNewObject;
}

PFObjectImpl * PFObjectImpl::detach()
{
	PFObjectImpl * pNewObject = new PFObjectImpl(m_objectType, m_nStreams, false);
	if (!pNewObject)
		return NULL;

	if (m_ppStreams)
	{
		for (int i=0; i<m_nStreams; i++)
		{
			if (m_ppStreams[i])
			{
				pNewObject->m_ppStreams[i] = m_ppStreams[i];
				m_ppStreams[i] = NULL;
			}
		}
	}

	return pNewObject;
}

PFObject * ProtocolFilters::PFObject_create(tPF_ObjectType type, int nStreams)
{
	return new PFObjectImpl(type, nStreams, false);
}
