//
// edrav2.libcore project
//
// Commands subsystem support
//
// Autor: Yury Podpruzhnikov (17.12.2018)
// Reviewer: Denis Bogdanov (18.01.2019)
//
#pragma once
#include "object.hpp"
#include "variant.hpp"
#include "threadpool.hpp"

namespace cmd {

///
/// @brief Interface of any command.
///
class ICommand : OBJECT_INTERFACE
{
public:
	
	///
	/// @brief Execute a command.
	///
	/// @param vParam Parameter(-s) of the command. Its content depends on the command.
	/// @return Result of command execution.
	///
	virtual Variant execute(Variant vParam = Variant()) = 0;

	///
	/// @brief Return an command's info block.
	///
	/// @return String from **info** field.
	///
	virtual std::string getDescriptionString() = 0;
};

///
/// @brief Fast constructor of Command object
/// 
/// @param vCommandDescriptor A command descriptor (see cmd::Command) or object 
///                           which implements ICommand
/// @return Command object
///
extern ObjPtr<ICommand> createCommand(Variant vCommandDescriptor);

///
/// @brief Fast constructor of Command object
/// 
/// @param vProcessor "processor" field from a command descriptor (see cmd::Command)
/// @param vCommand "command" field from a command descriptor (see cmd::Command)
/// @param vParams "params" field from a command descriptor (see cmd::Command)
/// @return Command object
///
extern ObjPtr<ICommand> createCommand(Variant vProcessor, Variant vCommand, Variant vParams = Variant());

///
/// @brief Fast exec of Command
/// 
/// @param vCommandDesc - A command descriptor (see cmd::Command) or object 
///                       which implements ICommand
/// @return Result of command execution
///
inline Variant execCommand(Variant vCommandDesc)
{
	return createCommand(vCommandDesc)->execute();
}

///
/// Runs the specified command asynchronously
///
/// @param [in] pCmd - A command for async call.
///
inline void execCommandAsync(ObjPtr<ICommand> pCmd)
{
	getCoreThreadPool().run(pCmd);
}

///
/// Runs the specified command asynchronously
///
/// @param vCommandDesc - A command descriptor (see cmd::Command) or object 
///                       which implements ICommand
///
inline void execCommandAsync(Variant vCommandDesc)
{
	getCoreThreadPool().run(createCommand(vCommandDesc));
}

///
/// Runs the specified command asynchronously after exceeding of the specified timeout.
/// If command returned true then it will be called again in the specified timeout.
///
/// @param [in] tDuration pimeout for call (in milliseconds)
/// @param [in] pCmd - A command for async call.
/// @return Pointer to timer context object
///
[[nodiscard]]
inline ThreadPool::TimerContextPtr execCommandAsync(Size nDuration, ObjPtr<ICommand> pCmd)
{
	return getCoreThreadPool().runWithDelay(nDuration, pCmd);
}

///
/// Runs the specified command asynchronously after exceeding of the specified timeout.
/// If command returned true then it will be called again in the specified timeout.
///
/// @param [in] tDuration pimeout for call (in milliseconds)
/// @param vCommandDesc - A command descriptor (see cmd::Command) or object 
///                       which implements ICommand
/// @return Pointer to timer context object
///
[[nodiscard]]
inline ThreadPool::TimerContextPtr execCommandAsync(Size nDuration, Variant vCommandDesc)
{
	return getCoreThreadPool().runWithDelay(nDuration, createCommand(vCommandDesc));
}

///
/// @brief Fast exec of Command
/// 
/// @param vProcessor "processor" field from a command descriptor (see cmd::Command)
/// @param vCommand "command" field from a command descriptor (see cmd::Command)
/// @param vParams "params" field from a command descriptor (see cmd::Command)
/// @return Result of command execution
///
inline Variant execCommand(Variant vProcessor, Variant vCommand, Variant vParams = Variant())
{
	return createCommand(vProcessor, vCommand, vParams)->execute();
}

///
/// @brief Interface of object which provides/processes commands.
///
class ICommandProcessor : OBJECT_INTERFACE
{
public:

	///
	/// Execute a specified command.
	///
	/// @param vCommand - Command identificator. Usually string.
	/// @param vParams - Parameter(-s) of the command. Its content depends on the command.
	/// @return Result of command execution.
	///
	virtual Variant execute(Variant vCommand, Variant vParams = Variant()) = 0;
};

///
/// @brief Interface of context aware command
///
class IContextCommand : OBJECT_INTERFACE
{
public:

	///
	/// @brief Execute a command.
	///
	/// @param vContext Call context.
	/// @param vParam Parameter(-s) of the command. Its content depends on the command.
	/// @return Result of command execution.
	///
	virtual Variant execute(Variant vContext, Variant vParam = Variant()) = 0;
};


///
/// Special context aware value, which can be calculated on access
/// Format of parameters of Context-aware-commands and Scenario.
///
/// ContextAwareValue is created from a source Variant (vSource passed into constructor or opeartor==).
/// ContextAwareValue has method "Variant get(Variant vContext)" 
/// which calculates (possibly using vContext) and returns the result value (vResult).
///   * If vSource is **Object implementing IContextCommand**, 
///     vResult is result of IContextCommand::execute(vContext, nullptr).
///   * If vSource is **Object implementing ICommand**,
///     vResult is result of ICommand::execute(nullptr).
///   * If vSource is **Dictionary with field "$path"** (and optionaly with "$default"),
///     if "$default" is present, vResult is getByPath(vContext, source["$path"], source.get("$default")).
///     if "$default" is absent, vResult is getByPath(vContext, source["$path"]).
///     vResult is result of getByPath(vContext, source["$path"], source.get("$default", null)).
///   * If vSource is **Dictionary with field "$gpath"** (and optionaly with "$default"),
///     if "$default" is present, vResult is getCatalogData(source["$gpath"], source.get("$default")).
///     if "$default" is absent, vResult is getCatalogData(source["$gpath"]).
///   * If vSource is **Dictionary with field "$val"**,
///     vResult is source["$val"]. This way allows to use ICommand, IContextCommand o
///     r DataProxy as the result value (without calculation).
///   * **Otherwise**, vResult is vSource. 
///
/// ContextAwareValue parses vSource at creation and saves a way and additional data to calculate vResult. 
/// 
class ContextAwareValue
{
private:
	enum class Type : uint8_t
	{
		None,
		Command,
		ContextCommand,
		Value,
		ContextPath,
		GlobalPath,
	};

	Type m_eType = Type::None;

	ObjPtr<ICommand> m_pCommand;
	ObjPtr<IContextCommand> m_pContextCommand;
	Variant m_vValue;
	std::optional<Variant> m_voptDefault;

	//
	// Initialization
	//
	void init(const Variant& vValue)
	{
		m_pContextCommand = queryInterfaceSafe<IContextCommand>(vValue);
		if (m_pContextCommand)
		{
			m_eType = Type::ContextCommand;
			return;
		}

		m_pCommand = queryInterfaceSafe<ICommand>(vValue);
		if (m_pCommand)
		{
			m_eType = Type::Command;
			return;
		}

		if (vValue.isDictionaryLike())
		{
			if (vValue.has("$val"))
			{
				// Need to use has() but not get() with default value. 
				// Checking result != default value leads to DataProxy calculation
				m_vValue = vValue.get("$val");
				m_eType = Type::Value;
				return;
			}

			if (vValue.has("$path"))
			{
				// Need to use has() but not get() with default value. 
				// Checking result != default value leads to DataProxy calculation
				m_vValue = vValue.get("$path");
				if (m_vValue.getType() != Variant::ValueType::String)
					error::InvalidArgument(SL, FMT("Invalid '$path' parameter. " << m_vValue)).throwException();

				m_voptDefault = vValue.getSafe("$default");
				m_eType = Type::ContextPath;
				return;
			}

			if (vValue.has("$gpath"))
			{
				// Need to use has() but not get() with default value. 
				// Checking result != default value leads to DataProxy calculation
				m_vValue = vValue.get("$gpath");
				if (m_vValue.getType() != Variant::ValueType::String)
					error::InvalidArgument(SL, FMT("Invalid '$path' parameter. " << m_vValue)).throwException();
				m_voptDefault = vValue.getSafe("$default");
				m_eType = Type::GlobalPath;
				return;
			}
		}

		m_vValue = vValue;
		m_eType = Type::Value;
	}

public:
	ContextAwareValue() = default;
	ContextAwareValue(const ContextAwareValue&) = default;
	ContextAwareValue(ContextAwareValue&&) = default;
	ContextAwareValue& operator=(const ContextAwareValue& vValue) = default;
	ContextAwareValue& operator=(ContextAwareValue&& vValue) = default;

	///
	/// Constructor
	///
	ContextAwareValue(const Variant& vValue)
	{
		init(vValue);
	}

	//
	//
	//
	template<typename T, IsSameType<T, std::optional<Variant>, int> = 0 >
	ContextAwareValue(const T& vValue)
	{
		if(vValue)
			init(*vValue);
	}

	//
	//
	//
	ContextAwareValue& operator=(const Variant& vValue)
	{
		init(vValue);
		return *this;
	}

	//
	//
	//
	template<typename T, IsSameType<T, std::optional<Variant>, int> = 0 >
	ContextAwareValue& operator=(const T& vValue)
	{
		if (vValue)
			init(*vValue);
		return *this;
	}

	///
	/// Calculation result value
	///
	Variant get(const Variant& vContext) const
	{
		switch (m_eType)
		{
			case Type::ContextCommand:
				return m_pContextCommand->execute(vContext);
			case Type::Command:
				return m_pCommand->execute();
			case Type::Value:
				return m_vValue;
			case Type::ContextPath:
				return (m_voptDefault.has_value()) ? getByPath(vContext, m_vValue, *m_voptDefault) :
					getByPath(vContext, m_vValue);
			case Type::GlobalPath:
				return (m_voptDefault.has_value()) ? getCatalogData(m_vValue, *m_voptDefault) :
					getCatalogData(m_vValue);
		}

		error::InvalidUsage(FMT("Invalid type <" << +(uint8_t)m_eType << ">")).throwException();
	}

	bool isValid() const
	{
		return m_eType != Type::None;
	}
};

} // namespace cmd 

