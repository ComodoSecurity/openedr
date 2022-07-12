//
// EDRAv2.cmdcon project
//
// Autor: Yury Ermakov (30.01.2019)
// Reviewer: Denis Bogdanov (06.02.2019)
//

///
/// @file 'rpcserver' mode handler for cmdcon
///

#include "pch.h"

namespace cmd {

//
//
//
class AppMode_rpcserver : public IApplicationMode
{
private:
	std::string m_sCmd;

public:
	static bool g_fCrtlBreakIsUsed;

	//
	//
	//
	static BOOL WINAPI CtrlBreakHandler(_In_ DWORD dwCtrlType)
	{
		auto pApp = queryInterfaceSafe<Application>(getCatalogData("objects.application"));
		g_fCrtlBreakIsUsed = true;
		if (pApp)
			pApp->shutdown();
		return TRUE;
	}

	//
	//
	//
	ErrorCode main(Application* pApp) override
	{
		using namespace std::chrono_literals;
		// FIXME: Why you don't use autostart objects in config?
		auto pServer = queryInterface<ICommandProcessor>(getCatalogData("app.config.server"));
		(void)pServer->execute("start", {});
		std::cout << "JSON-RPC server running" << std::endl;

		std::cout << "Press CTRL+C or CTRL+BREAK to stop server" << std::endl;
		if (!SetConsoleCtrlHandler(CtrlBreakHandler, TRUE))
			error::win::WinApiError(SL, "Can't set CTRL+BREAK handler").throwException();

		pApp->waitShutdown();

		(void)pServer->execute("stop", {});
		std::cout << "JSON-RPC server stopped" << std::endl;

		return (g_fCrtlBreakIsUsed) ? ErrorCode::UserBreak : ErrorCode::OK;
	}

	//
	//
	//
	void initParams(clara::Parser& clp) override
	{
	}

};

bool AppMode_rpcserver::g_fCrtlBreakIsUsed = false;

//
//
//
std::shared_ptr<IApplicationMode> createAppMode_rpcserver()
{
	return std::make_shared<AppMode_rpcserver>();
}

} // namespace cmd
