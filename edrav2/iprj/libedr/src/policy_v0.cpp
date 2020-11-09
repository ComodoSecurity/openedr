//
// edrav2.libedr project
// 
// Author: Yury Ermakov (16.04.2019)
// Reviewer: Denis Bogdanov (30.04.2019)
//
///
/// @file Policy compiler (legacy) implementation
///
/// @addtogroup edr
/// @{
///
#include "pch.h"
#include "policy.h"
#include "policy_v0.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "policy"

namespace cmd {
namespace edr {
namespace policy {
namespace v0 {

	//
//
//
ObjPtr<IPolicyCompiler> PolicyCompilerImpl::create(Variant vConfig)
{
	TRACE_BEGIN;
	auto pObj = createObject<PolicyCompilerImpl>();
	pObj->m_vConfig = vConfig["config"];
	pObj->m_vEventConfig = Dictionary();
	for (const auto& [sCode, vCfg] : Dictionary(pObj->m_vConfig["events"]))
	{
		Variant vEventConfig = pObj->loadEventConfig(vCfg);
		vEventConfig.erase("baseGroup");
		pObj->m_vEventConfig.put(sCode, vEventConfig);
	}

	pObj->m_sPolicyCatalogPath = vConfig.get("policyCatalogPath", "app.config.policy");
	pObj->m_fVerbose = vConfig.get("verbose", pObj->m_fVerbose);

	pObj->m_pRegistryReplacer = queryInterface<IStringMatcher>(createObject(CLSID_StringMatcher, Dictionary({
		{"schema", Sequence({
			Dictionary({
				{"target", "HKEY_LOCAL_MACHINE"},
				{"value", sys::win::c_sHKeyLocalMachinePattern},
				{"mode", variant::operation::MatchMode::Begin},
			}),
			Dictionary({
				{"target", "HKEY_CURRENT_USER"},
				{"value", sys::win::c_sHKeyCurrentUserPattern},
				{"mode", variant::operation::MatchMode::Begin},
			})
		})
		} })));

	pObj->m_pFileReplacer = queryInterface<IStringMatcher>(createObject(CLSID_StringMatcher, Dictionary({
		{"schema", Sequence({
			Dictionary({
				{"target", "%windir%"},
				{"value", "%systemroot%"},
				{"mode", variant::operation::MatchMode::Begin},
			})
		})
		} })));
	return pObj;
	TRACE_END("Error during object creation");
}

//
//
//
Variant PolicyCompilerImpl::parseRule(const Variant& vRule, Context& ctx)
{
	if (vRule.isString())
		return parseCommonRule(vRule, ctx);

	if (!vRule.isDictionaryLike())
		error::CompileError(SL, ctx.location, "Rule must be a dictionary").throwException();

	Variant vCodeLine = Dictionary();

	if (vRule.get("Discard", false) == true)
	{
		if (ctx.fGlobalScope)
			vCodeLine.put("$goto", "exit");
		else
			vCodeLine.put("$ret", Dictionary({ {"discard", {}} }));
	}
	else
		vCodeLine.put("$ret", Dictionary({
			{"eventType", vRule["EventType"]},
			{"baseEventType", vRule["BaseEventType"]}
			}));

	if (vRule.has("Condition"))
		vCodeLine.put("$if", parseCondition(vRule["Condition"], ctx));
	return Sequence({ vCodeLine });
}

//
//
//
std::string PolicyCompilerImpl::getFunctionFullName(const std::string& sName) const
{
	std::string sResult("functions.");
	sResult += sName;
	return sResult;
}

//
//
//
void PolicyCompilerImpl::addFunction(const std::string& sName, Variant vCode, Context& ctx)
{
	if (ctx.functions.has(sName))
		return;

	ctx.functions.put(sName, Dictionary({
		{"$$proxy", "cachedObj"},
		{"clsid", CLSID_Scenario},
		{"name", sName},
		{"code", vCode},
		{"regInMgr", false}
		}));
}

//
//
//
Variant PolicyCompilerImpl::parseCommonRule(std::string_view sName, Context& ctx)
{
	if (!ctx.commonRules.has(sName))
		error::CompileError(SL, ctx.location, FMT("Unknown common rules group <" << sName <<
			">")).throwException();

	// Save current scope state and set non-global scope
	bool fOuterGlobalScope = ctx.fGlobalScope;
	ctx.fGlobalScope = false;

	PolicySourceLocation scopedLocation(ctx.location);
	scopedLocation.enterChain(sName);

	Variant vFuncCode = Sequence();
	for (const auto& vRule : Sequence(ctx.commonRules[sName]))
	{
		try
		{
			scopedLocation.enterRule();
			for (const auto& vCodeLine : Sequence(parseRule(vRule, ctx)))
				vFuncCode.push_back(vCodeLine);
		}
		catch (error::NotFound& e)
		{
			error::CompileError(SL, scopedLocation,
				FMT("Rule is skipped (" << e.what() << ")")).logAsWarning();
		}
	}

	// Calculate functions's hash
	uint64_t hash = crypt::xxh::getHash(vFuncCode, crypt::VariantHash::ViaJson);
	std::string sFuncName = FMT(sName << "-" << hex(hash));

	// Add new common function
	addFunction(sFuncName, vFuncCode, ctx);

	// Restore global scope state
	ctx.fGlobalScope = fOuterGlobalScope;

	return makeFunctionCall(sFuncName, fOuterGlobalScope);
}

//
//
//
Variant PolicyCompilerImpl::makeFunctionCall(const std::string& sName, bool fGlobalScope) const
{
	// Scenario: Call a function and save <result>
	Variant vCode = Sequence({ Dictionary({
		{"clsid", CLSID_CallCtxCmd},
		{"command", Dictionary({ {"$path", getFunctionFullName(sName)} })},
		{"$dst", "result"}
		}) });

	// Scenario: Check if <discard> has been returned
	Variant vDiscardCheck = Dictionary({
		{"$if", Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", "has"},
			{"path", "discard"},
			{"args", Sequence({ Dictionary({ {"$path", "result"}, {"$default", Dictionary()} }) })}
			})}
		});
	if (fGlobalScope)
		vDiscardCheck.put("$goto", "exit");
	else
		vDiscardCheck.put("$ret", Dictionary({ {"discard", {}} }));
	vCode.push_back(vDiscardCheck);

	// Scenario: Return if <result> has non-null value
	vCode.push_back(Dictionary({
		{"$ret", Dictionary({ {"$path", "result"}, {"$default", {}} })},
		{"$if", Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", "!equal"},
			{"args", Sequence({
				Dictionary({ {"$path", "result"}, {"$default", {}} }),
				{}
				})}
			})}
		}));

	return vCode;
}

//
//
//
Variant PolicyCompilerImpl::parseCondition(const Variant& vCondition, Context& ctx) const
{
	if (vCondition.has("BooleanOperator"))
	{
		std::string sOperation = m_vConfig["boolOperations"][(std::string_view)
			vCondition["BooleanOperator"]];
		
		if (!vCondition.has("Conditions"))
			error::CompileError(SL, ctx.location, "Missing <Conditions> field").throwException();
		Variant vConditions = vCondition.get("Conditions");
		if (!vConditions.isSequenceLike())
			error::CompileError(SL, ctx.location, "<Conditions> field must be a sequence").throwException();
		if (vConditions.getSize() == 1)
			return parseCondition(vCondition.get(0), ctx);

		Sequence seqArgs;
		for (const auto& vInnerCondition : vConditions)
			seqArgs.push_back(parseCondition(vInnerCondition, ctx));
	
		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", sOperation},
			{"args", seqArgs}
			});
	}
	
	if (vCondition.has("Operator"))
	{
		if (!vCondition.has("Field"))
			error::CompileError(SL, ctx.location, "Missing <Field> field").throwException();
		if (!vCondition.has("Value"))
			error::CompileError(SL, ctx.location, "Missing <Value> field").throwException();

		Variant vOperationConfig = m_vConfig["operations"][(std::string_view)vCondition["Operator"]];
		std::string sOperation = vOperationConfig.isDictionaryLike() ?
			vOperationConfig["value"] : vOperationConfig;
		bool fListValue = vOperationConfig.isDictionaryLike() ?
			(bool)vOperationConfig.get("isListValue", false) : false;

		// Check if a received field is known
		std::string sSrcField(vCondition["Field"]);
		Variant vFieldsConfig = m_vEventConfig[ctx.sEventCode].get("fields");
		if (!vFieldsConfig.has(sSrcField))
			error::NotFound(SL, FMT("Unknown field <" << sSrcField << ">")).throwException();
		
		// Get field config
		Variant vFieldConfig = vFieldsConfig[sSrcField];
		std::string sField = vFieldConfig.isDictionaryLike() ? vFieldConfig["value"] : vFieldConfig;
		bool fUseRegistryReplacer = vFieldConfig.isDictionaryLike() ?
			(bool)vFieldConfig.get("useRegistryReplacer", false) : false;
		bool fCaseInsensitive = vFieldConfig.isDictionaryLike() ?
			(bool)vFieldConfig.get("caseInsensitive", false) : false;
		bool fUseFileReplacer = vFieldConfig.isDictionaryLike() ?
			(bool)vFieldConfig.get("useFileReplacer", false) : false;

		Variant vTargetValue = Dictionary({ {"$path", sField} });

		std::optional<Variant> optDefaultValue;
		if (vFieldConfig.isDictionaryLike() && vFieldConfig.has("default"))
			vTargetValue.put("$default", vFieldConfig["default"]);

		Variant vValue = vCondition["Value"];
		if (fUseRegistryReplacer && vValue.isString())
			vValue = m_pRegistryReplacer->replace((std::string_view)vValue);
		if (fUseFileReplacer && vValue.isString())
			vValue = m_pFileReplacer->replace((std::string_view)vValue);
		if (fCaseInsensitive && !fListValue && vValue.isString())
			vValue = string::convertToLower((std::string_view)vValue);

		// Do field value tranformation if it's configured
		if (m_vConfig["fieldValueTransform"].has(sSrcField))
		{
			Variant vNewValue = m_vConfig["fieldValueTransform"][sSrcField].get(
				(std::string_view)variant::convert<variant::ValueType::String>(vValue),
				nullptr);
			if (!vNewValue.isNull())
				vValue = vNewValue;
		}

		// Check value type
		Variant::ValueType valueType = vFieldConfig.isDictionaryLike() ?
			(Variant::ValueType)vFieldConfig.get("valueType", Variant::ValueType::String) :
			Variant::ValueType::String;
		// If value types are mismatched, convert the value according to a type specified
		// in order to prevent a further implicit conversion in runtime
		if (vValue.getType() != valueType)
		{
			if (m_fVerbose)
				LOGWRN("Event <" << ctx.sEventCode <<
					"> has a value with a mismatched type in the field <" << sSrcField <<
					"> (received: <" << vValue.getType() << "> expected: <" << valueType <<
					">). Converting the value...");
			switch (valueType)
			{
				case Variant::ValueType::String:
				{
					vValue = variant::convert<variant::ValueType::String>(vValue);
					break;
				}
				case Variant::ValueType::Integer:
				{
					vValue = variant::convert<variant::ValueType::Integer>(vValue);
					break;
				}
				case Variant::ValueType::Boolean:
				{
					vValue = variant::convert<variant::ValueType::Boolean>(vValue);
					break;
				}
			}
		}

		if ((sOperation == "match") || (sOperation == "!match") ||
			(sOperation == "imatch") || (sOperation == "!imatch"))
		{
			std::string sValue(vValue);
			if (fListValue)
			{
				if (!ctx.matchPatterns.has(sValue))
					error::CompileError(SL, ctx.location,
						FMT("Unknown list <" << sValue << ">")).throwException();

				Variant vPatterns = ctx.matchPatterns[sValue];
				if (vPatterns.getSize() == 1)
					return Dictionary({
						{"$$proxy", "cachedObj"},
						{"clsid", CLSID_VariantCtxCmd},
						{"operation", sOperation},
						//{"pattern", vPatterns[0]},
						{"pattern", Dictionary({
							{"$path", FMT("matchPatterns." << sValue << "[0]")}
							})},
						{"args", Sequence({	vTargetValue })}
						});

				Sequence seqArgs;
				for (Size nIndex = 0; nIndex < vPatterns.getSize(); ++nIndex)
				{
					seqArgs.push_back(Dictionary({
						{"$$proxy", "cachedObj"},
						{"clsid", CLSID_VariantCtxCmd},
						{"operation", sOperation},
						{"pattern", Dictionary({
							{"$path", FMT("matchPatterns." << sValue << "[" << nIndex << "]")}
							})},
						{"args", Sequence({ vTargetValue })}
						}));
				}
				return Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", (sOperation.front() == '!' ? "and" : "or")},
					{"args", seqArgs}
					});
			}
			Variant vPattern = parseMatchValue(sValue);
			if (vPattern.isDictionaryLike())
				vPattern = Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_StringMatcher},
					{"schema", (vPattern.getSize() == 1 ? vPattern[0] : vPattern)}
					});

			return Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_VariantCtxCmd},
				{"operation", sOperation},
				{"pattern", vPattern},
				{"args", Sequence({ vTargetValue })}
				});
		}

		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", sOperation},
			{"args", Sequence({
				vValue,
				vTargetValue
				})}
			});
	}
	error::CompileError(SL, ctx.location, "Missing <BooleanOperator> or <Operator> field").throwException();
}

//
//
//
Variant PolicyCompilerImpl::parseList(const Variant& vList) const
{
	Sequence seqMatcherSchema;
	Sequence seqRegexPatterns;

	for (const auto& vItem : vList)
	{
		Variant vValue = parseMatchValue((std::string_view)vItem);
		if (vValue.isString())
			seqRegexPatterns.push_back(vValue);
		else
			seqMatcherSchema.push_back(vValue);
	}

	Sequence seqResult;
	if (!seqMatcherSchema.isEmpty())
		seqResult.push_back(Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_StringMatcher},
			{"schema", seqMatcherSchema}
			}));
	if (!seqRegexPatterns.isEmpty())
		seqResult.push_back(seqRegexPatterns);

	return seqResult;
}

//
//
//
Variant PolicyCompilerImpl::parseMatchValue(std::string_view sValue) const
{
	if (sValue.empty())
		return "";

	std::string sNewValue;
	std::string sMode;
	if ((sValue.front() == '*') && (sValue.back() == '*'))
	{
		sMode = "all";
		sNewValue = sValue.substr(1, sValue.size() - 2);
	}
	else if (sValue.front() == '*')
	{
		sMode = "end";
		sNewValue = sValue.substr(1, sValue.size() - 1);
	}
	else if (sValue.back() == '*')
	{
		sMode = "begin";
		sNewValue = sValue.substr(0, sValue.size() - 1);
	}
	else
	{
		// TODO: We're in potentially non-wildcard matching.
		// So may be just transform 'match' to 'equal'?
		sMode = "begin";
		sNewValue = sValue;
	}

	if (sNewValue.find_first_of("*") == sValue.npos)
		return Dictionary({
				{"target", sNewValue},
				{"mode", sMode}
			});

	return string::convertWildcardToRegex(sValue);
}

//
//
//
Variant PolicyCompilerImpl::loadEventConfig(Variant vCfg) const
{
	TRACE_BEGIN;
	if (vCfg.has("baseGroup"))
	{
		auto vBaseCfg = loadEventConfig(m_vConfig["eventGroups"][(std::string_view)vCfg["baseGroup"]]);
		return vBaseCfg.merge(vCfg, variant::MergeMode::All |
			variant::MergeMode::MergeNestedDict | variant::MergeMode::MergeNestedSeq);
	}
	return vCfg.clone();
	TRACE_END(FMT("Failed to load an event group <" << vCfg.get("baseGroup", "") << ">"));
}

//
//
//
void PolicyCompilerImpl::addMatchPatterns(Variant vData, Context& ctx)
{
	for (const auto& [sName, vValues] : Dictionary(vData))
	{
		auto vPatterns = parseList(vValues);
		ctx.matchPatterns.put(sName, vPatterns);
	}
}

//
//
//
void PolicyCompilerImpl::addCommonRules(Variant vData, Context& ctx)
{
	for (const auto& [sName, vRules] : Dictionary(vData))
	{
		if (!vRules.isSequenceLike())
			error::CompileError(SL, ctx.location, FMT("Common rules group <" << sName <<
				"> must be a sequence")).throwException();
		if (vRules.isEmpty())
		{
			LOGLVL(Detailed, "Common rules group <" << sName << "> is empty -> skipped");
			continue;
		}
		ctx.commonRules.put(sName, vRules);
	}
}

//
//
//
void PolicyCompilerImpl::compileEvent(std::string_view sCode, Variant vSource, Variant vLocal, Context& ctx)
{
	TRACE_BEGIN;
	// Set current compiling event in the context
	ctx.sEventCode = sCode;

	Variant vEventCode = Sequence();
	Variant vEventType = m_vEventConfig[sCode].get("eventType");

	// Single mapped events are in the global scope, but others aren't
	ctx.fGlobalScope = !vEventType.isSequenceLike();

	if (vLocal.has("override"))
	{
		ctx.location.enterPolicy("evmLocal");
		ctx.location.enterChain(std::string(sCode) + "-override");
		if (vLocal.has("before") || vLocal.has("after"))
			error::CompileError(SL, ctx.location, FMT("Local: Event < " << sCode <<
				"> has override rules. Sections <before> and <after> are ignored")).logAsWarning();

		// Override the source rules by the local ones
		for (const auto& vRule : Sequence(vLocal["override"]))
		{
			try
			{
				ctx.location.enterRule();
				for (const auto& vCodeLine : Sequence(parseRule(vRule, ctx)))
					vEventCode.push_back(vCodeLine);
			}
			catch (error::NotFound& e)
			{
				error::CompileError(SL, ctx.location,
					FMT("Rule is skipped (" << e.what() << ")")).logAsWarning();
			}
		}
	}

	if (!vEventCode.isEmpty())
		LOGLVL(Detailed, "Event <" << sCode << "> is overridden by the local policy");
	else
	{
		// Apply local before-rules
		if (vLocal.has("before"))
		{
			ctx.location.enterPolicy("evmLocal");
			ctx.location.enterChain(std::string(sCode) + "-before");
			for (const auto& vRule : Sequence(vLocal["before"]))
			{
				try
				{
					ctx.location.enterRule();
					for (const auto& vCodeLine : Sequence(parseRule(vRule, ctx)))
						vEventCode.push_back(vCodeLine);
				}
				catch (error::NotFound& e)
				{
					error::CompileError(SL, ctx.location,
						FMT("Rule is skipped (" << e.what() << ")")).logAsWarning();
				}
			}
		}

		// Apply source rules
		if (!vSource.isEmpty())
		{
			ctx.location.enterPolicy("evmCloud");
			ctx.location.enterChain(sCode);
			for (const auto& vRule : vSource)
			{
				try
				{
					ctx.location.enterRule();
					for (const auto& vCodeLine : Sequence(parseRule(vRule, ctx)))
						vEventCode.push_back(vCodeLine);
				}
				catch (error::NotFound& e)
				{
					error::CompileError(SL, ctx.location,
						FMT("Rule is skipped (" << e.what() << ")")).logAsWarning();
				}
			}
		}

		// Apply local after-rules
		if (vLocal.has("after"))
		{
			ctx.location.enterPolicy("evmLocal");
			ctx.location.enterChain(std::string(sCode) + "-after");
			for (const auto& vRule : Sequence(vLocal["after"]))
			{
				try
				{
					ctx.location.enterRule();
					for (const auto& vCodeLine : Sequence(parseRule(vRule, ctx)))
						vEventCode.push_back(vCodeLine);
				}
				catch (error::NotFound& e)
				{
					error::CompileError(SL, ctx.location,
						FMT("Rule is skipped (" << e.what() << ")")).logAsWarning();
				}
			}
		}
	}
	ctx.location.enterPolicy("evmCloud");

	if (vEventType.isSequenceLike())
	{
		std::string sFuncName = FMT("fn-" << hex(++ctx.nBranchId));
		addFunction(sFuncName, vEventCode, ctx);

		for (const auto& vItem : vEventType)
		{
			std::string sBranchName = FMT(vItem);
			if (ctx.code.has(sBranchName))
				error::CompileError(SL, ctx.location, FMT("Branch <" << sBranchName <<
					"> already exists. Check compiler.cfg")).throwException();

			ctx.code.put(sBranchName, makeFunctionCall(sFuncName, true /* fOuterGlobalScope */));
		}
	}
	else
	{
		std::string sBranchName = FMT(vEventType);
		if (ctx.code.has(sBranchName))
			error::CompileError(SL, ctx.location, FMT("Branch <" << sBranchName <<
				"> already exists. Check compiler.cfg")).throwException();
		ctx.code.put(sBranchName, vEventCode);
	}

	TRACE_END(FMT("Failed to compile the event <" << sCode << ">"));
}

//
//
//
Variant PolicyCompilerImpl::compile(PolicyGroup group, Variant vData, bool fActivate)
{
	TRACE_BEGIN;
	if (group != PolicyGroup::EventsMatching)
		error::CompileError(SL, FMT("Unsupported policy group <" << getPolicyGroupString(group) <<
			">")).throwException();

	if (!vData.has("evmCloud"))
		error::CompileError(SL, "Missing policy <evmCloud>").throwException();
	if (!vData.has("evmLocal"))
		error::CompileError(SL, "Missing policy <evmLocal>").throwException();

	Variant vSourcePolicy = vData["evmCloud"];
	Variant vLocalPolicy = vData["evmLocal"];

	Context ctx(m_pRegistryReplacer, m_pFileReplacer);

	if (vSourcePolicy.has("Lists"))
	{
		ctx.location.enterPolicy("evmCloud");
		addMatchPatterns(vSourcePolicy["Lists"], ctx);
	}

	if (vLocalPolicy.isDictionaryLike())
	{
		ctx.location.enterPolicy("evmLocal");
		if (vLocalPolicy.has("Lists"))
			addMatchPatterns(vLocalPolicy["Lists"], ctx);

		if (vLocalPolicy.has("CommonRules"))
			addCommonRules(vLocalPolicy["CommonRules"], ctx);
	}

	// Add <exit> branch
	ctx.code.put("exit", Sequence({ Dictionary({ {"$ret", {}} }) }));

	// Compile the source policy (merging the local policy on-the-fly)
	ctx.location.enterPolicy("evmCloud");
	std::unordered_set<std::string> processedEvents;
	for (const auto& [sCode, vRules] : Dictionary(vSourcePolicy["Events"]))
	{
		TRACE_BEGIN;
		ctx.location.enterPolicy("evmCloud");
		if (sCode.size() < 3)
			error::CompileError(SL, ctx.location, FMT("Source: Unknown event type <" << sCode <<
				">")).throwException();
		if (!vRules.isSequenceLike())
			error::CompileError(SL, ctx.location, FMT("Source: Event <" << sCode <<
				"> is not a sequence")).throwException();
		if (vRules.isEmpty())
		{
			LOGLVL(Detailed, "Source: Event <" << sCode << "> has no rules -> skipped");
			continue;
		}
		if (!m_vEventConfig.has(sCode))
		{
			LOGLVL(Detailed, "Source: Event <" << sCode << "> isn't enabled in config -> skipped");
			continue;
		}

		ctx.location.enterPolicy("evmLocal");
		std::string sLocalEventPath("Events.");
		sLocalEventPath += sCode;
		Variant vLocalEvent = variant::getByPath(vLocalPolicy, sLocalEventPath, Dictionary());
		if (!vLocalEvent.isNull() && !vLocalEvent.isDictionaryLike())
			error::CompileError(SL, ctx.location, FMT("Local: Event <" << sCode <<
				"must be a dictionary")).throwException();

		compileEvent(sCode, vRules, vLocalEvent, ctx);
		processedEvents.emplace(sCode);
		LOGLVL(Debug, "Event <" << sCode << "> is added");
		TRACE_END(FMT("Failed to compile the event <" << sCode << ">"));
	}

	// Compile new events from the local policy (absent in the source policy)
	if (vLocalPolicy.isDictionaryLike() && vLocalPolicy.has("Events"))
	{
		ctx.location.enterPolicy("evmLocal");
		for (const auto& [sCode, vRules] : Dictionary(vLocalPolicy["Events"]))
		{
			TRACE_BEGIN;
			if (processedEvents.find(std::string(sCode)) != processedEvents.end())
				continue;

			ctx.location.enterPolicy("evmLocal");
			if (sCode.size() < 3)
				error::CompileError(SL, ctx.location, FMT("Local: Unknown event type <" << sCode <<
					">")).throwException();
			if (!vRules.isDictionaryLike())
				error::CompileError(SL, ctx.location, FMT("Local: Event <" << sCode <<
					"> is not a dictionary")).throwException();
			if (vRules.isEmpty())
			{
				LOGLVL(Detailed, "Local: Event <" << sCode << "> has no rules -> skipped");
				continue;
			}
			if (!m_vEventConfig.has(sCode))
			{
				LOGLVL(Detailed, "Local: Event <" << sCode << "> isn't enabled in config -> skipped");
				continue;
			}

			compileEvent(sCode, {}, vRules, ctx);
			processedEvents.emplace(sCode);
			TRACE_END(FMT("Local: Failed to compile the event <" << sCode << ">"));
		}
	}
	ctx.location.leavePolicy();

	// Add <main> branch
	Variant vMainBranch = Sequence();
	if (!ctx.matchPatterns.isEmpty())
		vMainBranch.push_back(Dictionary({ {"$set", ctx.matchPatterns}, {"$dst", "matchPatterns"} }));
	if (!ctx.functions.isEmpty())
		vMainBranch.push_back(Dictionary({ {"$set", ctx.functions}, {"$dst", "functions"} }));
	vMainBranch.push_back(Dictionary({
		{"$goto", Dictionary({ {"$path", "event.type"} })},
		{"$default", "exit"}
		}));
	ctx.code.put("main", vMainBranch);

	std::string sPolicyName(getPolicyGroupShortString(group));
	sPolicyName += "_policy_v0";

	Variant vScenario = Dictionary({
		{"$$proxy", "cachedObj"},
		{"clsid", CLSID_Scenario},
		{"name", sPolicyName},
		{"code", ctx.code}
		});

	if (fActivate)
		activate(group, vScenario);
	return vScenario;
	TRACE_END(FMT("Error during policy compiling"));
}

} // namespace v0
} // namespace policy
} // namespace edr
} // namespace cmd

/// @}
