//
// Test framework initialization functions
//
#include "pch.h"

//
//
//
bool initTestFramework()
{
	openEdr::logging::initLogging();
	//openEdr::logging::setRootLogLevel(openEdr::LogLevel::Normal);
	return true;
}
