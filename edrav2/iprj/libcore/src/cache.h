//
// edrav2.libcore
// 
// Author: Yury Ermakov (05.06.2019)
// Reviewer:
//
/// @file Cache objects declaration
/// @addtogroup cache Cache
/// @{
///
#pragma once
#include <objects.h>
#include <command.hpp>

namespace openEdr {

///
/// String cache class
///
class StringCache : public ObjectBase<CLSID_StringCache>,
	public IDataReceiver,
	public ICommandProcessor
{

private:

	std::unordered_set<std::string> m_data;
	std::mutex m_mtxData;
	Size m_nMaxSize = 100;

	bool contains(const std::string& sData);

public:

	///
	/// Object final construction
	///
	/// @param vConfig The object's configuration including the following fields:
	///   **maxSize** [int,opt] - the maximum possible number of elements (by default, 100)
	///
	void finalConstruct(Variant vConfig);

	// IDataReceiver

	///
	/// @copydoc IDataReceiver::put()
	///
	void put(const Variant& vData) override;

	// ICommandProcessor

	///
	/// @copydoc ICommandProcessor::execute()
	///
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace openEdr
/// @} 
