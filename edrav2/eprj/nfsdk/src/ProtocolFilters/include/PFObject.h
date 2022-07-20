//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_PFOBJECT)
#define _PFOBJECT

#include "PFCDefs.h"
#include "PFStream.h"
#include "PFFilterDefs.h"

namespace ProtocolFilters
{
	/**
	 *	Interface to object data
	 */
	class PFObject  
	{
	public:
		virtual ~PFObject(){}

		/**
		 *	Returns object type
		 */
		virtual tPF_ObjectType getType() = 0;

		/**
		 *	Set object type	
		 */
		virtual void setType(tPF_ObjectType type) = 0;

		/**
		 *	Returns true if the object is read-only, i.e. 
		 *	posting it to session has no effect
		 */ 
		virtual bool isReadOnly() = 0;

		/**
		 *	Assigns read-only flag
		 */
		virtual void setReadOnly(bool value) = 0;
		
		/**
		 *	Returns a number of object streams
		 */
		virtual int getStreamCount() = 0;

		/**
		 *	Returns a stream at the specified position
		 */
		virtual PFStream * getStream(int index = 0) = 0;

		/**
		 *	Clears object streams
		 */
		virtual void clear() = 0;

		/**
		 *	Creates a copy of object
		 */
		virtual PFObject * clone() = 0;

		/**
		 *	Creates a copy of object by moving all streams of old object to new one.
		 */
		virtual PFObject * detach() = 0;

		/**
		 *	Deletes object in case if it was created via PFObject_create() or clone()
		 */
		virtual void free() = 0;
	};

	/**
	 *	Creates a new object with specified type and number of streams
	 */
	PF_API PFObject * PFObject_create(tPF_ObjectType type, int nStreams);
}

#endif // _PFObject
