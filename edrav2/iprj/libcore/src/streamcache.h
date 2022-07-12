//
// edrav2.libcore project
// 
// Author: Denis Kroshin (19.12.2018)
// Reviewer: Denis Bogdanov (29.12.2018)
//
///
/// @file Stream cache objects declaration
///
/// @addtogroup io IO subsystem
/// @{
#pragma once
#include <io.hpp>
#include <objects.h>

namespace cmd {
namespace io {

//
//
//
class RawReadableStreamCache : public ObjectBase <CLSID_RawReadableStreamCache>, 
	public IRawReadableStream
{
private:
	static const Size c_nDefaultCacheSize = 0x10000;	// 64 Kb

	ObjPtr<IRawReadableStream> m_pSourceStream;
	std::unique_ptr<Byte[]>	m_pCacheBuffer;
	Size m_nCacheSize = 0;
	Size m_nCurrCacheSize = 0;
	Size m_nCachePos = 0;

protected:
	RawReadableStreamCache()
	{};
	
	virtual ~RawReadableStreamCache() override 
	{};

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **stream** - source stream.
	///   **cacheSize** [opt] - a size of the cache.
	///
	void finalConstruct(Variant vConfig);

	// IRawReadableStream

	/// @copydoc IRawReadableStream::read()
	Size read(void* pBuffer, Size nSize, Size* pProceedSize) override;
};

//
//
//
class ReadableStreamCache : public ObjectBase <CLSID_ReadableStreamCache>, 
	public IReadableStream
{
private:
	static const Size c_nDefaultCacheSize = 0x00001000;
	static const Size c_nReadBackDevider = 20;

	ObjPtr<IReadableStream>	m_pSourceStream;
	IoSize m_nStreamPos = 0;
	IoSize m_nStreamSize = c_nMaxIoSize;

	std::unique_ptr<Byte[]>	m_pCacheBuffer;
	Size m_nCacheSize = 0;
	Size m_nCurrCacheSize = 0;
	IoSize m_nCachePos = 0;

protected:

	//
	//
	//
	ReadableStreamCache() 
	{};

	//
	//
	//
	virtual ~ReadableStreamCache() override 
	{};

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **stream** - source stream.
	///   **cacheSize** [opt] - a size of the cache.
	///
	void finalConstruct(Variant vConfig);

	// IReadableStream

	/// @copydoc IReadableStream::read()
	Size read(void* pBuffer, Size nSize, Size* pProceedSize) override;
	
	// ISeekableStream

	/// @copydoc ISeekableStream::getPosition()
	IoSize getPosition() override;
	/// @copydoc ISeekableStream::setPosition()
	IoSize setPosition(IoOffset nOffset, StreamPos eType = StreamPos::Begin) override;
	/// @copydoc ISeekableStream::getSize()
	IoSize getSize() override;
};

//
// TODO: WritableStreamCache
//

//
// 
//
class RawReadableStreamConverter : public ObjectBase<CLSID_RawReadableStreamConverter>, 
	public IReadableStream
{
	enum class ReadStatus
	{
		ok,
		eos,
		error
	};

	ReadStatus m_eLastStatus = ReadStatus::ok;

	ObjPtr<IRawReadableStream> m_pSourceStream;
	ObjPtr<IReadableStream> m_pInCacheStream;
	ObjPtr<IWritableStream> m_pOutCacheStream;

	IoSize m_nOffset = 0;
	IoSize m_nStreamSize = c_nMaxIoSize;

	void fillCache(IoSize nNewSize);

protected:
	RawReadableStreamConverter() {};
	virtual ~RawReadableStreamConverter() override {};

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **stream** - source stream.
	///   **size** [opt] - a size of the stream.
	///
	void finalConstruct(Variant vConfig);

	// IReadableStream
	
	/// @copydoc IReadableStream::read()
	Size read(void* pBuffer, Size nSize, Size* pProceedSize) override;
	
	// ISeekableStream

	/// @copydoc ISeekableStream::getPosition()
	IoSize getPosition() override;
	/// @copydoc ISeekableStream::setPosition()
	IoSize setPosition(IoOffset nOffset, StreamPos eType = StreamPos::Begin) override;
	/// @copydoc ISeekableStream::getSize()
	IoSize getSize() override;
};

} // namespace io
} // namespace cmd 

/// @}