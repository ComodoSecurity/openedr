//
// edrav2.libcore
// 
// Author: Yury Ermakov (05.06.2019)
// Reviewer:
//
///
/// @file Cache objects implementation
/// @addtogroup cache
/// @{
///
#include "pch.h"
#include "cache.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "cache"

namespace openEdr {

//
//
//
void StringCache::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	m_nMaxSize = vConfig.get("maxSize", m_nMaxSize);
	TRACE_END("Error during configuration");
}

//
//
//
void StringCache::put(const Variant& vData)
{
	std::scoped_lock sync(m_mtxData);
	if (m_data.size() >= m_nMaxSize)
	{
		m_data.clear();
		LOGLVL(Trace, "Maximum cache capacity <" << m_nMaxSize << "> is reached. Cache is cleared");
	}
	m_data.insert(vData);
}

//
//
//
bool StringCache::contains(const std::string& sValue)
{
	std::scoped_lock sync(m_mtxData);
	return (m_data.find(sValue) != m_data.end());
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * '???'
///
/// #### Supported commands
///
Variant StringCache::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Trace, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant Queue::execute()
	///
	/// ##### put()
	/// Puts data into the cache
	///   * data [var] - a variant with a string value to be placed into the cache
	///
	if (vCommand == "put")
	{
		put(vParams["data"]);
		return {};
	}

	///
	/// @fn Variant Queue::execute()
	///
	/// ##### contains()
	/// Checks if the cache contains element with specific key
	/// 
	else if (vCommand == "contains")
	{
		return contains(vParams["data"]);
	}

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
	error::OperationNotSupported(SL, FMT("Unsupported command <" << vCommand << ">")).throwException();
}

} // namespace openEdr 
/// @}
