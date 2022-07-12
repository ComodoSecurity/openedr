//
// edrav2.edrsvc project
//
// Autor: Denis Bogdanov (09.10.2019)
// Reviewer: 
//
///
/// @file Enroll mode handler for edrsvc
///
#include "pch.h"
#include "service.h"

namespace cmd {
namespace win {

//
//
//
class AppMode_enroll : public IApplicationMode
{
	Size m_nStep{ c_nMaxSize };
	std::string m_sRootUrl;
	std::string m_sToken;
	std::string m_sMachineId;
	std::string m_sClientId;
	std::string m_sCustomerId;
	std::string m_sEndpointId;
	std::string m_sLegacyCustomerId;
	bool m_fStartService = false;
	bool m_fManualStart = false;
	bool m_fClear = false;

public:

	//
	//
	//
	void enroll(Application* pApp, ObjPtr<ICommandProcessor>& pProcessor)
	{
		// Try to stop service
		std::cout << "Stopping <" << getCatalogData("app.fullName")
			<< ">: " << std::flush;

		auto vRes = execCommand(pProcessor, "execute", Dictionary({
			{ "clsid", CLSID_Command },
			{ "command", "stop" },
			{ "processor", "objects.application" }
		}));

		bool fRestart = m_fStartService;

		if (vRes.getType() == variant::ValueType::Boolean && vRes == false)
			std::cout << "SKIPPED" << std::endl;
		else
		{
			std::cout << "OK" << std::endl;
			fRestart = true;
		}

		// Set startup parameter
		if (m_fManualStart)
		{
			putCatalogData("app.config.serviceStartMode", "manual");
			(void)execCommand(pProcessor, "putCatalogData", Dictionary({
				{"path", "app.config.serviceStartMode"},
				{"data", "manual"}
			}));
		}

		if (!m_sRootUrl.empty())
		{
			(void)execCommand(pProcessor, "putCatalogData", Dictionary({
				{"path", "app.config.extern.endpoint.cloud.rootUrl"},
				{"data", m_sRootUrl}
			}));			
		}

		// Clear current config credentials for forcing reenrollment
		(void)execCommand(pProcessor, "clearCatalogData", 
			Dictionary({{"path", "app.config.extern.endpoint.license.customerId"}}));
		(void)execCommand(pProcessor, "clearCatalogData", 
			Dictionary({{"path", "app.config.extern.endpoint.license.endpointId"}}));
		(void)execCommand(pProcessor, "putCatalogData", Dictionary({
			{"path", "app.config.extern.endpoint.license.needUpdate"},
			{"data", true}
		}));
		


		// Clear current service backup credentials
		if (m_fClear)
		{
			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "endpointId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "customerId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "machineId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "clearCatalogData", 
				Dictionary({{"path", "app.config.extern.endpoint.license.machineId"}}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "legacyCustomerId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "clearCatalogData", 
				Dictionary({{"path", "app.config.extern.endpoint.license.legacyCustomerId"}}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "token"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "clearCatalogData", 
				Dictionary({{"path", "app.config.extern.endpoint.license.token"}}));

			(void)execCommand(pProcessor, "clearCatalogData",
				Dictionary({ {"path", "app.config.extern.endpoint.license.clientId"} }));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "clientId"},
				{"data", ""}
			}));

		}

		if (!m_sMachineId.empty())
		{
			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "machineId"},
				{"data", m_sMachineId}
			}));

			(void)execCommand(pProcessor, "putCatalogData", Dictionary({
				{"path", "app.config.extern.endpoint.license.machineId"},
				{"data", m_sMachineId}
			}));
		}

		//
		// Use customer/endpoint pair
		//
		if (!m_sEndpointId.empty())
		{
			if (!m_sCustomerId.empty())
			{
				(void)execCommand(pProcessor, "setServiceData", Dictionary({
					{"name", "customerId"},
					{"data", m_sCustomerId}
				}));

				(void)execCommand(pProcessor, "putCatalogData", Dictionary({
					{"path", "app.config.extern.endpoint.license.customerId"},
					{"data", m_sCustomerId}
				}));

				(void)execCommand(pProcessor, "setServiceData", Dictionary({
					{"name", "clientId"},
					{"data", ""}
				}));

				(void)execCommand(pProcessor, "clearCatalogData",
					Dictionary({ {"path", "app.config.extern.endpoint.license.clientId"} }));
			}
			else if (!m_sClientId.empty())
			{
				(void)execCommand(pProcessor, "setServiceData", Dictionary({
					{"name", "clientId"},
					{"data", m_sClientId}
				}));

				(void)execCommand(pProcessor, "putCatalogData", Dictionary({
					{"path", "app.config.extern.endpoint.license.clientId"},
					{"data", m_sClientId}
				}));

				(void)execCommand(pProcessor, "setServiceData", Dictionary({
					{"name", "customerId"},
					{"data", ""}
				}));
			}

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "endpointId"},
				{"data", m_sEndpointId}
			}));

			(void)execCommand(pProcessor, "putCatalogData", Dictionary({
				{"path", "app.config.extern.endpoint.license.endpointId"},
				{"data", m_sEndpointId}
			}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "token"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "clearCatalogData", 
				Dictionary({{"path", "app.config.extern.endpoint.license.token"}}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "legacyCustomerId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "clearCatalogData", 
				Dictionary({{"path", "app.config.extern.endpoint.license.legacyCustomerId"}}));
		}
		//
		// Use token
		//
		else if (!m_sToken.empty())
		{
			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "token"},
				{"data", m_sToken}
			}));

			(void)execCommand(pProcessor, "putCatalogData", Dictionary({
				{"path", "app.config.extern.endpoint.license.token"},
				{"data", m_sToken}
			}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "customerId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "endpointId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "legacyCustomerId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "clearCatalogData", 
				Dictionary({{"path", "app.config.extern.endpoint.license.legacyCustomerId"}}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "clientId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "clearCatalogData",
				Dictionary({ {"path", "app.config.extern.endpoint.license.clientId"} }));

		}
		else if (!m_sLegacyCustomerId.empty())
		{
			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "legacyCustomerId"},
				{"data", m_sLegacyCustomerId}
			}));

			(void)execCommand(pProcessor, "putCatalogData", Dictionary({
				{"path", "app.config.extern.endpoint.license.legacyCustomerId"},
				{"data", m_sLegacyCustomerId}
			}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "customerId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "endpointId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "token"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "clearCatalogData", 
				Dictionary({{"path", "app.config.extern.endpoint.license.token"}}));

			(void)execCommand(pProcessor, "setServiceData", Dictionary({
				{"name", "clientId"},
				{"data", ""}
			}));

			(void)execCommand(pProcessor, "clearCatalogData",
				Dictionary({ {"path", "app.config.extern.endpoint.license.clientId"} }));

		}

		//
		auto vScript = getCatalogData("app.config.enrollScript");
		std::cout << "Running enrollment handlers ("
			<< vScript.getSize() << " steps)..." << std::endl;
		if (m_nStep != c_nMaxSize)
			vScript = Sequence{ vScript[m_nStep] };
		for (auto vCmdInfo : vScript)
		{
			bool fOut = vCmdInfo.has("description");
			if (fOut) std::cout << "--> " <<
				vCmdInfo.get("description", "<unnamed>") << ": " << std::flush;
			auto vRes = execCommand(pProcessor, "execute", vCmdInfo["command"]);			
			if (fOut) 
				if (vRes.getType() == variant::ValueType::Boolean && vRes == false)
					std::cout << "SKIPPED" << std::endl;
				else
					std::cout << "OK" << std::endl;
		}

		// Start service if need
		if (fRestart)
		{
			std::cout << "Restarting <" << getCatalogData("app.fullName")
				<< ">: " << std::flush;

			// Release policy files
			(void)execCommand(pProcessor, "unsubscribeFromMessage", Dictionary({
				{"subscriptionId", "evmPolicyCompilationHandler"}
			}));
			(void)execCommand(pProcessor, "unsubscribeFromMessage", Dictionary({
				{"subscriptionId", "ptmPolicyCompilationHandler"}
			}));
			(void)execCommand(pProcessor, "putCatalogData", Dictionary({
				{"path", "app.config.policy"},
				{"data", Variant()}
			}));
			(void)execCommand(pProcessor, "putCatalogData", Dictionary({
				{"path", "app.config.messageHandlers.RequestPolicyUpdate"},
				{"data", Variant()}
			}));

			// Start service
			auto vRes = execCommand(pProcessor, "execute", Dictionary({
				{ "command", "start" },
				{ "processor", "objects.application" }
			}));

			if (vRes.getType() == variant::ValueType::Boolean && vRes == false)
				std::cout << "SKIPPED" << std::endl;
			else
				std::cout << "OK" << std::endl;
		}
	}

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		ObjPtr<ICommandProcessor> pProcessor = startElevatedInstance(true);
		try
		{
			// Check parameters

			if (((!m_sCustomerId.empty() || !m_sClientId.empty()) && m_sEndpointId.empty()) ||
				(m_sCustomerId.empty() && m_sClientId.empty() && !m_sEndpointId.empty()))
				error::InvalidArgument("The one of parameters clientId, customerId or endpointId is absent").throwException();

			if ((!m_sCustomerId.empty() || !m_sClientId.empty()) && !m_sToken.empty())
				error::InvalidArgument("You can't use clientId/endpointId and token at one time").throwException();

			if (!m_sMachineId.empty())
			{
				std::string sToken = execCommand(pProcessor, "getCatalogData", Dictionary({
					{"path", "app.config.extern.endpoint.license.token"},
					{"default", ""}
				}));

				if (sToken.empty() && m_sLegacyCustomerId.empty() && m_sToken.empty())
					error::InvalidArgument("For specifying machineId the token or legacyClientId also should be defined").throwException();
			}

			// Process enroll
			enroll(pApp, pProcessor);
			if (m_nStep == c_nMaxSize)
				std::cout << "Enrollment of this endpoint is completed successfully." << std::endl;
		}
		catch (error::Exception& e)
		{
			std::cout << "ERROR - " << e.what() << std::endl;
			stopElevatedInstance(pProcessor, true);
			return ErrorCode::RuntimeError;
		}
		catch (...)
		{
			std::cout << "GENERIC ERROR" << std::endl;
			stopElevatedInstance(pProcessor, true);
			return ErrorCode::RuntimeError;
		}
		stopElevatedInstance(pProcessor, true);
		return ErrorCode::OK;
	}

	//
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{
		clp += clara::Opt(m_nStep, "0..N")["--step"]
			("set the step in an install script");
		clp += clara::Opt(m_sToken, "token")["--token"]
			("set the token of a client");		
		clp += clara::Opt(m_sRootUrl, "rootUrl")["--rootUrl"]
			("set alternate cloud URL");		
		clp += clara::Opt(m_sMachineId, "machineId")["--machineId"]
			("set the machine id of an endpoint");
		clp += clara::Opt(m_sLegacyCustomerId, "legacyClientId")["--legacyClientId"]
			("legacy client id from an instance of agent v.1");
		clp += clara::Opt(m_sClientId, "clientId")["--clientId"]
			("predefined C1 client id");
		clp += clara::Opt(m_sCustomerId, "customerId")["--customerId"]
			("predefined native EDR client id");
		clp += clara::Opt(m_sEndpointId, "endpointId")["--endpointId"]
			("predefined endpoint id");
		clp += clara::Opt(m_fManualStart)["-m"]["--manual"]
			("set manual start mode");	
		clp += clara::Opt(m_fClear)["-c"]["--clear"]
			("clear all previous EDR settings");
		clp += clara::Opt(m_fStartService)["-s"]["--start"]
			("start service after successful enrollment");		
	}

	//
	//
	//
	virtual Variant execute(Application* pApp, Variant vCmd, Variant vParams)
	{
		error::OperationNotSupported(SL, "The object doesn't support any commands").throwException();
		return {};
	}

};

} // namespace win

 //
//
//
std::shared_ptr<IApplicationMode> createAppMode_enroll()
{
	return std::make_shared<win::AppMode_enroll>();
}

} // namespace cmd