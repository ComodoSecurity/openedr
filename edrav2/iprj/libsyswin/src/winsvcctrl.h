//
// edrav2.libsyswin project
//
// Author: Yury Podpruzhnikov (04.01.2019)
// Reviewer: Denis Bogdanov (06.02.2019)
//
///
/// @file Windows service controller declaration
///
/// @addtogroup syswin System library (Windows)
/// @{
#pragma once
#include "objects.h"

namespace openEdr {
namespace sys {
namespace win {

///
/// Windows SCM wrapper class.
///
// FIXME: Describe command for doxygen
class WinServiceController: public ObjectBase<CLSID_WinServiceController>,
	public ICommandProcessor
{
public:

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace win
} // namespace sys
} // namespace openEdr 

/// @}
