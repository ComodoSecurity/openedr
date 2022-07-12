//
// edrav2.libedr project
// 
// Author: Yury Ermakov (13.11.2019)
// Reviewer:
//
///
/// @file Output filter implementation
///
/// @addtogroup edr
/// @{
#include "pch.h"
#include "outputfilter.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "outflt"

namespace cmd {

//
//
//
OutputFilter::OutputFilter()
{
}

//
//
//
OutputFilter::~OutputFilter()
{
}

//
//
//
void OutputFilter::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	if (!vConfig.has("events"))
		error::InvalidArgument(SL, "Missing field <events>").throwException();

	auto vEvents = vConfig["events"];
	if (!vEvents.isDictionaryLike())
		error::InvalidArgument(SL, "Field <events> must be a dictionary").throwException();

	for (const auto& [sKey, vValue] : Dictionary(vEvents))
		addRegex(sKey, parseEvent(vValue, vEvents));
	
	TRACE_END("Error during object creation");
}

//
//
//
OutputFilter::RegexPtr OutputFilter::parseEvent(Variant vData, const Variant& vEvents, LinkedGroups linkedGroups)
{
	if (vData.isSequenceLike())
	{
		std::string sRegex;
		for (const auto& sItem : vData)
		{
			sRegex += sItem;
			sRegex += '|';
		}
		// Cut-off the last '|'
		if (!sRegex.empty())
			sRegex.resize(sRegex.size() - 1);
		return std::make_shared<std::regex>(sRegex, std::regex::extended);
	}

	if (!vData.isString())
		error::InvalidArgument(SL, "Value must be sequence or string").throwException();

	std::string sGroup = vData;
	auto pRegex = getRegex(sGroup);
	if (pRegex)
		return pRegex;

	if (!vEvents.has(sGroup))
		error::InvalidArgument(SL, FMT("Unknown event group <" << sGroup << ">")).throwException();
	
	if (linkedGroups.find(sGroup) != linkedGroups.end())
		error::InvalidArgument(SL, FMT("Looping group <" << sGroup << ">")).throwException();

	linkedGroups.emplace(sGroup);
	pRegex = parseEvent(vEvents[sGroup], vEvents, linkedGroups);
	addRegex(sGroup, pRegex);
	return pRegex;
}

//
//
//
void OutputFilter::addRegex(std::string_view sType, RegexPtr pRegex)
{
	m_regexMap.insert_or_assign(std::string(sType), pRegex);
}

//
//
//
OutputFilter::RegexPtr OutputFilter::getRegex(const std::string& sType) const
{
	auto it = m_regexMap.find(sType);
	if (it != m_regexMap.end())
		return it->second;
	return nullptr;
}

//
//
//
void fillDictionaryMap(std::vector<std::string>& map, Variant vData, std::string_view sPath = "")
{
	for (const auto& [sKey, vValue] : Dictionary(vData))
	{
		std::string sFullPath(sPath);
		if (!sFullPath.empty())
			sFullPath += '.';
		sFullPath += sKey;

		if (vValue.isDictionaryLike())
		{
			fillDictionaryMap(map, vValue, sFullPath);
			continue;
		}

		map.push_back(sFullPath);
	}
}

//
//
//
std::vector<std::string> getDictionaryMap(Variant vData)
{
	std::vector<std::string> map;
	if (vData.isDictionaryLike())
		fillDictionaryMap(map, vData);
	return map;
}

//
//
//
void fillDictionaryMap(std::string& sMap, Variant vData, std::string_view sPath = "")
{
	for (const auto& [sKey, vValue] : Dictionary(vData))
	{
		std::string sFullPath(sPath);
		if (!sFullPath.empty())
			sFullPath += '.';
		sFullPath += sKey;

		if (vValue.isDictionaryLike())
		{
			fillDictionaryMap(sMap, vValue, sFullPath);
			continue;
		}

		sMap += sFullPath;
		sMap += '\n';
	}
}

//
//
//
std::string getDictionaryMapString(Variant vData)
{
	std::string sMap;
	if (vData.isDictionaryLike())
		fillDictionaryMap(sMap, vData);
	return sMap;
}

//
//
//
Variant OutputFilter::filter(const std::string& sType, Variant vData) const
{
	if (!vData.isDictionaryLike())
		error::InvalidArgument(SL, "Data must be a dictionary").throwException();

	auto pRegex = getRegex(sType);
	if (!pRegex)
	{
		pRegex = getRegex("default");
		if (!pRegex)
			return Dictionary();
	}

	Variant vResult = Dictionary();
	std::string sPaths = getDictionaryMapString(vData);
	for (auto it = std::sregex_iterator(sPaths.begin(), sPaths.end(), *pRegex); it != std::sregex_iterator();
		++it)
	{
		auto sPath = it->str();
		variant::putByPath(vResult, sPath, variant::getByPath(vData, sPath), true /* fCreatePaths */);
	}
	return vResult;
}

///
/// #### Supported commands
///
Variant OutputFilter::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command's parameters:\n" << vParams);

	///
	/// @fn Variant OutputFilter::execute()
	///
	/// ##### filter()
	/// Filters data using the schema according to the given event type.
	///   * type [str] - event type.
	///   * data [Variant] - raw data to be filtered.
	/// Returns a filtered data.
	///
	if (vCommand == "filter")
	{
		if (!vParams.has("type"))
			error::InvalidArgument(SL, "Missing field <type>").throwException();
		if (!vParams.has("data"))
			error::InvalidArgument(SL, "Missing field <data>").throwException();
		return filter(vParams["type"], vParams["data"]);
	}

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
	error::OperationNotSupported(SL, FMT("Unsupported command <" << vCommand << ">")).throwException();
}

} // namespace cmd

/// @}
