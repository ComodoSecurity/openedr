//
// edrav2.libcore
// 
// Author: Denis Kroshin (19.12.2018)
// Reviewer: Denis Bogdanov (29.12.2018)
//
///
/// @file Streams processing definitions
///
/// @addtogroup io IO subsystem
/// IO subsystem provides a set of the objects and interfaces which
/// are intended to cover all the input-output operations using the
/// streams ideology.
/// @{
///
#pragma once

namespace cmd {
namespace io {

typedef std::uintmax_t IoSize;
typedef std::intmax_t IoOffset;

static constexpr IoSize c_nMaxIoSize = UINTMAX_MAX;

///
/// Basic read from stream interface.
///
class IRawReadableStream : OBJECT_INTERFACE
{
public:

	///
	/// Reads the data to a buffer.
	///
	/// @param[in] pBuffer - pointer to the buffer to read.
	/// @param[in] nSize - size to read.
	/// @param[out] pProceedSize - pointer to size of data read in fact.
	/// @returns The function returns an amount of data that is actually read.
	/// @throw IoException Throw exception if can't read data.
	virtual Size read(void* pBuffer, Size nSize, Size* pProceedSize = nullptr) = 0;
};

///
/// Basic write to stream interface.
///
class IRawWritableStream : OBJECT_INTERFACE
{
public:

	///
	/// Writes the data to a stream from buffer.
	///
	/// @param[in] pBuffer - pointer to the buffer to read from.
	/// @param[in] nSize - size of write data.
	/// @throw IoException Throw exception if can't write data.
	virtual void write(const void* pBuffer, Size nSize) = 0;

	///
	/// Flushes the cached data.
	///
	/// @throw IoException Throw exception if can't flush data.
	virtual void flush() = 0;
};

///
/// Relative position type.
///
enum class StreamPos
{
	Begin,
	End,
	Current
};

///
/// Basic interface for streams with position.
///
class ISeekableStream : OBJECT_INTERFACE
{
public:
	///
	/// Get current position in the stream.
	///
	/// @return The function returns a current position in the stream.
	/// @throw IoException Throw exception if can't get position.
	///
	virtual IoSize getPosition() = 0;

	///
	/// Set current position in the stream.
	///
	/// @param[in] nOffset - relative offset.
	/// @param[in] eType - type of relative offset.
	/// @return The function returns a current position in the stream.
	/// @throw IoException Throw exception if can't set position.
	///
	virtual IoSize setPosition(IoOffset nOffset, StreamPos eType = StreamPos::Begin) = 0;

	///
	/// Returns current size of the stream.
	///
	/// @return The function returns a size of the stream.
	/// @throw IoException Throw exception if can't get size.
	///
	virtual IoSize getSize() = 0;
};

///
/// Generic interface for streams that implement read functions.
///
class IReadableStream : public IRawReadableStream, public virtual ISeekableStream 
{};

///
/// Generic interface for streams that implement write functions.
///
class IWritableStream : public IRawWritableStream, public virtual ISeekableStream
{
public:
	///
	/// Set size of the stream.
	///
	/// @param[in] nSize - new size of the stream.
	/// @throw IoException Throw exception if can't set size.
	///
	virtual void setSize(IoSize nSize) = 0;
};

///
/// File processing mode.
///
CMD_DEFINE_ENUM_FLAG(FileMode)
{
	Read			= 0x00001,	///< Open existing file for reading
	Write			= 0x00002,	///< Open existing file for writing or create new file
	Append			= 0x00004,	///< Set file pointer to the end of file on every write 
	Attributes		= 0x00008,	///< Open file for attributes read only 
	Create			= 0x00010,	///< Always create file. Return error if file exists
	Truncate		= 0x00020,	///< Open file and truncate it to zero.
	Temp			= 0x00040,	///< Create temporary file. Removes after clothing all handles.
	CreatePath		= 0x00080,	///< Create directory path before file creation.
	ShareRead		= 0x01000,	///< Allow read access to the file.
	ShareWrite		= 0x02000,	///< Allow write access to the file.
};

///
/// File attributes
///
enum class FileAttribute : Enum
{
	Readonly,
	Hidden,
	System,
	Directory,
	Archive,
	Device,
	Normal,
	Temporary
};

///
/// File time types
///
enum class FileTime
{
	Create,
	LastAccess,
	LastWrite
};

///
///
///
class IFile : OBJECT_INTERFACE
{
public:
	///
	/// Checks the attribute of the file.
	///
	/// @param[in] eFileAttr - attribute type.
	/// @return The function returns a status of attribute (set or not).
	/// @throw IoException Throw exception if can't get attributes.
	///
	virtual bool checkAttribute(FileAttribute eFileAttr) = 0;
	
	///
	/// Gets file time.
	///
	/// @param[in] eFileTime - type time.
	/// @return The function returns the file time.
	/// @throw IoException Throw exception if can't get file time.
	///
	virtual tm getTime(FileTime eFileTime) = 0;

	///
	/// Closes file.
	///
	/// @throw IoException Throw exception if can't close file.
	///
	virtual void close() = 0;

	// TODO: Implement IInfoProvider
	//virtual Variant getInfo(Enum) = 0;
};

///
///
///
class IMemoryBuffer : OBJECT_INTERFACE
{
public:
	//
	// Gets a pointer to the buffer and size of data.
	//
	// @return The function returns a pair of pointer to and size of buffer.
	///
	virtual std::pair<void*, Size> getData() = 0;
};

///
/// TODO: Implement locking for files
///
enum class LockMode
{
};

///
///
///
class ILockableStream : OBJECT_INTERFACE
{
	virtual void lock(IoSize nOffset, IoSize nSize, LockMode eMode) = 0;
	virtual void unlock(IoSize nOffset, IoSize nSize) = 0;
};

///
/// Interface for operations on DictionaryStreamView.
///
class IDictionaryStreamView : OBJECT_INTERFACE
{
public:
	///
	/// Reloads the data from the stream.
	//
	virtual void reload() = 0;
};

///
/// Copy data from one stream to another.
///
/// @param[in] pDst - destination stream.
/// @param[in] pSrc - source stream.
/// @param[in] nSize [opt] - size of data to copy (default: UINTMAX_MAX).
/// @param[out] pProceedSize [opt] - size of data been copied.
/// @param[in] nBufferSize [opt] - size of cache buffer (default: 0x1000).
/// @return The function returns a size of the data read from source stream.
/// @throw IoException Throw exception if can't copy all data.
///
IoSize write(ObjPtr<IRawWritableStream> pDst, ObjPtr<IRawReadableStream> pStr, IoSize nSize = c_nMaxIoSize,
	IoSize* pProceedSize = nullptr, Size nBufferSize = 0x1000);

///
/// Writes string to stream.
///
/// @param pDst - destination stream.
/// @param sString - string to be written.
///
void write(ObjPtr<IRawWritableStream> pStream, const std::string_view sString);

///
/// Creates a memory stream based on external buffer
///
/// @param[in] pBuffer - pointer to buffer.
/// @param[in] nBufferSize - size of buffer.
/// @return The function returns a smart pointer to created stream.
///
ObjPtr<IReadableStream> createMemoryStream(void* pBuffer, Size nBufferSize);
ObjPtr<IReadableStream> createMemoryStream(const void* pBuffer, Size nBufferSize);

///
/// Creates memory stream with defined size.
///
/// @param[in] nBufferSize [opt] - size of the stream (default: SIZE_MAX).
/// @return The function returns a smart pointer to created stream.
/// @throw std::bad_alloc Throw exception if can't allocate memory for internal buffer.
///
ObjPtr<IReadableStream> createMemoryStream(Size nBufferSize = c_nMaxSize);

///
/// Creates a cache for IRawReadableStream.
///
/// @param[in] pStream - source stream.
/// @param[in] nCacheSize [opt] - size of cache buffer (default: 0x10000).
/// @return The function returns a smart pointer to created stream.
ObjPtr<IRawReadableStream> createStreamCache(ObjPtr<IRawReadableStream> pStream, Size nCacheSize = 0x10000);

///
/// Creates a cache for IReadableStream.
///
/// @param[in] pStream - source stream.
/// @param[in] nCacheSize [opt] - size of cache buffer (default: 0x10000).
/// @return The function returns a smart pointer to created stream.
///
ObjPtr<IReadableStream> createStreamCache(ObjPtr<IReadableStream> pStream, Size nCacheSize = 0x1000);

///
/// Converts IRawReadableStream to IReadableStream.
///
/// @param[in] pStream - source stream.
/// @param[in] nStreamSize [opt] - maximal output stream size (default: UINTMAX_MAX).
/// @return The function returns a smart pointer to created stream.
///
ObjPtr<IReadableStream> convertStream(ObjPtr<IRawReadableStream> pStream, IoSize nStreamSize = c_nMaxIoSize);

///
/// Creates a part of stream.
///
/// @param[in] pStream - source stream.
/// @param[in] nOffset - offset in the source stream.
/// @param[in] nSize [opt] - size of output stream (default: UINTMAX_MAX).
/// @return The function returns a smart pointer to created stream.
///
ObjPtr<IReadableStream> createChildStream(ObjPtr<IReadableStream> pStream, IoSize nOffset, IoSize nSize = c_nMaxIoSize);

/*
///
/// Create stream based on file (UTF-8 filename)
///
/// @param[in] sFileName Name of file
/// @param[in] eFlag Open mode
/// @return Smart pointer to stream
/// @throw IoException Throw exception if can't open/create file
///
ObjPtr<IReadableStream> createFileStream(const std::string_view& sFileName, const FileMode eFlag);

///
/// Create stream based on file (Unicode filename)
///
/// @param[in] sFileName Name of file
/// @param[in] eFlag Open mode
/// @return Smart pointer to stream
/// @throw IoException Throw exception if can't open/create file
///
ObjPtr<IReadableStream> createFileStream(const std::wstring_view& sFileName, const FileMode eFlag);
*/

///
/// Createa a stream based on file.
///
/// @param[in] "pathFile" - path to the file.
/// @param[in] "eFlag" - open mode.
/// @return The function returns a smart pointer to created stream.
/// @throw IoException Throw exception if can't open/create file
///
ObjPtr<IReadableStream> createFileStream(const std::filesystem::path& pathFile, const FileMode eFlag);

///
/// Safely creates a stream based on file
///
/// @param[in] "pathFile" - path to the file.
/// @param[in] "eFlag" - open mode
/// @param[out] "pEC" [opt] - pointer to error code.
/// @return The function returns a smart pointer to created stream or `nullptr` if fail.
///
ObjPtr<IReadableStream> createFileStreamSafe(const std::filesystem::path& pathFile,
	const FileMode eFlag, error::ErrorCode* pEC = nullptr);

///
/// Creates a temporary file stream.
///
/// @param[in] nMaxSize [opt] - maximal size of temp stream (default: UINTMAX_MAX).
/// @return The function returns a smart pointer to created stream.
/// @throw IoException Throw exception if can't open/create file.
///
ObjPtr<IReadableStream> createTempStream(IoSize nMaxSize = c_nMaxIoSize);

///
/// Create a null-stream.
///
/// @param[in] nSize [opt] - an initial stream size (default: 0).
/// @return The function returns a smart pointer to created stream.
///
ObjPtr<IReadableStream> createNullStream(Size nSize = 0);

///
/// Generates a temporary filename.
///
/// @return The function returns a temporary filename.
/// @throw IoException Throw exception if can't generate name.
///
std::filesystem::path getTempFileName();

///
/// Definitions to safely work with read/write functions.
///

#define CMD_IOEXCEPTION	pIoExpt

#define CMD_IOBEGIN		std::exception_ptr CMD_IOEXCEPTION = nullptr;

// FIXME: Inherit old stack trace
#define CMD_IOEND		if (pIoExpt != nullptr) \
							std::rethrow_exception(CMD_IOEXCEPTION);

#define CMD_IOSAFE(FN)	try { FN; } \
						catch (...) { \
							if (CMD_IOEXCEPTION == nullptr) \
								CMD_IOEXCEPTION = std::current_exception(); \
						}

///
/// Saving stream data into Variant.
///
/// The function converts a stream into base64 string 
/// and return wrapper for create stream from this stream during deserialization.
///
/// @param pStream - the stream object.
/// @return The function returns the Variant value.
///
Variant serializeStream(ObjPtr<io::IReadableStream> pStream);

namespace win {

///
/// Creates a stream based on file.
///
/// @param[in] "pathFile" - path to file.
/// @param[in] "eFlag" - open mode.
/// @return The function returns a smart pointer to created stream.
/// @throw IoException Throw exception if can't open/create file
///
ObjPtr<IReadableStream> createFileStream(const std::filesystem::path& pathFile, const FileMode eFlag);

///
/// Creates a stream based on file handle.
///
/// @param[in] "handle" - handle to file.
/// @return The function returns a smart pointer to created stream.
/// @throw IoException Throw exception if can't open/create file.
///
ObjPtr<cmd::io::IReadableStream> createFileStream(const HANDLE hFile);

///
/// Safely creates a stream based on file.
///
/// @param[in] "pathFile" - path to file.
/// @param[in] "eFlag" - open mode.
/// @param[out] "pEC" [opt] - pointer to error code.
/// @return The function returns a smart pointer to created stream or `nullptr` if fail.
///
ObjPtr<IReadableStream> createFileStreamSafe(const std::filesystem::path& pathFile, 
	const FileMode eFlag, error::ErrorCode* pEC = nullptr);

} // namespace win
} // namespace io 
} // namespace cmd 

/// @}
