// 
// EDRAv2.libcore project
//
// Author: Yury Ermakov (22.12.2018)
// Reviewer: Denis Bogdanov (14.01.2019)
//
///
/// @file Logging sinks implementation
///
#include "pch.h"
#include "logsink.h"

namespace openEdr {
namespace logging {

//
//
//
void Log4CPlusSink::finalConstruct(Variant vConfig)
{
	m_config = Dictionary(vConfig);
}

//
//
//
void Log4CPlusSink::fillProperties(const std::string& sAppenderName,
	log4cplus::helpers::Properties& properties)
{
	fillProperties("log4cplus.appender." + sAppenderName, m_config, properties);
}

//
//
//
void Log4CPlusSink::fillProperties(const std::string& sPrefix, Dictionary config,
	log4cplus::helpers::Properties& properties)
{
	TRACE_BEGIN;

	for (auto param : config)
	{
		if (param.second.isDictionaryLike())
			fillProperties(sPrefix + "." + std::string(param.first),
				param.second, properties);
		else
		{
			switch (param.second.getType())
			{
				case Variant::ValueType::String:
				{
					std::string sKey(param.first);
					std::string sValue(param.second);
					// Convert relative path to file to absolute
					if (sKey == "File")
					{
						std::filesystem::path pathToFile(getCatalogData("app.logPath", std::string()));
						if (!pathToFile.empty())
							sValue = (pathToFile / sValue).string();
					}
					properties.setProperty(sPrefix + (sKey == "factory" ? "" : "." + sKey), sValue);
					break;
				}
				case Variant::ValueType::Boolean:
				{
					properties.setProperty(sPrefix + "." + std::string(param.first),
						((bool)param.second ? "true" : "false"));
					break;
				}
				case Variant::ValueType::Integer:
				{
					properties.setProperty(sPrefix + "." + std::string(param.first),
						std::to_string((int)param.second));
					break;
				}
				default:
				{
					// Skip values of unsupported types
				}
			}
		}
	}

	TRACE_END(FMT("Error during filling properties for <" << sPrefix << ">"));
}

} // namespace logging
} // namespace openEdr 
