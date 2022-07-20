//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_PFOBJECT_IMPL)
#define _PFOBJECT_IMPL

#include "PFObject.h"

namespace ProtocolFilters
{
	/**
	 *	An instance of filtered object
	 */
	class PFObjectImpl : public PFObject 
	{
	public:
		/**
		 *	@param type Type of the object
		 */
		PFObjectImpl(tPF_ObjectType type, int nStreams = 1, bool isStatic = true);
		virtual ~PFObjectImpl();

		/**
		 *	Copy constructor	
		 */		
		PFObjectImpl(const PFObjectImpl & object);
		
		virtual PFObjectImpl & operator = (const PFObjectImpl & object);

		/**
		 *	Returns object type
		 */
		tPF_ObjectType getType();

		/**
		 *	Set object type	
		 */
		void setType(tPF_ObjectType type);

		bool isReadOnly();

		void setReadOnly(bool value);

		int getStreamCount();

		PFStream * getStream(int index = 0);

		void clear();

		PFObjectImpl * clone();

		PFObjectImpl * detach();

		void free();
	
	protected:
		void freeStreams();

	private:
		tPF_ObjectType	m_objectType;
		PFStream **		m_ppStreams;
		int				m_nStreams;
		bool			m_readOnly;
		bool			m_static;
	};


}

#endif 
