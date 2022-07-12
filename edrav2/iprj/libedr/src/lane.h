//
// edrav2.libedr
// 
// Author: Podpruzhnikov Yury (10.09.2019)
// Reviewer: Denis Bogdanov (19.09.2019)
//
/// @addtogroup edr
/// @{
#pragma once
#include <objects.h>

namespace cmd {
namespace edr {

///
/// Operations with lanes.
///
/// Implements ICommand. All parameter passed to finalConstruct.
///
class LaneOperation : public ObjectBase<CLSID_LaneOperation>,
	public ICommand
{
private:
	std::function<Variant()> m_fnOperation;

public:

	void finalConstruct(Variant vConfig);

	// ICommand

	/// @copydoc ICommand::execute(Variant)
	Variant execute(Variant vParam = Variant()) override;
	/// @copydoc ICommand::getDescriptionString(Variant)
	std::string getDescriptionString() override;
};

} // namespace edr
} // namespace cmd

/// @}
