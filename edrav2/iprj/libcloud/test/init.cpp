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


void finalizeTestFramework()
{ 
	openEdr::shutdownGlobalPools();
	openEdr::putCatalogData("objects", nullptr);
	openEdr::clearCatalog();
	openEdr::unsubscribeAll();
}
