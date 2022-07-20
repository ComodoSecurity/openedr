//
// Test framework initialization functions
//
#include "pch.h"

//
//
//
bool initTestFramework()
{
	cmd::logging::initLogging();
	//cmd::logging::setRootLogLevel(cmd::LogLevel::Normal);
	return true;
}
