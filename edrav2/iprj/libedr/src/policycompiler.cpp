//
// edrav2.libedr project
// 
// Author: Yury Ermakov (20.08.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Policy compiler implementation
///
/// @addtogroup edr
/// @{
#include "pch.h"
#include "policycompiler.h"
#include "policy_v1.h"
#include "policy_v2.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "policy"

namespace openEdr {
namespace edr {
namespace policy {

//
//
//
void PolicyCompiler::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	m_sPolicyCatalogPath = vConfig.get("policyCatalogPath", "app.config.policy");
	m_fVerbose = vConfig.get("verbose", m_fVerbose);

	if (!vConfig.has("config"))
		error::InvalidArgument(SL, "Missing field <config>").throwException();

	Variant vCompilerConfig = vConfig["config"];
	if (!vCompilerConfig.isDictionaryLike())
		error::InvalidArgument(SL, "Compiler's config is not a dictionary").throwException();
	if (!vCompilerConfig.has("objects"))
		error::InvalidArgument(SL, "Missing field <config.objects>").throwException();
	if (!vCompilerConfig.has("eventClasses"))
		error::InvalidArgument(SL, "Missing field <config.eventClasses>").throwException();
	m_vConfig = vCompilerConfig;
	TRACE_END("Error during object creation");
}

//
//
//
void PolicyCompiler::parsePolicy(PolicyPtr pPolicy, Context& ctx)
{
	if (pPolicy->fParsed)
		return;

	ctx.location.enterPolicy(pPolicy->sName);
	for (const auto& sDependency : pPolicy->dependencies)
	{
		auto pDependentPolicy = ctx.getPolicy(sDependency);
		if (!pDependentPolicy)
			error::CompileError(SL, FMT("Policy <" << sDependency << "> is not found")).throwException();
		parsePolicy(pDependentPolicy, ctx);
	}

	ctx.location.enterPolicy(pPolicy->sName);
	auto pParser = getParser(pPolicy->nVersion);
	pParser->parse(pPolicy, ctx);
}

//
//
//
Variant PolicyCompiler::compile(PolicyGroup group, Variant vData, bool fActivate)
{
	TRACE_BEGIN;
	if (!vData.isDictionaryLike())
		error::CompileError(SL, "Data must be a dictionary").throwException();

	Context ctx(m_vConfig["objects"], m_vConfig["eventClasses"]);

	// Load policies
	for (const auto& [sName, vPolicyDescr] : Dictionary(vData))
	{
		if (vPolicyDescr.isEmpty())
		{
			LOGLVL(Detailed, "Policy <" << sName << "> is empty. Skipped");
			continue;
		}

		auto pPolicy = Policy::create(sName, vPolicyDescr);

		if ((ctx.policyGroup == PolicyGroup::Undefined) && (group != PolicyGroup::AutoDetect))
			ctx.policyGroup = group;

		if (pPolicy->group != PolicyGroup::Undefined)
		{
			if (ctx.policyGroup == PolicyGroup::Undefined)
				ctx.policyGroup = pPolicy->group;
			else if (pPolicy->group != ctx.policyGroup)
			{
				ctx.location.enterPolicy(sName);
				error::CompileError(SL, ctx.location,
					FMT("Wrong policy group (received: <" << getPolicyGroupString(pPolicy->group) <<
						">, expected <" << getPolicyGroupString(ctx.policyGroup) << ">")).throwException();
			}
		}

		ctx.addPolicy(pPolicy);
	}

	// Parse all policies with dependencies
	for (const auto& [sName, pPolicy] : ctx.policies)
		parsePolicy(pPolicy, ctx);

	auto vConstants = compileConstants(ctx);
	//auto vChains = compileChains(ctx);
	auto vEventRules = compileEventRules(ctx);

	Variant vPolicyCode = Dictionary();

	// Build <exit> branch
	Variant vExitBranch = Sequence({ Dictionary({ {"$ret", {}} }) });
	vPolicyCode.put("exit", vExitBranch);

	// Build <main> branch
	Variant vMainBranch = Sequence();
	if (!vConstants.isNull())
		vMainBranch.push_back(Dictionary({
			{"$set", vConstants},
			{"$dst", ctx.resolveValue("@policy.const", false /* fRuntime */)}
			}));
	//if (!vChains.isNull())
	//	vMainBranch.push_back(Dictionary({
	//		{"$set", vChains},
	//		{"$dst", ctx.resolveValue("@policy.chain", false /* fRuntime */)}
	//		}));
	vMainBranch.push_back(Dictionary({
		{"$goto", ctx.resolveValue("@event.type")},
		{"$default", "exit"}
		}));
	vPolicyCode.put("main", vMainBranch);

	// Build event branches
	if (!vEventRules.isNull())
	{
		for (const auto& [sBranchName, vCode] : Dictionary(vEventRules))
		{
			if (vPolicyCode.has(sBranchName))
				error::CompileError(SL, ctx.location,
					FMT("Duplicate branch <" << sBranchName << ">")).throwException();
			vPolicyCode.put(sBranchName, vCode);
		}
	}
	else
		LOGLVL(Detailed, "Policy has no event branches");

	if (ctx.policyGroup == PolicyGroup::Undefined)
		error::CompileError(SL, ctx.location, "Can't determine the policy group").throwException();

	// Build the final scenario and activate it
	std::string sPolicyName(getPolicyGroupShortString(ctx.policyGroup));
	sPolicyName += "_policy";
	Variant vScenario = Dictionary({
		{"$$proxy", "cachedObj"},
		{"clsid", CLSID_Scenario},
		{"name", sPolicyName},
		{"code", vPolicyCode}
		});
	if (fActivate)
		activate(ctx.policyGroup, vScenario);
	return vScenario;
	TRACE_END("Error during policy compiling");
}

//
//
//
ParserPtr PolicyCompiler::createParser(Size nVersion) const
{
	switch (nVersion)
	{
		case 1: return v1::Parser::create(m_vConfig);
		case 2: return v2::Parser::create();
	}
	error::CompileError(SL, FMT("Unknown policy version <" << nVersion << ">")).throwException();
}

//
//
//
ParserPtr PolicyCompiler::getParser(Size nVersion)
{
	auto it = m_parsers.find(nVersion);
	if (it != m_parsers.end())
		return it->second;

	auto pParser = createParser(nVersion);
	m_parsers[nVersion] = pParser;
	return pParser;
}

//
//
//
Variant PolicyCompiler::compileConstants(Context& ctx) const
{
	TRACE_BEGIN;
	if (ctx.runtimeConstants.empty())
		return {};

	Variant vCode = Dictionary();
	for (const auto& [sName, vValue] : ctx.runtimeConstants)
		vCode.put(sName, vValue);
	return vCode;
	TRACE_END("Failed to compile constants");
}

//
//
//
Variant PolicyCompiler::compileChains(Context& ctx) const
{
	TRACE_BEGIN;
	Variant vCode = Dictionary();
	for (const auto& [sName, pChain] : ctx.chains)
	{
		auto vCompiled = pChain->compileBody(ctx);
		if (!vCompiled.isNull())
			vCode.put(sName, vCompiled);
	}
	return vCode;
	TRACE_END("Failed to compile chains");
}

//
//
//
Variant PolicyCompiler::compileEventRules(Context& ctx) const
{
	TRACE_BEGIN;

	// Arrange all pattern rules by event
	EventRuleMap patternEventRules;
	for (const auto& [sName, pPattern] : ctx.patterns)
	{
		for (const auto& [eventType, prioritizedRules] : pPattern->getRules())
		{
			for (const auto& pItem : prioritizedRules)
				patternEventRules[eventType].insert(pItem);
		}
	}

	// Arrange all chained rules by event
	EventRuleMap chainedEventRules;
	for (const auto& [sEventType, pChain] : ctx.bindings)
	{
		for (const auto& pItem : pChain->getRules())
			chainedEventRules[sEventType].insert(pItem);
	}

	std::multimap<std::string, Variant> eventMap;

	// Append chained event rules
	//for (const auto& [eventType, prioritizedChains] : ctx.m_bindings)
	//{
	//	for (const auto& [nPriority, pChain] : prioritizedChains)
	//		for (const auto& vCodeLine : pChain->compileCall(ctx))
	//			eventMap.insert(std::make_pair(eventType, vCodeLine));
	//}
	for (const auto& [sEventType, prioritizedRules] : chainedEventRules)
	{
		for (const auto& [nPriority, pRule] : prioritizedRules)
			for (const auto& vCodeLine : pRule->compile(ctx))
				eventMap.insert(std::make_pair(sEventType, vCodeLine));
	}

	// Append pattern event rules
	for (const auto& [eventType, prioritizedRules] : patternEventRules)
	{
		for (const auto& [nPriority, pRule] : prioritizedRules)
			for (const auto& vCodeLine : pRule->compile(ctx))
				eventMap.insert(std::make_pair(eventType, vCodeLine));
	}

	if (eventMap.empty())
		return {};

	// Build the result
	Variant vCode = Dictionary();
	for (const auto& [sEventType, vValue] : eventMap)
	{
		if (!vCode.has(sEventType))
			vCode.put(sEventType, Sequence());
		vCode.get(sEventType).push_back(vValue);
	}
	return vCode;
	TRACE_END("Failed to compile event rules");
}

//
//
//
void PolicyCompiler::activate(PolicyGroup group, Variant vScenario) const
{
	TRACE_BEGIN;
	std::string sPolicyGroupPath(m_sPolicyCatalogPath);
	sPolicyGroupPath += ".groups.";
	sPolicyGroupPath += getPolicyGroupString(group);

	std::string sScenarioPath(sPolicyGroupPath);
	sScenarioPath += ".scenario";

	std::string sActivePolicyPath(sPolicyGroupPath);
	sActivePolicyPath += ".active";

	std::string sHandlerPath(sPolicyGroupPath);
	sHandlerPath += ".handlerName";

	// Resolve the 'scenario' field in config manually to avoid the races
	// while 'apply_policy' scenario is being compiled during the first-time
	// execution (see EDR-2241)
	(void)getCatalogData(sScenarioPath).getType();

	putCatalogData(sScenarioPath, variant::deserializeObject(vScenario.clone(), /* fRecursively */ true));
	auto vDstPolicy = getCatalogData(sActivePolicyPath);
	vDstPolicy.put("scenario", vScenario);

	// Mark the event handler scenario for reloading
	std::string sHandlerVersionPath(sHandlerPath);
	sHandlerVersionPath += ".version";
	putCatalogData(sHandlerVersionPath, getTickCount());

	sendMessage(Message::PolicyIsUpdated);
	LOGLVL(Detailed, "Policy has been updated");
	TRACE_END("Error during the policy activation");
}

///
/// #### Supported commands
///
Variant PolicyCompiler::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;
	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command's parameters:\n" << vParams);

	///
	/// @fn Variant PolicyCompiler::execute()
	///
	/// ##### compile ()
	/// Compiles and activates the policy.
	///   * group [str] - a name of the policy group.
	///   * data [variant] - a sequence of source policies.
	///
	if (vCommand == "compile")
	{
		if (!vParams.has("group"))
			error::InvalidArgument(SL, "Missing field <group>").throwException();
		auto group = getPolicyGroupFromString(vParams["group"]);
		if (!vParams.has("data"))
			error::InvalidArgument(SL, "Missing field <data>").throwException();
		(void)compile(group, vParams["data"], true /* fActivate */);
		return true;
	}

	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
	error::OperationNotSupported(SL, FMT("PolicyCompiler doesn't support command <" << vCommand
		<< ">")).throwException();
}

} // namespace policy
} // namespace edr
} // namespace openEdr

/// @}
