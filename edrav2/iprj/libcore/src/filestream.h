//
// edrav2.libcore project
// 
// Author: Denis Kroshin (15.01.2019)
// Reviewer: Denis Bogdanov (15.01.2019)
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
#include <objects.h>

namespace cmd {
namespace io {

///
/// TODO: brief description place here
///
/// TODO: full description place here
///
class File : public ObjectBase <CLSID_File>, 
	public IReadableStream, 
	public IWritableStream, 
	public IFile,
	public ICommandProcessor,
	public IDataReceiver
{

private:
	int m_nHandle = -1;
	bool fOpen = false;
	bool m_fUseSimplePut = false;
	std::string m_sPutDelimiter;
	std::filesystem::path m_pathToFile;
	std::mutex m_mtxPutVariant;

	//
	// Get mode string from enum
	//
	inline std::pair<int, int> getMode(FileMode eMode)
	{
		int nOpenFlag = O_BINARY;
		if (testFlag(eMode, FileMode::Read) && !testFlag(eMode, FileMode::Write))
			nOpenFlag |= O_RDONLY;
		else if (!testFlag(eMode, FileMode::Read) && testFlag(eMode, FileMode::Write))
			nOpenFlag |= O_WRONLY | O_CREAT;
		else if (testFlag(eMode, FileMode::Read) && testFlag(eMode, FileMode::Write))
			nOpenFlag |= O_RDWR | O_CREAT;
		else
			error::InvalidArgument(SL, "R/W mode is not specified").throwException();

		if (testFlag(eMode, FileMode::Append))
			nOpenFlag |= O_APPEND;

		if (testFlag(eMode, FileMode::Create))
			if (testFlag(eMode, FileMode::Write))
				nOpenFlag |= O_EXCL;
			else
				error::InvalidArgument(SL, "Invalid mode <Create without Write>").throwException();

		if (testFlag(eMode, FileMode::Truncate))
			if (!testFlag(nOpenFlag, O_RDONLY))
				nOpenFlag |= O_TRUNC;
			else
				error::InvalidArgument(SL, "Invalid mode <Truncate & Read>").throwException();

		if (testFlag(eMode, FileMode::Temp))
			if (testFlag(nOpenFlag, O_CREAT))
				nOpenFlag |= O_TEMPORARY;
			else
				error::InvalidArgument(SL).throwException();

		int nShareFlag = 0;
		if (testFlag(eMode, FileMode::ShareRead) && !testFlag(eMode, FileMode::ShareWrite))
			nShareFlag = SH_DENYWR;
		else if (!testFlag(eMode, FileMode::ShareRead) && testFlag(eMode, FileMode::ShareWrite))
			nShareFlag = SH_DENYRD;
		else if (testFlag(eMode, FileMode::ShareRead) && testFlag(eMode, FileMode::ShareWrite))
			nShareFlag = SH_DENYNO;
		else
			nShareFlag = SH_DENYRW;

		return std::pair<int, int>(nOpenFlag, nShareFlag);
	}

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
	{
		close();
	};

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

} // namespace io
} // namespace cmd
/// @} 
