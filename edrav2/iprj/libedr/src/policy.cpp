//
// edrav2.libedr project
// 
// Author: Yury Ermakov (20.08.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Policy internals implementation
///
/// @addtogroup edr
/// @{
#include "pch.h"
#include "policy.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "policy"

namespace openEdr {
namespace edr {
namespace policy {

constexpr char c_sConst[] = "const";
constexpr char c_sVar[] = "var";
constexpr char c_sEvent[] = "event";
constexpr char c_sContext[] = "context";
constexpr char c_sRule[] = "rule";
constexpr char c_sPolicy[] = "policy";

//
//
//
const char* getSectionString(Section section)
{
	switch (section)
	{
		case Section::Const: return c_sConst;
		case Section::Var: return c_sVar;
		case Section::Event: return c_sEvent;
		case Section::Context: return c_sContext;
		case Section::Rule: return c_sRule;
		case Section::Policy: return c_sPolicy;
	}
	error::InvalidArgument(SL, FMT("Section <" << +(std::uint8_t)section <<
		"> has no text string")).throwException();
}

//
//
//
Section getSectionFromString(std::string_view sSection)
{
	if (sSection == c_sConst) return Section::Const;
	if (sSection == c_sVar) return Section::Var;
	if (sSection == c_sEvent) return Section::Event;
	if (sSection == c_sContext) return Section::Context;
	if (sSection == c_sRule) return Section::Rule;
	if (sSection == c_sPolicy) return Section::Policy;
	error::InvalidArgument(SL, FMT("Unknown section <" << sSection << ">")).throwException();
}

constexpr char c_sPatternsMatching[] = "patternsMatching";
constexpr char c_sEventsMatching[] = "eventsMatching";
constexpr char c_sUndefined[] = "<undefined>";

constexpr char c_sPatternsMatchingShort[] = "ptm";
constexpr char c_sEventsMatchingShort[] = "evm";

//
//
//
const char* getPolicyGroupString(PolicyGroup group)
{
	switch (group)
	{
		case PolicyGroup::PatternsMatching: return c_sPatternsMatching;
		case PolicyGroup::EventsMatching: return c_sEventsMatching;
		case PolicyGroup::Undefined: return c_sUndefined;
	}
	error::InvalidArgument(SL, FMT("Policy group <" << +(std::uint8_t)group <<
		"> has no text string")).throwException();
}

//
//
//
const char* getPolicyGroupShortString(PolicyGroup group)
{
	switch (group)
	{
		case PolicyGroup::PatternsMatching: return c_sPatternsMatchingShort;
		case PolicyGroup::EventsMatching: return c_sEventsMatchingShort;
	}
	error::InvalidArgument(SL, FMT("Policy group <" << +(std::uint8_t)group <<
		"> has no short text string")).throwException();
}

//
//
//
PolicyGroup getPolicyGroupFromString(std::string_view sGroup)
{
	if (sGroup == c_sPatternsMatching) return PolicyGroup::PatternsMatching;
	if (sGroup == c_sEventsMatching) return PolicyGroup::EventsMatching;
	if (sGroup == c_sUndefined) return PolicyGroup::Undefined;
	error::InvalidArgument(SL, FMT("Unknown policy group <" << sGroup << ">")).throwException();
}


constexpr char c_sIn[] = "IN";
constexpr char c_sOut[] = "OUT";
constexpr char c_sInOut[] = "IN_OUT";

//
//
//
const char* getDestinationString(Destination destination)
{
	switch (destination)
	{
		case Destination::In: return c_sIn;
		case Destination::Out: return c_sOut;
		case Destination::InOut: return c_sInOut;
	}
	error::InvalidArgument(SL, FMT("Destination <" << +(std::uint8_t)destination <<
		"> has no text string")).throwException();
}

//
//
//
Destination getDestinationFromString(std::string_view sDestination)
{
	if (sDestination == c_sIn) return Destination::In;
	if (sDestination == c_sOut) return Destination::Out;
	if (sDestination == c_sInOut) return Destination::InOut;
	error::InvalidArgument(SL, FMT("Unknown destination <" << sDestination << ">")).throwException();
}

//
//
//
Context::Context(Variant vObjects, Variant vEventClasses, bool fVerbose_) :
	m_vObjects(vObjects), m_vEventClasses(vEventClasses), fVerbose(fVerbose_)
{
}

//
//
//
void Context::addPolicy(PolicyPtr pPolicy_)
{
	policies[pPolicy_->sName] = pPolicy_;
}

//
//
//
void Context::addChain(ChainPtr pChain)
{
	chains[pChain->sName] = pChain;
}

//
//
//
void Context::addPattern(PatternPtr pPattern)
{
	patterns[pPattern->sName] = pPattern;
}

//
//
//
void Context::addBinding(const std::string& sEventType, ChainPtr pChain)
{
//	m_bindings[eventType].insert(std::make_pair(m_pPolicy->m_nPriority, pChain));
	bindings.insert(std::make_pair(sEventType, pChain));
}

//
//
//
void Context::addVariable(VariablePtr pVariable)
{
	m_variables[pVariable->sName] = pVariable;
}

//
//
//
ChainPtr Context::getChain(std::string_view sName) const
{
	auto iter = chains.find(std::string(sName));
	if (iter == chains.end())
		return nullptr;
	return iter->second;
}

//
//
//
PolicyPtr Context::getPolicy(std::string_view sName) const
{
	auto iter = policies.find(std::string(sName));
	if (iter == policies.end())
		return nullptr;
	return iter->second;
}

//
//
//
VariablePtr Context::getVariable(std::string_view sName) const
{
	auto iter = m_variables.find(std::string(sName));
	if (iter == m_variables.end())
		return nullptr;
	return iter->second;
}

//
//
//
Variant Context::resolveValue(const Variant& vValue, bool fRuntime)
{
	if (isLink(vValue))
	{
		auto optValue = getValueByLink(vValue, fRuntime, true /* fRecursive */);
		if (optValue.has_value())
			return optValue.value();
	}
	if (vValue.isDictionaryLike())
	{
		Variant vData = vValue.clone();
		resolveValueDict(vData, "", fRuntime);
		return vData;
	}
	if (vValue.isSequenceLike())
	{
		Variant vData = vValue.clone();
		resolveValueSeq(vData, "", fRuntime);
		return vData;
	}
	return vValue;
}

//
//
//
void Context::resolveValueDict(Variant vData, const std::string& sPath, bool fRuntime)
{
	for (auto& [sKey, vValue] : Dictionary(vData))
	{
		std::string sFullPath(sPath);
		if (!sFullPath.empty())
			sFullPath += ".";
		sFullPath += sKey;

		if (vValue.isDictionaryLike())
			resolveValueDict(vValue, sFullPath, fRuntime);
		else if (vValue.isSequenceLike())
			resolveValueSeq(vValue, sFullPath, fRuntime);
		else if (isLink(vValue))
		{
			auto optValue = getValueByLink(vValue, fRuntime, true /* fRecursive */);
			if (optValue.has_value())
				vData.put(sKey, optValue.value());
				//variant::putByPath(vData, sKey, optValue.value(), true /* fCreatePaths */);
		}
	}
}

//
//
//
void Context::resolveValueSeq(Variant vData, const std::string& sPath, bool fRuntime)
{
	Size nIndex = 0;
	for (auto& vValue : vData)
	{
		std::string sFullPath(sPath);
		sFullPath += "[";
		sFullPath += std::to_string(nIndex);
		sFullPath += "]";

		if (vValue.isDictionaryLike())
			resolveValueDict(vValue, sFullPath, fRuntime);
		else if (vValue.isSequenceLike())
			resolveValueSeq(vValue, sFullPath, fRuntime);
		else if (isLink(vValue))
		{
			auto optValue = getValueByLink(vValue, fRuntime, true /* fRecursive */);
			if (optValue.has_value())
				vData.put(nIndex, optValue.value());
				//variant::putByPath(vData, nIndex, optValue.value(), true /* fCreatePaths */);
		}
		++nIndex;
	}
}

//
//
//
bool Context::isLink(std::string_view sValue) const
{
	return (!sValue.empty() && (sValue.front() == '@'));
}

//
//
//
bool Context::isLink(const Variant& vValue) const
{
	return (vValue.isString() && !((std::string_view)vValue).empty() &&
		(((std::string_view)vValue).front() == '@'));
}

//
//
//
std::optional<Variant> Context::getValueByLink(std::string_view sLink, bool fRuntime, bool fRecursive)
{
	if (!isLink(sLink))
		error::CompileError(SL, location, "Parameter is not a @-link").throwException();
	auto nDotPos = sLink.find_first_of('.', 1);
	auto sSection = (nDotPos != sLink.npos) ? sLink.substr(1, nDotPos - 1) : sLink.substr(1);
	auto section = getSectionFromString(sSection);
	auto sName = (nDotPos != sLink.npos) ? sLink.substr(nDotPos + 1) : "";

	switch (section)
	{
		case Section::Const: return getConstValue(sName, fRuntime, fRecursive);
		case Section::Var: return getVarValue(sName, fRuntime);
		case Section::Rule: return getRuleValue(sName, fRuntime);
		case Section::Context: return getContextValue(sName, fRuntime);
		case Section::Event: return getEventValue(sName, fRuntime);
		case Section::Policy: return getPolicyValue(sName, fRuntime);
	}
	return std::nullopt;
	//error::InvalidArgument(SL, FMT("Unsupported section <" << sSection << ">")).throwException();
}

//
//
//
std::optional<Variant> Context::getConstValue(std::string_view sName, bool fRuntime, bool fRecursive)
{
	if (sName.empty())
		error::CompileError(SL, location, "Empty constant name").throwException();

	std::optional<Variant> optValue;
	if (!pPolicy->vConstants.isNull())
		optValue = variant::getByPathSafe(pPolicy->vConstants, sName);
	if (!optValue.has_value() && !vConstants.isNull())
		optValue = variant::getByPathSafe(vConstants, sName);
	if (!optValue.has_value())
		return std::nullopt;

	auto vValue = optValue.value();
	if (!isLink(vValue))
	{
		if (!fRuntime)
			return resolveValue(vValue, fRuntime);

		if (!vValue.isDictionaryLike() && !vValue.isSequenceLike())
			return optValue;

		runtimeConstants[std::string(sName)] = vValue;
		std::string sPath = resolveValue("@policy.const", false /* fRuntime */);
		sPath += ".";
		sPath += sName;
		return Dictionary({ {"$path", sPath } });
	}

	try
	{
		return getValueByLink((std::string_view)vValue, fRuntime, fRecursive);
	}
	catch (const error::InvalidArgument&)
	{
		// Return the original value for probably non-@-links
		return optValue;
	}
}

//
//
//
std::optional<Variant> Context::getVarValue(std::string_view sName, bool fRuntime)
{
	std::string sPath = resolveValue("@policy.var", false /* fRuntime */);
	if (!sName.empty())
	{
		sPath += ".";
		sPath += sName;
	}

	if (fRuntime)
		return getVariable(sName)->compile(*this);
	return sPath;
}

//
//
//
std::optional<Variant> Context::getRuleValue(std::string_view sName, bool fRuntime)
{
	std::string sPath = resolveValue("@policy.rule", false /* fRuntime */);
	if (!sName.empty())
	{
		sPath += ".";
		sPath += sName;
	}

	if (fRuntime)
		return Dictionary({ {"$path", sPath } });
	return sPath;
}

//
//
//
std::optional<Variant> Context::getContextValue(std::string_view sName, bool fRuntime)
{
	std::string sPath = resolveValue("@policy.context", false /* fRuntime */);
	if (!sName.empty())
	{
		sPath += ".";
		sPath += sName;
	}

	if (fRuntime)
		return Dictionary({ {"$path", sPath } });
	return sPath;
}

//
//
//
std::optional<Variant> Context::getDefaultValue(std::string_view sPath) const
{
	if (sPath.empty())
		return std::nullopt;

	auto optFieldInfo = getObjectFieldInfo(m_vObjects, sPath);
	if (!optFieldInfo.has_value())
		return std::nullopt;

	return optFieldInfo.value().getSafe("default");
}

//
//
//
std::optional<Variant> Context::getObjectFieldInfo(Variant vRoot, const std::string_view& sPath) const
{
	if (sPath.empty())
		return vRoot;

	// Dictionary index
	std::string_view sKey;
	std::string_view sTail;

	auto nEndOfKey = sPath.find_first_of(".");
	if (nEndOfKey == std::string_view::npos)
		sKey = sPath;
	else
	{
		sKey = sPath.substr(0, nEndOfKey);
		if (sPath[nEndOfKey] == '.')
			sTail = sPath.substr(nEndOfKey + 1);
		else
			sTail = sPath.substr(nEndOfKey);
	}

	if (sKey.empty())
		error::InvalidArgument(SL, FMT("Invalid path format <" << sPath << ">")).throwException();

	auto optNext = vRoot.getSafe(sKey);
	if (!optNext.has_value())
		return std::nullopt;

	auto vNext = optNext.value();
	if (vNext.isString())
	{
		auto optObject = m_vObjects.getSafe((std::string_view)vNext);
		if (!optObject.has_value())
			return std::nullopt;
		return getObjectFieldInfo(optObject.value(), sTail);
	}

	if (sTail.empty())
		return optNext;

	return getObjectFieldInfo(optNext.value(), sTail);
}

//
//
//
std::optional<Variant> Context::getEventValue(std::string_view sName, bool fRuntime) const
{
	std::string sPath = "event";
	if (!sName.empty())
	{
		sPath += ".";
		sPath += sName;
	}

	if (fRuntime)
	{
		auto optDefaultValue = getDefaultValue(sPath);
		if (optDefaultValue.has_value())
			return Dictionary({ {"$path", sPath}, {"$default", optDefaultValue.value()} });
		return Dictionary({ {"$path", sPath} });
	}
	return sPath;
}

//
//
//
std::optional<Variant> Context::getPolicyValue(std::string_view sName, bool fRuntime) const
{
	std::string sPath = "policy";
	if (!sName.empty())
	{
		sPath += ".";
		sPath += sName;
	}

	if (fRuntime)
		return Dictionary({ {"$path", sPath } });
	return sPath;
}

//
//
//
Variant Context::normalizeDictSchema(const Variant& vData) const
{
	TRACE_BEGIN;
	if (vData.isDictionaryLike())
	{
		// Create a new schema
		Variant vResult = Sequence();
		for (const auto& [sKey, vValue] : Dictionary(vData))
		{
			vResult.push_back(Dictionary({
				{"name", sKey},
				{"value", vValue}
				}));
		}
		return vResult;
	}

	if (vData.isSequenceLike())
	{
		// Check a given schema
		for (const auto& vItem : Sequence(vData))
		{
			if (!vItem.has("name"))
				error::CompileError(SL, location, "Missing <name> field").throwException();
			if (!vItem.has("value"))
				error::CompileError(SL, location, "Missing <value> field").throwException();
		}
		return vData.clone();
	}
	error::CompileError(SL, location, "Data must be dictionary or sequence").throwException();
	TRACE_END("Failed to normalize dictionary schema");
}

//
//
//
void Chain::addRule(RulePtr pRule)
{
	m_rules.insert(std::make_pair(pRule->nPriority, pRule));
}

//
//
//
Variant Chain::compileBody(Context& ctx) const
{
	TRACE_BEGIN;
	if (m_rules.empty())
		return {};

	Variant vCode = Sequence();
	for (const auto& [nPriority, pRule] : m_rules)
		for (const auto& vCodeLine : pRule->compile(ctx))
			vCode.push_back(vCodeLine);

	Variant vResult = Dictionary({
		{"$$proxy", "cachedObj"},
		{"clsid", CLSID_Scenario},
		{"name", sName},
		{"code", vCode},
		{"regInMgr", false}
		});
	return vResult;
	TRACE_END("Failed to compile a chain's body");
}

//
//
//
Variant Chain::compileCall(Context& ctx) const
{
	TRACE_BEGIN;
	std::string sChainFullName("@policy.chain.");
	sChainFullName += sName;

	Variant vCode = Sequence({
		// Call the chain
		Dictionary({
			{"clsid", CLSID_CallCtxCmd},
			{"command", ctx.resolveValue(sChainFullName)},
			{"$dst", ctx.resolveValue("@policy.__chainCallResult", false /* fRuntime */)}
			}),
		// Return if the <result> exist
		Dictionary({
			{"$ret", ctx.resolveValue("@policy.__chainCallResult")},
			{"$if", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_VariantCtxCmd},
				{"operation", "has"},
				{"path", "result"},
				{"args", Sequence({ ctx.resolveValue("@policy.__chainCallResult") })}
				})}
			})
		});
	return vCode;
	TRACE_END("Failed to compile a chain's call");
}

//
//
//
const RuleMap Chain::getRules() const
{
	return m_rules;
}

//
//
//
void Pattern::addRule(const std::string& sEventType, RulePtr pRule)
{
	m_rules[sEventType].insert(std::make_pair(pRule->nPriority, pRule));
}

//
//
//
const EventRuleMap Pattern::getRules() const
{
	return m_rules;
}

//
//
//
PolicyPtr Policy::create(std::string_view sName, Variant vData)
{
	TRACE_BEGIN;
	auto pObj = std::make_shared<Policy>();

	if (!vData.isDictionaryLike())
		error::CompileError(SL, "Data must be a dictionary").throwException();
	pObj->nVersion = vData.get("version", 1);
	pObj->sName = vData.get("name", sName);
	pObj->nPriority = vData.get("priority", pObj->nPriority);

	if (pObj->nVersion == 1)
		pObj->group = PolicyGroup::EventsMatching;
	else
		pObj->group = getPolicyGroupFromString(vData.get("group", getPolicyGroupString(pObj->group)));

	if (vData.has("dependencies"))
	{
		auto vDependencies = vData["dependencies"];
		if (!vDependencies.isSequenceLike())
			error::CompileError(SL, "Dependencies must be a sequence").throwException();
		for (const auto& sDependingName : vDependencies)
			pObj->dependencies.push_back(sDependingName);
	}
	pObj->vData = vData.clone();
	return pObj;
	TRACE_END("Error during object creation");
}

//////////////////////////////////////////////////////////////////////////////
//
// CompileError methods
//
//////////////////////////////////////////////////////////////////////////////


//
//
//
CompileError::CompileError(const SourceLocation srcLoc, const PolicySourceLocation& policySrcLoc,
	std::string_view sMsg) :
	error::RuntimeError(srcLoc, ErrorCode::CompileError, createWhat(policySrcLoc, sMsg))
{
}

//
//
//
CompileError::CompileError(const SourceLocation srcLoc, PolicySourceLocation&& policySrcLoc,
	std::string_view sMsg) :
	error::RuntimeError(srcLoc, ErrorCode::CompileError, createWhat(std::move(policySrcLoc), sMsg))
{
}

//
//
//
CompileError::CompileError(const SourceLocation srcLoc, std::string_view sMsg) :
	error::RuntimeError(srcLoc, ErrorCode::CompileError, sMsg)
{
}


//
//
//
CompileError::CompileError(std::string_view sMsg) :
	error::RuntimeError(ErrorCode::CompileError, sMsg)
{
}

//
//
//
CompileError::~CompileError()
{
}

//
//
//
std::exception_ptr CompileError::getRealException()
{
	return std::make_exception_ptr(*this);
}

//
//
//
std::string CompileError::createWhat(const PolicySourceLocation& policySrcLoc, std::string_view sMsg)
{
	std::string s(!sMsg.empty() ? sMsg : error::getErrorString(ErrorCode::CompileError));

	if (!policySrcLoc.m_sPolicyName.empty())
	{
		s += " <";
		s += policySrcLoc.m_sPolicyName;

		do
		{
			if (!policySrcLoc.m_sPatternName.empty())
			{
				s += ":patterns:";
				s += policySrcLoc.m_sPatternName;
			}
			else if (!policySrcLoc.m_sChainName.empty())
			{
				s += ":chains:";
				s += policySrcLoc.m_sChainName;
			}
			else if (!policySrcLoc.m_sVariableName.empty())
			{
				s += ":variables:";
				s += policySrcLoc.m_sVariableName;
				break;
			}

			if (policySrcLoc.m_nRuleNumber != 0)
			{
				s += ":";
				s += std::to_string(policySrcLoc.m_nRuleNumber);
			}
			else if (policySrcLoc.m_nBindingNumber != 0)
			{
				s += ":bindings:";
				s += std::to_string(policySrcLoc.m_nBindingNumber);
			}
		} while (false);

		s += ">";
	}

	return s;
}

//////////////////////////////////////////////////////////////////////////////
//
// PolicySourceLocation methods
//
//////////////////////////////////////////////////////////////////////////////

PolicySourceLocation::PolicySourceLocation(std::string_view sPolicyName, std::string_view sPatternName,
	std::string_view sChainName, Size nRuleNumber, Size nBindingNumber, std::string_view sVariableName) :
	m_sPolicyName(sPolicyName), m_sPatternName(sPatternName), m_sChainName(sChainName),
	m_nRuleNumber(nRuleNumber), m_nBindingNumber(nBindingNumber), m_sVariableName(sVariableName)
{
}

//
//
//
void PolicySourceLocation::clear()
{
	m_sPolicyName.clear();
	m_sPatternName.clear();
	m_sChainName.clear();
	clearRule();
	clearBinding();
	m_sVariableName.clear();
}

//
//
//
void PolicySourceLocation::clearRule()
{
	m_nRuleNumber = 0;
}

//
//
//
void PolicySourceLocation::clearBinding()
{
	m_nBindingNumber = 0;
}

//
//
//
void PolicySourceLocation::enterPolicy(std::string_view sName)
{
	clear();
	m_sPolicyName = sName;
}

//
//
//
void PolicySourceLocation::leavePolicy()
{
	clear();
}

//
//
//
void PolicySourceLocation::enterPattern(std::string_view sName)
{
	leaveChain();
	m_sPatternName = sName;
	clearRule();
	clearBinding();
}

//
//
//
void PolicySourceLocation::leavePattern()
{
	m_sPatternName.clear();
	clearRule();
	clearBinding();
}

//
//
//
void PolicySourceLocation::enterChain(std::string_view sName)
{
	leavePattern();
	m_sChainName = sName;
	clearRule();
	clearBinding();
	leaveVariable();
}

//
//
//
void PolicySourceLocation::leaveChain()
{
	m_sChainName.clear();
	clearRule();
	clearBinding();
}

//
//
//
void PolicySourceLocation::enterRule()
{
	++m_nRuleNumber;
}

//
//
//
void PolicySourceLocation::enterBinding()
{
	++m_nBindingNumber;
}

//
//
//
void PolicySourceLocation::enterVariable(std::string_view sName)
{
	leaveChain();
	leavePattern();
	m_sVariableName = sName;
}

//
//
//
void PolicySourceLocation::leaveVariable()
{
	m_sVariableName.clear();
}

} // namespace policy
} // namespace edr
} // namespace openEdr

/// @}
