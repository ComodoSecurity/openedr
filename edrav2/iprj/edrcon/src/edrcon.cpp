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

namespace cmd {

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

} // namespace cmd

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

	auto pApp = createObject<cmd::Application>();
	pApp->addMode(cmd::Application::c_sDefModeName, cmd::createAppMode_default());
	pApp->addMode("dump", cmd::createAppMode_dump());
	pApp->addMode("unprot", cmd::createAppMode_unprot());
	pApp->addMode("debug", cmd::createAppMode_debug());
	pApp->addMode("wait", cmd::createAppMode_wait());
	pApp->addMode("run", cmd::createAppMode_run());
	pApp->addMode("file", cmd::createAppMode_file());
	pApp->addMode("process", cmd::createAppMode_process());
	pApp->addMode("rpcserver", cmd::createAppMode_rpcserver());
	pApp->addMode("compile", cmd::createAppMode_compile());
	// Add your application mode handlers below

	int ec = pApp->run("edrcon", "Comodo EDR console", argc, argv);
	SetConsoleOutputCP(nConsoleCP);
	return ec;

}
