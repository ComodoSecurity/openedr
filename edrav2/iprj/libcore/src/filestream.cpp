//
// edrav2::libcore
// File stream class implementation
// 
// Author: Denis Kroshin (15.01.2019)
// Reviewer: Denis Bogdanov (15.01.2019)
//
///
/// @file File stream class implementation
///
/// @weakgroup io IO subsystem
/// @{
#include "pch.h"
#include "filestream.h"
#include "stream.h"

namespace cmd {
namespace io {

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "filestrm"

/*
//
//
//
ObjPtr<IReadableStream> createFileStream(const std::string_view& sFileName, const FileMode eFlag)
{
	TRACE_BEGIN;
	auto pObj = createObject(CLSID_File,
		Dictionary({ {"path", sFileName}, {"mode", eFlag} }));
	return queryInterface<IReadableStream>(pObj);
	TRACE_END(std::string("Can't process file <") + std::string(sFileName) + ">");
}

//
//
//
ObjPtr<IReadableStream> createFileStream(const std::wstring_view& sFileName, const FileMode eFlag)
{
	auto pObj = createObject(CLSID_File,
		Dictionary({ {"path", sFileName}, {"mode", eFlag} }));
	return queryInterface<IReadableStream>(pObj);
}
*/

//
//
//
ObjPtr<cmd::io::IReadableStream> createFileStream(const std::filesystem::path& pathFile, const FileMode eFlag)
{
	TRACE_BEGIN;
	auto pObj = createObject(CLSID_File,
		Dictionary({ {"path", pathFile.c_str()}, {"mode", eFlag} }));
	return queryInterface<IReadableStream>(pObj);
	TRACE_END(std::string("Can't process file <") + pathFile.u8string() + ">");
}

//
//
//
ObjPtr<cmd::io::IReadableStream> createFileStreamSafe(const std::filesystem::path& pathFile,
	const FileMode eFlag, error::ErrorCode* pEC)
{
	if (pEC != nullptr)
		*pEC = error::ErrorCode::OK;
	CMD_TRY
	{
		auto pObj = createObject(CLSID_File,
			Dictionary({ {"path", pathFile.c_str()}, {"mode", eFlag} }));
		return queryInterface<IReadableStream>(pObj);
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{
		if (pEC != nullptr)
			*pEC = e.getErrorCode();
	}
	catch (...)
	{
		if (pEC != nullptr)
			*pEC = error::ErrorCode::Undefined;
	}
	return nullptr;
}


//
// @kroshin FIXME: Get temp path from global catalog
//
std::filesystem::path getTempFileName()
{
#ifdef _MSC_VER
	wchar_t sFileName[L_tmpnam_s];
	if (_wtmpnam_s(sFileName, L_tmpnam_s))
		error::CrtError(SL).throwException();
#else
#error Add other OS implementations.
	//	tmpnam_s(sFileName, L_tmpnam_s)
#endif
	return sFileName;
}

//
//
//
ObjPtr<IReadableStream> createTempStream(IoSize nMaxSize)
{
	// FIXME: Implement nMaxSize restrictions (@kroshin)
	if (nMaxSize > c_nMaxIoSize)
		nMaxSize = c_nMaxIoSize;

	auto pObj = createObject(CLSID_File,
		Dictionary({ {"path", getTempFileName().c_str()},
			{"mode", FileMode::Read | FileMode::Write | FileMode::Create | FileMode::Temp} }));
	return queryInterface<IReadableStream>(pObj);
}


//
//
//
void File::finalConstruct(Variant vConfig)
{
	m_pathToFile = vConfig["path"];
	Variant vMode = vConfig["mode"];
	m_fUseSimplePut = vConfig.get("useSimplePut", m_fUseSimplePut);
	m_sPutDelimiter = vConfig.get("putDelimiter", "\n");
	FileMode eMode = (vMode.isString() ? convertModeStr(vMode) : static_cast<FileMode>(vMode));

	if (testFlag(eMode, FileMode::CreatePath))
	{
		std::filesystem::path sPath(m_pathToFile);
		if (sPath.has_parent_path())
			std::filesystem::create_directory(sPath.parent_path());
	}

#ifdef _MSC_VER
	auto nMode = getMode(eMode);
	if (_wsopen_s(&m_nHandle, m_pathToFile.c_str(), nMode.first, nMode.second, _S_IREAD | _S_IWRITE))
		error::CrtError(SL, FMT("Can't create or open file <" << m_pathToFile.u8string() << ">")).throwException();
#else
#error Add implementations for other OS 
//	if (fopen_s(&pFile, m_pathFile.c_str(), sMode.data()))
//		CMD_CRT_THROW(error::IoException("Can't open file"));
#endif
	fOpen = true;
}

//
//
//
Size File::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	if (pBuffer == NULL)
		error::InvalidArgument(SL, "Invalid buffer ptr").throwException();
#ifdef _MSC_VER
	int nReadSize = _read(m_nHandle, pBuffer, static_cast<unsigned int>(nSize));
	if (nReadSize == -1)
		error::CrtError(SL, "File read error").throwException();
#else
#error Add other OS implementations.
#endif
	if (pProceedSize != nullptr)
		*pProceedSize = nReadSize;
	return nReadSize;
}

//
//
//
void File::write(const void* pBuffer, Size nSize)
{
	if (pBuffer == NULL)
		error::InvalidArgument(SL, "Invalid buffer ptr").throwException();
#ifdef _MSC_VER
	if (_write(m_nHandle, pBuffer, static_cast<unsigned int>(nSize)) == -1)
		error::CrtError(SL, "File write error").throwException();
#else
#error Add other OS implementations.
#endif
	return;
}

//
//
//
void File::setSize(IoSize nSize)
{
	flush();
#ifdef _MSC_VER
	if (_chsize_s(m_nHandle, nSize))
		error::CrtError(SL, "Can't set file size").throwException();
#else
#error Add other OS implementations.
//	ftruncate(_fileno(m_pFile.get()), nSize)
#endif
	setPosition(nSize);
}

//
//
//
void File::flush()
{
#ifdef _MSC_VER
	if (_commit(m_nHandle))
		error::CrtError(SL, "Can't flush file").throwException();
#else
#error Add other OS implementations.
#endif
}

//
//
//
IoSize File::getPosition()
{
#ifdef _MSC_VER
	IoSize nPos = _telli64(m_nHandle);
	if (nPos == -1L)
		error::CrtError(SL, "Can't get position").throwException();
#else
#error Add other OS implementations.
//	ftello(m_pFile.get());
#endif
	return nPos;
}

//
//
//
IoSize File::setPosition(IoOffset nOffset, StreamPos eType)
{
	// Throw if position outside the stream
	IoSize nOldPosition = getPosition();
	convertPosition(nOffset, eType, nOldPosition, getSize());

#ifdef _MSC_VER
	if (_lseeki64(m_nHandle, nOffset, getOrigin(eType)) == -1L)
		error::CrtError(SL, "Can't set position").throwException();
#else
#error Add other OS implementations.
//	fseeko(m_pFile.get(), nOffset, getOrigin(eType));
#endif
	 return nOldPosition;
}

//
//
//
IoSize File::getSize()
{
#ifdef _MSC_VER
	__int64 nFileSize = _filelengthi64(m_nHandle);
	if (nFileSize == -1L)
		error::CrtError(SL, "Can't set position").throwException();
#else
#error Add other OS implementations.
	// Use stat() function under Linux
#endif // _MSC_VER
	return static_cast<IoSize>(nFileSize);
}

//
//
//
bool File::checkAttribute(FileAttribute eFileAttr)
{
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA m_pFileInfo;
	if (!GetFileAttributesEx(m_pathToFile.c_str(), GetFileExInfoStandard, &m_pFileInfo))
		error::win::WinApiError(SL).throwException();
	switch (eFileAttr)
	{
		case cmd::io::FileAttribute::Readonly:
			return testFlag(m_pFileInfo.dwFileAttributes, DWORD(FILE_ATTRIBUTE_READONLY));
		case cmd::io::FileAttribute::Hidden:
			return testFlag(m_pFileInfo.dwFileAttributes, DWORD(FILE_ATTRIBUTE_HIDDEN));
		case cmd::io::FileAttribute::System:
			return testFlag(m_pFileInfo.dwFileAttributes, DWORD(FILE_ATTRIBUTE_SYSTEM));
		case cmd::io::FileAttribute::Directory:
			return testFlag(m_pFileInfo.dwFileAttributes, DWORD(FILE_ATTRIBUTE_DIRECTORY));
		case cmd::io::FileAttribute::Archive:
			return testFlag(m_pFileInfo.dwFileAttributes, DWORD(FILE_ATTRIBUTE_ARCHIVE));
		case cmd::io::FileAttribute::Device:
			return testFlag(m_pFileInfo.dwFileAttributes, DWORD(FILE_ATTRIBUTE_DEVICE));
		case cmd::io::FileAttribute::Normal:
			return testFlag(m_pFileInfo.dwFileAttributes, DWORD(FILE_ATTRIBUTE_NORMAL));
		case cmd::io::FileAttribute::Temporary:
			return testFlag(m_pFileInfo.dwFileAttributes, DWORD(FILE_ATTRIBUTE_TEMPORARY));
		default:
			error::InvalidArgument(SL, "Invalid attribute is").throwException();
	}
#else
#error Add other OS implementations.
	// Add *NIX code here
#endif
}

//
//
//
tm File::getTime(FileTime eFileTime)
{
	tm FileTime;
#ifdef _MSC_VER
	struct __stat64 Stat;
	if (_fstat64(m_nHandle, &Stat))
		error::CrtError(SL, "Can't set position").throwException();

	__time64_t tmpTime;
	switch (eFileTime)
	{
		case cmd::io::FileTime::Create:
			tmpTime = Stat.st_ctime;
			break;
		case cmd::io::FileTime::LastAccess:
			tmpTime = Stat.st_atime;
			break;
		case cmd::io::FileTime::LastWrite:
			tmpTime = Stat.st_mtime;
			break;
		default:
			error::InvalidArgument(SL, "Invalid time id").throwException();
	}
	if(_gmtime64_s(&FileTime, &tmpTime))
		error::CrtError(SL).throwException();
#else
#error Add other OS implementations.
	// Add *NIX code here
#endif
	return FileTime;
}

//
//
//
void File::close()
{
	if (!fOpen) return;
	if (_close(m_nHandle))
		error::CrtError(SL, "Can't close file").throwException();
	fOpen = false;
}

//
//
//
void File::put(const Variant& vData)
{
	std::scoped_lock lock(m_mtxPutVariant);
	auto pStream = queryInterface<IRawWritableStream>(getPtrFromThis());
	if (m_fUseSimplePut)
		io::write(pStream, vData.print());
	else
		variant::serializeToJson(pStream, vData);
	io::write(pStream, m_sPutDelimiter);
}


///
/// #### Supported commands
///
Variant File::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant File::execute()
	///
	/// ##### write ()
	/// Writes data string into the file at current position.
	///   * data [str] - data string for writing.
	///
	if (vCommand == "write")
	{
		std::string sData = vParams["data"];
		write(sData.data(), sData.size());
		if (vParams.get("eol", false))
			write("\n", 1);
		return {};
	}
	
	///
	/// @fn Variant File::execute()
	///
	/// ##### write ()
	/// Puts the data into the file.
	///   * data [str] - data for writing.
	///
	if (vCommand == "put")
	{
		Variant vData = vParams["data"];
		put(vData);
		return {};
	}

	///
	/// @fn Variant File::execute()
	///
	/// ##### deserialize()
	/// Deserializes the file stream.
	/// Returns deserialized data.
	///
	if (vCommand == "deserialize")
	{
		return variant::deserializeFromJson(queryInterface<IRawReadableStream>(getPtrFromThis()));
	}

	TRACE_END(FMT("Error during execution of a command <" << vCommand << ">"));
	error::InvalidArgument(SL, FMT("FileStream doesn't support a command <"
		<< vCommand << ">")).throwException();
}

} // namespace io
} // namespace cmd 

/// @}
