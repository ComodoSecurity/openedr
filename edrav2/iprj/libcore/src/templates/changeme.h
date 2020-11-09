//
// edrav2.libcore
// 
// Author: ??? ??? (99.99.2019)
// Reviewer: ??? ??? (99.99.2019)
//
/// @file ??? class declaration
/// @addtogroup ???
/// @{
///
#pragma once
#include <objects.h>
#include <command.hpp>
#include <common.hpp>

//
// DELETE THIS COMMENT!
// These files are templates for basic libcore object.
// Please make a copy if h and cpp file, rename it and put into your src directory.
// Fill free to delete the interfaces unneccessary for you.
//

namespace openEdr {
namespace templates {

///
/// ChangeMe brief description
///
/// ChangeMe full description place here
///
class ChangeMe : public ObjectBase<CLSID_ChangeMe>,
	public ICommandProcessor,
	public IDataReceiver
{

private:

protected:
	
	//
	//
	//
	virtual ~ChangeMe() override
	{};

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object configuration includes the following fields:
	///   **field1** - ???
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// ICommandProcessor

	///
	/// @copydoc ICommandProcessor::execute()
	///
	virtual Variant execute(Variant vCommand, Variant vParams) override;

	// IDataReceiver

	///
	/// @copydoc IDataReceiver::put()
	///
	virtual void put(const Variant& vData) override;
};

} // namespace templates
} // namespace openEdr
/// @} 
