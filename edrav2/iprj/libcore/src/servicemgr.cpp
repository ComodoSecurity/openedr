//
// edrav2.libcore project
//
// Author: Yury Podpruzhnikov (15.01.2019)
// Reviewer: Denis Bogdanov (21.01.2019)
//
/// @file Global object and service manager implementation
///
#include "pch.h"
#include <service.hpp>
#include "servicemgr.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "svcmgr"

namespace openEdr {

//
//
//
void ServiceManager::finalConstruct(Variant vCfg)
{
}

//
//
//
Variant ServiceManager::getServiceConfig(std::string_view sServiceName) const
{
	Variant vServicesConfigs = getCatalogData(c_sServiceConfigPath, Dictionary());
	return vServicesConfigs.get(sServiceName, Variant());
}

//
//
//
std::string ServiceManager::getServiceStatePath(std::string_view sServiceName, 
	Variant vServiceConfig) const
{
	if (vServiceConfig.isNull())
		vServiceConfig = getServiceConfig(sServiceName);
	
	// Try get from config
	Variant vStatePath = vServiceConfig.get("statePath", nullptr);
	if (vStatePath.isNull())
		return "";

	return vStatePath;
}

//
//
//
ObjPtr<IObject> ServiceManager::createService(std::string_view sServiceName)
{
	CMD_TRACE_BEGIN;
	std::scoped_lock lock(m_mtxCreateService);

	// Check. possibly service was created
	{
		Variant vObj = m_vServices.get(sServiceName, nullptr);
		if (!vObj.isNull())
			return vObj;
	}

	Variant vServiceConfig = getServiceConfig(sServiceName);
	if (vServiceConfig.isNull())
		error::InvalidArgument(SL, 
			FMT("Can't find a descriptor for a service <" << sServiceName << ">")).throwException();
	ObjPtr<IObject> pObj = createObject(vServiceConfig);
	
	auto pService = queryInterfaceSafe<IService>(pObj);
	if (pService)
	{
		auto sStatePath = getServiceStatePath(sServiceName, vServiceConfig);
		if (!sStatePath.empty())
		{
			Variant vPreviousState = getCatalogData(sStatePath, nullptr);
			pService->loadState(vPreviousState);
		}
	}

	m_vServices.put(sServiceName, pObj);
	return pObj;

	CMD_TRACE_END(FMT("Can't create the service <" << sServiceName << ">"));
}

//
//
//
bool ServiceManager::hasServiceDeclaration(std::string_view sServiceName) const
{
	return m_vServices.has(sServiceName) || !getServiceConfig(sServiceName).isNull();
}

//
//
//
Size ServiceManager::getSize() const
{
	if (m_fDisableQueryService) return 0;
	return m_vServices.getSize() + m_vNamedObjects.getSize();
}

//
//
//
bool ServiceManager::isEmpty() const
{
	if (m_fDisableQueryService) return true;
	return m_vServices.isEmpty() && m_vNamedObjects.isEmpty();
}

//
//
//
bool ServiceManager::has(variant::DictionaryKeyRefType sName) const
{
	if (m_fDisableQueryService) return false;
	return m_vNamedObjects.has(sName) || hasServiceDeclaration(sName);
}

//
//
//
Size ServiceManager::erase(variant::DictionaryKeyRefType sName)
{
	if (m_fDisableQueryService) return 0;

	// try remove named object
	if (m_vNamedObjects.has(sName))
		return m_vNamedObjects.erase(sName);
	// check service
	if (hasServiceDeclaration(sName))
		error::InvalidArgument(SL, FMT("Can't delete native service <" << sName << ">")).throwException();
	return 0;
}

//
//
//
void ServiceManager::clear()
{
	error::InvalidUsage(SL, "Service manager can't support clear()").throwException();
}

//
//
//
Variant ServiceManager::get(variant::DictionaryKeyRefType sName) const
{
	CMD_TRACE_BEGIN;
	if (m_fDisableQueryService)
		error::RuntimeError(SL, "Services querying is disabled").throwException();

	{
		Variant vObj = m_vNamedObjects.get(sName, nullptr);
		if (!vObj.isNull())
			return vObj;
	}

	{
		Variant vObj = m_vServices.get(sName, nullptr);
		if (!vObj.isNull())
			return vObj;
	}

	return const_cast<ServiceManager*>(this)->createService(sName);
	CMD_TRACE_END(FMT("Can't get service with name: " << sName));
}

//
// FIXME: May we rename DictionaryKeyRefType ?
//
std::optional<Variant> ServiceManager::getSafe(variant::DictionaryKeyRefType sName) const
{
	if (m_fDisableQueryService) return {};

	try 
	{
		return get(sName);
	}
	catch (...)
	{
		return {};
	}
}

//
//
//
void ServiceManager::put(variant::DictionaryKeyRefType sName, const Variant& Value)
{
	if (m_fDisableQueryService)
		error::RuntimeError(SL, "Services adding is disabled").throwException();

	if (hasServiceDeclaration(sName))
		error::InvalidArgument(SL, FMT("Can't add named object with name of exist service: " << sName)).throwException();

	if (Value.isNull())
	{
		m_vNamedObjects.erase(sName);
		return;
	}

	m_vNamedObjects.put(sName, Value);
}

//
//
//
std::unique_ptr<variant::IIterator> ServiceManager::createIterator(bool /*fUnsafe*/) const
{
	// @podpruzhnikov FIXME: Add itertion support !!!
	error::InvalidUsage(SL, "Iteration is not supported").throwException();
}


//
//
//
Variant ServiceManager::execute(Variant vCommand, Variant vParams)
{
	//
	// Stopping all services
	//
	if (vCommand == "stop")
	{
		// Stopping services
		for (auto item : m_vServices)
		{
			CMD_TRY
			{
				auto pService = queryInterfaceSafe<IService>(Variant(item.second));
				if (!pService) continue;
				pService->stop();
			}
			CMD_PREPARE_CATCH
			catch (error::Exception& e)
			{
				e.log(SL, FMT("Error on stopping service <" << item.first << ">"));
			}
		}
		return {};
	}

	//
	// Shutting down all services and removing all objects and services
	//
	if (vCommand == "shutdown")
	{
		// Save services' states
		for (auto item : m_vServices)
		{
			auto pService = queryInterfaceSafe<IService>(Variant(item.second));
			if (!pService) continue;

			CMD_TRY
			{
				std::string sStatePath = getServiceStatePath(item.first);
				if (!sStatePath.empty())
					putCatalogData(sStatePath, pService->saveState());
			}
			CMD_PREPARE_CATCH
			catch (error::Exception& e)
			{
				e.log(SL, FMT("Error on state saving for service <" << item.first << ">"));
			}
		}

		m_fDisableQueryService = true;

		// Clearing named object list
		m_vNamedObjects.clear();

		// Shutdown services
		for (auto item : m_vServices)
		{
			CMD_TRY
			{
				auto pService = queryInterfaceSafe<IService>(Variant(item.second));
				if (!pService) continue;
				pService->shutdown();
			}
			CMD_PREPARE_CATCH
			catch (error::Exception& e)
			{
				e.log(SL, FMT("Error on shutdown of service <" << item.first << ">"));
			}
		}

		// Clearing services list
		m_vServices.clear();

		return {};
	}

	error::InvalidArgument(SL, FMT("Service manager doesn't support command <"
		<< vCommand << ">")).throwException();
}

} // namespace openEdr 
