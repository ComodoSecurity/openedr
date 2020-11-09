///
/// Catch2 entrypoint
///
#define CATCH_CONFIG_RUNNER

// Catch2
#include <catch2/catch.hpp>
#include "catch_discover.hpp"

extern bool initTestFramework();
extern void finalizeTestFramework();

//
//
//
int main(int argc, char* argv[])
{
    Catch::Session session;

    bool doDiscover = false;
    Catch::addDiscoverOption(session, doDiscover);
    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) return returnCode;

	if (!initTestFramework()) return -3;

	auto nResult = Catch::runDiscoverSession(session, doDiscover);

	finalizeTestFramework();

    return nResult;
}
