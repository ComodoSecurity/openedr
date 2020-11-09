//
// EDRAv2.edrsvc project
//
// Autor: Denis Bogdanov (05.02.2019)
// Reviewer: Yury Podpruzhnikov (11.02.2019)
//
///
/// @file Default mode handler for edrsvc
///
#include "pch.h"
#include "service.h"

#undef CMD_COMPONENT
#define CMD_COMPONENT "service" 

namespace openEdr {


bool g_fCrtlBreakIsUsed = false;

//
//
//
BOOL WINAPI CtrlBreakHandler(_In_ DWORD dwCtrlType)
{
	auto pApp = queryInterfaceSafe<Application>(getCatalogData("objects.application"));
	g_fCrtlBreakIsUsed = true;
	std::cout << "Application received CRTL+BREAK signal - stopping..." << std::endl;
	if (pApp)
		pApp->shutdown();
	return TRUE;
}

//
//
//
class AppMode_default : public IApplicationMode
{
public:

	//
	//
	//
	virtual ErrorCode main(Application* pApp) override
	{
		LOGINF("Service runs");
		bool fAppMode = !getCatalogData("app.config.isService", false);
		if (fAppMode)
		{
			std::cout << "Service is started in an Application mode." << std::endl 
				<< "Use CRTL+BREAK for stopping" << std::endl;
			if (!SetConsoleCtrlHandler(CtrlBreakHandler, TRUE))
				error::win::WinApiError(SL, "Can't set CTRL-BREAK handler").throwException();
		}
		pApp->waitShutdown();
		LOGINF("Service starts shutdowning");
		return (g_fCrtlBreakIsUsed) ? ErrorCode::UserBreak : ErrorCode::OK;
	}

	//
	//
	//
	virtual void initParams(clara::Parser& clp) override
	{
		// clp += clara::Arg(m_sTestMode, "mode3|mode4") ("Working mode 2");
	}

};

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_default()
{
	return std::make_shared<AppMode_default>();
}

} // namespace openEdr