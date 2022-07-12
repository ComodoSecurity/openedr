//
// edrav2.libcore project
//
// Basic context aware commands
//
// Author: Yury Podpruzhnikov (03.02.2019)
// Reviewer: Denis Bogdanov (14.03.2019)
//
#include "pch.h"
#include "command.h"
#include "variantcmd.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "ctxcmd"

namespace cmd {

// Internal namespace to avoid names errors
namespace ctxoper {

constexpr char c_sNot[] = "not";
constexpr char c_sEqual[] = "equal";
constexpr char c_sNotEqual[] = "!equal";
constexpr char c_sAnd[] = "and";
constexpr char c_sOr[] = "or";
constexpr char c_sGreater[] = "greater";
constexpr char c_sLess[] = "less";
constexpr char c_sMatch[] = "match";
constexpr char c_sIMatch[] = "imatch";
constexpr char c_sNotMatch[] = "!match";
constexpr char c_sNotIMatch[] = "!imatch";
constexpr char c_sContain[] = "contain";
constexpr char c_sAdd[] = "add";
constexpr char c_sExtract[] = "extract";
constexpr char c_sTransform[] = "transform";
constexpr char c_sEmplaceTransform[] = "transform=";
constexpr char c_sEmplaceAdd[] = "add=";
constexpr char c_sMerge[] = "merge";
constexpr char c_sEmplaceMerge[] = "merge=";
constexpr char c_sFilter[] = "filter";
constexpr char c_sHas[] = "has";
constexpr char c_sNotHas[] = "!has";
constexpr char c_sClone[] = "clone";
constexpr char c_sLogWarning[] = "logwrn";
constexpr char c_sLogInfo[] = "loginf";

constexpr char c_sReplace[] = "replace";



///
/// Get operation name string by given code
///
/// @param operation a code of the operation
/// @return Returns a string with a name of the operation corresponding to a given code.
///
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
		case Operation::IMatch: return c_sIMatch;
		case Operation::NotMatch: return c_sNotMatch;
		case Operation::NotIMatch: return c_sNotIMatch;
		case Operation::Contain: return c_sContain;
		case Operation::Add: return c_sAdd;
		case Operation::Extract: return c_sExtract;
		case Operation::Transform: return c_sTransform;
		case Operation::EmplaceTransform: return c_sEmplaceTransform;
		case Operation::EmplaceAdd: return c_sEmplaceAdd;
		case Operation::Replace: return c_sReplace;
		case Operation::Merge: return c_sMerge;
		case Operation::EmplaceMerge: return c_sEmplaceMerge;
		case Operation::Filter: return c_sFilter;
		case Operation::Has: return c_sHas;
		case Operation::NotHas: return c_sNotHas;
		case Operation::Clone: return c_sClone;
		case Operation::LogWarning: return c_sLogWarning;
		case Operation::LogInfo: return c_sLogInfo;
	}
	error::InvalidArgument(SL, FMT("Variant operation <"
		<< +(std::uint8_t)operation << "> has no text string")).throwException();
}

///
/// Get operation code by given name
///
/// @param sOperation a name of the operation
/// @return Returns a code of the operation corresponding to a given name.
///
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
	if (sOperation == c_sAdd) return Operation::Add;
	if (sOperation == c_sExtract) return Operation::Extract;
	if (sOperation == c_sTransform) return Operation::Transform;
	if (sOperation == c_sReplace) return Operation::Replace;
	if (sOperation == c_sEmplaceTransform) return Operation::EmplaceTransform;
	if (sOperation == c_sEmplaceAdd) return Operation::EmplaceAdd;
	if (sOperation == c_sMerge) return Operation::Merge;
	if (sOperation == c_sEmplaceMerge) return Operation::EmplaceMerge;
	if (sOperation == c_sFilter) return Operation::Filter;
	if (sOperation == c_sHas) return Operation::Has;
	if (sOperation == c_sNotHas) return Operation::NotHas;
	if (sOperation == c_sClone) return Operation::Clone;
	if (sOperation == c_sLogWarning) return Operation::LogWarning;
	if (sOperation == c_sLogInfo) return Operation::LogInfo;
	error::InvalidArgument(SL, FMT("Unknown Variant operation - <"
		<< sOperation << ">")).throwException();
}

//
// OperationAdapter provide param parse and provide operation arguments
// Main target of its architecture is performance. For example minimization 
// virtual calls
//
template<typename OperationExecutor>
class BasicOperation: public IOperation
{
private:
	OperationExecutor m_opExecutor;
	Operation m_eOperation;
	bool m_fDebug = false;

	typedef std::vector<ContextAwareValue> ContextAwareValueVector;
	ContextAwareValueVector m_cavArgs;

	void initArgs(const Variant& vArgs)
	{
		m_cavArgs.reserve(vArgs.getSize());
		for (auto vItem : vArgs)
			m_cavArgs.emplace_back(std::forward<Variant>(vItem) );
	}

public:
	 
	//
	//
	//
	BasicOperation(Operation eOperation, const Variant& vCfg, Size nArgCount,
		OperationExecutor&& executor) : m_opExecutor(std::move(executor))
	{
		Variant vArgs = Sequence(vCfg.get("args", {}));
		if (vArgs.getSize() != nArgCount)
			error::InvalidArgument(SL, FMT("Operation <" << getOperationString(eOperation) <<
				"> expects <" << nArgCount << "> argument(-s).")).throwException();

		m_fDebug = vCfg.get("$debug", m_fDebug);
		initArgs(vArgs);
		m_opExecutor.init(eOperation, vCfg);
	}

	//
	//
	//
	BasicOperation(Operation eOperation, const Variant& vCfg, Size nMinArgCount, Size nMaxArgCount,
		OperationExecutor&& executor) : m_opExecutor(std::move(executor))
	{
		Variant vArgs = Sequence(vCfg.get("args", {}));
		Size nArgsCount = vArgs.getSize();
		if (nArgsCount < nMinArgCount || nArgsCount > nMaxArgCount)
			error::InvalidArgument(SL, FMT("Operation <" << getOperationString(eOperation) <<
				"> expects at least <" << nMinArgCount << "> argument(-s).")).throwException();

		initArgs(vArgs);
		m_opExecutor.init(eOperation, vCfg);
	}

	//
	//
	//
	Variant execute(const Variant& vContext) override
	{
		auto fnGetArg = [&vContext, this](Size nIndex)
		{
			return this->m_cavArgs[nIndex].get(vContext);
		};

		if (m_fDebug)
			DebugBreak();
		return m_opExecutor.execute(vContext, fnGetArg, m_cavArgs.size());
	}
};


//
// OperationAdapterN - operation without params
//
template <typename FnOperation>
class NoParam
{
	FnOperation m_fnOperation;
public:

	//
	//
	//
	NoParam(FnOperation&& fnOperation) : m_fnOperation(std::move(fnOperation))
	{
	}

	//
	// Empty init because no parameters
	//
	void init(Operation /*eOperation*/, const Variant& /*vCfg*/) {}

	//
	// Call m_fnOperation
	//
	template<typename FnGetArg>
	Variant execute(const Variant& /*vCtx*/, FnGetArg&& fnGetArg, size_t nArgCount)
	{
		return m_fnOperation(std::forward<FnGetArg>(fnGetArg), nArgCount);
	}
};

//
// Param1Arg1OperAdapter - operation with params and 1 argument
//
template <typename FnOperation>
class OneArgOneParam
{
	FnOperation m_fnOperation;
	std::string m_sParamName;
	ContextAwareValue m_cavParam;
	std::optional<Variant> m_vParamDefaultValue;
public:
	
	//
	//
	//
	OneArgOneParam(std::string_view sParamName, std::optional<Variant> vParamDefaultValue, FnOperation fnOperation) :
		m_fnOperation(fnOperation), m_sParamName(sParamName), m_vParamDefaultValue(vParamDefaultValue)
	{
	}

	//
	//
	//
	void init(Operation eOperation, const Variant& vCfg)
	{
		// get param from vCfg
		m_cavParam = vCfg.getSafe(m_sParamName);
		if (m_cavParam.isValid())
			return;

		// get param default value
		if (m_vParamDefaultValue.has_value())
		{
			m_cavParam = m_vParamDefaultValue.value();
			return;
		}

		// Error
		error::InvalidArgument(FMT("Operation <" << getOperationString(eOperation)
			<< "> expects the <" << m_sParamName << "> parameter")).throwException();
	}

	//
	// Call m_fnOperation
	//
	template<typename FnGetArg>
	Variant execute(const Variant& vContext, FnGetArg&& fnGetArg, Size /*nArgCount*/)
	{
		return m_fnOperation(fnGetArg(0), m_cavParam.get(vContext));
	}
};


//
// Param1Arg1OperAdapter - operation with params and 1 argument
//
template <typename FnOperation>
class ManyArgsOneParam
{
private:

	FnOperation m_fnOperation;
	std::string m_sParamName;
	ContextAwareValue m_cavParam;
	std::optional<Variant> m_optParamDefaultValue;

public:
	
	//
	//
	//
	ManyArgsOneParam(std::string_view sParamName, std::optional<Variant> optParamDefaultValue,
		FnOperation fnOperation) :
		m_fnOperation(fnOperation), m_sParamName(sParamName), m_optParamDefaultValue(optParamDefaultValue)
	{
	}

	//
	//
	//
	void init(Operation eOperation, const Variant& vCfg)
	{
		// get param from vCfg
		m_cavParam = vCfg.getSafe(m_sParamName);
		if (m_cavParam.isValid())
			return;

		// get param default value
		if (m_optParamDefaultValue.has_value())
		{
			m_cavParam = m_optParamDefaultValue.value();
			return;
		}

		// Error
		error::InvalidArgument(FMT("Operation <" << getOperationString(eOperation)
			<< "> expects the <" << m_sParamName << "> parameter")).throwException();
	}

	//
	// Call m_fnOperation
	//
	template<typename FnGetArg>
	Variant execute(const Variant& vContext, FnGetArg&& fnGetArg, Size nArgCount)
	{
		return m_fnOperation(std::forward<FnGetArg>(fnGetArg), nArgCount, m_cavParam.get(vContext));
	}
};

//
// Operation with two params and many arguments
//
template <typename FnOperation>
class ManyArgsTwoParams
{
private:

	FnOperation m_fnOperation;
	std::string m_sParam1Name;
	ContextAwareValue m_cavParam1;
	std::optional<Variant> m_optParam1DefaultValue;
	std::string m_sParam2Name;
	ContextAwareValue m_cavParam2;
	std::optional<Variant> m_optParam2DefaultValue;

public:

	//
	//
	//
	ManyArgsTwoParams(std::string_view sParam1Name, std::optional<Variant> optParam1DefaultValue,
		std::string_view sParam2Name, std::optional<Variant> optParam2DefaultValue, FnOperation fnOperation) :
		m_fnOperation(fnOperation), m_sParam1Name(sParam1Name), m_optParam1DefaultValue(optParam1DefaultValue),
		m_sParam2Name(sParam2Name), m_optParam2DefaultValue(optParam2DefaultValue)
	{
	}

	//
	//
	//
	void init(Operation eOperation, const Variant& vCfg)
	{
		m_cavParam1 = vCfg.getSafe(m_sParam1Name);
		if (!m_cavParam1.isValid())
		{
			if (!m_optParam1DefaultValue.has_value())
				error::InvalidArgument(FMT("Operation <" << getOperationString(eOperation)
					<< "> expects the <" << m_sParam1Name << "> parameter")).throwException();
			m_cavParam1 = m_optParam1DefaultValue.value();
		}

		m_cavParam2 = vCfg.getSafe(m_sParam2Name);
		if (!m_cavParam2.isValid())
		{
			if (!m_optParam2DefaultValue.has_value())
				error::InvalidArgument(FMT("Operation <" << getOperationString(eOperation)
					<< "> expects the <" << m_sParam2Name << "> parameter")).throwException();
			m_cavParam2 = m_optParam2DefaultValue.value();
		}
	}

	//
	// Call m_fnOperation
	//
	template<typename FnGetArg>
	Variant execute(const Variant& vContext, FnGetArg&& fnGetArg, Size nArgCount)
	{
		return m_fnOperation(std::forward<FnGetArg>(fnGetArg), nArgCount,
			m_cavParam1.get(vContext), m_cavParam2.get(vContext));
	}
};

//
// Operation with three params and many arguments
//
template <typename FnOperation>
class ManyArgsThreeParams
{
private:
	FnOperation m_fnOperation;
	std::string m_sParam1Name;
	ContextAwareValue m_cavParam1;
	std::optional<Variant> m_optParam1DefaultValue;
	std::string m_sParam2Name;
	ContextAwareValue m_cavParam2;
	std::optional<Variant> m_optParam2DefaultValue;
	std::string m_sParam3Name;
	ContextAwareValue m_cavParam3;
	std::optional<Variant> m_optParam3DefaultValue;

public:

	//
	//
	//
	ManyArgsThreeParams(std::string_view sParam1Name, std::optional<Variant> optParam1DefaultValue,
		std::string_view sParam2Name, std::optional<Variant> optParam2DefaultValue,
		std::string_view sParam3Name, std::optional<Variant> optParam3DefaultValue,
		FnOperation fnOperation) :
		m_fnOperation(fnOperation),
		m_sParam1Name(sParam1Name), m_optParam1DefaultValue(optParam1DefaultValue),
		m_sParam2Name(sParam2Name), m_optParam2DefaultValue(optParam2DefaultValue),
		m_sParam3Name(sParam3Name), m_optParam3DefaultValue(optParam3DefaultValue)
	{
	}

	//
	//
	//
	void init(Operation eOperation, const Variant& vCfg)
	{
		m_cavParam1 = vCfg.getSafe(m_sParam1Name);
		if (!m_cavParam1.isValid())
		{
			if (!m_optParam1DefaultValue.has_value())
				error::InvalidArgument(FMT("Operation <" << getOperationString(eOperation)
					<< "> expects the <" << m_sParam1Name << "> parameter")).throwException();
			m_cavParam1 = m_optParam1DefaultValue.value();
		}

		m_cavParam2 = vCfg.getSafe(m_sParam2Name);
		if (!m_cavParam2.isValid())
		{
			if (!m_optParam2DefaultValue.has_value())
				error::InvalidArgument(FMT("Operation <" << getOperationString(eOperation)
					<< "> expects the <" << m_sParam2Name << "> parameter")).throwException();
			m_cavParam2 = m_optParam2DefaultValue.value();
		}

		m_cavParam3 = vCfg.getSafe(m_sParam3Name);
		if (!m_cavParam3.isValid())
		{
			if (!m_optParam3DefaultValue.has_value())
				error::InvalidArgument(FMT("Operation <" << getOperationString(eOperation)
					<< "> expects the <" << m_sParam3Name << "> parameter")).throwException();
			m_cavParam3 = m_optParam3DefaultValue.value();
		}
	}

	//
	// Call m_fnOperation
	//
	template<typename FnGetArg>
	Variant execute(const Variant& vContext, FnGetArg&& fnGetArg, Size nArgCount)
	{
		return m_fnOperation(std::forward<FnGetArg>(fnGetArg), nArgCount,
			m_cavParam1.get(vContext), m_cavParam2.get(vContext), m_cavParam3.get(vContext));
	}
};

//
//
//
OperationPtr createOperation(const Variant& vOperation, const Variant& vConfig)
{
	using namespace variant;
	using namespace variant::operation;

	Operation eOperation = Operation (-1);
	switch (vOperation.getType())
	{
		case Variant::ValueType::String:
			eOperation = getOperationFromString(vOperation);
			break;
		case Variant::ValueType::Integer:
			eOperation = vOperation;
			break;
		default:
			error::InvalidArgument(FMT("Operation <" << vOperation
				<< "> is not supported")).throwException();
	}

	switch (eOperation)
	{
		case Operation::Not:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				NoParam([](auto fnGetArg, Size /*nArgCount*/)
				{
					return !(bool)convert<ValueType::Boolean>(fnGetArg(0));
				}
			)));
		case Operation::Or:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 2, c_nMaxSize,
				NoParam([](auto fnGetArg, Size nArgCount)
				{
					for (Size i = 0; i < nArgCount; ++i)
						if ((bool)convert<ValueType::Boolean>(fnGetArg(i)))
							return true;
					return false;
				}
			)));
		case Operation::And:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 2, c_nMaxSize,
				NoParam([](auto fnGetArg, Size nArgCount)
				{
					for (Size i = 0; i < nArgCount; ++i)
						if (!(bool)convert<ValueType::Boolean>(fnGetArg(i)))
							return false;
					return true;
				}
			)));
		case Operation::Add:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 2, c_nMaxSize,
				NoParam([](auto fnGetArg, Size nArgCount)
			{
				Variant v = fnGetArg(0);
				for (Size i = 1; i < nArgCount; ++i)
					v = operation::add(v, fnGetArg(i));
				return v;
				//	return operation::add(fnGetArg(0), fnGetArg(1));
			}
			)));
		case Operation::Equal:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 2,
				NoParam([](auto fnGetArg, Size /*nArgCount*/)
				{
					return operation::equal(fnGetArg(0), fnGetArg(1));
				}
			)));
		case Operation::NotEqual:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 2,
				NoParam([](auto fnGetArg, Size /*nArgCount*/)
			{
				return !operation::equal(fnGetArg(0), fnGetArg(1));
			}
			)));
		case Operation::Greater:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 2,
				NoParam([](auto fnGetArg, Size /*nArgCount*/)
				{
					return operation::greater(fnGetArg(0), fnGetArg(1));
				}
			)));
		case Operation::Less:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 2,
				NoParam([](auto fnGetArg, Size /*nArgCount*/)
				{
					return operation::less(fnGetArg(0), fnGetArg(1));
				}
			)));
		case Operation::Extract:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("path", std::nullopt, [](const Variant& vData, const Variant& vPath)
				{
					return operation::extract(vData, vPath);
				}
			)));
		case Operation::Match:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("pattern", std::nullopt, [](const Variant& vData, const Variant& vPattern)
				{
					auto pMatcher = queryInterfaceSafe<IStringMatcher>(vPattern);
					if (pMatcher != nullptr)
						return pMatcher->match(std::string_view(vData), false);
					return operation::match(vData, vPattern);
				}
			)));
		case Operation::NotMatch:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("pattern", std::nullopt, [](const Variant& vData, const Variant& vPattern)
			{
				auto pMatcher = queryInterfaceSafe<IStringMatcher>(vPattern);
				if (pMatcher != nullptr)
					return !pMatcher->match(std::string_view(vData), false);
				return !operation::match(vData, vPattern);
			}
			)));
		case Operation::IMatch:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("pattern", std::nullopt, [](const Variant& vData, const Variant& vPattern)
			{
				auto pMatcher = queryInterfaceSafe<IStringMatcher>(vPattern);
				if (pMatcher != nullptr)
					return pMatcher->match(std::string_view(vData), true);
				return operation::match(vData, vPattern, true);
			}
			)));
		case Operation::NotIMatch:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("pattern", std::nullopt, [](const Variant& vData, const Variant& vPattern)
			{
				auto pMatcher = queryInterfaceSafe<IStringMatcher>(vPattern);
				if (pMatcher != nullptr)
					return !pMatcher->match(std::string_view(vData), true);
				return !operation::match(vData, vPattern, true);
			}
			)));
		case Operation::Contain:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("item", std::nullopt, [](const Variant& vData, const Variant& vItem)
				{
					return operation::contain(vData, vItem);
				}
			)));
		case Operation::Transform:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("schema", std::nullopt, [](const Variant& vData, const Variant& vSchema)
				{
					return operation::transform(vData, vSchema, TransformSourceMode::CloneSource);
				}
			)));
		case Operation::EmplaceTransform:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("schema", std::nullopt, [](const Variant& vData, const Variant& vSchema)
				{
					return operation::transform(vData, vSchema, TransformSourceMode::ChangeSource);
				}
			)));
		case Operation::Filter:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("schema", std::nullopt, [](const Variant& vData, const Variant& vSchema)
			{
				return operation::transform(vData, vSchema, TransformSourceMode::ReplaceSource);
			}
			)));
		case Operation::Merge:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 2, c_nMaxSize,
				ManyArgsOneParam("flags", std::nullopt,
					[](auto fnGetArg, Size nArgCount, const Variant& vFlags)
				{
					auto vDst = fnGetArg(0);
					if (!vDst.isEmpty())
						vDst = vDst.clone();
					for (Size i = 1; i < nArgCount; ++i)
						vDst.merge(fnGetArg(i), vFlags);
					return vDst;
				}
			)));
		case Operation::Has:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("path", std::nullopt, [](const Variant& vData, const Variant& vPath)
				{
					return getByPathSafe(vData, vPath).has_value();
				}
			)));
		case Operation::NotHas:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("path", std::nullopt, [](const Variant& vData, const Variant& vPath)
					{
						return !getByPathSafe(vData, vPath).has_value();
					}
			)));
		case Operation::Clone:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1,
				OneArgOneParam("resolve", false, [](const Variant& vData, const Variant& vResolve)
				{
					bool fResolve = convert<ValueType::Boolean>(vResolve);
					return vData.clone(fResolve);
				}
			)));
		case Operation::LogWarning:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1, c_nMaxSize,
				ManyArgsTwoParams("component", Variant(), "resolve", false,
					[](auto fnGetArg, Size nArgCount, const Variant& vComponent, const Variant& vResolve)
			{
				bool fResolve = convert<ValueType::Boolean>(vResolve);

				std::string sMsg;
				for (Size i = 0; i < nArgCount; ++i)
				{
					Variant vArg = fnGetArg(i);
					(void)vArg.getType(); // resolve a proxy
					sMsg += (fResolve ? vArg.clone(true).print() : vArg.print());
				}

				if (!vComponent.isNull())
					LOGWRN(std::string(vComponent), sMsg);
				else
					LOGWRN(sMsg);
				return true;
			}
			)));
		case Operation::LogInfo:
			return OperationPtr(new BasicOperation(eOperation, vConfig, 1, c_nMaxSize,
				ManyArgsThreeParams("logLevel", Variant(), "component", Variant(), "resolve", false,
					[](auto fnGetArg, Size nArgCount, const Variant& vLogLevel, const Variant& vComponent,
						const Variant& vResolve)
			{
				LogLevel logLevel = (vLogLevel.isNull() ? LogLevel::Normal : (LogLevel)vLogLevel);
				bool fResolve = convert<ValueType::Boolean>(vResolve);

				std::string sMsg;
				for (Size i = 0; i < nArgCount; ++i)
				{
					Variant vArg = fnGetArg(i);
					(void)vArg.getType(); // resolve a proxy
					sMsg += (fResolve ? vArg.clone(true).print() : vArg.print());
				}

				if (!vComponent.isNull())
					LOGLVL2(logLevel, std::string(vComponent), sMsg);
				else
					LOGLVL2(logLevel, sMsg);
				return true;
			}
			)));
		default:
			error::InvalidUsage(FMT("Invalid operation <" << vOperation << ">")).throwException();
	}
}

} // namespace ctxoper;

} // namespace cmd 
