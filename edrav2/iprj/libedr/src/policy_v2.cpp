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
#include "policy_v2.h"
#include "policyoperation.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "policy2"

namespace openEdr {
namespace edr {
namespace policy {
namespace v2 {

constexpr char c_sCondition[] = "condition";
constexpr char c_sLoadContext[] = "loadContext";
constexpr char c_sSaveContext[] = "saveContext";
constexpr char c_sDeleteContext[] = "deleteContext";
constexpr char c_sCreateEvent[] = "createEvent";
constexpr char c_sDiscard[] = "discard";
constexpr char c_sDebug[] = "debug";

constexpr char c_sOperationMarker[] = "$operation";

//
//
//
const char* getDirectiveString(Directive directive)
{
	switch (directive)
	{
		case Directive::Condition: return c_sCondition;
		case Directive::LoadContext: return c_sLoadContext;
		case Directive::SaveContext: return c_sSaveContext;
		case Directive::DeleteContext: return c_sDeleteContext;
		case Directive::CreateEvent: return c_sCreateEvent;
		case Directive::Discard: return c_sDiscard;
		case Directive::Debug: return c_sDebug;
	}
	error::InvalidArgument(SL, FMT("Directive <" << +(std::uint8_t)directive <<
		"> has no text string")).throwException();
}

//
//
//
Directive getDirectiveFromString(std::string_view sDirective)
{
	if (sDirective == c_sCondition) return Directive::Condition;
	if (sDirective == c_sLoadContext) return Directive::LoadContext;
	if (sDirective == c_sSaveContext) return Directive::SaveContext;
	if (sDirective == c_sDeleteContext) return Directive::DeleteContext;
	if (sDirective == c_sCreateEvent) return Directive::CreateEvent;
	if (sDirective == c_sDiscard) return Directive::Discard;
	if (sDirective == c_sDebug) return Directive::Debug;
	error::InvalidArgument(SL, FMT("Unknown directive <" << sDirective << ">")).throwException();
}

//
//
//
class LoadContextDirective : public IDirective
{
private:
	std::string m_sBasketName;
	Variant m_vKey;
	std::optional<bool> m_optDelete;

	//
	//
	//
	Variant compilePeekContext(Context& ctx) const
	{
		Variant vCodeLine = Dictionary({
			{"clsid", CLSID_CallCtxCmd},
			{"command", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_Command},
				{"processor", "objects.contextService"},
				{"command", "has"}
				})},
			{"ctxParams", Dictionary({
				{"id", Dictionary({
					{"$val", Dictionary({
						{"$$proxy", "cachedCmd"},
						{"processor", "objects.contextService"},
						{"command", "getBasketId"},
						{"params", Dictionary({ {"name", m_sBasketName} })}
						})}
					})},
				})},
			{"$dst", ctx.resolveValue("@rule.__contextExist", false /* fRuntime */)}
			});

		// Append <key>
		if (m_vKey.isSequenceLike())
			variant::putByPath(vCodeLine, "ctxParams.key", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_MakeSequenceCtxCmd},
				{"data", m_vKey},
				}),
				true /* fCreatePaths */);
		else
			variant::putByPath(vCodeLine, "ctxParams.key", m_vKey, true /* fCreatePaths */);

		return Sequence({
			// Call the check context command
			vCodeLine,
			// Set <rule.state> to fail if context is not found
			Dictionary({
				{"$set", RuleState::CheckContextNotFound},
				{"$dst", ctx.resolveValue("@rule.state", false /* fRuntime */)},
				{"$if", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", "equal"},
					{"args", Sequence({
						Dictionary({
							{"$path", ctx.resolveValue("@rule.__contextExist", false /* fRuntime */)},
							{"$default", false}
							}),
						false
						})}
					})}
				})
			});
	}

	//
	//
	//
	Variant compileLoadContext(Context& ctx) const
	{
		Variant vCodeLine = Dictionary({
			{"clsid", CLSID_CallCtxCmd},
			{"command", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_Command},
				{"processor", "objects.contextService"},
				{"command", "load"}
				})},
			{"ctxParams", Dictionary({
				{"id", Dictionary({
					{"$val", Dictionary({
						{"$$proxy", "cachedCmd"},
						{"processor", "objects.contextService"},
						{"command", "getBasketId"},
						{"params", Dictionary({ {"name", m_sBasketName} })}
						})}
					})},
				})},
			{"$dst", ctx.resolveValue("@rule.__context", false /* fRuntime */)}
			});

		// Append <key>
		if (m_vKey.isSequenceLike())
			variant::putByPath(vCodeLine, "ctxParams.key", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_MakeSequenceCtxCmd},
				{"data", m_vKey},
				}),
				true /* fCreatePaths */);
		else
			variant::putByPath(vCodeLine, "ctxParams.key", m_vKey, true /* fCreatePaths */);

		// Append the optional parameters
		if (m_optDelete.has_value())
			variant::putByPath(vCodeLine, "ctxParams.delete", m_optDelete.value(), true /* fCreatePaths */);

		Variant vResultCode = Sequence({
			// Clear <context> field in the context
			//Dictionary({
			//	{"$set", {}},
			//	{"$dst", "context"}
			//	}),
			// Call the load context command
			vCodeLine,
			// Set <rule.state> to fail if no context loaded
			Dictionary({
				{"$set", RuleState::LoadContextNotFound},
				{"$dst", ctx.resolveValue("@rule.state", false /* fRuntime */)},
				{"$if", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", "equal"},
					{"args", Sequence({
						ctx.resolveValue("@rule.__context"),
						{}
						})}
					})}
				})
			});
		return vResultCode;
	}

public:
	LoadContextDirective() {}
	virtual ~LoadContextDirective() {}

	//
	//
	//
	static DirectivePtr create()
	{
		return std::make_shared<LoadContextDirective>();
	}

	//
	//
	//
	void parse(const Variant& vData, Context& ctx) override
	{
		TRACE_BEGIN;
		if (!vData.has("basket"))
			error::CompileError(SL, ctx.location, "Missing <basket> field").throwException();
		m_sBasketName = ctx.resolveValue(vData["basket"]);

		if (!vData.has("key"))
			error::CompileError(SL, ctx.location, "Missing <key> field").throwException();
		m_vKey = ctx.resolveValue(vData["key"]);

		if (vData.has("delete"))
			m_optDelete = ctx.resolveValue(vData["delete"]);
		TRACE_END("Failed to parse <loadContext> directive");
	}

	//
	//
	//
	Variant compile(Context& ctx) const override
	{
		TRACE_BEGIN;
		Variant vResultCode = Dictionary({
			{"peekContext", compilePeekContext(ctx)},
			{"loadContext", compileLoadContext(ctx)}
			});
		return vResultCode;
		TRACE_END("Failed to compile <loadContext> directive");
	}
};

//
//
//
class SaveContextDirective : public IDirective
{
private:
	std::string m_sBasketName;
	Variant m_vKey;
	Size m_nLifetime = 0;
	std::optional<Variant> m_optGroup;
	std::optional<Size> m_optMaxCopyCount;
	std::optional<Variant> m_optData;

public:
	SaveContextDirective() {}
	virtual ~SaveContextDirective() {}

	//
	//
	//
	static DirectivePtr create()
	{
		return std::make_shared<SaveContextDirective>();
	}

	//
	//
	//
	void parse(const Variant& vData, Context& ctx) override
	{
		TRACE_BEGIN;
		if (!vData.has("basket"))
			error::CompileError(SL, ctx.location, "Missing <basket> field").throwException();
		m_sBasketName = ctx.resolveValue(vData["basket"]);

		if (!vData.has("key"))
			error::CompileError(SL, ctx.location, "Missing <key> field").throwException();
		m_vKey = ctx.resolveValue(vData["key"]);

		if (!vData.has("lifetime"))
			error::CompileError(SL, ctx.location, "Missing <lifetime> field").throwException();
		m_nLifetime = ctx.resolveValue(vData["lifetime"]);

		if (vData.has("group"))
			m_optGroup = ctx.resolveValue(vData["group"]);
		if (vData.has("maxCopyCount"))
			m_optMaxCopyCount = ctx.resolveValue(vData["maxCopyCount"]);
		if (vData.has("data"))
			m_optData = ctx.resolveValue(ctx.normalizeDictSchema(vData["data"]));
		TRACE_END("Failed to parse <saveContext> directive");
	}

	//
	//
	//
	Variant compile([[maybe_unused]] Context& ctx) const override
	{
		TRACE_BEGIN;
		Variant vCodeLine = Dictionary({
			{"clsid", CLSID_CallCtxCmd},
			{"command", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_Command},
				{"processor", "objects.contextService"},
				{"command", "save"}
				})},
			{"ctxParams", Dictionary({
				{"id", Dictionary({
					{"$val", Dictionary({
						{"$$proxy", "cachedCmd"},
						{"processor", "objects.contextService"},
						{"command", "getBasketId"},
						{"params", Dictionary({ {"name", m_sBasketName} })}
						})}
					})},
				{"timeout", m_nLifetime}
				})},
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
			});

		// Append <key>
		if (m_vKey.isSequenceLike())
			variant::putByPath(vCodeLine, "ctxParams.key", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_MakeSequenceCtxCmd},
				{"data", m_vKey},
				}),
				true /* fCreatePaths */);
		else
			variant::putByPath(vCodeLine, "ctxParams.key", m_vKey, true /* fCreatePaths */);

		// Append the optional parameters
		if (m_optGroup.has_value())
		{
			// Append <group>
			if (m_optGroup.value().isSequenceLike())
				variant::putByPath(vCodeLine, "ctxParams.group", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_MakeSequenceCtxCmd},
					{"data", m_optGroup.value()},
					}),
					true /* fCreatePaths */);
			else
				variant::putByPath(vCodeLine, "ctxParams.group", m_optGroup.value(),
					true /* fCreatePaths */);
		}
		if (m_optMaxCopyCount.has_value())
			variant::putByPath(vCodeLine, "ctxParams.delete", m_optMaxCopyCount.value(),
				true /* fCreatePaths */);
		if (m_optData.has_value())
			variant::putByPath(vCodeLine, "ctxParams.data", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_MakeDictionaryCtxCmd},
				{"data", m_optData.value()},
				}),
				true /* fCreatePaths */);

		return Sequence({ vCodeLine });
		TRACE_END("Failed to compile <saveContext> directive");
	}
};

//
//
//
class DeleteContextDirective : public IDirective
{
public:
	std::optional<std::string> m_optBasketName;
	std::optional<Variant> m_optKey;
	std::optional<Variant> m_optGroup;

public:
	DeleteContextDirective() {}
	virtual ~DeleteContextDirective() {}

	//
	//
	//
	static DirectivePtr create()
	{
		return std::make_shared<DeleteContextDirective>();
	}

	//
	//
	//
	void parse(const Variant& vData, Context& ctx) override
	{
		TRACE_BEGIN;
		auto optBasketName = vData.getSafe("basket");
		if (!optBasketName.has_value())
			return;
		m_optBasketName = ctx.resolveValue(optBasketName.value());

		auto optKey = vData.getSafe("key");
		if (optKey.has_value())
		{
			m_optKey = ctx.resolveValue(optKey.value());
			return;
		}

		auto optGroup = vData.getSafe("group");
		if (optGroup.has_value())
		{
			m_optGroup = ctx.resolveValue(optGroup.value());
			return;
		}

		error::CompileError(SL, ctx.location,
			"One of <key> or <group> has to be specified").throwException();
		TRACE_END("Failed to parse <deleteContext> directive");
	}

	//
	//
	//
	Variant compile([[maybe_unused]] Context& ctx) const override
	{
		TRACE_BEGIN;
		Variant vCodeLine = Dictionary({
			{"clsid", CLSID_CallCtxCmd},
			{"command", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_Command},
				{"processor", "objects.contextService"},
				{"command", "remove"}
				})},
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
			});

		// Append the optional parameters
		if (m_optBasketName.has_value())
			variant::putByPath(vCodeLine, "ctxParams.id", Dictionary({
				{"$val", Dictionary({
					{"$$proxy", "cachedCmd"},
					{"processor", "objects.contextService"},
					{"command", "getBasketId"},
					{"params", Dictionary({ {"name", m_optBasketName.value()} })}
					})}
				}),
				true /* fCreatePaths */);

		if (m_optKey.has_value())
		{
			// Append <key>
			if (m_optKey.value().isSequenceLike())
				variant::putByPath(vCodeLine, "ctxParams.group", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_MakeSequenceCtxCmd},
					{"data", m_optKey.value()},
					}),
					true /* fCreatePaths */);
			else
				variant::putByPath(vCodeLine, "ctxParams.group", m_optKey.value(),
					true /* fCreatePaths */);
		}

		if (m_optGroup.has_value())
		{
			// Append <group>
			if (m_optGroup.value().isSequenceLike())
				variant::putByPath(vCodeLine, "ctxParams.group", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_MakeSequenceCtxCmd},
					{"data", m_optGroup.value()},
					}),
					true /* fCreatePaths */);
			else
				variant::putByPath(vCodeLine, "ctxParams.group", m_optGroup.value(),
					true /* fCreatePaths */);
		}

		return Sequence({ vCodeLine });
		TRACE_END("Failed to compile <deleteContext> directive");
	}
};

//
//
//
inline bool isOperation(const Variant vData)
{
	return (vData.isDictionaryLike() && vData.has(c_sOperationMarker));
}

//
//
//
Variant createOperation(Variant& vData, Context& ctx)
{
	if (!isOperation(vData))
		return vData;

	Operation operation = operation::getOperationFromString(vData[c_sOperationMarker]);

	if (vData.has("args"))
	{
		Variant vNewArgs = Sequence();
		for (auto vArg : vData["args"])
			vNewArgs.push_back(createOperation(vArg, ctx));
		vData.put("args", vNewArgs);
	}
	return createOperation(operation, vData, ctx);
}

//
//
//
class ConditionDirective : public IDirective
{
private:
	OperationPtr m_pCondition;
	bool m_fHasAccessToContext = false;

public:
	ConditionDirective() {}
	virtual ~ConditionDirective() {}

	//
	//
	//
	static DirectivePtr create()
	{
		return std::make_shared<ConditionDirective>();
	}

	//
	//
	//
	void parse(const Variant& vData, Context& ctx) override
	{
		TRACE_BEGIN;
		try
		{
			auto vTmp = vData.clone();
			m_pCondition = queryInterface<operation::IOperation>(createOperation(vTmp, ctx));
		}
		catch (const error::InvalidArgument& ex)
		{
			error::CompileError(SL, ctx.location, ex.what()).throwException();
		}
		TRACE_END("Failed to parse <condition> directive");
	}

	//
	//
	//
	bool hasAccessToContext() const override
	{
		return m_fHasAccessToContext;
	}

	//
	//
	//
	Variant compile(Context& ctx) const override
	{
		TRACE_BEGIN;
		return Sequence({ Dictionary({
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
			}) });
		TRACE_END("Failed to compile <condition> directive");
	}
};

//
//
//
class CreateEventDirective : public IDirective
{
private:
	Destination m_destination = Destination::Undefined;
	bool m_fClone = true;
	Variant m_vData;
	bool m_fHasAccessToContext = false;

	//
	//
	//
	bool checkAccessToContextValue(const Variant& vValue, Context& ctx) const
	{
		constexpr char c_sContextPattern[] = "policy.context";
		if (!ctx.isLink(vValue))
			return false;
		auto vTmp = ctx.resolveValue(vValue, false /* fRuntime */);
		if (!vTmp.isString())
			return false;
		if (((std::string_view)vTmp).compare(0, sizeof(c_sContextPattern) - 1, c_sContextPattern))
			return false;
		return true;
	}

	//
	//
	//
	bool checkAccessToContext(const Variant& vData, Context& ctx) const
	{
		TRACE_BEGIN;
		if (vData.isDictionaryLike())
		{
			// Check all values in dictionary
			for (const auto& vValue : vData)
			{
				if (checkAccessToContextValue(vValue, ctx))
					return true;
			}
			return false;
		}
		if (vData.isSequenceLike())
		{
			for (const auto& vItem : Sequence(vData))
			{
				auto vValue = vItem.get("value", "");
				if (checkAccessToContextValue(vValue, ctx))
					return true;
			}
			return false;
		}
		if (vData.isString() && checkAccessToContextValue(vData, ctx))
			return true;
		return false;
		TRACE_END("Failed to check access to @context");
	}

	//
	//
	//
	Variant prepareData(const Variant& vData, Context& ctx) const
	{
		auto vNormalized = ctx.normalizeDictSchema(vData);
		for (auto vItem : vNormalized)
		{
			std::string sName = vItem["name"];
			Variant vValue = vItem["value"];

			if (!isOperation(vValue))
			{
				vItem.put("value", ctx.resolveValue(vValue));
				continue;
			}

			try
			{
				auto vTmp = ctx.resolveValue(vValue);
				auto pOperation = queryInterface<operation::IOperation>(createOperation(vTmp, ctx));
				vItem.put("value", pOperation->compile(ctx));
			}
			catch (const error::InvalidArgument & ex)
			{
				error::CompileError(SL, ctx.location, ex.what()).throwException();
			}
		}
		return vNormalized;
	}

public:
	CreateEventDirective() {}
	virtual ~CreateEventDirective() {}

	//
	//
	//
	static DirectivePtr create()
	{
		return std::make_shared<CreateEventDirective>();
	}

	//
	//
	//
	void parse(const Variant& vData, Context& ctx) override
	{
		TRACE_BEGIN;
		if (!vData.has("eventType"))
			error::CompileError(SL, ctx.location, "Missing <eventType> field").throwException();
		Variant vEventType = ctx.resolveValue(vData["eventType"]);
		if (!vEventType.isString())
			error::CompileError(SL, ctx.location, "Event type must be a string").throwException();

		if (!vData.has("destination"))
			error::CompileError(SL, ctx.location, "Missing <destination> field").throwException();
		m_destination = getDestinationFromString(vData["destination"]);

		m_fClone = vData.get("clone", m_fClone);
		m_fHasAccessToContext = false;

		if (vData.has("data"))
		{
			m_vData = prepareData(vData["data"], ctx);
			m_fHasAccessToContext = checkAccessToContext(vData["data"], ctx);
		}
		else
			m_vData = Sequence();

		// Append eventType to data
		m_vData.push_back(Dictionary({
			{"name", "type"},
			{"value", vEventType}
			}));

		// TODO: @ermakov Append <time> and <tickTime> if aren't exist
		TRACE_END("Failed to parse <createEvent> directive");
	}

	//
	//
	//
	bool hasAccessToContext() const override
	{
		return m_fHasAccessToContext;
	}

	//
	//
	//
	Variant compile(Context& ctx) const override
	{
		TRACE_BEGIN;
		Variant vData = (m_fClone ?
			Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_VariantCtxCmd},
				{"operation", "merge"},
				{"flags", variant::MergeMode::All |	variant::MergeMode::CheckType |
					variant::MergeMode::MergeNestedDict | variant::MergeMode::MergeNestedSeq |
					variant::MergeMode::NonErasingNulls},
				{"args", Sequence({
					Dictionary({ {"$path", "event"} }),
					Dictionary({
						{"$$proxy", "cachedObj"},
						{"clsid", CLSID_MakeDictionaryCtxCmd},
						{"data", m_vData}
						})
					})}
				})
			:
			Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_MakeDictionaryCtxCmd},
				{"data", m_vData}
				})
			);

		if (m_fHasAccessToContext)
			// Call createEvent command with multiple context support
			return Sequence({ Dictionary({
				{"clsid", CLSID_ForEachCtxCmd},
				{"path", ctx.resolveValue("@context", false /* fRuntime */)},
				{"data", ctx.resolveValue("@rule.__context")},
				{"command", Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_CallCtxCmd},
					{"command", Dictionary({ {"$path", "eventHandler.call.createEvent"} })},
					{"ctxParams", Dictionary({
						{"destination", getDestinationString(m_destination)},
						{"data", vData}
						})},
					{"ctx", {}},	// Use blank context
					})},
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
				}) });
		// Call createEvent command without context
		return Sequence({ Dictionary({
			{"clsid", CLSID_CallCtxCmd},
			{"command", Dictionary({ {"$path", "eventHandler.call.createEvent"} })},
			{"ctxParams", Dictionary({
				{"destination", getDestinationString(m_destination)},
				{"data", vData}
				})},
			{"ctx", {}},	// Use blank context
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
			}) });
		TRACE_END("Failed to compile <createEvent> directive");
	}
};

//
//
//
class DiscardDirective : public IDirective
{
public:
	DiscardDirective() {}
	virtual ~DiscardDirective() {}

	//
	//
	//
	static DirectivePtr create()
	{
		return std::make_shared<DiscardDirective>();
	}

	//
	//
	//
	void parse([[maybe_unused]] const Variant& vData, [[maybe_unused]] Context& ctx) override
	{
	}

	//
	//
	//
	Variant compile([[maybe_unused]] Context& ctx) const override
	{
		TRACE_BEGIN;
		return Sequence({ Dictionary({
			{"$ret", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_MakeDictionaryCtxCmd},
				{"data", Dictionary({ {"discard", true} })}
				})},
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
			}) });
		TRACE_END("Failed to compile <discard> directive");
	}
};

//
// Creation factory for directives 
//
DirectivePtr createDirective(Directive directive)
{
	switch (directive)
	{
		case Directive::Condition: return ConditionDirective::create();
		case Directive::LoadContext: return LoadContextDirective::create();
		case Directive::SaveContext: return SaveContextDirective::create();
		case Directive::DeleteContext: return DeleteContextDirective::create();
		case Directive::CreateEvent: return CreateEventDirective::create();
		case Directive::Discard: return DiscardDirective::create();
	}
	error::OperationNotSupported(SL, FMT("Directive <" << getDirectiveString(directive) <<
		"> is not supported")).throwException();
}

//
//
//
RulePtr RuleImpl::create()
{
	return std::make_shared<RuleImpl>();
}

//
// List of supported directives
//
const std::vector<Directive> c_directiveList{
	Directive::Condition,
	Directive::LoadContext,
	Directive::DeleteContext,
	Directive::SaveContext,
	Directive::CreateEvent,
	Directive::Discard
};

//
//
//
void RuleImpl::parse(const Variant& vData, Context& ctx)
{
	TRACE_BEGIN;
	if (!vData.isDictionaryLike())
		error::CompileError(SL, ctx.location, "Rule must be a dictionary").throwException();

	nPriority = static_cast<Size>(vData.get("priority", ctx.pPolicy->nPriority));

	bool fHasAccessToContext = false;
	for (const auto directive : c_directiveList)
	{
		auto sDirective = getDirectiveString(directive);
		if (!vData.has(sDirective))
			continue;
		auto pDirective = createDirective(directive);
		pDirective->parse(vData[sDirective], ctx);
		if (!fHasAccessToContext)
			fHasAccessToContext = pDirective->hasAccessToContext();
		directives[directive] = pDirective;
	}

	if (fHasAccessToContext && (directives.find(Directive::LoadContext) == directives.end()))
		error::CompileError(SL, ctx.location,
			"Cannot use @context without <loadContext> directive").throwException();
	TRACE_END("Failed to parse a rule");
}

//
//
//
Variant RuleImpl::compile(Context& ctx)
{
	TRACE_BEGIN;
	// Clear a rule state (from previous rule execution)
	Variant vResultCode = Sequence({ Dictionary({
		{"$set", RuleState::Undefined},
		{"$dst", ctx.resolveValue("@rule.state", false /* fRuntime */)},
		}) });

	Variant vPeekContextCode;
	Variant vLoadContextCode;
	auto pLoadContext = directives.find(Directive::LoadContext);
	if (pLoadContext != directives.end())
	{
		auto vCode = pLoadContext->second->compile(ctx);
		vPeekContextCode = vCode["peekContext"];
		vLoadContextCode = vCode["loadContext"];
	}

	for (const auto& vCodeLine : Sequence(vPeekContextCode))
		vResultCode.push_back(vCodeLine);

	auto pCondition = directives.find(Directive::Condition);
	if (pCondition != directives.end())
	{
		for (const auto& vCodeLine : Sequence(pCondition->second->compile(ctx)))
			vResultCode.push_back(vCodeLine);
	}

	for (const auto& vCodeLine : Sequence(vLoadContextCode))
		vResultCode.push_back(vCodeLine);

	// Here the rule with undefined state turn into the matched one
	// Optimization: if the result code currently contain only ONE line
	// (state = Undefined), replace it with (state = Matched)
	if (vResultCode.getSize() == 1)
		vResultCode = Sequence({ Dictionary({
			{"$set", RuleState::Matched},
			{"$dst", ctx.resolveValue("@rule.state", false /* fRuntime */)},
			}) });
	else
		vResultCode.push_back(Dictionary({
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
			}));

	auto pDeleteContext = directives.find(Directive::DeleteContext);
	if (pDeleteContext != directives.end())
	{
		for (const auto& vCodeLine : Sequence(pDeleteContext->second->compile(ctx)))
			vResultCode.push_back(vCodeLine);
	}

	auto pSaveContext = directives.find(Directive::SaveContext);
	if (pSaveContext != directives.end())
	{
		for (const auto& vCodeLine : Sequence(pSaveContext->second->compile(ctx)))
			vResultCode.push_back(vCodeLine);
	}

	auto pCreateEvent = directives.find(Directive::CreateEvent);
	if (pCreateEvent != directives.end())
	{
		for (const auto& vCodeLine : Sequence(pCreateEvent->second->compile(ctx)))
			vResultCode.push_back(vCodeLine);
	}

	auto pDiscard = directives.find(Directive::Discard);
	if (pDiscard != directives.end())
	{
		for (const auto& vCodeLine : Sequence(pDiscard->second->compile(ctx)))
			vResultCode.push_back(vCodeLine);
	}

	return vResultCode;
	TRACE_END("Failed to compile a rule");
}

//
//
//
VariablePtr VariableImpl::create(std::string_view sName_)
{
	TRACE_BEGIN;
	auto pObj = std::make_shared<VariableImpl>();
	pObj->sName = sName_;
	return pObj;
	TRACE_END("Error during object creation");
}

//
//
//
void VariableImpl::parse(const Variant& vData, Context& ctx)
{
	TRACE_BEGIN;
	if (!isOperation(vData))
	{
		m_vData = ctx.resolveValue(vData);
		return;
	}

	try
	{
		auto vTmp = vData.clone();
		m_pOperation = queryInterface<operation::IOperation>(createOperation(vTmp, ctx));
	}
	catch (const error::InvalidArgument & ex)
	{
		error::CompileError(SL, ctx.location, ex.what()).throwException();
	}
	TRACE_END("Failed to parse variable");
}

//
//
//
Variant VariableImpl::compile(Context& ctx) const
{
	TRACE_BEGIN;
	std::string sPath("@var.");
	sPath += sName;

	if (m_pOperation)
		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_CachedValueCtxCmd},
			{"path", ctx.resolveValue(sPath, false /* fRuntime */)},
			{"value", m_pOperation->compile(ctx)}
			});

	if (m_vData.isSequenceLike())
		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_CachedValueCtxCmd},
			{"path", ctx.resolveValue(sPath, false /* fRuntime */)},
			{"value", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_MakeSequenceCtxCmd},
				{"data", m_vData}
				})}
			});

	return Dictionary({
		{"$$proxy", "cachedObj"},
		{"clsid", CLSID_CachedValueCtxCmd},
		{"path", ctx.resolveValue(sPath, false /* fRuntime */)},
		{"value", m_vData}
		});
	TRACE_END("Failed to compile variable");
}

//
//
//
ParserPtr Parser::create()
{
	return std::make_shared<Parser>();
}

//
//
//
void Parser::parseConstants(const Variant& vData, Context &ctx) const
{
	TRACE_BEGIN;
	if (!vData.isDictionaryLike())
		error::CompileError(SL, ctx.location, "Constants must be a dictionary").throwException();

	ctx.pPolicy->vConstants = vData.clone();
	while (!ctx.pPolicy->fAllLinksResolved)
	{
		ctx.pPolicy->fAllLinksResolved = true;
		parseConstDict(ctx.pPolicy->vConstants, ctx, "");
	};

	// Add constants to context
	if (!ctx.pPolicy->vConstants.isEmpty())
		ctx.vConstants.merge(ctx.pPolicy->vConstants, variant::MergeMode::All |
			variant::MergeMode::CheckType | variant::MergeMode::MergeNestedDict);
	TRACE_END("Failed to parse constants");
}

//
//
//
void Parser::parseConstDict(const Variant& vData, Context &ctx, const std::string& sPath) const
{
	for (const auto& [sKey, vValue] : Dictionary(vData))
	{
		std::string sFullPath(sPath);
		if (!sFullPath.empty())
			sFullPath += ".";
		sFullPath += sKey;

		if (vValue.isDictionaryLike())
			parseConstDict(vValue, ctx, sFullPath);
		else if (vValue.isSequenceLike())
			parseConstSeq(vValue, ctx, sFullPath);
		else if (ctx.isLink(vValue))
		{
			auto optValue = ctx.getValueByLink(vValue, false /* fRuntime */, true /* fRecursive */);
			if (optValue.has_value())
			{
				variant::putByPath(ctx.pPolicy->vConstants, sFullPath, optValue.value(),
					true /* fCreatePaths */);
				ctx.pPolicy->fAllLinksResolved = false;
			}
		}
	}
}

//
//
//
void Parser::parseConstSeq(const Variant& vData, Context& ctx, const std::string& sPath) const
{
	Size nIndex = 0;
	for (const auto& vValue : vData)
	{
		std::string sFullPath(sPath);
		sFullPath += "[";
		sFullPath += std::to_string(nIndex);
		sFullPath += "]";
		++nIndex;

		if (vValue.isDictionaryLike())
			parseConstDict(vValue, ctx, sFullPath);
		else if (vValue.isSequenceLike())
			parseConstSeq(vValue, ctx, sFullPath);
		else if (ctx.isLink(vValue))
		{
			auto optValue = ctx.getValueByLink(vValue, false /* fRuntime */, true /* fRecursive */);
			if (optValue.has_value())
			{
				variant::putByPath(ctx.pPolicy->vConstants, sFullPath, optValue.value(),
					true /* fCreatePaths */);
				ctx.pPolicy->fAllLinksResolved = false;
			}
		}
	}
}

//
//
//
void Parser::parseVariables(const Variant& vData, Context& ctx) const
{
	TRACE_BEGIN;
	if (!vData.isDictionaryLike())
		error::CompileError(SL, ctx.location, "Variables section must be a dictionary").throwException();

	Variant vVariables = Dictionary();
	for (const auto& [sName, vVariableDescr] : Dictionary(vData))
	{
		ctx.location.enterVariable(sName);
		auto pVariable = VariableImpl::create(sName);
		pVariable->parse(vVariableDescr, ctx);
		ctx.addVariable(pVariable);
	}	
	ctx.location.leaveVariable();
	TRACE_END("Failed to parse variables");
}

//
//
//
void Parser::parseChains(const Variant& vData, Context& ctx) const
{
	TRACE_BEGIN;
	if (!vData.isDictionaryLike())
		error::CompileError(SL, ctx.location, "Chains section must be a dictionary").throwException();

	for (const auto& [sName, vChainDescr] : Dictionary(vData))
	{
		ctx.location.enterChain(sName);
		auto pChain = Chain::create(sName);
		if (vChainDescr.isDictionaryLike())
			pChain->addRule(parseRule(vChainDescr, ctx));
		else
		{
			for (const auto& vRuleDescr : vChainDescr)
				pChain->addRule(parseRule(vRuleDescr, ctx));
		}
		ctx.addChain(pChain);
	}
	ctx.location.leaveChain();
	TRACE_END("Failed to parse chains");
}

//
//
//
void Parser::parseBindings(const Variant& vData, Context& ctx) const
{
	TRACE_BEGIN;
	if (!vData.isSequenceLike())
		error::CompileError(SL, ctx.location, "Bindings section must be a sequence").throwException();

	for (const auto& vBindingDescr : vData)
	{
		std::vector<ChainPtr> chains;

		ctx.location.enterBinding();

		Variant vEventTypes = ctx.resolveValue(vBindingDescr["eventType"], false /* fRuntime */);
		if (!vEventTypes.isSequenceLike())
			vEventTypes = Sequence({ vEventTypes });

		auto vChainNames = vBindingDescr["rules"];

		for (const std::string& sEventType : vEventTypes)
		{
			if (vChainNames.isString())
			{
				auto pChain = ctx.getChain(ctx.resolveValue(vChainNames, false /* fRuntime */));
				if (!pChain)
					error::CompileError(SL, ctx.location,
						FMT("Chain <" << vChainNames << "> is not found")).throwException();
				ctx.addBinding(sEventType, pChain);
				continue;
			}

			if (vChainNames.isSequenceLike())
			{
				for (const auto& vName : vChainNames)
				{
					std::string sName = ctx.resolveValue(vName, false /* fRuntime */);
					auto pChain = ctx.getChain(sName);
					if (!pChain)
						error::CompileError(SL, ctx.location,
							FMT("Chain <" << sName << "> is not found")).throwException();
					ctx.addBinding(sEventType, pChain);
				}
			}
		}
	}
	TRACE_END("Failed to parse bindings");
}

//
//
//
void Parser::parsePatterns(const Variant& vData, Context& ctx) const
{
	TRACE_BEGIN;
	if (!vData.isDictionaryLike())
		error::CompileError(SL, ctx.location, "Patterns section must be a dictionary").throwException();

	for (const auto& [sName, vPattern] : Dictionary(vData))
	{
		ctx.location.enterPattern(sName);
		if (!vPattern.isSequenceLike())
			error::CompileError(SL, ctx.location, "Pattern must be a sequence").throwException();

		auto pPattern = Pattern::create(sName);
		for (const auto& vBinding : vPattern)
		{
			if (!vBinding.isDictionaryLike())
				error::CompileError(SL, ctx.location,
					"Pattern binding must be a dictionary").throwException();

			pPattern->addRule(ctx.resolveValue(vBinding["eventType"]), parseRule(vBinding["rule"], ctx));
		}
		ctx.addPattern(pPattern);
	}
	ctx.location.leavePattern();
	TRACE_END("Failed to parse patterns");
}

//
//
//
RulePtr Parser::parseRule(const Variant& vData, Context& ctx) const
{
	ctx.location.enterRule();
	auto pRule = RuleImpl::create();
	pRule->parse(vData, ctx);
	return pRule;
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

		if (!pPolicy->vData.isDictionaryLike())
			error::CompileError(SL, ctx.location, "Policy must be a dictionary").throwException();
		if (!pPolicy->vData.has("name") || ((std::string_view)pPolicy->vData["name"]).empty())
			error::CompileError(SL, ctx.location, "Missing or empty field <name>").throwException();

		if (!pPolicy->vData.has("version"))
			error::CompileError(SL, ctx.location, "Missing field <version>").throwException();
		if (pPolicy->vData["version"] != c_nVersion)
			error::CompileError(SL, ctx.location, FMT("Invalid policy version <" <<
				pPolicy->vData["version"] << "<")).throwException();

		if (pPolicy->vData.has("constants"))
			parseConstants(pPolicy->vData["constants"], ctx);

		if (pPolicy->vData.has("variables"))
			parseVariables(pPolicy->vData["variables"], ctx);

		if (pPolicy->vData.has("chains"))
			parseChains(pPolicy->vData["chains"], ctx);

		if (pPolicy->vData.has("bindings"))
			parseBindings(pPolicy->vData["bindings"], ctx);

		if (pPolicy->vData.has("patterns"))
			parsePatterns(pPolicy->vData["patterns"], ctx);

		// Mark the policy as successfully parsed
		pPolicy->fParsed = true;
	}
	CMD_PREPARE_CATCH
	CMD_CATCH_COMPILE(ctx.location);
}

} // namespace v2
} // namespace policy
} // namespace edr
} // namespace openEdr

/// @}
