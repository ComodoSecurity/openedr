// 
// EDRAv2.libcore project
//
// Author: Yury Ermakov (22.12.2018)
// Reviewer: Denis Bogdanov (29.12.2018)
//
///
/// @file Logging sinks declarations
///
#pragma once
#include <objects.h>

namespace openEdr {
namespace logging {

//
//
//
class ILog4CPlusAppender : OBJECT_INTERFACE
{
public:
	virtual void fillProperties(const std::string& sAppenderName, 
		log4cplus::helpers::Properties& properties) = 0;
};

//
// Base logging sink class for log4plus appenders
//
class Log4CPlusSink : public ObjectBase<CLSID_Log4CPlusSink>,
	public ILog4CPlusAppender
{
private:
	Dictionary m_config;

	//
	// Fills object's properties in log4cplus format (recursively)
	//
	void fillProperties(const std::string& sPrefix, Dictionary config, 
		log4cplus::helpers::Properties& properties);

public:
	//
	// Object initialization
	//
	virtual void finalConstruct(Variant vConfig);

	// ILog4CPlusAppender interface

	//
	// Gets object's properties in log4cplus format
	//
	void fillProperties(const std::string& sAppenderName, 
		log4cplus::helpers::Properties& properties) override;
};

} // namespace logging
} // namespace openEdr /// @}