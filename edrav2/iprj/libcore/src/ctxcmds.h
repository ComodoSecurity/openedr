//
// edrav2.libcore project
//
// Basic conext aware commands
//
// Author: Yury Podpruzhnikov (03.02.2019)
// Reviewer: Denis Bogdanov (14.03.2019)
//
#pragma once
#include <command.hpp>
#include <objects.h>
#include <common.hpp>

namespace cmd {

//////////////////////////////////////////////////////////////////////////
//
// ConditionalCtxCmd
//

///
/// Conditional calculated value
///
class ConditionalCtxCmd : public ObjectBase<CLSID_ConditionalCtxCmd>, 
	public IContextCommand
{
private:
	ContextAwareValue m_cavIf;
	ContextAwareValue m_cavThen;
	ContextAwareValue m_cavElse;

public:
	
	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   * "if" [cav] - condition
	///   * "then" [cav, opt] - return if true. default null
	///   * "else" [cav, opt] - return if false. default null
	///
	void finalConstruct(Variant vConfig)
	{
		m_cavIf = vConfig.get("if");
		m_cavThen = vConfig.get("then", nullptr);
		m_cavElse = vConfig.get("else", nullptr);
	}

	///
	/// @copydoc IContextCommand::execute()
	///
	Variant execute(Variant vContext, Variant vParam) override
	{
		if ((bool)variant::convert<variant::ValueType::Boolean>(m_cavIf.get(vContext)))
			return m_cavThen.get(vContext);
		return m_cavElse.get(vContext);
	}
};


//////////////////////////////////////////////////////////////////////////
//
// CallCtxCmd
//

///
/// Command to call another command (usually ICommand) with complex preparation 
/// of parameters It provides ability to set parameters and context of the wrapped 
/// command.
///
class CallCtxCmd : public ObjectBase<CLSID_CallCtxCmd>, 
	public IContextCommand
{
private:

	//
	// The class encapsulates processing:
	//   source Variant (i.e. 'params'),
	//   its patching (i.e. 'ctxParams')
	//   and its cloning
	//
	class VariantWithVars
	{
	private:
		ContextAwareValue m_cavSrc; //< source

		struct Patch 
		{
			std::string sDstPath;
			ContextAwareValue cavValue;
		};

		std::vector<Patch> m_patches;
		bool m_fClone = true;

	public:
		void init(const ContextAwareValue& cavSrc, const Variant& vPatches, bool fClone);

		// calculates result.
		// returns nullopt if neither cavSrc nor vPatches was not specified
		std::optional<Variant> get(const Variant& vContext);
	};

	// Wrapped command (ICommand or IContextCommand or dynamic cmd)
	ObjPtr<IContextCommand> m_pWrappedCtxCmd;
	ObjPtr<ICommand> m_pWrappedCmd;
	ContextAwareValue m_cavWrappedDynCmd;

	VariantWithVars m_params;
	VariantWithVars m_ctx;

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   * "command" [obj] - Wrapped command.
	///     It should contain one of:
	///       - object which implements IContextCommand.
	///       - object which implements ICommand.
	///       - ContextAwareValue to get object which implements IContextCommand or ICommand
	///     If the object implements both, IContextCommand will be used.
	///   * "params" [cav, opt] - Arguments of the wrapped command
	///     If it not specified, null is used.
	///   * "ctxParams" [dict, opt] - Dictionary of cav. It describes variable parts of 'param'.
	///     A key is path to insert value. A value is cav which contains calculation rule.
	///     All values is added into 'param' at each execute().
	///   * "cloneParam" [bool, opt] - Explicit enabling or disabling cloning 'param'
	///     before execution of a wrapped command. Default value is true.
	///   * "ctx" [cav, opt] - vContext argument of the wrapped command
	///   * "ctxVars" [dict, opt] - Dictionary of cav. It describes variable parts of 'ctx'.
	///     A key is path to insert value. A value is cav which contains calculation rule.
	///     All values is added into 'ctx' at each execute().
	///   * "cloneCtx" [bool, opt] - Explicit enabling or disabling 'ctx' 
	///     before execution of a wrapped command. Default value is true.
	///
	/// If 'cmd' is IContextCommand and neither 'ctx' nor 'ctxVars' is not specified, 
	/// the current call context is passed. In this case the 'cloneCtx' option does not work.
	/// 
	void finalConstruct(Variant vConfig);

	///
	/// Executes the wrapped command.
	///
	/// @param vParam is not used
	/// @param vContext is used to calculate ContextAwareValues
	/// @returns Result of the wrapped command.
	///
	Variant execute(Variant vContext, Variant vParam) override;
};

//////////////////////////////////////////////////////////////////////////
//
// ForEachCtxCmd
//

///
/// Command to call another command (usually ICommand) for each element of the given data.
///
class ForEachCtxCmd : public ObjectBase<CLSID_ForEachCtxCmd>,
	public IContextCommand
{
private:

	// Wrapped command (ICommand or IContextCommand or dynamic cmd)
	ObjPtr<IContextCommand> m_pWrappedCtxCmd;
	ObjPtr<ICommand> m_pWrappedCmd;
	ContextAwareValue m_cavWrappedDynCmd;
	ContextAwareValue m_cavData;
	std::string m_sPath;

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   * "command" [obj] - Wrapped command.
	///     It should contain one of:
	///       - object which implements IContextCommand.
	///       - object which implements ICommand.
	///       - ContextAwareValue to get object which implements IContextCommand or ICommand
	///     If the object implements both, IContextCommand will be used.
	///   * "data" [cav] - Each element of the data will be supplied to the wrapped command as parameter.
	///   * "path" [str, opt] - If specified the data will be placed into context (instead of params).
	///     Use only with context-related commands. 
	///
	void finalConstruct(Variant vConfig);

	///
	/// Executes the wrapped command.
	///
	/// @param vParam is not used
	/// @param vContext is used to calculate ContextAwareValues
	/// @returns Result of the wrapped command.
	///
	Variant execute(Variant vContext, Variant vParam) override;
};

//////////////////////////////////////////////////////////////////////////
//
// MakeDictionaryCtxCmd
//

///
/// Command for construction of new dictionary with given context-aware values
///
class MakeDictionaryCtxCmd : public ObjectBase<CLSID_MakeDictionaryCtxCmd>,
	public IContextCommand
{
private:

	struct Value
	{
		std::string sPath;
		ContextAwareValue cavValue;
		Value(std::string_view _sPath, ContextAwareValue _cavValue) :
			sPath(_sPath), cavValue(_cavValue) {}
	};

	std::vector<Value> m_data;

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   * "data" [cav] - Each element of the data will be supplied to the wrapped command as parameter.
	///
	void finalConstruct(Variant vConfig);

	///
	/// Executes the wrapped command.
	///
	/// @param vParam is not used
	/// @param vContext is used to calculate ContextAwareValues
	/// @returns Result of the wrapped command.
	///
	Variant execute(Variant vContext, Variant vParam) override;
};

//////////////////////////////////////////////////////////////////////////
//
// MakeSequenceCtxCmd
//

///
/// Command for construction of new sequence with given context-aware values
///
class MakeSequenceCtxCmd : public ObjectBase<CLSID_MakeSequenceCtxCmd>,
	public IContextCommand
{
private:
	std::vector<ContextAwareValue> m_data;

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   * "data" [cav] - Each element of the data will be supplied to the wrapped command as parameter.
	///
	void finalConstruct(Variant vConfig);

	///
	/// Executes the wrapped command.
	///
	/// @param vParam is not used
	/// @param vContext is used to calculate ContextAwareValues
	/// @returns Result of the wrapped command.
	///
	Variant execute(Variant vContext, Variant vParam) override;
};

//////////////////////////////////////////////////////////////////////////
//
// CachedValueCtxCmd
//

///
/// Command returns a cached value from the context. If cached value hasn't been found,
/// it's created using the specified path and given context-aware value and is saved into
/// the context for further use.
///
class CachedValueCtxCmd : public ObjectBase<CLSID_CachedValueCtxCmd>,
	public IContextCommand
{
private:
	std::string m_sPath;
	ContextAwareValue m_cavValue;

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   * "path" [str] - a path in the context where the cached value will be placed to.
	///   * "value" [cav] - a context-aware value to be calulated and cached.
	///
	void finalConstruct(Variant vConfig);

	///
	/// Executes the wrapped command.
	///
	/// @param vParam is not used
	/// @param vContext is used to calculate ContextAwareValues
	/// @returns Result of the wrapped command.
	///
	Variant execute(Variant vContext, Variant vParam) override;
};

} // namespace cmd 
