//
// edrav2.libcloud project
// 
// Author: Denis Bogdanov (16.03.2019)
// Reviewer:
//
///
/// @file Cloud service class declaration
///
/// @addtogroup cloud Cloud communication library
/// @{
#pragma once
#include <objects.h>
#include <net.hpp>

namespace cmd {
namespace cloud {

///
/// The class provides interaction with EDR backend portal.
///
class CloudService : public ObjectBase<CLSID_CloudService>,
	public ICommandProcessor,
	public IService,
	public IInformationProvider,
	public ICommand
{

private:
	std::recursive_mutex m_mtxData;
	std::string m_sPolicyCatalogPath;
	std::string m_sCurrVersion;
	ThreadPool m_threadPool{ "CloudService::CommunitctionPool" };
	ThreadPool::TimerContextPtr m_pTimerCtx;
	Time m_lastHertbeatTime = 0;
	net::HttpClient m_httpClient;
	uint64_t m_nAvgDuration = 0;
	Size m_nHeartbeatCount = 0;
	Size m_nNetworkUpdateMask = 0;
	Size m_nPeriod = 30000; // 30 second
	Variant m_seqLastIp4;
	std::wstring m_sLastActiveUser;
	bool m_fFirstStart = true;
	bool m_fFinished = false;

protected:

	Variant callHeartbeat();
	Variant doHeartbeat();
	Variant getConfig(Variant vParams = Dictionary());
	Variant getIdentity(Variant vParams = Dictionary());
	Variant getPolicy(Variant vParams = Dictionary());
	Variant getToken(Variant vParams = Dictionary());
	Variant enroll(Variant vParams = Dictionary());
	Variant reportEndpointInfo(bool fFull);
	bool checkCredentials();
	void reportOffline();
	void reportUninstall(bool fFailSafe);
	void confirmCommand(std::string_view sCommandId);
	void updateSettings(Variant vConfig);


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

} // namespace cloud
} // namespace cmd
/// @} 
