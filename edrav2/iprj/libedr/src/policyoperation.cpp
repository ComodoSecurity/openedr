//
// edrav2.libedr project
// 
// Author: Yury Ermakov (20.08.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
///
/// @file Policy operations implementation
///
/// @addtogroup edr
/// @{
#include "pch.h"
#include "policyoperation.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "policyop"

namespace cmd {
namespace edr {
namespace policy {
namespace operation {

constexpr char c_sNot[] = "not";
constexpr char c_sAnd[] = "and";
constexpr char c_sOr[] = "or";
constexpr char c_sEqual[] = "equal";
constexpr char c_sNotEqual[] = "!equal";
constexpr char c_sGreater[] = "greater";
constexpr char c_sLess[] = "less";
constexpr char c_sMatch[] = "match";
constexpr char c_sIMatch[] = "imatch";
constexpr char c_sNotMatch[] = "!match";
constexpr char c_sNotIMatch[] = "!imatch";
constexpr char c_sContain[] = "contain";
constexpr char c_sNotContain[] = "!contain";
constexpr char c_sAdd[] = "add";
constexpr char c_sExtract[] = "extract";
constexpr char c_sHas[] = "has";
constexpr char c_sNotHas[] = "!has";
constexpr char c_sMakeDescriptor[] = "makeDescriptor";

//
//
//
const char* getOperationString(Operation operation)
{
	switch (operation)
	{
		case Operation::Not: return c_sNot;
		case Operation::And: return c_sAnd;
		case Operation::Or: return c_sOr;
		case Operation::Equal: return c_sEqual;
		case Operation::NotEqual: return c_sNotEqual;
		case Operation::Greater: return c_sGreater;
		case Operation::Less: return c_sLess;
		case Operation::Match: return c_sMatch;
		case Operation::NotMatch: return c_sNotMatch;
		case Operation::IMatch: return c_sIMatch;
		case Operation::NotIMatch: return c_sNotIMatch;
		case Operation::Contain: return c_sContain;
		case Operation::NotContain: return c_sNotContain;
		case Operation::Add: return c_sAdd;
		case Operation::Extract: return c_sExtract;
		case Operation::Has: return c_sHas;
		case Operation::NotHas: return c_sNotHas;
		case Operation::MakeDescriptor: return c_sMakeDescriptor;
	}
	error::InvalidArgument(SL, FMT("Operation <" << +(std::uint8_t)operation << 
		"> has no text string")).throwException();
}

//
//
//
Operation getOperationFromString(std::string_view sOperation)
{
	if (sOperation == c_sNot) return Operation::Not;
	if (sOperation == c_sAnd) return Operation::And;
	if (sOperation == c_sOr) return Operation::Or;
	if (sOperation == c_sEqual) return Operation::Equal;
	if (sOperation == c_sNotEqual) return Operation::NotEqual;
	if (sOperation == c_sGreater) return Operation::Greater;
	if (sOperation == c_sLess) return Operation::Less;
	if (sOperation == c_sMatch) return Operation::Match;
	if (sOperation == c_sIMatch) return Operation::IMatch;
	if (sOperation == c_sNotMatch) return Operation::NotMatch;
	if (sOperation == c_sNotIMatch) return Operation::NotIMatch;
	if (sOperation == c_sContain) return Operation::Contain;
	if (sOperation == c_sNotContain) return Operation::NotContain;
	if (sOperation == c_sAdd) return Operation::Add;
	if (sOperation == c_sExtract) return Operation::Extract;
	if (sOperation == c_sHas) return Operation::Has;
	if (sOperation == c_sNotHas) return Operation::NotHas;
	if (sOperation == c_sMakeDescriptor) return Operation::MakeDescriptor;
	error::InvalidArgument(SL, FMT("Unknown operation <" << sOperation << ">")).throwException();
}

constexpr char c_sFile[] = "FILE";

/// 
/// Enum class for descriptor types.
///
enum class DescriptorType : std::uint8_t
{
	File = 0,			///< File descriptor
	Unknown = UINT8_MAX
};

//
//
//
const char* getDescriptorTypeString(DescriptorType type)
{
	switch (type)
	{
		case DescriptorType::File: return c_sFile;
	}
	error::InvalidArgument(SL, FMT("Descriptor type <" << +(std::uint8_t)type << 
		"> has no text string")).throwException();
}

//
//
//
DescriptorType getDescriptorTypeFromString(std::string_view sType)
{
	if (sType == c_sFile) return DescriptorType::File;
	error::InvalidArgument(SL, FMT("Unknown descriptor type <" << sType << ">")).throwException();
}


//
//
//
class OperationBase : public ObjectBase<>,
	public IOperation
{
private:
	Operation m_operation = Operation::Unknown;

protected:
	std::vector<Variant> m_args;
	Size m_nArgsLimit;

	//
	//
	//
	void initArgs(Variant vArgs)
	{
		if (!vArgs.isSequenceLike())
			error::InvalidArgument(SL, FMT("Operation <" << operation::getOperationString(m_operation) <<
				"> has non-sequence arguments")).throwException();
		if ((m_nArgsLimit != c_nMaxSize) && (m_nArgsLimit != vArgs.getSize()))
			error::InvalidArgument(SL, FMT("Operation <" << operation::getOperationString(m_operation) <<
				"> has wrong number of arguments (received: " << vArgs.getSize() << " expected: " <<
				m_nArgsLimit << ")")).throwException();

		m_args.reserve(vArgs.getSize());
		for (const auto& vArg : vArgs)
			m_args.push_back(vArg);
	}

	//
	//
	//
	Variant compileArgs(Context& ctx) const
	{
		Variant vResult = Sequence();
		for (const auto& vArg : m_args)
		{
			auto pOperation = queryInterfaceSafe<IOperation>(vArg);
			if (pOperation)
			{
				vResult.push_back(pOperation->compile(ctx));
				continue;
			}
			vResult.push_back(vArg);
		}
		return vResult;
	}

public:
	OperationBase(Operation operation, Size nArgsLimit = c_nMaxSize) :
		m_operation(operation), m_nArgsLimit(nArgsLimit)
	{}
};

//
//
//
class CtxOperation : public OperationBase
{
protected:
	std::string m_sCtxOperation;

public:
	CtxOperation(Operation operation, std::string_view sCtxOperation, Size nArgsLimit = c_nMaxSize) :
		OperationBase(operation, nArgsLimit),
		m_sCtxOperation(sCtxOperation)
	{}

	//
	//
	//
	Variant compile(Context& ctx) const override
	{
		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", m_sCtxOperation},
			{"args", compileArgs(ctx)}
			});
	}
};

//
//
//
class Not : public CtxOperation
{
public:
	Not() : CtxOperation(Operation::Not, "not", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Not>();
		pObj->initArgs(vArgs);
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class And : public CtxOperation
{
public:
	And() : CtxOperation(Operation::And, "and") {}

	//
	//
	//
	static OperationPtr create(Variant vArgs)
	{
		TRACE_BEGIN;
		auto pObj = createObject<And>();
		pObj->initArgs(vArgs);
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class Or : public CtxOperation
{
public:
	Or() : CtxOperation(Operation::Or, "or") {}

	//
	//
	//
	static OperationPtr create(Variant vArgs)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Or>();
		pObj->initArgs(vArgs);
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class Equal : public CtxOperation
{
public:
	Equal() : CtxOperation(Operation::Equal, "equal", 2) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Equal>();
		pObj->initArgs(vArgs);
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class NotEqual : public CtxOperation
{
public:
	NotEqual() : CtxOperation(Operation::NotEqual, "!equal", 2) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs)
	{
		TRACE_BEGIN;
		auto pObj = createObject<NotEqual>();
		pObj->initArgs(vArgs);
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class Greater : public CtxOperation
{
public:
	Greater() : CtxOperation(Operation::Greater, "greater", 2) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Greater>();
		pObj->initArgs(vArgs);
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class Less : public CtxOperation
{
public:
	Less() : CtxOperation(Operation::Less, "less", 2) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Less>();
		pObj->initArgs(vArgs);
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class PatternBasedOperation : public CtxOperation
{
protected:
	Variant m_vPattern;

	//
	//
	//
	Variant parsePattern(std::string_view sValue, std::string_view sDefaultMode = "") const
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
			sNewValue = sValue;

		if (sNewValue.find_first_of("*") == sValue.npos)
		{
			if (sMode.empty())
			{
				if (sDefaultMode.empty())
					return {};
				return Dictionary({
					{"target", sValue},
					{"mode", sDefaultMode}
					});
			}
			return Dictionary({
				{"target", sNewValue},
				{"mode", sMode}
				});
		}

		return string::convertWildcardToRegex(sValue);
	}

public:
	PatternBasedOperation(Operation operation, std::string_view sCtxOperation,
		Size nArgsLimit = c_nMaxSize) :
		CtxOperation(operation, sCtxOperation, nArgsLimit) {}

	//
	//
	//
	Variant compile(Context& ctx) const override
	{
		if (m_vPattern.isSequenceLike())
		{
			Variant vStringMatcherRules = Sequence();
			Variant vRegexRules = Sequence();
			for (const auto& vItem : m_vPattern)
			{
				if (!vItem.isString())
					error::CompileError(SL, ctx.location,
						"Pattern must be string or sequence of strings").throwException();

				auto vPattern = parsePattern(vItem, "begin");
				if (vPattern.isString())
					vRegexRules.push_back(vPattern);
				else
					vStringMatcherRules.push_back(vPattern);
			}

			Variant vObjects = Sequence();
			if (!vStringMatcherRules.isEmpty())
				vObjects.push_back(Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", m_sCtxOperation},
					{"pattern", Dictionary({
						{"$$proxy", "cachedObj"},
						{"clsid", CLSID_StringMatcher},
						{"schema", vStringMatcherRules}
						})},
					{"args", compileArgs(ctx)}
					}));
			if (!vRegexRules.isEmpty())
				vObjects.push_back(Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", m_sCtxOperation},
					{"pattern", vRegexRules},
					{"args", compileArgs(ctx)}
					}));

			if (vObjects.getSize() == 1)
				return vObjects[0];

			Operation operation = ((m_sCtxOperation.front() == '!') ? Operation::And : Operation::Or);
			return Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_VariantCtxCmd},
				{"operation", operation},
				{"args", vObjects}
				});
		}
		
		if (!m_vPattern.isString())
			error::CompileError(SL, ctx.location,
				"Pattern must be string or sequence of strings").throwException();

		Variant vPattern = parsePattern(m_vPattern);
		if (vPattern.isNull())
		{
			// If the pattern has no wildcards, convert <match> operation to <equal>
			Variant vArgs = Sequence({ m_vPattern });
			for (const auto& vArg : m_args)
				vArgs.push_back(vArg);
			auto operation = (m_sCtxOperation.front() == '!') ? Operation::NotEqual : Operation::Equal;
			auto pEqual = createOperation(operation, Dictionary({ {"args", vArgs} }), ctx);
			return pEqual->compile(ctx);
		}
		
		if (vPattern.isDictionaryLike())
			vPattern = Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_StringMatcher},
				{"schema", vPattern}
				});

		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", m_sCtxOperation},
			{"pattern", vPattern},
			{"args", compileArgs(ctx)}
			});
	}
};

//
//
//
class Match : public PatternBasedOperation
{
public:
	Match() : PatternBasedOperation(Operation::Match, "match", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs, Variant vPattern)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Match>();
		pObj->initArgs(vArgs);
		pObj->m_vPattern = vPattern;
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class NotMatch : public PatternBasedOperation
{
public:
	NotMatch() : PatternBasedOperation(Operation::NotMatch, "!match", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs, Variant vPattern)
	{
		TRACE_BEGIN;
		auto pObj = createObject<NotMatch>();
		pObj->initArgs(vArgs);
		pObj->m_vPattern = vPattern;
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class IMatch : public PatternBasedOperation
{
public:
	IMatch() : PatternBasedOperation(Operation::IMatch, "imatch", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs, Variant vPattern)
	{
		TRACE_BEGIN;
		auto pObj = createObject<IMatch>();
		pObj->initArgs(vArgs);
		pObj->m_vPattern = vPattern;
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class NotIMatch : public PatternBasedOperation
{
public:
	NotIMatch() : PatternBasedOperation(Operation::NotIMatch, "!imatch", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs, Variant vPattern)
	{
		TRACE_BEGIN;
		auto pObj = createObject<NotIMatch>();
		pObj->initArgs(vArgs);
		pObj->m_vPattern = vPattern;
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class Contain : public CtxOperation
{
private:
	Variant m_vItem;

public:
	Contain() : CtxOperation(Operation::Contain, "contain", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs, Variant vItem)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Contain>();
		pObj->initArgs(vArgs);
		pObj->m_vItem = vItem;
		return pObj;
		TRACE_END("Error during object creation");
	}

	//
	//
	//
	Variant compile(Context& ctx) const override
	{
		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", m_sCtxOperation},
			{"item", m_vItem},
			{"args", compileArgs(ctx)}
			});
	}
};

//
//
//
class NotContain : public CtxOperation
{
private:
	Variant m_vItem;

public:
	NotContain() : CtxOperation(Operation::NotContain, "contain", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs, Variant vItem)
	{
		TRACE_BEGIN;
		auto pObj = createObject<NotContain>();
		pObj->initArgs(vArgs);
		pObj->m_vItem = vItem;
		return pObj;
		TRACE_END("Error during object creation");
	}

	//
	//
	//
	Variant compile(Context& ctx) const override
	{
		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", "not"},
			{"args", Sequence({
				Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", m_sCtxOperation},
					{"item", m_vItem},
					{"args", compileArgs(ctx)}
					})
				})}
			});
	}
};

//
//
//
class Add : public CtxOperation
{
public:
	Add() : CtxOperation(Operation::Add, "add", 2) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Add>();
		pObj->initArgs(vArgs);
		return pObj;
		TRACE_END("Error during object creation");
	}
};



//
//
//
class PathBasedOperation : public CtxOperation
{
protected:
	std::string m_sPath;

public:
	PathBasedOperation(Operation operation, std::string_view sCtxOperation,
		Size nArgsLimit = c_nMaxSize) :
		CtxOperation(operation, sCtxOperation, nArgsLimit) {}

	//
	//
	//
	Variant compile(Context& ctx) const override
	{
		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", m_sCtxOperation},
			{"path", m_sPath},
			{"args", compileArgs(ctx)}
			});
	}
};

//
//
//
class Extract : public PathBasedOperation
{
public:
	Extract() : PathBasedOperation(Operation::Extract, "extract", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs, std::string_view sPath)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Extract>();
		pObj->initArgs(vArgs);
		pObj->m_sPath = sPath;
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class Has : public PathBasedOperation
{
public:
	Has() : PathBasedOperation(Operation::Has, "has", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs, std::string_view sPath)
	{
		TRACE_BEGIN;
		auto pObj = createObject<Has>();
		pObj->initArgs(vArgs);
		pObj->m_sPath = sPath;
		return pObj;
		TRACE_END("Error during object creation");
	}
};

//
//
//
class NotHas : public PathBasedOperation
{
public:
	NotHas() : PathBasedOperation(Operation::NotHas, "has", 1) {}

	//
	//
	//
	static OperationPtr create(Variant vArgs, std::string_view sPath)
	{
		TRACE_BEGIN;
		auto pObj = createObject<NotHas>();
		pObj->initArgs(vArgs);
		pObj->m_sPath = sPath;
		return pObj;
		TRACE_END("Error during object creation");
	}

	//
	//
	//
	Variant compile(Context& ctx) const override
	{
		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_VariantCtxCmd},
			{"operation", "not"},
			{"args", Sequence({
				Dictionary({
					{"$$proxy", "cachedObj"},
					{"clsid", CLSID_VariantCtxCmd},
					{"operation", m_sCtxOperation},
					{"path", m_sPath},
					{"args", compileArgs(ctx)}
					})
				})}
			});
	}
};

//
//
//
class MakeDescriptor : public OperationBase
{
protected:
	DescriptorType m_type;

public:
	MakeDescriptor(DescriptorType type) :
		OperationBase(Operation::MakeDescriptor),
		m_type(type)
	{}
};

//
//
//
class MakeFileDescriptor : public MakeDescriptor
{
protected:
	Variant m_vPath;

public:
	MakeFileDescriptor() : MakeDescriptor(DescriptorType::File) {}

	//
	//
	//
	static OperationPtr create(Variant vPath)
	{
		TRACE_BEGIN;
		auto pObj = createObject<MakeFileDescriptor>();
		pObj->m_vPath = vPath;
		return pObj;
		TRACE_END("Error during object creation");
	}

	//
	//
	//
	Variant compile([[maybe_unused]] Context& ctx) const override
	{
		return Dictionary({
			{"$$proxy", "cachedObj"},
			{"clsid", CLSID_CallCtxCmd},
			{"command", Dictionary({
				{"$$proxy", "cachedObj"},
				{"clsid", CLSID_Command},
				{"processor", "objects.fileDataProvider"},
				{"command", "getFileInfo"}
				})},
			{"ctxParams", Dictionary({ {"path", m_vPath} })}
			});
	}
};

} // namespace operation

//
//
//
OperationPtr createOperation(Operation operation, Variant vParams, Context& ctx)
{
	switch (operation)
	{
		case Operation::Not:
			return operation::Not::create(ctx.resolveValue(vParams["args"]));
		case Operation::And:
			return operation::And::create(ctx.resolveValue(vParams["args"]));
		case Operation::Or:
			return operation::Or::create(ctx.resolveValue(vParams["args"]));
		case Operation::Equal:
			return operation::Equal::create(ctx.resolveValue(vParams["args"]));
		case Operation::NotEqual:
			return operation::NotEqual::create(ctx.resolveValue(vParams["args"]));
		case Operation::Greater:
			return operation::Greater::create(ctx.resolveValue(vParams["args"]));
		case Operation::Less:
			return operation::Less::create(ctx.resolveValue(vParams["args"]));
		case Operation::Match:
			return operation::Match::create(ctx.resolveValue(vParams["args"]),
				ctx.resolveValue(vParams["pattern"], false /* fRuntime */));
		case Operation::NotMatch:
			return operation::NotMatch::create(ctx.resolveValue(vParams["args"]),
				ctx.resolveValue(vParams["pattern"], false /* fRuntime */));
		case Operation::IMatch:
			return operation::IMatch::create(ctx.resolveValue(vParams["args"]),
				ctx.resolveValue(vParams["pattern"], false /* fRuntime */));
		case Operation::NotIMatch:
			return operation::NotIMatch::create(ctx.resolveValue(vParams["args"]),
				ctx.resolveValue(vParams["pattern"], false /* fRuntime */));
		case Operation::Contain:
			return operation::Contain::create(ctx.resolveValue(vParams["args"]),
				ctx.resolveValue(vParams["item"]));
		case Operation::NotContain:
			return operation::NotContain::create(ctx.resolveValue(vParams["args"]),
				ctx.resolveValue(vParams["item"]));
		case Operation::Add:
			return operation::Add::create(ctx.resolveValue(vParams["args"]));
		case Operation::Extract:
			return operation::Extract::create(ctx.resolveValue(vParams["args"]),
				ctx.resolveValue(vParams["path"]));
		case Operation::Has:
			return operation::Has::create(ctx.resolveValue(vParams["args"]),
				ctx.resolveValue(vParams["path"]));
		case Operation::NotHas:
			return operation::NotHas::create(ctx.resolveValue(vParams["args"]),
				ctx.resolveValue(vParams["path"]));
		case Operation::MakeDescriptor:
		{
			switch (operation::getDescriptorTypeFromString(vParams["type"]))
			{
				case operation::DescriptorType::File:
					return operation::MakeFileDescriptor::create(ctx.resolveValue(vParams["path"]));
			}
			break;
		}
	}
	error::InvalidArgument(SL, FMT("Operation <" << operation::getOperationString(operation) <<
		"> is not supported")).throwException();
}

} // namespace policy 
} // namespace edr
} // namespace cmd 

/// @} 
