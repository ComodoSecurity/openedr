//
// EDRAv2.libcore
// 
// Author: Vasily Plotnikov (20.10.2020)
// Reviewer: ???
//
///
/// @file LocalEventsLogFile class implementation
///
/// @weakgroup io IO subsystem
/// @{
#include "pch.h"
#include "localeventslogfile.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "locevents"

namespace openEdr {
namespace io {

//
//
//
void LocalEventsLogFile::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	if (!vConfig.isDictionaryLike())
		error::InvalidArgument(SL, "finalConstruct() supports only dictionary as a parameter")
		.throwException();

	if (!vConfig.has("path") || vConfig["path"].getType() != variant::ValueType::String)
		error::InvalidArgument(SL, "Parameter <path> isn't found or invalid").throwException();

	m_nDaysStore = vConfig.get("daysStore", m_nDaysStore);
	m_sLogPath = std::filesystem::path(vConfig["path"]);
	m_bMultiline = vConfig.get("multiline", m_bMultiline);

	std::string sMode = vConfig.get("mode", "");

	if (sMode.find("create") != std::string::npos &&
		!std::filesystem::exists(m_sLogPath))
		std::filesystem::create_directory(m_sLogPath);

	if (!std::filesystem::exists(m_sLogPath) ||
		!std::filesystem::is_directory(m_sLogPath))
		error::NotFound(SL, FMT("Data directory <" << m_sLogPath
			<< "> doesn't exist or invalid")).throwException();

	checkFullPathValid();
	updateFullPath();

	TRACE_END("Error during configuration");
}

//
//
//
bool LocalEventsLogFile::save(const Variant& vData)
{
	if (!checkFullPathValid())
	{
		updateFullPath();
		removeOutdated();
	}

	if (m_pWriteStream)
	{
		variant::serializeToJson(m_pWriteStream, vData, m_bMultiline ? variant::JsonFormat::Pretty : variant::JsonFormat::SingleLine);
		io::write(m_pWriteStream, "\r\n");
		return true;
	}

	return false;
}

void LocalEventsLogFile::updateFullPath()
{
	std::filesystem::path pathToFile(m_sLogPath);
	auto filename = getFilename();

	if (!pathToFile.empty())
	{
		m_sLogFullPath = (pathToFile / filename).string();

		m_pWriteStream = queryInterface<io::IWritableStream>(io::createFileStream(m_sLogFullPath,
			io::FileMode::Write | io::FileMode::ShareRead | io::FileMode::CreatePath | io::FileMode::Truncate));
	}
}

bool LocalEventsLogFile::checkFullPathValid()
{
	using namespace std::chrono;

	auto currentTime = steady_clock::now();
	if(duration_cast<minutes>(currentTime - m_lastCheckTime).count() > 5)
	{
		m_lastCheckTime = currentTime;
		auto tnow =system_clock::to_time_t(system_clock::now());
		tm tmData;
		gmtime_s(&tmData, &tnow);
		if (m_currentMonthDay != tmData.tm_mday)
		{
			m_currentMonthDay = tmData.tm_mday;
			return false;
		}
	}
	
	return true;
}

std::string LocalEventsLogFile::getFilename()
{
	//name format: %Y-%m-%d.log
	using namespace std::chrono;
	auto now = system_clock::now();
	auto in_time_t = system_clock::to_time_t(now);

	tm tmTime;
	gmtime_s(&tmTime, &in_time_t);

	std::stringstream ss;
	ss << std::put_time(&tmTime, "%Y-%m-%d") << ".log";
	return ss.str();
}

void LocalEventsLogFile::removeOutdated()
{
	using namespace std::filesystem;
	using namespace std::chrono;
	auto now = file_time_type::clock::now();
	auto maxdurationhrs = 24 * m_nDaysStore;

	auto isOutdated = [now, maxdurationhrs](const directory_entry& p) -> bool
	{
		auto last_write_time = p.last_write_time();
		if (duration_cast<hours>(now - last_write_time).count() > maxdurationhrs )
			return true;
		return false;
	};

	for (auto& sub_path : directory_iterator(m_sLogPath))
	{
		if (sub_path.is_directory())
			continue;
		auto ext = string::convertToLower(sub_path.path().extension().c_str());
		if (ext.compare(L".log") != 0)
			continue;

		if (isOutdated(sub_path))
			remove(sub_path);
	}
}

//
//
//
Variant LocalEventsLogFile::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	if (vCommand == "put")
	{
		if (!vParams.has("data"))
			error::InvalidArgument(SL, "Missing field <data> in parameters").throwException();
		if (!vParams.has("default"))
			return save(vParams["data"]);

		CMD_TRY
		{
			return save(vParams["data"]);
		}
		CMD_PREPARE_CATCH
		catch (const error::Exception& ex)
		{
			LOGLVL(Debug, ex.what());
			return vParams["default"];
		}
	}

	error::OperationNotSupported(SL, FMT("Unsupported command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during execution of a command <" << vCommand << ">"));
}

} // namespace io
} // namespace openEdr

/// @}