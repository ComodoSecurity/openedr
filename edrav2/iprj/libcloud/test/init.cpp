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


void finalizeTestFramework()
{ 
	cmd::shutdownGlobalPools();
	cmd::putCatalogData("objects", nullptr);
	cmd::clearCatalog();
	cmd::unsubscribeAll();
}
