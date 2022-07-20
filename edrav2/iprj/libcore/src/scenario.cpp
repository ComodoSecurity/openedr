//
// edrav2.libcore project
//
// Implementation of the Scenario object
//
// Author: Yury Podpruzhnikov (27.02.2019)
// Reviewer: Denis Bogdanov (04.03.2019)
//
#include "pch.h"
#include "service.hpp"
#include "scenariomgr.h"
#include "scenario.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "scenario"

namespace cmd {


//////////////////////////////////////////////////////////////////////////
//
// Scenario actions
//

namespace scenario {

//
// Base class for all actions
//
class ActionBase : public IScenarioAction
{
private:
	ContextAwareValue m_cavDebug; // Debug flag
	ContextAwareValue m_cavCondition; // If false, action is not executed

	//
	// An exception catch rule
	//
	struct CatchRule
	{
		error::ErrorCode eErrorCode = error::ErrorCode(0); //< condition to catch exception (0 - all exception)
		std::string sJumpBranchName; //< branch name to jump when exception is occurred, empty string - just go to next line
		bool fEnableLog = false; //< log an exception when it is occurred.
		std::string sDstName; //< Name of the context field to serialize exception. If it is "", serialization is skipped

		CatchRule() = default;
		~CatchRule() = default;
		CatchRule(const CatchRule&) = default;
		CatchRule(CatchRule&&) = default;
		CatchRule& operator=(const CatchRule&) = default;
		CatchRule& operator=(CatchRule&&) = default;

		CatchRule(Variant& vCfg)
		{
			if (vCfg.isNull())
				return;

			if (vCfg.isString())
			{
				sJumpBranchName = vCfg;
				return;
			}

			if (!vCfg.isDictionaryLike())
				error::InvalidArgument(SL, FMT("Invalid $catch rule: " << vCfg)).throwException();

			// Process full descriptor
			sJumpBranchName = vCfg.get("goto", "");
			sDstName = vCfg.get("dst", "");
			fEnableLog = vCfg.get("log", false);

			auto vCode = vCfg.getSafe("code");
			if (!vCode.has_value())
				eErrorCode = error::ErrorCode(0);
			else if (vCode->getType() == Variant::ValueType::Integer)
			{
				eErrorCode = vCode.value();
			}
			else if (vCode->isString())
			{
				std::string sCode = vCode.value();
				eErrorCode =  error::ErrorCode(sCode.empty() ? 0 : std::stoul(sCode, nullptr, 16));
			}
			else
			{
				error::InvalidArgument(SL, FMT("Invalid $catch rule: " << vCfg)).throwException();
			}
		}
	};

	std::vector<CatchRule> m_catchRules; //< Rules list



	enum class DebugAction
	{
		None = 0,
		BreakIfDebuggerPresent = 1,
		Break = 2,
	};

	//
	//
	//
	bool checkDebug(const Variant& vContext) const
	{
		if (!m_cavDebug.isValid())
			return false;

		DebugAction eDebugAction = variant::convert<Variant::ValueType::Integer>(m_cavDebug.get(vContext));

		if (eDebugAction == DebugAction::BreakIfDebuggerPresent || eDebugAction == DebugAction::Break)
		{
			#ifdef PLATFORM_WIN
			if (!::IsDebuggerPresent())
			{
				if (eDebugAction == DebugAction::BreakIfDebuggerPresent)
					return true;

				// Wait debugger
				LOGINF("Waiting for debugger connection...");
				while (!::IsDebuggerPresent())
					Sleep(100);
			}

			// interrupt
			::DebugBreak();
			#else
			#error Add code for different platforms here
			#endif
		}
		return true;
	}

	//
	//
	//
	bool isEnabled(const Variant& vContext) const
	{
		if (!m_cavCondition.isValid()) return true;
		return variant::convert<Variant::ValueType::Boolean>(m_cavCondition.get(vContext));
	}

	//
	// Contain common code of all actions
	//
	std::optional<ScenarioActResult> executeWithoutCatch(const Variant& vContext) const
	{
		checkDebug(vContext);
		if (!isEnabled(vContext)) return std::nullopt;

		return executeInt(vContext);
	}



protected:

	//
	//
	//
	ActionBase(const Variant& vCfg)
	{
		m_cavDebug = vCfg.getSafe("$debug");
		m_cavCondition = vCfg.getSafe("$if");

		// Fill catch info
		auto vCatchInfo = vCfg.getSafe("$catch");
		if (vCatchInfo.has_value())
		{
			if (vCatchInfo->isSequenceLike())
			{
				for (auto vRule : vCatchInfo.value())
					m_catchRules.push_back(CatchRule(vRule));
			}
			else
			{
				m_catchRules.push_back(CatchRule(vCatchInfo.value()));
			}
		}

	}

	//
	// Contain code of specific action
	//
	virtual std::optional<ScenarioActResult> executeInt(const Variant& vContext) const = 0;

public:

	//
	// Contain exception processing
	//
	virtual std::optional<ScenarioActResult> execute(const Variant& vContext) const final
	{
		// Process action if no $catch
		if (m_catchRules.empty())
			return executeWithoutCatch(vContext);

		// Process action if $catch
		CMD_TRY
		{
			return executeWithoutCatch(vContext);
		}
		CMD_PREPARE_CATCH
		catch (error::Exception& e)
		{
			// find rule
			const CatchRule* pCatchRule = nullptr;
			for (auto& rule : m_catchRules)
			{
				if (rule.eErrorCode == error::ErrorCode(0) || e.isBasedOnCode(rule.eErrorCode))
				{
					pCatchRule = &rule;
					break;
				}
			}
			if (pCatchRule == nullptr)
				throw;

			// Process rule

			// Logging exception
			if(pCatchRule->fEnableLog)
				e.log(SL);

			// Saving exception into context
			if(!pCatchRule->sDstName.empty())
				putByPath(vContext, pCatchRule->sDstName, e.serialize(), true);

			// If a branch is not specified, go to next line
			if (pCatchRule->sJumpBranchName.empty())
				return std::nullopt;

			// If a branch is specified, jump to the branch
			ScenarioActResult result;
			result.eType = ScenarioActResult::Type::JumpToBranch;
			result.sBranchName = pCatchRule->sJumpBranchName;
			return result;
		}
	}
};

//
// $set
//
class ExecAction : public ActionBase
{
private:
	ContextAwareValue m_cavCommand;
	bool m_fHasDstPath = false;
	ContextAwareValue m_cavDstPath;

public:

	///
	/// Constructor
	///
	/// @param vCfg The configuration.
	///
	ExecAction(Variant vCfg) : ActionBase(vCfg)
	{
		auto vExecValue = vCfg.getSafe("$exec");
		// if $exec is null or absent, vCfg is an object descriptor
		// FIXME: Don't access to variant::detail
		if (!vExecValue.has_value())
		{
			ObjPtr<IObject> pObject;
			if(vCfg.has("clsid"))
				pObject = createObject(vCfg);
			else
				pObject = createObject(CLSID_Command, vCfg);

			if (!queryInterfaceSafe<ICommand>(pObject) && 
				!queryInterfaceSafe<IContextCommand>(pObject))
					error::InvalidArgument(
						FMT("Object in $exec doesn't support ICommand/IContextCommand <"
							<< pObject << ">")).throwException();
			m_cavCommand = pObject;
		}
		else
		{
			Variant vCmd = vExecValue.value();
			if (!vCmd.isObject() || (!queryInterfaceSafe<ICommand>(vCmd) &&
				!queryInterfaceSafe<IContextCommand>(vCmd)))
				error::InvalidArgument(
					FMT("Value of the '$exec' is not object or does not support ICommand/IContextCommand."
						" Value: <" << vCmd << ">")).throwException();

			m_cavCommand = vExecValue.value();
		}

		if (vCfg.has("$dst"))
		{
			m_fHasDstPath = true;
			m_cavDstPath = vCfg.get("$dst");
		}
	}

	//
	//
	//
	virtual std::optional<ScenarioActResult> executeInt(const Variant& vContext) const override
	{
		Variant vResult = m_cavCommand.get(vContext);
		if (m_fHasDstPath)
			putByPath(vContext, m_cavDstPath.get(vContext), vResult, true);
		
		return std::nullopt;
	}
};

//
// $set
//
class SetAction : public ActionBase
{
private:
	ContextAwareValue m_cavValue;
	ContextAwareValue m_cavDstPath;
public:

	//
	//
	//
	SetAction(Variant vCfg) : ActionBase(vCfg)
	{
		m_cavValue = vCfg.get("$set");
		m_cavDstPath = vCfg.get("$dst");
	}

	//
	//
	//
	virtual std::optional<ScenarioActResult> executeInt(const Variant& vContext) const override
	{
		putByPath(vContext, m_cavDstPath.get(vContext), m_cavValue.get(vContext), true);
		return std::nullopt;
	}
};

//
// $goto
//
class GotoAction : public ActionBase
{
private:
	ContextAwareValue m_cavBranchName;
	ContextAwareValue m_cavDefaultBranchName;
public:
	GotoAction(Variant vCfg) : ActionBase(vCfg)
	{
		m_cavBranchName = vCfg.get("$goto");
		m_cavDefaultBranchName = vCfg.getSafe("$default");
	}

	//
	//
	//
	virtual std::optional<ScenarioActResult> executeInt(const Variant& vContext) const override
	{
		ScenarioActResult result;

		result.eType = ScenarioActResult::Type::JumpToBranch;
		result.sBranchName = variant::convert<Variant::ValueType::String>(m_cavBranchName.get(vContext));
		if (m_cavDefaultBranchName.isValid())
			result.sDefaultBranchName = variant::convert<Variant::ValueType::String>(
				m_cavDefaultBranchName.get(vContext));

		return result;
	}
};


//
// $ret
//
class RetAction : public ActionBase
{
private:
	ContextAwareValue m_cavReturnedValue;

public:
	RetAction(Variant vCfg) : ActionBase(vCfg)
	{
		m_cavReturnedValue = vCfg.get("$ret");
	}

	//
	//
	//
	virtual std::optional<ScenarioActResult> executeInt(const Variant& vContext) const override
	{
		ScenarioActResult result;
		result.eType = ScenarioActResult::Type::Return;
			result.vValue = m_cavReturnedValue.get(vContext);
		return result;
	}
};

}// namespace scenario


//////////////////////////////////////////////////////////////////////////
//
// Scenario
//


//
//
//
Scenario::Branch Scenario::compileBranch(Sequence seqBranch)
{
	Size nLine = 0;
	const char* sActType = "unknown";
	CMD_TRACE_BEGIN
	Branch branch;
	branch.actions.reserve(seqBranch.getSize());

	bool fEnableStatistic = m_timer.isEnabled();

	for (auto vAction : seqBranch)
	{
		std::unique_ptr<IScenarioAction> pAction;
		if (vAction.has("$set"))
		{
			sActType = "$set";
			pAction = std::make_unique<scenario::SetAction>(vAction);
		}
		else if (vAction.has("$goto"))
		{
			sActType = "$goto";
			pAction = std::make_unique<scenario::GotoAction>(vAction);
		}
		else if (vAction.has("$ret"))
		{
			sActType = "$ret";
			pAction = std::make_unique<scenario::RetAction>(vAction);
		}
		else // "$exec". - The "$exec" field can be skipped
		{
			sActType = "$exec";
			pAction = std::make_unique<scenario::ExecAction>(vAction);
		}

		branch.actions.push_back(LineInfo(std::move(pAction), fEnableStatistic));
		//branch.actions.push_back(std::move(lineInfo) );
		++nLine;
	}

	return branch;
	CMD_TRACE_END(FMT("line <" << nLine << ">, action <" << sActType << ">"));
}

//
//
//
void Scenario::compileScenario()
{
	std::scoped_lock lock(m_mtxCompiler);
	if (m_fIsCompiled) return;

	std::string sBranchName = "INVALID_BRANCH_NAME";
	CMD_TRACE_BEGIN

	if (m_vBranches.isDictionaryLike())
	{
		BranchesMap branchesMap;
		Dictionary dictBranches(m_vBranches);
		for (auto kv : dictBranches)
		{
			sBranchName = kv.first;
			Sequence seqBranch(kv.second);
			Branch branch = compileBranch(seqBranch);
			branchesMap[sBranchName] = std::move(branch);
		}

		swap(branchesMap, m_branchesMap);
	}
	else if (m_vBranches.isSequenceLike())
	{
		sBranchName = c_sStartBranch;
		BranchesMap branchesMap;
		Sequence seqBranch(m_vBranches);
		Branch branch = compileBranch(seqBranch);
		branchesMap[sBranchName] = std::move(branch);

		swap(branchesMap, m_branchesMap);
	}
	else
	{
		error::InvalidArgument(FMT("Invalid type of <branches> field (" 
			<< m_vBranches.getType() << ")"));
	}

	m_fIsCompiled = true;
	CMD_TRACE_END(FMT("Can't compile scenario <" << m_sScenarioName <<
		">, branch <" << sBranchName << ">"));
}

//
// Return branch or throw InvalidParameter
//
const Scenario::Branch* Scenario::getBranch(const std::string& sName) const
{
	auto it = m_branchesMap.find(sName);
	if (it == m_branchesMap.end())
		return nullptr;
	return &(it->second);
}

//
//
//
Variant Scenario::executeScenario(Variant vContext, Variant vParams) const
{
	std::string sBranchName = c_sStartBranch;
	Size nCurLine = 0;

	auto scenarioScopeTimer = m_timer.measureScopeTime();

	// Create context
	if (vContext.isNull())
		vContext = Dictionary();

	if (m_fAddParamsIntoContext)
		vContext.put("params", vParams);

	const Branch* pCurBranch = getBranch(c_sStartBranch);
	if(pCurBranch == nullptr)
		error::InvalidUsage(FMT("Scenario <" << m_sScenarioName << "> does not have default branch <"
		<< c_sStartBranch << ">")).throwException();

	CMD_TRACE_BEGIN;
	
	Variant vReturnValue;
	// Process commands
	while (true)
	{
		// Checking branch finish
		if (nCurLine >= pCurBranch->actions.size())
			break;
		
		auto lineScopeTimer = pCurBranch->actions[nCurLine].timer.measureScopeTime();

		// Doing action
		auto actionResult = pCurBranch->actions[nCurLine].pAction->execute(vContext);

		// if no special result - goto next action
		if (!actionResult.has_value())
		{
			++nCurLine;
			continue;
		}

		// Stopping scenario and return value
		if (actionResult->eType == ScenarioActResult::Type::Return)
		{
			vReturnValue = actionResult->vValue;
			break;
		}

		// Jump to branch
		if (actionResult->eType == ScenarioActResult::Type::JumpToBranch)
		{
			// Get branch
			std::string sNewBranchName = actionResult->sBranchName;
			const Branch* pNewBranch = getBranch(actionResult->sBranchName);

			// Get default branch
			if (pNewBranch == nullptr && !actionResult->sDefaultBranchName.empty())
			{
				sNewBranchName = actionResult->sDefaultBranchName;
				pNewBranch = getBranch(actionResult->sDefaultBranchName);
			}

			if (pNewBranch == nullptr)
				error::InvalidArgument(SL, FMT("Jump to absent branch <" << actionResult->sBranchName <<
					">. Default branch <" << actionResult->sDefaultBranchName << "> is not set or absent too.")).
					throwException();

			sBranchName = std::move(sNewBranchName);
			pCurBranch = pNewBranch;
			nCurLine = 0;
			continue;
		}

		error::InvalidUsage(FMT("Invalid action result type <" 
			<< (uint32_t)actionResult->eType << ">")).throwException();
	}

		return vReturnValue;

	CMD_TRACE_END(FMT("Scenario execution error <" << m_sScenarioName << ":" << sBranchName << ":" <<
		nCurLine << ">"));
}


//
//
//
void Scenario::finalConstruct(Variant vConfig)
{
	m_fAddParamsIntoContext = vConfig.get("addParams", m_fAddParamsIntoContext);
	
	auto vName = vConfig.getSafe("name");
	m_sScenarioName = (vName.has_value())? std::string(variant::convert<variant::ValueType::String>(*vName)) :
		std::string(FMT("sid-" << hex(getId())));
	m_vBranches = vConfig.get("code");

	if(vConfig.get("compileOnCall", false))
		compileScenario();




	// Register in ScenarioManager
	CMD_TRY
	{
		do 
		{
			auto vRegInMgr = vConfig.getSafe("regInMgr");
			bool fNeedRegister = vRegInMgr.has_value() ? (bool) vRegInMgr.value() : true;
			bool fForceRegister = vRegInMgr.has_value() ? fNeedRegister : false;

			// Check registration is need
			if(!fNeedRegister)
				break;

			// Check scenarioManager existence
			if (!fForceRegister && !(getCatalogData("objects").has("scenarioManager")))
				break;

			// registration
			auto pManager = queryInterface<ScenarioManager>(queryService("scenarioManager"));
			pManager->registerScenario(getPtrFromThis());
		} while (false);
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& ex)
	{
		ex.log(SL, "Can't register Scenario in scenario manager.");
	}
}

//
//
//
Variant Scenario::execute(Variant vContext, Variant vParam /*= Variant()*/)
{
	compileScenario();
	return executeScenario(vContext, vParam);
}

//
//
//
Variant Scenario::execute(Variant vParams)
{
	compileScenario();
	return executeScenario(nullptr, vParams);
}

//
//
//
std::string Scenario::getDescriptionString()
{
	return m_sScenarioName;
}

std::string_view Scenario::getName()
{
	return m_sScenarioName;
}


//
//
//
void Scenario::enableStatInfo(bool fEnable)
{
	if (m_fEnableStatistic == fEnable)
		return;

	// switch status
	// We should lock m_mtxCompiler to avoid racing with compileScenario
	std::scoped_lock lock(m_mtxCompiler);
	m_fEnableStatistic = fEnable;

	// Enable global timer
	m_timer.enable(fEnable);

	// Enable line timers
	for (const auto&[sBranchName, branch] : m_branchesMap)
		for (auto& lineInfo : branch.actions)
			lineInfo.timer.enable(fEnable);
}

//
//
//
Variant Scenario::getStatInfo() const
{
	if (!m_fEnableStatistic)
		return {};

	Dictionary vLines;

	for (const auto&[sBranchName, branch] : m_branchesMap)
	{
		const auto& actions = branch.actions;
		Size nLineCount = actions.size();
		for (Size i=0; i<nLineCount; ++i)
		{
			Variant vLineInfo = actions[i].timer.getInfo();
			std::string sLineName = FMT(sBranchName << "@" << i);
			vLines.put(sLineName, vLineInfo);
		}
	}

	Dictionary vInfo({ 
		{"total", m_timer.getInfo()},
		{"lines", vLines},
	});

	return vInfo;
}

} // namespace cmd 
