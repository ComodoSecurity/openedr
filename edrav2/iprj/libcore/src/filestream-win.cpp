//
// edrav2::libcore
// File stream class implementation
// 
// Author: Denis Kroshin (01.04.2019)
// Reviewer: Denis Bogdanov
//
///
/// @file File stream class implementation
///
/// @weakgroup io IO subsystem
/// @{
#include "pch.h"
#include "filestream-win.h"
#include "stream.h"

namespace cmd {
namespace io {
namespace win {

//
//
//
ObjPtr<cmd::io::IReadableStream> createFileStream(const std::filesystem::path& pathFile, const FileMode eFlag)
{
	TRACE_BEGIN;
	auto pObj = createObject(CLSID_FileWin,
		Dictionary({ {"path", pathFile.c_str()}, {"mode", eFlag} }));
	return queryInterface<IReadableStream>(pObj);
	TRACE_END(std::string("Can't process file <") + pathFile.u8string() + ">");
}

struct SharedHandle : public ObjectBase<>
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
};

//
//
//
ObjPtr<cmd::io::IReadableStream> createFileStream(const HANDLE hFile)
{
	TRACE_BEGIN;
	auto pHandle = createObject<SharedHandle>();
	pHandle->hFile = hFile;
	auto pObj = createObject(CLSID_FileWin, Dictionary({ {"handle", pHandle} }));
	return queryInterface<IReadableStream>(pObj);
	TRACE_END(std::string("Can't create file stream"));
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
		auto pObj = createObject(CLSID_FileWin,
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
	wchar_t sFileName[L_tmpnam_s];
	if (_wtmpnam_s(sFileName, L_tmpnam_s))
		error::CrtError(SL).throwException();
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

	auto pObj = createObject(CLSID_FileWin,
		Dictionary({ {"path", getTempFileName().c_str()},
			{"mode", FileMode::Read | FileMode::Write | FileMode::Create | FileMode::Temp} }));
	return queryInterface<IReadableStream>(pObj);
}


//
//
//
void File::finalConstruct(Variant vConfig)
{
	m_sPutDelimiter = vConfig.get("putDelimiter", "\n");

	if (vConfig.has("handle"))
	{
		HANDLE hDuplicate = INVALID_HANDLE_VALUE;
		auto hCurProcess = ::GetCurrentProcess();
		auto pHandle = queryInterface<SharedHandle>(vConfig["handle"]);
		if (!::DuplicateHandle(hCurProcess, pHandle->hFile, hCurProcess, &hDuplicate, 0, FALSE, DUPLICATE_SAME_ACCESS))
			error::win::WinApiError(SL, "Can't duplicate file handle").throwException();
		m_nHandle.reset(hDuplicate);
		return;
	}

	m_pathToFile = vConfig["path"];
	Variant vMode = vConfig["mode"];
	FileMode eMode = (vMode.isString() ? convertModeStr(vMode) : static_cast<FileMode>(vMode));

	if (testFlag(eMode, FileMode::CreatePath))
	{
		if (m_pathToFile.has_parent_path())
			std::filesystem::create_directory(m_pathToFile.parent_path());
	}

	uint32_t nAccessFlag = 0;
	uint32_t nDispositionFlag = 0;
	if (testFlag(eMode, FileMode::Read))
	{
		nAccessFlag |= GENERIC_READ;
		nDispositionFlag = OPEN_EXISTING;
	}
	if (testFlag(eMode, FileMode::Write))
	{
		nAccessFlag |= GENERIC_WRITE;
		nDispositionFlag = testFlag(eMode, FileMode::Truncate) ? CREATE_ALWAYS : OPEN_ALWAYS;
	}
	if (testFlag(eMode, FileMode::Attributes))
	{
		nAccessFlag |= FILE_READ_ATTRIBUTES;
		nDispositionFlag = OPEN_EXISTING;
	}
	if (nAccessFlag == 0)
		error::InvalidArgument(SL, "R/W mode is not specified").throwException();

	if (testFlag(eMode, FileMode::Create))
		if (testFlag(eMode, FileMode::Write))
			nDispositionFlag = CREATE_NEW;
		else
			error::InvalidArgument(SL, "Invalid mode <Create without Write>").throwException();

	if (testFlag(eMode, FileMode::Append))
		error::InvalidArgument(SL, "Mode <Append> not supported").throwException();

	uint32_t nShareFlag = 0;
	if (testFlag(eMode, FileMode::ShareRead))
		nShareFlag |= FILE_SHARE_READ;
	if (testFlag(eMode, FileMode::ShareWrite))
		nShareFlag |= (FILE_SHARE_WRITE | FILE_SHARE_DELETE);

	m_nHandle.reset(::CreateFileW(m_pathToFile.c_str(), nAccessFlag, nShareFlag, NULL, nDispositionFlag, 
		(testFlag(eMode, FileMode::Temp) ? FILE_ATTRIBUTE_TEMPORARY : FILE_ATTRIBUTE_NORMAL), NULL));
	if (!m_nHandle)
		error::win::WinApiError(SL, FMT("Can't create or open file <" << m_pathToFile.u8string() << ">")).throwException();
}

//
//
//
Size File::read(void* pBuffer, Size nSize, Size* pProceedSize)
{
	// TODO: Add handling of sizeof(size_t) > sizeof(DWORD)
	if (nSize > UINT32_MAX)
		error::OutOfRange(SL).throwException();
	DWORD nReadSize = 0;
	if(!::ReadFile(m_nHandle, pBuffer, DWORD(nSize), &nReadSize, NULL))
		error::win::WinApiError(SL, "File read error").throwException();
	if (pProceedSize != nullptr)
		*pProceedSize = Size(nReadSize);
	return nReadSize;
}

//
//
//
void File::write(const void* pBuffer, Size nSize)
{
	// TODO: Add handling of sizeof(size_t) > sizeof(DWORD)
	if (nSize > UINT32_MAX)
		error::OutOfRange(SL).throwException();
	DWORD nWriteSize = 0;
	if (!::WriteFile(m_nHandle, pBuffer, DWORD(nSize), &nWriteSize, NULL))
		error::win::WinApiError(SL, "File write error").throwException();
	return;
}

//
//
//
void File::setSize(IoSize nSize)
{
	flush();
	LARGE_INTEGER nOldPos = {};
	LARGE_INTEGER nNewPos = {};
	nNewPos.QuadPart = nSize;
	if (!::SetFilePointerEx(m_nHandle, nNewPos, &nOldPos, FILE_BEGIN))
		error::win::WinApiError(SL, "Can't set file position").throwException();
	if (!::SetEndOfFile(m_nHandle))
		error::win::WinApiError(SL, "Can't set file size").throwException();
	if (!::SetFilePointerEx(m_nHandle, nOldPos, NULL, FILE_BEGIN))
		error::win::WinApiError(SL, "Can't set file position").throwException();
}

//
//
//
void File::flush()
{
	if (!::FlushFileBuffers(m_nHandle))
		error::win::WinApiError(SL, "Can't flush file").throwException();
}

//
//
//
IoSize File::getPosition()
{
	LARGE_INTEGER nFakePos = { {0, 0} };
	LARGE_INTEGER nPos = {};
	if (!::SetFilePointerEx(m_nHandle, nFakePos, &nPos, FILE_CURRENT))
		error::win::WinApiError(SL, "Can't get file position").throwException();
	return nPos.QuadPart;
}

//
//
//
IoSize File::setPosition(IoOffset nOffset, StreamPos eType)
{
	// Throw if position outside the stream
	LARGE_INTEGER nOldPos;
	nOldPos.QuadPart = getPosition();
	LARGE_INTEGER nNewPos;
	nNewPos.QuadPart = convertPosition(nOffset, eType, nOldPos.QuadPart, getSize());
	if (!::SetFilePointerEx(m_nHandle, nNewPos, &nOldPos, FILE_BEGIN))
		error::win::WinApiError(SL, "Can't set file position").throwException();
	return nOldPos.QuadPart;
}

//
//
//
IoSize File::getSize()
{
	LARGE_INTEGER nFileSize = {};
	if (!::GetFileSizeEx(m_nHandle, &nFileSize))
		error::win::WinApiError(SL, "Can't set file position").throwException();
	return static_cast<IoSize>(nFileSize.QuadPart);
}

//
//
//
bool File::checkAttribute(FileAttribute eFileAttr)
{
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
}

//
//
//
tm File::getTime(FileTime eFileTime)
{
	FILETIME nCreationTime = {};
	FILETIME nAccessTime = {};
	FILETIME nWriteTime = {};
	if (!::GetFileTime(m_nHandle, &nCreationTime, &nAccessTime, &nWriteTime))
		error::win::WinApiError(SL, "Can't get file time").throwException();

	time_t nTime;
	switch (eFileTime)
	{
	case cmd::io::FileTime::Create:
		nTime = fileTimeToTimeT(nCreationTime);
		break;
	case cmd::io::FileTime::LastAccess:
		nTime = fileTimeToTimeT(nAccessTime);
		break;
	case cmd::io::FileTime::LastWrite:
		nTime = fileTimeToTimeT(nWriteTime);
		break;
	default:
		error::InvalidArgument(SL, "Invalid time id").throwException();
	}

	tm FileTime;
	if(_gmtime64_s(&FileTime, &nTime))
		error::CrtError(SL).throwException();
	return FileTime;
}

//
//
//
void File::close()
{
	m_nHandle.reset();
}

//
//
//
void File::put(const Variant& vData)
{
	auto pStream = queryInterface<IRawWritableStream>(getPtrFromThis());
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

} // namespace win
} // namespace io
} // namespace cmd

/// @}
