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
class AppMode_process : public IApplicationMode
{
private:
	Size m_nTimeout = c_nMaxSize;
	uint32_t m_nPid = 0;
	std::string m_sProcName;
	std::string m_sSubMode;

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
		std::cout << "Process monitor is started [" << m_sSubMode << "]" << std::endl;

		if (m_sSubMode == "info")
		{
			auto vProcess = execCommand(createObject(CLSID_ProcessDataProvider), "getProcessInfo",
				Dictionary({ {"pid", m_nPid} }));
			std::cout << "Process info:" << std::endl;
			std::cout << variant::serializeToJson(vProcess, variant::JsonFormat::Pretty) << std::endl;
		}
		else if (m_sSubMode == "inject" || m_sSubMode == "uninject")
		{
			auto pObj = getCatalogData("objects.processMonitorController");
			auto pCommand = createCommand(pObj, m_sSubMode, Dictionary({ {"pid", m_nPid}, {"procName", m_sProcName} }));
			auto pResult = pCommand->execute();
		}
	
		return (fCrtlBreakIsUsed) ? ErrorCode::UserBreak : ErrorCode::OK;
	}

	//
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{	
		clp += clara::Arg(m_sSubMode, "info|inject|uninject") ("process monitor working mode");
		clp += clara::Opt(m_nPid, "pid")["--pid"]("set process id");
		clp += clara::Opt(m_sProcName, "procname")["--pname"]("set process name");
	}

};

bool AppMode_process::fCrtlBreakIsUsed = false;

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_process()
{
	return std::make_shared<AppMode_process>();
}

} // namespace cmd


// FIXME: remove garbage functional from MadCHook (@kroshin)
DWORD DynamicHookWithDllLoadDelay(LPCWSTR dllName, HMODULE hModule)
{
	return 0;
}
