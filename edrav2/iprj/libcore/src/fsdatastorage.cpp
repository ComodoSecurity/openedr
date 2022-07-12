//
// edrav2.libcore
// 
// Author: Denis Bogdanov (10.03.2019)
// Reviewer: Yury Podpruzhnikov (13.03.2019)
//
///
/// @file File system data storage implementation
///
/// @addtogroup io IO subsystem
/// @{
#include "pch.h"
#include "fsdatastorage.h"
#include "io.hpp"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "fsdtastg"

namespace cmd {
namespace io {

///
/// Initializes object with following parameters:
///   * path [int, opt] - path to data (absolute or relating to the current directory)
///   * maxSize [int, opt] - the maximal size of stored objects (works only for put());
///   * deleteOnGet [bool, opt] - if it is true then the files are deleted on get() request.
///   * mode [string, opt] - creation mode ("create", "truncate" or their combination);
///   * prettyOutput [bool, opt] - use pretty json output.
///
void FsDataStorage::finalConstruct(Variant vConfig)
{
	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "finalConstruct() supports only dictionary as a parameter")
			.throwException();

	if (!vConfig.has("path") || vConfig["path"].getType() != variant::ValueType::String)
		error::InvalidArgument(SL, "Parameter <path> isn't found or invalid").throwException();

	m_nMaxSize = vConfig.get("maxSize", m_nMaxSize);
	m_fDeleteOnGet = vConfig.get("deleteOnGet", m_fDeleteOnGet);
	m_pathFileData = std::filesystem::path(vConfig["path"]);
	m_fPrettyOutput = vConfig.get("prettyOutput", m_fPrettyOutput);
	
	std::string sMode = vConfig.get("mode", "");

	if (sMode.find("create") != std::string::npos &&
		!std::filesystem::exists(m_pathFileData))
		std::filesystem::create_directory(m_pathFileData);

	if (sMode.find("truncate") != std::string::npos)
		erase();

	if (!std::filesystem::exists(m_pathFileData) ||
		!std::filesystem::is_directory(m_pathFileData))
		error::NotFound(SL, FMT("Data directory <" << m_pathFileData
			<< "> doesn't exist or invalid")).throwException();

	for (auto& item : std::filesystem::directory_iterator(m_pathFileData))
		if (item.is_regular_file() && item.file_size() > 2)
			m_fileNames.push_back(item.path().filename());
	if (!m_fileNames.empty())
	{
		std::sort(m_fileNames.begin(), m_fileNames.end());
		try 
		{ 
			m_nPutIndex = std::stoul(m_fileNames.back(), nullptr, 16); 
			++m_nPutIndex;
		} 
		catch (...) {}
	}
}

//
//
//
void FsDataStorage::erase()
{
	if (!std::filesystem::exists(m_pathFileData) ||
		!std::filesystem::is_directory(m_pathFileData)) return;
	for (auto& item : std::filesystem::directory_iterator(m_pathFileData))
		if (item.path().extension() == ".dict")
			std::filesystem::remove(item.path());
}

//
//
//
std::optional<Variant> FsDataStorage::get()
{
	std::scoped_lock lock(m_mtxData);

	while (!m_fileNames.empty())
	{
		auto fullPath = m_pathFileData / m_fileNames.front();
		m_fileNames.pop_front();
		if (!std::filesystem::exists(fullPath)) continue;
		LOGLVL(Debug, "Get data from file <" << fullPath.filename() << ">");

		CMD_TRY
		{
			auto vRes = variant::deserializeFromJson(createFileStream(fullPath,
				io::FileMode::Read | io::FileMode::ShareRead));
			if (m_fDeleteOnGet)
				std::filesystem::remove(fullPath);
			return vRes;
		}
		CMD_PREPARE_CATCH
		catch (error::Exception& e)
		{
			e.log();
		}
	}

	return std::nullopt;
}

//
//
//
void FsDataStorage::put(const Variant& vData)
{
	std::scoped_lock sync(m_mtxData);
	if (m_fileNames.size() >= m_nMaxSize)
		error::LimitExceeded(SL, FMT("Size limit is exceeded <" << m_nMaxSize << ">")).throwException();
	auto sName = string::convertToHexW(m_nPutIndex);
	sName += L".dict";
	auto fullPath = m_pathFileData / sName;
	auto pStream = queryInterface<IRawWritableStream>(createFileStream(fullPath,
		io::FileMode::Write | io::FileMode::Create | io::FileMode::CreatePath));
	variant::serialize(pStream, vData, Dictionary({
		{"format", m_fPrettyOutput ? "jsonPretty" : "json"}
	}));
	LOGLVL(Debug, "Put data into file <" << string::convertWCharToUtf8(sName) << ">");
	m_fileNames.push_back(sName);
	++m_nPutIndex;
}

//
//
//
Variant FsDataStorage::getInfo()
{
	std::scoped_lock sync(m_mtxData);
	return Dictionary({
			{"size", m_fileNames.size()},
			{"maxSize", m_nMaxSize}
		});
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * '???'
///
/// #### Supported commands
///
Variant FsDataStorage::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant DirectoryDataProvider::execute()
	///
	/// ##### get()
	/// Gets the data.
	///
	if (vCommand == "get")
	{
		auto v = get();
		return v.has_value() ? v.value() : Variant();
	}

	///
	/// @fn Variant FsDataStorage::execute()
	///
	/// ##### put()
	/// Puts the data to the queue
	///   * data [var] - data.
	///
	if (vCommand == "put")
	{
		put(vParams["data"]);
		return {};
	}

	///
	/// @fn Variant FsDataStorage::execute()
	///
	/// ##### getInfo()
	/// Gets the provider information.
	/// 
	else if (vCommand == "getInfo")
	{
		return getInfo();
	}

	error::OperationNotSupported(SL, 
		FMT("FsDataStorage doesn't support command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // namespace io 
} // namespace cmd 
/// @}
