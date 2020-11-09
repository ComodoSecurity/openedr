//
// edrav2.libcloud project
// 
// Author: Yury Ermakov (01.04.2019)
// Reviewer: Denis Bogdanov (18.04.2019)
//
///
/// @file Valkyrie Client Service objects declaration
///
/// @addtogroup cloud Cloud communication library
/// @{
#pragma once
#include <objects.h>
#include <valkyrie.hpp>
#include <net.hpp>

namespace openEdr {
namespace cloud {
namespace valkyrie {

///
/// Valkyrie client service.
///
class ValkyrieService : public ObjectBase<CLSID_ValkyrieService>,
	public IService,
	public IValkyrieClient,
	public ICommandProcessor
{
private:

	using Hash = std::string;

	static constexpr Size c_nHashSize = 20;
	static constexpr char c_sZeroHash[] = "0000000000000000000000000000000000000000";

	std::mutex m_mtxConfig;
	std::string m_sApiKey; // shall be used under m_mtxConfig
	std::string m_sEndpointId; // shall be used under m_mtxConfig
	std::string m_sUrl; // shall be used under m_mtxConfig
	std::atomic_bool m_fInitialized = false;

	std::unique_ptr<net::HttpClient> m_pHttp;
	std::mutex m_mtxHttp;
	std::string m_sSubmitToken;

	std::atomic_bool m_fStarted = false; // the service is started
	ObjPtr<sys::win::IFileInformation> m_pFileDataProvider;
	Size m_nSubmitRetryCount = 3;

	//
	//
	//
	struct CacheItem
	{
		Time time;
		CacheItem(Time _time) :
			time(_time) {}
	};

	std::unordered_map<Hash, CacheItem> m_cache;
	std::mutex m_mtxCache;

	void init();
	void updateSettings(Variant vConfig);
	bool isFileInCache(const Hash& hash);
	void addFileToCache(const Hash& hash);
	Variant getBasicInfo(std::string_view sHash);
	Variant submitToAutomaticAnalysis(ObjPtr<io::IReadableStream> pStream,
		std::filesystem::path pathFile, std::string_view sSubmitToken,
		Size nTimeout);

public:

	///
	/// Object final construction.
	///
	/// @param vConfig - a dictionary with the configuration including
	///	the following fields:
	///   **host** [opt] - Valkyrie server hostname.
	///   **port** [opt] - Valkyrie server port number.
	///   **apiKey** [opt] - API key for access to Valkyrie.
	///
	void finalConstruct(Variant vConfig);

	// IService

	/// @copydoc IService::loadState(Variant)
	void loadState(Variant vState) override;
	/// @copydoc IService::saveState()
	Variant saveState() override;
	/// @copydoc IService::start()
	void start() override;
	/// @copydoc IService::stop()
	void stop() override;
	/// @copydoc IService::shutdown()
	void shutdown() override;

	// IValkyrieClient

	/// @copydoc IValkyrieClient::submit(Variant)
	bool submit(Variant vFile) override;

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute(Variant,Variant)
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace valkyrie
} // namespace cloud
} // namespace openEdr

/// @} 
