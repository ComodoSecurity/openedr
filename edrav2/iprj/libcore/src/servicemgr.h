//
// EDRAv2.libcore project
//
// Author: Yury Podpruzhnikov (15.01.2019)
// Reviewer: Denis Bogdanov (21.01.2019)
//
/// @file Global object and service manager
/// @addtogroup basicObjects Basic objects and interfaces
/// @{
#pragma once
#include <command.hpp>
#include <objects.h>

namespace cmd {

/// 
/// Manager of global named object and services
/// 
class ServiceManager : public ObjectBase<CLSID_ServiceManager>, 
	public variant::IDictionaryProxy, 
	public ICommandProcessor
{
	Dictionary m_vServices;
	Dictionary m_vNamedObjects;

	std::atomic<bool> m_fDisableQueryService{ false };

	ObjPtr<IObject> createService(std::string_view sServiceName);
	std::recursive_mutex m_mtxCreateService;

	bool hasServiceDeclaration(std::string_view sServiceName) const;

	static inline constexpr char c_sServiceConfigPath[] = "app.config.objects";
	Variant getServiceConfig(std::string_view sServiceName) const;

	std::string getServiceStatePath(std::string_view sServiceName, Variant vServiceConfig=nullptr) const;

public:

	void finalConstruct(Variant vCfg);

	// IDictionaryProxy
	Size getSize() const override;
	bool isEmpty() const override;
	bool has(variant::DictionaryKeyRefType sName) const override;
	Size erase(variant::DictionaryKeyRefType sName) override;
	void clear() override;
	Variant get(variant::DictionaryKeyRefType sName) const override;
	std::optional<Variant> getSafe(variant::DictionaryKeyRefType sName) const override;
	void put(variant::DictionaryKeyRefType sName, const Variant& Value) override;
	std::unique_ptr<variant::IIterator> createIterator(bool fUnsafe) const override;

	// ICommandProcessor
	Variant execute(Variant vCommand, Variant vParams) override;
};

} // namespace cmd 
/// @}
