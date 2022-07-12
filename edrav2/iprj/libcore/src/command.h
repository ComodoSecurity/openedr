//
// EDRAv2.libcore project
//
// Implementation of the Command object
//
// Author: Yury Podpruzhnikov (09.01.2019)
// Reviewer: Denis Bogdanov (15.01.2019)
//
#pragma once
#include <command.hpp>
#include <objects.h>
#include <common.hpp>

namespace cmd {

///
/// @brief Wrapper of any command. ICommand implementation.
/// 
/// It prepares command data in finalConstruct: processor, command, etc.
/// And use this data every execution
///
/// **command descriptor:**
/// * "processor" - one of:
///     * Object which implements ICommandProvider
///     * String with name of service which implements ICommandProvider
/// * "command" - A command identificator (usually string). This value will be pass to ICommandProvider::exec.
/// * "params" - (optional) parameters default values.
/// * "cacheProcessor" [bool, opt] - force a processor to be created only once and then cached for further usage
///     (default: false).
///
/// **A command execution steps:**
/// *# Parsing of command descriptor at Command object creation.
///   *# ICommandProvider and other info is extracted/created. This data will be used on every call of ICommand::exec.
/// *# A command Execution on a call of ICommand::exec
///   *# Preparing command parameters. 
///     * Cloning of default params (from descriptor)
///     * if vParams (parameter of ICommand::exec) is not null, it is merged into default parameters
///   *# Execution of ICommandProcessor::exec with specified command and parameters
///
class Command : public ObjectBase<CLSID_Command>, 
	public ICommand
{
private:

	///
	/// Get ICommandProcessor from command descriptor
	///
	ObjPtr<ICommandProcessor> getProcessor();

	Variant m_vProcessor;
	Variant m_vCommand;
	Variant m_vDefaultParam;
	std::string m_sInfo;
	bool m_fConstParams{ false };
	ObjPtr<ICommandProcessor> m_pProcessor;

public:
	
	///
	/// @param[in] "vConfig" A command descriptor.
	///
	void finalConstruct(Variant vConfig);

	// ICommand interface
	virtual Variant execute(Variant vParams) override;
	virtual std::string getDescriptionString() override;

};


///
/// Adapter of IDataReceiver for any ICommand
/// 
/// It provides IDataReceiver and wraps any command.
/// It calls the command on every call of the IDataReceiver::put.
/// The object passes to command Dictionary which contains the "data" 
/// field  with  value passed into IDataReceiver::put()
///
class CommandDataReceiver : public ObjectBase<CLSID_CommandDataReceiver>, 
	public IDataReceiver
{
private:
	ObjPtr<ICommand> m_pCommand;
public:
	
	///
	/// @param[in] "command" - ICommand object or a command descriptor.
	///
	void finalConstruct(Variant vConfig);

	// IDataReceiver
	void put(const Variant& vData) override;
};

} // namespace cmd 
