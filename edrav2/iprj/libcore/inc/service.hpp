//
// edrav2.libcore project
//
// Declaration of the `IObject` interface
//
// Autor: Yury Podpruzhnikov (17.12.2018)
// Reviewer: Denis Bogdanov (18.01.2019)
//
#pragma once
#include "object.hpp"
#include "variant.hpp"

namespace cmd {

///
/// Service (singleton) interface
///
class IService : OBJECT_INTERFACE
{
public:
	///
	/// Loading of service state after creation
	/// @param vState The value which is returned by saveState before previous shutdown
	///        It can be null if a previous state can not be found.
	///
	virtual void loadState(Variant vState) = 0;

	///
	/// Saving of service state before shutdown
	/// @return Any serializable Variant which will be passed into loadState after next loading of application.
	///
	virtual Variant saveState() = 0;

	///
	/// Service starting 
	///
	virtual void start() = 0;

	///
	/// Service stopping 
	///
	virtual void stop() = 0;

	///
	/// Service shutting down (just before destroy)
	///
	virtual void shutdown() = 0;
};

///
/// Returning path to service in GlobalCatalog
/// @param sServiceName A name of a service.
///
inline std::string getServicePath(std::string_view sServiceName)
{
	return std::string("objects.") + std::string(sServiceName);
}

//
//
//
inline ObjPtr<IObject> queryService(std::string_view sServiceName)
{
	return getCatalogData(getServicePath(sServiceName));
}

//
//
//
inline ObjPtr<IObject> queryServiceSafe(std::string_view sServiceName)
{
	auto pSrv = getCatalogDataSafe(getServicePath(sServiceName));
	if (pSrv.has_value())
		return pSrv.value();
	return nullptr;
}

//
//
//
inline void startService(std::string_view sServiceName)
{
	auto pService = queryInterface<IService>(queryService(sServiceName));
	pService->start();
}

//
//
//
inline void stopService(std::string_view sServiceName)
{
	auto pService = queryInterface<IService>(queryService(sServiceName));
	pService->stop();
}

//
//
//
inline void addGlobalObject(std::string_view sName, Variant vValue)
{
	putCatalogData(getServicePath(sName), vValue);
}

//
//
//
inline void removeGlobalObject(std::string_view sName)
{
	putCatalogData(getServicePath(sName), nullptr);
}

} // namespace cmd 

