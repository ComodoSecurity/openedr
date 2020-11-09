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

namespace openEdr {

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

} // namespace openEdr

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
		auto pApp = createObject<openEdr::win::WinService>();
		pApp->addMode(openEdr::Application::c_sDefModeName, openEdr::createAppMode_default());
		pApp->addMode("start", openEdr::createAppMode_start());
		pApp->addMode("stop", openEdr::createAppMode_stop());
		pApp->addMode("install", openEdr::createAppMode_install());
		pApp->addMode("enroll", openEdr::createAppMode_enroll());
		pApp->addMode("uninstall", openEdr::createAppMode_install());
		pApp->addMode("server", openEdr::createAppMode_wait());
		//pApp->addMode("dump", openEdr::createAppMode_dump());
		//pApp->addMode("test", openEdr::createAppMode_wait());
		ec = pApp->run("edrsvc", "OpenEDR service", argc, argv);
	}
	// std::cout << "Application finished." << std::endl;
	SetConsoleOutputCP(nConsoleCP);
	return ec;
}
