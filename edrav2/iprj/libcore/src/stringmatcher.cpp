//
// edrav2.libcore project
//
// Author: Yury Ermakov (27.02.2019)
// Reviewer: Denis Bogdanov (06.03.2019)
//
///
/// @file StringMatcher object implementation
///
/// @addtogroup basicDataObjects Basic data objects
/// @{

#include "pch.h"
#include "stringmatcher.h"

namespace openEdr {
namespace string {

using namespace variant::operation;

//
//
//
void StringMatcher::finalConstruct(Variant vConfig)
{
	if (!vConfig.has("schema"))
		error::InvalidArgument(SL, "Missing field <schema>").throwException();

	Variant vSchema = vConfig["schema"];
	if (vSchema.isDictionaryLike())
		addRule(vSchema);
	else if (vSchema.isSequenceLike())
		for (const auto& vRule : vSchema)
			addRule(vRule);
	else
		error::InvalidArgument(SL, "Schema must be a sequence or dictionary").throwException();

	std::sort(m_vecPlainRules.begin(), m_vecPlainRules.end(),
		[](const Rule& r1, const Rule& r2)
		{
			if (r1.sTarget->hasWChar() && r2.sTarget->hasWChar())
				return (std::wstring_view(*r1.sTarget).size() > std::wstring_view(*r2.sTarget).size());
			return (std::string_view(*r1.sTarget).size() > std::string_view(*r2.sTarget).size());
		});

	for (const auto& srcRule : m_vecPlainRules)
		m_vecPlainRulesCI.emplace_back(convertToLower(srcRule.sTarget),
			convertToLower(srcRule.sTarget), srcRule.mode);
}

//
//
//
MatchMode convertMode(std::string_view sMode)
{
	if (sMode == "begin") return MatchMode::Begin;
	if (sMode == "end") return MatchMode::End;
	if (sMode == "all") return MatchMode::All;
	error::InvalidArgument(SL, "Invalid matching mode").throwException();
}

//
//
//
void StringMatcher::addRule(const Variant& vRule)
{
	if (!vRule.isDictionaryLike())
		error::InvalidArgument(SL, "Rule must be a dictionary").throwException();
	if (!vRule.has("target") || !vRule["target"].isString())
		error::InvalidArgument(SL, "Invalid/missing field <target> in the rule").throwException();

	MatchMode mode = MatchMode::All;
	if (vRule.has("mode"))
	{
		if (vRule["mode"].getType() == variant::ValueType::String)
			mode = convertMode(vRule["mode"]);
		else if (vRule["mode"].getType() == variant::ValueType::Integer)
			mode = (Variant::IntValue)vRule["mode"];
		else
			error::InvalidArgument(SL, "Mode must be a string or integer").throwException();
	}

	m_vecPlainRules.emplace_back(vRule["target"], vRule.get("value", ""), mode);
}

//
//
//
Variant StringMatcher::convertToLower(Variant::StringValuePtr pStrValue) const
{
	if (pStrValue->hasWChar())
		return string::convertToLower((std::wstring_view)*pStrValue);
	return string::convertToLower((std::string_view)*pStrValue);
}

//
//
//
bool StringMatcher::match(std::string_view sValue, bool fCaseInsensitive) const
{
	if (fCaseInsensitive)
	{
		std::string sLower = string::convertToLower(sValue);
		return matchPlain<char>(sLower, m_vecPlainRulesCI);
	}
	return matchPlain<char>(sValue, m_vecPlainRules);
}

//
//
//
bool StringMatcher::match(std::wstring_view sValue, bool fCaseInsensitive) const
{
	if (fCaseInsensitive)
	{
		std::wstring sLower = string::convertToLower(sValue);
		return matchPlain<wchar_t>(sLower, m_vecPlainRulesCI);
	}
	return matchPlain<wchar_t>(sValue, m_vecPlainRules);
}

//
//
//
template <typename CharT>
bool StringMatcher::matchPlain(std::basic_string_view<CharT> sValue, const std::vector<Rule>& vecRules) const
{
	for (const auto& vRule : vecRules)
	{	
		std::basic_string_view<CharT> sTarget(*vRule.sTarget);
		if (sTarget.size() > sValue.size())
			continue;
		if (testFlag(vRule.mode, MatchMode::Begin) &&
			!sValue.compare(0, sTarget.size(), sTarget))
			return true;
		if (testFlag(vRule.mode, MatchMode::End) &&
			!sValue.compare(sValue.size() - sTarget.size(), sTarget.size(), sTarget))
			return true;
		if (testFlag(vRule.mode, MatchMode::All) && (sValue.find(sTarget) != sValue.npos))
			return true;
	}
	return false;
}

//
//
//
std::string StringMatcher::replace(std::string_view sValue) const
{
	return replacePlain<char>(sValue);
}

//
//
//
std::wstring StringMatcher::replace(std::wstring_view sValue) const
{
	return replacePlain<wchar_t>(sValue);
}

//
//
//
template <typename CharT>
std::basic_string<CharT> StringMatcher::replacePlain(std::basic_string_view<CharT> sValue) const
{
	std::basic_string<CharT> sResult(sValue);
	Size nBeginCut = 0;
	Size nEndCut = 0;
	for (const auto& vRule : m_vecPlainRules)
	{
		std::basic_string_view<CharT> sTarget(*vRule.sTarget);
		if (sTarget.size() > sResult.size())
			continue;
		std::basic_string_view<CharT> sNewValue(*vRule.sValue);

		// String starts with
		if ((nBeginCut == 0) && testFlag(vRule.mode, MatchMode::Begin) &&
			// Ensure that the target string doesn't overlap the result string's affected ending
			(sTarget.size() < sResult.size() - nEndCut) &&
			!sResult.compare(0, sTarget.size(), sTarget))
		{
			sResult.replace(0, sTarget.size(), sNewValue);
			nBeginCut = sNewValue.size();
		}
		
		// String ends with
		if ((nEndCut == 0) && testFlag(vRule.mode, MatchMode::End) &&
			// Ensure that the target string doesn't overlap the result string's affected beginning
			(sTarget.size() < sResult.size() - nBeginCut) &&
			!sResult.compare(sResult.size() - sTarget.size(), sTarget.size(), sTarget))
		{
			sResult.replace(sResult.size() - sTarget.size(), sTarget.size(), sNewValue);
			nEndCut = sNewValue.size();
		}

		// All over the string
		if (testFlag(vRule.mode, MatchMode::All))
		{
			Size nPos = sResult.find(sTarget.data(), nBeginCut);
			while (nPos != sResult.npos)
			{
				// Ensure that the target string doesn't overlap the result string's affected ending
				if (nPos + sTarget.size() > sResult.size() - nEndCut)
					break;
				sResult.replace(nPos, sTarget.size(), sNewValue);
				nPos = sResult.find(sTarget.data(), nPos + sNewValue.size());
			}
		}
	}
	return sResult;
}

//
//
//
Variant StringMatcher::execute(Variant vCommand, Variant vParams /*= Variant()*/)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	// TODO: call command updateScenarios on updating

	///
	/// @fn Variant ScenarioManager::execute()
	///
	/// ##### match()
	/// Match string according to schema
	///
	if (vCommand == "match")
	{
		return match(std::string_view(vParams["data"]), vParams.get("caseInsensitive", false));
	}

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));

	error::OperationNotSupported(SL,
		FMT("ScenarioManager doesn't support command <" << vCommand << ">")).throwException();
}

} // namespace string
} // namespace openEdr

// @}
