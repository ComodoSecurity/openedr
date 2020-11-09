//
// edrav2.edrsvc project
//
// Autor: Denis Bogdanov (06.02.2019)
// Reviewer: Yury Podpruzhnikov (11.02.2019)
//
///
/// @file Install mode handler for edrsvc
///
#include "pch.h"
#include "service.h"

namespace openEdr {
namespace win {

//
//
//
class AppMode_install : public IApplicationMode
{
	Size m_nStep{ c_nMaxSize };
	bool m_fManualStart = false;

public:

	//
	//
	//
	void install(Application* pApp, ObjPtr<ICommandProcessor>& pProcessor)
	{
		// Set startup parameter
		if (m_fManualStart)
		{
			putCatalogData("app.config.script.params.startMode", "manual");
			(void)execCommand(pProcessor, "putCatalogData", Dictionary({
				{"path", "app.config.script.params.startMode"},
				{"data", "manual"}
			}));
		}

		//
		auto vScript = getCatalogData("app.config.installScript");
		std::cout << "Running installation handlers ("
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
	}
	
	//
	//
	//
	void uninstall(Application* pApp, ObjPtr<ICommandProcessor>& pProcessor)
	{
		auto vScript = getCatalogData("app.config.uninstallScript");
		std::cout << "Running uninstallation handlers ("
			<< vScript.getSize() << " steps)..." << std::endl;
		if (m_nStep != c_nMaxSize)
			vScript = Sequence{ vScript[m_nStep] };
		for (auto vCmdInfo : vScript)
		{
			bool fOut = vCmdInfo.has("description");
			if (fOut) std::cout << "--> " 
				<< vCmdInfo.get("description", "<unnamed>") << ": " << std::flush;
			try
			{
				auto vRes = execCommand(pProcessor, "execute", vCmdInfo["command"]);
				if (fOut) 
					if (vRes.getType() == variant::ValueType::Boolean && vRes == false)
						std::cout << "SKIPPED" << std::endl;
					else
						std::cout << "OK" << std::endl;
			}
			catch (error::Exception& ex)
			{
				std::cout << "ERROR" << std::endl;
				std::cerr << "    " << ex.getErrorCode() << " - " << ex.what() << std::endl;
			}
			catch (...)
			{
				std::cout << "GENERIC ERROR" << std::endl;
			}
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
			// Process uninstall
			if (getCatalogData("app.mode", "install") == "uninstall")
			{
				uninstall(pApp, pProcessor);
				if (m_nStep == c_nMaxSize)
					std::cout << "Uninstallation of <" << getCatalogData("app.fullName")
					<< "> is completed." << std::endl;
			}

			// Process install
			else
			{
				try
				{
					install(pApp, pProcessor);
					if (m_nStep == c_nMaxSize)
						std::cout << "Installation of <" << getCatalogData("app.fullName")
						<< "> is completed successfully." << std::endl;
				}
				catch (error::Exception& ex)
				{
					std::cout << "ERROR" << std::endl;
					std::cerr << "    " << ex.getErrorCode() << " - " << ex.what() << std::endl;
					ex.log();

					std::cout << "Performing rollback..." << std::endl;
					if (ex.getErrorCode() == ErrorCode::ConnectionError)
					{
						std::cout << "Restarting elevated instance..." << std::endl;
						pProcessor = startElevatedInstance(false);
					}
					uninstall(pApp, pProcessor);
					return ex.getErrorCode();
				}
			}
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
		clp += clara::Opt(m_fManualStart)["-m"]["--manual"]
			("set service manual start mode");	
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
std::shared_ptr<IApplicationMode> createAppMode_install()
{
	return std::make_shared<win::AppMode_install>();
}

} // namespace openEdr