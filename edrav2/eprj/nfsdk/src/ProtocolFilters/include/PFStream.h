//
// 	NetFilterSDK 
// 	Copyright (C) 2009 Vitaly Sidorov
//	All rights reserved.
//
//	This file is a part of the NetFilter SDK.
//	The code and information is provided "as-is" without
//	warranty of any kind, either expressed or implied.
//


#ifndef _PFSTREAM
#define _PFSTREAM

namespace ProtocolFilters
{
	typedef unsigned __int64 tStreamPos;
	typedef unsigned long tStreamSize;

#ifndef WIN32
#define FILE_BEGIN SEEK_SET
#define FILE_CURRENT SEEK_CUR
#define FILE_END SEEK_END
#endif

	class PFStream
	{
	public:
		virtual ~PFStream() {}

		/**
		 *	Deletes stream object
		 */
		virtual void free() = 0;

		/**
		 *	Moves the stream pointer to a specified location.
		 *	@param offset Offset from origin 
		 *	@param origin FILE_BEGIN, FILE_CURRENT or FILE_END
		 */
		virtual tStreamPos seek(tStreamPos offset, int origin) = 0;
		/**
		 *	Reads data from a stream and returns the number of bytes actually read.
		 *	@param buffer Pointer to data buffer 
		 *	@param size Size of buffer
		 */
		virtual tStreamSize read(void * buffer, tStreamSize size) = 0;
		/**
		 *	Writes data to a stream and returns the number of bytes actually written.
		 *	@param buffer Pointer to data buffer 
		 *	@param size Size of buffer
		 */
		virtual tStreamSize write(const void * buffer, tStreamSize size) = 0;
		/**
		 *	Returns number of bytes in stream.
		 */
		virtual tStreamPos size() = 0;
		/**
		 *	Copies stream contents to another stream.
		 */
		virtual tStreamPos copyTo(PFStream * pStream) = 0;
		/**
		 *	Set the stream end at the current position.
		 */
		virtual tStreamPos setEndOfStream() = 0;

		/**
		 *	Clear the stream contents.
		 */
		virtual void reset() = 0;
	};

}

#endif // _PFStream