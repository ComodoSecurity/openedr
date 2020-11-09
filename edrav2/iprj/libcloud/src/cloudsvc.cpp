//
// edrav2.libcloud project
// 
// Author: Denis Bogdanov (16.03.2019)
// Reviewer:
//
///
/// @file Cloud service class implementation
///
/// @addtogroup cloud Cloud communication library
/// @{
#include "pch.h"
#include "cloudsvc.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "cloudsvc"

namespace openEdr {
namespace cloud {

//
//
//
void CloudService::finalConstruct(Variant vConfig)
{
	if (!vConfig.isDictionaryLike()) error::InvalidArgument(SL, 
		"finalConstruct() supports only dictionary as a parameter").throwException();
	
	TRACE_BEGIN;
	m_httpClient.setRootUrl(getCatalogData("app.config.cloud.rootUrl", ""));
	m_sPolicyCatalogPath = "app.config.policy";
	m_sCurrVersion = CMD_STD_VERSION;

	updateSettings(vConfig);
	TRACE_END("Error during configuration");
}

//
//
//
void CloudService::updateSettings(Variant vConfig)
{
	std::scoped_lock lock(m_mtxData);
	
	if (vConfig.has("rootUrl"))
		m_httpClient.setRootUrl(vConfig["rootUrl"]);

	if (vConfig.has("networkUpdateMask"))
		m_nNetworkUpdateMask = vConfig["networkUpdateMask"];

	if (vConfig.has("policyCatalogPath"))
		m_sPolicyCatalogPath = vConfig["policyCatalogPath"];

	if (vConfig.has("currVersion"))
		m_sCurrVersion = vConfig["currVersion"];

	if (vConfig.has("period"))
		m_nPeriod = vConfig["period"];

	if (vConfig.has("httpParams"))
		m_httpClient.setParams(vConfig["httpParams"]);
}

//
//
//
void CloudService::start()
{
	LOGLVL(Detailed, "CloudService is being started");
	m_fFinished = false;
	// Restart the client after abort
	m_httpClient.restart();

	// Try to sync load config in case of damaged cloud section
	auto vCloud = getCatalogDataSafe("app.config.extern.endpoint.cloud");
	if (!vCloud.has_value())
	{
		CMD_TRY
		{ 
			(void)getConfig();
		} 
		CMD_PREPARE_CATCH
		catch (error::Exception& e)
		{
			e.logAsWarning("Error during initial request to the cloud");
		}
	}

	// Start async loop
	m_pTimerCtx = m_threadPool.runInLoop(m_nPeriod,
		[](ObjPtr<CloudService> pThis) -> bool 
		{
			CMD_TRY
			{
				(void)pThis->callHeartbeat();
			}
			CMD_PREPARE_CATCH				
			catch (error::OperationCancelled& e)
			{
				e.logAsWarning("Cloud request was cancelled");
			}
			catch (error::ConnectionError& e)
			{
				e.logAsWarning("Can't connect to the EDR cloud");
			}
			catch (error::Exception& e)
			{
				e.log(SL);
			}
			return true;
		}, getPtrFromThis(this));

	subscribeToMessage("AppFinishing", getPtrFromThis(this), "CloudService");
	LOGLVL(Detailed, "CloudService is started");
}

//
//
//
void CloudService::stop()
{
	LOGLVL(Detailed, "CloudService is being stopped");
	// Abort a current operation
	m_httpClient.abort();
	{
		std::scoped_lock lock(m_mtxData);
		m_fFirstStart = true;
	}
	m_pTimerCtx.reset();
	LOGLVL(Detailed, "CloudService is stopped");
}

//
//
//
void CloudService::shutdown()
{
	m_threadPool.stop(true);
}

//
// Messages handler
//
Variant CloudService::execute(Variant vParam)
{
	if (vParam["messageId"] != "AppFinishing") return {};
	reportOffline();
	m_fFinished = true;
	return {};
}

//
//
//
bool CloudService::checkCredentials()
{
	TRACE_BEGIN;

	std::string sEndpointId = getCatalogData("app.config.license.endpointId","");	
	std::string sCustomerId = getCatalogData("app.config.license.customerId", "");
	bool fNeedUpdate = getCatalogData("app.config.license.needUpdate", true);

	// Check if credentials are available
	if (sEndpointId != "" && sCustomerId != "" && !fNeedUpdate)
		return false;

	auto vParams = Dictionary({
		{"triesCount", 3},
		{"tryTimeout", 5000}
	});
	
	std::string sClientId = getCatalogData("app.config.license.clientId", "");

	// Request identity if the endpoint isn't registered yet
	if (sClientId.empty() && (sEndpointId.empty() || sCustomerId.empty()))
	{
		LOGLVL(Detailed, "Trying to get credentials using provided toked");
		std::string sToken = getCatalogData("app.config.license.token", "");
		if (sToken.empty())
		{
			LOGLVL(Detailed, "Trying to obtain a new token for EDRv1 instance");
			auto vCurrParams = vParams.clone();
			vCurrParams.put("legacyCustomerId", getCatalogData("app.config.license.legacyCustomerId", ""));
			vCurrParams.put("apiKey", getCatalogData("app.config.cloud.apiKey", ""));
			sToken = getToken(vCurrParams)["token"];
		}
		LOGLVL(Detailed, "Trying to get credentials using provided toked");
		auto vCurrParams = vParams.clone();
		vCurrParams.put("token", sToken);
		vCurrParams.put("machineId", getCatalogData("app.config.license.machineId", ""));
		auto vResult = getIdentity(vCurrParams);
		sEndpointId = vResult["endpointId"];
		sCustomerId = vResult["customerId"];
	}

	auto vCurrParams = vParams.clone();
	vCurrParams.put("endpointId", sEndpointId);
	vCurrParams.put("customerId", sCustomerId);
	vCurrParams.put("clientId", sClientId);
	auto vResult = enroll(vCurrParams);
	sEndpointId = vResult["endpointId"];
	sCustomerId = vResult["customerId"];

	LOGLVL(Detailed, "New credentials are ready (companyId <" << sCustomerId << ">, endpointId <" << sEndpointId << ">)");
	putCatalogData("app.config.extern.endpoint.license.customerId", sCustomerId);
	putCatalogData("app.config.extern.endpoint.license.endpointId", sEndpointId);
	putCatalogData("app.config.extern.endpoint.license.needUpdate", false);

	return true;
	TRACE_END("Error during checking cloud credentials");
}

//
//
//
Variant CloudService::callHeartbeat()
{
	std::scoped_lock lock(m_mtxData);
	
	if (m_fFinished)
		return {};
	
	using namespace std::chrono;
	auto timeStart = steady_clock::now();
	m_lastHertbeatTime = getCurrentTime();
	m_nHeartbeatCount++;

	if (m_fFirstStart)
	{
		(void)reportEndpointInfo(true);
		m_fFirstStart = false;
		(void)getConfig();
		(void)getPolicy();
	}
	else if (m_nNetworkUpdateMask == 0 || (m_nHeartbeatCount & m_nNetworkUpdateMask))
		(void)reportEndpointInfo(false);
	
	auto vRes = doHeartbeat();

	auto duration = duration_cast<milliseconds>(steady_clock::now() - timeStart);
	m_nAvgDuration = ((m_nAvgDuration * (m_nHeartbeatCount - 1)) + duration.count()) / m_nHeartbeatCount;
	return vRes;
}

//
//
//
Variant CloudService::doHeartbeat()
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	LOGLVL(Detailed, "Send heartbeart request");

	checkCredentials();

	//REMOVED: heartbit request
	Variant vResult;
	LOGLVL(Detailed, "Receive heartbeat response (command <" << vResult.get("command", Variant()) << ">)");

	if (vResult.isNull())
		return vResult;

	auto vCommand = vResult.get("command", Variant());
	if (vCommand.isNull())
		return vResult;

	std::string sCommandId = vResult.get("commandID", "");

	if (vCommand == "uninstall")
	{
		LOGLVL(Detailed, "Process <uninstall> command for commandId <" << sCommandId << ">");
		putCatalogData("app.config.extern.endpoint.uninstall.commandId", sCommandId);
		sendMessage(Message::RequestAppUninstall, {});
	}
	else if (vCommand == "updatePolicy")
	{
		LOGLVL(Detailed, "Process <updatePolicy> command");
		(void)getPolicy();
		confirmCommand(sCommandId);
	}
	else if (vCommand == "updateAgent")
	{		
		if (!vResult.has("params"))
			error::InvalidFormat("Response shall have <params> field").throwException();
		auto vParams = vResult["params"];
		if (!vParams.has("version"))
			error::InvalidFormat("Response shall have <version> field").throwException();

		std::string sVersion = vParams.get("version", m_sCurrVersion);
		if (compareVersion(sVersion, m_sCurrVersion) > 0)
		{
			LOGLVL(Detailed, "Process <updateAgent> request");
			Variant vUrl;
			Variant vChecksum;
			std::string sCpuType = getCatalogData("os.cpuType");
			if ((sCpuType == c_sAmd64) && vParams.has("x64"))
			{
				auto vX64 = vParams["x64"];
				if (!vX64.has("url") || !vX64.has("checksum"))
					error::InvalidFormat("Response shall have <x64.url> and <x64.checksum> fields").throwException();
				vUrl = vX64["url"];
				vChecksum = vX64["checksum"];
			}
			else if ((sCpuType == c_sX86) && vParams.has("x86"))
			{
				auto vX86 = vParams["x86"];
				if (!vX86.has("url") || !vX86.has("checksum"))
					error::InvalidFormat("Response shall have <x86.url> and <x86.checksum> fields").throwException();
				vUrl = vX86["url"];
				vChecksum = vX86["checksum"];
			}
			else if (vParams.has("URL"))
			{
				// Legacy protocol
				vUrl = vParams["URL"];
				if (!vParams.has("md5Checksum"))
					error::InvalidFormat("Response shall have <md5Checksum> field").throwException();
				vChecksum = vParams["md5Checksum"];
			}
			else
				error::InvalidFormat("Response shall have <URL> field").throwException();

			putCatalogData("app.update.version", vParams["version"]);
			putCatalogData("app.update.url", vUrl);
			putCatalogData("app.update.checksum", vChecksum);
			//auto vHead = m_httpClient.head(vUrl);
			//if (vHead.has("contentLength"))
			//	putCatalogData("app.update.size", vHead["contentLength"]);

			sendMessage(Message::RequestAppUpdate, getCatalogData("app.update"));
			return vResult;
		}
		else
		{
			LOGLVL(Detailed, "Skip <updateAgent> because the current version <" << m_sCurrVersion << 
				"> is the same or greater than <" << sVersion << ">");
			confirmCommand(sCommandId);
		}
	}
	else if (vCommand == "updateSettings")
	{
		LOGLVL(Detailed, "Process <updateSettings> command");
		(void)getConfig();
		confirmCommand(sCommandId);
	}
	else if (vCommand == "setLogLevel")
	{
		LOGLVL(Detailed, "Process <setLogLevel> command");
		confirmCommand(sCommandId);
		if (!vResult.has("params"))
			error::InvalidFormat("Response shall have <params> field").throwException();
		auto vParams = vResult["params"];
		if (vParams.has("root"))
			logging::setRootLogLevel(vParams["root"]);
		for (auto& vItem : Dictionary(vParams))
			if (vItem.first != "root")
				logging::setComponentLogLevel(std::string(vItem.first), vItem.second);
	}
	else
		error::OperationNotSupported(SL, FMT("Unknown cloud command <" <<
			vCommand << ">")).log();

	return vResult;
	TRACE_END("Error during request to heartbeat service");
}

//
//
//
void CloudService::confirmCommand(std::string_view sCommandId)
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	if (sCommandId.empty())
		return;

	checkCredentials();

	//REMOVED: command/ack request

	TRACE_END("Error during request to acknowledge service");

}

//
//
//
Variant CloudService::getConfig(Variant vParams)
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	LOGLVL(Detailed, "Send configuration request");
	checkCredentials();

	auto vCurrParams = vParams.isDictionaryLike() ? Dictionary(vParams.clone()) : Dictionary();
	if (!vCurrParams.has("triesCount"))
		vCurrParams.put("triesCount", 3);
	if (!vCurrParams.has("tryTimeout"))
		vCurrParams.put("tryTimeout", 5000);

	//REMOVED: config retrieval from cloud
	Variant vResult;

	if (vResult.has("gcpPubSubTopic"))
		putCatalogData("app.config.extern.endpoint.cloud.gcp.pubSubTopic", vResult["gcpPubSubTopic"]);

	if (vResult.has("gcpSACredentials"))
		putCatalogData("app.config.extern.endpoint.cloud.gcp.saCredentials", vResult["gcpSACredentials"]);

	if (vResult.has("valkyrieURL"))
		putCatalogData("app.config.extern.endpoint.cloud.valkyrie.url", vResult["valkyrieURL"]);

	if (vResult.has("valkyrieAPIKey"))
		putCatalogData("app.config.extern.endpoint.cloud.valkyrie.apiKey", vResult["valkyrieAPIKey"]);

	if (vResult.has("flsURL"))
		putCatalogData("app.config.extern.endpoint.cloud.fls.url", vResult["flsURL"]);

	if (vResult.has("heartbeatInterval"))
	{
		m_nPeriod = Size(variant::convert<variant::ValueType::Integer>(vResult["heartbeatInterval"])) * 1000;
		putCatalogData("app.config.extern.endpoint.cloud.heartbeatPeriod", m_nPeriod);
	}

	if (vResult.has("config"))
	{
		auto vConfig = getCatalogData("app.config.extern.cloud");
		vConfig.clear();
		vConfig.merge(vResult["config"], variant::MergeMode::All);
	}

	sendMessage(Message::CloudConfigurationIsChanged);

	return vResult;
	TRACE_END("Error during request to settings service");

}

//
//
//
Variant CloudService::getPolicy(Variant vParams)
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	LOGLVL(Detailed, "Send policy request");
	checkCredentials();

	auto vCurrParams = vParams.isDictionaryLike() ? Dictionary(vParams.clone()) : Dictionary();
	if (!vCurrParams.has("triesCount"))
		vCurrParams.put("triesCount", 3);
	if (!vCurrParams.has("tryTimeout"))
		vCurrParams.put("tryTimeout", 5000);

	//REMOVED: policy retrieval from cloud
	Variant vResult;
	// Update EVM source policy in the catalog
	auto vEvmPolicy = getCatalogData(m_sPolicyCatalogPath + ".groups.eventsMatching.source.evmCloud");
	if (vResult.has("policy"))
	{
		vEvmPolicy.clear();
		vEvmPolicy.merge(vResult["policy"], variant::MergeMode::All);
	}
	else if (vResult.has("evm"))
	{
		vEvmPolicy.clear();
		vEvmPolicy.merge(vResult["evm"], variant::MergeMode::All);
	}
	else
		error::InvalidFormat("Response shall have <evm> or <policy> field").throwException();

	// Update PTM source policy in the catalog
	if (vResult.has("ptm"))
	{
		auto vPtmPolicy = getCatalogData(m_sPolicyCatalogPath + ".groups.patternsMatching.source.ptmCloud");
		vPtmPolicy.clear();
		vPtmPolicy.merge(vResult["ptm"], variant::MergeMode::All);
	}

	sendMessage(Message::RequestPolicyUpdate);
	return vResult;
	TRACE_END("Error during request to policy service");
}

//
//
//
Variant CloudService::getToken(Variant vParams)
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	LOGLVL(Detailed, "Send EDRv1 client resolving request");

	std::string sLegacyCustomerId = vParams.get("legacyCustomerId", "");
	if (sLegacyCustomerId.empty())
		throw error::InvalidArgument(SL, "The <legacyCustomerId> is not specifed");

	Dictionary vCurrParams = vParams.clone();
	vCurrParams.put("httpHeader", Dictionary({
		{"edr-customerid", sLegacyCustomerId},
		{"x-api-key", vParams["apiKey"]}
	}));

	//REMOVED: token retrieval from cloud
	Variant vResult;

	if (!vResult.has("token"))
		error::InvalidFormat("Response shall have <token> field").throwException();

	return Dictionary({ 
		{"token",  variant::convert<variant::ValueType::String>(vResult["token"])} 
	});

	TRACE_END("Error during EDRv1 client resolving request");
}

//
//
//
Variant CloudService::enroll(Variant vParams)
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	LOGLVL(Detailed, "Send enrollment request");

	std::string sEndpointId = vParams.get("endpointId", getCatalogData("app.config.license.endpointId", ""));
	if (sEndpointId.empty())
		throw error::InvalidArgument(SL, "The <endpointId> is not specifed");

	
	std::string sCustomerId = vParams.get("customerId", "");
	std::string sClientId = vParams.get("clientId", "");
	
	//REMOVED: enroll procedure
	Variant vResult;
	// enrollment service had an error then returns customerID instead companyID
	if (!vResult.has("companyID"))
		vResult.put("companyID", variant::convert<variant::ValueType::String>(vResult["customerID"]));

	if (!vResult.has("companyID"))
		error::InvalidFormat("Response shall have <companyID> field").throwException();
	sCustomerId = variant::convert<variant::ValueType::String>(vResult["companyID"]);
	if (sCustomerId.empty())
		error::InvalidFormat("Field <companyID> is empty").throwException();

	if (!vResult.has("endpointID"))
		error::InvalidFormat("Response shall have <endpointID> field").throwException();
	sEndpointId = variant::convert<variant::ValueType::String>(vResult["endpointID"]);
	if (sEndpointId.empty())
		error::InvalidFormat("Field <endpointID> is empty").throwException();

	return Dictionary({ 
		{"customerId", sCustomerId},
		{"endpointId", sEndpointId}
	});
	TRACE_END("Error during enrollment request");
}

//
//
//
Variant CloudService::getIdentity(Variant vParams)
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	LOGLVL(Detailed, "Send identity request");

	std::string sToken = vParams.get("token", "");
	if (sToken.empty())
		throw error::InvalidArgument(SL, "The <token> parameter is not specifed");
	
	std::string sMachineId = vParams.get("machineId", "");
	if (sMachineId.empty())
		throw error::InvalidArgument(SL, "The <machineId> parameter is not specifed");
	
	//REMOVED: identity retrieval from cloud
	Variant vResult;

	if (!vResult.has("endpointID"))
		error::InvalidFormat("Response shall have <endpointID> field").throwException();

	if (!vResult.has("companyID"))
		error::InvalidFormat("Response shall have <companyID> field").throwException();

	//if (!getCatalogDataSafe("app.config.extern.endpoint.license.machineId").has_value())
	//	putCatalogData("app.config.extern.endpoint.license.machineId", sMachineId);
	//if (!getCatalogDataSafe("app.config.extern.endpoint.license.token").has_value())
	//	putCatalogData("app.config.extern.endpoint.license.token", sToken);

	return Dictionary({ 
		{"endpointId", variant::convert<variant::ValueType::String>(vResult["endpointID"])},
		{"customerId", variant::convert<variant::ValueType::String>(vResult["companyID"])}
	});
	TRACE_END("Error during request to identity service");
}

//
//
//
Variant CloudService::reportEndpointInfo(bool fFull)
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	LOGLVL(Detailed, "Prepare <info_update> report");

	checkCredentials();

	Dictionary vData;
	if (fFull)
	{
		vData.put("computerName", getCatalogData("os.hostName", "unknown"));
		vData.put("operatingSystem", getCatalogData("os.osName", "Unknown OS"));
		vData.put("lastReboot", getIsoTime(getCatalogData("os.bootTime", 0)));
		vData.put("agentVersion", m_sCurrVersion);
	}

	auto seqNetInfo = net::getNetworkAdaptorsInfo(true, true, true);
	Variant seqIp4 = Sequence();
	for (auto& vInfo : seqNetInfo)
		seqIp4.merge(vInfo["ip4"], variant::MergeMode::All);
	if (fFull || m_seqLastIp4 != seqIp4)
	{
		vData.put("localIPAddress", seqIp4);
		m_seqLastIp4 = seqIp4;
	}

	auto sActiveUser = sys::getLocalConsoleUser();
	if (fFull || m_sLastActiveUser != sActiveUser)
	{
		vData.put("loggedOnUser", sActiveUser);
		m_sLastActiveUser = sActiveUser;
	}

	if (vData.isEmpty())
		return vData;

	LOGLVL(Detailed, "Send <info_update> report");

	//REMOVED: status/update request

	return vData;
	TRACE_END("Error during request to info_update service");
}

//
//
//
void CloudService::reportOffline()
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	LOGLVL(Detailed, "Send <go_offline> report");

	CMD_TRY
	{
		checkCredentials();

		//REMOVED: status/update request
	}
	CMD_PREPARE_CATCH
	catch (error::OperationCancelled& e)
	{
		e.logAsWarning("Cloud request was cancelled");
	}
	catch (error::ConnectionError& e)
	{
		e.logAsWarning("Can't connect to the EDR cloud");
	}

	TRACE_END("Error during request to info_update (offline) service");
}

//
//
//
void CloudService::reportUninstall(bool fFailSafe)
{
	TRACE_BEGIN;
	std::scoped_lock lock(m_mtxData);

	LOGLVL(Detailed, "Send <uninstall> report");

	try
	{
		checkCredentials();

		std::string sUninstallCommandId = getCatalogData("app.config.extern.endpoint.uninstall.commandId", "");
		if (!sUninstallCommandId.empty())
		{
			LOGLVL(Detailed, "Send ack relating to uninstallation");
			confirmCommand(sUninstallCommandId);
			putCatalogData("app.config.extern.endpoint.uninstall.commandId", nullptr);
		}
		//REMOVED: status/update request
		
	}
	catch (...)
	{
		if (!fFailSafe)
			throw;
	}

	TRACE_END("Error during request to info_update (uninstall) service");
}

//
//
//
Variant CloudService::getInfo()
{
	std::scoped_lock lock(m_mtxData);
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
	/// Calls the heartbeat cloud service.
	///
	if (vCommand == "doHeartbeat")
	{
		return doHeartbeat();
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### reportEndpointInfo()
	/// Sends the endpoint info to the cloud.
	///
	else if (vCommand == "reportEndpointInfo")
	{
		return reportEndpointInfo(vParams.get("full", false));
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### updateSettings()
	/// Updates settings of current service.
	///
	else if (vCommand == "updateSettings")
	{
		updateSettings(vParams);
		return true;
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### confirmCommand()
	/// Confirms a command processing in the cloud.
	///
	else if (vCommand == "confirmCommand")
	{
		confirmCommand(std::string(vParams["commandId"]));
		return true;
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### getIdentity()
	/// Get endpoint identification information using cloud service.
	///
	else if (vCommand == "getIdentity")
	{
		return getIdentity(vParams);
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### enroll()
	/// Enrolls the endpoint to the cloud.
	///
	else if (vCommand == "enroll")
	{
		return enroll(vParams);
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### reportUninstall()
	/// Sends uninstall report to the cloud.
	///
	else if (vCommand == "reportUninstall")
	{
		reportUninstall(vParams.get("failSafe", false));
		return true;
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### resetDataCache()
	/// Resets the endpoint data cache.
	///
	else if (vCommand == "resetDataCache")
	{
		std::scoped_lock lock(m_mtxData);
		m_seqLastIp4 = Sequence();
		m_sLastActiveUser.clear();
		return true;
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### getConfig()
	/// Gets the config using cloud service.
	///
	else if (vCommand == "getConfig")
	{
		return getConfig(vParams);
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### getToken()
	/// Obtains a token using legacyCustomerId parameter.
	///
	else if (vCommand == "getToken")
	{
		return getToken(vParams);
	}

	///
	/// @fn Variant CloudService::execute()
	///
	/// ##### getPolicy()
	/// Gets a policy using the cloud service.
	///
	else if (vCommand == "getPolicy")
	{
		return getPolicy(vParams);
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

} // namespace cloud
} // namespace openEdr 
/// @}
