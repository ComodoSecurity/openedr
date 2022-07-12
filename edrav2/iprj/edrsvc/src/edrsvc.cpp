//
// EDRAv2.edrsvc project
//
// Autor: Denis Bogdanov (15.02.2019)
// Reviewer: Yury Podpruzhnikov (11.02.2019)
//
///
/// @file Entrypoint for edrsvc applcation
///
#include "pch.h"
#include "service.h"

namespace cmd {

//
// Put appMode class factories here
//
extern std::shared_ptr<IApplicationMode> createAppMode_default();
extern std::shared_ptr<IApplicationMode> createAppMode_install();
extern std::shared_ptr<IApplicationMode> createAppMode_empty();
extern std::shared_ptr<IApplicationMode> createAppMode_wait();
extern std::shared_ptr<IApplicationMode> createAppMode_dump();
extern std::shared_ptr<IApplicationMode> createAppMode_start();
extern std::shared_ptr<IApplicationMode> createAppMode_stop();
extern std::shared_ptr<IApplicationMode> createAppMode_enroll();
extern std::shared_ptr<IApplicationMode> createAppMode_restart();

} // namespace cmd

//
//
//
int wmain(int argc, wchar_t* argv[])
{
	// Set console code page to UTF-8 so console known how to interpret string data
	uint32_t nConsoleCP = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);
	// Enable buffering to prevent VS from chopping up UTF-8 byte sequences
	setvbuf(stdout, nullptr, _IOFBF, 1000);

	CHECK_IN_SOURCE_LOCATION("MainThread", 0);

	// TODO: Check if we need this mode
	// ::SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
	int ec = 0;
	{
		auto pApp = createObject<cmd::win::WinService>();
		pApp->addMode(cmd::Application::c_sDefModeName, cmd::createAppMode_default());
		pApp->addMode("start", cmd::createAppMode_start());
		pApp->addMode("stop", cmd::createAppMode_stop());
		pApp->addMode("restart", cmd::createAppMode_restart());
		pApp->addMode("install", cmd::createAppMode_install());
		pApp->addMode("enroll", cmd::createAppMode_enroll());
		pApp->addMode("uninstall", cmd::createAppMode_install());
		pApp->addMode("server", cmd::createAppMode_wait());
		//pApp->addMode("dump", cmd::createAppMode_dump());
		//pApp->addMode("test", cmd::createAppMode_wait());
		ec = pApp->run("edrsvc", "Comodo EDR service", argc, argv);
	}
	// std::cout << "Application finished." << std::endl;
	SetConsoleOutputCP(nConsoleCP);
	return ec;
}
