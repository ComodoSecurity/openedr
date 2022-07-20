//
// edrav2.libcore project
//
// Implementation of the Scenario object
//
// Author: Yury Podpruzhnikov (27.02.2019)
// Reviewer: Denis Bogdanov (04.03.2019)
//
/// @file Scenario object
/// @addtogroup basicCmd Basic objects for support commands and scenarios
/// @{
#pragma once
#include <command.hpp>
#include <objects.h>
#include <common.hpp>

namespace cmd {

//////////////////////////////////////////////////////////////////////////
//
// TimerStatAgent
//



//
// Statistical agent for time measuring
//
class TimerStatAgent
{
private:
	std::atomic_bool m_fEnabled = false;

	std::atomic_uint64_t m_nThreadTimeMillisec = 0;
	std::atomic_uint64_t m_nHighResTimeMicrosec = 0;
	std::atomic_uint64_t m_nCounter = 0;

public:

	//
	// Measurer of scope time 
	//
	class ScopeTimer
	{
	private:
		using high_resolution_clock = std::chrono::high_resolution_clock;
		using thread_clock = boost::chrono::thread_clock;

		TimerStatAgent& m_agent;
		high_resolution_clock::time_point m_nHighResStart;
		thread_clock::time_point m_nThreadStart;

	public:
		explicit ScopeTimer(TimerStatAgent& agent) : m_agent(agent)
		{
			m_nHighResStart = high_resolution_clock::now();
			m_nThreadStart = thread_clock::now();
		}

		ScopeTimer(const ScopeTimer&) = delete;
		ScopeTimer& operator=(const ScopeTimer&) = delete;

		~ScopeTimer()
		{
			uint64_t nHighResTime = std::chrono::duration_cast<std::chrono::microseconds>(
				high_resolution_clock::now() - m_nHighResStart).count();
			if (nHighResTime != 0)
				m_agent.m_nHighResTimeMicrosec += nHighResTime;

			uint64_t nThreadTime = boost::chrono::duration_cast<boost::chrono::milliseconds>(
				thread_clock::now() - m_nThreadStart).count();
			if (nThreadTime != 0)
				m_agent.m_nThreadTimeMillisec += nThreadTime;

			++m_agent.m_nCounter;
		}
	};

	TimerStatAgent() = default;
	TimerStatAgent(const TimerStatAgent&) = delete;
	TimerStatAgent& operator=(const TimerStatAgent&) = delete;

	TimerStatAgent(TimerStatAgent&& other) noexcept
	{
		m_fEnabled = other.m_fEnabled.load();
		m_nCounter = other.m_nCounter.load();
		m_nHighResTimeMicrosec = other.m_nHighResTimeMicrosec.load();
		m_nThreadTimeMillisec = other.m_nThreadTimeMillisec.load();
	}
	TimerStatAgent& operator=(TimerStatAgent&&) noexcept = default;

	explicit TimerStatAgent(bool fEnabled) : m_fEnabled(fEnabled)
	{
	}

	//
	// Switches on/off
	//
	void enable(bool fEnabled)
	{
		if (fEnabled && !m_fEnabled)
		{
			m_nThreadTimeMillisec = 0;
			m_nHighResTimeMicrosec = 0;
			m_nCounter = 0;
		}
		m_fEnabled = fEnabled;
	}

	//
	// Return state
	//
	bool isEnabled()
	{
		return m_fEnabled;
	}

	//
	// Creates timer for measure time of scope
	//
	std::optional<ScopeTimer> measureScopeTime()
	{
		if (!m_fEnabled)
			return std::nullopt;
		return std::make_optional<ScopeTimer>(*this);
	}

	//
	// Returns accumulated info
	//
	Variant getInfo() const
	{
		return Dictionary({
			{"globalTime", m_nHighResTimeMicrosec.load() / 1000 },
			{"threadTime", m_nThreadTimeMillisec.load() },
			{"count", m_nCounter.load() },
		});
	}
};


///
/// Result of scenario command.
/// It is mainly intended to change execution flow.
///
struct ScenarioActResult
{
	enum class Type 
	{
		Invalid, //< Invalid value (initial value)
		Return, //< Stop scenario and return vValue

		// Jump to other branch.
		// If branch with name sBranchName is exist, jump to it.
		// If the sBranchName branch is absent and branch with name sDefaultBranchName is exist,
		// Jump to the sDefaultBranchName branch.
		// Otherwise exception is thrown.
		JumpToBranch,
	};

	Type eType = Type::Invalid; //< Result type

	// Additional values
	std::string sBranchName; //< Name of branch for JumpToBranch and JumpToBranchWithDefault
	std::string sDefaultBranchName; //< Name of default branch for JumpToBranchWithDefault
	Variant vValue; //< returned value
};

///
/// Base Interface for actions (instructions) in scenario
///
class IScenarioAction
{
public:
	virtual ~IScenarioAction() {};

	///
	/// Execute command.
	/// If command needs change execution flow, it return special ScenarioActResult.
	///
	/// @param vContext - Call context. Dictionary
	/// @return ScenarioActResult - Information to change execution flow
	///
	virtual std::optional<ScenarioActResult> execute(const Variant& vContext) const = 0;
};


///
/// A Scenario object provides execution of a sequence of actions.
/// Scenario implements both IContextCommand and ICommand.
///
/// #### Call context
/// Main target of scenario actions is a process (read/write) a call context
/// Scenario process exist context if it passed via IContextCommand::execute.
/// Otherwise every execution it create new context - empty Dictionary.
///
///
/// #### Actions
/// Action is elementary item of scenario. 
/// An action description is a Dictionary (see below).
/// Scenario supports following actions:
///   * $set
///   * $ret
///   * $goto
///   * $exec
/// Action type detected by presence one of this string in the descriptor keys.
/// If no one of them is present, then type is $exec.
///
/// ##### Common fields
/// Each action support following fields:
///   * $if [opt, ContextAwareValue, logical bool] - if exist and false, action will be skipped.
///   * $debug [opt, ContextAwareValue, logical bool] - if exist and true, DebugBreak() is called.
///   * $catch [opt] - if exist, exception processing is enabled (see below). 
///
/// Exception processing.
/// "$catch" describes special exception in action execution.
/// if "$catch" is absent, An exception is passed outside.  
/// "$catch" can contain CatchRule or sequence of CatchRule-s.
/// CatchRule can be:
///   * dictionary with fields:
///     * code [int|str, opt] - Catched ErrorCode as int or hex string. 
///                             0 or "" catch all exceptions. Default value: 0.
///     * goto [str, opt] - Branch to jump when exception is occurred.
///                         If it contains "", the scenario just goes to next line.
///                         Default value: "".
///     * log [bool, opt] - If it is true, exception is logged. Default value: false.
///     * dst [str, opt] - Serialize the exception to specified path in context. 
///                        If it contains "", the exception is not serialized. Default value: "".
///   * null - Shortcut of {"goto":""}
///   * string - Shortcut of {"goto":%SpecifiedString%}
///
/// ##### $set
/// Sets the value inside the call context.  
/// Action fields:
///   * $set [ContextAwareValue] - Value which will be set into the context.
///   * $dst [ContextAwareValue, str] - String. Path inside the context to set value.
///
/// ##### $ret
/// Stops scenario and returns specified value.
/// Action fields:
///   * $ret [ContextAwareValue] - the value returned by Scenario.
///
/// ##### $goto
/// Jumps to specified branch.
/// Action fields:
///   * $goto [ContextAwareValue, str] - Name of branch to jump.
///   * $default [ContextAwareValue, opt, str] - default branch name. Scenario jump to it if 
///     branch name in $goto is not found.
///
/// ##### $exec
/// Executes ICommand or IContextCommand.
/// Action fields:
///   * $exec [ContextAwareValue, obj, opt] - ICommand, IContextCommand.
///   * $dst [ContextAwareValue, str, opt] - Path (String) inside the context to set value.
///                                          If it is absent, the result is not saved.
/// If the $exec key is absent, whole descriptor should be object descriptor.
/// If the object descriptor does not contain the "clsid" field, CLSID_Command is used.
/// The object is created on compilation. Object should implement ICommand, IContextCommand.
///
///
/// #### Branches
/// The scenario have one or more named branches. 
/// The scenario always starts execution from the "main" branch.
/// A branch descriptor is Sequence with actions.
/// If execution of the current branch is completed (without $ret and $goto), 
/// Scenario will be stopped and return null 
/// 
///
/// #### Scenario compilation
/// Each scenario is compiled (preprocessed) at first execution. 
/// The compilation is thread safe.
///
///
/// #### Multi thread safety
/// Scenario actions are thread safe themselves.
/// So execution of Scenario is thread safe if called commands and processed data are thread safe.
///
///
/// #### Examples
/// 
/// ##### Example 1
/// {"clsid": "0x61A30254", // CLSID_Scenario
/// "name": "exampleScenario", 
/// "code" : [
/// 	{"$set" : 1, "$dst" : "a"}, // $ctx["a"] = 1
/// 	{"$set" : {"$path" : "a"}, "$dst" : "b"},  // $ctx["b"] = $ctx["a"]
/// 	{"$set" : {"$path" : "a"}, "$dst" : "c", "$if" : false},  // Skipped action ($if == false)
/// 	{"processor" : "objects.app", "command" : "setLogLevel", "root" : 4},  // Calls command of application object
/// 	{"$ret" : {"$path": "b"} }  // Returns $ctx["b"]
/// ]}
/// 
/// ##### Example 2
/// {"clsid": "0x61A30254", // CLSID_Scenario
/// "name" : "exampleScenario",
/// "code" : {
/// 	"main" : [
/// 		{"$set" : 1, "$dst" : "a"}, // $ctx["a"] = 1
/// 		{"$goto" : "second"} // Jumps to the "second" branch
/// 	],
/// 	"second" : [
/// 		{"$ret" : {"$path": ""} }  // Returns $ctx
/// 	]
/// }
/// 


//
//
//
class Scenario : public ObjectBase<CLSID_Scenario>, 
	public ICommand, 
	public IContextCommand
{
private:

	inline static constexpr char c_sStartBranch[] = "main";
	typedef std::unique_ptr<IScenarioAction> ScenarioActPtr;

	std::string m_sScenarioName; //< Name of Scenario (for logging)

	//
	// Line
	//
	struct LineInfo
	{
		ScenarioActPtr pAction;
		mutable TimerStatAgent timer; //< line time statistic

		LineInfo(const LineInfo&) = delete;
		LineInfo& operator=(const LineInfo&) = delete;
		LineInfo(LineInfo&&) noexcept = default;
		LineInfo& operator=(LineInfo&&) noexcept = default;
		LineInfo() = default;
		~LineInfo() = default;

		LineInfo(ScenarioActPtr&& pAction_, bool fEnableTimer) :
			pAction(std::move(pAction_)), timer(fEnableTimer)
		{
		}
	};

	mutable TimerStatAgent m_timer; //< whole scenario time statistic
	std::atomic_bool m_fEnableStatistic = false;

	//
	// Compiled branch of execution
	//
	struct Branch
	{
		std::vector<LineInfo> actions;
	};

	typedef std::unordered_map<std::string, Branch> BranchesMap; //< Compiled branches type
	BranchesMap m_branchesMap;  //< Compiled branches

	Variant m_vBranches; //< Branches source (not compiled branches)

	std::mutex m_mtxCompiler; //< compilation sync
	bool m_fIsCompiled = false; //< compilation flag
	bool m_fAddParamsIntoContext = false;

	// Compile whole scenario
	// compileScenario should be called before any execution of scenario
	void compileScenario();

	// Compile one branch
	Branch compileBranch(Sequence seqBranch);

	// Execute scenario
	Variant executeScenario(Variant vContext, Variant vParams) const;

	//
	// Return branch or throw InvalidParameter
	//
	const Branch* getBranch(const std::string& sName) const;

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   * "name" [str, opt] - A name of scenario for logging. 
	///     If it is not specified, automatic name will be generated.
	///   * "code" [dict or seq] - Description of one or several branches. (see below)
	///   * "compileOnCall" [bool, opt] - compile on first. Default value: false.
	///   * "regInMgr" [bool, opt] - Enable registration in ScenarioManager. Default value: true.
	///   * "addParams" [bool, opt] - Add value of the vParam parameters of execute() method into context
	///     The value has name "params". Default value is false.
	///
	/// "code" can be:
	///   * Sequence which contain the "main" branch descriptor. 
	///     In this case the scenario does not support $goto.
	///   * Dictionary with descriptors of several branches. 
	///     Each item key is name of branch. Each value is a branch descriptor 
	///     The Dictionary should contain the "main" branch.
	///
	void finalConstruct(Variant vConfig);

	// ICommand interface
	virtual Variant execute(Variant vParams) override;
	virtual std::string getDescriptionString() override;

	// IContextCommand interface
	virtual Variant execute(Variant vContext, Variant vParam) override;

	// Self public methods
	
	//
	// Returns Scenario name
	//
	std::string_view getName();

	//
	// Enables/Disables statistic collection
	//
	void enableStatInfo(bool fEnable);

	//
	// Returns collected statistic
	// @return statistic or null, if disabled
	//
	Variant getStatInfo() const;
};

} // namespace cmd 
/// @}
