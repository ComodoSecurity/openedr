//
// edrav2.libcloud project
// 
// Author: Denis Bogdanov (16.03.2019)
// Reviewer:
//
///
/// @file Legacy cloud service class declaration
///
/// @addtogroup cloud Cloud communication library
/// @{
#pragma once
#include <objects.h>
#include <net.hpp>

namespace cmd {
namespace cloud {
namespace legacy {

///
/// The class provides interaction with EDR backend portal (legacy).
///
class CloudService : public ObjectBase<CLSID_LegacyCloudService>,
	public ICommandProcessor,
	public IService,
	public IInformationProvider,
	public ICommand
{

private:

	std::string m_sRootUrl;
	std::string m_sCurrStatus;
	std::string m_sPolicyCatalogPath;
	std::string m_sCurrVersion;
	ThreadPool m_threadPool{ "CloudService::CommunitctionPool" };
	ThreadPool::TimerContextPtr m_pTimerCtx;
	Time m_lastHertbeatTime = 0;
	net::HttpClient m_httpClient;
	Dictionary m_cmdHandlers;
	Sequence m_seqNetInfo;
	uint64_t m_nAvgDuration = 0;
	Size m_nHeartbeatCount = 0;
	Size m_nNetworkUpdateMask = 0;
	Size m_nPeriod = 30000; // 30 second

protected:

	Variant doHeartbeat(bool fFastTrack = false);
	Variant doHeartbeat2(bool fFastTrack);
	Variant getConfig();

	//
	//
	//
	virtual ~CloudService() override {}

public:

	///
	/// Object final construction.
	///
	/// @param vConfig The object configuration includes the following fields:
	///   **field1** - ???
	/// @throw error::InvalidArgument In case of invalid configuration in vConfig.
	///
	void finalConstruct(Variant vConfig);


	// IService

	/// @copydoc IService::loadState()
	virtual void loadState(Variant vState) override {}
	/// @copydoc IService::saveState()
	virtual Variant saveState() override { return {}; }
	/// @copydoc IService::start()
	virtual void start() override;
	/// @copydoc IService::start()
	virtual void stop() override;
	/// @copydoc IService::shutdown()
	virtual void shutdown() override;

	// ICommand

	/// @copydoc ICommand::execute()
	virtual Variant execute(Variant vParam) override;
	/// @copydoc ICommand::getInfo()
	virtual std::string getDescriptionString() override { return {}; }

	// ICommandProcessor

	/// @copydoc ICommandProcessor::execute()
	virtual Variant execute(Variant vCommand, Variant vParams) override;

	// IInformationProvider

	/// @copydoc - IInformationProvider::getInfo()
	virtual Variant getInfo() override;
};

} // namespace legacy
} // namespace cloud
} // namespace cmd
/// @} 
