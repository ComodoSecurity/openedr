//
// edrav2.libcore project
// 
// Author: Denis Kroshin (19.12.2018)
// Reviewer: Denis Bogdanov (29.12.2018)
//
///
/// @file Streams processing declaration
///
/// @weakgroup io IO subsystem
/// @{
#pragma once
#include <io.hpp>
#include <queue.hpp>
#include <objects.h>

namespace cmd {

namespace io {

///
/// Converts a position from relative to absolute.
///
/// @param[in] nOffset - relative offset.
/// @param[in] eType - type of relative offset.
/// @param[in] nCurPos - current absolute position.
/// @param[in] nMaxPos - current relative position (often stream size).
/// @return The function returns a new absolute position.
///
IoSize convertPosition(IoOffset nOffset, StreamPos eType, IoSize nCurPos, IoSize nMaxPos);

///
/// Child stream class.
///
class ChildStream : public ObjectBase <CLSID_PartOfStream>,
	public IReadableStream
{
private:
	ObjPtr<IReadableStream>	m_pSourceStream;
	
	IoSize m_nSourceOffset = 0;
	IoSize m_nSize = 0;
	IoSize m_nCurrPos = 0;

protected:
	ChildStream() {};
	virtual ~ChildStream() override {};

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **stream** - source stream.
	///   **offset** - offset in source stream.
	///   **size** - size of new stream.
	///
	void finalConstruct(Variant vConfig);

	// IReadableStream

	/// @copydoc IReadableStream::read()
	Size read(void* pBuffer, Size nSize, Size* pProceedSize) override;

	// ISeekableStream

	/// @copydoc ISeekableStream::getPosition()
	IoSize getPosition() override;
	/// @copydoc ISeekableStream::setPosition()
	IoSize setPosition(IoOffset nOffset, StreamPos eType) override;
	/// @copydoc ISeekableStream::getSize()
	IoSize getSize() override;
};


///
/// The class provides stream access to utf8 string.
///
class StringReadableStream : public ObjectBase <CLSID_StringReadableStream>,
	public IReadableStream
{
private:
	std::string m_sData;
	Size m_nOffset = 0;

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **data** - data of the new stream.
	/// 
	void finalConstruct(Variant vConfig);

	// IReadableStream

	/// @copydoc IReadableStream::read()
	Size read(void* pBuffer, Size nSize, Size* pProceedSize) override;

	// ISeekableStream

	/// @copydoc ISeekableStream::getPosition()
	IoSize getPosition() override;
	/// @copydoc ISeekableStream::setPosition()
	IoSize setPosition(IoOffset nOffset, StreamPos eType) override;
	/// @copydoc ISeekableStream::getSize()
	IoSize getSize() override;
};

///
/// Null stream class.
///
class NullStream : public ObjectBase <CLSID_NullStream>,
	public IReadableStream,
	public IWritableStream,
	public IDataReceiver,
	public IQueueNotificationAcceptor
{
private:
	ObjPtr<IWritableStream>	m_pDstStream;
	IoSize m_nCurrPos = 0;
	IoSize m_nSize = c_nMaxSize;
	std::string m_sQueueName;

protected:
	NullStream() {};
	virtual ~NullStream() override {};

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **size** - initial size of the stream.
	///   **queueName** - a name of the queue for getting the data.
	/// 
	void finalConstruct(Variant vConfig);

	// IRawReadableStream

	/// @copydoc IRawReadableStream::read()
	Size read(void* pBuffer, Size nSize, Size* pProceedSize) override;

	// IWritableStream

	/// @copydoc IWritableStream::write()
	void write(const void* pBuffer, Size nSize) override;
	/// @copydoc IWritableStream::flush()
	void flush() override;
	/// @copydoc IWritableStream::setSize()
	void setSize(IoSize nSize) override;

	// ISeekableStream

	/// @copydoc ISeekableStream::getPosition()
	IoSize getPosition() override;
	/// @copydoc ISeekableStream::setPosition()
	IoSize setPosition(IoOffset nOffset, StreamPos eType = StreamPos::Begin) override;
	/// @copydoc ISeekableStream::getSize()
	IoSize getSize() override;

	// IDataReceiver

	/// @copydoc IDataReceiver::put()
	void put(const Variant& vData) override;

	// IQueueNotificationAcceptor

	/// @copydoc IQueueNotificationAcceptor::notifyAddQueueData()
	void notifyAddQueueData(Variant vTag) override;
	/// @copydoc IQueueNotificationAcceptor::notifyQueueOverflowWarning()
	void notifyQueueOverflowWarning(Variant vTag) override;
};

} // namespace io
} // namespace cmd 
/// @}