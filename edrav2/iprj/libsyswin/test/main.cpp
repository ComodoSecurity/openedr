/** Basic Info **

Copyright: 2018 Johnny Hendriks

Author : Johnny Hendriks
Year   : 2018
Project: VSTestAdapter for Catch2
Licence: MIT

Notes: None
 
** Basic Info **/

#define CATCH_CONFIG_RUNNER


// Catch2
#include <catch2/catch.hpp>
#include "catch_discover.hpp"

int main(int argc, char* argv[])
{
    Catch::Session session;

    bool doDiscover = false;

    Catch::addDiscoverOption(session, doDiscover);

    int returnCode = session.applyCommandLine(argc, argv);
    if (returnCode != 0) return returnCode;

    return Catch::runDiscoverSession(session, doDiscover);
}
