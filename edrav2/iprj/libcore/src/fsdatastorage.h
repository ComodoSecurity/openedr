//
// edrav2.libcore
// 
// Author: Denis Bogdanov (10.03.2019)
// Reviewer: Yury Podpruzhnikov (13.03.2019)
//
///
/// @file File system data storage class declaration
///
/// @addtogroup io IO subsystem
/// @{
#pragma once
#include <objects.h>
#include <command.hpp>
#include <common.hpp>

namespace openEdr {
namespace io {

///
/// DirectoryDataProvider allows to enumerate files in the specified
/// directory, loads them as variants and returns using IDataProvider interface.
///
class FsDataStorage : public ObjectBase<CLSID_FsDataStorage>,
	public ICommandProcessor,
	public IDataProvider,
	public IDataReceiver,
	public IInformationProvider
{
private:
	std::mutex m_mtxData;
	bool m_fDeleteOnGet = false;
	bool m_fPrettyOutput = false;
	uint32_t m_nPutIndex = 0;
	Size m_nMaxSize = c_nMaxSize;
	std::filesystem::path m_pathFileData;
	std::deque<std::wstring> m_fileNames;

	void erase();

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - the object configuration includes the following fields:
	///   **path** - path to the directoru with data;
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);

	// IInformationProvider

	/// @copydoc IInformationProvider::getInfo()
	virtual Variant getInfo() override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	virtual Variant execute(Variant vCommand, Variant vParams) override;

	// IDataProvider

	/// @copydoc IDataProvider::get()
	virtual std::optional<Variant> get() override;

	// IDataReceiver

	/// @copydoc IDataReceiver::put()
	virtual void put(const Variant& vData) override;
};

} // namespace io
} // namespace openEdr
/// @} 
