//
// EDRAv2.libcore
// 
// Author: Denis Kroshin (01.04.2019)
// Reviewer: Denis Bogdanov
//
///
/// @file File stream class declaration
///
/// @weakgroup io IO subsystem
/// @{
#pragma once
#include <io.hpp>
#include <command.hpp>
#include <common.hpp>
#include <utilities.hpp>
#include <system.hpp>
#include <objects.h>

namespace cmd {
namespace io {
namespace win {

///
/// TODO: brief description place here
///
/// TODO: full description place here
///
class File : public ObjectBase <CLSID_FileWin>, 
	public io::IReadableStream, 
	public IWritableStream, 
	public IFile,
	public ICommandProcessor,
	public IDataReceiver
{

private:
	sys::win::ScopedFileHandle m_nHandle;
	std::filesystem::path m_pathToFile;
	std::string m_sPutDelimiter;

	//
	// Convert mode definition from string to flags
	//
	// 'r' - FileMode::Read
	// 'w' - FileMode::Write
	// 'a' - FileMode::Append
	// 'c' - FileMode::Create
	// 't' - FileMode::Truncate
	// 'T' - FileMode::Temp
	// 'P' - FileMode::CreatePath
	// 'R' - FileMode::ShareRead
	// 'W' - FileMode::ShareWrite
	//
	inline FileMode convertModeStr(std::string_view sMode)
	{
		FileMode eMode = static_cast<FileMode>(0);
		for (char ch : sMode)
		{
			if (ch == 'r')
				eMode |= FileMode::Read;
			else if (ch == 'w')
				eMode |= FileMode::Write;
			else if (ch == 'a')
				eMode |= FileMode::Append;
			else if (ch == 'c')
				eMode |= FileMode::Create;
			else if (ch == 't')
				eMode |= FileMode::Truncate;
			else if (ch == 'T')
				eMode |= FileMode::Temp;
			else if (ch == 'P')
				eMode |= FileMode::CreatePath;
			else if (ch == 'R')
				eMode |= FileMode::ShareRead;
			else if (ch == 'W')
				eMode |= FileMode::ShareWrite;
			else
				error::InvalidArgument(SL).throwException();
		}
		return eMode;
	}

	//
	// Get position type
	//
	inline int getOrigin(StreamPos ePos)
	{
		if (ePos == StreamPos::Begin)
			return SEEK_SET;
		if (ePos == StreamPos::End)
			return SEEK_END;
		if (ePos == StreamPos::Current)
			return SEEK_CUR;
		error::InvalidArgument(SL, "Invalid position type").throwException();
	}


	inline time_t fileTimeToTimeT(const FILETIME& ft)
	{
		ULARGE_INTEGER ull;
		ull.LowPart = ft.dwLowDateTime;
		ull.HighPart = ft.dwHighDateTime;
		return ull.QuadPart / 10000000ULL - 11644473600ULL;
	}

protected:
	//
	//
	//
	File()
	{};
	
	//
	//
	//
	~File() override
	{};

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	/// the following fields:
	///   **fileName** - a name of the file.
	///   **mode** - file creation/open mode.
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
	void flush() override;

	// ISeekableStream

	/// @copydoc ISeekableStream::getPosition()
	IoSize getPosition() override;
	/// @copydoc ISeekableStream::setPosition()
	IoSize setPosition(IoOffset nOffset, StreamPos eType = StreamPos::Begin) override;
	/// @copydoc ISeekableStream::getSize()
	IoSize getSize() override;

	// IFile

	/// @copydoc IFile::checkAttribute()
	bool checkAttribute(FileAttribute eFileAttribute) override;
	/// @copydoc IFile::getTime()
	tm getTime(FileTime eFileTime) override;
	/// @copydoc IFile::close()
	void close() override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;

	// IDataReceiver

	/// @copydoc IDataReceiver::put()
	void put(const Variant& vData) override;
};

} // namespace win
} // namespace io
} // namespace cmd
/// @} 
