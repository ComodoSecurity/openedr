//
// edrav2.libcore project
// 
// Author: Denis Kroshin (19.12.2018)
// Reviewer: Denis Bogdanov (29.12.2018)
//
///
/// @file File stream class declaration
///
/// @weakgroup io IO subsystem
/// @{
#pragma once
#include <io.hpp>
#include <objects.h>

namespace cmd {
namespace io {

///
/// TODO: brief description place here
///
/// TODO: full description place here
///
class MemoryStream : public ObjectBase <CLSID_MemoryStream>, 
	public IReadableStream,
	public IWritableStream,
	public IMemoryBuffer
{
private:
	static const Size c_nMaxStreamSize = 0x40000000;
	static const Size c_nStreamGranulation = 0x0400;
	static const Size c_nMaxGrowSize = 0x00400000;

	Byte* m_pData = nullptr;
	MemPtr<Byte> m_pBuffer;
	
	Size m_nMaxStreamSize = c_nMaxStreamSize;
	bool m_fCanRealloc = true;
	bool m_fReadOnly = false;
	Size m_nBufferSize = 0;
	Size m_nDataSize = 0;
	Size m_nOffset = 0;	

	Size m_nResizeCount = 1;

protected:
	MemoryStream() 
	{};
	
	~MemoryStream() override 
	{}

	//
	// Reserve buffer memory to prevent frequent memory allocations without logical resizing
	//
	void reserveMemory(Size nSize);

public:

	//
	//
	//
	static auto create(void* pBuffer, Size nSize)
	{
		if (pBuffer == NULL)
			error::InvalidArgument(SL, "Invalid buffer ptr").throwException();

		auto pObj = createObject<MemoryStream>();
		pObj->m_pData = static_cast<Byte*>(pBuffer);
		pObj->m_nBufferSize = nSize;
		pObj->m_nDataSize = nSize;
		pObj->m_fCanRealloc = false;
		return pObj;
	}

	//
	//
	//
	static auto create(const void* pBuffer, Size nSize)
	{
		auto pObj = create((void*)pBuffer, nSize);
		pObj->m_fReadOnly = true;
		return pObj;
	}

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **size** [opt] - max size of the stream.
	///   **allocMem** [opt] - allocate all memory at once (default: `false`).
	/// @throw std::bad_alloc Throw if can't allocate memory for stream.
	///
	void finalConstruct(Variant vConfig);

	// IReadableStream

	/// @copydoc IReadableStream::read()
	Size read(void* pBuffer, Size nSize, Size* pProceedSize) override;

	// IWritableStream

	/// @copydoc IWritableStream::write()
	void write(const void* pBuffer, Size nSize) override;
	/// @copydoc IWritableStream::setSize()
	void setSize(IoSize nSize) override;
	/// @copydoc IWritableStream::flush()
	void flush() override {};

	// ISeekableStream

	/// @copydoc ISeekableStream::getPosition()
	IoSize getPosition() override;
	/// @copydoc ISeekableStream::setPosition()
	IoSize setPosition(IoOffset nOffset, StreamPos eType) override;
	/// @copydoc ISeekableStream::getSize()
	IoSize getSize() override;

	// IMemoryBuffer

	/// @copydoc IMemoryBuffer::getData()
	std::pair<void*, Size> getData() override;
};

} // namespace io
} // namespace cmd 
/// @}