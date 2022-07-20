//
// EDRAv2.libcore project
//
// Implementation of the Command object
//
// Author: Yury Podpruzhnikov (09.01.2019)
// Reviewer: Denis Bogdanov (15.01.2019)
//
#include "pch.h"
#include "command.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "core"

namespace cmd {

//
//
//
ObjPtr<ICommandProcessor> Command::getProcessor()
{
	Variant vProcessor = m_vProcessor;
	// try get ICommandProcessor
	{
		auto pProcessor = queryInterfaceSafe<ICommandProcessor>(vProcessor);
		if (pProcessor)
			return pProcessor;
	}

	// try to get service name
	if (vProcessor.getType() == Variant::ValueType::String)
	{
		std::string sServiceName = vProcessor;
		return queryInterface<ICommandProcessor>(getCatalogData(sServiceName));
	}
	
	// try to create object
	if (vProcessor.getType() == Variant::ValueType::Dictionary)
	{
		// Create processor from descriptor
		return queryInterface<ICommandProcessor>(createObject(vProcessor));
	}

	error::InvalidArgument(SL, "Invalid command processor").throwException();
}

//
//
//
void Command::finalConstruct(Variant vConfig)
{
	m_vProcessor = vConfig["processor"];
	m_vCommand = vConfig["command"];
	m_vDefaultParam = vConfig.get("params", Variant());
	m_sInfo = vConfig.get("info", "");
	m_fConstParams = vConfig.get("constParams", false);
	if (vConfig.get("cacheProcessor", false))
		m_pProcessor = getProcessor();
}

//
//
//
std::string Command::getDescriptionString()
{
	return m_sInfo;
}

//
//
//
Variant Command::execute(Variant vParams)
{
	Variant vCompleteParams = m_vDefaultParam.clone();
	if (!vParams.isNull() && !m_fConstParams)
	{
		if (vCompleteParams.isNull())
			vCompleteParams = vParams;
		else
			vCompleteParams.merge(vParams, Variant::MergeMode::All);
	}
	if (m_pProcessor)
		return m_pProcessor->execute(m_vCommand, vCompleteParams);
	return getProcessor()->execute(m_vCommand, vCompleteParams);
}

//
//
//
ObjPtr<ICommand> createCommand(Variant vCommandDescriptor)
{
	auto pCommand = queryInterfaceSafe<ICommand>(vCommandDescriptor);
	if (pCommand)
		return pCommand;
	if (vCommandDescriptor.has("clsid"))
		return queryInterface<ICommand>(createObject(vCommandDescriptor));
	return createObject<Command>(vCommandDescriptor);
}

//
//
//
ObjPtr<ICommand> createCommand(Variant vProcessor, Variant vCommand, Variant vParams)
{
	return createObject<Command>(Dictionary({ 
		{"processor", vProcessor},
		{"command", vCommand},
		{"params", vParams},
	}));
}

//////////////////////////////////////////////////////////////////////////
//
// CommandDataReceiver
//


//
//
//
void CommandDataReceiver::finalConstruct(Variant vConfig)
{
	m_pCommand = createCommand(vConfig["command"]);
}

//
//
//
void CommandDataReceiver::put(const Variant& vData)
{
	(void)m_pCommand->execute(Dictionary({ {"data", vData} }));
}

} // namespace cmd 
