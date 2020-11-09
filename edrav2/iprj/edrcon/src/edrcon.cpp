//
// EDRAv2.cmdcon project
//
// Autor: Denis Bogdanov (11.01.2019)
// Reviewer: Yury Podpruzhnikov (18.01.2019)
//
///
/// @file Entrypoint for cmdcon applcation
///
#include "pch.h"

namespace openEdr {

//
// Put appMode class factories here
//
extern std::shared_ptr<IApplicationMode> createAppMode_default();
extern std::shared_ptr<IApplicationMode> createAppMode_dump();
extern std::shared_ptr<IApplicationMode> createAppMode_unprot();
extern std::shared_ptr<IApplicationMode> createAppMode_debug();
extern std::shared_ptr<IApplicationMode> createAppMode_wait();
extern std::shared_ptr<IApplicationMode> createAppMode_empty();
extern std::shared_ptr<IApplicationMode> createAppMode_process();
extern std::shared_ptr<IApplicationMode> createAppMode_rpcserver();
extern std::shared_ptr<IApplicationMode> createAppMode_run();
extern std::shared_ptr<IApplicationMode> createAppMode_file();
extern std::shared_ptr<IApplicationMode> createAppMode_compile();

} // namespace openEdr

//
//
//
int wmain(int argc, wchar_t* argv[])
{
	CHECK_IN_SOURCE_LOCATION("MainThread", 0);

	// Set console code page to UTF-8 so console known how to interpret string data
	uint32_t nConsoleCP = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);

	// Enable buffering to prevent VS from chopping up UTF-8 byte sequences
	setvbuf(stdout, nullptr, _IOFBF, 1000);

	auto pApp = createObject<openEdr::Application>();
	pApp->addMode(openEdr::Application::c_sDefModeName, openEdr::createAppMode_default());
	pApp->addMode("dump", openEdr::createAppMode_dump());
	pApp->addMode("unprot", openEdr::createAppMode_unprot());
	pApp->addMode("debug", openEdr::createAppMode_debug());
	pApp->addMode("wait", openEdr::createAppMode_wait());
	pApp->addMode("run", openEdr::createAppMode_run());
	pApp->addMode("file", openEdr::createAppMode_file());
	pApp->addMode("process", openEdr::createAppMode_process());
	pApp->addMode("rpcserver", openEdr::createAppMode_rpcserver());
	pApp->addMode("compile", openEdr::createAppMode_compile());
	// Add your application mode handlers below

	int ec = pApp->run("edrcon", "OpenEDR console", argc, argv);
	SetConsoleOutputCP(nConsoleCP);
	return ec;

}
