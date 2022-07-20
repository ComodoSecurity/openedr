//
// edrav2.libedr project
// 
// Author: Yury Ermakov (16.04.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Policy compiler (legacy) implementation
///
/// @addtogroup edr
/// @{
#include "pch.h"
#include <events.hpp>
#include "policy.h"
#include "policy_v1.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "policy1"

namespace cmd {
namespace edr {
namespace policy {
namespace v1 {

//
//
//
RulePtr RuleImpl::create()
{
	return std::make_shared<RuleImpl>();
}

constexpr char c_sAnd[] = "And";
constexpr char c_sOr[] = "Or";
constexpr char c_sEqual[] = "Equal";
constexpr char c_sNotEqual[] = "!Equal";
constexpr char c_sMatch[] = "Match";
constexpr char c_sNotMatch[] = "!Match";
constexpr char c_sMatchInList[] = "MatchInList";
constexpr char c_sNotMatchInList[] = "!MatchInList";

//
//
//
Operation RuleImpl::getOperation(std::string_view sName) const
{
	if (sName == c_sAnd) return Operation::And;
	if (sName == c_sOr) return Operation::Or;
	if (sName == c_sEqual) return Operation::Equal;
	if (sName == c_sNotEqual) return Operation::NotEqual;
	if (sName == c_sMatch) return Operation::IMatch;
	if (sName == c_sNotMatch) return Operation::NotIMatch;
	if (sName == c_sMatchInList) return Operation::IMatch;
	if (sName == c_sNotMatchInList) return Operation::NotIMatch;
	error::InvalidArgument(SL, FMT("Unknown operator <" << sName<< ">")).throwException();
}

//
//
//
Variant RuleImpl::parseOperation(const Variant& vData, Context& ctx)
{
	if (!vData.has("BooleanOperator") && !vData.has("Operator"))
		error::CompileError(SL, ctx.location,
			"Missing both fields <BooleanOperator> and <Operator>").throwException();

	if (vData.has("BooleanOperator"))
	{
		auto operation = getOperation(vData["BooleanOperator"]);
		
		if (!vData.has("Conditions"))
			error::CompileError(SL, ctx.location, "Missing field <Conditions>").throwException();
		
		Variant vConditions = vData["Conditions"];
		if (!vConditions.isSequenceLike())
			error::CompileError(SL, ctx.location, "Conditions must be a sequence").throwException();
		if (vConditions.getSize() == 1)
			return parseOperation(vConditions.get(0), ctx);

		Variant vArgs = Sequence();
		for (const auto& vInner : vConditions)
			vArgs.push_back(parseOperation(vInner, ctx));

		Variant vParams = Dictionary({ {"args", vArgs} });
		return createOperation(operation, vParams, ctx);
	}

	if (!vData.has("Operator"))
		error::CompileError(SL, ctx.location,
			"Missing <BooleanOperator> or <Operator> field").throwException();

	auto operation = getOperation(vData["Operator"]);

	if (!vData.has("Field"))
		error::CompileError(SL, ctx.location, "Missing field <Field>").throwException();
	if (!vData.has("Value"))
		error::CompileError(SL, ctx.location, "Missing field <Value>").throwException();

	bool fListValue = ((vData["Operator"] == c_sMatchInList) || (vData["Operator"] == c_sNotMatchInList));

	// Check if the received field is known
	std::string sSrcField(vData["Field"]);
	Variant vFieldsConfig = ctx.vEventConfig[ctx.sEventCode].get("fields");
	if (!vFieldsConfig.has(sSrcField))
		error::NotFound(SL, FMT("Unknown field <" << sSrcField << ">")).throwException();

	// Get field config
	Variant vFieldConfig = vFieldsConfig[sSrcField];
	std::string sFieldLink("@");
	sFieldLink += vFieldConfig.isDictionaryLike() ? vFieldConfig["value"] : vFieldConfig;
	bool fUseRegistryReplacer = vFieldConfig.isDictionaryLike() ?
		(bool)vFieldConfig.get("useRegistryReplacer", false) : false;
	bool fCaseInsensitive = vFieldConfig.isDictionaryLike() ?
		(bool)vFieldConfig.get("caseInsensitive", false) : false;
	bool fUseFileReplacer = vFieldConfig.isDictionaryLike() ?
		(bool)vFieldConfig.get("useFileReplacer", false) : false;

	Variant vValue = vData["Value"];
	if (fUseRegistryReplacer && vValue.isString())
		vValue = ctx.pRegistryReplacer->replace((std::string_view)vValue);
	if (fUseFileReplacer && vValue.isString())
		vValue = ctx.pFileReplacer->replace((std::string_view)vValue);
		
	if (fCaseInsensitive && !fListValue && vValue.isString())
		vValue = string::convertToLower((std::string_view)vValue);

	// Do value tranformation for some fields
	if (sSrcField == "ftype")
	{
		std::string sValue = variant::convert<variant::ValueType::String>(vValue);
		if (sValue == "PORTABLE_EXECUTABLE")
			vValue = "EXECUTABLE";
	}
	if (sSrcField == "transProto")
	{
		std::string sValue = variant::convert<variant::ValueType::String>(vValue);
		if (sValue == "0")
			vValue = IpProtocol::Tcp;
		if (sValue == "1")
			vValue = IpProtocol::Udp;
		if (sValue == "2")
			vValue = IpProtocol::Unknown;
	}

	// Check value type
	Variant::ValueType valueType = vFieldConfig.isDictionaryLike() ?
		(Variant::ValueType)vFieldConfig.get("valueType", Variant::ValueType::String) :
		Variant::ValueType::String;
	// If value types are mismatched, convert the value according to a type specified
	// in order to prevent a further implicit conversion in runtime
	if (vValue.getType() != valueType)
	{
		if (ctx.fVerbose)
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

	if ((operation != Operation::Match) && (operation != Operation::NotMatch) &&
		(operation != Operation::IMatch) && (operation != Operation::NotIMatch))
	{
		Variant vParams = Dictionary({
			{"args", Sequence({
				vValue,
				ctx.resolveValue(sFieldLink, true /* fRuntime */)
				})}
			});
		return createOperation(operation, vParams, ctx);
	}

	// Match-like operations processing
	std::string sValue(vValue);
	if (!fListValue)
	{
		Variant vParams = Dictionary({
			{"pattern", sValue},
			{"args", Sequence({ ctx.resolveValue(sFieldLink, true /* fRuntime */) })}
			});
		return createOperation(operation, vParams, ctx);
	}

	// MatchInList and !MatchInList operators
	if (!ctx.pPolicy->vConstants.has(sValue))
		error::CompileError(SL, ctx.location, FMT("Unknown list <" << sValue << ">")).throwException();

	std::string sListLink("@const.");
	sListLink += sValue;

	Variant vParams = Dictionary({
		{"pattern", ctx.resolveValue(sListLink, false /* fRuntime */)},
		{"args", Sequence({ ctx.resolveValue(sFieldLink, true /* fRuntime */) })}
		});
	return createOperation(operation, vParams, ctx);
}

//
//
//
void RuleImpl::parseCondition(const Variant vData, Context& ctx)
{
	TRACE_BEGIN;
	try
	{
		m_pCondition = queryInterface<operation::IOperation>(parseOperation(vData, ctx));
	}
	catch (const error::InvalidArgument& ex)
	{
		error::CompileError(SL, ctx.location, ex.what()).throwException();
	}
	TRACE_END("Failed to parse a condition");
}

//
//
//
void RuleImpl::parse(const Variant& vData, Context& ctx)
{
	TRACE_BEGIN;
	if (!vData.isDictionaryLike())
		error::CompileError(SL, ctx.location, "Rule must be a dictionary").throwException();

	if (vData.has("BaseEventType"))
	{
		m_optBaseEventType = vData["BaseEventType"];
		m_sType = ctx.sEventCode;
		m_sType += '.';
		m_sType += std::to_string(m_optBaseEventType.value());

		if (vData.has("EventType") && vData["EventType"].isString())
		{
			m_vEventType = vData["EventType"];
			std::string sInnerType(m_vEventType);
			if (!sInnerType.empty())
			{
				m_sType += '.';
				m_sType += sInnerType;
			}
		}
	}

	if (vData.has("Condition"))
		parseCondition(vData["Condition"], ctx);

	TRACE_END("Failed to parse a rule");
}

//
//
//
Variant RuleImpl::compile(Context& ctx)
{
	TRACE_BEGIN;
	Variant vNewEventData;
	if (m_optBaseEventType.has_value())
	{
		vNewEventData = Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", "merge"},
			{"flags", variant::MergeMode::All | variant::MergeMode::CheckType |
				variant::MergeMode::MergeNestedDict | variant::MergeMode::MergeNestedSeq |
				variant::MergeMode::NonErasingNulls},
			{"args", Sequence({
				Dictionary({ {"$path", "event"} }),
				Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_MakeDictionaryCtxCmd},
					{"data", Sequence({
						Dictionary({
							{"name", "type"},
							{"value", m_sType}
							}),
						Dictionary({
							{"name", "baseEventType"},
							{"value", m_optBaseEventType.value()}
							}),
						Dictionary({
							{"name", "eventType"},
							{"value", m_vEventType}
							})
						})}
					})
				})}
			});
	}

	if (m_pCondition)
	{
		Variant vResultCode = Sequence({
			Dictionary({
				{"$set", RuleState::Undefined},
				{"$dst", ctx.resolveValue("@rule.state", false /* fRuntime */)},
				}),
			Dictionary({
				{"$set", RuleState::FalseCondition},
				{"$dst", ctx.resolveValue("@rule.state", false /* fRuntime */)},
				{"$if", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", "and"},
					{"args", Sequence({
						Dictionary({
							{"$$proxy", "cachedObj"},
							{"clsid", CLSID_VariantCtxCmd},
							{"operation", "equal"},
							{"args", Sequence({
								Dictionary({
									{"$path", ctx.resolveValue("@rule.state", false /* fRuntime */)},
									{"$default", RuleState::Undefined}
									}),
								RuleState::Undefined
								})}
							}),
						Dictionary({
							{"$$proxy", "cachedObj"},
							{"clsid", CLSID_VariantCtxCmd},
							{"operation", "not"},
							{"args", Sequence({	m_pCondition->compile(ctx) })}
							})
						})}
					})}
				}),
			Dictionary({
				{"$set", RuleState::Matched},
				{"$dst", ctx.resolveValue("@rule.state", false /* fRuntime */)},
				{"$if", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", "equal"},
					{"args", Sequence({
						Dictionary({
							{"$path", ctx.resolveValue("@rule.state", false /* fRuntime */)},
							{"$default", RuleState::Undefined}
							}),
						RuleState::Undefined
						})}
					})}
				})
			});

		if (!vNewEventData.isNull())
		{
			vResultCode.push_back(Dictionary({
				{"clsid", CLSID_CallCtxCmd},
				{"command", Dictionary({ {"$path", "eventHandler.call.createEvent"} })},
				{"ctxParams", Dictionary({
					{"destination", getDestinationString(Destination::Out)},
					{"data", vNewEventData}
					})},
				{"ctx", {}},
				{"$if", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", "equal"},
					{"args", Sequence({
						Dictionary({
							{"$path", ctx.resolveValue("@rule.state", false /* fRuntime */)},
							{"$default", RuleState::Undefined}
							}),
						RuleState::Matched
						})}
					})}
				}));
			vResultCode.push_back(Dictionary({
				{"$ret", {}},
				{"$if", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", "equal"},
					{"args", Sequence({
						Dictionary({
							{"$path", ctx.resolveValue("@rule.state", false /* fRuntime */)},
							{"$default", RuleState::Undefined}
							}),
						RuleState::Matched
						})}
					})}
				}));
		}
		return vResultCode;
	}

	Variant vResultCode = Sequence({ Dictionary({
		{"$set", RuleState::Matched},
		{"$dst", ctx.resolveValue("@rule.state", false /* fRuntime */)},
		}) });

	if (!vNewEventData.isNull())
	{
		vResultCode.push_back(Dictionary({
			{"clsid", CLSID_CallCtxCmd},
			{"command", Dictionary({ {"$path", "eventHandler.call.createEvent"} })},
			{"ctxParams", Dictionary({
				{"destination", getDestinationString(Destination::Out)},
				{"data", vNewEventData}
				})},
			{"ctx", {}}
			}));
		vResultCode.push_back(Dictionary({ {"$ret", {}} }));
	}
	return vResultCode;
	TRACE_END("Failed to compile a rule");
}

//
//
//
ParserPtr Parser::create(Variant vConfig)
{
	TRACE_BEGIN;
	auto pObj = std::make_shared<Parser>();
	pObj->m_vEventConfig = Dictionary();
	for (const auto& [sCode, vCfg] : Dictionary(vConfig["events"]))
	{
		Variant vEventConfig = pObj->loadEventConfig(vConfig["eventGroups"], vCfg);
		vEventConfig.erase("baseGroup");
		pObj->m_vEventConfig.put(sCode, vEventConfig);
	}

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
RulePtr Parser::parseRule(const Variant& vData, Context& ctx)
{
	ctx.location.enterRule();
	auto pRule = RuleImpl::create();
	pRule->parse(vData, ctx);
	return pRule;
}

//
//
//
Variant Parser::loadEventConfig(const Variant& vEventGroups, const Variant& vEventConfig) const
{
	TRACE_BEGIN;
	if (vEventConfig.has("baseGroup"))
	{
		auto vBaseConfig = loadEventConfig(vEventGroups,
			vEventGroups[(std::string_view)vEventConfig["baseGroup"]]);
		return vBaseConfig.merge(vEventConfig, variant::MergeMode::All |
			variant::MergeMode::MergeNestedDict | variant::MergeMode::MergeNestedSeq);
	}
	return vEventConfig.clone();
	TRACE_END(FMT("Failed to load the event group <" << vEventConfig.get("baseGroup", "") << ">"));
}

//
//
//
void Parser::parseEvent(std::string_view sCode, Variant vSource, Context& ctx)
{
	TRACE_BEGIN;
	// Set current compiling event in the context
	ctx.sEventCode = sCode;

	ctx.location.enterChain(sCode);
	auto pChain = Chain::create(sCode);

	for (const auto& vRuleDescr : vSource)
	{
		try
		{
			pChain->addRule(parseRule(vRuleDescr, ctx));
		}
		catch (error::NotFound& e)
		{
			error::CompileError(SL, ctx.location,
				FMT("Rule is skipped (" << e.what() << ")")).logAsWarning();
		}
	}
	ctx.addChain(pChain);

	ctx.location.enterBinding();

	Variant vEventTypes = m_vEventConfig[sCode].get("eventType");
	if (!vEventTypes.isSequenceLike())
	{
		ctx.addBinding(vEventTypes, pChain);
		return;
	}

	for (const std::string& sEventType : vEventTypes)
		ctx.addBinding(sEventType, pChain);

	TRACE_END(FMT("Failed to compile the event <" << sCode << ">"));
}

//
//
//
void Parser::parse(PolicyPtr pPolicy, Context& ctx)
{
	CMD_TRY
	{
		ctx.location.enterPolicy(pPolicy->sName);
		ctx.pPolicy = pPolicy;

		// Initialize all relative stuff the context
		ctx.vEventConfig = m_vEventConfig;
		ctx.pRegistryReplacer = m_pRegistryReplacer;
		ctx.pFileReplacer = m_pFileReplacer;

		if (pPolicy->vData.has("Lists"))
		{
			auto vLists = pPolicy->vData["Lists"];
			if (!vLists.isDictionaryLike())
				error::CompileError(SL, ctx.location, "Lists must be a dictionary").throwException();
			pPolicy->vConstants = vLists;

			// Add constants to context
			if (!pPolicy->vConstants.isEmpty())
				ctx.vConstants.merge(pPolicy->vConstants, variant::MergeMode::All |
					variant::MergeMode::CheckType | variant::MergeMode::MergeNestedDict);
		}

		if (!pPolicy->vData.has("Events"))
			error::CompileError(SL, ctx.location, "Missing field <Events>").throwException();

		auto vEvents = pPolicy->vData["Events"];
		if (!vEvents.isDictionaryLike())
			error::CompileError(SL, ctx.location, "Lists must be a dictionary").throwException();

		for (const auto& [sCode, vRules] : Dictionary(vEvents))
		{
			TRACE_BEGIN;
			if (sCode.size() < 3)
				error::CompileError(SL, ctx.location, FMT("Unknown event type <" << sCode <<
					">")).throwException();
			if (!vRules.isSequenceLike())
				error::CompileError(SL, ctx.location, FMT("Event <" << sCode << 
					"> is not a sequence")).throwException();
			if (vRules.isEmpty())
			{
				LOGLVL(Detailed, "Event <" << sCode << "> has no rules -> skipped");
				continue;
			}
			if (!m_vEventConfig.has(sCode))
			{
				LOGLVL(Detailed, "Event <" << sCode << "> isn't enabled in config -> skipped");
				continue;
			}

			parseEvent(sCode, vRules, ctx);
			LOGLVL(Debug, "Event <" << sCode << "> is added");
			TRACE_END(FMT("Failed to compile the event <" << sCode << ">"));
		}
		// Mark the policy as successfully parsed
		pPolicy->fParsed = true;
	}
	CMD_PREPARE_CATCH
	CMD_CATCH_COMPILE(ctx.location);
}

} // namespace v1
} // namespace policy
} // namespace edr
} // namespace cmd

/// @}
