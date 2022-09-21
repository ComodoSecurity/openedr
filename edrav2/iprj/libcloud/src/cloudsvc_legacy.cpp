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
#include "pch.h"
#include "cloudsvc_legacy.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "cloudsvc"

namespace cmd {
namespace cloud {
namespace legacy {

const char c_nCloudServiceName[] = "heartbeat";
const char c_nVersionServiceName[] = "version-details";
const char c_nConfigServiceName[] = "settings";


//
//
//
void CloudService::finalConstruct(Variant vConfig)
{
	if (!vConfig.isDictionaryLike()) error::InvalidArgument(SL, 
		"finalConstruct() supports only dictionary as a parameter").throwException();

	m_sRootUrl = vConfig.get("rootUrl",
		getCatalogData("app.config.cloud.rootLegacyUrl", ""));
	if (m_sRootUrl.empty()) error::InvalidArgument(SL,
		"Parameter <rootUrl> is not specified").throwException();

	m_nNetworkUpdateMask = vConfig.get("networkUpdateMask", m_nNetworkUpdateMask);
	m_sPolicyCatalogPath = vConfig.get("policyCatalogPath", "app.config.policy");
	m_cmdHandlers = vConfig.get("commandHandlers", m_cmdHandlers);
	m_sCurrVersion = vConfig.get("currVersion", CMD_STD_VERSION);
	m_nPeriod = vConfig.get("period", m_nPeriod);

	m_httpClient.setRootUrl(m_sRootUrl);
	m_sCurrStatus = "Online";
	m_seqNetInfo = net::getNetworkAdaptorsInfo(true, true, true);
}

//
//
//
void CloudService::start()
{
	CMD_TRY
	{
		(void) doHeartbeat();
	}
	CMD_PREPARE_CATCH
	catch (error::Exception& e)
	{ 
		e.log(SL);
	}
	m_pTimerCtx = runWithDelay(m_nPeriod,
		[](ObjPtr<CloudService> pThis) -> bool 
		{
			CMD_TRY
			{
				(void)pThis->doHeartbeat();
			}
			CMD_PREPARE_CATCH
			catch (error::Exception& e)
			{
				e.log(SL);
			}
			return true;
		}, getPtrFromThis(this));
	subscribeToMessage("AppFinishing", getPtrFromThis(this), "CloudService");
}

//
// Messages handler
//
Variant CloudService::execute(Variant vParam)
{
	if (vParam["messageId"] != "AppFinishing") return {};
	LOGLVL(Detailed, "Send final heartbeat");
	m_sCurrStatus = "Offline";
	(void)doHeartbeat(true);
	return {};
}


//
//
//
void CloudService::stop()
{
	m_pTimerCtx.reset();
}

//
//
//
void CloudService::shutdown()
{
}

//
//
//
Variant CloudService::getConfig()
{
	std::string sToken = getCatalogData("app.config.license.token");
	std::string sMachineId = getCatalogData("app.config.license.machineId");
	//REMOVED: config request
	Variant vResult;
	
	// First start
	if (!getCatalogDataSafe("app.config.extern.endpoint.cloud.endpointId").has_value())
	{
		if (!vResult.has("endpointID") || !vResult.has("customerID"))
			error::InvalidFormat("Response shall have <endpointID> and <customerID> fields").throwException();
		putCatalogData("app.config.extern.endpoint.license.endpointId", vResult["endpointID"]);
		putCatalogData("app.config.extern.endpoint.license.customerId", vResult["customerID"]);
		putCatalogData("app.config.extern.endpoint.license.token", sToken);
		putCatalogData("app.config.extern.endpoint.license.machineId", sMachineId);
	}

	if (vResult.has("AWSAccessKey"))
		putCatalogData("app.config.extern.endpoint.cloud.aws.accessKey", vResult["AWSAccessKey"]);

	if (vResult.has("AWSSecretKey"))
		putCatalogData("app.config.extern.endpoint.cloud.aws.secretKey", vResult["AWSSecretKey"]);

	if (vResult.has("AWSDeliveryStream"))
		putCatalogData("app.config.extern.endpoint.cloud.aws.deliveryStream", vResult["AWSDeliveryStream"]);

	if (vResult.has("valkyrie_api_key"))
		putCatalogData("app.config.extern.endpoint.cloud.valkyrie.apiKey", vResult["valkyrie_api_key"]);

	sendMessage(Message::CloudConfigurationIsChanged, {});
	return vResult;
}

//
//
//
Variant CloudService::doHeartbeat(bool fFastTrack)
{
	using namespace std::chrono;
	auto timeStart = steady_clock::now();
	m_lastHertbeatTime = getCurrentTime();
	m_nHeartbeatCount++;
	if ((m_nNetworkUpdateMask == 0 || (m_nHeartbeatCount & m_nNetworkUpdateMask)) && 
		!fFastTrack)
	{
		LOGLVL(Detailed, "Update network information");
		m_seqNetInfo = net::getNetworkAdaptorsInfo(true, true, true);
	}
	auto vRes = doHeartbeat2(fFastTrack);
	auto duration = duration_cast<milliseconds>(steady_clock::now() - timeStart);
	m_nAvgDuration = ((m_nAvgDuration * (m_nHeartbeatCount - 1)) + duration.count()) / m_nHeartbeatCount;
	return vRes;
}

//
//
//
Variant CloudService::doHeartbeat2(bool fFastTrack)
{
	LOGLVL(Detailed, "Send heartbeart request <" << m_sCurrStatus << ">");

	Variant seqIp4 = Sequence();
	for (auto& vInfo : m_seqNetInfo)
		seqIp4.merge(vInfo["ip4"], variant::MergeMode::All);

	auto pUDP = queryInterface<sys::win::ISessionInformation>(queryService("userDataProvider"));
	auto seqSessions = pUDP->getSessionsInfo();
	std::string sUserName;
	for (auto& vSession : seqSessions)
	{
		if (vSession.get("isLocalConsole", false))
		{ 
			sUserName = vSession["user"]["name"];
			break;
		}
		else if (sUserName.empty())
			sUserName = vSession["user"]["name"];
	}
	if (sUserName.empty())
		sUserName = "unknown";

	auto vData = Dictionary({
		{"agentID", getCatalogData("app.config.license.endpointId")},
		{"customerID", getCatalogData("app.config.license.customerId")},
		{"agentVersion", m_sCurrVersion},
		{"computerName", getCatalogData("os.hostName", "unknown")},
		{"domain", getCatalogData("os.domainName", "unknown")},
		{"operatingSystem", getCatalogData("os.osName", "Unknown OS")},
		{"lastReboot", getIsoTime(getCatalogData("os.bootTime", 0))},
		{"connectionStatus", m_sCurrStatus},
		{"localIPAddress", seqIp4},
		{"macAddress", m_seqNetInfo.get(0, Dictionary()).get("mac", "") },
		// TODO: Add real name after adding receiving user info function
		{"loggedOnUser", sUserName},
		{"policyHash", getCatalogData(m_sPolicyCatalogPath + ".source.hash", "00000000000000000000000000000000")},
	});
			
	//REMOVED: heartbit request
	Variant vResult;

	LOGLVL(Detailed, "Receive heartbeat response (status <" <<
		vResult.get("status", "unknown") << ">, command <" <<
		vResult.get("command", Variant()) << ">)");

	if (fFastTrack)
		return vResult;

	auto vCommand = vResult.get("command", Variant());
	if (!vCommand.isNull())
	{
		// Process "uninstall"
		if (vCommand == "uninstall")
		{
			LOGLVL(Detailed, "Process application uninstall signal");
			sendMessage(Message::RequestAppUninstall, {});
		}
		else
		{
			error::OperationNotSupported(SL, FMT("Unknown cloud command <" << 
				vCommand << ">")).log();
		}
		return vResult;
	}
	
	std::string sVer = vResult.get("latestVersion", m_sCurrVersion);
	if (compareVersion(sVer, m_sCurrVersion) > 0)
	{
		LOGLVL(Detailed, "Process application update signal");
		LOGLVL(Detailed, "Send version-details request");
		//REMOVED: Do version request
		Variant vVerInfo;
		
		if (!vVerInfo.has("version") || !vVerInfo.has("download_url"))
			error::InvalidFormat("Response shall have <version> and <download_url> fields").throwException();
		auto vHead = m_httpClient.head(vVerInfo["download_url"]);
		putCatalogData("app.update.version", vVerInfo["version"]);
		putCatalogData("app.update.url", vVerInfo["download_url"]);
		if (vHead.has("contentLength"))
			putCatalogData("app.update.size", vHead["contentLength"]);		
		sendMessage(Message::RequestAppUpdate, getCatalogData("app.update"));
		return vResult;
	}

	if (vResult.get("status", "unknown") == "NEWPOLICY")
	{
		LOGLVL(Detailed, "Process policy update signal");
		if (!vResult.has("newPolicy") || !vResult.has("newPolicyHash"))
			error::InvalidFormat("Response shall have <newPolicy> and <newPolicyHash> fields").throwException();
		vResult.get("newPolicy").put("hash", vResult["newPolicyHash"]);
		auto vSrcPol = getCatalogData(m_sPolicyCatalogPath + ".source");
		vSrcPol.clear();
		vSrcPol.merge(vResult["newPolicy"], variant::MergeMode::All);
		sendMessage(Message::RequestPolicyUpdate, vResult["newPolicy"]);
		return vResult;
	}

	return vResult;
}

//
//
//
Variant CloudService::getInfo()
{
	return Dictionary({
			{"count", m_nHeartbeatCount},
			{"lastTime", m_lastHertbeatTime},
			{"avgDuration", m_nAvgDuration}
		});
}

///
/// @copydoc ICommandProcessor::execute() 
///
/// #### Processors
///   * '???'
///
/// #### Supported commands
///
Variant CloudService::execute(Variant vCommand, Variant vParams)
{
	TRACE_BEGIN;

	LOGLVL(Debug, "Process command <" << vCommand << ">");
	if (!vParams.isEmpty())
		LOGLVL(Trace, "Command parameters:\n" << vParams);

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### call()
	/// Calls the heartbeat the cloud service.
	///
	if (vCommand == "doHeartbeat")
	{
		m_sCurrStatus = vParams.get("status", "Online");
		return doHeartbeat();
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### getConfig()
	/// Gets config using the cloud service.
	///
	if (vCommand == "getConfig")
	{
		return getConfig();
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### getPolicy()
	/// STUB
	///
	if (vCommand == "getPolicy")
	{
		return true;
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### reportUninstall()
	/// Stub
	///
	if (vCommand == "reportUninstall")
	{
		return true;
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### start()
	/// Starts the service.
	/// 
	else if (vCommand == "start")
	{
		start();
		return true;
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### stop()
	/// Stops the service.
	/// 
	else if (vCommand == "stop")
	{
		stop();
		return true;
	}

	error::OperationNotSupported(SL, 
		FMT("CloudService doesn't support command <" << vCommand << ">")).throwException();
	TRACE_END(FMT("Error during processing of the command <" << vCommand << ">"));
}

} // namespace legacy
} // namespace cloud
} // namespace cmd 
/// @}
