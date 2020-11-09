//
// edrav2.libcore
// 
// Author: ??? ??? (99.99.2019)
// Reviewer: ??? ??? (99.99.2019)
//
///
/// @file ChangeMe implementation
/// @addtogroup ???
/// @{
///
#include "pch.h"
#include "changeme.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "changeme"

namespace openEdr {
namespace templates {

//
//
//
void ChangeMe::finalConstruct(Variant vConfig)
{
	// m_Var = vConfig["var"];
}

//
//
//
void ChangeMe::put(const Variant& /*vData*/)
{
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * '???'
///
/// #### Supported commands
///
Variant ChangeMe::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant ChangeMe::execute()
	///
	/// ##### command1()
	/// Put here the description of a command1.
	///
	if (vCommand == "command1")
	{
		return {};
	}

	///
	/// @fn Variant ChangeMe::execute()
	///
	/// ##### command2()
	/// Put here the description of a command2.
	///   * param1 [int] - param1 description;
	///   * param2 [int] - param2 description;
	///   * param3 [str,opt] - param3 description. Default valuse is ???; 
	/// 
	else if (vCommand == "command2")
	{
		return true;
	}

	error::OperationNotSupported(SL, 
		FMT("??? doesn't support command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // namespace templates
} // namespace openEdr 
/// @}
