//
// EDRAv2.cmdcon project
//
// Autor: Denis Kroshin (11.01.2019)
// Reviewer: 
//
///
/// @file Install mode handler for cmdcon
///
#include "pch.h"

namespace cmd {

//
//
//
class AppMode_sysmon : public IApplicationMode
{
private:
	Size m_nTimeout = c_nMaxSize;
	std::string m_sSubMode;
	std::string m_sDrvName;
	bool m_fSystem = false;

public:
	static bool fCrtlBreakIsUsed;

	//
	//
	//
	static BOOL WINAPI CtrlBreakHandler(_In_ DWORD dwCtrlType)
	{
		auto pApp = queryInterfaceSafe<Application>(getCatalogData("objects.application"));
		if (pApp)
			pApp->shutdown();
		fCrtlBreakIsUsed = true;
		return TRUE;
	}

	void monitor(Application* pApp)
	{
		using namespace std::chrono_literals;
		using namespace std::chrono;
		if (m_nTimeout == c_nMaxSize)
			m_nTimeout = getCatalogData("app.config.timeout", c_nMaxSize);

		if (m_nTimeout == c_nMaxSize)
			std::cout << "Start application and wait infinitely" << std::endl;
		else
			std::cout << "Start application and wait " << m_nTimeout << "ms" << std::endl;

		std::cout << "Use CRTL_BREAK for stopping" << std::endl;
		if (!SetConsoleCtrlHandler(CtrlBreakHandler, TRUE))
			error::win::WinApiError(SL, "Can't set CTRL-BREAK handler").throwException();

		std::chrono::milliseconds msTimeout(m_nTimeout);
		auto startTime = steady_clock::now();
		while (!pApp->needShutdown())
		{
			milliseconds dur = duration_cast<milliseconds>(steady_clock::now() - startTime);
			if (m_nTimeout != c_nMaxSize && dur > msTimeout) break;
			std::this_thread::sleep_for(100ms);
		}
	}

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		std::cout << "System monitor is started [" << m_sSubMode << "]" << std::endl;

		if (m_sSubMode == "install" || m_sSubMode == "uninstall")
		{
			auto pObj = getCatalogData("objects.systemMonitorController");
			auto pCommand = createCommand(pObj, m_sSubMode, Dictionary({ {"driverName", m_sDrvName}, {"useSystemDir", m_fSystem} }));
			auto pResult = pCommand->execute();
			auto pService = queryInterface<IService>(pObj);
		}
		else if (m_sSubMode == "monitor")
		{
			auto pObj = queryInterface<IService>(getCatalogData("objects.systemMonitorController"));
			pObj->start();
			monitor(pApp);
		}
		else
		{
			std::cout << "Error: Unknown or undefined submode" << std::endl;
			return ErrorCode::InvalidArgument;
		}
		
		return (fCrtlBreakIsUsed)? ErrorCode::UserBreak : ErrorCode::OK;
	}

	//
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{	
		clp += clara::Arg(m_sSubMode, "install|uninstall|monitor") ("system monitor working mode");
		clp += clara::Opt(m_nTimeout, "msec")["-t"]["--timeout"]("set work time (msec)");
		clp += clara::Opt(m_sDrvName, "drivername")["--drvname"]("driver name (full path)");
		clp += clara::Opt(m_fSystem)["--system"]("copy driver to system directory");

	}

};

bool AppMode_sysmon::fCrtlBreakIsUsed = false;

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_sysmon()
{
	return std::make_shared<AppMode_sysmon>();
}

} // namespace cmd
