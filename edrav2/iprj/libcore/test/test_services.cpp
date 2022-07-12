//
//  EDRAv2. Unit test
//    Services test (ServiceManager)
//
#include "pch.h"

using namespace cmd;

CMD_DECLARE_LIBRARY_CLSID(CLSID_ExampleService, 0xE05E45C6);


//
//
//
class ExampleService : public ObjectBase<CLSID_ExampleService>, public IService
{
	Variant m_Info;

public:
	void finalConstruct(Variant vCfg)
	{
		m_Info = vCfg["info"];
		m_Info.put("finalConstruct", true);
		m_Info.put("id", getId());
	}

	virtual void loadState(Variant vState) override 
	{
		m_Info.put("loadState", vState);
	}
	
	virtual Variant saveState()  override 
	{
		m_Info.put("saveState", true);
		return Dictionary({ {"a", 1} });
	}

	virtual void start() override 
	{
		m_Info.put("start", true);
	}

	virtual void stop() override 
	{
		m_Info.put("stop", true);
	}

	virtual void shutdown() override 
	{
		m_Info.put("shutdown", true);
	}

	Variant getInfo()
	{
		return m_Info;
	}

};

CMD_DEFINE_LIBRARY_CLASS(ExampleService)

TEST_CASE("ServiceManager.queryService")
{
	putCatalogData("objects", nullptr);
	putCatalogData("app", Dictionary());

	SECTION("service_creation")
	{
		Variant vSvcInfo = Dictionary();

		// Init
		Variant vConfig = Dictionary({
			{"objects", Dictionary({
				{"svc1", Dictionary({{"clsid", CLSID_ExampleService}, {"info", vSvcInfo}}) },
			})},
		});
		putCatalogData("app.config", vConfig);
		putCatalogData("objects", createObject(CLSID_ServiceManager));

		REQUIRE(vSvcInfo == Dictionary());

		// creation
		ObjPtr<IObject> pObj = getCatalogData("objects.svc1");
		REQUIRE(pObj->getClassId() == CLSID_ExampleService);
		//Variant vCatalog = getCatalogData("");

		vSvcInfo = queryInterface<ExampleService>(pObj)->getInfo();
		REQUIRE(vSvcInfo == Dictionary({ {"finalConstruct", true}, {"id", pObj->getId()} }));

		// query exist
		REQUIRE(queryInterface<IObject>(getCatalogData("objects.svc1"))->getId() == pObj->getId());
		REQUIRE(queryService("svc1")->getId() == pObj->getId());
		REQUIRE(vSvcInfo == Dictionary({ {"finalConstruct", true}, {"id", pObj->getId()} }));

		// start
		startService("svc1");
		REQUIRE(vSvcInfo == Dictionary({ {"finalConstruct", true}, {"id", pObj->getId()}, 
			{"start", true} }));

		// start
		stopService("svc1");
		REQUIRE(vSvcInfo == Dictionary({ {"finalConstruct", true}, {"id", pObj->getId()}, 
			{"start", true}, {"stop", true} }));


		// request unknown sevice
		REQUIRE_THROWS_AS([]() {
			(void)getCatalogData("objects.unknownSvc");
		}(), error::InvalidArgument);

		REQUIRE_THROWS_AS([]() {
			(void)queryService("unknownSvc");
		}(), error::InvalidArgument);
	}
}