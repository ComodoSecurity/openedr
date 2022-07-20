//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#if !defined(_IOB_H_)
#define _IOB_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

namespace ProtocolFilters
{
	/**
	 *	Wrapper for operations with memory buffers	
	 */
	class IOB  
	{
	public:
		IOB();
		
		virtual ~IOB();

		IOB(const IOB & copyFrom);
		
		IOB & operator = (const IOB & copyFrom);

		/**
		 *	Appends the specified buffer to IOB. The IOB buffer is 
		 *	resized if needed.
		 *	@param _pBuf Buffer to append
		 *	@param _uLen Length of _pBuf
		 *	@param copy true, if _pBuf contents must be copied. 
		 *		If <tt>copy</tt> is false, then _pBuf is ignored
		 *		and IOB is simply resized by _uLen bytes.
		 */
		bool append(const char * _pBuf, unsigned long _uLen, bool copy = true);

		/**
		 *	Appends a byte to the end of IOB
		 */
		bool append(char c);
		
		/**
		 *	Returns the size of allocated memory in IOB
		 */
		unsigned long capacity() const;

		/**
		 *	Returns the size of object stored in IOB	
		 */
		unsigned long size() const;
		
		/**
		 *	Makes the IOB capacity greater or equal to len
		 *	@param len The minimum required IOB capacity
		 */
		bool reserve(unsigned long len);

		/**
		 *	Sets the size for the object in IOB. 
		 *	The object length is truncated if necessary.
		 */
		bool resize(unsigned long len);
		
		/**
		 *	Returns a pointer to IOB buffer	
		 */
		char * buffer() const;
		
		/**
		 *	Free the buffer
		 */
		void reset();
		
		/**
		 *	Returns maximum possible size of stored object	
		 */
		unsigned long getLimit() const;
		
		/**
		 *	Set the maximum possible size of stored object
		 */
		void setLimit(unsigned long _uLimit);

		enum eAllocationStrategy
		{
			/**
			 *	Allocate no more than needed	
			 */
			AS_STRICT, 
			/**
			 *	Assumes that many appends are possible and does the best 
			 *  for making less reallocations
			 */
			AS_RESERVE
		};

		void setAllocationStrategy(eAllocationStrategy strategy)
		{
			m_strategy = strategy;
		}

	private:
		char *		m_pBuffer;
		unsigned long	m_uOffset;
		unsigned long	m_uSize;
		unsigned long	m_uLimit;
		eAllocationStrategy m_strategy;
	};
}

#endif // !defined(_IOB_H_)
